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

#ifndef POLARPHP_VMAPI_UTILS_EXCEPTION_H
#define POLARPHP_VMAPI_UTILS_EXCEPTION_H

#include "polarphp/vm/ZendApi.h"

#include <exception>
#include <string>

namespace polar {
namespace vmapi {

class VMAPI_DECL_EXPORT Exception : public std::exception
{
public:
   /**
    * Constructor
    *
    * @param  message The exception message
    */
   Exception(std::string message);
   /**
    * Destructor
    */
   virtual ~Exception();

   /**
    * Overridden what method
    * @return const char *
    */
   virtual const char *what() const noexcept override;

   /**
    *  Returns the message of the exception.
    *  @return &string
    */
   const std::string &getMessage() const noexcept;

   /**
    * Returns the exception code
    *
    * @return The exception code
    */
   virtual long int getCode() const noexcept;

   /**
    * Retrieve the filename the exception was thrown in
    *
    * @note   This only works if the exception was originally
    *         thrown in PHP userland. If the native() member
    *         function returns true, this function will not
    *         be able to correctly provide the filename.
    *
    * @return The filename the exception was thrown in
    */
   virtual const std::string &getFileName() const noexcept;

   /**
    * Retrieve the line at which the exception was thrown
    */
   virtual long int getLine() const noexcept;

   /**
    * Is this a native exception (one that was thrown from C++ code)
    * @return bool
    */
   virtual bool native() const;
   /**
    * Report this error as a fatal error
    * @return bool
    */
   virtual bool report() const;
protected:
   /**
    * The exception message
    * @var    char*
    */
   std::string m_message;
};

} // vmapi
} // polar

#endif // POLARPHP_VMAPI_UTILS_EXCEPTION_H
