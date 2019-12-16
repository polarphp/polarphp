//===--- PrefixMap.cpp - m_out-of-line helpers for the PrefixMap structure --===//
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

#include "polarphp/basic/PrefixMap.h"
#include "polarphp/basic/QuotedString.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/Compiler.h"

namespace polar {

namespace {

enum class ChildKind { Left, Right, Further, Root };

// We'd like the dump routine to be present in all builds, but it's
// a pretty large amount of code, most of which is not sensitive to the
// actual key and value data.  If we try to have a common implementation,
// we're left with the problem of describing the layout of a node when
// that's technically instantiation-specific.  Redefining the struct here
// is technically an aliasing violation, but we can just tell the compilers
// that actually use TBAA that this is okay.
struct Node
{
   // If you change the layout in the header, you'll need to change it here.
   // (This comment is repeated there.)
   Node *left;
   Node *right;
   Node *further;
};

class TreePrinter
{
   llvm::raw_ostream &m_out;
   void (&m_printNodeData)(llvm::raw_ostream &out, void *node);
   SmallString<40> m_indent;
public:
   TreePrinter(llvm::raw_ostream &out,
               void (&printNodeData)(llvm::raw_ostream &out, void *node))
      : m_out(out), m_printNodeData(printNodeData)
   {}

   struct IndentScope
   {
      TreePrinter *printer;
      size_t oldLength;
      IndentScope(TreePrinter *printer, StringRef indent)
         : printer(printer),
           oldLength(printer->m_indent.size())
      {
         printer->m_indent += indent;
      }
      ~IndentScope()
      {
         printer->m_indent.resize(oldLength);
      }
   };

   void print(Node *node, ChildKind childKind)
   {
      // The top-level indents here create the line to the node we're
      // trying to print.

      if (node->left) {
         IndentScope ms(this, (childKind == ChildKind::Left ||
                               childKind == ChildKind::Root) ? "  " : "| ");
         print(node->left, ChildKind::Left);
      }

      {
         m_out << m_indent;
         if (childKind == ChildKind::Root) {
            m_out << "+- ";
         } else if (childKind == ChildKind::Left) {
            m_out << "/- ";
         } else if (childKind == ChildKind::Right) {
            m_out << "\\- ";
         } else if (childKind == ChildKind::Further) {
            m_out << "\\-> ";
         }
         m_printNodeData(m_out, node);
         m_out << '\n';
      }

      if (node->further || node->right) {
         IndentScope ms(this, (childKind == ChildKind::Right ||
                               childKind == ChildKind::Further ||
                               childKind == ChildKind::Root) ? "  " : "| ");

         if (node->further) {
            // Further indent, and include the line to the right child if
            // there is one.
            IndentScope is(this, node->right ? "|   " : "    ");
            print(node->further, ChildKind::Further);
         }

         if (node->right) {
            print(node->right, ChildKind::Right);
         }
      }
   }
};

} // end anonymous namespace

void print_opaque_prefix_map(raw_ostream &out, void *root,
                             void (*printNodeData)(raw_ostream &out, void *node))
{
   auto rootNode = reinterpret_cast<Node*>(root);
   if (!rootNode) {
      out << "(empty)\n";
      return;
   }
   TreePrinter(out, *printNodeData).print(rootNode, ChildKind::Root);
}

void PrefixMapKeyPrinter<char>::print(raw_ostream &out, ArrayRef<char> key)
{
   out << QuotedString(StringRef(key.data(), key.size()));
}

void PrefixMapKeyPrinter<unsigned char>::print(raw_ostream &out,
                                               ArrayRef<unsigned char> key)
{
   out << '\'';
   for (auto byte : key) {
      if (byte < 16) {
         out << '0';
      }
      out.write_hex(byte);
   }
   out << '\'';
}

} // polar
