//===--- OutputFileMap.cpp - map of inputs to multiple outputs ------------===//
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
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/12/01.

#include "polarphp/basic/OutputFileMap.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"
#include <system_error>

namespace polar {

llvm::Expected<OutputFileMap>
OutputFileMap::loadFromPath(StringRef path, StringRef workingDirectory)
{
   llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> fileBufOrErr =
         llvm::MemoryBuffer::getFile(path);
   if (!fileBufOrErr) {
      return llvm::errorCodeToError(fileBufOrErr.getError());
   }
   return loadFromBuffer(std::move(fileBufOrErr.get()), workingDirectory);
}

llvm::Expected<OutputFileMap>
OutputFileMap::loadFromBuffer(StringRef data, StringRef workingDirectory)
{
   std::unique_ptr<llvm::MemoryBuffer> buffer{
      llvm::MemoryBuffer::getMemBuffer(data)};
   return loadFromBuffer(std::move(buffer), workingDirectory);
}

llvm::Expected<OutputFileMap>
OutputFileMap::loadFromBuffer(std::unique_ptr<llvm::MemoryBuffer> buffer,
                              StringRef workingDirectory)
{
   return parse(std::move(buffer), workingDirectory);
}

const TypeToPathMap *OutputFileMap::getOutputMapForInput(StringRef input) const
{
   auto iter = m_inputToOutputsMap.find(input);
   if (iter == m_inputToOutputsMap.end()) {
      return nullptr;
   } else {
      return &iter->second;
   }
}

TypeToPathMap &
OutputFileMap::getOrCreateOutputMapForInput(StringRef input)
{
   return m_inputToOutputsMap[input];
}

const TypeToPathMap *OutputFileMap::getOutputMapForSingleOutput() const
{
   return getOutputMapForInput(StringRef());
}

TypeToPathMap &
OutputFileMap::getOrCreateOutputMapForSingleOutput()
{
   return m_inputToOutputsMap[StringRef()];
}

void OutputFileMap::dump(llvm::raw_ostream &ostream, bool sortFlag) const
{
   using TypePathPair = std::pair<filetypes::FileTypeId, std::string>;
   auto printOutputPair = [&ostream](StringRef inputPath,
         const TypePathPair &outputPair) -> void {
      ostream << inputPath << " -> " << filetypes::get_type_name(outputPair.first)
              << ": \"" << outputPair.second << "\"\n";
   };

   if (sortFlag) {
      using PathMapPair = std::pair<StringRef, TypeToPathMap>;
      std::vector<PathMapPair> maps;
      for (auto &inputPair : m_inputToOutputsMap) {
         maps.emplace_back(inputPair.first(), inputPair.second);
      }
      std::sort(maps.begin(), maps.end(), [] (const PathMapPair &lhs,
                const PathMapPair &rhs) -> bool {
         return lhs.first < rhs.first;
      });
      for (auto &inputPair : maps) {
         const TypeToPathMap &map = inputPair.second;
         std::vector<TypePathPair> pairs;
         pairs.insert(pairs.end(), map.begin(), map.end());
         std::sort(pairs.begin(), pairs.end());
         for (auto &outputPair : pairs) {
            printOutputPair(inputPair.first, outputPair);
         }
      }
   } else {
      for (auto &inputPair : m_inputToOutputsMap) {
         const TypeToPathMap &map = inputPair.second;
         for (const TypePathPair &outputPair : map) {
            printOutputPair(inputPair.first(), outputPair);
         }
      }
   }
}

static void writeQuotedEscaped(llvm::raw_ostream &ostream,
                               const StringRef fileName)
{
   ostream << "\"" << llvm::yaml::escape(fileName) << "\"";
}

void OutputFileMap::write(llvm::raw_ostream &ostream,
                          ArrayRef<StringRef> inputs) const
{
   for (const auto &input : inputs) {
      writeQuotedEscaped(ostream, input);
      ostream << ":";
      const TypeToPathMap *outputMap = getOutputMapForInput(input);
      if (!outputMap) {
         // The map doesn't have an entry for this input. (Perhaps there were no
         // outputs and thus the entry was never created.) Put an empty sub-map
         // into the output and move on.
         ostream << " {}\n";
         continue;
      }

      ostream << "\n";
      for (auto &typeAndOutputPath : *outputMap) {
         filetypes::FileTypeId type = typeAndOutputPath.getFirst();
         StringRef output = typeAndOutputPath.getSecond();
         ostream << "  " << filetypes::get_type_name(type) << ": ";
         writeQuotedEscaped(ostream, output);
         ostream << "\n";
      }
   }
}

llvm::Expected<OutputFileMap>
OutputFileMap::parse(std::unique_ptr<llvm::MemoryBuffer> buffer,
                     StringRef workingDirectory)
{
   auto constructError =
         [](const char *errorString) -> llvm::Expected<OutputFileMap> {
      return llvm::make_error<llvm::StringError>(errorString,
                                                 llvm::inconvertibleErrorCode());
   };
   /// FIXME: Make the returned error strings more specific by including some of
   /// the source.
   llvm::SourceMgr sourceMgr;
   llvm::yaml::Stream yamlStream(buffer->getMemBufferRef(), sourceMgr);
   auto iter = yamlStream.begin();
   if (iter == yamlStream.end()) {
      return constructError("empty YAML stream");
   }
   auto root = iter->getRoot();
   if (!root) {
      return constructError("no root");
   }
   OutputFileMap outFileMap;
   auto *map = dyn_cast<llvm::yaml::MappingNode>(root);
   if (!map) {
      return constructError("root was not a MappingNode");
   }
   auto resolvePath =
         [workingDirectory](
         llvm::yaml::ScalarNode *path,
         llvm::SmallVectorImpl<char> &pathStorage) -> StringRef {
      StringRef pathStr = path->getValue(pathStorage);
      if (workingDirectory.empty() || pathStr.empty() || pathStr == "-" ||
          llvm::sys::path::is_absolute(pathStr)) {
         return pathStr;
      }
      // Copy the path to avoid making assumptions about how getValue deals with
      // Storage.
      SmallString<128> PathStrCopy(pathStr);
      pathStorage.clear();
      pathStorage.reserve(PathStrCopy.size() + workingDirectory.size() + 1);
      pathStorage.insert(pathStorage.begin(), workingDirectory.begin(),
                         workingDirectory.end());
      llvm::sys::path::append(pathStorage, PathStrCopy);
      return StringRef(pathStorage.data(), pathStorage.size());
   };

   for (auto &pair : *map) {
      llvm::yaml::Node *key = pair.getKey();
      llvm::yaml::Node *value = pair.getValue();
      if (!key) {
         return constructError("bad key");
      }
      if (!value) {
         return constructError("bad value");
      }
      auto *inputPath = dyn_cast<llvm::yaml::ScalarNode>(key);
      if (!inputPath)
         return constructError("input path not a scalar node");

      llvm::yaml::MappingNode *outputMapNode =
            dyn_cast<llvm::yaml::MappingNode>(value);
      if (!outputMapNode) {
         return constructError("output map not a MappingNode");
      }
      TypeToPathMap outputMap;
      for (auto &outputPair : *outputMapNode) {
         llvm::yaml::Node *key = outputPair.getKey();
         llvm::yaml::Node *value = outputPair.getValue();
         auto *kindNode = dyn_cast<llvm::yaml::ScalarNode>(key);
         if (!kindNode) {
            return constructError("kind not a ScalarNode");
         }
         auto *path = dyn_cast<llvm::yaml::ScalarNode>(value);
         if (!path) {
            return constructError("path not a scalar node");
         }

         llvm::SmallString<16> kindStorage;
         filetypes::FileTypeId kind =
               filetypes::lookup_type_for_name(kindNode->getValue(kindStorage));

         // Ignore unknown types, so that an older swiftc can be used with a newer
         // build system.
         if (kind == filetypes::TY_INVALID) {
            continue;
         }
         llvm::SmallString<128> pathStorage;
         outputMap.insert(std::pair<filetypes::FileTypeId, std::string>(
                             kind, resolvePath(path, pathStorage)));
      }

      llvm::SmallString<128> InputStorage;
      outFileMap.m_inputToOutputsMap[resolvePath(inputPath, InputStorage)] =
            std::move(outputMap);
   }

   if (yamlStream.failed()) {
       return constructError("Output file map parse failed");
   }
   return outFileMap;
}

} // polar
