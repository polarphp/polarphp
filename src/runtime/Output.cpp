// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/25.

#ifndef PHP_OUTPUT_DEBUG
# define PHP_OUTPUT_DEBUG 0
#endif
#ifndef PHP_OUTPUT_NOINLINE
# define PHP_OUTPUT_NOINLINE 0
#endif

#include "polarphp/runtime/Output.h"
#include "polarphp/runtime/RtDefs.h"
#include "polarphp/runtime/ExecEnv.h"

namespace polar {
namespace runtime {

ZEND_DECLARE_MODULE_GLOBALS(output)
const char php_output_default_handler_name[sizeof("default output handler")] = "default output handler";
const char php_output_devnull_handler_name[sizeof("null output handler")] = "null output handler";

static HashTable php_output_handler_aliases;
static HashTable php_output_handler_conflicts;
static HashTable php_output_handler_reverse_conflicts;

namespace {

inline int php_output_lock_error(int op);
inline void php_output_op(int op, const char *str, size_t len);

inline PhpOutputHandler *php_output_handler_init(zend_string *name, size_t chunk_size, int flags);
inline PhpOutputHandlerStatusType php_output_handler_op(PhpOutputHandler *handler, PhpOutputContext *context);
inline int php_output_handler_append(PhpOutputHandler *handler, const PhpOutputBuffer *buf);
inline zval *php_output_handler_status(PhpOutputHandler *handler, zval *entry);

inline PhpOutputContext *php_output_context_init(PhpOutputContext *context, int op);
inline void php_output_context_reset(PhpOutputContext *context);
inline void php_output_context_swap(PhpOutputContext *context);
inline void php_output_context_dtor(PhpOutputContext *context);

inline int php_output_stack_pop(int flags);

int php_output_stack_apply_op(void *h, void *c);
int php_output_stack_apply_clean(void *h, void *c);

///
/// TODO review functions
///
POLAR_DECL_UNUSED int php_output_stack_apply_list(void *h, void *z);
POLAR_DECL_UNUSED int php_output_stack_apply_status(void *h, void *z);

bool php_output_handler_compat_func(void **handler_context, PhpOutputContext *output_context);
bool php_output_handler_default_func(void **handler_context, PhpOutputContext *output_context);
bool php_output_handler_devnull_func(void **handler_context, PhpOutputContext *output_context);

inline void php_output_init_globals(zend_output_globals *G)
{
   ZEND_TSRMLS_CACHE_UPDATE();
   memset(G, 0, sizeof(*G));
}

size_t php_output_stdout(const char *str, size_t str_len)
{
   fwrite(str, 1, str_len, stdout);
   return str_len;
}

size_t php_output_stderr(const char *str, size_t str_len)
{
   fwrite(str, 1, str_len, stderr);
   /* See http://support.microsoft.com/kb/190351 */
#ifdef POLAR_OS_WIN32
   fflush(stderr);
#endif
   return str_len;
}

size_t (*php_output_direct)(const char *str, size_t str_len) = php_output_stderr;

void reverse_conflict_dtor(zval *zv)
{
   HashTable *ht = reinterpret_cast<HashTable *>(Z_PTR_P(zv));
   zend_hash_destroy(ht);
}

} // anonymous namespace

void php_output_startup()
{
   ZEND_INIT_MODULE_GLOBALS(output, php_output_init_globals, nullptr);
   zend_hash_init(&php_output_handler_aliases, 8, nullptr, nullptr, 1);
   zend_hash_init(&php_output_handler_conflicts, 8, nullptr, nullptr, 1);
   zend_hash_init(&php_output_handler_reverse_conflicts, 8, nullptr, reverse_conflict_dtor, 1);
   php_output_direct = php_output_stdout;
}

void php_output_shutdown()
{
   php_output_direct = php_output_stderr;
   zend_hash_destroy(&php_output_handler_aliases);
   zend_hash_destroy(&php_output_handler_conflicts);
   zend_hash_destroy(&php_output_handler_reverse_conflicts);
}

bool php_output_activate()
{
   memset((*(reinterpret_cast<void ***>(ZEND_TSRMLS_CACHE)))[TSRM_UNSHUFFLE_RSRC_ID(output_globals_id)], 0, sizeof(zend_output_globals));
   zend_stack_init(&OG(handlers), sizeof(PhpOutputHandler *));
   OG(flags) |= PHP_OUTPUT_ACTIVATED;

   return true;
}

void php_output_deactivate()
{
   PhpOutputHandler **handler = nullptr;

   if ((OG(flags) & PHP_OUTPUT_ACTIVATED)) {
      OG(flags) ^= PHP_OUTPUT_ACTIVATED;
      OG(active) = nullptr;
      OG(running) = nullptr;
      /* release all output handlers */
      if (OG(handlers).elements) {
         while ((handler = reinterpret_cast<PhpOutputHandler **>(zend_stack_top(&OG(handlers))))) {
            php_output_handler_free(handler);
            zend_stack_del_top(&OG(handlers));
         }
      }
      zend_stack_destroy(&OG(handlers));
   }
}

void php_output_register_constants()
{
   //   REGISTER_MAIN_LONG_CONSTANT("PHP_OUTPUT_HANDLER_START", PHP_OUTPUT_HANDLER_START, CONST_CS | CONST_PERSISTENT);
   //   REGISTER_MAIN_LONG_CONSTANT("PHP_OUTPUT_HANDLER_WRITE", PHP_OUTPUT_HANDLER_WRITE, CONST_CS | CONST_PERSISTENT);
   //   REGISTER_MAIN_LONG_CONSTANT("PHP_OUTPUT_HANDLER_FLUSH", PHP_OUTPUT_HANDLER_FLUSH, CONST_CS | CONST_PERSISTENT);
   //   REGISTER_MAIN_LONG_CONSTANT("PHP_OUTPUT_HANDLER_CLEAN", PHP_OUTPUT_HANDLER_CLEAN, CONST_CS | CONST_PERSISTENT);
   //   REGISTER_MAIN_LONG_CONSTANT("PHP_OUTPUT_HANDLER_FINAL", PHP_OUTPUT_HANDLER_FINAL, CONST_CS | CONST_PERSISTENT);
   //   REGISTER_MAIN_LONG_CONSTANT("PHP_OUTPUT_HANDLER_CONT", PHP_OUTPUT_HANDLER_WRITE, CONST_CS | CONST_PERSISTENT);
   //   REGISTER_MAIN_LONG_CONSTANT("PHP_OUTPUT_HANDLER_END", PHP_OUTPUT_HANDLER_FINAL, CONST_CS | CONST_PERSISTENT);

   //   REGISTER_MAIN_LONG_CONSTANT("PHP_OUTPUT_HANDLER_CLEANABLE", PHP_OUTPUT_HANDLER_CLEANABLE, CONST_CS | CONST_PERSISTENT);
   //   REGISTER_MAIN_LONG_CONSTANT("PHP_OUTPUT_HANDLER_FLUSHABLE", PHP_OUTPUT_HANDLER_FLUSHABLE, CONST_CS | CONST_PERSISTENT);
   //   REGISTER_MAIN_LONG_CONSTANT("PHP_OUTPUT_HANDLER_REMOVABLE", PHP_OUTPUT_HANDLER_REMOVABLE, CONST_CS | CONST_PERSISTENT);
   //   REGISTER_MAIN_LONG_CONSTANT("PHP_OUTPUT_HANDLER_STDFLAGS", PHP_OUTPUT_HANDLER_STDFLAGS, CONST_CS | CONST_PERSISTENT);
   //   REGISTER_MAIN_LONG_CONSTANT("PHP_OUTPUT_HANDLER_STARTED", PHP_OUTPUT_HANDLER_STARTED, CONST_CS | CONST_PERSISTENT);
   //   REGISTER_MAIN_LONG_CONSTANT("PHP_OUTPUT_HANDLER_DISABLED", PHP_OUTPUT_HANDLER_DISABLED, CONST_CS | CONST_PERSISTENT);
}

void php_output_set_status(int status)
{
   OG(flags) = (OG(flags) & ~0xf) | (status & 0xf);
}

int php_output_get_status(void)
{
   return (
            OG(flags)
            |	(OG(active) ? PHP_OUTPUT_ACTIVE : 0)
            |	(OG(running)? PHP_OUTPUT_LOCKED : 0)
            ) & 0xff;
}

size_t php_output_write_unbuffered(const char *str, size_t len)
{
      if (OG(flags) & PHP_OUTPUT_ACTIVATED) {
         return retrieve_global_execenv().unbufferWrite(str, len);
      }
      return php_output_direct(str, len);
}

size_t php_output_write(const char *str, size_t len)
{
   if (OG(flags) & PHP_OUTPUT_ACTIVATED) {
      php_output_op(PHP_OUTPUT_HANDLER_WRITE, str, len);
      return len;
   }
   if (OG(flags) & PHP_OUTPUT_DISABLED) {
      return 0;
   }
   return php_output_direct(str, len);
}

bool php_output_flush()
{
   PhpOutputContext context;

   if (OG(active) && (OG(active)->flags & PHP_OUTPUT_HANDLER_FLUSHABLE)) {
      php_output_context_init(&context, PHP_OUTPUT_HANDLER_FLUSH);
      php_output_handler_op(OG(active), &context);
      if (context.out.data && context.out.used) {
         zend_stack_del_top(&OG(handlers));
         php_output_write(context.out.data, context.out.used);
         zend_stack_push(&OG(handlers), &OG(active));
      }
      php_output_context_dtor(&context);
      return true;
   }
   return false;
}

void php_output_flush_all()
{
   if (OG(active)) {
      php_output_op(PHP_OUTPUT_HANDLER_FLUSH, nullptr, 0);
   }
}

bool php_output_clean(void)
{
   PhpOutputContext context;
   if (OG(active) && (OG(active)->flags & PHP_OUTPUT_HANDLER_CLEANABLE)) {
      php_output_context_init(&context, PHP_OUTPUT_HANDLER_CLEAN);
      php_output_handler_op(OG(active), &context);
      php_output_context_dtor(&context);
      return true;
   }
   return false;
}

void php_output_clean_all()
{
   PhpOutputContext context;
   if (OG(active)) {
      php_output_context_init(&context, PHP_OUTPUT_HANDLER_CLEAN);
      zend_stack_apply_with_argument(&OG(handlers), ZEND_STACK_APPLY_TOPDOWN, php_output_stack_apply_clean, &context);
   }
}

bool php_output_end()
{
   if (php_output_stack_pop(PHP_OUTPUT_POP_TRY)) {
      return true;
   }
   return false;
}

void php_output_end_all()
{
   while (OG(active) && php_output_stack_pop(PHP_OUTPUT_POP_FORCE));
}

bool php_output_discard()
{
   if (php_output_stack_pop(PHP_OUTPUT_POP_DISCARD|PHP_OUTPUT_POP_TRY)) {
      return true;
   }
   return false;
}

void php_output_discard_all()
{
   while (OG(active)) {
      php_output_stack_pop(PHP_OUTPUT_POP_DISCARD|PHP_OUTPUT_POP_FORCE);
   }
}

int php_output_get_level(void)
{
   return OG(active) ? zend_stack_count(&OG(handlers)) : 0;
}

bool php_output_get_contents(zval *p)
{
   if (OG(active)) {
      ZVAL_STRINGL(p, OG(active)->buffer.data, OG(active)->buffer.used);
      return true;
   } else {
      ZVAL_NULL(p);
      return false;
   }
}

bool php_output_get_length(zval *p)
{
   if (OG(active)) {
      ZVAL_LONG(p, OG(active)->buffer.used);
      return true;
   } else {
      ZVAL_NULL(p);
      return false;
   }
}

PhpOutputHandler *php_output_get_active_handler(void)
{
   return OG(active);
}

bool php_output_start_default(void)
{
   PhpOutputHandler *handler;

   handler = php_output_handler_create_internal(ZEND_STRL(php_output_default_handler_name), php_output_handler_default_func, 0, PHP_OUTPUT_HANDLER_STDFLAGS);
   if (php_output_handler_start(handler)) {
      return true;
   }
   php_output_handler_free(&handler);
   return false;
}

bool php_output_start_devnull(void)
{
   PhpOutputHandler *handler;

   handler = php_output_handler_create_internal(ZEND_STRL(php_output_devnull_handler_name), php_output_handler_devnull_func, PHP_OUTPUT_HANDLER_DEFAULT_SIZE, 0);
   if (php_output_handler_start(handler)) {
      return true;
   }
   php_output_handler_free(&handler);
   return false;
}

bool php_output_start_user(zval *output_handler, size_t chunk_size, int flags)
{
   PhpOutputHandler *handler;

   if (output_handler) {
      handler = php_output_handler_create_user(output_handler, chunk_size, flags);
   } else {
      handler = php_output_handler_create_internal(ZEND_STRL(php_output_default_handler_name), php_output_handler_default_func, chunk_size, flags);
   }
   if (php_output_handler_start(handler)) {
      return true;
   }
   php_output_handler_free(&handler);
   return false;
}

bool php_output_start_internal(const char *name, size_t name_len,
                               PhpOutputHandlerFuncType output_handler,
                               size_t chunk_size, int flags)
{
   PhpOutputHandler *handler;

   handler = php_output_handler_create_internal(name, name_len, php_output_handler_compat_func, chunk_size, flags);
   php_output_handler_set_context(handler, reinterpret_cast<void *>(output_handler), nullptr);
   if (php_output_handler_start(handler)) {
      return true;
   }
   php_output_handler_free(&handler);
   return false;
}

PhpOutputHandler *php_output_handler_create_user(zval *output_handler, size_t chunk_size, int flags)
{
   zend_string *handler_name = nullptr;
   char *error = nullptr;
   PhpOutputHandler *handler = nullptr;
   PhpOutputHandlerAliasCtorType alias = nullptr;
   PhpOutputHandlerUserFuncType *user = nullptr;

   switch (Z_TYPE_P(output_handler)) {
   case IS_NULL:
      handler = php_output_handler_create_internal(ZEND_STRL(php_output_default_handler_name), php_output_handler_default_func, chunk_size, flags);
      break;
   case IS_STRING:
      if (Z_STRLEN_P(output_handler) && (alias = php_output_handler_alias(Z_STRVAL_P(output_handler), Z_STRLEN_P(output_handler)))) {
         handler = alias(Z_STRVAL_P(output_handler), Z_STRLEN_P(output_handler), chunk_size, flags);
         break;
      }
      POLAR_FALLTHROUGH;
   default:
      user = reinterpret_cast<PhpOutputHandlerUserFuncType *>(ecalloc(1, sizeof(PhpOutputHandlerUserFuncType)));
      if (zend_fcall_info_init(output_handler, 0, &user->fci, &user->fcc, &handler_name, &error)) {
         handler = php_output_handler_init(handler_name, chunk_size, (flags & ~0xf) | PHP_OUTPUT_HANDLER_USER);
         ZVAL_COPY(&user->zoh, output_handler);
         handler->func.user = user;
      } else {
         efree(user);
      }
      if (error) {
         php_error_docref("ref.outcontrol", E_WARNING, "%s", error);
         efree(error);
      }
      if (handler_name) {
         zend_string_release_ex(handler_name, 0);
      }
   }

   return handler;
}

PhpOutputHandler *php_output_handler_create_internal(const char *name, size_t name_len,
                                                     PhpOutputHandlerContextFuncType output_handler,
                                                     size_t chunk_size, int flags)
{
   PhpOutputHandler *handler;
   zend_string *str = zend_string_init(name, name_len, 0);

   handler = php_output_handler_init(str, chunk_size, (flags & ~0xf) | PHP_OUTPUT_HANDLER_INTERNAL);
   handler->func.internal = output_handler;
   zend_string_release_ex(str, 0);

   return handler;
}

void php_output_handler_set_context(PhpOutputHandler *handler, void *opaq, void (*dtor)(void*))
{
   if (handler->dtor && handler->opaq) {
      handler->dtor(handler->opaq);
   }
   handler->dtor = dtor;
   handler->opaq = opaq;
}

bool php_output_handler_start(PhpOutputHandler *handler)
{
   HashTable *rconflicts;
   void *conflict;

   if (php_output_lock_error(PHP_OUTPUT_HANDLER_START) || !handler) {
      return false;
   }
   if (nullptr != (conflict = zend_hash_find_ptr(&php_output_handler_conflicts, handler->name))) {
      PhpOutputHandlerConflictCheckType conflictFunc = reinterpret_cast<PhpOutputHandlerConflictCheckType>(conflict);
      if (!conflictFunc(ZSTR_VAL(handler->name), ZSTR_LEN(handler->name))) {
         return false;
      }
   }
   if (nullptr != (rconflicts = reinterpret_cast<HashTable *>(zend_hash_find_ptr(&php_output_handler_reverse_conflicts, handler->name)))) {
      ZEND_HASH_FOREACH_PTR(rconflicts, conflict) {
         PhpOutputHandlerConflictCheckType conflictFunc = reinterpret_cast<PhpOutputHandlerConflictCheckType>(conflict);
         if (!conflictFunc(ZSTR_VAL(handler->name), ZSTR_LEN(handler->name))) {
            return false;
         }
      } ZEND_HASH_FOREACH_END();
   }
   /* zend_stack_push returns stack level */
   handler->level = zend_stack_push(&OG(handlers), &handler);
   OG(active) = handler;
   return true;
}

int php_output_handler_started(const char *name, size_t name_len)
{
   PhpOutputHandler **handlers;
   int i, count = php_output_get_level();

   if (count) {
      handlers = (PhpOutputHandler **) zend_stack_base(&OG(handlers));

      for (i = 0; i < count; ++i) {
         if (name_len == ZSTR_LEN(handlers[i]->name) && !memcmp(ZSTR_VAL(handlers[i]->name), name, name_len)) {
            return 1;
         }
      }
   }

   return 0;
}

int php_output_handler_conflict(const char *handler_new, size_t handler_new_len,
                                const char *handler_set, size_t handler_set_len)
{
   if (php_output_handler_started(handler_set, handler_set_len)) {
      if (handler_new_len != handler_set_len || memcmp(handler_new, handler_set, handler_set_len)) {
         php_error_docref("ref.outcontrol", E_WARNING, "output handler '%s' conflicts with '%s'", handler_new, handler_set);
      } else {
         php_error_docref("ref.outcontrol", E_WARNING, "output handler '%s' cannot be used twice", handler_new);
      }
      return 1;
   }
   return 0;
}

bool php_output_handler_conflict_register(const char *name, size_t name_len,
                                          PhpOutputHandlerConflictCheckType check_func)
{
   zend_string *str;

   if (!EG(current_module)) {
      zend_error(E_ERROR, "Cannot register an output handler conflict outside of MINIT");
      return false;
   }
   str = zend_string_init_interned(name, name_len, 1);
   zend_hash_update_ptr(&php_output_handler_conflicts, str, reinterpret_cast<void *>(check_func));
   zend_string_release_ex(str, 1);
   return true;
}

bool php_output_handler_reverse_conflict_register(const char *name, size_t name_len,
                                                  PhpOutputHandlerConflictCheckType check_func)
{
   HashTable rev, *rev_ptr = nullptr;

   if (!EG(current_module)) {
      zend_error(E_ERROR, "Cannot register a reverse output handler conflict outside of MINIT");
      return false;
   }

   if (nullptr != (rev_ptr = reinterpret_cast<HashTable *>(zend_hash_str_find_ptr(&php_output_handler_reverse_conflicts, name, name_len)))) {
      return zend_hash_next_index_insert_ptr(rev_ptr, reinterpret_cast<void *>(check_func)) ? true : false;
   } else {
      zend_string *str;

      zend_hash_init(&rev, 8, nullptr, nullptr, 1);
      if (nullptr == zend_hash_next_index_insert_ptr(&rev, reinterpret_cast<void *>(check_func))) {
         zend_hash_destroy(&rev);
         return false;
      }
      str = zend_string_init_interned(name, name_len, 1);
      zend_hash_update_mem(&php_output_handler_reverse_conflicts, str, &rev, sizeof(HashTable));
      zend_string_release_ex(str, 1);
      return true;
   }
}

PhpOutputHandlerAliasCtorType php_output_handler_alias(const char *name, size_t name_len)
{
   return reinterpret_cast<PhpOutputHandlerAliasCtorType>(zend_hash_str_find_ptr(&php_output_handler_aliases, name, name_len));
}

bool php_output_handler_alias_register(const char *name, size_t name_len,
                                       PhpOutputHandlerAliasCtorType func)
{
   zend_string *str;

   if (!EG(current_module)) {
      zend_error(E_ERROR, "Cannot register an output handler alias outside of MINIT");
      return false;
   }
   str = zend_string_init_interned(name, name_len, 1);
   zend_hash_update_ptr(&php_output_handler_aliases, str, reinterpret_cast<void *>(func));
   zend_string_release_ex(str, 1);
   return true;
}

int php_output_handler_hook(PhpOutputHandlerHookType type, void *arg)
{
   if (OG(running)) {
      switch (type) {
      case PHP_OUTPUT_HANDLER_HOOK_GET_OPAQ:
         *(void ***) arg = &OG(running)->opaq;
         return SUCCESS;
      case PHP_OUTPUT_HANDLER_HOOK_GET_FLAGS:
         *(int *) arg = OG(running)->flags;
         return SUCCESS;
      case PHP_OUTPUT_HANDLER_HOOK_GET_LEVEL:
         *(int *) arg = OG(running)->level;
         return SUCCESS;
      case PHP_OUTPUT_HANDLER_HOOK_IMMUTABLE:
         OG(running)->flags &= ~(PHP_OUTPUT_HANDLER_REMOVABLE|PHP_OUTPUT_HANDLER_CLEANABLE);
         return SUCCESS;
      case PHP_OUTPUT_HANDLER_HOOK_DISABLE:
         OG(running)->flags |= PHP_OUTPUT_HANDLER_DISABLED;
         return SUCCESS;
      default:
         break;
      }
   }
   return FAILURE;
}

void php_output_handler_dtor(PhpOutputHandler *handler)
{
   if (handler->name) {
      zend_string_release_ex(handler->name, 0);
   }
   if (handler->buffer.data) {
      efree(handler->buffer.data);
   }
   if (handler->flags & PHP_OUTPUT_HANDLER_USER) {
      zval_ptr_dtor(&handler->func.user->zoh);
      efree(handler->func.user);
   }
   if (handler->dtor && handler->opaq) {
      handler->dtor(handler->opaq);
   }
   memset(handler, 0, sizeof(*handler));
}

void php_output_handler_free(PhpOutputHandler **h)
{
   if (*h) {
      php_output_handler_dtor(*h);
      efree(*h);
      *h = nullptr;
   }
}

void php_output_set_implicit_flush(int flush)
{
   if (flush) {
      OG(flags) |= PHP_OUTPUT_IMPLICITFLUSH;
   } else {
      OG(flags) &= ~PHP_OUTPUT_IMPLICITFLUSH;
   }
}

const char *php_output_get_start_filename(void)
{
   return OG(output_start_filename);
}

int php_output_get_start_lineno(void)
{
   return OG(output_start_lineno);
}

namespace {

inline int php_output_lock_error(int op)
{
   /* if there's no ob active, ob has been stopped */
   if (op && OG(active) && OG(running)) {
      /* fatal error */
      php_output_deactivate();
      php_error_docref("ref.outcontrol", E_ERROR, "Cannot use output buffering in output buffering display handlers");
      return 1;
   }
   return 0;
}

inline PhpOutputContext *php_output_context_init(PhpOutputContext *context, int op)
{
   if (!context) {
      context = reinterpret_cast<PhpOutputContext *>(emalloc(sizeof(PhpOutputContext)));
   }
   memset(context, 0, sizeof(PhpOutputContext));
   context->op = op;
   return context;
}

inline void php_output_context_reset(PhpOutputContext *context)
{
   int op = context->op;
   php_output_context_dtor(context);
   memset(context, 0, sizeof(PhpOutputContext));
   context->op = op;
}

inline void php_output_context_feed(PhpOutputContext *context, char *data,
                                    size_t size, size_t used, zend_bool free)
{
   if (context->in.free && context->in.data) {
      efree(context->in.data);
   }
   context->in.data = data;
   context->in.used = used;
   context->in.free = free;
   context->in.size = size;
}

inline void php_output_context_swap(PhpOutputContext *context)
{
   if (context->in.free && context->in.data) {
      efree(context->in.data);
   }
   context->in.data = context->out.data;
   context->in.used = context->out.used;
   context->in.free = context->out.free;
   context->in.size = context->out.size;
   context->out.data = nullptr;
   context->out.used = 0;
   context->out.free = 0;
   context->out.size = 0;
}

inline void php_output_context_pass(PhpOutputContext *context)
{
   context->out.data = context->in.data;
   context->out.used = context->in.used;
   context->out.size = context->in.size;
   context->out.free = context->in.free;
   context->in.data = nullptr;
   context->in.used = 0;
   context->in.free = 0;
   context->in.size = 0;
}

inline void php_output_context_dtor(PhpOutputContext *context)
{
   if (context->in.free && context->in.data) {
      efree(context->in.data);
      context->in.data = nullptr;
   }
   if (context->out.free && context->out.data) {
      efree(context->out.data);
      context->out.data = nullptr;
   }
}

inline PhpOutputHandler *php_output_handler_init(zend_string *name, size_t chunk_size, int flags)
{
   PhpOutputHandler *handler;

   handler = reinterpret_cast<PhpOutputHandler *>(ecalloc(1, sizeof(PhpOutputHandler)));
   handler->name = zend_string_copy(name);
   handler->size = chunk_size;
   handler->flags = flags;
   handler->buffer.size = PHP_OUTPUT_HANDLER_INITBUF_SIZE(chunk_size);
   handler->buffer.data = reinterpret_cast<char *>(emalloc(handler->buffer.size));

   return handler;
}

inline int php_output_handler_append(PhpOutputHandler *handler, const PhpOutputBuffer *buf)
{
   if (buf->used) {
      OG(flags) |= PHP_OUTPUT_WRITTEN;
      /* store it away */
      if ((handler->buffer.size - handler->buffer.used) <= buf->used) {
         size_t grow_int = PHP_OUTPUT_HANDLER_INITBUF_SIZE(handler->size);
         size_t grow_buf = PHP_OUTPUT_HANDLER_INITBUF_SIZE(buf->used - (handler->buffer.size - handler->buffer.used));
         size_t grow_max = MAX(grow_int, grow_buf);

         handler->buffer.data = reinterpret_cast<char *>(erealloc(handler->buffer.data, handler->buffer.size + grow_max));
         handler->buffer.size += grow_max;
      }
      memcpy(handler->buffer.data + handler->buffer.used, buf->data, buf->used);
      handler->buffer.used += buf->used;

      /* chunked buffering */
      if (handler->size && (handler->buffer.used >= handler->size)) {
         /* store away errors and/or any intermediate output */
         return OG(running) ? 1 : 0;
      }
   }
   return 1;
}

inline PhpOutputHandlerStatusType php_output_handler_op(PhpOutputHandler *handler, PhpOutputContext *context)
{
   PhpOutputHandlerStatusType status;
   int original_op = context->op;

#if PHP_OUTPUT_DEBUG
   fprintf(stderr, ">>> op(%d, "
                   "handler=%p, "
                   "name=%s, "
                   "flags=%d, "
                   "buffer.data=%s, "
                   "buffer.used=%zu, "
                   "buffer.size=%zu, "
                   "in.data=%s, "
                   "in.used=%zu)\n",
           context->op,
           handler,
           handler->name,
           handler->flags,
           handler->buffer.used?handler->buffer.data:"",
           handler->buffer.used,
           handler->buffer.size,
           context->in.used?context->in.data:"",
           context->in.used
           );
#endif

   if (php_output_lock_error(context->op)) {
      /* fatal error */
      return PHP_OUTPUT_HANDLER_FAILURE;
   }

   /* storable? */
   if (php_output_handler_append(handler, &context->in) && !context->op) {
      context->op = original_op;
      return PHP_OUTPUT_HANDLER_NO_DATA;
   } else {
      /* need to start? */
      if (!(handler->flags & PHP_OUTPUT_HANDLER_STARTED)) {
         context->op |= PHP_OUTPUT_HANDLER_START;
      }

      OG(running) = handler;
      if (handler->flags & PHP_OUTPUT_HANDLER_USER) {
         zval retval, ob_data, ob_mode;

         ZVAL_STRINGL(&ob_data, handler->buffer.data, handler->buffer.used);
         ZVAL_LONG(&ob_mode, (zend_long) context->op);
         zend_fcall_info_argn(&handler->func.user->fci, 2, &ob_data, &ob_mode);
         zval_ptr_dtor(&ob_data);

#define PHP_OUTPUT_USER_SUCCESS(retval) ((Z_TYPE(retval) != IS_UNDEF) && !(Z_TYPE(retval) == IS_FALSE))
         if (SUCCESS == zend_fcall_info_call(&handler->func.user->fci, &handler->func.user->fcc, &retval, nullptr) && PHP_OUTPUT_USER_SUCCESS(retval)) {
            /* user handler may have returned TRUE */
            status = PHP_OUTPUT_HANDLER_NO_DATA;
            if (Z_TYPE(retval) != IS_FALSE && Z_TYPE(retval) != IS_TRUE) {
               convert_to_string_ex(&retval);
               if (Z_STRLEN(retval)) {
                  context->out.data = estrndup(Z_STRVAL(retval), Z_STRLEN(retval));
                  context->out.used = Z_STRLEN(retval);
                  context->out.free = 1;
                  status = PHP_OUTPUT_HANDLER_SUCCESS;
               }
            }
         } else {
            /* call failed, pass internal buffer along */
            status = PHP_OUTPUT_HANDLER_FAILURE;
         }

         zend_fcall_info_argn(&handler->func.user->fci, 0);
         zval_ptr_dtor(&retval);

      } else {

         php_output_context_feed(context, handler->buffer.data, handler->buffer.size, handler->buffer.used, 0);

         if (SUCCESS == handler->func.internal(&handler->opaq, context)) {
            if (context->out.used) {
               status = PHP_OUTPUT_HANDLER_SUCCESS;
            } else {
               status = PHP_OUTPUT_HANDLER_NO_DATA;
            }
         } else {
            status = PHP_OUTPUT_HANDLER_FAILURE;
         }
      }
      handler->flags |= PHP_OUTPUT_HANDLER_STARTED;
      OG(running) = nullptr;
   }

   switch (status) {
   case PHP_OUTPUT_HANDLER_FAILURE:
      /* disable this handler */
      handler->flags |= PHP_OUTPUT_HANDLER_DISABLED;
      /* discard any output */
      if (context->out.data && context->out.free) {
         efree(context->out.data);
      }
      /* returns handlers buffer */
      context->out.data = handler->buffer.data;
      context->out.used = handler->buffer.used;
      context->out.free = 1;
      handler->buffer.data = nullptr;
      handler->buffer.used = 0;
      handler->buffer.size = 0;
      break;
   case PHP_OUTPUT_HANDLER_NO_DATA:
      /* handler ate all */
      php_output_context_reset(context);
      POLAR_FALLTHROUGH;
      /* no break */
   case PHP_OUTPUT_HANDLER_SUCCESS:
      /* no more buffered data */
      handler->buffer.used = 0;
      handler->flags |= PHP_OUTPUT_HANDLER_PROCESSED;
      break;
   }

   context->op = original_op;
   return status;
}

inline void php_output_op(int op, const char *str, size_t len)
{
   PhpOutputContext context;
   PhpOutputHandler **active;
   int obh_cnt;

   if (php_output_lock_error(op)) {
      return;
   }
   php_output_context_init(&context, op);
   /*
       * broken up for better performance:
       *  - apply op to the one active handler; note that OG(active) might be popped off the stack on a flush
       *  - or apply op to the handler stack
       */
   if (OG(active) && (obh_cnt = zend_stack_count(&OG(handlers)))) {
      context.in.data = const_cast<char *>(reinterpret_cast<const char *>(str));
      context.in.used = len;

      if (obh_cnt > 1) {
         zend_stack_apply_with_argument(&OG(handlers), ZEND_STACK_APPLY_TOPDOWN, php_output_stack_apply_op, &context);
      } else if ((active = reinterpret_cast<PhpOutputHandler **>(zend_stack_top(&OG(handlers)))) && (!((*active)->flags & PHP_OUTPUT_HANDLER_DISABLED))) {
         php_output_handler_op(*active, &context);
      } else {
         php_output_context_pass(&context);
      }
   } else {
      context.out.data = const_cast<char *>(reinterpret_cast<const char *>(str));
      context.out.used = len;
   }

   if (context.out.data && context.out.used) {
      if (!(OG(flags) & PHP_OUTPUT_DISABLED)) {
#if PHP_OUTPUT_DEBUG
         fprintf(stderr, "::: sapi_write('%s', %zu)\n", context.out.data, context.out.used);
#endif
         ExecEnv &execEnv = retrieve_global_execenv();
         execEnv.unbufferWrite(context.out.data, context.out.used);
         if (OG(flags) & PHP_OUTPUT_IMPLICITFLUSH) {
            cli_flush();
         }
         OG(flags) |= PHP_OUTPUT_SENT;
      }
   }
   php_output_context_dtor(&context);
}

int php_output_stack_apply_op(void *h, void *c)
{
   int was_disabled;
   PhpOutputHandlerStatusType status;
   PhpOutputHandler *handler = *(PhpOutputHandler **) h;
   PhpOutputContext *context = (PhpOutputContext *) c;

   if ((was_disabled = (handler->flags & PHP_OUTPUT_HANDLER_DISABLED))) {
      status = PHP_OUTPUT_HANDLER_FAILURE;
   } else {
      status = php_output_handler_op(handler, context);
   }

   /*
    * handler ate all => break
    * handler returned data or failed resp. is disabled => continue
    */
   switch (status) {
   case PHP_OUTPUT_HANDLER_NO_DATA:
      return 1;

   case PHP_OUTPUT_HANDLER_SUCCESS:
      /* swap contexts buffers, unless this is the last handler in the stack */
      if (handler->level) {
         php_output_context_swap(context);
      }
      return 0;

   case PHP_OUTPUT_HANDLER_FAILURE:
   default:
      if (was_disabled) {
         /* pass input along, if it's the last handler in the stack */
         if (!handler->level) {
            php_output_context_pass(context);
         }
      } else {
         /* swap buffers, unless this is the last handler */
         if (handler->level) {
            php_output_context_swap(context);
         }
      }
      return 0;
   }
}

int php_output_stack_apply_clean(void *h, void *c)
{
   PhpOutputHandler *handler = *(PhpOutputHandler **) h;
   PhpOutputContext *context = (PhpOutputContext *) c;

   handler->buffer.used = 0;
   php_output_handler_op(handler, context);
   php_output_context_reset(context);
   return 0;
}

int php_output_stack_apply_list(void *h, void *z)
{
   PhpOutputHandler *handler = *(PhpOutputHandler **) h;
   zval *array = (zval *) z;
   add_next_index_str(array, zend_string_copy(handler->name));
   return 0;
}

int php_output_stack_apply_status(void *h, void *z)
{
   PhpOutputHandler *handler = *(PhpOutputHandler **) h;
   zval arr, *array = (zval *) z;

   add_next_index_zval(array, php_output_handler_status(handler, &arr));

   return 0;
}

inline zval *php_output_handler_status(PhpOutputHandler *handler, zval *entry)
{
   ZEND_ASSERT(entry != nullptr);

   array_init(entry);
   add_assoc_str(entry, "name", zend_string_copy(handler->name));
   add_assoc_long(entry, "type", (zend_long) (handler->flags & 0xf));
   add_assoc_long(entry, "flags", (zend_long) handler->flags);
   add_assoc_long(entry, "level", (zend_long) handler->level);
   add_assoc_long(entry, "chunk_size", (zend_long) handler->size);
   add_assoc_long(entry, "buffer_size", (zend_long) handler->buffer.size);
   add_assoc_long(entry, "buffer_used", (zend_long) handler->buffer.used);

   return entry;
}

inline int php_output_stack_pop(int flags)
{
   PhpOutputContext context;
   PhpOutputHandler **current, *orphan = OG(active);

   if (!orphan) {
      if (!(flags & PHP_OUTPUT_POP_SILENT)) {
         php_error_docref("ref.outcontrol", E_NOTICE, "failed to %s buffer. No buffer to %s", (flags&PHP_OUTPUT_POP_DISCARD)?"discard":"send", (flags&PHP_OUTPUT_POP_DISCARD)?"discard":"send");
      }
      return 0;
   } else if (!(flags & PHP_OUTPUT_POP_FORCE) && !(orphan->flags & PHP_OUTPUT_HANDLER_REMOVABLE)) {
      if (!(flags & PHP_OUTPUT_POP_SILENT)) {
         php_error_docref("ref.outcontrol", E_NOTICE, "failed to %s buffer of %s (%d)", (flags&PHP_OUTPUT_POP_DISCARD)?"discard":"send", ZSTR_VAL(orphan->name), orphan->level);
      }
      return 0;
   } else {
      php_output_context_init(&context, PHP_OUTPUT_HANDLER_FINAL);

      /* don't run the output handler if it's disabled */
      if (!(orphan->flags & PHP_OUTPUT_HANDLER_DISABLED)) {
         /* didn't it start yet? */
         if (!(orphan->flags & PHP_OUTPUT_HANDLER_STARTED)) {
            context.op |= PHP_OUTPUT_HANDLER_START;
         }
         /* signal that we're cleaning up */
         if (flags & PHP_OUTPUT_POP_DISCARD) {
            context.op |= PHP_OUTPUT_HANDLER_CLEAN;
         }
         php_output_handler_op(orphan, &context);
      }

      /* pop it off the stack */
      zend_stack_del_top(&OG(handlers));
      if ((current = reinterpret_cast<PhpOutputHandler **>(zend_stack_top(&OG(handlers))))) {
         OG(active) = *current;
      } else {
         OG(active) = nullptr;
      }

      /* pass output along */
      if (context.out.data && context.out.used && !(flags & PHP_OUTPUT_POP_DISCARD)) {
         php_output_write(context.out.data, context.out.used);
      }

      /* destroy the handler (after write!) */
      php_output_handler_free(&orphan);
      php_output_context_dtor(&context);

      return 1;
   }
}

bool php_output_handler_compat_func(void **handler_context, PhpOutputContext *output_context)
{
   PhpOutputHandlerFuncType func = *(PhpOutputHandlerFuncType *) handler_context;

   if (func) {
      char *out_str = nullptr;
      size_t out_len = 0;

      func(output_context->in.data, output_context->in.used, &out_str, &out_len, output_context->op);

      if (out_str) {
         output_context->out.data = out_str;
         output_context->out.used = out_len;
         output_context->out.free = 1;
      } else {
         php_output_context_pass(output_context);
      }

      return true;
   }
   return false;
}

bool php_output_handler_default_func(void **handler_context, PhpOutputContext *output_context)
{
   php_output_context_pass(output_context);
   return true;
}

bool php_output_handler_devnull_func(void **handler_context, PhpOutputContext *output_context)
{
   return true;
}

} // anonymous namespace

} // runtime
} // polar
