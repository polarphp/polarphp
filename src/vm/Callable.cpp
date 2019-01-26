// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/24.

#include "polarphp/vm/Callable.h"
#include "polarphp/vm/internal/CallablePrivate.h"
#include "polarphp/vm/lang/Parameter.h"
#include "polarphp/vm/utils/OrigException.h"
#include "polarphp/vm/ds/Variant.h"

#include <ostream>
#include <cstring>

namespace polar {
namespace vmapi {

namespace internal
{

int get_raw_type(Type type)
{
   if (Type::Undefined == type) {
      return IS_UNDEF;
   } else if (Type::Null == type) {
      return IS_NULL;
   } else if (Type::Boolean == type || Type::True == type || Type::False == type) {
      return _IS_BOOL;
   } else if (Type::Numeric == type) {
      return IS_LONG;
   } else if (Type::Double == type) {
      return IS_DOUBLE;
   } else if (Type::String == type) {
      return IS_STRING;
   } else if (Type::Array == type) {
      return IS_ARRAY;
   } else if (Type::Object == type) {
      return IS_OBJECT;
   } else if (Type::Callable == type) {
      return IS_CALLABLE;
   }else {
      return IS_UNDEF;
   }
}

CallablePrivate::CallablePrivate(StringRef name, ZendCallable callable, const Arguments &arguments)
   : m_argc(arguments.size()),
     m_callable(callable),
     m_name(name),
     m_argv(new zend_internal_arg_info[arguments.size() + 2])
{
   // first entry record function infomation, so skip it
   int i = 1;
   for (auto &argument : arguments) {
      if (argument.isRequired()) {
         m_required++;
      }
      // setup the actually argument info
      setupCallableArgInfo(&m_argv[i++], argument);
   }
   /// last item record callable object address info
   m_argv[i].name = nullptr;
}

CallablePrivate::CallablePrivate(StringRef name, const Arguments &arguments)
   : CallablePrivate(name, nullptr,  arguments)
{}

void CallablePrivate::initialize(zend_function_entry *entry, bool isMethod, int flags) const
{
   if (m_callable) {
      entry->handler = m_callable;
   } else {
      m_argv[m_argc + 1].type = reinterpret_cast<zend_type>(this);
      // we use our own invoke method, which does a lookup
      // in the map we just installed ourselves in
      entry->handler = &Callable::invoke;
   }
   entry->fname = m_name.data();
   entry->arg_info = m_argv.get();
   entry->num_args = m_argc;
   entry->flags = flags;
   // first arg info save the infomation of callable itself
   initialize(reinterpret_cast<zend_internal_function_info *>(m_argv.get()), isMethod);
}

void CallablePrivate::initialize(zend_internal_function_info *info, bool isMethod) const
{
   // we use new facility type system for zend_internal_function_info / zend_function_info
   if (isMethod) {
      // method
      if (m_name != "__construct" && m_name != "__destruct") {
         if (m_returnType == Type::Object) {
            info->type = ZEND_TYPE_ENCODE_CLASS(m_retClsName.c_str(), m_returnTypeNullable);
         } else {
            info->type = ZEND_TYPE_ENCODE(get_raw_type(m_returnType), m_returnTypeNullable);
         }
      } else {
         ///
         /// need review here, Z_L(1) mean nullable
         ///
         info->type = Z_L(1);
      }
   } else {
      // function
      if (m_returnType == Type::Object) {
         info->type = ZEND_TYPE_ENCODE_CLASS(m_retClsName.c_str(), m_returnTypeNullable);
      } else {
         info->type = ZEND_TYPE_ENCODE(get_raw_type(m_returnType), m_returnTypeNullable);
      }
   }
   info->required_num_args = m_required;
   ///
   /// TODO current we don't support return by reference
   ///
   info->return_reference = false;
   info->_is_variadic = false;
}

void CallablePrivate::initialize(const std::string &prefix, zend_function_entry *entry, int flags)
{
   // if there is a namespace prefix, we should adjust the name
   if (!prefix.empty()) {
      m_name = prefix + '\\' + m_name;
   }
   // call base initialize
   CallablePrivate::initialize(entry, false, flags);
}

void CallablePrivate::setupCallableArgInfo(zend_internal_arg_info *info, const Argument &arg) const
{
   std::memset(info, 0, sizeof(zend_internal_arg_info));
   info->name = arg.getName();
   int rawType = get_raw_type(arg.getType());
   if (rawType == IS_OBJECT) {
      if (nullptr != arg.getClassName()) {
         info->type = ZEND_TYPE_ENCODE_CLASS(arg.getClassName(), arg.isNullable());
      } else {
         info->type = ZEND_TYPE_ENCODE(IS_OBJECT, arg.isNullable());
      }
   } else {
      info->type = ZEND_TYPE_ENCODE(rawType, arg.isNullable());
   }
   // from PHP 5.6 and onwards, an is_variadic property can be set, this
   // specifies whether this argument is the first argument that specifies
   // the type for a variable length list of arguments. For now we only
   // support methods and functions with a fixed number of arguments.
   info->is_variadic       = arg.isVariadic();
   info->pass_by_reference = arg.isReference();
}

} // internal

Callable::Callable()
{}

Callable::Callable(StringRef name, ZendCallable callable, const Arguments &arguments)
   : m_implPtr(new CallablePrivate(name, callable, arguments))
{
}

Callable::Callable(StringRef name, const Arguments &arguments)
   : Callable(name, nullptr, arguments)
{}

Callable::Callable(Callable &&other) noexcept
   : m_implPtr(std::move(other.m_implPtr))
{
}

Callable::Callable(CallablePrivate *implPtr)
   : m_implPtr(implPtr)
{}

Callable::Callable(const Callable &other)
   : m_implPtr(other.m_implPtr)
{}


Callable::~Callable()
{}

Callable &Callable::operator=(const Callable &other)
{
   if (this != &other) {
      m_implPtr = other.m_implPtr;
   }
   return *this;
}

Callable &Callable::operator=(Callable &&other) noexcept
{
   assert(this != &other);
   m_implPtr = std::move(other.m_implPtr);
   return *this;
}

Callable &Callable::setReturnType(Type type, bool nullable) noexcept
{
   if (Type::Object != type && Type::Resource != type &&
       Type::Ptr != type && Type::ConstantAST != type &&
       Type::Error != type) {
      VMAPI_D(Callable);
      implPtr->m_returnType = type;
      implPtr->m_returnTypeNullable = nullable;
      implPtr->m_retClsName.clear();
   }
   return *this;
}

Callable &Callable::setReturnType(StringRef clsName, bool nullable) noexcept
{
   VMAPI_D(Callable);
   implPtr->m_returnType = Type::Object;
   implPtr->m_retClsName = clsName.getStr();
   implPtr->m_returnTypeNullable = nullable;
   return *this;
}

Callable &Callable::markDeprecated() noexcept
{
   m_implPtr->m_flags |= Modifier::Deprecated;
   return *this;
}

Variant Callable::invoke(Parameters &)
{
   return nullptr;
}

zend_function_entry Callable::buildCallableEntry(bool isMethod) const noexcept
{
   zend_function_entry entry;
   VMAPI_D(const Callable);
   implPtr->initialize(&entry, isMethod, static_cast<int>(implPtr->m_flags));
   return entry;
}

void Callable::setupCallableArgInfo(zend_internal_arg_info *info, const Argument &arg) const
{
   getImplPtr()->setupCallableArgInfo(info, arg);
}

void Callable::invoke(INTERNAL_FUNCTION_PARAMETERS)
{
   uint32_t argc       = EX(func)->common.num_args;
   zend_arg_info *info = EX(func)->common.arg_info;
   assert(info[argc].type != 0 && info[argc].name == nullptr);
   Callable *callable = reinterpret_cast<Callable *>(info[argc].name);
   // check if sufficient parameters were passed (for some reason this check
   // is not done by Zend, so we do it here ourselves)
   if (ZEND_NUM_ARGS() < callable->m_implPtr->m_required) {
      vmapi::warning() << get_active_function_name() << "() expects at least "
                       << callable->m_implPtr->m_required << " parameter(s)," << ZEND_NUM_ARGS()
                       << " given" << std::flush;
      RETURN_NULL();
   } else {
      Parameters params(getThis(), ZEND_NUM_ARGS());
      // the function we called may throw exception
      try {
         Variant result(callable->invoke(params));
         RETVAL_ZVAL(&result.getZval(), 1, 0);
      } catch (Exception &exception) {
         process_exception(exception);
      }
   }
}

void Callable::initialize(zend_function_entry *entry, bool isMethod, int flags) const
{
   VMAPI_D(const Callable);
   implPtr->initialize(entry, isMethod, flags);
}

void Callable::initialize(zend_internal_function_info *info, bool isMethod) const
{
   VMAPI_D(const Callable);
   implPtr->initialize(info, isMethod);
}

void Callable::initialize(const std::string &prefix, zend_function_entry *entry)
{
   VMAPI_D(Callable);
   implPtr->initialize(prefix, entry);
}

} // vmapi
} // polar
