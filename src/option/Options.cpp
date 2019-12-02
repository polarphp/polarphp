//===--- Options.cpp - Option info & table --------------------------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/12/02.

#include "polarphp/option/Options.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Option/OptTable.h"
#include "llvm/Option/Option.h"

namespace polar::options {

using namespace llvm::opt;

#define PREFIX(NAME, VALUE) static const char *const NAME[] = VALUE;
#include "polarphp/option/OptionsDef.h"
#undef PREFIX

static const OptTable::Info sg_infoTable[] = {
   #define OPTION(PREFIX, NAME, ID, KIND, GROUP, ALIAS, ALIASARGS, FLAGS, PARAM,  \
      HELPTEXT, METAVAR, VALUES)                                      \
{PREFIX, NAME,  HELPTEXT,    METAVAR,     OPT_##ID,  Option::KIND##Class,    \
      PARAM,  FLAGS, OPT_##GROUP, OPT_##ALIAS, ALIASARGS, VALUES},
   #include "polarphp/option/OptionsDef.h"
   #undef OPTION
};

namespace {

class PolarphpOptTable : public OptTable
{
public:
   PolarphpOptTable()
      : OptTable(sg_infoTable)
   {}
};

} // end anonymous namespace

} // polar::options

namespace polar {
using namespace llvm::opt;
using polar::options::PolarphpOptTable;
std::unique_ptr<OptTable> create_polarphp_opt_table()
{
   return std::unique_ptr<OptTable>(new PolarphpOptTable());
}
} // polar
