//===--- XMLUtils.h - Various XML utility routines --------------*- C++ -*-===//
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
// Created by polarboy on 2019/04/27.

#ifndef POLARPHP_MARKUP_XML_UTILS_H
#define POLARPHP_MARKUP_XML_UTILS_H

#include "llvm/ADTStringRef.h"
#include "llvm/Support/raw_ostream.h"

namespace polar::markup {

using polar::utils::raw_ostream;
using polar::basic::StringRef;

// FIXME: copied from Clang's
// CommentASTToXMLConverter::appendToResultWithXMLEscaping
static inline void append_with_xml_escaping(raw_ostream &outStream, StringRef str)
{
   for (const char c : str) {
      switch (c) {
      case '&':
         outStream << "&amp;";
         break;
      case '<':
         outStream << "&lt;";
         break;
      case '>':
         outStream << "&gt;";
         break;
      case '"':
         outStream << "&quot;";
         break;
      case '\'':
         outStream << "&apos;";
         break;
      default:
         outStream << c;
         break;
      }
   }
}

// FIXME: copied from Clang's
// CommentASTToXMLConverter::appendToResultWithCDATAEscaping
static inline void append_with_cdata_escaping(raw_ostream &outStream, StringRef str)
{
   if (str.empty()) {
      return;
   }
   outStream << "<![CDATA[";
   while (!str.empty()) {
      size_t pos = str.find("]]>");
      if (pos == 0) {
         outStream << "]]]]><![CDATA[>";
         str = str.drop_front(3);
         continue;
      }
      if (pos == StringRef::npos) {
         pos = str.size();
      }
      outStream << str.substr(0, pos);
      str = str.drop_front(pos);
   }
   outStream << "]]>";
}

} // polar::markup

#endif // POLARPHP_MARKUP_XML_UTILS_H
