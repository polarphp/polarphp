//===--- Syntax.cpp - Swift Syntax Implementation -------------------------===//
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
// Created by polarboy on 2019/05/11.

#include "polarphp/syntax/Syntax.h"
#include "polarphp/syntax/SyntaxData.h"
#include "polarphp/syntax/SyntaxNodeVisitor.h"

#include <optional>

namespace polar::syntax {

Syntax::~Syntax()
{}

RefCountPtr<RawSyntax> Syntax::getRaw() const
{
   return m_data->getRaw();
}

SyntaxKind Syntax::getKind() const
{
   return getRaw()->getKind();
}

void Syntax::print(utils::raw_ostream &outStream, SyntaxPrintOptions opts) const
{
   if (auto raw = getRaw()) {
      raw->print(outStream, opts);
   }
}

void Syntax::dump() const
{
   return getRaw()->dump();
}

void Syntax::dump(utils::raw_ostream &outStream, [[maybe_unused]] unsigned indent) const
{
   return getRaw()->dump(outStream, 0);
}

bool Syntax::isDecl() const
{
  return m_data->isDecl();
}

bool Syntax::isStmt() const
{
  return m_data->isStmt();
}

bool Syntax::isExpr() const
{
  return m_data->isExpr();
}

bool Syntax::isToken() const
{
  return getRaw()->isToken();
}

bool Syntax::isUnknown() const
{
  return m_data->isUnknown();
}

bool Syntax::isPresent() const
{
  return getRaw()->isPresent();
}

bool Syntax::isMissing() const
{
  return getRaw()->isMissing();
}

std::optional<Syntax> Syntax::getParent() const
{
  auto parentData = getData().getParent();
  if (!parentData) {
     return std::nullopt;
  }
  return std::optional<Syntax> {
    Syntax { m_root, parentData }
  };
}

Syntax Syntax::getRoot() const
{
  return { m_root, m_root.get() };
}

size_t Syntax::getNumChildren() const
{
  return m_data->getNumChildren();
}

std::optional<Syntax> Syntax::getChild(const size_t index) const
{
  auto childData = m_data->getChild(index);
  if (!childData) {
     return std::nullopt;
  }
  return Syntax {m_root, childData.get()};
}

} // polar::syntax
