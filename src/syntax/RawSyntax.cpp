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
   case SyntaxKind::SourceFile:
   case SyntaxKind::InnerStmt:
   case SyntaxKind::TopStmt:
   default:
      return false;
   }
}

void print_syntax_kind(SyntaxKind kind, raw_ostream &outStream,
                       SyntaxPrintOptions opts, bool open)
{
   std::unique_ptr<polar::basic::OsColor> color;
   if (opts.visual) {
      color.reset(new polar::basic::OsColor(outStream, raw_ostream::Colors::GREEN));
   }
   outStream << "<";
   if (!open) {
      outStream << "/";
   }
   dump_syntax_kind(outStream, kind);
   outStream << ">";
}

} // anonymous namespace

unsigned RawSyntax::sm_nextFreeNodeId = 1;

RawSyntax::RawSyntax(SyntaxKind kind, ArrayRef<RefCountPtr<RawSyntax>> layout,
                     SourcePresence presence, const RefCountPtr<SyntaxArena> &arena,
                     std::optional<unsigned> nodeId)
{
   assert(kind != SyntaxKind::Token &&
         "'token' syntax node must be constructed with dedicated constructor");

   m_refCount = 0;

   if (nodeId.has_value()) {
      this->m_nodeId = nodeId.value();
      sm_nextFreeNodeId = std::max(this->m_nodeId + 1, sm_nextFreeNodeId);
   } else {
      this->m_nodeId = sm_nextFreeNodeId++;
   }
   m_bits.common.kind = unsigned(kind);
   m_bits.common.presence = unsigned(presence);
   m_bits.layout.numChildren = layout.size();
   m_bits.layout.textLength = UINT32_MAX;

   this->arena = arena;

   // Initialize layout data.
   std::uninitialized_copy(layout.begin(), layout.end(),
                           getTrailingObjects<RefCountPtr<RawSyntax>>());
}

RawSyntax::RawSyntax(TokenKindType tokenKind, OwnedString text,
                     ArrayRef<TriviaPiece> leadingTrivia,
                     ArrayRef<TriviaPiece> trailingTrivia,
                     SourcePresence presence, const RefCountPtr<SyntaxArena> &arena,
                     std::optional<unsigned> nodeId)
{
   m_refCount = 0;

   if (nodeId.has_value()) {
      this->m_nodeId = nodeId.value();
      sm_nextFreeNodeId = std::max(this->m_nodeId + 1, sm_nextFreeNodeId);
   } else {
      this->m_nodeId = sm_nextFreeNodeId++;
   }
   m_bits.common.kind = unsigned(SyntaxKind::Token);
   m_bits.common.presence = unsigned(presence);
   m_bits.token.tokenKind = unsigned(tokenKind);
   m_bits.token.numLeadingTrivia = leadingTrivia.size();
   m_bits.token.numTrailingTrivia = trailingTrivia.size();

   this->arena = arena;

   // Initialize token text.
   ::new (static_cast<void *>(getTrailingObjects<OwnedString>()))
         OwnedString(text);
   // Initialize leading trivia.
   std::uninitialized_copy(leadingTrivia.begin(), leadingTrivia.end(),
                           getTrailingObjects<TriviaPiece>());
   // Initialize trailing trivia.
   std::uninitialized_copy(trailingTrivia.begin(), trailingTrivia.end(),
                           getTrailingObjects<TriviaPiece>() +
                           m_bits.token.numLeadingTrivia);
}

RawSyntax::RawSyntax(TokenKindType tokenKind, OwnedString text, std::int64_t value, ArrayRef<TriviaPiece> leadingTrivia,
          ArrayRef<TriviaPiece> trailingTrivia, SourcePresence presence,
          const RefCountPtr<SyntaxArena> &arena, std::optional<SyntaxNodeId> nodeId)
{
   RawSyntax(tokenKind, text, leadingTrivia, trailingTrivia, presence, arena, nodeId);
   *getTrailingObjects<std::int64_t>() = value;
}

RawSyntax::RawSyntax(TokenKindType tokenKind, OwnedString text, double value, ArrayRef<TriviaPiece> leadingTrivia,
          ArrayRef<TriviaPiece> trailingTrivia, SourcePresence presence,
          const RefCountPtr<SyntaxArena> &arena, std::optional<SyntaxNodeId> nodeId)
{
   RawSyntax(tokenKind, text, leadingTrivia, trailingTrivia, presence, arena, nodeId);
   *getTrailingObjects<double>() = value;
}

RawSyntax::~RawSyntax()
{
   if (isToken()) {
      getTrailingObjects<OwnedString>()->~OwnedString();
      for (auto &trivia : getLeadingTrivia()) {
         trivia.~TriviaPiece();
      }

      for (auto &trivia : getTrailingTrivia()) {
         trivia.~TriviaPiece();
      }
   } else {
      for (auto &child : getLayout()) {
         child.~RefCountPtr<RawSyntax>();
      }
   }
}

RefCountPtr<RawSyntax> RawSyntax::make(SyntaxKind kind, ArrayRef<RefCountPtr<RawSyntax>> layout,
                                       SourcePresence presence,
                                       const RefCountPtr<SyntaxArena> &arena,
                                       std::optional<unsigned> nodeId)
{
   auto size = totalSizeToAlloc<RefCountPtr<RawSyntax>, OwnedString, std::int64_t, double, TriviaPiece>(
            layout.size(), 0, 0, 0, 0);
   void *data = arena ? arena->allocate(size, alignof(RawSyntax))
                      : ::operator new(size);
   return RefCountPtr<RawSyntax>(
            new (data) RawSyntax(kind, layout, presence, arena, nodeId));
}

RefCountPtr<RawSyntax> RawSyntax::make(TokenKindType tokenKind, OwnedString text,
                                       ArrayRef<TriviaPiece> leadingTrivia,
                                       ArrayRef<TriviaPiece> trailingTrivia,
                                       SourcePresence presence,
                                       const RefCountPtr<SyntaxArena> &arena,
                                       std::optional<unsigned> nodeId)
{
   auto size = totalSizeToAlloc<RefCountPtr<RawSyntax>, OwnedString, std::int64_t, double, TriviaPiece>(
            0, 1, 0, 0, leadingTrivia.size() + trailingTrivia.size());
   void *data = arena ? arena->allocate(size, alignof(RawSyntax))
                      : ::operator new(size);
   return RefCountPtr<RawSyntax>(new (data) RawSyntax(tokenKind, text, leadingTrivia,
                                                      trailingTrivia, presence,
                                                      arena, nodeId));
}

RefCountPtr<RawSyntax> RawSyntax::make(TokenKindType tokenKind, OwnedString text,
                                       std::int64_t value,
                                       ArrayRef<TriviaPiece> leadingTrivia,
                                       ArrayRef<TriviaPiece> trailingTrivia,
                                       SourcePresence presence,
                                       const RefCountPtr<SyntaxArena> &arena,
                                       std::optional<unsigned> nodeId)
{
   auto size = totalSizeToAlloc<RefCountPtr<RawSyntax>, OwnedString, std::int64_t, double, TriviaPiece>(
            0, 1, 1, 0, leadingTrivia.size() + trailingTrivia.size());
   void *data = arena ? arena->allocate(size, alignof(RawSyntax))
                      : ::operator new(size);
   return RefCountPtr<RawSyntax>(new (data) RawSyntax(tokenKind, text, value, leadingTrivia,
                                                      trailingTrivia, presence,
                                                      arena, nodeId));
}


RefCountPtr<RawSyntax> RawSyntax::make(TokenKindType tokenKind, OwnedString text,
                                       double value,
                                       ArrayRef<TriviaPiece> leadingTrivia,
                                       ArrayRef<TriviaPiece> trailingTrivia,
                                       SourcePresence presence,
                                       const RefCountPtr<SyntaxArena> &arena,
                                       std::optional<unsigned> nodeId)
{
   auto size = totalSizeToAlloc<RefCountPtr<RawSyntax>, OwnedString, std::int64_t, double, TriviaPiece>(
            0, 1, 0, 1, leadingTrivia.size() + trailingTrivia.size());
   void *data = arena ? arena->allocate(size, alignof(RawSyntax))
                      : ::operator new(size);
   return RefCountPtr<RawSyntax>(new (data) RawSyntax(tokenKind, text, value, leadingTrivia,
                                                      trailingTrivia, presence,
                                                      arena, nodeId));
}

RefCountPtr<RawSyntax> RawSyntax::append(RefCountPtr<RawSyntax> newLayoutElement) const
{
   auto layout = getLayout();
   std::vector<RefCountPtr<RawSyntax>> newLayout;
   newLayout.reserve(layout.size() + 1);
   std::copy(layout.begin(), layout.end(), std::back_inserter(newLayout));
   newLayout.push_back(newLayoutElement);
   return RawSyntax::make(getKind(), newLayout, SourcePresence::Present);
}

RefCountPtr<RawSyntax> RawSyntax::replaceChild(CursorIndex index,
                                               RefCountPtr<RawSyntax> newLayoutElement) const
{
   auto layout = getLayout();
   std::vector<RefCountPtr<RawSyntax>> newLayout;
   newLayout.reserve(layout.size());

   std::copy(layout.begin(), layout.begin() + index,
             std::back_inserter(newLayout));

   newLayout.push_back(newLayoutElement);

   std::copy(layout.begin() + index + 1, layout.end(),
             std::back_inserter(newLayout));

   return RawSyntax::make(getKind(), newLayout, getPresence());
}

std::optional<AbsolutePosition>
RawSyntax::accumulateAbsolutePosition(AbsolutePosition &pos) const
{
   std::optional<AbsolutePosition> ret;
   if (isToken()) {
      if (isMissing()) {
         return std::nullopt;
      }
      for (auto &leader : getLeadingTrivia()) {
         leader.accumulateAbsolutePosition(pos);
      }
      ret = pos;
      pos.addText(getTokenText());
      for (auto &trailer : getTrailingTrivia()) {
         trailer.accumulateAbsolutePosition(pos);
      }
   } else {
      for (auto &child : getLayout()) {
         if (!child) {
            continue;
         }
         auto result = child->accumulateAbsolutePosition(pos);
         if (!ret && result) {
            ret = result;
         }
      }
   }
   return ret;
}

bool RawSyntax::accumulateLeadingTrivia(AbsolutePosition &pos) const
{
   if (isToken()) {
      if (!isMissing()) {
         for (auto &leader: getLeadingTrivia()) {
            leader.accumulateAbsolutePosition(pos);
         }
         return true;
      }
   } else {
      for (auto &child: getLayout()) {
         if (!child || child->isMissing()) {
            continue;
         }
         if (child->accumulateLeadingTrivia(pos)) {
            return true;
         }
      }
   }
   return false;
}

void RawSyntax::print(raw_ostream &outStream, SyntaxPrintOptions opts) const
{
   if (isMissing()) {
      return;
   }
   if (isToken()) {
      for (const auto &leader : getLeadingTrivia()) {
         leader.print(outStream);
      }
      outStream << getTokenText();
      for (const auto &trailer : getTrailingTrivia()) {
         trailer.print(outStream);
      }
   } else {
      auto kind = getKind();
      const bool printKind = opts.printSyntaxKind && (opts.printTrivialNodeKind ||
                                                      !is_trivial_syntax_kind(kind));
      if (printKind) {
         print_syntax_kind(kind, outStream, opts, true);
      }
      for (const auto &layout : getLayout()) {
         if (layout) {
            layout->print(outStream, opts);
         }
      }
      if (printKind) {
         print_syntax_kind(kind, outStream, opts, false);
      }
   }
}

void RawSyntax::dump() const
{
   dump(llvm::errs(), /*indent*/ 0);
   llvm::errs() << '\n';
}

void RawSyntax::dump(raw_ostream &outStream, unsigned indent) const
{
   auto indentFunc = [&](unsigned Amount) {
      for (decltype(Amount) i = 0; i < Amount; ++i) {
         outStream << ' ';
      }
   };

   indentFunc(indent);
   outStream << '(';
   dump_syntax_kind(outStream, getKind());

   if (isMissing()) {
      outStream << " [missing] ";
   }
   if (isToken()) {
      outStream << " ";
      dump_token_kind(outStream, getTokenKind());
      for (auto &leader : getLeadingTrivia()) {
         outStream << "\n";
         leader.dump(outStream, indent + 1);
      }
      outStream << "\n";
      indentFunc(indent + 1);
      outStream << "(text=\"";
      outStream.write_escaped(getTokenText(), /*UseHexEscapes=*/true);
      outStream << "\")";

      for (auto &trailer : getTrailingTrivia()) {
         outStream << "\n";
         trailer.dump(outStream, indent + 1);
      }
   } else {
      for (auto &child : getLayout()) {
         if (!child) {
            continue;
         }
         outStream << "\n";
         child->dump(outStream, indent + 1);
      }
   }
   outStream << ')';
}

void AbsolutePosition::printLineAndColumn(raw_ostream &outStream) const
{
   outStream << getLine() << ':' << getColumn();
}

void AbsolutePosition::dump(raw_ostream &outStream) const
{
   outStream << "(absolute_position ";
   outStream << "offset=" << getOffset() << " ";
   outStream << "line=" << getLine() << " ";
   outStream << "column=" << getColumn();
   outStream << ')';
}

void RawSyntax::profile(FoldingSetNodeID &id, TokenKindType tokenKind,
                        OwnedString text, ArrayRef<TriviaPiece> leadingTrivia,
                        ArrayRef<TriviaPiece> trailingTrivia)
{
   id.AddInteger(unsigned(tokenKind));
     switch (tokenKind) {
     default:
       id.AddString(text.str());
       break;
     }
   for (auto &piece : leadingTrivia) {
      piece.profile(id);
   }
   for (auto &piece : trailingTrivia) {
      piece.profile(id);
   }
}

} // polar::syntax

namespace polar::utils {
using polar::syntax::AbsolutePosition;
raw_ostream &operator<<(raw_ostream &outStream, AbsolutePosition pos)
{
   pos.printLineAndColumn(outStream);
   return outStream;
}
}
