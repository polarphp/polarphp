// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/23.

#include "polarphp/vm/utils/Exception.h"
#include "polarphp/vm/utils/FatalError.h"
#include "polarphp/vm/utils/NotImplemented.h"
#include "polarphp/vm/utils/OrigException.h"

namespace polar {
namespace vmapi {

Exception::Exception(std::string message)
   : m_message(std::move(message))
{}

Exception::~Exception()
{}

const char *Exception::what() const noexcept
{
   return m_message.c_str();
}

const std::string &Exception::getMessage() const noexcept
{
   return m_message;
}

long int Exception::getCode() const noexcept
{
   return -1;
}

const std::string &Exception::getFileName() const noexcept
{
   static std::string file{"<filename unknown>"};
   return file;
}

long int Exception::getLine() const noexcept
{
   return -1;
}

bool Exception::native() const
{
   return true;
}

bool Exception::report() const
{
   return false;
}


OrigException::OrigException(zend_object *object)
   : Exception(std::string{ZSTR_VAL(object->ce->name), ZSTR_LEN(object->ce->name)})
{
   // the result value from zend and the object zval
   zval result;
   zval properties;
   ZVAL_OBJ(&properties, object);
   // retrieve the message, filename, error code and line number
   auto message = zval_get_string(zend_read_property(Z_OBJCE(properties), &properties,
                                                     ZEND_STRL("message"), 1, &result));
   auto file = zval_get_string(zend_read_property(Z_OBJCE(properties), &properties, ZEND_STRL("file"), 1, &result));
   auto code = zval_get_long(zend_read_property(Z_OBJCE(properties), &properties, ZEND_STRL("code"), 1, &result));
   auto line = zval_get_long(zend_read_property(Z_OBJCE(properties), &properties, ZEND_STRL("line"), 1, &result));
   // store the message, code, filename and line number
   m_message.assign(ZSTR_VAL(message), ZSTR_LEN(message));
   m_code = code;
   m_file.assign(ZSTR_VAL(file), ZSTR_LEN(file));
   m_line = line;
   // clean up message and file strings
   /// review for memory leak
   zend_string_release(message);
   zend_string_release(file);
}

OrigException::OrigException(const OrigException &exception)
   : Exception("OrigException"),
     m_handled(exception.m_handled)
{}

OrigException::OrigException(OrigException &&exception)
   : Exception("OrigException"),
     m_handled(exception.m_handled)
{
   exception.m_handled = true;
}

OrigException::~OrigException() noexcept
{
   if (!m_handled) {
      return;
   }
   zend_clear_exception();
}

bool OrigException::native() const
{
   return false;
}

void OrigException::reactivate()
{
   // it was not handled by extension C++ code
   m_handled = false;
}

long int OrigException::getCode() const noexcept
{
   return m_code;
}

const std::string &OrigException::getFileName() const noexcept
{
   return m_file;
}

long int OrigException::getLine() const noexcept
{
   return m_line;
}

NotImplemented::NotImplemented()
   : std::exception()
{}

NotImplemented::~NotImplemented() noexcept
{}

FatalError::FatalError(const std::string &message)
   : Exception(message)
{}

FatalError::~FatalError()
{}

bool FatalError::native() const
{
   // although it is native, we return 0 because it should not persist
   // as exception, but it should live on as zend_error() in stead
   return false;
}

bool FatalError::report() const
{
   zend_error(E_ERROR, "%s", what());
   return true;
}

} // vmapi
} // polar
