// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarPHP software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarPHP project authors
//
// Created by polarboy on 2018/01/26.

#include "php/kernel/Utils.h"
#include "polarphp/global/PolarVersion.h"

namespace php {
namespace kernel {

std::string retrieve_version_str()
{
   return POLARPHP_PACKAGE_STRING;
}

int retrieve_major_version()
{
   return POLARPHP_VERSION_MAJOR;
}

int retrieve_minor_version()
{
   return POLARPHP_VERSION_MINOR;
}

int retrieve_patch_version()
{
   return POLARPHP_VERSION_PATCH;
}

int retrieve_version_id()
{
   return POLARPHP_VERSION_ID;
}

} // kernel
} // php
