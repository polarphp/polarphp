// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/02/11.

#include "polarphp/runtime/langsupport/Reflection.h"
#include "polarphp/runtime/Spprintf.h"

#include <cstdarg>

#define reflection_update_property(object, name, value) do { \
   zval member; \
   ZVAL_STR(&member, name); \
   zend_std_write_property(object, &member, value, nullptr); \
   Z_TRY_DELREF_P(value); \
   zval_ptr_dtor(&member); \
   } while (0)

#define reflection_update_property_name(object, value) \
   reflection_update_property(object, ZSTR_KNOWN(ZEND_STR_NAME), value)

#define reflection_update_property_class(object, value) \
   reflection_update_property(object, ZSTR_KNOWN(ZEND_STR_CLASS), value)

namespace polar {
namespace runtime {

/* Class entry pointers */

POLAR_DECL_EXPORT zend_class_entry *g_reflectorPtr = nullptr;
POLAR_DECL_EXPORT zend_class_entry *g_reflectionExceptionPtr = nullptr;
POLAR_DECL_EXPORT zend_class_entry *g_reflectionPtr = nullptr;
POLAR_DECL_EXPORT zend_class_entry *g_reflectionFunctionAbstractPtr = nullptr;
POLAR_DECL_EXPORT zend_class_entry *g_reflectionFunctionPtr = nullptr;
POLAR_DECL_EXPORT zend_class_entry *g_reflectionGeneratorPtr = nullptr;
POLAR_DECL_EXPORT zend_class_entry *g_reflectionParameterPtr = nullptr;
POLAR_DECL_EXPORT zend_class_entry *g_reflectionTypePtr = nullptr;
POLAR_DECL_EXPORT zend_class_entry *g_reflectionNamedTypePtr = nullptr;
POLAR_DECL_EXPORT zend_class_entry *g_reflectionClassPtr = nullptr;
POLAR_DECL_EXPORT zend_class_entry *g_reflectionClassConstantPtr = nullptr;
POLAR_DECL_EXPORT zend_class_entry *g_reflectionObjectPtr = nullptr;
POLAR_DECL_EXPORT zend_class_entry *g_reflectionMethodPtr = nullptr;
POLAR_DECL_EXPORT zend_class_entry *g_reflectionPropertyPtr = nullptr;

/* Exception throwing macro */
#define _DO_THROW(msg)                                                                                      \
   zend_throw_exception(g_reflectionExceptionPtr, msg, 0);                                       \
   return;                                                                                                 \

#define RETURN_ON_EXCEPTION                                                                                 \
   if (EG(exception) && EG(exception)->ce == g_reflectionExceptionPtr) {                            \
   return;                                                                                             \
}

#define GET_REFLECTION_OBJECT()	                                                                   			\
   intern = Z_REFLECTION_P(getThis());                                                      				\
   if (intern->ptr == nullptr) {                                                            \
   RETURN_ON_EXCEPTION                                                                                 \
   zend_throw_error(nullptr, "Internal error: Failed to retrieve the reflection object");        \
   return;                                                                                             \
}                                                                                                       \

#define GET_REFLECTION_OBJECT_PTR(target, type)                                                                   \
   GET_REFLECTION_OBJECT()																					\
   target = reinterpret_cast<type>(intern->ptr);                                                                                   \

/* Class constants */
#define REGISTER_REFLECTION_CLASS_CONST_LONG(class_name, const_name, value)                                        \
   zend_declare_class_constant_long(g_reflection ## class_name ## Ptr, const_name, sizeof(const_name)-1, (zend_long)value);

#define Z_REFLECTION_P(zv)  reflection_object_from_obj(Z_OBJ_P((zv)))

/* Struct for properties */
struct property_reference
{
   zend_class_entry *ce;
   zend_property_info prop;
   zend_string *unmangledName;
};

/* Struct for parameters */
struct parameter_reference
{
   uint32_t offset;
   zend_bool required;
   struct _zend_arg_info *argInfo;
   zend_function *fptr;
} ;

/* Struct for type hints */
struct type_reference
{
   struct _zend_arg_info *argInfo;
   zend_function *fptr;
} ;

enum reflection_type_t
{
   REF_TYPE_OTHER,      /* Must be 0 */
   REF_TYPE_FUNCTION,
   REF_TYPE_GENERATOR,
   REF_TYPE_PARAMETER,
   REF_TYPE_TYPE,
   REF_TYPE_PROPERTY,
   REF_TYPE_CLASS_CONSTANT
};

/* Struct for reflection objects */
struct reflection_object
{
   zval dummy; /* holder for the second property */
   zval obj;
   void *ptr;
   zend_class_entry *ce;
   reflection_type_t refType;
   unsigned int ignoreVisibility:1;
   zend_object zo;
};

static zend_object_handlers sg_reflectionObjectHandlers;

namespace {
inline reflection_object *reflection_object_from_obj(zend_object *obj)
{
   return (reflection_object*)((char*)(obj) - XtOffsetOf(reflection_object, zo));
}

zval *get_default_load_name(zval *object)
{
   return zend_hash_find_ex_ind(Z_OBJPROP_P(object), ZSTR_KNOWN(ZEND_STR_NAME), 1);
}

void default_get_name(zval *object, zval *return_value)
{
   zval *value;
   if ((value = get_default_load_name(object)) == nullptr) {
      RETURN_FALSE;
   }
   ZVAL_COPY(return_value, value);
}

zend_function *copy_function(zend_function *fptr)
{
   if (fptr
       && (fptr->internal_function.fn_flags & ZEND_ACC_CALL_VIA_TRAMPOLINE)) {
      zend_function *copyFuncPtr;
      copyFuncPtr = reinterpret_cast<zend_function *>(emalloc(sizeof(zend_function)));
      memcpy(copyFuncPtr, fptr, sizeof(zend_function));
      copyFuncPtr->internal_function.function_name = zend_string_copy(fptr->internal_function.function_name);
      return copyFuncPtr;
   } else {
      /* no copy needed */
      return fptr;
   }
}

void free_function(zend_function *fptr)
{
   if (fptr
       && (fptr->internal_function.fn_flags & ZEND_ACC_CALL_VIA_TRAMPOLINE)) {
      zend_string_release_ex(fptr->internal_function.function_name, 0);
      zend_free_trampoline(fptr);
   }
}

void reflection_free_objects_storage(zend_object *object)
{
   reflection_object *intern = reflection_object_from_obj(object);
   parameter_reference *reference;
   property_reference *propReference;
   type_reference *typeReference;

   if (intern->ptr) {
      switch (intern->refType) {
      case REF_TYPE_PARAMETER:
         reference = (parameter_reference*)intern->ptr;
         free_function(reference->fptr);
         efree(intern->ptr);
         break;
      case REF_TYPE_TYPE:
         typeReference = (type_reference*)intern->ptr;
         free_function(typeReference->fptr);
         efree(intern->ptr);
         break;
      case REF_TYPE_FUNCTION:
         free_function(reinterpret_cast<zend_function *>(intern->ptr));
         break;
      case REF_TYPE_PROPERTY:
         propReference = (property_reference*)intern->ptr;
         zend_string_release_ex(propReference->unmangledName, 0);
         efree(intern->ptr);
         break;
      case REF_TYPE_GENERATOR:
      case REF_TYPE_CLASS_CONSTANT:
      case REF_TYPE_OTHER:
         break;
      }
   }
   intern->ptr = nullptr;
   zval_ptr_dtor(&intern->obj);
   zend_object_std_dtor(object);
}

HashTable *reflection_get_gc(zval *obj, zval **gc_data, int *gc_data_count)
{
   reflection_object *intern = Z_REFLECTION_P(obj);
   *gc_data = &intern->obj;
   *gc_data_count = 1;
   return zend_std_get_properties(obj);
}

zend_object *reflection_objects_new(zend_class_entry *class_type)
{
   reflection_object *intern = reinterpret_cast<reflection_object *>(zend_object_alloc(sizeof(reflection_object), class_type));
   zend_object_std_init(&intern->zo, class_type);
   object_properties_init(&intern->zo, class_type);
   intern->zo.handlers = &sg_reflectionObjectHandlers;
   return &intern->zo;
}

zval *reflection_instantiate(zend_class_entry *pce, zval *object)
{
   object_init_ex(object, pce);
   return object;
}

void function_string(smart_str *str, zend_function *fptr, zend_class_entry *scope, char* indent);
void property_string(smart_str *str, zend_property_info *prop, const char *prop_name, char* indent);
void class_const_string(smart_str *str, char *name, zend_class_constant *c, char* indent);
void class_string(smart_str *str, zend_class_entry *ce, zval *obj, char *indent);

void class_string(smart_str *str, zend_class_entry *ce, zval *obj, char *indent)
{
   int count, count_static_props = 0, count_static_funcs = 0, count_shadow_props = 0;
   zend_string *sub_indent = polar_strpprintf(0, "%s    ", indent);
   /* TBD: Repair indenting of doc comment (or is this to be done in the parser?) */
   if (ce->type == ZEND_USER_CLASS && ce->info.user.doc_comment) {
      smart_str_append_printf(str, "%s%s", indent, ZSTR_VAL(ce->info.user.doc_comment));
      smart_str_appendc(str, '\n');
   }
   if (obj && Z_TYPE_P(obj) == IS_OBJECT) {
      smart_str_append_printf(str, "%sObject of class [ ", indent);
   } else {
      char *kind = const_cast<char *>("Class");
      if (ce->ce_flags & ZEND_ACC_INTERFACE) {
         kind = const_cast<char *>("Interface");
      } else if (ce->ce_flags & ZEND_ACC_TRAIT) {
         kind = const_cast<char *>("Trait");
      }
      smart_str_append_printf(str, "%s%s [ ", indent, kind);
   }
   smart_str_append_printf(str, (ce->type == ZEND_USER_CLASS) ? "<user" : "<internal");
   if (ce->type == ZEND_INTERNAL_CLASS && ce->info.internal.module) {
      smart_str_append_printf(str, ":%s", ce->info.internal.module->name);
   }
   smart_str_append_printf(str, "> ");
   if (ce->get_iterator != nullptr) {
      smart_str_append_printf(str, "<iterateable> ");
   }
   if (ce->ce_flags & ZEND_ACC_INTERFACE) {
      smart_str_append_printf(str, "interface ");
   } else if (ce->ce_flags & ZEND_ACC_TRAIT) {
      smart_str_append_printf(str, "trait ");
   } else {
      if (ce->ce_flags & (ZEND_ACC_IMPLICIT_ABSTRACT_CLASS|ZEND_ACC_EXPLICIT_ABSTRACT_CLASS)) {
         smart_str_append_printf(str, "abstract ");
      }
      if (ce->ce_flags & ZEND_ACC_FINAL) {
         smart_str_append_printf(str, "final ");
      }
      smart_str_append_printf(str, "class ");
   }
   smart_str_append_printf(str, "%s", ZSTR_VAL(ce->name));
   if (ce->parent) {
      smart_str_append_printf(str, " extends %s", ZSTR_VAL(ce->parent->name));
   }
   if (ce->num_interfaces) {
      uint32_t i;
      if (ce->ce_flags & ZEND_ACC_INTERFACE) {
         smart_str_append_printf(str, " extends %s", ZSTR_VAL(ce->interfaces[0]->name));
      } else {
         smart_str_append_printf(str, " implements %s", ZSTR_VAL(ce->interfaces[0]->name));
      }
      for (i = 1; i < ce->num_interfaces; ++i) {
         smart_str_append_printf(str, ", %s", ZSTR_VAL(ce->interfaces[i]->name));
      }
   }
   smart_str_append_printf(str, " ] {\n");

   /* The information where a class is declared is only available for user classes */
   if (ce->type == ZEND_USER_CLASS) {
      smart_str_append_printf(str, "%s  @@ %s %d-%d\n", indent, ZSTR_VAL(ce->info.user.filename),
                              ce->info.user.line_start, ce->info.user.line_end);
   }
   /* Constants */
   smart_str_append_printf(str, "\n");
   count = zend_hash_num_elements(&ce->constants_table);
   smart_str_append_printf(str, "%s  - Constants [%d] {\n", indent, count);
   if (count > 0) {
      zend_string *key;
      zend_class_constant *c;
      POLAR_HASH_FOREACH_STR_KEY_PTR_WITH_TYPE(&ce->constants_table, key, zend_class_constant *, c) {
         class_const_string(str, ZSTR_VAL(key), c, ZSTR_VAL(sub_indent));
         if (UNEXPECTED(EG(exception))) {
            return;
         }
      } POLAR_HASH_FOREACH_END();
   }
   smart_str_append_printf(str, "%s  }\n", indent);

   /* Static properties */
   /* counting static properties */
   count = zend_hash_num_elements(&ce->properties_info);
   if (count > 0) {
      zend_property_info *prop;
      POLAR_HASH_FOREACH_PTR_WITH_TYPE(&ce->properties_info, zend_property_info *, prop) {
         if(prop->flags & ZEND_ACC_SHADOW) {
            count_shadow_props++;
         } else if (prop->flags & ZEND_ACC_STATIC) {
            count_static_props++;
         }
      } POLAR_HASH_FOREACH_END();
   }
   /* static properties */
   smart_str_append_printf(str, "\n%s  - Static properties [%d] {\n", indent, count_static_props);
   if (count_static_props > 0) {
      zend_property_info *prop;
      POLAR_HASH_FOREACH_PTR_WITH_TYPE(&ce->properties_info, zend_property_info *, prop) {
         if ((prop->flags & ZEND_ACC_STATIC) && !(prop->flags & ZEND_ACC_SHADOW)) {
            property_string(str, prop, nullptr, ZSTR_VAL(sub_indent));
         }
      } POLAR_HASH_FOREACH_END();
   }
   smart_str_append_printf(str, "%s  }\n", indent);
   /* Static methods */
   /* counting static methods */
   count = zend_hash_num_elements(&ce->function_table);
   if (count > 0) {
      zend_function *mptr;
      POLAR_HASH_FOREACH_PTR_WITH_TYPE(&ce->function_table, zend_function *, mptr) {
         if (mptr->common.fn_flags & ZEND_ACC_STATIC
             && ((mptr->common.fn_flags & ZEND_ACC_PRIVATE) == 0 || mptr->common.scope == ce))
         {
            count_static_funcs++;
         }
      } POLAR_HASH_FOREACH_END();
   }

   /* static methods */
   smart_str_append_printf(str, "\n%s  - Static methods [%d] {", indent, count_static_funcs);
   if (count_static_funcs > 0) {
      zend_function *mptr;

      POLAR_HASH_FOREACH_PTR_WITH_TYPE(&ce->function_table, zend_function *, mptr) {
         if (mptr->common.fn_flags & ZEND_ACC_STATIC
             && ((mptr->common.fn_flags & ZEND_ACC_PRIVATE) == 0 || mptr->common.scope == ce))
         {
            smart_str_append_printf(str, "\n");
            function_string(str, mptr, ce, ZSTR_VAL(sub_indent));
         }
      } POLAR_HASH_FOREACH_END();
   } else {
      smart_str_append_printf(str, "\n");
   }
   smart_str_append_printf(str, "%s  }\n", indent);

   /* Default/Implicit properties */
   count = zend_hash_num_elements(&ce->properties_info) - count_static_props - count_shadow_props;
   smart_str_append_printf(str, "\n%s  - Properties [%d] {\n", indent, count);
   if (count > 0) {
      zend_property_info *prop;

      POLAR_HASH_FOREACH_PTR_WITH_TYPE(&ce->properties_info, zend_property_info *, prop) {
         if (!(prop->flags & (ZEND_ACC_STATIC|ZEND_ACC_SHADOW))) {
            property_string(str, prop, nullptr, ZSTR_VAL(sub_indent));
         }
      } POLAR_HASH_FOREACH_END();
   }
   smart_str_append_printf(str, "%s  }\n", indent);

   if (obj && Z_TYPE_P(obj) == IS_OBJECT && Z_OBJ_HT_P(obj)->get_properties) {
      HashTable    *properties = Z_OBJ_HT_P(obj)->get_properties(obj);
      zend_string  *prop_name;
      smart_str prop_str = {0};
      count = 0;
      if (properties && zend_hash_num_elements(properties)) {
         ZEND_HASH_FOREACH_STR_KEY(properties, prop_name) {
            if (prop_name && ZSTR_LEN(prop_name) && ZSTR_VAL(prop_name)[0]) { /* skip all private and protected properties */
               if (!zend_hash_exists(&ce->properties_info, prop_name)) {
                  count++;
                  property_string(&prop_str, nullptr, ZSTR_VAL(prop_name), ZSTR_VAL(sub_indent));
               }
            }
         } ZEND_HASH_FOREACH_END();
      }
      smart_str_append_printf(str, "\n%s  - Dynamic properties [%d] {\n", indent, count);
      smart_str_append_smart_str(str, &prop_str);
      smart_str_append_printf(str, "%s  }\n", indent);
      smart_str_free(&prop_str);
   }

   /* Non static methods */
   count = zend_hash_num_elements(&ce->function_table) - count_static_funcs;
   if (count > 0) {
      zend_function *mptr;
      zend_string *key;
      smart_str method_str = {0};

      count = 0;
      POLAR_HASH_FOREACH_STR_KEY_PTR_WITH_TYPE(&ce->function_table, key, zend_function *, mptr) {
         if ((mptr->common.fn_flags & ZEND_ACC_STATIC) == 0
             && ((mptr->common.fn_flags & ZEND_ACC_PRIVATE) == 0 || mptr->common.scope == ce))
         {
            size_t len = ZSTR_LEN(mptr->common.function_name);

            /* Do not display old-style inherited constructors */
            if ((mptr->common.fn_flags & ZEND_ACC_CTOR) == 0
                || mptr->common.scope == ce
                || !key
                || zend_binary_strcasecmp(ZSTR_VAL(key), ZSTR_LEN(key), ZSTR_VAL(mptr->common.function_name), len) == 0)
            {
               zend_function *closure;
               /* see if this is a closure */
               if (ce == zend_ce_closure && obj && (len == sizeof(ZEND_INVOKE_FUNC_NAME)-1)
                   && memcmp(ZSTR_VAL(mptr->common.function_name), ZEND_INVOKE_FUNC_NAME, sizeof(ZEND_INVOKE_FUNC_NAME)-1) == 0
                   && (closure = zend_get_closure_invoke_method(Z_OBJ_P(obj))) != nullptr)
               {
                  mptr = closure;
               } else {
                  closure = nullptr;
               }
               smart_str_appendc(&method_str, '\n');
               function_string(&method_str, mptr, ce, ZSTR_VAL(sub_indent));
               count++;
               free_function(closure);
            }
         }
      } POLAR_HASH_FOREACH_END();
      smart_str_append_printf(str, "\n%s  - Methods [%d] {", indent, count);
      smart_str_append_smart_str(str, &method_str);
      if (!count) {
         smart_str_append_printf(str, "\n");
      }
      smart_str_free(&method_str);
   } else {
      smart_str_append_printf(str, "\n%s  - Methods [0] {\n", indent);
   }
   smart_str_append_printf(str, "%s  }\n", indent);
   smart_str_append_printf(str, "%s}\n", indent);
   zend_string_release_ex(sub_indent, 0);
}

void class_const_string(smart_str *str, char *name, zend_class_constant *c, char *indent)
{
   char *visibility = zend_visibility_string(Z_ACCESS_FLAGS(c->value));
   char *type;
   zval_update_constant_ex(&c->value, c->ce);
   type = zend_zval_type_name(&c->value);
   if (Z_TYPE(c->value) == IS_ARRAY) {
      smart_str_append_printf(str, "%sConstant [ %s %s %s ] { Array }\n",
                              indent, visibility, type, name);
   } else {
      zend_string *tmp_value_str;
      zend_string *value_str = zval_get_tmp_string(&c->value, &tmp_value_str);
      smart_str_append_printf(str, "%sConstant [ %s %s %s ] { %s }\n",
                              indent, visibility, type, name, ZSTR_VAL(value_str));

      zend_tmp_string_release(tmp_value_str);
   }
}

zend_op* get_recv_op(zend_op_array *op_array, uint32_t offset)
{
   zend_op *op = op_array->opcodes;
   zend_op *end = op + op_array->last;

   ++offset;
   while (op < end) {
      if ((op->opcode == ZEND_RECV || op->opcode == ZEND_RECV_INIT
           || op->opcode == ZEND_RECV_VARIADIC) && op->op1.num == offset)
      {
         return op;
      }
      ++op;
   }
   return nullptr;
}

void parameter_string(smart_str *str, zend_function *fptr, struct _zend_arg_info *arg_info,
                      uint32_t offset, zend_bool required, char* indent)
{
   smart_str_append_printf(str, "Parameter #%d [ ", offset);
   if (!required) {
      smart_str_append_printf(str, "<optional> ");
   } else {
      smart_str_append_printf(str, "<required> ");
   }
   if (ZEND_TYPE_IS_CLASS(arg_info->type)) {
      smart_str_append_printf(str, "%s ",
                              ZSTR_VAL(ZEND_TYPE_NAME(arg_info->type)));
      if (ZEND_TYPE_ALLOW_NULL(arg_info->type)) {
         smart_str_append_printf(str, "or nullptr ");
      }
   } else if (ZEND_TYPE_IS_CODE(arg_info->type)) {
      smart_str_append_printf(str, "%s ", zend_get_type_by_const(ZEND_TYPE_CODE(arg_info->type)));
      if (ZEND_TYPE_ALLOW_NULL(arg_info->type)) {
         smart_str_append_printf(str, "or nullptr ");
      }
   }
   if (arg_info->pass_by_reference) {
      smart_str_appendc(str, '&');
   }
   if (arg_info->is_variadic) {
      smart_str_appends(str, "...");
   }
   if (arg_info->name) {
      smart_str_append_printf(str, "$%s",
                              (fptr->type == ZEND_INTERNAL_FUNCTION &&
                               !(fptr->common.fn_flags & ZEND_ACC_USER_ARG_INFO)) ?
                                 ((zend_internal_arg_info*)arg_info)->name :
                                 ZSTR_VAL(arg_info->name));
   } else {
      smart_str_append_printf(str, "$param%d", offset);
   }
   if (fptr->type == ZEND_USER_FUNCTION && !required) {
      zend_op *precv = get_recv_op((zend_op_array*)fptr, offset);
      if (precv && precv->opcode == ZEND_RECV_INIT && precv->op2_type != IS_UNUSED) {
         zval zv;

         smart_str_appends(str, " = ");
         ZVAL_COPY(&zv, RT_CONSTANT(precv, precv->op2));
         if (UNEXPECTED(zval_update_constant_ex(&zv, fptr->common.scope) == FAILURE)) {
            zval_ptr_dtor(&zv);
            return;
         }
         if (Z_TYPE(zv) == IS_TRUE) {
            smart_str_appends(str, "true");
         } else if (Z_TYPE(zv) == IS_FALSE) {
            smart_str_appends(str, "false");
         } else if (Z_TYPE(zv) == IS_NULL) {
            smart_str_appends(str, "nullptr");
         } else if (Z_TYPE(zv) == IS_STRING) {
            smart_str_appendc(str, '\'');
            smart_str_appendl(str, Z_STRVAL(zv), MIN(Z_STRLEN(zv), 15));
            if (Z_STRLEN(zv) > 15) {
               smart_str_appends(str, "...");
            }
            smart_str_appendc(str, '\'');
         } else if (Z_TYPE(zv) == IS_ARRAY) {
            smart_str_appends(str, "Array");
         } else {
            zend_string *tmp_zv_str;
            zend_string *zv_str = zval_get_tmp_string(&zv, &tmp_zv_str);
            smart_str_append(str, zv_str);
            zend_tmp_string_release(tmp_zv_str);
         }
         zval_ptr_dtor(&zv);
      }
   }
   smart_str_appends(str, " ]");
}

void function_parameter_string(smart_str *str, zend_function *fptr, char* indent)
{
   struct _zend_arg_info *arg_info = fptr->common.arg_info;
   uint32_t i, num_args, num_required = fptr->common.required_num_args;

   if (!arg_info) {
      return;
   }

   num_args = fptr->common.num_args;
   if (fptr->common.fn_flags & ZEND_ACC_VARIADIC) {
      num_args++;
   }
   smart_str_appendc(str, '\n');
   smart_str_append_printf(str, "%s- Parameters [%d] {\n", indent, num_args);
   for (i = 0; i < num_args; i++) {
      smart_str_append_printf(str, "%s  ", indent);
      parameter_string(str, fptr, arg_info, i, i < num_required, indent);
      smart_str_appendc(str, '\n');
      arg_info++;
   }
   smart_str_append_printf(str, "%s}\n", indent);
}

void function_closure_string(smart_str *str, zend_function *fptr, char* indent)
{
   uint32_t i, count;
   zend_string *key;
   HashTable *static_variables;

   if (fptr->type != ZEND_USER_FUNCTION || !fptr->op_array.static_variables) {
      return;
   }

   static_variables = fptr->op_array.static_variables;
   count = zend_hash_num_elements(static_variables);

   if (!count) {
      return;
   }

   smart_str_append_printf(str, "\n");
   smart_str_append_printf(str, "%s- Bound Variables [%d] {\n", indent, zend_hash_num_elements(static_variables));
   i = 0;
   ZEND_HASH_FOREACH_STR_KEY(static_variables, key) {
      smart_str_append_printf(str, "%s    Variable #%d [ $%s ]\n", indent, i++, ZSTR_VAL(key));
   } ZEND_HASH_FOREACH_END();
   smart_str_append_printf(str, "%s}\n", indent);
}

void function_string(smart_str *str, zend_function *fptr, zend_class_entry *scope, char* indent)
{
   smart_str param_indent = {0};
   zend_function *overwrites;
   zend_string *lc_name;

   /* TBD: Repair indenting of doc comment (or is this to be done in the parser?)
    * What's "wrong" is that any whitespace before the doc comment start is
    * swallowed, leading to an unaligned comment.
    */
   if (fptr->type == ZEND_USER_FUNCTION && fptr->op_array.doc_comment) {
      smart_str_append_printf(str, "%s%s\n", indent, ZSTR_VAL(fptr->op_array.doc_comment));
   }

   smart_str_appendl(str, indent, strlen(indent));
   smart_str_append_printf(str, fptr->common.fn_flags & ZEND_ACC_CLOSURE ? "Closure [ " : (fptr->common.scope ? "Method [ " : "Function [ "));
   smart_str_append_printf(str, (fptr->type == ZEND_USER_FUNCTION) ? "<user" : "<internal");
   if (fptr->common.fn_flags & ZEND_ACC_DEPRECATED) {
      smart_str_appends(str, ", deprecated");
   }
   if (fptr->type == ZEND_INTERNAL_FUNCTION && ((zend_internal_function*)fptr)->module) {
      smart_str_append_printf(str, ":%s", ((zend_internal_function*)fptr)->module->name);
   }

   if (scope && fptr->common.scope) {
      if (fptr->common.scope != scope) {
         smart_str_append_printf(str, ", inherits %s", ZSTR_VAL(fptr->common.scope->name));
      } else if (fptr->common.scope->parent) {
         lc_name = zend_string_tolower(fptr->common.function_name);
         if ((overwrites = reinterpret_cast<zend_function *>(zend_hash_find_ptr(&fptr->common.scope->parent->function_table, lc_name))) != nullptr) {
            if (fptr->common.scope != overwrites->common.scope) {
               smart_str_append_printf(str, ", overwrites %s", ZSTR_VAL(overwrites->common.scope->name));
            }
         }
         zend_string_release_ex(lc_name, 0);
      }
   }
   if (fptr->common.prototype && fptr->common.prototype->common.scope) {
      smart_str_append_printf(str, ", prototype %s", ZSTR_VAL(fptr->common.prototype->common.scope->name));
   }
   if (fptr->common.fn_flags & ZEND_ACC_CTOR) {
      smart_str_appends(str, ", ctor");
   }
   if (fptr->common.fn_flags & ZEND_ACC_DTOR) {
      smart_str_appends(str, ", dtor");
   }
   smart_str_appends(str, "> ");

   if (fptr->common.fn_flags & ZEND_ACC_ABSTRACT) {
      smart_str_appends(str, "abstract ");
   }
   if (fptr->common.fn_flags & ZEND_ACC_FINAL) {
      smart_str_appends(str, "final ");
   }
   if (fptr->common.fn_flags & ZEND_ACC_STATIC) {
      smart_str_appends(str, "static ");
   }

   if (fptr->common.scope) {
      /* These are mutually exclusive */
      switch (fptr->common.fn_flags & ZEND_ACC_PPP_MASK) {
      case ZEND_ACC_PUBLIC:
         smart_str_appends(str, "public ");
         break;
      case ZEND_ACC_PRIVATE:
         smart_str_appends(str, "private ");
         break;
      case ZEND_ACC_PROTECTED:
         smart_str_appends(str, "protected ");
         break;
      default:
         smart_str_appends(str, "<visibility error> ");
         break;
      }
      smart_str_appends(str, "method ");
   } else {
      smart_str_appends(str, "function ");
   }

   if (fptr->op_array.fn_flags & ZEND_ACC_RETURN_REFERENCE) {
      smart_str_appendc(str, '&');
   }
   smart_str_append_printf(str, "%s ] {\n", ZSTR_VAL(fptr->common.function_name));
   /* The information where a function is declared is only available for user classes */
   if (fptr->type == ZEND_USER_FUNCTION) {
      smart_str_append_printf(str, "%s  @@ %s %d - %d\n", indent,
                              ZSTR_VAL(fptr->op_array.filename),
                              fptr->op_array.line_start,
                              fptr->op_array.line_end);
   }
   smart_str_append_printf(&param_indent, "%s  ", indent);
   smart_str_0(&param_indent);
   if (fptr->common.fn_flags & ZEND_ACC_CLOSURE) {
      function_closure_string(str, fptr, ZSTR_VAL(param_indent.s));
   }
   function_parameter_string(str, fptr, ZSTR_VAL(param_indent.s));
   smart_str_free(&param_indent);
   if (fptr->op_array.fn_flags & ZEND_ACC_HAS_RETURN_TYPE) {
      smart_str_append_printf(str, "  %s- Return [ ", indent);
      if (ZEND_TYPE_IS_CLASS(fptr->common.arg_info[-1].type)) {
         smart_str_append_printf(str, "%s ",
                                 ZSTR_VAL(ZEND_TYPE_NAME(fptr->common.arg_info[-1].type)));
         if (ZEND_TYPE_ALLOW_NULL(fptr->common.arg_info[-1].type)) {
            smart_str_appends(str, "or nullptr ");
         }
      } else if (ZEND_TYPE_IS_CODE(fptr->common.arg_info[-1].type)) {
         smart_str_append_printf(str, "%s ", zend_get_type_by_const(ZEND_TYPE_CODE(fptr->common.arg_info[-1].type)));
         if (ZEND_TYPE_ALLOW_NULL(fptr->common.arg_info[-1].type)) {
            smart_str_appends(str, "or nullptr ");
         }
      }
      smart_str_appends(str, "]\n");
   }
   smart_str_append_printf(str, "%s}\n", indent);
}

void property_string(smart_str *str, zend_property_info *prop, const char *prop_name, char* indent)
{
   smart_str_append_printf(str, "%sProperty [ ", indent);
   if (!prop) {
      smart_str_append_printf(str, "<dynamic> public $%s", prop_name);
   } else {
      if (!(prop->flags & ZEND_ACC_STATIC)) {
         if (prop->flags & ZEND_ACC_IMPLICIT_PUBLIC) {
            smart_str_appends(str, "<implicit> ");
         } else {
            smart_str_appends(str, "<default> ");
         }
      }

      /* These are mutually exclusive */
      switch (prop->flags & ZEND_ACC_PPP_MASK) {
      case ZEND_ACC_PUBLIC:
         smart_str_appends(str, "public ");
         break;
      case ZEND_ACC_PRIVATE:
         smart_str_appends(str, "private ");
         break;
      case ZEND_ACC_PROTECTED:
         smart_str_appends(str, "protected ");
         break;
      }
      if (prop->flags & ZEND_ACC_STATIC) {
         smart_str_appends(str, "static ");
      }
      if (!prop_name) {
         const char *class_name;
         zend_unmangle_property_name(prop->name, &class_name, &prop_name);
      }
      smart_str_append_printf(str, "$%s", prop_name);
   }

   smart_str_appends(str, " ]\n");
}

void function_check_flag(INTERNAL_FUNCTION_PARAMETERS, int mask)
{
   reflection_object *intern;
   zend_function *mptr;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(mptr, zend_function *);
   RETURN_BOOL(mptr->common.fn_flags & mask);
}

void reflection_parameter_factory(zend_function *fptr, zval *closure_object, struct _zend_arg_info *arg_info,
                                  uint32_t offset, zend_bool required, zval *object)
{
   reflection_object *intern;
   parameter_reference *reference;
   zval name;

   if (arg_info->name) {
      if (fptr->type == ZEND_INTERNAL_FUNCTION &&
          !(fptr->common.fn_flags & ZEND_ACC_USER_ARG_INFO)) {
         ZVAL_STRING(&name, ((zend_internal_arg_info*)arg_info)->name);
      } else {
         ZVAL_STR_COPY(&name, arg_info->name);
      }
   } else {
      ZVAL_NULL(&name);
   }
   reflection_instantiate(g_reflectionParameterPtr, object);
   intern = Z_REFLECTION_P(object);
   reference = (parameter_reference*) emalloc(sizeof(parameter_reference));
   reference->argInfo = arg_info;
   reference->offset = offset;
   reference->required = required;
   reference->fptr = fptr;
   intern->ptr = reference;
   intern->refType = REF_TYPE_PARAMETER;
   intern->ce = fptr->common.scope;
   if (closure_object) {
      Z_ADDREF_P(closure_object);
      ZVAL_COPY_VALUE(&intern->obj, closure_object);
   }
   reflection_update_property_name(object, &name);
}

void reflection_type_factory(zend_function *fptr, zval *closure_object, struct _zend_arg_info *arg_info, zval *object)
{
   reflection_object *intern;
   type_reference *reference;

   reflection_instantiate(g_reflectionNamedTypePtr, object);
   intern = Z_REFLECTION_P(object);
   reference = (type_reference*) emalloc(sizeof(type_reference));
   reference->argInfo = arg_info;
   reference->fptr = fptr;
   intern->ptr = reference;
   intern->refType = REF_TYPE_TYPE;
   intern->ce = fptr->common.scope;
   if (closure_object) {
      Z_ADDREF_P(closure_object);
      ZVAL_COPY_VALUE(&intern->obj, closure_object);
   }
}

void reflection_function_factory(zend_function *function, zval *closure_object, zval *object)
{
   reflection_object *intern;
   zval name;
   ZVAL_STR_COPY(&name, function->common.function_name);
   reflection_instantiate(g_reflectionFunctionPtr, object);
   intern = Z_REFLECTION_P(object);
   intern->ptr = function;
   intern->refType = REF_TYPE_FUNCTION;
   intern->ce = nullptr;
   if (closure_object) {
      Z_ADDREF_P(closure_object);
      ZVAL_COPY_VALUE(&intern->obj, closure_object);
   }
   reflection_update_property_name(object, &name);
}

void reflection_method_factory(zend_class_entry *ce, zend_function *method, zval *closure_object, zval *object)
{
   reflection_object *intern;
   zval name;
   zval classname;

   ZVAL_STR_COPY(&name, (method->common.scope && method->common.scope->trait_aliases)?
                    zend_resolve_method_name(ce, method) : method->common.function_name);
   ZVAL_STR_COPY(&classname, method->common.scope->name);
   reflection_instantiate(g_reflectionMethodPtr, object);
   intern = Z_REFLECTION_P(object);
   intern->ptr = method;
   intern->refType = REF_TYPE_FUNCTION;
   intern->ce = ce;
   if (closure_object) {
      Z_ADDREF_P(closure_object);
      ZVAL_COPY_VALUE(&intern->obj, closure_object);
   }
   reflection_update_property_name(object, &name);
   reflection_update_property_class(object, &classname);
}

void reflection_property_factory(zend_class_entry *ce, zend_string *name, zend_property_info *prop, zval *object)
{
   reflection_object *intern;
   zval propname;
   zval classname;
   property_reference *reference;

   if (!(prop->flags & ZEND_ACC_PRIVATE)) {
      /* we have to search the class hierarchy for this (implicit) public or protected property */
      zend_class_entry *tmp_ce = ce, *store_ce = ce;
      zend_property_info *tmp_info = nullptr;

      while (tmp_ce && (tmp_info = reinterpret_cast<zend_property_info *>(zend_hash_find_ptr(&tmp_ce->properties_info, name))) == nullptr) {
         ce = tmp_ce;
         tmp_ce = tmp_ce->parent;
      }

      if (tmp_info && !(tmp_info->flags & ZEND_ACC_SHADOW)) { /* found something and it's not a parent's private */
         prop = tmp_info;
      } else { /* not found, use initial value */
         ce = store_ce;
      }
   }

   ZVAL_STR_COPY(&propname, name);
   ZVAL_STR_COPY(&classname, prop->ce->name);

   reflection_instantiate(g_reflectionPropertyPtr, object);
   intern = Z_REFLECTION_P(object);
   reference = (property_reference*) emalloc(sizeof(property_reference));
   reference->ce = ce;
   reference->prop = *prop;
   reference->unmangledName = zend_string_copy(name);
   intern->ptr = reference;
   intern->refType = REF_TYPE_PROPERTY;
   intern->ce = ce;
   intern->ignoreVisibility = 0;
   reflection_update_property_name(object, &propname);
   reflection_update_property_class(object, &classname);
}

void reflection_property_factory_str(zend_class_entry *ce, const char *name_str, size_t name_len, zend_property_info *prop, zval *object)
{
   zend_string *name = zend_string_init(name_str, name_len, 0);
   reflection_property_factory(ce, name, prop, object);
   zend_string_release(name);
}

void reflection_class_constant_factory(zend_class_entry *ce, zend_string *name_str, zend_class_constant *constant, zval *object)
{
   reflection_object *intern;
   zval name;
   zval classname;

   ZVAL_STR_COPY(&name, name_str);
   ZVAL_STR_COPY(&classname, ce->name);

   reflection_instantiate(g_reflectionClassConstantPtr, object);
   intern = Z_REFLECTION_P(object);
   intern->ptr = constant;
   intern->refType = REF_TYPE_CLASS_CONSTANT;
   intern->ce = constant->ce;
   intern->ignoreVisibility = 0;
   reflection_update_property_name(object, &name);
   reflection_update_property_class(object, &classname);
}

void reflection_export(INTERNAL_FUNCTION_PARAMETERS, zend_class_entry *ce_ptr, int ctor_argc)
{
   zval reflector;
   zval *argument_ptr, *argument2_ptr;
   zval retval, params[2];
   int result;
   int return_output = 0;
   zend_fcall_info fci;
   zend_fcall_info_cache fcc;

   if (ctor_argc == 1) {
      if (zend_parse_parameters(ZEND_NUM_ARGS(), "z|b", &argument_ptr, &return_output) == FAILURE) {
         return;
      }
      ZVAL_COPY_VALUE(&params[0], argument_ptr);
      ZVAL_NULL(&params[1]);
   } else {
      if (zend_parse_parameters(ZEND_NUM_ARGS(), "zz|b", &argument_ptr, &argument2_ptr, &return_output) == FAILURE) {
         return;
      }
      ZVAL_COPY_VALUE(&params[0], argument_ptr);
      ZVAL_COPY_VALUE(&params[1], argument2_ptr);
   }

   /* Create object */
   if (object_and_properties_init(&reflector, ce_ptr, nullptr) == FAILURE) {
      _DO_THROW("Could not create reflector");
   }

   /* Call __construct() */

   fci.size = sizeof(fci);
   ZVAL_UNDEF(&fci.function_name);
   fci.object = Z_OBJ(reflector);
   fci.retval = &retval;
   fci.param_count = ctor_argc;
   fci.params = params;
   fci.no_separation = 1;

   fcc.function_handler = ce_ptr->constructor;
   fcc.called_scope = Z_OBJCE(reflector);
   fcc.object = Z_OBJ(reflector);

   result = zend_call_function(&fci, &fcc);

   zval_ptr_dtor(&retval);

   if (EG(exception)) {
      zval_ptr_dtor(&reflector);
      return;
   }
   if (result == FAILURE) {
      zval_ptr_dtor(&reflector);
      _DO_THROW("Could not create reflector");
   }

   /* Call static reflection::export */
   ZVAL_COPY_VALUE(&params[0], &reflector);
   ZVAL_BOOL(&params[1], return_output);

   ZVAL_STRINGL(&fci.function_name, "reflection::export", sizeof("reflection::export") - 1);
   fci.object = nullptr;
   fci.retval = &retval;
   fci.param_count = 2;
   fci.params = params;
   fci.no_separation = 1;

   result = zend_call_function(&fci, nullptr);

   zval_ptr_dtor(&fci.function_name);

   if (result == FAILURE && EG(exception) == nullptr) {
      zval_ptr_dtor(&reflector);
      zval_ptr_dtor(&retval);
      _DO_THROW("Could not execute reflection::export()");
   }

   if (return_output) {
      ZVAL_COPY_VALUE(return_value, &retval);
   } else {
      zval_ptr_dtor(&retval);
   }

   /* Destruct reflector which is no longer needed */
   zval_ptr_dtor(&reflector);
}

parameter_reference *reflection_param_get_default_param(INTERNAL_FUNCTION_PARAMETERS)
{
   reflection_object *intern;
   parameter_reference *param;

   intern = Z_REFLECTION_P(getThis());
   if (intern->ptr == nullptr) {
      if (EG(exception) && EG(exception)->ce == g_reflectionExceptionPtr) {
         return nullptr;
      }
      zend_throw_error(nullptr, "Internal error: Failed to retrieve the reflection object");
      return nullptr;
   }

   param = reinterpret_cast<parameter_reference *>(intern->ptr);
   if (param->fptr->type != ZEND_USER_FUNCTION) {
      zend_throw_exception_ex(g_reflectionExceptionPtr, 0, "Cannot determine default value for internal functions");
      return nullptr;
   }

   return param;
}

zend_op *reflection_param_get_default_precv(INTERNAL_FUNCTION_PARAMETERS, parameter_reference *param)
{
   zend_op *precv;

   if (param == nullptr) {
      return nullptr;
   }

   precv = get_recv_op((zend_op_array*)param->fptr, param->offset);
   if (!precv || precv->opcode != ZEND_RECV_INIT || precv->op2_type == IS_UNUSED) {
      zend_throw_exception_ex(g_reflectionExceptionPtr, 0, "Internal error: Failed to retrieve the default value");
      return nullptr;
   }

   return precv;
}

} // anonymous namespace


void zend_reflection_class_factory(zend_class_entry *ce, zval *object)
{
   reflection_object *intern;
   zval name;

   ZVAL_STR_COPY(&name, ce->name);
   reflection_instantiate(g_reflectionClassPtr, object);
   intern = Z_REFLECTION_P(object);
   intern->ptr = ce;
   intern->refType = REF_TYPE_OTHER;
   intern->ce = ce;
   reflection_update_property_name(object, &name);
}

ZEND_METHOD(reflection, __clone)
{
   /* Should never be executable */
   _DO_THROW("Cannot clone object using __clone()");
}

ZEND_METHOD(reflection, export)
{
   zval *object, fname, retval;
   int result;
   zend_bool return_output = 0;

   ZEND_PARSE_PARAMETERS_START(1, 2)
         Z_PARAM_OBJECT_OF_CLASS(object, g_reflectorPtr)
         Z_PARAM_OPTIONAL
         Z_PARAM_BOOL(return_output)
         ZEND_PARSE_PARAMETERS_END();

   /* Invoke the __toString() method */
   ZVAL_STRINGL(&fname, "__tostring", sizeof("__tostring") - 1);
   result= call_user_function(nullptr, object, &fname, &retval, 0, nullptr);
   zval_ptr_dtor_str(&fname);

   if (result == FAILURE) {
      _DO_THROW("Invocation of method __toString() failed");
      /* Returns from this function */
   }

   if (Z_TYPE(retval) == IS_UNDEF) {
      php_error_docref(nullptr, E_WARNING, "%s::__toString() did not return anything", ZSTR_VAL(Z_OBJCE_P(object)->name));
      RETURN_FALSE;
   }

   if (return_output) {
      ZVAL_COPY_VALUE(return_value, &retval);
   } else {
      /* No need for _r variant, return of __toString should always be a string */
      zend_print_zval(&retval, 0);
      zend_printf("\n");
      zval_ptr_dtor(&retval);
   }
}

ZEND_METHOD(reflection, getModifierNames)
{
   zend_long modifiers;

   if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &modifiers) == FAILURE) {
      return;
   }

   array_init(return_value);

   if (modifiers & (ZEND_ACC_ABSTRACT | ZEND_ACC_EXPLICIT_ABSTRACT_CLASS)) {
      add_next_index_stringl(return_value, "abstract", sizeof("abstract")-1);
   }
   if (modifiers & ZEND_ACC_FINAL) {
      add_next_index_stringl(return_value, "final", sizeof("final")-1);
   }
   if (modifiers & ZEND_ACC_IMPLICIT_PUBLIC) {
      add_next_index_stringl(return_value, "public", sizeof("public")-1);
   }

   /* These are mutually exclusive */
   switch (modifiers & ZEND_ACC_PPP_MASK) {
   case ZEND_ACC_PUBLIC:
      add_next_index_stringl(return_value, "public", sizeof("public")-1);
      break;
   case ZEND_ACC_PRIVATE:
      add_next_index_stringl(return_value, "private", sizeof("private")-1);
      break;
   case ZEND_ACC_PROTECTED:
      add_next_index_stringl(return_value, "protected", sizeof("protected")-1);
      break;
   }

   if (modifiers & ZEND_ACC_STATIC) {
      add_next_index_stringl(return_value, "static", sizeof("static")-1);
   }
}

ZEND_METHOD(reflection_function, export)
{
   reflection_export(INTERNAL_FUNCTION_PARAM_PASSTHRU, g_reflectionFunctionPtr, 1);
}

ZEND_METHOD(reflection_function, __construct)
{
   zval name;
   zval *object;
   zval *closure = nullptr;
   reflection_object *intern;
   zend_function *fptr;
   zend_string *fname, *lcname;

   object = getThis();
   intern = Z_REFLECTION_P(object);

   if (zend_parse_parameters_ex(ZEND_PARSE_PARAMS_QUIET, ZEND_NUM_ARGS(), "O", &closure, zend_ce_closure) == SUCCESS) {
      fptr = const_cast<zend_function *>(zend_get_closure_method_def(closure));
      Z_ADDREF_P(closure);
   } else {
      ALLOCA_FLAG(use_heap)

            if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "S", &fname) == FAILURE) {
         return;
      }

      if (UNEXPECTED(ZSTR_VAL(fname)[0] == '\\')) {
         /* Ignore leading "\" */
         ZSTR_ALLOCA_ALLOC(lcname, ZSTR_LEN(fname) - 1, use_heap);
         zend_str_tolower_copy(ZSTR_VAL(lcname), ZSTR_VAL(fname) + 1, ZSTR_LEN(fname) - 1);
         fptr = zend_fetch_function(lcname);
         ZSTR_ALLOCA_FREE(lcname, use_heap);
      } else {
         lcname = zend_string_tolower(fname);
         fptr = zend_fetch_function(lcname);
         zend_string_release(lcname);
      }

      if (fptr == nullptr) {
         zend_throw_exception_ex(g_reflectionExceptionPtr, 0,
                                 "Function %s() does not exist", ZSTR_VAL(fname));
         return;
      }
   }

   ZVAL_STR_COPY(&name, fptr->common.function_name);
   reflection_update_property_name(object, &name);
   intern->ptr = fptr;
   intern->refType = REF_TYPE_FUNCTION;
   if (closure) {
      ZVAL_COPY_VALUE(&intern->obj, closure);
   } else {
      ZVAL_UNDEF(&intern->obj);
   }
   intern->ce = nullptr;
}

ZEND_METHOD(reflection_function, __toString)
{
   reflection_object *intern;
   zend_function *fptr;
   smart_str str = {0};

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(fptr, zend_function *);
   function_string(&str, fptr, intern->ce, const_cast<char *>(""));
   RETURN_STR(smart_str_extract(&str));
}

ZEND_METHOD(reflection_function, getName)
{
   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   default_get_name(getThis(), return_value);
}

ZEND_METHOD(reflection_function, isClosure)
{
   reflection_object *intern;
   zend_function *fptr;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(fptr, zend_function *);
   RETURN_BOOL(fptr->common.fn_flags & ZEND_ACC_CLOSURE);
}

ZEND_METHOD(reflection_function, getClosureThis)
{
   reflection_object *intern;
   zval* closure_this;
   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT();
   if (!Z_ISUNDEF(intern->obj)) {
      closure_this = zend_get_closure_this_ptr(&intern->obj);
      if (!Z_ISUNDEF_P(closure_this)) {
         ZVAL_COPY(return_value, closure_this);
      }
   }
}

ZEND_METHOD(reflection_function, getClosureScopeClass)
{
   reflection_object *intern;
   const zend_function *closure_func;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT();
   if (!Z_ISUNDEF(intern->obj)) {
      closure_func = zend_get_closure_method_def(&intern->obj);
      if (closure_func && closure_func->common.scope) {
         zend_reflection_class_factory(closure_func->common.scope, return_value);
      }
   }
}

ZEND_METHOD(reflection_function, getClosure)
{
   reflection_object *intern;
   zend_function *fptr;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(fptr, zend_function *);

   if (!Z_ISUNDEF(intern->obj)) {
      /* Closures are immutable objects */
      ZVAL_COPY(return_value, &intern->obj);
   } else {
      zend_create_fake_closure(return_value, fptr, nullptr, nullptr, nullptr);
   }
}

ZEND_METHOD(reflection_function, isInternal)
{
   reflection_object *intern;
   zend_function *fptr;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(fptr, zend_function *);
   RETURN_BOOL(fptr->type == ZEND_INTERNAL_FUNCTION);
}

ZEND_METHOD(reflection_function, isUserDefined)
{
   reflection_object *intern;
   zend_function *fptr;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(fptr, zend_function *);
   RETURN_BOOL(fptr->type == ZEND_USER_FUNCTION);
}

ZEND_METHOD(reflection_function, isDisabled)
{
   reflection_object *intern;
   zend_function *fptr;

   GET_REFLECTION_OBJECT_PTR(fptr, zend_function *);
   RETURN_BOOL(fptr->type == ZEND_INTERNAL_FUNCTION && fptr->internal_function.handler == zif_display_disabled_function);
}

ZEND_METHOD(reflection_function, getFileName)
{
   reflection_object *intern;
   zend_function *fptr;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(fptr, zend_function *);
   if (fptr->type == ZEND_USER_FUNCTION) {
      RETURN_STR_COPY(fptr->op_array.filename);
   }
   RETURN_FALSE;
}

ZEND_METHOD(reflection_function, getStartLine)
{
   reflection_object *intern;
   zend_function *fptr;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(fptr, zend_function *);
   if (fptr->type == ZEND_USER_FUNCTION) {
      RETURN_LONG(fptr->op_array.line_start);
   }
   RETURN_FALSE;
}

ZEND_METHOD(reflection_function, getEndLine)
{
   reflection_object *intern;
   zend_function *fptr;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(fptr, zend_function *);
   if (fptr->type == ZEND_USER_FUNCTION) {
      RETURN_LONG(fptr->op_array.line_end);
   }
   RETURN_FALSE;
}

ZEND_METHOD(reflection_function, getDocComment)
{
   reflection_object *intern;
   zend_function *fptr;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(fptr, zend_function *);
   if (fptr->type == ZEND_USER_FUNCTION && fptr->op_array.doc_comment) {
      RETURN_STR_COPY(fptr->op_array.doc_comment);
   }
   RETURN_FALSE;
}

ZEND_METHOD(reflection_function, getStaticVariables)
{
   reflection_object *intern;
   zend_function *fptr;
   zval *val;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(fptr, zend_function *);

   /* Return an empty array in case no static variables exist */
   if (fptr->type == ZEND_USER_FUNCTION && fptr->op_array.static_variables != nullptr) {
      array_init(return_value);
      if (GC_REFCOUNT(fptr->op_array.static_variables) > 1) {
         if (!(GC_FLAGS(fptr->op_array.static_variables) & IS_ARRAY_IMMUTABLE)) {
            GC_DELREF(fptr->op_array.static_variables);
         }
         fptr->op_array.static_variables = zend_array_dup(fptr->op_array.static_variables);
      }
      ZEND_HASH_FOREACH_VAL(fptr->op_array.static_variables, val) {
         if (UNEXPECTED(zval_update_constant_ex(val, fptr->common.scope) != SUCCESS)) {
            return;
         }
      } ZEND_HASH_FOREACH_END();
      zend_hash_copy(Z_ARRVAL_P(return_value), fptr->op_array.static_variables, zval_add_ref);
   } else {
      POLAR_ZVAL_EMPTY_ARRAY(return_value);
   }
}

ZEND_METHOD(reflection_function, invoke)
{
   zval retval;
   zval *params = nullptr;
   int result, num_args = 0;
   zend_fcall_info fci;
   zend_fcall_info_cache fcc;
   reflection_object *intern;
   zend_function *fptr;

   GET_REFLECTION_OBJECT_PTR(fptr, zend_function *);

   if (zend_parse_parameters(ZEND_NUM_ARGS(), "*", &params, &num_args) == FAILURE) {
      return;
   }

   fci.size = sizeof(fci);
   ZVAL_UNDEF(&fci.function_name);
   fci.object = nullptr;
   fci.retval = &retval;
   fci.param_count = num_args;
   fci.params = params;
   fci.no_separation = 1;

   fcc.function_handler = fptr;
   fcc.called_scope = nullptr;
   fcc.object = nullptr;

   if (!Z_ISUNDEF(intern->obj)) {
      Z_OBJ_HT(intern->obj)->get_closure(
               &intern->obj, &fcc.called_scope, &fcc.function_handler, &fcc.object);
   }

   result = zend_call_function(&fci, &fcc);

   if (result == FAILURE) {
      zend_throw_exception_ex(g_reflectionExceptionPtr, 0,
                              "Invocation of function %s() failed", ZSTR_VAL(fptr->common.function_name));
      return;
   }

   if (Z_TYPE(retval) != IS_UNDEF) {
      if (Z_ISREF(retval)) {
         zend_unwrap_reference(&retval);
      }
      ZVAL_COPY_VALUE(return_value, &retval);
   }
}

ZEND_METHOD(reflection_function, invokeArgs)
{
   zval retval;
   zval *params, *val;
   int result;
   int i, argc;
   zend_fcall_info fci;
   zend_fcall_info_cache fcc;
   reflection_object *intern;
   zend_function *fptr;
   zval *param_array;

   GET_REFLECTION_OBJECT_PTR(fptr, zend_function *);

   if (zend_parse_parameters(ZEND_NUM_ARGS(), "a", &param_array) == FAILURE) {
      return;
   }

   argc = zend_hash_num_elements(Z_ARRVAL_P(param_array));

   params = reinterpret_cast<zval *>(safe_emalloc(sizeof(zval), argc, 0));
   argc = 0;
   ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(param_array), val) {
      ZVAL_COPY(&params[argc], val);
      argc++;
   } ZEND_HASH_FOREACH_END();

   fci.size = sizeof(fci);
   ZVAL_UNDEF(&fci.function_name);
   fci.object = nullptr;
   fci.retval = &retval;
   fci.param_count = argc;
   fci.params = params;
   fci.no_separation = 1;

   fcc.function_handler = fptr;
   fcc.called_scope = nullptr;
   fcc.object = nullptr;

   if (!Z_ISUNDEF(intern->obj)) {
      Z_OBJ_HT(intern->obj)->get_closure(
               &intern->obj, &fcc.called_scope, &fcc.function_handler, &fcc.object);
   }

   result = zend_call_function(&fci, &fcc);

   for (i = 0; i < argc; i++) {
      zval_ptr_dtor(&params[i]);
   }
   efree(params);

   if (result == FAILURE) {
      zend_throw_exception_ex(g_reflectionExceptionPtr, 0,
                              "Invocation of function %s() failed", ZSTR_VAL(fptr->common.function_name));
      return;
   }

   if (Z_TYPE(retval) != IS_UNDEF) {
      if (Z_ISREF(retval)) {
         zend_unwrap_reference(&retval);
      }
      ZVAL_COPY_VALUE(return_value, &retval);
   }
}

ZEND_METHOD(reflection_function, returnsReference)
{
   reflection_object *intern;
   zend_function *fptr;
   GET_REFLECTION_OBJECT_PTR(fptr, zend_function *);
   RETURN_BOOL((fptr->op_array.fn_flags & ZEND_ACC_RETURN_REFERENCE) != 0);
}

ZEND_METHOD(reflection_function, getNumberOfParameters)
{
   reflection_object *intern;
   zend_function *fptr;
   uint32_t num_args;
   GET_REFLECTION_OBJECT_PTR(fptr, zend_function *);
   num_args = fptr->common.num_args;
   if (fptr->common.fn_flags & ZEND_ACC_VARIADIC) {
      num_args++;
   }
   RETURN_LONG(num_args);
}

ZEND_METHOD(reflection_function, getNumberOfRequiredParameters)
{
   reflection_object *intern;
   zend_function *fptr;
   GET_REFLECTION_OBJECT_PTR(fptr, zend_function *);
   RETURN_LONG(fptr->common.required_num_args);
}

ZEND_METHOD(reflection_function, getParameters)
{
   reflection_object *intern;
   zend_function *fptr;
   uint32_t i, num_args;
   struct _zend_arg_info *arg_info;
   GET_REFLECTION_OBJECT_PTR(fptr, zend_function *);
   arg_info= fptr->common.arg_info;
   num_args = fptr->common.num_args;
   if (fptr->common.fn_flags & ZEND_ACC_VARIADIC) {
      num_args++;
   }
   if (!num_args) {
      POLAR_ZVAL_EMPTY_ARRAY(return_value);
      return;
   }
   array_init(return_value);
   for (i = 0; i < num_args; i++) {
      zval parameter;
      reflection_parameter_factory(
               copy_function(fptr),
               Z_ISUNDEF(intern->obj) ? nullptr : &intern->obj,
               arg_info,
               i,
               i < fptr->common.required_num_args,
               &parameter
               );
      add_next_index_zval(return_value, &parameter);
      arg_info++;
   }
}

ZEND_METHOD(reflection_generator, __construct)
{
   zval *generator, *object;
   reflection_object *intern;
   zend_execute_data *ex;

   object = getThis();
   intern = Z_REFLECTION_P(object);

   if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "O", &generator, zend_ce_generator) == FAILURE) {
      return;
   }

   ex = ((zend_generator *) Z_OBJ_P(generator))->execute_data;
   if (!ex) {
      _DO_THROW("Cannot create ReflectionGenerator based on a terminated Generator");
      return;
   }

   intern->refType = REF_TYPE_GENERATOR;
   ZVAL_COPY(&intern->obj, generator);
   intern->ce = zend_ce_generator;
}

#define REFLECTION_CHECK_VALID_GENERATOR(ex) \
   if (!ex) { \
   _DO_THROW("Cannot fetch information from a terminated Generator"); \
   return; \
}

ZEND_METHOD(reflection_generator, getTrace)
{
   zend_long options = DEBUG_BACKTRACE_PROVIDE_OBJECT;
   zend_generator *generator = (zend_generator *) Z_OBJ(Z_REFLECTION_P(getThis())->obj);
   zend_generator *root_generator;
   zend_execute_data *ex_backup = EG(current_execute_data);
   zend_execute_data *ex = generator->execute_data;
   zend_execute_data *root_prev = nullptr, *cur_prev;

   if (zend_parse_parameters(ZEND_NUM_ARGS(), "|l", &options) == FAILURE) {
      return;
   }

   REFLECTION_CHECK_VALID_GENERATOR(ex)

         root_generator = zend_generator_get_current(generator);

   cur_prev = generator->execute_data->prev_execute_data;
   if (generator == root_generator) {
      generator->execute_data->prev_execute_data = nullptr;
   } else {
      root_prev = root_generator->execute_data->prev_execute_data;
      generator->execute_fake.prev_execute_data = nullptr;
      root_generator->execute_data->prev_execute_data = &generator->execute_fake;
   }

   EG(current_execute_data) = root_generator->execute_data;
   zend_fetch_debug_backtrace(return_value, 0, options, 0);
   EG(current_execute_data) = ex_backup;

   root_generator->execute_data->prev_execute_data = root_prev;
   generator->execute_data->prev_execute_data = cur_prev;
}

ZEND_METHOD(reflection_generator, getExecutingLine)
{
   zend_generator *generator = (zend_generator *) Z_OBJ(Z_REFLECTION_P(getThis())->obj);
   zend_execute_data *ex = generator->execute_data;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }

   REFLECTION_CHECK_VALID_GENERATOR(ex)

         ZVAL_LONG(return_value, ex->opline->lineno);
}

ZEND_METHOD(reflection_generator, getExecutingFile)
{
   zend_generator *generator = (zend_generator *) Z_OBJ(Z_REFLECTION_P(getThis())->obj);
   zend_execute_data *ex = generator->execute_data;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }

   REFLECTION_CHECK_VALID_GENERATOR(ex)

         ZVAL_STR_COPY(return_value, ex->func->op_array.filename);
}

ZEND_METHOD(reflection_generator, getFunction)
{
   zend_generator *generator = (zend_generator *) Z_OBJ(Z_REFLECTION_P(getThis())->obj);
   zend_execute_data *ex = generator->execute_data;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }

   REFLECTION_CHECK_VALID_GENERATOR(ex)

         if (ex->func->common.fn_flags & ZEND_ACC_CLOSURE) {
      zval closure;
      ZVAL_OBJ(&closure, ZEND_CLOSURE_OBJECT(ex->func));
      reflection_function_factory(ex->func, &closure, return_value);
   } else if (ex->func->op_array.scope) {
      reflection_method_factory(ex->func->op_array.scope, ex->func, nullptr, return_value);
   } else {
      reflection_function_factory(ex->func, nullptr, return_value);
   }
}

ZEND_METHOD(reflection_generator, getThis)
{
   zend_generator *generator = (zend_generator *) Z_OBJ(Z_REFLECTION_P(getThis())->obj);
   zend_execute_data *ex = generator->execute_data;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }

   REFLECTION_CHECK_VALID_GENERATOR(ex)

         if (Z_TYPE(ex->This) == IS_OBJECT) {
      ZVAL_COPY(return_value, &ex->This);
   } else {
      ZVAL_NULL(return_value);
   }
}

ZEND_METHOD(reflection_generator, getExecutingGenerator)
{
   zend_generator *generator = (zend_generator *) Z_OBJ(Z_REFLECTION_P(getThis())->obj);
   zend_execute_data *ex = generator->execute_data;
   zend_generator *current;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }

   REFLECTION_CHECK_VALID_GENERATOR(ex)

         current = zend_generator_get_current(generator);
   GC_ADDREF(&current->std);

   ZVAL_OBJ(return_value, (zend_object *) current);
}

ZEND_METHOD(reflection_parameter, export)
{
   reflection_export(INTERNAL_FUNCTION_PARAM_PASSTHRU, g_reflectionParameterPtr, 2);
}

ZEND_METHOD(reflection_parameter, __construct)
{
   parameter_reference *ref;
   zval *reference, *parameter;
   zval *object;
   zval name;
   reflection_object *intern;
   zend_function *fptr;
   struct _zend_arg_info *arg_info;
   int position;
   uint32_t num_args;
   zend_class_entry *ce = nullptr;
   zend_bool is_closure = 0;

   if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "zz", &reference, &parameter) == FAILURE) {
      return;
   }

   object = getThis();
   intern = Z_REFLECTION_P(object);

   /* First, find the function */
   switch (Z_TYPE_P(reference)) {
   case IS_STRING: {
      size_t lcname_len;
      char *lcname;

      lcname_len = Z_STRLEN_P(reference);
      lcname = zend_str_tolower_dup(Z_STRVAL_P(reference), lcname_len);
      if ((fptr = reinterpret_cast<zend_function *>(zend_hash_str_find_ptr(EG(function_table), lcname, lcname_len))) == nullptr) {
         efree(lcname);
         zend_throw_exception_ex(g_reflectionExceptionPtr, 0,
                                 "Function %s() does not exist", Z_STRVAL_P(reference));
         return;
      }
      efree(lcname);
   }
      ce = fptr->common.scope;
      break;

   case IS_ARRAY: {
      zval *classref;
      zval *method;
      size_t lcname_len;
      char *lcname;

      if (((classref =zend_hash_index_find(Z_ARRVAL_P(reference), 0)) == nullptr)
          || ((method = zend_hash_index_find(Z_ARRVAL_P(reference), 1)) == nullptr))
      {
         _DO_THROW("Expected array($object, $method) or array($classname, $method)");
         /* returns out of this function */
      }

      if (Z_TYPE_P(classref) == IS_OBJECT) {
         ce = Z_OBJCE_P(classref);
      } else {
         convert_to_string_ex(classref);
         if ((ce = zend_lookup_class(Z_STR_P(classref))) == nullptr) {
            zend_throw_exception_ex(g_reflectionExceptionPtr, 0,
                                    "Class %s does not exist", Z_STRVAL_P(classref));
            return;
         }
      }

      convert_to_string_ex(method);
      lcname_len = Z_STRLEN_P(method);
      lcname = zend_str_tolower_dup(Z_STRVAL_P(method), lcname_len);
      if (ce == zend_ce_closure && Z_TYPE_P(classref) == IS_OBJECT
          && (lcname_len == sizeof(ZEND_INVOKE_FUNC_NAME)-1)
          && memcmp(lcname, ZEND_INVOKE_FUNC_NAME, sizeof(ZEND_INVOKE_FUNC_NAME)-1) == 0
          && (fptr = zend_get_closure_invoke_method(Z_OBJ_P(classref))) != nullptr)
      {
         /* nothing to do. don't set is_closure since is the invoke handler,
                  not the closure itself */
      } else if ((fptr = reinterpret_cast<zend_function *>(zend_hash_str_find_ptr(&ce->function_table, lcname, lcname_len))) == nullptr) {
         efree(lcname);
         zend_throw_exception_ex(g_reflectionExceptionPtr, 0,
                                 "Method %s::%s() does not exist", ZSTR_VAL(ce->name), Z_STRVAL_P(method));
         return;
      }
      efree(lcname);
   }
      break;

   case IS_OBJECT: {
      ce = Z_OBJCE_P(reference);

      if (instanceof_function(ce, zend_ce_closure)) {
         fptr = const_cast<zend_function *>(zend_get_closure_method_def(reference));
         Z_ADDREF_P(reference);
         is_closure = 1;
      } else if ((fptr = reinterpret_cast<zend_function *>(zend_hash_str_find_ptr(&ce->function_table, ZEND_INVOKE_FUNC_NAME, sizeof(ZEND_INVOKE_FUNC_NAME)))) == nullptr) {
         zend_throw_exception_ex(g_reflectionExceptionPtr, 0,
                                 "Method %s::%s() does not exist", ZSTR_VAL(ce->name), ZEND_INVOKE_FUNC_NAME);
         return;
      }
   }
      break;

   default:
      _DO_THROW("The parameter class is expected to be either a string, an array(class, method) or a callable object");
      /* returns out of this function */
   }

   /* Now, search for the parameter */
   arg_info = fptr->common.arg_info;
   num_args = fptr->common.num_args;
   if (fptr->common.fn_flags & ZEND_ACC_VARIADIC) {
      num_args++;
   }
   if (Z_TYPE_P(parameter) == IS_LONG) {
      position= (int)Z_LVAL_P(parameter);
      if (position < 0 || (uint32_t)position >= num_args) {
         if (fptr->common.fn_flags & ZEND_ACC_CALL_VIA_TRAMPOLINE) {
            if (fptr->type != ZEND_OVERLOADED_FUNCTION) {
               zend_string_release_ex(fptr->common.function_name, 0);
            }
            zend_free_trampoline(fptr);
         }
         if (is_closure) {
            zval_ptr_dtor(reference);
         }
         _DO_THROW("The parameter specified by its offset could not be found");
         /* returns out of this function */
      }
   } else {
      uint32_t i;

      position= -1;
      convert_to_string_ex(parameter);
      if (fptr->type == ZEND_INTERNAL_FUNCTION &&
          !(fptr->common.fn_flags & ZEND_ACC_USER_ARG_INFO)) {
         for (i = 0; i < num_args; i++) {
            if (arg_info[i].name) {
               if (strcmp(((zend_internal_arg_info*)arg_info)[i].name, Z_STRVAL_P(parameter)) == 0) {
                  position= i;
                  break;
               }

            }
         }
      } else {
         for (i = 0; i < num_args; i++) {
            if (arg_info[i].name) {
               if (strcmp(ZSTR_VAL(arg_info[i].name), Z_STRVAL_P(parameter)) == 0) {
                  position= i;
                  break;
               }
            }
         }
      }
      if (position == -1) {
         if (fptr->common.fn_flags & ZEND_ACC_CALL_VIA_TRAMPOLINE) {
            if (fptr->type != ZEND_OVERLOADED_FUNCTION) {
               zend_string_release_ex(fptr->common.function_name, 0);
            }
            zend_free_trampoline(fptr);
         }
         if (is_closure) {
            zval_ptr_dtor(reference);
         }
         _DO_THROW("The parameter specified by its name could not be found");
         /* returns out of this function */
      }
   }

   if (arg_info[position].name) {
      if (fptr->type == ZEND_INTERNAL_FUNCTION &&
          !(fptr->common.fn_flags & ZEND_ACC_USER_ARG_INFO)) {
         ZVAL_STRING(&name, ((zend_internal_arg_info*)arg_info)[position].name);
      } else {
         ZVAL_STR_COPY(&name, arg_info[position].name);
      }
   } else {
      ZVAL_NULL(&name);
   }
   reflection_update_property_name(object, &name);

   ref = (parameter_reference*) emalloc(sizeof(parameter_reference));
   ref->argInfo = &arg_info[position];
   ref->offset = (uint32_t)position;
   ref->required = position < static_cast<int>(fptr->common.required_num_args);
   ref->fptr = fptr;
   /* TODO: copy fptr */
   intern->ptr = ref;
   intern->refType = REF_TYPE_PARAMETER;
   intern->ce = ce;
   if (reference && is_closure) {
      ZVAL_COPY_VALUE(&intern->obj, reference);
   }
}

ZEND_METHOD(reflection_parameter, __toString)
{
   reflection_object *intern;
   parameter_reference *param;
   smart_str str = {0};

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(param, parameter_reference *);
   parameter_string(&str, param->fptr, param->argInfo, param->offset, param->required, const_cast<char *>(""));
   RETURN_STR(smart_str_extract(&str));
}

ZEND_METHOD(reflection_parameter, getName)
{
   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   default_get_name(getThis(), return_value);
}

ZEND_METHOD(reflection_parameter, getDeclaringFunction)
{
   reflection_object *intern;
   parameter_reference *param;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(param, parameter_reference *);

   if (!param->fptr->common.scope) {
      reflection_function_factory(copy_function(param->fptr), Z_ISUNDEF(intern->obj)? nullptr : &intern->obj, return_value);
   } else {
      reflection_method_factory(param->fptr->common.scope, copy_function(param->fptr), Z_ISUNDEF(intern->obj)? nullptr : &intern->obj, return_value);
   }
}

ZEND_METHOD(reflection_parameter, getDeclaringClass)
{
   reflection_object *intern;
   parameter_reference *param;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(param, parameter_reference *);

   if (param->fptr->common.scope) {
      zend_reflection_class_factory(param->fptr->common.scope, return_value);
   }
}

ZEND_METHOD(reflection_parameter, getClass)
{
   reflection_object *intern;
   parameter_reference *param;
   zend_class_entry *ce;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(param, parameter_reference *);

   if (ZEND_TYPE_IS_CLASS(param->argInfo->type)) {
      /* Class name is stored as a string, we might also get "self" or "parent"
       * - For "self", simply use the function scope. If scope is nullptr then
       *   the function is global and thus self does not make any sense
       *
       * - For "parent", use the function scope's parent. If scope is nullptr then
       *   the function is global and thus parent does not make any sense.
       *   If the parent is nullptr then the class does not extend anything and
       *   thus parent does not make any sense, either.
       *
       * TODO: Think about moving these checks to the compiler or some sort of
       * lint-mode.
       */
      zend_string *class_name;

      class_name = ZEND_TYPE_NAME(param->argInfo->type);
      if (0 == zend_binary_strcasecmp(ZSTR_VAL(class_name), ZSTR_LEN(class_name), "self", sizeof("self")- 1)) {
         ce = param->fptr->common.scope;
         if (!ce) {
            zend_throw_exception_ex(g_reflectionExceptionPtr, 0,
                                    "Parameter uses 'self' as type hint but function is not a class member!");
            return;
         }
      } else if (0 == zend_binary_strcasecmp(ZSTR_VAL(class_name), ZSTR_LEN(class_name), "parent", sizeof("parent")- 1)) {
         ce = param->fptr->common.scope;
         if (!ce) {
            zend_throw_exception_ex(g_reflectionExceptionPtr, 0,
                                    "Parameter uses 'parent' as type hint but function is not a class member!");
            return;
         }
         if (!ce->parent) {
            zend_throw_exception_ex(g_reflectionExceptionPtr, 0,
                                    "Parameter uses 'parent' as type hint although class does not have a parent!");
            return;
         }
         ce = ce->parent;
      } else {
         ce = zend_lookup_class(class_name);
         if (!ce) {
            zend_throw_exception_ex(g_reflectionExceptionPtr, 0,
                                    "Class %s does not exist", ZSTR_VAL(class_name));
            return;
         }
      }
      zend_reflection_class_factory(ce, return_value);
   }
}

ZEND_METHOD(reflection_parameter, hasType)
{
   reflection_object *intern;
   parameter_reference *param;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(param, parameter_reference *);

   RETVAL_BOOL(ZEND_TYPE_IS_SET(param->argInfo->type));
}

ZEND_METHOD(reflection_parameter, getType)
{
   reflection_object *intern;
   parameter_reference *param;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(param, parameter_reference *);

   if (!ZEND_TYPE_IS_SET(param->argInfo->type)) {
      RETURN_NULL();
   }
   reflection_type_factory(copy_function(param->fptr), Z_ISUNDEF(intern->obj)? nullptr : &intern->obj, param->argInfo, return_value);
}

ZEND_METHOD(reflection_parameter, isArray)
{
   reflection_object *intern;
   parameter_reference *param;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(param, parameter_reference *);
   RETVAL_BOOL(ZEND_TYPE_CODE(param->argInfo->type) == IS_ARRAY);
}

ZEND_METHOD(reflection_parameter, isCallable)
{
   reflection_object *intern;
   parameter_reference *param;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(param, parameter_reference *);

   RETVAL_BOOL(ZEND_TYPE_CODE(param->argInfo->type) == IS_CALLABLE);
}

ZEND_METHOD(reflection_parameter, allowsNull)
{
   reflection_object *intern;
   parameter_reference *param;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(param, parameter_reference *);

   RETVAL_BOOL(ZEND_TYPE_ALLOW_NULL(param->argInfo->type));
}

ZEND_METHOD(reflection_parameter, isPassedByReference)
{
   reflection_object *intern;
   parameter_reference *param;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(param, parameter_reference *);

   RETVAL_BOOL(param->argInfo->pass_by_reference);
}

ZEND_METHOD(reflection_parameter, canBePassedByValue)
{
   reflection_object *intern;
   parameter_reference *param;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(param, parameter_reference *);

   /* true if it's ZEND_SEND_BY_VAL or ZEND_SEND_PREFER_REF */
   RETVAL_BOOL(param->argInfo->pass_by_reference != ZEND_SEND_BY_REF);
}

ZEND_METHOD(reflection_parameter, getPosition)
{
   reflection_object *intern;
   parameter_reference *param;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(param, parameter_reference *);

   RETVAL_LONG(param->offset);
}

ZEND_METHOD(reflection_parameter, isOptional)
{
   reflection_object *intern;
   parameter_reference *param;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(param, parameter_reference *);

   RETVAL_BOOL(!param->required);
}

ZEND_METHOD(reflection_parameter, isDefaultValueAvailable)
{
   reflection_object *intern;
   parameter_reference *param;
   zend_op *precv;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(param, parameter_reference *);

   if (param->fptr->type != ZEND_USER_FUNCTION)
   {
      RETURN_FALSE;
   }

   precv = get_recv_op(reinterpret_cast<zend_op_array*>(param->fptr), param->offset);
   if (!precv || precv->opcode != ZEND_RECV_INIT || precv->op2_type == IS_UNUSED) {
      RETURN_FALSE;
   }
   RETURN_TRUE;
}

ZEND_METHOD(reflection_parameter, getDefaultValue)
{
   parameter_reference *param;
   zend_op *precv;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }

   param = reflection_param_get_default_param(INTERNAL_FUNCTION_PARAM_PASSTHRU);
   if (!param) {
      return;
   }

   precv = reflection_param_get_default_precv(INTERNAL_FUNCTION_PARAM_PASSTHRU, param);
   if (!precv) {
      return;
   }

   ZVAL_COPY(return_value, RT_CONSTANT(precv, precv->op2));
   if (Z_TYPE_P(return_value) == IS_CONSTANT_AST) {
      zval_update_constant_ex(return_value, param->fptr->common.scope);
   }
}

ZEND_METHOD(reflection_parameter, isDefaultValueConstant)
{
   zend_op *precv;
   parameter_reference *param;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }

   param = reflection_param_get_default_param(INTERNAL_FUNCTION_PARAM_PASSTHRU);
   if (!param) {
      RETURN_FALSE;
   }

   precv = reflection_param_get_default_precv(INTERNAL_FUNCTION_PARAM_PASSTHRU, param);
   if (precv && Z_TYPE_P(RT_CONSTANT(precv, precv->op2)) == IS_CONSTANT_AST) {
      zend_ast *ast = Z_ASTVAL_P(RT_CONSTANT(precv, precv->op2));

      if (ast->kind == ZEND_AST_CONSTANT
          || ast->kind == ZEND_AST_CONSTANT_CLASS) {
         RETURN_TRUE;
      }
   }

   RETURN_FALSE;
}

ZEND_METHOD(reflection_parameter, getDefaultValueConstantName)
{
   zend_op *precv;
   parameter_reference *param;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }

   param = reflection_param_get_default_param(INTERNAL_FUNCTION_PARAM_PASSTHRU);
   if (!param) {
      return;
   }

   precv = reflection_param_get_default_precv(INTERNAL_FUNCTION_PARAM_PASSTHRU, param);
   if (precv && Z_TYPE_P(RT_CONSTANT(precv, precv->op2)) == IS_CONSTANT_AST) {
      zend_ast *ast = Z_ASTVAL_P(RT_CONSTANT(precv, precv->op2));

      if (ast->kind == ZEND_AST_CONSTANT) {
         RETURN_STR_COPY(zend_ast_get_constant_name(ast));
      } else if (ast->kind == ZEND_AST_CONSTANT_CLASS) {
         RETURN_STRINGL("__CLASS__", sizeof("__CLASS__")-1);
      }
   }
}

ZEND_METHOD(reflection_parameter, isVariadic)
{
   reflection_object *intern;
   parameter_reference *param;
   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(param, parameter_reference *);
   RETVAL_BOOL(param->argInfo->is_variadic);
}

ZEND_METHOD(reflection_type, allowsNull)
{
   reflection_object *intern;
   type_reference *param;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(param, type_reference *);

   RETVAL_BOOL(ZEND_TYPE_ALLOW_NULL(param->argInfo->type));
}

ZEND_METHOD(reflection_type, isBuiltin)
{
   reflection_object *intern;
   type_reference *param;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(param, type_reference *);
   RETVAL_BOOL(ZEND_TYPE_IS_CODE(param->argInfo->type));
}

namespace {
zend_string *reflection_type_name(type_reference *param)
{
   if (ZEND_TYPE_IS_CLASS(param->argInfo->type)) {
      return zend_string_copy(ZEND_TYPE_NAME(param->argInfo->type));
   } else {
      char *name = zend_get_type_by_const(ZEND_TYPE_CODE(param->argInfo->type));
      return zend_string_init(name, strlen(name), 0);
   }
}
}

ZEND_METHOD(reflection_type, __toString)
{
   reflection_object *intern;
   type_reference *param;
   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(param, type_reference *);
   RETURN_STR(reflection_type_name(param));
}

ZEND_METHOD(reflection_named_type, getName)
{
   reflection_object *intern;
   type_reference *param;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(param, type_reference *);

   RETURN_STR(reflection_type_name(param));
}

ZEND_METHOD(reflection_method, export)
{
   reflection_export(INTERNAL_FUNCTION_PARAM_PASSTHRU, g_reflectionMethodPtr, 2);
}

ZEND_METHOD(reflection_method, __construct)
{
   zval name, *classname;
   zval *object, *orig_obj;
   reflection_object *intern;
   char *lcname;
   zend_class_entry *ce;
   zend_function *mptr;
   char *name_str, *tmp;
   size_t name_len, tmp_len;
   zval ztmp;

   if (zend_parse_parameters_ex(ZEND_PARSE_PARAMS_QUIET, ZEND_NUM_ARGS(), "zs", &classname, &name_str, &name_len) == FAILURE) {
      if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "s", &name_str, &name_len) == FAILURE) {
         return;
      }

      if ((tmp = strstr(name_str, "::")) == nullptr) {
         zend_throw_exception_ex(g_reflectionExceptionPtr, 0,
                                 "Invalid method name %s", name_str);
         return;
      }
      classname = &ztmp;
      tmp_len = tmp - name_str;
      ZVAL_STRINGL(classname, name_str, tmp_len);
      name_len = name_len - (tmp_len + 2);
      name_str = tmp + 2;
      orig_obj = nullptr;
   } else if (Z_TYPE_P(classname) == IS_OBJECT) {
      orig_obj = classname;
   } else {
      orig_obj = nullptr;
   }

   object = getThis();
   intern = Z_REFLECTION_P(object);

   /* Find the class entry */
   switch (Z_TYPE_P(classname)) {
   case IS_STRING:
      if ((ce = zend_lookup_class(Z_STR_P(classname))) == nullptr) {
         if (!EG(exception)) {
            zend_throw_exception_ex(g_reflectionExceptionPtr, 0,
                                    "Class %s does not exist", Z_STRVAL_P(classname));
         }
         if (classname == &ztmp) {
            zval_ptr_dtor_str(&ztmp);
         }
         return;
      }
      break;

   case IS_OBJECT:
      ce = Z_OBJCE_P(classname);
      break;

   default:
      if (classname == &ztmp) {
         zval_ptr_dtor_str(&ztmp);
      }
      _DO_THROW("The parameter class is expected to be either a string or an object");
      /* returns out of this function */
   }

   if (classname == &ztmp) {
      zval_ptr_dtor_str(&ztmp);
   }

   lcname = zend_str_tolower_dup(name_str, name_len);

   if (ce == zend_ce_closure && orig_obj && (name_len == sizeof(ZEND_INVOKE_FUNC_NAME)-1)
       && memcmp(lcname, ZEND_INVOKE_FUNC_NAME, sizeof(ZEND_INVOKE_FUNC_NAME)-1) == 0
       && (mptr = zend_get_closure_invoke_method(Z_OBJ_P(orig_obj))) != nullptr)
   {
      /* do nothing, mptr already set */
   } else if ((mptr = reinterpret_cast<zend_function *>(zend_hash_str_find_ptr(&ce->function_table, lcname, name_len))) == nullptr) {
      efree(lcname);
      zend_throw_exception_ex(g_reflectionExceptionPtr, 0,
                              "Method %s::%s() does not exist", ZSTR_VAL(ce->name), name_str);
      return;
   }
   efree(lcname);

   ZVAL_STR_COPY(&name, mptr->common.scope->name);
   reflection_update_property_class(object, &name);
   ZVAL_STR_COPY(&name, mptr->common.function_name);
   reflection_update_property_name(object, &name);
   intern->ptr = mptr;
   intern->refType = REF_TYPE_FUNCTION;
   intern->ce = ce;
}

ZEND_METHOD(reflection_method, __toString)
{
   reflection_object *intern;
   zend_function *mptr;
   smart_str str = {0};

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(mptr, zend_function *);
   function_string(&str, mptr, intern->ce, const_cast<char *>(""));
   RETURN_STR(smart_str_extract(&str));
}

ZEND_METHOD(reflection_method, getClosure)
{
   reflection_object *intern;
   zval *obj;
   zend_function *mptr;

   GET_REFLECTION_OBJECT_PTR(mptr, zend_function *);

   if (mptr->common.fn_flags & ZEND_ACC_STATIC)  {
      zend_create_fake_closure(return_value, mptr, mptr->common.scope, mptr->common.scope, nullptr);
   } else {
      if (zend_parse_parameters(ZEND_NUM_ARGS(), "o", &obj) == FAILURE) {
         return;
      }

      if (!instanceof_function(Z_OBJCE_P(obj), mptr->common.scope)) {
         _DO_THROW("Given object is not an instance of the class this method was declared in");
         /* Returns from this function */
      }

      /* This is an original closure object and __invoke is to be called. */
      if (Z_OBJCE_P(obj) == zend_ce_closure &&
          (mptr->internal_function.fn_flags & ZEND_ACC_CALL_VIA_TRAMPOLINE))
      {
         ZVAL_COPY(return_value, obj);
      } else {
         zend_create_fake_closure(return_value, mptr, mptr->common.scope, Z_OBJCE_P(obj), obj);
      }
   }
}

namespace {
void reflection_method_invoke(INTERNAL_FUNCTION_PARAMETERS, int variadic)
{
   zval retval;
   zval *params = nullptr, *val, *object;
   reflection_object *intern;
   zend_function *mptr;
   int i, argc = 0, result;
   zend_fcall_info fci;
   zend_fcall_info_cache fcc;
   zend_class_entry *obj_ce;
   zval *param_array;

   GET_REFLECTION_OBJECT_PTR(mptr, zend_function *);

   if (mptr->common.fn_flags & ZEND_ACC_ABSTRACT) {
      zend_throw_exception_ex(g_reflectionMethodPtr, 0,
                              "Trying to invoke abstract method %s::%s()",
                              ZSTR_VAL(mptr->common.scope->name), ZSTR_VAL(mptr->common.function_name));
      return;
   }

   if (!(mptr->common.fn_flags & ZEND_ACC_PUBLIC) && intern->ignoreVisibility == 0) {
      zend_throw_exception_ex(g_reflectionMethodPtr, 0,
                              "Trying to invoke %s method %s::%s() from scope %s",
                              mptr->common.fn_flags & ZEND_ACC_PROTECTED ? "protected" : "private",
                              ZSTR_VAL(mptr->common.scope->name), ZSTR_VAL(mptr->common.function_name),
                              ZSTR_VAL(Z_OBJCE_P(getThis())->name));
      return;
   }

   if (variadic) {
      if (zend_parse_parameters(ZEND_NUM_ARGS(), "o!*", &object, &params, &argc) == FAILURE) {
         return;
      }
   } else {
      if (zend_parse_parameters(ZEND_NUM_ARGS(), "o!a", &object, &param_array) == FAILURE) {
         return;
      }

      argc = zend_hash_num_elements(Z_ARRVAL_P(param_array));

      params = reinterpret_cast<zval *>(safe_emalloc(sizeof(zval), argc, 0));
      argc = 0;
      ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(param_array), val) {
         ZVAL_COPY(&params[argc], val);
         argc++;
      } ZEND_HASH_FOREACH_END();
   }

   /* In case this is a static method, we should'nt pass an object_ptr
    * (which is used as calling context aka $this). We can thus ignore the
    * first parameter.
    *
    * Else, we verify that the given object is an instance of the class.
    */
   if (mptr->common.fn_flags & ZEND_ACC_STATIC) {
      object = nullptr;
      obj_ce = mptr->common.scope;
   } else {
      if (!object) {
         zend_throw_exception_ex(g_reflectionExceptionPtr, 0,
                                 "Trying to invoke non static method %s::%s() without an object",
                                 ZSTR_VAL(mptr->common.scope->name), ZSTR_VAL(mptr->common.function_name));
         return;
      }

      obj_ce = Z_OBJCE_P(object);

      if (!instanceof_function(obj_ce, mptr->common.scope)) {
         if (!variadic) {
            efree(params);
         }
         _DO_THROW("Given object is not an instance of the class this method was declared in");
         /* Returns from this function */
      }
   }

   fci.size = sizeof(fci);
   ZVAL_UNDEF(&fci.function_name);
   fci.object = object ? Z_OBJ_P(object) : nullptr;
   fci.retval = &retval;
   fci.param_count = argc;
   fci.params = params;
   fci.no_separation = 1;

   fcc.function_handler = mptr;
   fcc.called_scope = intern->ce;
   fcc.object = object ? Z_OBJ_P(object) : nullptr;

   /*
    * Copy the zend_function when calling via handler (e.g. Closure::__invoke())
    */
   if ((mptr->internal_function.fn_flags & ZEND_ACC_CALL_VIA_TRAMPOLINE)) {
      fcc.function_handler = copy_function(mptr);
   }

   result = zend_call_function(&fci, &fcc);

   if (!variadic) {
      for (i = 0; i < argc; i++) {
         zval_ptr_dtor(&params[i]);
      }
      efree(params);
   }

   if (result == FAILURE) {
      zend_throw_exception_ex(g_reflectionExceptionPtr, 0,
                              "Invocation of method %s::%s() failed", ZSTR_VAL(mptr->common.scope->name), ZSTR_VAL(mptr->common.function_name));
      return;
   }

   if (Z_TYPE(retval) != IS_UNDEF) {
      if (Z_ISREF(retval)) {
         zend_unwrap_reference(&retval);
      }
      ZVAL_COPY_VALUE(return_value, &retval);
   }
}
}

ZEND_METHOD(reflection_method, invoke)
{
   reflection_method_invoke(INTERNAL_FUNCTION_PARAM_PASSTHRU, 1);
}

ZEND_METHOD(reflection_method, invokeArgs)
{
   reflection_method_invoke(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0);
}

ZEND_METHOD(reflection_method, isFinal)
{
   function_check_flag(INTERNAL_FUNCTION_PARAM_PASSTHRU, ZEND_ACC_FINAL);
}

ZEND_METHOD(reflection_method, isAbstract)
{
   function_check_flag(INTERNAL_FUNCTION_PARAM_PASSTHRU, ZEND_ACC_ABSTRACT);
}

ZEND_METHOD(reflection_method, isPublic)
{
   function_check_flag(INTERNAL_FUNCTION_PARAM_PASSTHRU, ZEND_ACC_PUBLIC);
}

ZEND_METHOD(reflection_method, isPrivate)
{
   function_check_flag(INTERNAL_FUNCTION_PARAM_PASSTHRU, ZEND_ACC_PRIVATE);
}

ZEND_METHOD(reflection_method, isProtected)
{
   function_check_flag(INTERNAL_FUNCTION_PARAM_PASSTHRU, ZEND_ACC_PROTECTED);
}

ZEND_METHOD(reflection_method, isStatic)
{
   function_check_flag(INTERNAL_FUNCTION_PARAM_PASSTHRU, ZEND_ACC_STATIC);
}

ZEND_METHOD(reflection_function, isDeprecated)
{
   function_check_flag(INTERNAL_FUNCTION_PARAM_PASSTHRU, ZEND_ACC_DEPRECATED);
}

ZEND_METHOD(reflection_function, isGenerator)
{
   function_check_flag(INTERNAL_FUNCTION_PARAM_PASSTHRU, ZEND_ACC_GENERATOR);
}

ZEND_METHOD(reflection_function, isVariadic)
{
   function_check_flag(INTERNAL_FUNCTION_PARAM_PASSTHRU, ZEND_ACC_VARIADIC);
}

ZEND_METHOD(reflection_function, inNamespace)
{
   zval *name;
   const char *backslash;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   if ((name = get_default_load_name(getThis())) == nullptr) {
      RETURN_FALSE;
   }
   if (Z_TYPE_P(name) == IS_STRING
       && (backslash = reinterpret_cast<const char *>(zend_memrchr(Z_STRVAL_P(name), '\\', Z_STRLEN_P(name))))
       && backslash > Z_STRVAL_P(name))
   {
      RETURN_TRUE;
   }
   RETURN_FALSE;
}

ZEND_METHOD(reflection_function, getNamespaceName)
{
   zval *name;
   const char *backslash;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   if ((name = get_default_load_name(getThis())) == nullptr) {
      RETURN_FALSE;
   }
   if (Z_TYPE_P(name) == IS_STRING
       && (backslash = reinterpret_cast<const char *>(zend_memrchr(Z_STRVAL_P(name), '\\', Z_STRLEN_P(name))))
       && backslash > Z_STRVAL_P(name))
   {
      RETURN_STRINGL(Z_STRVAL_P(name), backslash - Z_STRVAL_P(name));
   }
   RETURN_EMPTY_STRING();
}

ZEND_METHOD(reflection_function, getShortName)
{
   zval *name;
   const char *backslash;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   if ((name = get_default_load_name(getThis())) == nullptr) {
      RETURN_FALSE;
   }
   if (Z_TYPE_P(name) == IS_STRING
       && (backslash = reinterpret_cast<const char *>(zend_memrchr(Z_STRVAL_P(name), '\\', Z_STRLEN_P(name))))
       && backslash > Z_STRVAL_P(name))
   {
      RETURN_STRINGL(backslash + 1, Z_STRLEN_P(name) - (backslash - Z_STRVAL_P(name) + 1));
   }
   ZVAL_COPY_DEREF(return_value, name);
}

ZEND_METHOD(reflection_function, hasReturnType)
{
   reflection_object *intern;
   zend_function *fptr;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(fptr, zend_function *);
   RETVAL_BOOL(fptr->op_array.fn_flags & ZEND_ACC_HAS_RETURN_TYPE);
}

ZEND_METHOD(reflection_function, getReturnType)
{
   reflection_object *intern;
   zend_function *fptr;
   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(fptr, zend_function *);
   if (!(fptr->op_array.fn_flags & ZEND_ACC_HAS_RETURN_TYPE)) {
      RETURN_NULL();
   }
   reflection_type_factory(copy_function(fptr), Z_ISUNDEF(intern->obj)? nullptr : &intern->obj, &fptr->common.arg_info[-1], return_value);
}

ZEND_METHOD(reflection_method, isConstructor)
{
   reflection_object *intern;
   zend_function *mptr;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(mptr, zend_function *);
   /* we need to check if the ctor is the ctor of the class level we we
    * looking at since we might be looking at an inherited old style ctor
    * defined in base class. */
   RETURN_BOOL(mptr->common.fn_flags & ZEND_ACC_CTOR && intern->ce->constructor && intern->ce->constructor->common.scope == mptr->common.scope);
}

ZEND_METHOD(reflection_method, isDestructor)
{
   reflection_object *intern;
   zend_function *mptr;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(mptr, zend_function *);
   RETURN_BOOL(mptr->common.fn_flags & ZEND_ACC_DTOR);
}

ZEND_METHOD(reflection_method, getModifiers)
{
   reflection_object *intern;
   zend_function *mptr;
   uint32_t keep_flags = ZEND_ACC_PPP_MASK | ZEND_ACC_IMPLICIT_PUBLIC
         | ZEND_ACC_STATIC | ZEND_ACC_ABSTRACT | ZEND_ACC_FINAL;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(mptr, zend_function *);

   RETURN_LONG((mptr->common.fn_flags & keep_flags));
}

ZEND_METHOD(reflection_method, getDeclaringClass)
{
   reflection_object *intern;
   zend_function *mptr;
   GET_REFLECTION_OBJECT_PTR(mptr, zend_function *);
   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   zend_reflection_class_factory(mptr->common.scope, return_value);
}

ZEND_METHOD(reflection_method, getPrototype)
{
   reflection_object *intern;
   zend_function *mptr;
   GET_REFLECTION_OBJECT_PTR(mptr, zend_function *);
   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   if (!mptr->common.prototype) {
      zend_throw_exception_ex(g_reflectionExceptionPtr, 0,
                              "Method %s::%s does not have a prototype", ZSTR_VAL(intern->ce->name), ZSTR_VAL(mptr->common.function_name));
      return;
   }
   reflection_method_factory(mptr->common.prototype->common.scope, mptr->common.prototype, nullptr, return_value);
}

ZEND_METHOD(reflection_method, setAccessible)
{
   reflection_object *intern;
   zend_bool visible;
   if (zend_parse_parameters(ZEND_NUM_ARGS(), "b", &visible) == FAILURE) {
      return;
   }
   intern = Z_REFLECTION_P(getThis());
   intern->ignoreVisibility = visible;
}

ZEND_METHOD(reflection_class_constant, __construct)
{
   zval *classname, *object, name, cname;
   zend_string *constname;
   reflection_object *intern;
   zend_class_entry *ce;
   zend_class_constant *constant = nullptr;

   if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "zS", &classname, &constname) == FAILURE) {
      return;
   }

   object = getThis();
   intern = Z_REFLECTION_P(object);

   /* Find the class entry */
   switch (Z_TYPE_P(classname)) {
   case IS_STRING:
      if ((ce = zend_lookup_class(Z_STR_P(classname))) == nullptr) {
         zend_throw_exception_ex(g_reflectionExceptionPtr, 0,
                                 "Class %s does not exist", Z_STRVAL_P(classname));
         return;
      }
      break;

   case IS_OBJECT:
      ce = Z_OBJCE_P(classname);
      break;

   default:
      _DO_THROW("The parameter class is expected to be either a string or an object");
      /* returns out of this function */
   }

   if ((constant = reinterpret_cast<zend_class_constant *>(zend_hash_find_ptr(&ce->constants_table, constname))) == nullptr) {
      zend_throw_exception_ex(g_reflectionExceptionPtr, 0, "Class Constant %s::%s does not exist", ZSTR_VAL(ce->name), ZSTR_VAL(constname));
      return;
   }

   ZVAL_STR_COPY(&name, constname);
   ZVAL_STR_COPY(&cname, ce->name);

   intern->ptr = constant;
   intern->refType = REF_TYPE_CLASS_CONSTANT;
   intern->ce = constant->ce;
   intern->ignoreVisibility = 0;
   reflection_update_property_name(object, &name);
   reflection_update_property_class(object, &cname);
}

ZEND_METHOD(reflection_class_constant, __toString)
{
   reflection_object *intern;
   zend_class_constant *ref;
   smart_str str = {0};
   zval name;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(ref, zend_class_constant *);
   default_get_name(getThis(), &name);
   class_const_string(&str, Z_STRVAL(name), ref, const_cast<char *>(""));
   zval_ptr_dtor(&name);
   RETURN_STR(smart_str_extract(&str));
}

ZEND_METHOD(reflection_class_constant, getName)
{
   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   default_get_name(getThis(), return_value);
}

namespace {
void class_constant_check_flag(INTERNAL_FUNCTION_PARAMETERS, int mask) /* {{{ */
{
   reflection_object *intern;
   zend_class_constant *ref;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(ref, zend_class_constant *);
   RETURN_BOOL(Z_ACCESS_FLAGS(ref->value) & mask);
}
} // anonymous namespace

ZEND_METHOD(reflection_class_constant, isPublic)
{
   class_constant_check_flag(INTERNAL_FUNCTION_PARAM_PASSTHRU, ZEND_ACC_PUBLIC | ZEND_ACC_IMPLICIT_PUBLIC);
}

ZEND_METHOD(reflection_class_constant, isPrivate)
{
   class_constant_check_flag(INTERNAL_FUNCTION_PARAM_PASSTHRU, ZEND_ACC_PRIVATE);
}

ZEND_METHOD(reflection_class_constant, isProtected)
{
   class_constant_check_flag(INTERNAL_FUNCTION_PARAM_PASSTHRU, ZEND_ACC_PROTECTED);
}

ZEND_METHOD(reflection_class_constant, getModifiers)
{
   reflection_object *intern;
   zend_class_constant *ref;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(ref, zend_class_constant *);

   RETURN_LONG(Z_ACCESS_FLAGS(ref->value));
}

ZEND_METHOD(reflection_class_constant, getValue)
{
   reflection_object *intern;
   zend_class_constant *ref;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(ref, zend_class_constant *);

   ZVAL_COPY_OR_DUP(return_value, &ref->value);
   if (Z_TYPE_P(return_value) == IS_CONSTANT_AST) {
      zval_update_constant_ex(return_value, ref->ce);
   }
}

ZEND_METHOD(reflection_class_constant, getDeclaringClass)
{
   reflection_object *intern;
   zend_class_constant *ref;
   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(ref, zend_class_constant *);
   zend_reflection_class_factory(ref->ce, return_value);
}

ZEND_METHOD(reflection_class_constant, getDocComment)
{
   reflection_object *intern;
   zend_class_constant *ref;
   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(ref, zend_class_constant *);
   if (ref->doc_comment) {
      RETURN_STR_COPY(ref->doc_comment);
   }
   RETURN_FALSE;
}

ZEND_METHOD(reflection_class, export)
{
   reflection_export(INTERNAL_FUNCTION_PARAM_PASSTHRU, g_reflectionClassPtr, 1);
}

namespace {
void reflection_class_object_ctor(INTERNAL_FUNCTION_PARAMETERS, int is_object)
{
   zval *argument;
   zval *object;
   zval classname;
   reflection_object *intern;
   zend_class_entry *ce;

   if (is_object) {
      if (zend_parse_parameters(ZEND_NUM_ARGS(), "o", &argument) == FAILURE) {
         return;
      }
   } else {
      if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &argument) == FAILURE) {
         return;
      }
   }

   object = getThis();
   intern = Z_REFLECTION_P(object);

   if (Z_TYPE_P(argument) == IS_OBJECT) {
      ZVAL_STR_COPY(&classname, Z_OBJCE_P(argument)->name);
      reflection_update_property_name(object, &classname);
      intern->ptr = Z_OBJCE_P(argument);
      if (is_object) {
         ZVAL_COPY(&intern->obj, argument);
      }
   } else {
      convert_to_string_ex(argument);
      if ((ce = zend_lookup_class(Z_STR_P(argument))) == nullptr) {
         if (!EG(exception)) {
            zend_throw_exception_ex(g_reflectionExceptionPtr, -1, "Class %s does not exist", Z_STRVAL_P(argument));
         }
         return;
      }

      ZVAL_STR_COPY(&classname, ce->name);
      reflection_update_property_name(object, &classname);

      intern->ptr = ce;
   }
   intern->refType = REF_TYPE_OTHER;
}
} // anonynous namespace

ZEND_METHOD(reflection_class, __construct)
{
   reflection_class_object_ctor(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0);
}

namespace {
void add_class_vars(zend_class_entry *ce, int statics, zval *return_value)
{
   zend_property_info *prop_info;
   zval *prop, prop_copy;
   zend_string *key;

   POLAR_HASH_FOREACH_STR_KEY_PTR_WITH_TYPE(&ce->properties_info, key, zend_property_info *, prop_info) {
      if (((prop_info->flags & ZEND_ACC_SHADOW) &&
           prop_info->ce != ce) ||
          ((prop_info->flags & ZEND_ACC_PROTECTED) &&
           !zend_check_protected(prop_info->ce, ce)) ||
          ((prop_info->flags & ZEND_ACC_PRIVATE) &&
           prop_info->ce != ce)) {
         continue;
      }
      prop = nullptr;
      if (statics && (prop_info->flags & ZEND_ACC_STATIC) != 0) {
         prop = &ce->default_static_members_table[prop_info->offset];
         ZVAL_DEINDIRECT(prop);
      } else if (!statics && (prop_info->flags & ZEND_ACC_STATIC) == 0) {
         prop = &ce->default_properties_table[OBJ_PROP_TO_NUM(prop_info->offset)];
      }
      if (!prop) {
         continue;
      }

      /* copy: enforce read only access */
      ZVAL_DEREF(prop);
      ZVAL_COPY_OR_DUP(&prop_copy, prop);

      /* this is necessary to make it able to work with default array
      * properties, returned to user */
      if (Z_TYPE(prop_copy) == IS_CONSTANT_AST) {
         if (UNEXPECTED(zval_update_constant_ex(&prop_copy, nullptr) != SUCCESS)) {
            return;
         }
      }

      zend_hash_update(Z_ARRVAL_P(return_value), key, &prop_copy);
   } POLAR_HASH_FOREACH_END();
}
} // anonymous namespace

ZEND_METHOD(reflection_class, getStaticProperties)
{
   reflection_object *intern;
   zend_class_entry *ce;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }

   GET_REFLECTION_OBJECT_PTR(ce, zend_class_entry *);

   if (UNEXPECTED(zend_update_class_constants(ce) != SUCCESS)) {
      return;
   }

   array_init(return_value);
   add_class_vars(ce, 1, return_value);
}

ZEND_METHOD(reflection_class, getStaticPropertyValue)
{
   reflection_object *intern;
   zend_class_entry *ce;
   zend_string *name;
   zval *prop, *def_value = nullptr;

   if (zend_parse_parameters(ZEND_NUM_ARGS(), "S|z", &name, &def_value) == FAILURE) {
      return;
   }

   GET_REFLECTION_OBJECT_PTR(ce, zend_class_entry *);

   if (UNEXPECTED(zend_update_class_constants(ce) != SUCCESS)) {
      return;
   }
   prop = zend_std_get_static_property(ce, name, 1);
   if (!prop) {
      if (def_value) {
         ZVAL_COPY(return_value, def_value);
      } else {
         zend_throw_exception_ex(g_reflectionExceptionPtr, 0,
                                 "Class %s does not have a property named %s", ZSTR_VAL(ce->name), ZSTR_VAL(name));
      }
      return;
   } else {
      ZVAL_COPY_DEREF(return_value, prop);
   }
}

ZEND_METHOD(reflection_class, setStaticPropertyValue)
{
   reflection_object *intern;
   zend_class_entry *ce;
   zend_string *name;
   zval *variable_ptr, *value;

   if (zend_parse_parameters(ZEND_NUM_ARGS(), "Sz", &name, &value) == FAILURE) {
      return;
   }

   GET_REFLECTION_OBJECT_PTR(ce, zend_class_entry *);

   if (UNEXPECTED(zend_update_class_constants(ce) != SUCCESS)) {
      return;
   }
   variable_ptr = zend_std_get_static_property(ce, name, 1);
   if (!variable_ptr) {
      zend_throw_exception_ex(g_reflectionExceptionPtr, 0,
                              "Class %s does not have a property named %s", ZSTR_VAL(ce->name), ZSTR_VAL(name));
      return;
   }
   ZVAL_DEREF(variable_ptr);
   zval_ptr_dtor(variable_ptr);
   ZVAL_COPY(variable_ptr, value);
}

ZEND_METHOD(reflection_class, getDefaultProperties)
{
   reflection_object *intern;
   zend_class_entry *ce;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(ce, zend_class_entry *);
   array_init(return_value);
   if (UNEXPECTED(zend_update_class_constants(ce) != SUCCESS)) {
      return;
   }
   add_class_vars(ce, 1, return_value);
   add_class_vars(ce, 0, return_value);
}

ZEND_METHOD(reflection_class, __toString)
{
   reflection_object *intern;
   zend_class_entry *ce;
   smart_str str = {0};

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(ce, zend_class_entry *);
   class_string(&str, ce, &intern->obj, const_cast<char *>(""));
   RETURN_STR(smart_str_extract(&str));
}

ZEND_METHOD(reflection_class, getName)
{
   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   default_get_name(getThis(), return_value);
}

ZEND_METHOD(reflection_class, isInternal)
{
   reflection_object *intern;
   zend_class_entry *ce;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(ce, zend_class_entry *);
   RETURN_BOOL(ce->type == ZEND_INTERNAL_CLASS);
}

ZEND_METHOD(reflection_class, isUserDefined)
{
   reflection_object *intern;
   zend_class_entry *ce;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(ce, zend_class_entry *);
   RETURN_BOOL(ce->type == ZEND_USER_CLASS);
}

ZEND_METHOD(reflection_class, isAnonymous)
{
   reflection_object *intern;
   zend_class_entry *ce;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(ce, zend_class_entry *);
   RETURN_BOOL(ce->ce_flags & ZEND_ACC_ANON_CLASS);
}

ZEND_METHOD(reflection_class, getFileName)
{
   reflection_object *intern;
   zend_class_entry *ce;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(ce, zend_class_entry *);
   if (ce->type == ZEND_USER_CLASS) {
      RETURN_STR_COPY(ce->info.user.filename);
   }
   RETURN_FALSE;
}

ZEND_METHOD(reflection_class, getStartLine)
{
   reflection_object *intern;
   zend_class_entry *ce;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(ce, zend_class_entry *);
   if (ce->type == ZEND_USER_CLASS) {
      RETURN_LONG(ce->info.user.line_start);
   }
   RETURN_FALSE;
}

ZEND_METHOD(reflection_class, getEndLine)
{
   reflection_object *intern;
   zend_class_entry *ce;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(ce, zend_class_entry *);
   if (ce->type == ZEND_USER_CLASS) {
      RETURN_LONG(ce->info.user.line_end);
   }
   RETURN_FALSE;
}

ZEND_METHOD(reflection_class, getDocComment)
{
   reflection_object *intern;
   zend_class_entry *ce;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(ce, zend_class_entry *);
   if (ce->type == ZEND_USER_CLASS && ce->info.user.doc_comment) {
      RETURN_STR_COPY(ce->info.user.doc_comment);
   }
   RETURN_FALSE;
}

ZEND_METHOD(reflection_class, getConstructor)
{
   reflection_object *intern;
   zend_class_entry *ce;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(ce, zend_class_entry *);

   if (ce->constructor) {
      reflection_method_factory(ce, ce->constructor, nullptr, return_value);
   } else {
      RETURN_NULL();
   }
}

ZEND_METHOD(reflection_class, hasMethod)
{
   reflection_object *intern;
   zend_class_entry *ce;
   char *name, *lc_name;
   size_t name_len;

   if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &name, &name_len) == FAILURE) {
      return;
   }

   GET_REFLECTION_OBJECT_PTR(ce, zend_class_entry *);
   lc_name = zend_str_tolower_dup(name, name_len);
   if ((ce == zend_ce_closure && (name_len == sizeof(ZEND_INVOKE_FUNC_NAME)-1)
        && memcmp(lc_name, ZEND_INVOKE_FUNC_NAME, sizeof(ZEND_INVOKE_FUNC_NAME)-1) == 0)
       || zend_hash_str_exists(&ce->function_table, lc_name, name_len)) {
      efree(lc_name);
      RETURN_TRUE;
   } else {
      efree(lc_name);
      RETURN_FALSE;
   }
}

ZEND_METHOD(reflection_class, getMethod)
{
   reflection_object *intern;
   zend_class_entry *ce;
   zend_function *mptr;
   zval obj_tmp;
   char *name, *lc_name;
   size_t name_len;

   if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &name, &name_len) == FAILURE) {
      return;
   }

   GET_REFLECTION_OBJECT_PTR(ce, zend_class_entry *);
   lc_name = zend_str_tolower_dup(name, name_len);
   if (ce == zend_ce_closure && !Z_ISUNDEF(intern->obj) && (name_len == sizeof(ZEND_INVOKE_FUNC_NAME)-1)
       && memcmp(lc_name, ZEND_INVOKE_FUNC_NAME, sizeof(ZEND_INVOKE_FUNC_NAME)-1) == 0
       && (mptr = zend_get_closure_invoke_method(Z_OBJ(intern->obj))) != nullptr)
   {
      /* don't assign closure_object since we only reflect the invoke handler
         method and not the closure definition itself */
      reflection_method_factory(ce, mptr, nullptr, return_value);
      efree(lc_name);
   } else if (ce == zend_ce_closure && Z_ISUNDEF(intern->obj) && (name_len == sizeof(ZEND_INVOKE_FUNC_NAME)-1)
              && memcmp(lc_name, ZEND_INVOKE_FUNC_NAME, sizeof(ZEND_INVOKE_FUNC_NAME)-1) == 0
              && object_init_ex(&obj_tmp, ce) == SUCCESS && (mptr = zend_get_closure_invoke_method(Z_OBJ(obj_tmp))) != nullptr) {
      /* don't assign closure_object since we only reflect the invoke handler
         method and not the closure definition itself */
      reflection_method_factory(ce, mptr, nullptr, return_value);
      zval_ptr_dtor(&obj_tmp);
      efree(lc_name);
   } else if ((mptr = reinterpret_cast<zend_function *>(zend_hash_str_find_ptr(&ce->function_table, lc_name, name_len))) != nullptr) {
      reflection_method_factory(ce, mptr, nullptr, return_value);
      efree(lc_name);
   } else {
      efree(lc_name);
      zend_throw_exception_ex(g_reflectionExceptionPtr, 0,
                              "Method %s does not exist", name);
      return;
   }
}

namespace {
void addmethod(zend_function *mptr, zend_class_entry *ce, zval *retval, zend_long filter, zval *obj)
{
   zval method;
   size_t len = ZSTR_LEN(mptr->common.function_name);
   zend_function *closure;
   if (mptr->common.fn_flags & filter) {
      if (ce == zend_ce_closure && obj && (len == sizeof(ZEND_INVOKE_FUNC_NAME)-1)
          && memcmp(ZSTR_VAL(mptr->common.function_name), ZEND_INVOKE_FUNC_NAME, sizeof(ZEND_INVOKE_FUNC_NAME)-1) == 0
          && (closure = zend_get_closure_invoke_method(Z_OBJ_P(obj))) != nullptr)
      {
         mptr = closure;
      }
      /* don't assign closure_object since we only reflect the invoke handler
         method and not the closure definition itself, even if we have a
         closure */
      reflection_method_factory(ce, mptr, nullptr, &method);
      add_next_index_zval(retval, &method);
   }
}

int addmethod_va(zval *el, int num_args, va_list args, zend_hash_key *hash_key)
{
   zend_function *mptr = (zend_function*)Z_PTR_P(el);
   zend_class_entry *ce = *va_arg(args, zend_class_entry**);
   zval *retval = va_arg(args, zval*);
   long filter = va_arg(args, long);
   zval *obj = va_arg(args, zval *);

   addmethod(mptr, ce, retval, filter, obj);
   return ZEND_HASH_APPLY_KEEP;
}

} // anonymous namespace

ZEND_METHOD(reflection_class, getMethods)
{
   reflection_object *intern;
   zend_class_entry *ce;
   zend_long filter = ZEND_ACC_PPP_MASK | ZEND_ACC_ABSTRACT | ZEND_ACC_FINAL | ZEND_ACC_STATIC;

   if (zend_parse_parameters(ZEND_NUM_ARGS(), "|l", &filter) == FAILURE) {
      return;
   }

   GET_REFLECTION_OBJECT_PTR(ce, zend_class_entry *);

   array_init(return_value);
   zend_hash_apply_with_arguments(&ce->function_table, (apply_func_args_t) addmethod_va, 4, &ce, return_value, filter, intern->obj);
   if (Z_TYPE(intern->obj) != IS_UNDEF && instanceof_function(ce, zend_ce_closure)) {
      zend_function *closure = zend_get_closure_invoke_method(Z_OBJ(intern->obj));
      if (closure) {
         addmethod(closure, ce, return_value, filter, &intern->obj);
         free_function(closure);
      }
   }
}

ZEND_METHOD(reflection_class, hasProperty)
{
   reflection_object *intern;
   zend_property_info *property_info;
   zend_class_entry *ce;
   zend_string *name;
   zval property;

   if (zend_parse_parameters(ZEND_NUM_ARGS(), "S", &name) == FAILURE) {
      return;
   }

   GET_REFLECTION_OBJECT_PTR(ce, zend_class_entry *);
   if ((property_info = reinterpret_cast<zend_property_info *>(zend_hash_find_ptr(&ce->properties_info, name))) != nullptr) {
      if (property_info->flags & ZEND_ACC_SHADOW) {
         RETURN_FALSE;
      }
      RETURN_TRUE;
   } else {
      if (Z_TYPE(intern->obj) != IS_UNDEF && Z_OBJ_HANDLER(intern->obj, has_property)) {
         ZVAL_STR_COPY(&property, name);
         if (Z_OBJ_HANDLER(intern->obj, has_property)(&intern->obj, &property, 2, nullptr)) {
            zval_ptr_dtor(&property);
            RETURN_TRUE;
         }
         zval_ptr_dtor(&property);
      }
      RETURN_FALSE;
   }
}

ZEND_METHOD(reflection_class, getProperty)
{
   reflection_object *intern;
   zend_class_entry *ce, *ce2;
   zend_property_info *property_info;
   zend_string *name, *classname;
   char *tmp, *str_name;
   size_t classname_len, str_name_len;

   if (zend_parse_parameters(ZEND_NUM_ARGS(), "S", &name) == FAILURE) {
      return;
   }

   GET_REFLECTION_OBJECT_PTR(ce, zend_class_entry *);
   if ((property_info = reinterpret_cast<zend_property_info *>(zend_hash_find_ptr(&ce->properties_info, name))) != nullptr) {
      if ((property_info->flags & ZEND_ACC_SHADOW) == 0) {
         reflection_property_factory(ce, name, property_info, return_value);
         return;
      }
   } else if (Z_TYPE(intern->obj) != IS_UNDEF) {
      /* Check for dynamic properties */
      if (zend_hash_exists(Z_OBJ_HT(intern->obj)->get_properties(&intern->obj), name)) {
         zend_property_info property_info_tmp;
         property_info_tmp.flags = ZEND_ACC_IMPLICIT_PUBLIC;
         property_info_tmp.name = name;
         property_info_tmp.doc_comment = nullptr;
         property_info_tmp.ce = ce;

         reflection_property_factory(ce, name, &property_info_tmp, return_value);
         return;
      }
   }
   str_name = ZSTR_VAL(name);
   if ((tmp = strstr(ZSTR_VAL(name), "::")) != nullptr) {
      classname_len = tmp - ZSTR_VAL(name);
      classname = zend_string_alloc(classname_len, 0);
      zend_str_tolower_copy(ZSTR_VAL(classname), ZSTR_VAL(name), classname_len);
      ZSTR_VAL(classname)[classname_len] = '\0';
      str_name_len = ZSTR_LEN(name) - (classname_len + 2);
      str_name = tmp + 2;

      ce2 = zend_lookup_class(classname);
      if (!ce2) {
         if (!EG(exception)) {
            zend_throw_exception_ex(g_reflectionExceptionPtr, -1, "Class %s does not exist", ZSTR_VAL(classname));
         }
         zend_string_release_ex(classname, 0);
         return;
      }
      zend_string_release_ex(classname, 0);

      if (!instanceof_function(ce, ce2)) {
         zend_throw_exception_ex(g_reflectionExceptionPtr, -1, "Fully qualified property name %s::%s does not specify a base class of %s", ZSTR_VAL(ce2->name), str_name, ZSTR_VAL(ce->name));
         return;
      }
      ce = ce2;

      if ((property_info = reinterpret_cast<zend_property_info *>(zend_hash_str_find_ptr(&ce->properties_info, str_name, str_name_len))) != nullptr && (property_info->flags & ZEND_ACC_SHADOW) == 0) {
         reflection_property_factory_str(ce, str_name, str_name_len, property_info, return_value);
         return;
      }
   }
   zend_throw_exception_ex(g_reflectionExceptionPtr, 0,
                           "Property %s does not exist", str_name);
}

namespace {
int addproperty(zval *el, int num_args, va_list args, zend_hash_key *hash_key)
{
   zval property;
   zend_property_info *pptr = (zend_property_info*)Z_PTR_P(el);
   zend_class_entry *ce = *va_arg(args, zend_class_entry**);
   zval *retval = va_arg(args, zval*);
   long filter = va_arg(args, long);

   if (pptr->flags	& ZEND_ACC_SHADOW) {
      return 0;
   }

   if (pptr->flags	& filter) {
      const char *class_name, *prop_name;
      size_t prop_name_len;
      zend_unmangle_property_name_ex(pptr->name, &class_name, &prop_name, &prop_name_len);
      reflection_property_factory_str(ce, prop_name, prop_name_len, pptr, &property);
      add_next_index_zval(retval, &property);
   }
   return 0;
}

int adddynproperty(zval *ptr, int num_args, va_list args, zend_hash_key *hash_key)
{
   zval property;
   zend_class_entry *ce = *va_arg(args, zend_class_entry**);
   zval *retval = va_arg(args, zval*);

   /* under some circumstances, the properties hash table may contain numeric
    * properties (e.g. when casting from array). This is a WONT FIX bug, at
    * least for the moment. Ignore these */
   if (hash_key->key == nullptr) {
      return 0;
   }

   if (ZSTR_VAL(hash_key->key)[0] == '\0') {
      return 0; /* non public cannot be dynamic */
   }
   if (zend_get_property_info(ce, hash_key->key, 1) == nullptr) {
      zend_property_info property_info;
      property_info.doc_comment = nullptr;
      property_info.flags = ZEND_ACC_IMPLICIT_PUBLIC;
      property_info.name = hash_key->key;
      property_info.ce = ce;
      property_info.offset = -1;
      reflection_property_factory(ce, hash_key->key, &property_info, &property);
      add_next_index_zval(retval, &property);
   }
   return 0;
}
} // anonymous namespace

ZEND_METHOD(reflection_class, getProperties)
{
   reflection_object *intern;
   zend_class_entry *ce;
   zend_long filter = ZEND_ACC_PPP_MASK | ZEND_ACC_STATIC;

   if (zend_parse_parameters(ZEND_NUM_ARGS(), "|l", &filter) == FAILURE) {
      return;
   }

   GET_REFLECTION_OBJECT_PTR(ce, zend_class_entry *);

   array_init(return_value);
   zend_hash_apply_with_arguments(&ce->properties_info, (apply_func_args_t) addproperty, 3, &ce, return_value, filter);

   if (Z_TYPE(intern->obj) != IS_UNDEF && (filter & ZEND_ACC_PUBLIC) != 0 && Z_OBJ_HT(intern->obj)->get_properties) {
      HashTable *properties = Z_OBJ_HT(intern->obj)->get_properties(&intern->obj);
      zend_hash_apply_with_arguments(properties, (apply_func_args_t) adddynproperty, 2, &ce, return_value);
   }
}

ZEND_METHOD(reflection_class, hasConstant)
{
   reflection_object *intern;
   zend_class_entry *ce;
   zend_string *name;

   if (zend_parse_parameters(ZEND_NUM_ARGS(), "S", &name) == FAILURE) {
      return;
   }

   GET_REFLECTION_OBJECT_PTR(ce, zend_class_entry *);
   if (zend_hash_exists(&ce->constants_table, name)) {
      RETURN_TRUE;
   } else {
      RETURN_FALSE;
   }
}

ZEND_METHOD(reflection_class, getConstants)
{
   reflection_object *intern;
   zend_class_entry *ce;
   zend_string *key;
   zend_class_constant *c;
   zval val;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(ce, zend_class_entry *);
   array_init(return_value);
   POLAR_HASH_FOREACH_STR_KEY_PTR_WITH_TYPE(&ce->constants_table, key, zend_class_constant *, c) {
      if (UNEXPECTED(zval_update_constant_ex(&c->value, ce) != SUCCESS)) {
         zend_array_destroy(Z_ARRVAL_P(return_value));
         RETURN_NULL();
      }
      ZVAL_COPY_OR_DUP(&val, &c->value);
      zend_hash_add_new(Z_ARRVAL_P(return_value), key, &val);
   } POLAR_HASH_FOREACH_END();
}

ZEND_METHOD(reflection_class, getReflectionConstants)
{
   reflection_object *intern;
   zend_class_entry *ce;
   zend_string *name;
   zend_class_constant *constant;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(ce, zend_class_entry *);
   array_init(return_value);
   POLAR_HASH_FOREACH_STR_KEY_PTR_WITH_TYPE(&ce->constants_table, name, zend_class_constant *, constant) {
      zval class_const;
      reflection_class_constant_factory(ce, name, constant, &class_const);
      zend_hash_next_index_insert(Z_ARRVAL_P(return_value), &class_const);
   } POLAR_HASH_FOREACH_END();
}

ZEND_METHOD(reflection_class, getConstant)
{
   reflection_object *intern;
   zend_class_entry *ce;
   zend_class_constant *c;
   zend_string *name;

   if (zend_parse_parameters(ZEND_NUM_ARGS(), "S", &name) == FAILURE) {
      return;
   }

   GET_REFLECTION_OBJECT_PTR(ce, zend_class_entry *);
   POLAR_HASH_FOREACH_PTR_WITH_TYPE(&ce->constants_table, zend_class_constant *, c) {
      if (UNEXPECTED(zval_update_constant_ex(&c->value, ce) != SUCCESS)) {
         return;
      }
   } POLAR_HASH_FOREACH_END();
   if ((c = reinterpret_cast<zend_class_constant *>(zend_hash_find_ptr(&ce->constants_table, name))) == nullptr) {
      RETURN_FALSE;
   }
   ZVAL_COPY_OR_DUP(return_value, &c->value);
}

ZEND_METHOD(reflection_class, getReflectionConstant)
{
   reflection_object *intern;
   zend_class_entry *ce;
   zend_class_constant *constant;
   zend_string *name;
   GET_REFLECTION_OBJECT_PTR(ce, zend_class_entry *);
   if (zend_parse_parameters(ZEND_NUM_ARGS(), "S", &name) == FAILURE) {
      return;
   }
   if ((constant = reinterpret_cast<zend_class_constant *>(zend_hash_find_ptr(&ce->constants_table, name))) == nullptr) {
      RETURN_FALSE;
   }
   reflection_class_constant_factory(ce, name, constant, return_value);
}

namespace {
void class_check_flag(INTERNAL_FUNCTION_PARAMETERS, int mask)
{
   reflection_object *intern;
   zend_class_entry *ce;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(ce, zend_class_entry *);
   RETVAL_BOOL(ce->ce_flags & mask);
}
} // anonymous namespace

ZEND_METHOD(reflection_class, isInstantiable)
{
   reflection_object *intern;
   zend_class_entry *ce;
   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(ce, zend_class_entry *);
   if (ce->ce_flags & (ZEND_ACC_INTERFACE | ZEND_ACC_TRAIT | ZEND_ACC_EXPLICIT_ABSTRACT_CLASS | ZEND_ACC_IMPLICIT_ABSTRACT_CLASS)) {
      RETURN_FALSE;
   }
   /* Basically, the class is instantiable. Though, if there is a constructor
    * and it is not publicly accessible, it isn't! */
   if (!ce->constructor) {
      RETURN_TRUE;
   }
   RETURN_BOOL(ce->constructor->common.fn_flags & ZEND_ACC_PUBLIC);
}

ZEND_METHOD(reflection_class, isCloneable)
{
   reflection_object *intern;
   zend_class_entry *ce;
   zval obj;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(ce, zend_class_entry *);
   if (ce->ce_flags & (ZEND_ACC_INTERFACE | ZEND_ACC_TRAIT | ZEND_ACC_EXPLICIT_ABSTRACT_CLASS | ZEND_ACC_IMPLICIT_ABSTRACT_CLASS)) {
      RETURN_FALSE;
   }
   if (!Z_ISUNDEF(intern->obj)) {
      if (ce->clone) {
         RETURN_BOOL(ce->clone->common.fn_flags & ZEND_ACC_PUBLIC);
      } else {
         RETURN_BOOL(Z_OBJ_HANDLER(intern->obj, clone_obj) != nullptr);
      }
   } else {
      if (ce->clone) {
         RETURN_BOOL(ce->clone->common.fn_flags & ZEND_ACC_PUBLIC);
      } else {
         if (UNEXPECTED(object_init_ex(&obj, ce) != SUCCESS)) {
            return;
         }
         RETVAL_BOOL(Z_OBJ_HANDLER(obj, clone_obj) != nullptr);
         zval_ptr_dtor(&obj);
      }
   }
}

ZEND_METHOD(reflection_class, isInterface)
{
   class_check_flag(INTERNAL_FUNCTION_PARAM_PASSTHRU, ZEND_ACC_INTERFACE);
}

ZEND_METHOD(reflection_class, isTrait)
{
   class_check_flag(INTERNAL_FUNCTION_PARAM_PASSTHRU, ZEND_ACC_TRAIT);
}

ZEND_METHOD(reflection_class, isFinal)
{
   class_check_flag(INTERNAL_FUNCTION_PARAM_PASSTHRU, ZEND_ACC_FINAL);
}

ZEND_METHOD(reflection_class, isAbstract)
{
   class_check_flag(INTERNAL_FUNCTION_PARAM_PASSTHRU, ZEND_ACC_IMPLICIT_ABSTRACT_CLASS|ZEND_ACC_EXPLICIT_ABSTRACT_CLASS);
}

ZEND_METHOD(reflection_class, getModifiers)
{
   reflection_object *intern;
   zend_class_entry *ce;
   uint32_t keep_flags = ZEND_ACC_FINAL
         | ZEND_ACC_EXPLICIT_ABSTRACT_CLASS | ZEND_ACC_IMPLICIT_ABSTRACT_CLASS;
   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(ce, zend_class_entry *);
   RETURN_LONG((ce->ce_flags & keep_flags));
}

ZEND_METHOD(reflection_class, isInstance)
{
   reflection_object *intern;
   zend_class_entry *ce;
   zval *object;
   if (zend_parse_parameters(ZEND_NUM_ARGS(), "o", &object) == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(ce, zend_class_entry *);
   RETURN_BOOL(instanceof_function(Z_OBJCE_P(object), ce));
}

ZEND_METHOD(reflection_class, newInstance)
{
   zval retval;
   reflection_object *intern;
   zend_class_entry *ce, *old_scope;
   zend_function *constructor;
   GET_REFLECTION_OBJECT_PTR(ce, zend_class_entry *);
   if (UNEXPECTED(object_init_ex(return_value, ce) != SUCCESS)) {
      return;
   }
   old_scope = EG(fake_scope);
   EG(fake_scope) = ce;
   constructor = Z_OBJ_HT_P(return_value)->get_constructor(Z_OBJ_P(return_value));
   EG(fake_scope) = old_scope;

   /* Run the constructor if there is one */
   if (constructor) {
      zval *params = nullptr;
      int ret, i, num_args = 0;
      zend_fcall_info fci;
      zend_fcall_info_cache fcc;
      if (!(constructor->common.fn_flags & ZEND_ACC_PUBLIC)) {
         zend_throw_exception_ex(g_reflectionExceptionPtr, 0, "Access to non-public constructor of class %s", ZSTR_VAL(ce->name));
         zval_ptr_dtor(return_value);
         RETURN_NULL();
      }
      if (zend_parse_parameters(ZEND_NUM_ARGS(), "*", &params, &num_args) == FAILURE) {
         zval_ptr_dtor(return_value);
         RETURN_FALSE;
      }
      for (i = 0; i < num_args; i++) {
         Z_TRY_ADDREF(params[i]);
      }
      fci.size = sizeof(fci);
      ZVAL_UNDEF(&fci.function_name);
      fci.object = Z_OBJ_P(return_value);
      fci.retval = &retval;
      fci.param_count = num_args;
      fci.params = params;
      fci.no_separation = 1;

      fcc.function_handler = constructor;
      fcc.called_scope = Z_OBJCE_P(return_value);
      fcc.object = Z_OBJ_P(return_value);

      ret = zend_call_function(&fci, &fcc);
      zval_ptr_dtor(&retval);
      for (i = 0; i < num_args; i++) {
         zval_ptr_dtor(&params[i]);
      }
      if (ret == FAILURE) {
         php_error_docref(nullptr, E_WARNING, "Invocation of %s's constructor failed", ZSTR_VAL(ce->name));
         zval_ptr_dtor(return_value);
         RETURN_NULL();
      }
   } else if (ZEND_NUM_ARGS()) {
      zend_throw_exception_ex(g_reflectionExceptionPtr, 0, "Class %s does not have a constructor, so you cannot pass any constructor arguments", ZSTR_VAL(ce->name));
   }
}

ZEND_METHOD(reflection_class, newInstanceWithoutConstructor)
{
   reflection_object *intern;
   zend_class_entry *ce;
   GET_REFLECTION_OBJECT_PTR(ce, zend_class_entry *);
   if (ce->create_object != nullptr && ce->ce_flags & ZEND_ACC_FINAL) {
      zend_throw_exception_ex(g_reflectionExceptionPtr, 0, "Class %s is an internal class marked as final that cannot be instantiated without invoking its constructor", ZSTR_VAL(ce->name));
      return;
   }
   object_init_ex(return_value, ce);
}

ZEND_METHOD(reflection_class, newInstanceArgs)
{
   zval retval, *val;
   reflection_object *intern;
   zend_class_entry *ce, *old_scope;
   int ret, i, argc = 0;
   HashTable *args;
   zend_function *constructor;

   GET_REFLECTION_OBJECT_PTR(ce, zend_class_entry *);

   if (zend_parse_parameters(ZEND_NUM_ARGS(), "|h", &args) == FAILURE) {
      return;
   }

   if (ZEND_NUM_ARGS() > 0) {
      argc = args->nNumOfElements;
   }

   if (UNEXPECTED(object_init_ex(return_value, ce) != SUCCESS)) {
      return;
   }

   old_scope = EG(fake_scope);
   EG(fake_scope) = ce;
   constructor = Z_OBJ_HT_P(return_value)->get_constructor(Z_OBJ_P(return_value));
   EG(fake_scope) = old_scope;
   /* Run the constructor if there is one */
   if (constructor) {
      zval *params = nullptr;
      zend_fcall_info fci;
      zend_fcall_info_cache fcc;
      if (!(constructor->common.fn_flags & ZEND_ACC_PUBLIC)) {
         zend_throw_exception_ex(g_reflectionExceptionPtr, 0, "Access to non-public constructor of class %s", ZSTR_VAL(ce->name));
         zval_ptr_dtor(return_value);
         RETURN_NULL();
      }
      if (argc) {
         params = reinterpret_cast<zval *>(safe_emalloc(sizeof(zval), argc, 0));
         argc = 0;
         ZEND_HASH_FOREACH_VAL(args, val) {
            ZVAL_COPY(&params[argc], val);
            argc++;
         } ZEND_HASH_FOREACH_END();
      }
      fci.size = sizeof(fci);
      ZVAL_UNDEF(&fci.function_name);
      fci.object = Z_OBJ_P(return_value);
      fci.retval = &retval;
      fci.param_count = argc;
      fci.params = params;
      fci.no_separation = 1;

      fcc.function_handler = constructor;
      fcc.called_scope = Z_OBJCE_P(return_value);
      fcc.object = Z_OBJ_P(return_value);

      ret = zend_call_function(&fci, &fcc);
      zval_ptr_dtor(&retval);
      if (params) {
         for (i = 0; i < argc; i++) {
            zval_ptr_dtor(&params[i]);
         }
         efree(params);
      }
      if (ret == FAILURE) {
         zval_ptr_dtor(&retval);
         php_error_docref(nullptr, E_WARNING, "Invocation of %s's constructor failed", ZSTR_VAL(ce->name));
         zval_ptr_dtor(return_value);
         RETURN_NULL();
      }
   } else if (argc) {
      zend_throw_exception_ex(g_reflectionExceptionPtr, 0, "Class %s does not have a constructor, so you cannot pass any constructor arguments", ZSTR_VAL(ce->name));
   }
}

ZEND_METHOD(reflection_class, getInterfaces)
{
   reflection_object *intern;
   zend_class_entry *ce;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(ce, zend_class_entry *);

   if (ce->num_interfaces) {
      uint32_t i;

      array_init(return_value);
      for (i=0; i < ce->num_interfaces; i++) {
         zval interface;
         zend_reflection_class_factory(ce->interfaces[i], &interface);
         zend_hash_update(Z_ARRVAL_P(return_value), ce->interfaces[i]->name, &interface);
      }
   } else {
      POLAR_ZVAL_EMPTY_ARRAY(return_value);
   }
}

ZEND_METHOD(reflection_class, getInterfaceNames)
{
   reflection_object *intern;
   zend_class_entry *ce;
   uint32_t i;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(ce, zend_class_entry *);

   if (!ce->num_interfaces) {
      /* Return an empty array if this class implements no interfaces */
      POLAR_ZVAL_EMPTY_ARRAY(return_value);
      return;
   }

   array_init(return_value);

   for (i=0; i < ce->num_interfaces; i++) {
      add_next_index_str(return_value, zend_string_copy(ce->interfaces[i]->name));
   }
}

ZEND_METHOD(reflection_class, getTraits)
{
   reflection_object *intern;
   zend_class_entry *ce;
   uint32_t i;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(ce, zend_class_entry *);

   if (!ce->num_traits) {
      POLAR_ZVAL_EMPTY_ARRAY(return_value);
      return;
   }

   array_init(return_value);

   for (i=0; i < ce->num_traits; i++) {
      zval trait;
      zend_reflection_class_factory(ce->traits[i], &trait);
      zend_hash_update(Z_ARRVAL_P(return_value), ce->traits[i]->name, &trait);
   }
}

ZEND_METHOD(reflection_class, getTraitNames)
{
   reflection_object *intern;
   zend_class_entry *ce;
   uint32_t i;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(ce, zend_class_entry *);

   if (!ce->num_traits) {
      POLAR_ZVAL_EMPTY_ARRAY(return_value);
      return;
   }

   array_init(return_value);

   for (i=0; i < ce->num_traits; i++) {
      add_next_index_str(return_value, zend_string_copy(ce->traits[i]->name));
   }
}

ZEND_METHOD(reflection_class, getTraitAliases)
{
   reflection_object *intern;
   zend_class_entry *ce;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(ce, zend_class_entry *);
   if (ce->trait_aliases) {
      uint32_t i = 0;
      array_init(return_value);
      while (ce->trait_aliases[i]) {
         zend_string *mname;
         zend_trait_method_reference *cur_ref = &ce->trait_aliases[i]->trait_method;
         if (ce->trait_aliases[i]->alias) {
            mname = zend_string_alloc(ZSTR_LEN(cur_ref->class_name) + ZSTR_LEN(cur_ref->method_name) + 2, 0);
            snprintf(ZSTR_VAL(mname), ZSTR_LEN(mname) + 1, "%s::%s", ZSTR_VAL(cur_ref->class_name), ZSTR_VAL(cur_ref->method_name));
            add_assoc_str_ex(return_value, ZSTR_VAL(ce->trait_aliases[i]->alias), ZSTR_LEN(ce->trait_aliases[i]->alias), mname);
         }
         i++;
      }
   } else {
      POLAR_ZVAL_EMPTY_ARRAY(return_value);
   }
}

ZEND_METHOD(reflection_class, getParentClass)
{
   reflection_object *intern;
   zend_class_entry *ce;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(ce, zend_class_entry *);

   if (ce->parent) {
      zend_reflection_class_factory(ce->parent, return_value);
   } else {
      RETURN_FALSE;
   }
}

ZEND_METHOD(reflection_class, isSubclassOf)
{
   reflection_object *intern, *argument;
   zend_class_entry *ce, *class_ce;
   zval *class_name;

   GET_REFLECTION_OBJECT_PTR(ce, zend_class_entry *);

   if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &class_name) == FAILURE) {
      return;
   }

   switch (Z_TYPE_P(class_name)) {
   case IS_STRING:
      if ((class_ce = zend_lookup_class(Z_STR_P(class_name))) == nullptr) {
         zend_throw_exception_ex(g_reflectionExceptionPtr, 0,
                                 "Class %s does not exist", Z_STRVAL_P(class_name));
         return;
      }
      break;
   case IS_OBJECT:
      if (instanceof_function(Z_OBJCE_P(class_name), g_reflectionClassPtr)) {
         argument = Z_REFLECTION_P(class_name);
         if (argument->ptr == nullptr) {
            zend_throw_error(nullptr, "Internal error: Failed to retrieve the argument's reflection object");
            return;
         }
         class_ce = reinterpret_cast<zend_class_entry *>(argument->ptr);
         break;
      }
      POLAR_FALLTHROUGH;
      /* no break */
   default:
      zend_throw_exception_ex(g_reflectionExceptionPtr, 0,
                              "Parameter one must either be a string or a ReflectionClass object");
      return;
   }

   RETURN_BOOL((ce != class_ce && instanceof_function(ce, class_ce)));
}

ZEND_METHOD(reflection_class, implementsInterface)
{
   reflection_object *intern, *argument;
   zend_class_entry *ce, *interface_ce;
   zval *interface;

   GET_REFLECTION_OBJECT_PTR(ce, zend_class_entry *);

   if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &interface) == FAILURE) {
      return;
   }

   switch (Z_TYPE_P(interface)) {
   case IS_STRING:
      if ((interface_ce = zend_lookup_class(Z_STR_P(interface))) == nullptr) {
         zend_throw_exception_ex(g_reflectionExceptionPtr, 0,
                                 "Interface %s does not exist", Z_STRVAL_P(interface));
         return;
      }
      break;
   case IS_OBJECT:
      if (instanceof_function(Z_OBJCE_P(interface), g_reflectionClassPtr)) {
         argument = Z_REFLECTION_P(interface);
         if (argument->ptr == nullptr) {
            zend_throw_error(nullptr, "Internal error: Failed to retrieve the argument's reflection object");
            return;
         }
         interface_ce = reinterpret_cast<zend_class_entry *>(argument->ptr);
         break;
      }
      POLAR_FALLTHROUGH;
      /* no break */
   default:
      zend_throw_exception_ex(g_reflectionExceptionPtr, 0,
                              "Parameter one must either be a string or a ReflectionClass object");
      return;
   }

   if (!(interface_ce->ce_flags & ZEND_ACC_INTERFACE)) {
      zend_throw_exception_ex(g_reflectionExceptionPtr, 0,
                              "Interface %s is a Class", ZSTR_VAL(interface_ce->name));
      return;
   }
   RETURN_BOOL(instanceof_function(ce, interface_ce));
}

ZEND_METHOD(reflection_class, isIterable)
{
   reflection_object *intern;
   zend_class_entry *ce;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }

   GET_REFLECTION_OBJECT_PTR(ce, zend_class_entry *);

   if (ce->ce_flags & (ZEND_ACC_INTERFACE | ZEND_ACC_IMPLICIT_ABSTRACT_CLASS |
                       ZEND_ACC_TRAIT     | ZEND_ACC_EXPLICIT_ABSTRACT_CLASS)) {
      RETURN_FALSE;
   }

   RETURN_BOOL(ce->get_iterator || instanceof_function(ce, zend_ce_traversable));
}

ZEND_METHOD(reflection_class, inNamespace)
{
   zval *name;
   const char *backslash;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   if ((name = get_default_load_name(getThis())) == nullptr) {
      RETURN_FALSE;
   }
   if (Z_TYPE_P(name) == IS_STRING
       && (backslash = reinterpret_cast<const char *>(zend_memrchr(Z_STRVAL_P(name), '\\', Z_STRLEN_P(name))))
       && backslash > Z_STRVAL_P(name))
   {
      RETURN_TRUE;
   }
   RETURN_FALSE;
}

ZEND_METHOD(reflection_class, getNamespaceName)
{
   zval *name;
   const char *backslash;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   if ((name = get_default_load_name(getThis())) == nullptr) {
      RETURN_FALSE;
   }
   if (Z_TYPE_P(name) == IS_STRING
       && (backslash = reinterpret_cast<const char *>(zend_memrchr(Z_STRVAL_P(name), '\\', Z_STRLEN_P(name))))
       && backslash > Z_STRVAL_P(name))
   {
      RETURN_STRINGL(Z_STRVAL_P(name), backslash - Z_STRVAL_P(name));
   }
   RETURN_EMPTY_STRING();
}

ZEND_METHOD(reflection_class, getShortName)
{
   zval *name;
   const char *backslash;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   if ((name = get_default_load_name(getThis())) == nullptr) {
      RETURN_FALSE;
   }
   if (Z_TYPE_P(name) == IS_STRING
       && (backslash = reinterpret_cast<const char *>(zend_memrchr(Z_STRVAL_P(name), '\\', Z_STRLEN_P(name))))
       && backslash > Z_STRVAL_P(name))
   {
      RETURN_STRINGL(backslash + 1, Z_STRLEN_P(name) - (backslash - Z_STRVAL_P(name) + 1));
   }
   ZVAL_COPY_DEREF(return_value, name);
}

ZEND_METHOD(reflection_object, export)
{
   reflection_export(INTERNAL_FUNCTION_PARAM_PASSTHRU, g_reflectionObjectPtr, 1);
}

ZEND_METHOD(reflection_object, __construct)
{
   reflection_class_object_ctor(INTERNAL_FUNCTION_PARAM_PASSTHRU, 1);
}

ZEND_METHOD(reflection_property, export)
{
   reflection_export(INTERNAL_FUNCTION_PARAM_PASSTHRU, g_reflectionPropertyPtr, 2);
}

ZEND_METHOD(reflection_class_constant, export)
{
   reflection_export(INTERNAL_FUNCTION_PARAM_PASSTHRU,  g_reflectionClassConstantPtr, 2);
}

ZEND_METHOD(reflection_property, __construct)
{
   zval propname, cname, *classname;
   zend_string *name;
   int dynam_prop = 0;
   zval *object;
   reflection_object *intern;
   zend_class_entry *ce;
   zend_property_info *property_info = nullptr;
   property_reference *reference;

   if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "zS", &classname, &name) == FAILURE) {
      return;
   }

   object = getThis();
   intern = Z_REFLECTION_P(object);

   /* Find the class entry */
   switch (Z_TYPE_P(classname)) {
   case IS_STRING:
      if ((ce = zend_lookup_class(Z_STR_P(classname))) == nullptr) {
         zend_throw_exception_ex(g_reflectionExceptionPtr, 0,
                                 "Class %s does not exist", Z_STRVAL_P(classname));
         return;
      }
      break;

   case IS_OBJECT:
      ce = Z_OBJCE_P(classname);
      break;

   default:
      _DO_THROW("The parameter class is expected to be either a string or an object");
      /* returns out of this function */
   }

   if ((property_info = reinterpret_cast<zend_property_info *>(zend_hash_find_ptr(&ce->properties_info, name))) == nullptr ||
       (property_info->flags & ZEND_ACC_SHADOW)) {
      /* Check for dynamic properties */
      if (property_info == nullptr && Z_TYPE_P(classname) == IS_OBJECT && Z_OBJ_HT_P(classname)->get_properties) {
         if (zend_hash_exists(Z_OBJ_HT_P(classname)->get_properties(classname), name)) {
            dynam_prop = 1;
         }
      }
      if (dynam_prop == 0) {
         zend_throw_exception_ex(g_reflectionExceptionPtr, 0, "Property %s::$%s does not exist", ZSTR_VAL(ce->name), ZSTR_VAL(name));
         return;
      }
   }

   if (dynam_prop == 0 && (property_info->flags & ZEND_ACC_PRIVATE) == 0) {
      /* we have to search the class hierarchy for this (implicit) public or protected property */
      zend_class_entry *tmp_ce = ce;
      zend_property_info *tmp_info;

      while (tmp_ce && (tmp_info = reinterpret_cast<zend_property_info *>(zend_hash_find_ptr(&tmp_ce->properties_info, name))) == nullptr) {
         ce = tmp_ce;
         property_info = tmp_info;
         tmp_ce = tmp_ce->parent;
      }
   }

   if (dynam_prop == 0) {
      ZVAL_STR_COPY(&cname, property_info->ce->name);
   } else {
      ZVAL_STR_COPY(&cname, ce->name);
   }
   reflection_update_property_class(object, &cname);

   ZVAL_STR_COPY(&propname, name);
   reflection_update_property_name(object, &propname);

   reference = (property_reference*) emalloc(sizeof(property_reference));
   if (dynam_prop) {
      reference->prop.flags = ZEND_ACC_IMPLICIT_PUBLIC;
      reference->prop.name = name;
      reference->prop.doc_comment = nullptr;
      reference->prop.ce = ce;
   } else {
      reference->prop = *property_info;
   }
   reference->ce = ce;
   reference->unmangledName = zend_string_copy(name);
   intern->ptr = reference;
   intern->refType = REF_TYPE_PROPERTY;
   intern->ce = ce;
   intern->ignoreVisibility = 0;
}

ZEND_METHOD(reflection_property, __toString)
{
   reflection_object *intern;
   property_reference *ref;
   smart_str str = {0};
   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(ref, property_reference *);
   property_string(&str, &ref->prop, ZSTR_VAL(ref->unmangledName), const_cast<char *>(""));
   RETURN_STR(smart_str_extract(&str));
}

ZEND_METHOD(reflection_property, getName)
{
   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   default_get_name(getThis(), return_value);
}

namespace {
void property_check_flag(INTERNAL_FUNCTION_PARAMETERS, int mask) /* {{{ */
{
   reflection_object *intern;
   property_reference *ref;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(ref, property_reference *);
   RETURN_BOOL(ref->prop.flags & mask);
}
} // anonymous namespace

ZEND_METHOD(reflection_property, isPublic)
{
   property_check_flag(INTERNAL_FUNCTION_PARAM_PASSTHRU, ZEND_ACC_PUBLIC | ZEND_ACC_IMPLICIT_PUBLIC);
}

ZEND_METHOD(reflection_property, isPrivate)
{
   property_check_flag(INTERNAL_FUNCTION_PARAM_PASSTHRU, ZEND_ACC_PRIVATE);
}

ZEND_METHOD(reflection_property, isProtected)
{
   property_check_flag(INTERNAL_FUNCTION_PARAM_PASSTHRU, ZEND_ACC_PROTECTED);
}

ZEND_METHOD(reflection_property, isStatic)
{
   property_check_flag(INTERNAL_FUNCTION_PARAM_PASSTHRU, ZEND_ACC_STATIC);
}

ZEND_METHOD(reflection_property, isDefault)
{
   property_check_flag(INTERNAL_FUNCTION_PARAM_PASSTHRU, ~ZEND_ACC_IMPLICIT_PUBLIC);
}

ZEND_METHOD(reflection_property, getModifiers)
{
   reflection_object *intern;
   property_reference *ref;
   uint32_t keep_flags = ZEND_ACC_PPP_MASK | ZEND_ACC_IMPLICIT_PUBLIC | ZEND_ACC_STATIC;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(ref, property_reference *);

   RETURN_LONG((ref->prop.flags & keep_flags));
}

ZEND_METHOD(reflection_property, getValue)
{
   reflection_object *intern;
   property_reference *ref;
   zval *object, *name;
   zval *member_p = nullptr;

   GET_REFLECTION_OBJECT_PTR(ref, property_reference *);

   if (!(ref->prop.flags & (ZEND_ACC_PUBLIC | ZEND_ACC_IMPLICIT_PUBLIC)) && intern->ignoreVisibility == 0) {
      name = get_default_load_name(getThis());
      zend_throw_exception_ex(g_reflectionExceptionPtr, 0,
                              "Cannot access non-public member %s::$%s", ZSTR_VAL(intern->ce->name), Z_STRVAL_P(name));
      return;
   }

   if (ref->prop.flags & ZEND_ACC_STATIC) {
      member_p = zend_read_static_property_ex(ref->ce, ref->unmangledName, 0);
      if (member_p) {
         ZVAL_COPY_DEREF(return_value, member_p);
      }
   } else {
      zval rv;

      if (zend_parse_parameters(ZEND_NUM_ARGS(), "o", &object) == FAILURE) {
         return;
      }

      if (!instanceof_function(Z_OBJCE_P(object), ref->prop.ce)) {
         _DO_THROW("Given object is not an instance of the class this property was declared in");
         /* Returns from this function */
      }

      member_p = zend_read_property_ex(ref->ce, object, ref->unmangledName, 0, &rv);
      if (member_p != &rv) {
         ZVAL_COPY_DEREF(return_value, member_p);
      } else {
         if (Z_ISREF_P(member_p)) {
            zend_unwrap_reference(member_p);
         }
         ZVAL_COPY_VALUE(return_value, member_p);
      }
   }
}

ZEND_METHOD(reflection_property, setValue)
{
   reflection_object *intern;
   property_reference *ref;
   zval *object, *name;
   zval *value;
   zval *tmp;

   GET_REFLECTION_OBJECT_PTR(ref, property_reference *);

   if (!(ref->prop.flags & ZEND_ACC_PUBLIC) && intern->ignoreVisibility == 0) {
      name = get_default_load_name(getThis());
      zend_throw_exception_ex(g_reflectionExceptionPtr, 0,
                              "Cannot access non-public member %s::$%s", ZSTR_VAL(intern->ce->name), Z_STRVAL_P(name));
      return;
   }

   if (ref->prop.flags & ZEND_ACC_STATIC) {
      if (zend_parse_parameters_ex(ZEND_PARSE_PARAMS_QUIET, ZEND_NUM_ARGS(), "z", &value) == FAILURE) {
         if (zend_parse_parameters(ZEND_NUM_ARGS(), "zz", &tmp, &value) == FAILURE) {
            return;
         }
      }

      zend_update_static_property_ex(ref->ce, ref->unmangledName, value);
   } else {
      if (zend_parse_parameters(ZEND_NUM_ARGS(), "oz", &object, &value) == FAILURE) {
         return;
      }

      zend_update_property_ex(ref->ce, object, ref->unmangledName, value);
   }
}

ZEND_METHOD(reflection_property, getDeclaringClass)
{
   reflection_object *intern;
   property_reference *ref;
   zend_class_entry *tmp_ce, *ce;
   zend_property_info *tmp_info;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(ref, property_reference *);

   ce = tmp_ce = ref->ce;
   while (tmp_ce && (tmp_info = reinterpret_cast<zend_property_info *>(zend_hash_find_ptr(&tmp_ce->properties_info, ref->unmangledName))) != nullptr) {
      if (tmp_info->flags & ZEND_ACC_PRIVATE || tmp_info->flags & ZEND_ACC_SHADOW) {
         /* it's a private property, so it can't be inherited */
         break;
      }
      ce = tmp_ce;
      if (tmp_ce == tmp_info->ce) {
         /* declared in this class, done */
         break;
      }
      tmp_ce = tmp_ce->parent;
   }

   zend_reflection_class_factory(ce, return_value);
}

ZEND_METHOD(reflection_property, getDocComment)
{
   reflection_object *intern;
   property_reference *ref;

   if (zend_parse_parameters_none() == FAILURE) {
      return;
   }
   GET_REFLECTION_OBJECT_PTR(ref, property_reference *);
   if (ref->prop.doc_comment) {
      RETURN_STR_COPY(ref->prop.doc_comment);
   }
   RETURN_FALSE;
}

ZEND_METHOD(reflection_property, setAccessible)
{
   reflection_object *intern;
   zend_bool visible;

   if (zend_parse_parameters(ZEND_NUM_ARGS(), "b", &visible) == FAILURE) {
      return;
   }

   intern = Z_REFLECTION_P(getThis());

   intern->ignoreVisibility = visible;
}

const zend_function_entry g_reflectionExceptionFunctions[] = {
   PHP_FE_END
};

ZEND_BEGIN_ARG_INFO(arginfo_reflection__void, 0)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO(arginfo_reflection_getModifierNames, 0)
ZEND_ARG_INFO(0, modifiers)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_reflection_export, 0, 0, 1)
ZEND_ARG_OBJ_INFO(0, reflector, Reflector, 0)
ZEND_ARG_INFO(0, return)
ZEND_END_ARG_INFO()


static const zend_function_entry sg_reflectionFunctions[] = {
   ZEND_ME(reflection, getModifierNames, arginfo_reflection_getModifierNames, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
   ZEND_ME(reflection, export, arginfo_reflection_export, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
   PHP_FE_END
};

static const zend_function_entry sg_reflectorFunctions[] = {
   ZEND_FENTRY(export, nullptr, nullptr, ZEND_ACC_STATIC|ZEND_ACC_ABSTRACT|ZEND_ACC_PUBLIC)
   ZEND_ABSTRACT_ME(reflector, __toString, arginfo_reflection__void)
   PHP_FE_END
};

ZEND_BEGIN_ARG_INFO_EX(arginfo_reflection_function_export, 0, 0, 1)
ZEND_ARG_INFO(0, name)
ZEND_ARG_INFO(0, return)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_reflection_function___construct, 0)
ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_reflection_function_invoke, 0, 0, 0)
ZEND_ARG_INFO(0, args)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_reflection_function_invokeArgs, 0)
ZEND_ARG_ARRAY_INFO(0, args, 0)
ZEND_END_ARG_INFO()

static const zend_function_entry sg_reflectionFunctionAbstractFunctions[] = {
   ZEND_ME(reflection, __clone, arginfo_reflection__void, ZEND_ACC_PRIVATE|ZEND_ACC_FINAL)
   ZEND_ME(reflection_function, inNamespace, arginfo_reflection__void, 0)
   ZEND_ME(reflection_function, isClosure, arginfo_reflection__void, 0)
   ZEND_ME(reflection_function, isDeprecated, arginfo_reflection__void, 0)
   ZEND_ME(reflection_function, isInternal, arginfo_reflection__void, 0)
   ZEND_ME(reflection_function, isUserDefined, arginfo_reflection__void, 0)
   ZEND_ME(reflection_function, isGenerator, arginfo_reflection__void, 0)
   ZEND_ME(reflection_function, isVariadic, arginfo_reflection__void, 0)
   ZEND_ME(reflection_function, getClosureThis, arginfo_reflection__void, 0)
   ZEND_ME(reflection_function, getClosureScopeClass, arginfo_reflection__void, 0)
   ZEND_ME(reflection_function, getDocComment, arginfo_reflection__void, 0)
   ZEND_ME(reflection_function, getEndLine, arginfo_reflection__void, 0)
   ZEND_ME(reflection_function, getFileName, arginfo_reflection__void, 0)
   ZEND_ME(reflection_function, getName, arginfo_reflection__void, 0)
   ZEND_ME(reflection_function, getNamespaceName, arginfo_reflection__void, 0)
   ZEND_ME(reflection_function, getNumberOfParameters, arginfo_reflection__void, 0)
   ZEND_ME(reflection_function, getNumberOfRequiredParameters, arginfo_reflection__void, 0)
   ZEND_ME(reflection_function, getParameters, arginfo_reflection__void, 0)
   ZEND_ME(reflection_function, getShortName, arginfo_reflection__void, 0)
   ZEND_ME(reflection_function, getStartLine, arginfo_reflection__void, 0)
   ZEND_ME(reflection_function, getStaticVariables, arginfo_reflection__void, 0)
   ZEND_ME(reflection_function, returnsReference, arginfo_reflection__void, 0)
   ZEND_ME(reflection_function, hasReturnType, arginfo_reflection__void, 0)
   ZEND_ME(reflection_function, getReturnType, arginfo_reflection__void, 0)
   PHP_FE_END
};

static const zend_function_entry sg_reflectionFunctionFunctions[] = {
   ZEND_ME(reflection_function, __construct, arginfo_reflection_function___construct, 0)
   ZEND_ME(reflection_function, __toString, arginfo_reflection__void, 0)
   ZEND_ME(reflection_function, export, arginfo_reflection_function_export, ZEND_ACC_STATIC|ZEND_ACC_PUBLIC)
   ZEND_ME(reflection_function, isDisabled, arginfo_reflection__void, 0)
   ZEND_ME(reflection_function, invoke, arginfo_reflection_function_invoke, 0)
   ZEND_ME(reflection_function, invokeArgs, arginfo_reflection_function_invokeArgs, 0)
   ZEND_ME(reflection_function, getClosure, arginfo_reflection__void, 0)
   PHP_FE_END
};

ZEND_BEGIN_ARG_INFO(arginfo_reflection_generator___construct, 0)
ZEND_ARG_INFO(0, generator)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_reflection_generator_trace, 0, 0, 0)
ZEND_ARG_INFO(0, options)
ZEND_END_ARG_INFO()

static const zend_function_entry sg_reflectionGeneratorFunctions[] = {
   ZEND_ME(reflection_generator, __construct, arginfo_reflection_generator___construct, 0)
   ZEND_ME(reflection_generator, getExecutingLine, arginfo_reflection__void, 0)
   ZEND_ME(reflection_generator, getExecutingFile, arginfo_reflection__void, 0)
   ZEND_ME(reflection_generator, getTrace, arginfo_reflection_generator_trace, 0)
   ZEND_ME(reflection_generator, getFunction, arginfo_reflection__void, 0)
   ZEND_ME(reflection_generator, getThis, arginfo_reflection__void, 0)
   ZEND_ME(reflection_generator, getExecutingGenerator, arginfo_reflection__void, 0)
   PHP_FE_END
};

ZEND_BEGIN_ARG_INFO_EX(arginfo_reflection_method_export, 0, 0, 2)
ZEND_ARG_INFO(0, class)
ZEND_ARG_INFO(0, name)
ZEND_ARG_INFO(0, return)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_reflection_method___construct, 0, 0, 1)
ZEND_ARG_INFO(0, class_or_method)
ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_reflection_method_invoke, 0)
ZEND_ARG_INFO(0, object)
ZEND_ARG_INFO(0, args)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_reflection_method_invokeArgs, 0)
ZEND_ARG_INFO(0, object)
ZEND_ARG_ARRAY_INFO(0, args, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_reflection_method_setAccessible, 0)
ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_reflection_method_getClosure, 0)
ZEND_ARG_INFO(0, object)
ZEND_END_ARG_INFO()

static const zend_function_entry sg_reflectionMethodFunctions[] = {
   ZEND_ME(reflection_method, export, arginfo_reflection_method_export, ZEND_ACC_STATIC|ZEND_ACC_PUBLIC)
   ZEND_ME(reflection_method, __construct, arginfo_reflection_method___construct, 0)
   ZEND_ME(reflection_method, __toString, arginfo_reflection__void, 0)
   ZEND_ME(reflection_method, isPublic, arginfo_reflection__void, 0)
   ZEND_ME(reflection_method, isPrivate, arginfo_reflection__void, 0)
   ZEND_ME(reflection_method, isProtected, arginfo_reflection__void, 0)
   ZEND_ME(reflection_method, isAbstract, arginfo_reflection__void, 0)
   ZEND_ME(reflection_method, isFinal, arginfo_reflection__void, 0)
   ZEND_ME(reflection_method, isStatic, arginfo_reflection__void, 0)
   ZEND_ME(reflection_method, isConstructor, arginfo_reflection__void, 0)
   ZEND_ME(reflection_method, isDestructor, arginfo_reflection__void, 0)
   ZEND_ME(reflection_method, getClosure, arginfo_reflection_method_getClosure, 0)
   ZEND_ME(reflection_method, getModifiers, arginfo_reflection__void, 0)
   ZEND_ME(reflection_method, invoke, arginfo_reflection_method_invoke, 0)
   ZEND_ME(reflection_method, invokeArgs, arginfo_reflection_method_invokeArgs, 0)
   ZEND_ME(reflection_method, getDeclaringClass, arginfo_reflection__void, 0)
   ZEND_ME(reflection_method, getPrototype, arginfo_reflection__void, 0)
   ZEND_ME(reflection_method, setAccessible, arginfo_reflection_method_setAccessible, 0)
   PHP_FE_END
};

ZEND_BEGIN_ARG_INFO_EX(arginfo_reflection_class_export, 0, 0, 1)
ZEND_ARG_INFO(0, argument)
ZEND_ARG_INFO(0, return)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_reflection_class___construct, 0)
ZEND_ARG_INFO(0, argument)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_reflection_class_getStaticPropertyValue, 0, 0, 1)
ZEND_ARG_INFO(0, name)
ZEND_ARG_INFO(0, default)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_reflection_class_setStaticPropertyValue, 0)
ZEND_ARG_INFO(0, name)
ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_reflection_class_hasMethod, 0)
ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_reflection_class_getMethod, 0)
ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_reflection_class_getMethods, 0, 0, 0)
ZEND_ARG_INFO(0, filter)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_reflection_class_hasProperty, 0)
ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_reflection_class_getProperty, 0)
ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_reflection_class_getProperties, 0, 0, 0)
ZEND_ARG_INFO(0, filter)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_reflection_class_hasConstant, 0)
ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_reflection_class_getConstant, 0)
ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_reflection_class_isInstance, 0)
ZEND_ARG_INFO(0, object)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_reflection_class_newInstance, 0)
ZEND_ARG_INFO(0, args)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_reflection_class_newInstanceWithoutConstructor, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_reflection_class_newInstanceArgs, 0, 0, 0)
ZEND_ARG_ARRAY_INFO(0, args, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_reflection_class_isSubclassOf, 0)
ZEND_ARG_INFO(0, class)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_reflection_class_implementsInterface, 0)
ZEND_ARG_INFO(0, interface)
ZEND_END_ARG_INFO()

static const zend_function_entry sg_reflectionClassFunctions[] = {
   ZEND_ME(reflection, __clone, arginfo_reflection__void, ZEND_ACC_PRIVATE|ZEND_ACC_FINAL)
   ZEND_ME(reflection_class, export, arginfo_reflection_class_export, ZEND_ACC_STATIC|ZEND_ACC_PUBLIC)
   ZEND_ME(reflection_class, __construct, arginfo_reflection_class___construct, 0)
   ZEND_ME(reflection_class, __toString, arginfo_reflection__void, 0)
   ZEND_ME(reflection_class, getName, arginfo_reflection__void, 0)
   ZEND_ME(reflection_class, isInternal, arginfo_reflection__void, 0)
   ZEND_ME(reflection_class, isUserDefined, arginfo_reflection__void, 0)
   ZEND_ME(reflection_class, isAnonymous, arginfo_reflection__void, 0)
   ZEND_ME(reflection_class, isInstantiable, arginfo_reflection__void, 0)
   ZEND_ME(reflection_class, isCloneable, arginfo_reflection__void, 0)
   ZEND_ME(reflection_class, getFileName, arginfo_reflection__void, 0)
   ZEND_ME(reflection_class, getStartLine, arginfo_reflection__void, 0)
   ZEND_ME(reflection_class, getEndLine, arginfo_reflection__void, 0)
   ZEND_ME(reflection_class, getDocComment, arginfo_reflection__void, 0)
   ZEND_ME(reflection_class, getConstructor, arginfo_reflection__void, 0)
   ZEND_ME(reflection_class, hasMethod, arginfo_reflection_class_hasMethod, 0)
   ZEND_ME(reflection_class, getMethod, arginfo_reflection_class_getMethod, 0)
   ZEND_ME(reflection_class, getMethods, arginfo_reflection_class_getMethods, 0)
   ZEND_ME(reflection_class, hasProperty, arginfo_reflection_class_hasProperty, 0)
   ZEND_ME(reflection_class, getProperty, arginfo_reflection_class_getProperty, 0)
   ZEND_ME(reflection_class, getProperties, arginfo_reflection_class_getProperties, 0)
   ZEND_ME(reflection_class, hasConstant, arginfo_reflection_class_hasConstant, 0)
   ZEND_ME(reflection_class, getConstants, arginfo_reflection__void, 0)
   ZEND_ME(reflection_class, getReflectionConstants, arginfo_reflection__void, 0)
   ZEND_ME(reflection_class, getConstant, arginfo_reflection_class_getConstant, 0)
   ZEND_ME(reflection_class, getReflectionConstant, arginfo_reflection_class_getConstant, 0)
   ZEND_ME(reflection_class, getInterfaces, arginfo_reflection__void, 0)
   ZEND_ME(reflection_class, getInterfaceNames, arginfo_reflection__void, 0)
   ZEND_ME(reflection_class, isInterface, arginfo_reflection__void, 0)
   ZEND_ME(reflection_class, getTraits, arginfo_reflection__void, 0)
   ZEND_ME(reflection_class, getTraitNames, arginfo_reflection__void, 0)
   ZEND_ME(reflection_class, getTraitAliases, arginfo_reflection__void, 0)
   ZEND_ME(reflection_class, isTrait, arginfo_reflection__void, 0)
   ZEND_ME(reflection_class, isAbstract, arginfo_reflection__void, 0)
   ZEND_ME(reflection_class, isFinal, arginfo_reflection__void, 0)
   ZEND_ME(reflection_class, getModifiers, arginfo_reflection__void, 0)
   ZEND_ME(reflection_class, isInstance, arginfo_reflection_class_isInstance, 0)
   ZEND_ME(reflection_class, newInstance, arginfo_reflection_class_newInstance, 0)
   ZEND_ME(reflection_class, newInstanceWithoutConstructor, arginfo_reflection_class_newInstanceWithoutConstructor, 0)
   ZEND_ME(reflection_class, newInstanceArgs, arginfo_reflection_class_newInstanceArgs, 0)
   ZEND_ME(reflection_class, getParentClass, arginfo_reflection__void, 0)
   ZEND_ME(reflection_class, isSubclassOf, arginfo_reflection_class_isSubclassOf, 0)
   ZEND_ME(reflection_class, getStaticProperties, arginfo_reflection__void, 0)
   ZEND_ME(reflection_class, getStaticPropertyValue, arginfo_reflection_class_getStaticPropertyValue, 0)
   ZEND_ME(reflection_class, setStaticPropertyValue, arginfo_reflection_class_setStaticPropertyValue, 0)
   ZEND_ME(reflection_class, getDefaultProperties, arginfo_reflection__void, 0)
   ZEND_ME(reflection_class, isIterable, arginfo_reflection__void, 0)
   ZEND_MALIAS(reflection_class, isIterateable, isIterable, arginfo_reflection__void, 0)
   ZEND_ME(reflection_class, implementsInterface, arginfo_reflection_class_implementsInterface, 0)
   ZEND_ME(reflection_class, inNamespace, arginfo_reflection__void, 0)
   ZEND_ME(reflection_class, getNamespaceName, arginfo_reflection__void, 0)
   ZEND_ME(reflection_class, getShortName, arginfo_reflection__void, 0)
   PHP_FE_END
};

ZEND_BEGIN_ARG_INFO_EX(arginfo_reflection_object_export, 0, 0, 1)
ZEND_ARG_INFO(0, argument)
ZEND_ARG_INFO(0, return)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_reflection_object___construct, 0)
ZEND_ARG_INFO(0, argument)
ZEND_END_ARG_INFO()

static const zend_function_entry sg_reflectionObjectFunctions[] = {
   ZEND_ME(reflection_object, export, arginfo_reflection_object_export, ZEND_ACC_STATIC|ZEND_ACC_PUBLIC)
   ZEND_ME(reflection_object, __construct, arginfo_reflection_object___construct, 0)
   PHP_FE_END
};

ZEND_BEGIN_ARG_INFO_EX(arginfo_reflection_property_export, 0, 0, 2)
ZEND_ARG_INFO(0, class)
ZEND_ARG_INFO(0, name)
ZEND_ARG_INFO(0, return)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_reflection_property___construct, 0, 0, 2)
ZEND_ARG_INFO(0, class)
ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_reflection_property_getValue, 0, 0, 0)
ZEND_ARG_INFO(0, object)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_reflection_property_setValue, 0, 0, 1)
ZEND_ARG_INFO(0, object)
ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_reflection_property_setAccessible, 0)
ZEND_ARG_INFO(0, visible)
ZEND_END_ARG_INFO()

static const zend_function_entry sg_reflectionPropertyFunctions[] = {
   ZEND_ME(reflection, __clone, arginfo_reflection__void, ZEND_ACC_PRIVATE|ZEND_ACC_FINAL)
   ZEND_ME(reflection_property, export, arginfo_reflection_property_export, ZEND_ACC_STATIC|ZEND_ACC_PUBLIC)
   ZEND_ME(reflection_property, __construct, arginfo_reflection_property___construct, 0)
   ZEND_ME(reflection_property, __toString, arginfo_reflection__void, 0)
   ZEND_ME(reflection_property, getName, arginfo_reflection__void, 0)
   ZEND_ME(reflection_property, getValue, arginfo_reflection_property_getValue, 0)
   ZEND_ME(reflection_property, setValue, arginfo_reflection_property_setValue, 0)
   ZEND_ME(reflection_property, isPublic, arginfo_reflection__void, 0)
   ZEND_ME(reflection_property, isPrivate, arginfo_reflection__void, 0)
   ZEND_ME(reflection_property, isProtected, arginfo_reflection__void, 0)
   ZEND_ME(reflection_property, isStatic, arginfo_reflection__void, 0)
   ZEND_ME(reflection_property, isDefault, arginfo_reflection__void, 0)
   ZEND_ME(reflection_property, getModifiers, arginfo_reflection__void, 0)
   ZEND_ME(reflection_property, getDeclaringClass, arginfo_reflection__void, 0)
   ZEND_ME(reflection_property, getDocComment, arginfo_reflection__void, 0)
   ZEND_ME(reflection_property, setAccessible, arginfo_reflection_property_setAccessible, 0)
   PHP_FE_END
};

ZEND_BEGIN_ARG_INFO_EX(arginfo_reflection_class_constant_export, 0, 0, 2)
ZEND_ARG_INFO(0, class)
ZEND_ARG_INFO(0, name)
ZEND_ARG_INFO(0, return)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_reflection_class_constant___construct, 0, 0, 2)
ZEND_ARG_INFO(0, class)
ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

static const zend_function_entry sg_reflectionClassConstantFunctions[] = {
   ZEND_ME(reflection, __clone, arginfo_reflection__void, ZEND_ACC_PRIVATE|ZEND_ACC_FINAL)
   ZEND_ME(reflection_class_constant, export, arginfo_reflection_class_constant_export, ZEND_ACC_STATIC|ZEND_ACC_PUBLIC)
   ZEND_ME(reflection_class_constant, __construct, arginfo_reflection_class_constant___construct, 0)
   ZEND_ME(reflection_class_constant, __toString, arginfo_reflection__void, 0)
   ZEND_ME(reflection_class_constant, getName, arginfo_reflection__void, 0)
   ZEND_ME(reflection_class_constant, getValue, arginfo_reflection__void, 0)
   ZEND_ME(reflection_class_constant, isPublic, arginfo_reflection__void, 0)
   ZEND_ME(reflection_class_constant, isPrivate, arginfo_reflection__void, 0)
   ZEND_ME(reflection_class_constant, isProtected, arginfo_reflection__void, 0)
   ZEND_ME(reflection_class_constant, getModifiers, arginfo_reflection__void, 0)
   ZEND_ME(reflection_class_constant, getDeclaringClass, arginfo_reflection__void, 0)
   ZEND_ME(reflection_class_constant, getDocComment, arginfo_reflection__void, 0)
   PHP_FE_END
};

ZEND_BEGIN_ARG_INFO_EX(arginfo_reflection_parameter_export, 0, 0, 2)
ZEND_ARG_INFO(0, function)
ZEND_ARG_INFO(0, parameter)
ZEND_ARG_INFO(0, return)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_reflection_parameter___construct, 0)
ZEND_ARG_INFO(0, function)
ZEND_ARG_INFO(0, parameter)
ZEND_END_ARG_INFO()

static const zend_function_entry sg_reflectionParameterFunctions[] = {
   ZEND_ME(reflection, __clone, arginfo_reflection__void, ZEND_ACC_PRIVATE|ZEND_ACC_FINAL)
   ZEND_ME(reflection_parameter, export, arginfo_reflection_parameter_export, ZEND_ACC_STATIC|ZEND_ACC_PUBLIC)
   ZEND_ME(reflection_parameter, __construct, arginfo_reflection_parameter___construct, 0)
   ZEND_ME(reflection_parameter, __toString, arginfo_reflection__void, 0)
   ZEND_ME(reflection_parameter, getName, arginfo_reflection__void, 0)
   ZEND_ME(reflection_parameter, isPassedByReference, arginfo_reflection__void, 0)
   ZEND_ME(reflection_parameter, canBePassedByValue, arginfo_reflection__void, 0)
   ZEND_ME(reflection_parameter, getDeclaringFunction, arginfo_reflection__void, 0)
   ZEND_ME(reflection_parameter, getDeclaringClass, arginfo_reflection__void, 0)
   ZEND_ME(reflection_parameter, getClass, arginfo_reflection__void, 0)
   ZEND_ME(reflection_parameter, hasType, arginfo_reflection__void, 0)
   ZEND_ME(reflection_parameter, getType, arginfo_reflection__void, 0)
   ZEND_ME(reflection_parameter, isArray, arginfo_reflection__void, 0)
   ZEND_ME(reflection_parameter, isCallable, arginfo_reflection__void, 0)
   ZEND_ME(reflection_parameter, allowsNull, arginfo_reflection__void, 0)
   ZEND_ME(reflection_parameter, getPosition, arginfo_reflection__void, 0)
   ZEND_ME(reflection_parameter, isOptional, arginfo_reflection__void, 0)
   ZEND_ME(reflection_parameter, isDefaultValueAvailable, arginfo_reflection__void, 0)
   ZEND_ME(reflection_parameter, getDefaultValue, arginfo_reflection__void, 0)
   ZEND_ME(reflection_parameter, isDefaultValueConstant, arginfo_reflection__void, 0)
   ZEND_ME(reflection_parameter, getDefaultValueConstantName, arginfo_reflection__void, 0)
   ZEND_ME(reflection_parameter, isVariadic, arginfo_reflection__void, 0)
   PHP_FE_END
};

static const zend_function_entry sg_reflectionTypeFunctions[] = {
   ZEND_ME(reflection, __clone, arginfo_reflection__void, ZEND_ACC_PRIVATE|ZEND_ACC_FINAL)
   ZEND_ME(reflection_type, allowsNull, arginfo_reflection__void, 0)
   ZEND_ME(reflection_type, isBuiltin, arginfo_reflection__void, 0)
   /* ReflectionType::__toString() is deprecated, but we currently do not mark it as such
          * due to bad interaction with the PHPUnit error handler and exceptions in __toString().
          * See PR2137. */
   ZEND_ME(reflection_type, __toString, arginfo_reflection__void, 0)
   PHP_FE_END
};

const zend_function_entry sg_reflectionNamedTypeFunctions[] = {
   ZEND_ME(reflection_named_type, getName, arginfo_reflection__void, 0)
   PHP_FE_END
};

static const zend_function_entry sg_reflectionExtFunctions[] = {
   PHP_FE_END
};

namespace {
void reflection_write_property(zval *object, zval *member, zval *value, void **cache_slot)
{
   if ((Z_TYPE_P(member) == IS_STRING)
       && zend_hash_exists(&Z_OBJCE_P(object)->properties_info, Z_STR_P(member))
       && ((Z_STRLEN_P(member) == sizeof("name") - 1  && !memcmp(Z_STRVAL_P(member), "name",  sizeof("name")))
           || (Z_STRLEN_P(member) == sizeof("class") - 1 && !memcmp(Z_STRVAL_P(member), "class", sizeof("class")))))
   {
      zend_throw_exception_ex(g_reflectionExceptionPtr, 0,
                              "Cannot set read-only property %s::$%s", ZSTR_VAL(Z_OBJCE_P(object)->name), Z_STRVAL_P(member));
   }
   else
   {
      zend_std_write_property(object, member, value, cache_slot);
   }
}
} // anonymous namespace

PHP_MINIT_FUNCTION(reflection)
{
   zend_class_entry _reflection_entry;
   memcpy(&sg_reflectionObjectHandlers, &std_object_handlers, sizeof(zend_object_handlers));
   sg_reflectionObjectHandlers.offset = XtOffsetOf(reflection_object, zo);
   sg_reflectionObjectHandlers.free_obj = reflection_free_objects_storage;
   sg_reflectionObjectHandlers.clone_obj = nullptr;
   sg_reflectionObjectHandlers.write_property = reflection_write_property;
   sg_reflectionObjectHandlers.get_gc = reflection_get_gc;

   INIT_CLASS_ENTRY(_reflection_entry, "ReflectionException", g_reflectionExceptionFunctions);
   g_reflectionExceptionPtr = zend_register_internal_class_ex(&_reflection_entry, zend_ce_exception);

   INIT_CLASS_ENTRY(_reflection_entry, "Reflection", sg_reflectionFunctions);
   g_reflectionPtr = zend_register_internal_class(&_reflection_entry);

   INIT_CLASS_ENTRY(_reflection_entry, "Reflector", sg_reflectorFunctions);
   g_reflectorPtr = zend_register_internal_interface(&_reflection_entry);

   INIT_CLASS_ENTRY(_reflection_entry, "ReflectionFunctionAbstract", sg_reflectionFunctionAbstractFunctions);
   _reflection_entry.create_object = reflection_objects_new;
   g_reflectionFunctionAbstractPtr = zend_register_internal_class(&_reflection_entry);
   zend_class_implements(g_reflectionFunctionAbstractPtr, 1, g_reflectorPtr);
   zend_declare_property_string(g_reflectionFunctionAbstractPtr, "name", sizeof("name")-1, "", ZEND_ACC_ABSTRACT);

   INIT_CLASS_ENTRY(_reflection_entry, "ReflectionFunction", sg_reflectionFunctionFunctions);
   _reflection_entry.create_object = reflection_objects_new;
   g_reflectionFunctionPtr = zend_register_internal_class_ex(&_reflection_entry, g_reflectionFunctionAbstractPtr);
   zend_declare_property_string(g_reflectionFunctionPtr, "name", sizeof("name")-1, "", ZEND_ACC_PUBLIC);

   REGISTER_REFLECTION_CLASS_CONST_LONG(Function, "IS_DEPRECATED", ZEND_ACC_DEPRECATED);

   INIT_CLASS_ENTRY(_reflection_entry, "ReflectionGenerator", sg_reflectionGeneratorFunctions);
   _reflection_entry.create_object = reflection_objects_new;
   g_reflectionGeneratorPtr = zend_register_internal_class(&_reflection_entry);

   INIT_CLASS_ENTRY(_reflection_entry, "ReflectionParameter", sg_reflectionParameterFunctions);
   _reflection_entry.create_object = reflection_objects_new;
   g_reflectionParameterPtr = zend_register_internal_class(&_reflection_entry);
   zend_class_implements(g_reflectionParameterPtr, 1, g_reflectorPtr);
   zend_declare_property_string(g_reflectionParameterPtr, "name", sizeof("name")-1, "", ZEND_ACC_PUBLIC);

   INIT_CLASS_ENTRY(_reflection_entry, "ReflectionType", sg_reflectionTypeFunctions);
   _reflection_entry.create_object = reflection_objects_new;
   g_reflectionTypePtr = zend_register_internal_class(&_reflection_entry);

   INIT_CLASS_ENTRY(_reflection_entry, "ReflectionNamedType", sg_reflectionNamedTypeFunctions);
   _reflection_entry.create_object = reflection_objects_new;
   g_reflectionNamedTypePtr = zend_register_internal_class_ex(&_reflection_entry, g_reflectionTypePtr);

   INIT_CLASS_ENTRY(_reflection_entry, "ReflectionMethod", sg_reflectionMethodFunctions);
   _reflection_entry.create_object = reflection_objects_new;
   g_reflectionMethodPtr = zend_register_internal_class_ex(&_reflection_entry, g_reflectionFunctionAbstractPtr);
   zend_declare_property_string(g_reflectionMethodPtr, "name", sizeof("name")-1, "", ZEND_ACC_PUBLIC);
   zend_declare_property_string(g_reflectionMethodPtr, "class", sizeof("class")-1, "", ZEND_ACC_PUBLIC);

   REGISTER_REFLECTION_CLASS_CONST_LONG(Method, "IS_STATIC", ZEND_ACC_STATIC);
   REGISTER_REFLECTION_CLASS_CONST_LONG(Method, "IS_PUBLIC", ZEND_ACC_PUBLIC);
   REGISTER_REFLECTION_CLASS_CONST_LONG(Method, "IS_PROTECTED", ZEND_ACC_PROTECTED);
   REGISTER_REFLECTION_CLASS_CONST_LONG(Method, "IS_PRIVATE", ZEND_ACC_PRIVATE);
   REGISTER_REFLECTION_CLASS_CONST_LONG(Method, "IS_ABSTRACT", ZEND_ACC_ABSTRACT);
   REGISTER_REFLECTION_CLASS_CONST_LONG(Method, "IS_FINAL", ZEND_ACC_FINAL);

   INIT_CLASS_ENTRY(_reflection_entry, "ReflectionClass", sg_reflectionClassFunctions);
   _reflection_entry.create_object = reflection_objects_new;
   g_reflectionClassPtr = zend_register_internal_class(&_reflection_entry);
   zend_class_implements(g_reflectionClassPtr, 1, g_reflectorPtr);
   zend_declare_property_string(g_reflectionClassPtr, "name", sizeof("name")-1, "", ZEND_ACC_PUBLIC);

   REGISTER_REFLECTION_CLASS_CONST_LONG(Class, "IS_IMPLICIT_ABSTRACT", ZEND_ACC_IMPLICIT_ABSTRACT_CLASS);
   REGISTER_REFLECTION_CLASS_CONST_LONG(Class, "IS_EXPLICIT_ABSTRACT", ZEND_ACC_EXPLICIT_ABSTRACT_CLASS);
   REGISTER_REFLECTION_CLASS_CONST_LONG(Class, "IS_FINAL", ZEND_ACC_FINAL);

   INIT_CLASS_ENTRY(_reflection_entry, "ReflectionObject", sg_reflectionObjectFunctions);
   _reflection_entry.create_object = reflection_objects_new;
   g_reflectionObjectPtr = zend_register_internal_class_ex(&_reflection_entry, g_reflectionClassPtr);

   INIT_CLASS_ENTRY(_reflection_entry, "ReflectionProperty", sg_reflectionPropertyFunctions);
   _reflection_entry.create_object = reflection_objects_new;
   g_reflectionPropertyPtr = zend_register_internal_class(&_reflection_entry);
   zend_class_implements(g_reflectionPropertyPtr, 1, g_reflectorPtr);
   zend_declare_property_string(g_reflectionPropertyPtr, "name", sizeof("name")-1, "", ZEND_ACC_PUBLIC);
   zend_declare_property_string(g_reflectionPropertyPtr, "class", sizeof("class")-1, "", ZEND_ACC_PUBLIC);

   INIT_CLASS_ENTRY(_reflection_entry, "ReflectionClassConstant", sg_reflectionClassConstantFunctions);
   _reflection_entry.create_object = reflection_objects_new;
   g_reflectionClassConstantPtr = zend_register_internal_class(&_reflection_entry);
   zend_class_implements(g_reflectionClassConstantPtr, 1, g_reflectorPtr);
   zend_declare_property_string(g_reflectionClassConstantPtr, "name", sizeof("name")-1, "", ZEND_ACC_PUBLIC);
   zend_declare_property_string(g_reflectionClassConstantPtr, "class", sizeof("class")-1, "", ZEND_ACC_PUBLIC);

   REGISTER_REFLECTION_CLASS_CONST_LONG(Property, "IS_STATIC", ZEND_ACC_STATIC);
   REGISTER_REFLECTION_CLASS_CONST_LONG(Property, "IS_PUBLIC", ZEND_ACC_PUBLIC);
   REGISTER_REFLECTION_CLASS_CONST_LONG(Property, "IS_PROTECTED", ZEND_ACC_PROTECTED);
   REGISTER_REFLECTION_CLASS_CONST_LONG(Property, "IS_PRIVATE", ZEND_ACC_PRIVATE);

   return SUCCESS;
}

zend_module_entry g_reflectionModuleEntry = {
   STANDARD_MODULE_HEADER,
   "Reflection",
   sg_reflectionExtFunctions,
   PHP_MINIT(reflection),
   nullptr,
   nullptr,
   nullptr,
   nullptr,
   PHP_REFLECTION_VERSION,
   STANDARD_MODULE_PROPERTIES
};

bool register_reflection_module()
{
   g_reflectionModuleEntry.type = MODULE_PERSISTENT;
   g_reflectionModuleEntry.module_number = zend_next_free_module();
   g_reflectionModuleEntry.handle = nullptr;
   if ((EG(current_module) = zend_register_module_ex(&g_reflectionModuleEntry)) == nullptr) {
      return false;
   }
   return true;
}

} // runtime
} // polar
