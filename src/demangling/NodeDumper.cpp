//===--- NodeDumper.cpp - Swift Demangling Debug Dump Functions -----------===//
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

#include "polarphp/demangling/Demangle.h"
#include "polarphp/demangling/Demangler.h"
#include <cstdio>

namespace polar::demangling {

const char *getNodeKindString(Node::Kind k) {
   switch (k) {
#define NODE(ID)                                                               \
  case Node::Kind::ID:                                                         \
    return #ID;

#include "polarphp/demangling/DemangleNodesDef.h"
   }
   return "Demangle::Node::Kind::???";
}

static void printNode(DemanglerPrinter &Out, const Node *node, unsigned depth) {
   // Indent two spaces per depth.
   for (unsigned i = 0; i < depth * 2; ++i) {
      Out << ' ';
   }
   if (!node) {
      Out << "<<NULL>>";
      return;
   }
   Out << "kind=" << getNodeKindString(node->getKind());
   if (node->hasText()) {
      Out << ", text=\"" << node->getText() << '\"';
   }
   if (node->hasIndex()) {
      Out << ", index=" << node->getIndex();
   }
   Out << '\n';
   for (auto &child : *node) {
      printNode(Out, child, depth + 1);
   }
}

std::string getNodeTreeAsString(NodePointer Root) {
   DemanglerPrinter Printer;
   printNode(Printer, Root, 0);
   return std::move(Printer).str();
}

void Node::dump() {
   std::string TreeStr = getNodeTreeAsString(this);
   fputs(TreeStr.c_str(), stderr);
}

void Demangler::dump() {
   for (unsigned Idx = 0; Idx < NodeStack.size(); ++Idx) {
      fprintf(stderr, "NodeStack[%u]:\n", Idx);
      NodeStack[Idx]->dump();
      fprintf(stderr, "\n");
   }
   fprintf(stderr, "Position = %zd:\n%.*s\n%*s\n", Pos,
           (int) Text.size(), Text.data(), (int) Pos + 1, "^");
}

} // polar::demangling
