// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/05/09.

#include "polarphp/syntax/RawSyntax.h"
#include "polarphp/basic/ColorUtils.h"

namespace polar::syntax {

namespace {

bool is_trivial_syntax_kind(SyntaxKind kind)
{
   if (is_unknown_kind(kind)) {
      return true;
   }
   if (is_collection_kind(kind)) {
      return true;
   }
   switch(kind) {
   return true;
   default:
      return false;
   }
}

void print_syntax_kind(SyntaxKind kind, RawOutStream &outStream,
                       SyntaxPrintOptions opts, bool open)
{
   std::unique_ptr<polar::basic::OsColor> color;
   if (opts.visual) {
      color.reset(new polar::basic::OsColor(outStream, RawOutStream::Colors::GREEN));
   }
   outStream << "<";
   if (!open) {
      outStream << "/";
   }
   dump_syntax_kind(outStream, kind);
   outStream << ">";
}

}



RawSyntax::~RawSyntax()
{

}

} // polar::syntax
