// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/10/10.

#include "polarphp/global/AbiBreaking.h"
#include "polarphp/basic/adt/Twine.h"
#include "polarphp/utils/Error.h"
#include "polarphp/utils/ErrorHandling.h"
#include "polarphp/utils/ManagedStatics.h"

#include <system_error>

namespace {

enum class ErrorErrorCode : int
{
   MultipleErrors = 1,
   FileError,
   InconvertibleError
};

// FIXME: This class is only here to support the transition to llvm::Error. It
// will be removed once this transition is complete. Clients should prefer to
// deal with the Error value directly, rather than converting to error_code.
class ErrorErrorCategory : public std::error_category
{
public:
   const char *name() const noexcept override
   {
      return "Error";
   }

   std::string message(int condition) const override
   {
      switch (static_cast<ErrorErrorCode>(condition)) {
      case ErrorErrorCode::MultipleErrors:
         return "Multiple errors";
      case ErrorErrorCode::InconvertibleError:
         return "Inconvertible error value. An error has occurred that could "
                "not be converted to a known std::error_code. Please file a "
                "bug.";
      case ErrorErrorCode::FileError:
         return "A file error occurred.";
      }
      polar_unreachable("Unhandled error code");
   }
};

} // anonymous namespace

static polar::utils::ManagedStatic<ErrorErrorCategory> sg_errorErrorCat;

namespace polar {
namespace utils {

void ErrorInfoBase::getAnchor() {}
char ErrorInfoBase::sm_id = 0;
char ErrorList::sm_id = 0;
char EcError::sm_id = 0;
char StringError::sm_id = 0;
char FileError::sm_id = 0;

void log_all_unhandled_errors(Error error, RawOutStream &out, const std::string &errorBanner)
{
   if (!error) {
      return;
   }
   out << errorBanner;
   handle_all_errors(std::move(error), [&](const ErrorInfoBase &errorInfo) {
      errorInfo.log(out);
      out << "\n";
   });
}


std::error_code ErrorList::convertToErrorCode() const
{
   return std::error_code(static_cast<int>(ErrorErrorCode::MultipleErrors),
                          *sg_errorErrorCat);
}

std::error_code inconvertible_error_code()
{
   return std::error_code(static_cast<int>(ErrorErrorCode::InconvertibleError),
                          *sg_errorErrorCat);
}

std::error_code FileError::convertToErrorCode() const
{
   return std::error_code(static_cast<int>(ErrorErrorCode::FileError),
                          *sg_errorErrorCat);
}

Error error_code_to_error(std::error_code errorCode)
{
   if (!errorCode) {
      return Error::getSuccess();
   }
   return Error(std::make_unique<EcError>(EcError(errorCode)));
}

std::error_code error_to_error_code(Error error)
{
   std::error_code errorCode;
   handle_all_errors(std::move(error), [&](const ErrorInfoBase &errorInfo) {
      errorCode = errorInfo.convertToErrorCode();
   });
   if (errorCode == inconvertible_error_code()) {
      report_fatal_error(errorCode.message());
   }
   return errorCode;
}

#if POLAR_ENABLE_ABI_BREAKING_CHECKS
void Error::fatalUncheckedError() const
{
   debug_stream() << "Program aborted due to an unhandled Error:\n";
   if (getPtr()) {
      getPtr()->log(debug_stream());
   } else {
      debug_stream() << "Error value was Success. (Note: Success values must still be "
                        "checked prior to being destroyed).\n";
   }
   abort();
}
#endif

StringError::StringError(std::error_code errorCode, const Twine &str)
   : m_msg(str.getStr()),
     m_errorCode(errorCode)
{}

StringError::StringError(const Twine &str, std::error_code errorCode)
   : m_msg(str.getStr()),
     m_errorCode(errorCode),
     m_printMsgOnly(true)
{}

void StringError::log(RawOutStream &out) const
{
   if (m_printMsgOnly) {
      out << m_msg;
   } else {
      out << m_errorCode.message();
      if (!m_msg.empty()) {
         out << (" " + m_msg);
      }
   }
}

std::error_code StringError::convertToErrorCode() const
{
   return m_errorCode;
}

Error create_string_error(std::error_code errorCode, char const *msg)
{
   return make_error<StringError>(msg, errorCode);
}

void report_fatal_error(Error error, bool)
{
   assert(error && "report_fatal_error called with success value");
   std::string errorMsg;
   {
      RawStringOutStream errorStream(errorMsg);
      log_all_unhandled_errors(std::move(error), errorStream);
   }
   report_fatal_error(errorMsg);
}

} // utils
} // polar

PolarErrorTypeId polar_get_error_type_id(PolarErrorRef error)
{
   return reinterpret_cast<polar::utils::ErrorInfoBase *>(error)->dynamicClassId();
}

void polar_consume_error(PolarErrorRef error)
{
   polar::utils::consume_error(polar::utils::unwrap(error));
}

char *polar_get_error_message(PolarErrorRef error)
{
   std::string temp = polar::utils::to_string(polar::utils::unwrap(error));
   char *errMsg = new char[temp.size() + 1];
   memcpy(errMsg, temp.data(), temp.size());
   errMsg[temp.size()] = '\0';
   return errMsg;
}

void polar_dispose_error_message(char *errorMsg)
{
   delete[] errorMsg;
}

PolarErrorTypeId polar_get_string_error_type_id()
{
   return reinterpret_cast<void *>(&polar::utils::StringError::sm_id);
}


#ifndef _MSC_VER
namespace polar {

// One of these two variables will be referenced by a symbol defined in
// We provide a link-time (or load time for DSO) failure when
// there is a mismatch in the build configuration of the API client and LLVM.
#if POLAR_ENABLE_ABI_BREAKING_CHECKS
int EnableABIBreakingChecks;
#else
int DisableABIBreakingChecks;
#endif

} // end namespace polar
#endif

