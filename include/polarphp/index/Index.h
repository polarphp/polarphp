//===--- Index.h - Swift Indexing -------------------------------*- C++ -*-===//
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

#ifndef POLARPHP_INDEX_INDEX_H
#define POLARPHP_INDEX_INDEX_H

#include "polarphp/index/IndexDataConsumer.h"

namespace polar {

class ModuleDecl;
class SourceFile;
class DeclContext;

namespace index {

void indexDeclContext(DeclContext *DC, IndexDataConsumer &consumer);
void indexSourceFile(SourceFile *SF, IndexDataConsumer &consumer);
void indexModule(ModuleDecl *module, IndexDataConsumer &consumer);

} // end namespace index
} // end namespace polar

#endif // POLARPHP_INDEX_INDEX_H
