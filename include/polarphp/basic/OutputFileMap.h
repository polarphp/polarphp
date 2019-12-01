// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/11/26.

#ifndef POLARPHP_BASIC_OUTPUT_FILE_MAP_H
#define POLARPHP_BASIC_OUTPUT_FILE_MAP_H

#include "polarphp/basic/FileTypes.h"
#include "polarphp/basic/LLVM.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/YAMLParser.h"

#include <memory>
#include <string>

namespace polar::basic {

using TypeToPathMap = llvm::DenseMap<filetypes::FileTypeId, std::string>;

/// A two-tiered map used to specify paths for multiple output files associated
/// with each input file in a compilation job.
///
/// The structure is a map from input paths to sub-maps, each of which maps
/// file types to output paths.
class OutputFileMap
{
public:
   OutputFileMap()
   {}

   ~OutputFileMap() = default;

   /// Loads an OutputFileMap from the given \p path into the receiver, if
   /// possible.
   static llvm::Expected<OutputFileMap> loadFromPath(StringRef path,
                                                     StringRef workingDirectory);

   static llvm::Expected<OutputFileMap>
   loadFromBuffer(StringRef data, StringRef workingDirectory);

   /// Loads an OutputFileMap from the given \p buffer, taking ownership
   /// of the buffer in the process.
   ///
   /// When non-empty, \p workingDirectory is used to resolve relative paths in
   /// the output file map.
   static llvm::Expected<OutputFileMap>
   loadFromBuffer(std::unique_ptr<llvm::MemoryBuffer> buffer,
                  StringRef workingDirectory);

   /// Get the map of outputs for the given \p input, if present in the
   /// OutputFileMap. (If not present, returns nullptr.)
   const TypeToPathMap *getOutputMapForInput(StringRef input) const;

   /// Get a map of outputs for the given \p input, creating it in
   /// the OutputFileMap if not already present.
   TypeToPathMap &getOrCreateOutputMapForInput(StringRef input);

   /// Get the map of outputs for a single compile product.
   const TypeToPathMap *getOutputMapForSingleOutput() const;

   /// Get or create the map of outputs for a single compile product.
   TypeToPathMap &getOrCreateOutputMapForSingleOutput();

   /// Dump the OutputFileMap to the given \p os.
   void dump(llvm::raw_ostream &os, bool isSort = false) const;

   /// Write the OutputFileMap for the \p inputs so it can be parsed.
   ///
   /// It is not an error if the map does not contain an entry for a particular
   /// input. Instead, an empty sub-map will be written into the output.
   void write(llvm::raw_ostream &os, ArrayRef<StringRef> inputs) const;

private:
   /// Parses the given \p buffer and returns either an OutputFileMap or
   /// error, taking ownership of \p buffer in the process.
   static llvm::Expected<OutputFileMap>
   parse(std::unique_ptr<llvm::MemoryBuffer> buffer, StringRef workingDirectory);
   llvm::StringMap<TypeToPathMap> m_inputToOutputsMap;
};

} // polar::basic

#endif // POLARPHP_BASIC_OUTPUT_FILE_MAP_H
