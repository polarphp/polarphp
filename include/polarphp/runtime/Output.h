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

#ifndef POLARPHP_RUNTIME_OUTPUT_H
#define POLARPHP_RUNTIME_OUTPUT_H

#include "polarphp/runtime/internal/DepsZendVmHeaders.h"

namespace polar {
namespace runtime {

#define PHP_OUTPUT_NEWAPI 1

/* handler ops */
#define PHP_OUTPUT_HANDLER_WRITE	0x00	/* standard passthru */
#define PHP_OUTPUT_HANDLER_START	0x01	/* start */
#define PHP_OUTPUT_HANDLER_CLEAN	0x02	/* restart */
#define PHP_OUTPUT_HANDLER_FLUSH	0x04	/* pass along as much as possible */
#define PHP_OUTPUT_HANDLER_FINAL	0x08	/* finalize */
#define PHP_OUTPUT_HANDLER_CONT		PHP_OUTPUT_HANDLER_WRITE
#define PHP_OUTPUT_HANDLER_END		PHP_OUTPUT_HANDLER_FINAL

/* handler types */
#define PHP_OUTPUT_HANDLER_INTERNAL		0x0000
#define PHP_OUTPUT_HANDLER_USER			0x0001

/* handler ability flags */
#define PHP_OUTPUT_HANDLER_CLEANABLE	0x0010
#define PHP_OUTPUT_HANDLER_FLUSHABLE	0x0020
#define PHP_OUTPUT_HANDLER_REMOVABLE	0x0040
#define PHP_OUTPUT_HANDLER_STDFLAGS		0x0070

/* handler status flags */
#define PHP_OUTPUT_HANDLER_STARTED		0x1000
#define PHP_OUTPUT_HANDLER_DISABLED		0x2000
#define PHP_OUTPUT_HANDLER_PROCESSED	0x4000

/* handler op return values */
enum PhpOutputHandlerStatusType
{
   PHP_OUTPUT_HANDLER_FAILURE,
   PHP_OUTPUT_HANDLER_SUCCESS,
   PHP_OUTPUT_HANDLER_NO_DATA
};

/* php_output_stack_pop() flags */
#define PHP_OUTPUT_POP_TRY			0x000
#define PHP_OUTPUT_POP_FORCE		0x001
#define PHP_OUTPUT_POP_DISCARD		0x010
#define PHP_OUTPUT_POP_SILENT		0x100

/* real global flags */
#define PHP_OUTPUT_IMPLICITFLUSH		0x01
#define PHP_OUTPUT_DISABLED				0x02
#define PHP_OUTPUT_WRITTEN				0x04
#define PHP_OUTPUT_SENT					0x08
/* supplementary flags for php_output_get_status() */
#define PHP_OUTPUT_ACTIVE				0x10
#define PHP_OUTPUT_LOCKED				0x20
/* output layer is ready to use */
#define PHP_OUTPUT_ACTIVATED		0x100000

/* handler hooks */
enum PhpOutputHandlerHookType
{
   PHP_OUTPUT_HANDLER_HOOK_GET_OPAQ,
   PHP_OUTPUT_HANDLER_HOOK_GET_FLAGS,
   PHP_OUTPUT_HANDLER_HOOK_GET_LEVEL,
   PHP_OUTPUT_HANDLER_HOOK_IMMUTABLE,
   PHP_OUTPUT_HANDLER_HOOK_DISABLE,
   /* unused */
   PHP_OUTPUT_HANDLER_HOOK_LAST
};

#define PHP_OUTPUT_HANDLER_INITBUF_SIZE(s) \
   ( ((s) > 1) ? \
   (s) + PHP_OUTPUT_HANDLER_ALIGNTO_SIZE - ((s) % (PHP_OUTPUT_HANDLER_ALIGNTO_SIZE)) : \
   PHP_OUTPUT_HANDLER_DEFAULT_SIZE \
   )
#define PHP_OUTPUT_HANDLER_ALIGNTO_SIZE		0x1000
#define PHP_OUTPUT_HANDLER_DEFAULT_SIZE		0x4000

struct PhpOutputBuffer
{
   char *data;
   size_t size;
   size_t used;
   uint32_t free:1;
   uint32_t _reserved:31;
};

struct PhpOutputContext
{
   int op;
   PhpOutputBuffer in;
   PhpOutputBuffer out;
};

struct PhpOutputHandler;

/* old-style, stateless callback */
typedef bool (*PhpOutputHandlerFuncType)(char *output, size_t output_len, char **handled_output, size_t *handled_output_len, int mode);
/* new-style, opaque context callback */
typedef bool (*PhpOutputHandlerContextFuncType)(void **handler_context, PhpOutputContext *output_context);
/* output handler context dtor */
typedef void (*PhpOutputHandlerContextDtorType)(void *opaq);
/* conflict check callback */
typedef bool (*PhpOutputHandlerConflictCheckType)(const char *handler_name, size_t handler_name_len);
/* ctor for aliases */
typedef PhpOutputHandler *(*PhpOutputHandlerAliasCtorType)(const char *handler_name, size_t handler_name_len, size_t chunk_size, int flags);

struct PhpOutputHandlerUserFuncType
{
   zend_fcall_info fci;
   zend_fcall_info_cache fcc;
   zval zoh;
};

struct PhpOutputHandler
{
   zend_string *name;
   int flags;
   int level;
   size_t size;
   PhpOutputBuffer buffer;
   void *opaq;
   void (*dtor)(void *opaq);
   union {
      PhpOutputHandlerUserFuncType *user;
      PhpOutputHandlerContextFuncType internal;
   } func;
};

ZEND_BEGIN_MODULE_GLOBALS(output)
zend_stack handlers;
PhpOutputHandler *active;
PhpOutputHandler *running;
const char *output_start_filename;
int output_start_lineno;
int flags;
ZEND_END_MODULE_GLOBALS(output)

POLAR_DECL_EXPORT ZEND_EXTERN_MODULE_GLOBALS(output)

/* there should not be a need to use OG() from outside of output.c */
# define OG(v) ZEND_TSRMG(output_globals_id, zend_output_globals *, v)

/* convenience macros */
#define PHPWRITE(str, str_len)		php_output_write((str), (str_len))
#define PHPWRITE_H(str, str_len)	php_output_write_unbuffered((str), (str_len))

#define PUTC(c)						php_output_write((const char *) &(c), 1)
#define PUTC_H(c)					php_output_write_unbuffered((const char *) &(c), 1)

#define PUTS(str)					do {				\
   const char *__str = (str);							\
   php_output_write(__str, strlen(__str));	\
} while (0)
#define PUTS_H(str)					do {							\
   const char *__str = (str);										\
   php_output_write_unbuffered(__str, strlen(__str));	\
} while (0)


extern const char php_output_default_handler_name[sizeof("default output handler")];
extern const char php_output_devnull_handler_name[sizeof("null output handler")];

#define php_output_tearup() \
   php_output_startup(); \
   php_output_activate()
#define php_output_teardown() \
   php_output_end_all(); \
   php_output_deactivate(); \
   php_output_shutdown()

/* MINIT */
POLAR_DECL_EXPORT void php_output_startup();
/* MSHUTDOWN */
POLAR_DECL_EXPORT void php_output_shutdown();

POLAR_DECL_EXPORT void php_output_register_constants();

/* RINIT */
POLAR_DECL_EXPORT bool php_output_activate();
/* RSHUTDOWN */
POLAR_DECL_EXPORT void php_output_deactivate();

POLAR_DECL_EXPORT void php_output_set_status(int status);
POLAR_DECL_EXPORT int php_output_get_status();
POLAR_DECL_EXPORT void php_output_set_implicit_flush(int flush);
POLAR_DECL_EXPORT const char *php_output_get_start_filename();
POLAR_DECL_EXPORT int php_output_get_start_lineno();

POLAR_DECL_EXPORT size_t php_output_write_unbuffered(const char *str, size_t len);
POLAR_DECL_EXPORT size_t php_output_write(const char *str, size_t len);

POLAR_DECL_EXPORT bool php_output_flush();
POLAR_DECL_EXPORT void php_output_flush_all();
POLAR_DECL_EXPORT bool php_output_clean();
POLAR_DECL_EXPORT void php_output_clean_all();
POLAR_DECL_EXPORT bool php_output_end();
POLAR_DECL_EXPORT void php_output_end_all();
POLAR_DECL_EXPORT bool php_output_discard();
POLAR_DECL_EXPORT void php_output_discard_all();

POLAR_DECL_EXPORT bool php_output_get_contents(zval *p);
POLAR_DECL_EXPORT bool php_output_get_length(zval *p);
POLAR_DECL_EXPORT int php_output_get_level();
POLAR_DECL_EXPORT PhpOutputHandler* php_output_get_active_handler();

POLAR_DECL_EXPORT bool php_output_start_default();
POLAR_DECL_EXPORT bool php_output_start_devnull();

POLAR_DECL_EXPORT bool php_output_start_user(zval *output_handler, size_t chunk_size, int flags);
POLAR_DECL_EXPORT bool php_output_start_internal(const char *name, size_t name_len,
                                                 PhpOutputHandlerFuncType output_handler,
                                                 size_t chunk_size, int flags);

POLAR_DECL_EXPORT PhpOutputHandler *php_output_handler_create_user(zval *handler, size_t chunk_size, int flags);
POLAR_DECL_EXPORT PhpOutputHandler *php_output_handler_create_internal(const char *name, size_t name_len,
                                                                       PhpOutputHandlerContextFuncType handler,
                                                                       size_t chunk_size, int flags);

POLAR_DECL_EXPORT void php_output_handler_set_context(PhpOutputHandler *handler, void *opaq, void (*dtor)(void*));
POLAR_DECL_EXPORT bool php_output_handler_start(PhpOutputHandler *handler);
POLAR_DECL_EXPORT int php_output_handler_started(const char *name, size_t name_len);
POLAR_DECL_EXPORT int php_output_handler_hook(PhpOutputHandlerHookType type, void *arg);
POLAR_DECL_EXPORT void php_output_handler_dtor(PhpOutputHandler *handler);
POLAR_DECL_EXPORT void php_output_handler_free(PhpOutputHandler **handler);

POLAR_DECL_EXPORT int php_output_handler_conflict(const char *handler_new,
                                                  size_t handler_new_len, const char *handler_set,
                                                  size_t handler_set_len);
POLAR_DECL_EXPORT bool php_output_handler_conflict_register(const char *handler_name, size_t handler_name_len,
                                                            PhpOutputHandlerConflictCheckType check_func);
POLAR_DECL_EXPORT bool php_output_handler_reverse_conflict_register(const char *handler_name, size_t handler_name_len,
                                                                    PhpOutputHandlerConflictCheckType check_func);

POLAR_DECL_EXPORT PhpOutputHandlerAliasCtorType php_output_handler_alias(const char *handler_name, size_t handler_name_len);
POLAR_DECL_EXPORT bool php_output_handler_alias_register(const char *handler_name, size_t handler_name_len,
                                                         PhpOutputHandlerAliasCtorType func);

} // runtime
} // polar

#endif // POLARPHP_RUNTIME_OUTPUT_H
