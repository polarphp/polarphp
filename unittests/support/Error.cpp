// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarPHP software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://polarphp.org/LICENSE.txt for license information
// See http://polarphp.org/CONTRIBUTORS.txt for the list of polarPHP project authors
//
// Created by softboy on 2018/07/12.

#include "Error.h"
#include "polarphp/basic/adt/StringRef.h"
#include <iostream>

namespace polar {
namespace unittest {

internal::ErrorHolder internal::take_error(Error error)
{
  bool succeeded = !static_cast<bool>(error);
  std::string message;
  if (!succeeded) {
     message = polar::utils::to_string(std::move(error));
  }
  return {succeeded, message};
}

} // unittest
} // polar
