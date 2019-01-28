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

#ifndef POLARPHP_VMAPI_UTILS_ORIG_EXCEPTION_H
#define POLARPHP_VMAPI_UTILS_ORIG_EXCEPTION_H

#include <string>

#include "polarphp/vm/internal/DepsZendVmHeaders.h"
#include "polarphp/vm/utils/Exception.h"

namespace polar {
namespace vmapi {

class OrigException : public Exception
{
public:
   /**
    * Constructor
    * @param  object  The object that was thrown
    */
   OrigException(zend_object *object);

   /**
    * Copy constructor
    * @param  exception
    */
   OrigException(const OrigException &exception);

   /**
    * Move constructor
    * @param  exception
    */
   OrigException(OrigException &&exception);
   /**
    * Destructor
    */
   virtual ~OrigException() noexcept;

   /**
    * This is _not_ a native exception, it was thrown by a PHP script
    * @return bool
    */
   virtual bool native() const override;

   /**
    * Reactivate the exception
    */
   void reactivate();

   /**
    * Returns the exception code
    *
    * @note   This only works if the exception was originally
    *         thrown in PHP userland. If the native() member
    *         function returns true, this function will not
    *         be able to correctly provide the filename.
    *
    * @return The exception code
    */
   virtual long int getCode() const noexcept override;

   /**
    * Retrieve the filename the exception was thrown in
    *
    * @return The filename the exception was thrown in
    */
   virtual const std::string &getFileName() const noexcept override;

   /**
    * Retrieve the line at which the exception was thrown
    *
    * @return The line number the exception was thrown at
    */
   virtual long int getLine() const noexcept override;
private:
   /**
    * Is this a an exception that was caught by extension C++ code.
    *
    * When the object is initially created, we assume that it will be caught
    * by C++ code. If it later turns out that the polarphp can catch this
    * exception after the extension C++ code ran, the variable is set back
    * to false.
    *
    * @var bool
    */
   bool m_handled = true;

   /**
    * The PHP exception code
    * @var    long int
    */
   long int m_code;

   /**
    * PHP source file
    * @var std::string
    */
   std::string m_file;

   /**
    * PHP source line
    * @var long int
    */
   long int m_line;

};

namespace {

inline void process_exception(Exception &exception)
{
   if (exception.native()) {
      zend_throw_exception(zend_exception_get_default(), exception.what(), 0);
   } else if (!exception.report()) {
      // this is not a native exception, so it was originally thrown by a
      // php script, and then not caught by the c++ of the extension, we are
      // going to tell to the exception that it is still active
      OrigException &orig = static_cast<OrigException &>(exception);
      orig.reactivate();
   }
}

} // anonymous namespace

} // vmapi
} // polar

#endif // POLARPHP_VMAPI_UTILS_ORIG_EXCEPTION_H
