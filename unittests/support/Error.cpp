// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarPHP software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarPHP project authors
//
// Created by polarboy on 2018/07/12.

#include "Error.h"
#include "polarphp/basic/adt/StringRef.h"
#include <iostream>

namespace polar {
namespace unittest {

using polar::utils::handle_all_errors;
using polar::utils::ErrorInfoBase;

internal::ErrorHolder internal::take_error(Error error)
{
   std::vector<std::shared_ptr<ErrorInfoBase>> infos;
   handle_all_errors(std::move(error),
                   [&infos](std::unique_ptr<ErrorInfoBase> info) {
      infos.emplace_back(std::move(info));
   });
   return {std::move(infos)};
}

} // unittest
} // polar
