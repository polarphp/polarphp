//===--------------- SyntaxKind.h - Syntax kind definitions ---------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2018 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
//
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/05/08.

#ifndef POLARPHP_SYNTAX_KIND_H
#define POLARPHP_SYNTAX_KIND_H

#include "polarphp/basic/InlineBitfield.h"
#include "polarphp/basic/ByteTreeSerialization.h"
#include "polarphp/syntax/SyntaxKindEnumDefs.h"
#include "llvm/Support/YAMLTraits.h"

namespace polar::syntax {

using polar::basic::count_bits_used;
using llvm::StringRef;
using llvm::raw_ostream;

enum : unsigned {
   NumSyntaxKindBits = count_bits_used(static_cast<unsigned>(SyntaxKind::Unknown))
};

void dump_syntax_kind(raw_ostream &outStream, const SyntaxKind kind);

/// Whether this kind is a syntax collection.
bool is_collection_kind(SyntaxKind kind);
bool is_decl_kind(SyntaxKind kind);
bool is_type_kind(SyntaxKind kind);
bool is_stmt_kind(SyntaxKind kind);
bool is_expr_kind(SyntaxKind kind);
bool is_token_kind(SyntaxKind kind);
bool is_unknown_kind(SyntaxKind kind);
SyntaxKind get_unknown_kind(SyntaxKind kind);
bool parser_shall_omit_when_no_children(SyntaxKind kind);
StringRef retrieve_syntax_kind_text(SyntaxKind kind);
int retrieve_syntax_kind_serialization_code(SyntaxKind kind);
std::pair<std::uint32_t, std::uint32_t> retrieve_syntax_kind_child_count(SyntaxKind kind, bool &exist);
} // polar::syntax

namespace polar::basic::bytetree {

using polar::syntax::SyntaxKind;

template <>
struct WrapperTypeTraits<syntax::SyntaxKind>
{
   // Explicitly spell out all SyntaxKinds to keep the serialized value stable
   // even if its members get reordered or members get removed
   static uint16_t numericValue(const syntax::SyntaxKind &kind)
   {
      switch (kind) {
      case syntax::SyntaxKind::Token:
         return 0;
      case syntax::SyntaxKind::Unknown:
         return 1;
      default:
         llvm_unreachable("unhandled kind");
      }
   }

   static void write(ByteTreeWriter &writer, const SyntaxKind &kind,
                     unsigned index)
   {
      writer.write(numericValue(kind), index);
   }
};

} // polar::basic::bytetree

namespace llvm::yaml {

/// Deserialization traits for SyntaxKind.
template <>
struct ScalarEnumerationTraits<polar::syntax::SyntaxKind>
{
   static void enumeration(IO &in, polar::syntax::SyntaxKind &value)
   {
      in.enumCase(value, "Token", polar::syntax::SyntaxKind::Token);
      in.enumCase(value, "Unknown", polar::syntax::SyntaxKind::Unknown);
   }
};

} // llvm::yaml

namespace polar::utils {
using llvm::raw_ostream;
raw_ostream &operator<<(raw_ostream &outStream, polar::syntax::SyntaxKind kind);
} // polar::utils

#endif // POLARPHP_SYNTAX_KIND_H
