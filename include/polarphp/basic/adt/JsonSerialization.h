// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/11/13.

#ifndef POLARPHP_BASIC_ADT_JSON_SERIALIZATION_H
#define POLARPHP_BASIC_ADT_JSON_SERIALIZATION_H

#include "nlohmann/json.hpp"

namespace polar::basic {

using nlohmann::json;
class StringRef;

void to_json(json &jsonObject, const StringRef &str);

} // polar::basic

#endif // POLARPHP_BASIC_ADT_JSON_SERIALIZATION_H