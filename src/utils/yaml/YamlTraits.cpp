// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//

#include "polarphp/utils/yaml/YamlTraits.h"
#include "polarphp/basic/adt/StlExtras.h"
#include "polarphp/basic/adt/SmallString.h"
#include "polarphp/basic/adt/StringExtras.h"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/basic/adt/Twine.h"
#include "polarphp/utils/Casting.h"
#include "polarphp/utils/ErrorCode.h"
#include "polarphp/utils/ErrorHandling.h"
#include "polarphp/utils/Format.h"
#include "polarphp/utils/LineIterator.h"
#include "polarphp/utils/MemoryBuffer.h"
#include "polarphp/utils/Unicode.h"
#include "polarphp/utils/yaml/YamlParser.h"
#include "polarphp/utils/RawOutStream.h"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace polar {
namespace yaml {

using polar::utils::SmallString;
using polar::basic::StringLiteral;
using polar::utils::MemoryBuffer;
using polar::utils::LineIterator;
using polar::utils::ErrorCode;
using polar::utils::isa;
using polar::utils::dyn_cast_or_null;
using polar::utils::dyn_cast;
using polar::utils::report_fatal_error;
using polar::utils::format;
using polar::basic::is_contained;
using polar::utils::format_hex_no_prefix;
using polar::basic::get_as_unsigned_integer;
using polar::basic::get_as_signed_integer;
using polar::basic::to_float;

//===----------------------------------------------------------------------===//
//  IO
//===----------------------------------------------------------------------===//

IO::IO(void *context) : m_context(context)
{}

IO::~IO()
{}

void *IO::getContext()
{
   return m_context;
}

void IO::setContext(void *context)
{
   m_context = context;
}

//===----------------------------------------------------------------------===//
//  Input
//===----------------------------------------------------------------------===//

Input::Input(StringRef inputContent, void *context,
             SourceMgr::DiagHandlerTy diagHandler, void *diagHandlerCtxt)
   : IO(context), m_strm(new Stream(inputContent, m_srcMgr, false, &m_errorCode))
{
   if (diagHandler) {
      m_srcMgr.setDiagHandler(diagHandler, diagHandlerCtxt);
   }
   m_docIterator = m_strm->begin();
}

Input::Input(MemoryBufferRef input, void *context,
             SourceMgr::DiagHandlerTy diagHandler, void *diagHandlerCtxt)
   : IO(context), m_strm(new Stream(input, m_srcMgr, false, &m_errorCode))
{
   if (diagHandler) {
      m_srcMgr.setDiagHandler(diagHandler, diagHandlerCtxt);
   }
   m_docIterator = m_strm->begin();
}

Input::~Input()
{}

std::error_code Input::getError()
{
   return m_errorCode;
}

// Pin the vtables to this file.
void Input::HNode::anchor()
{}

void Input::EmptyHNode::anchor()
{}

void Input::ScalarHNode::anchor()
{}

void Input::MapHNode::anchor()
{}

void Input::SequenceHNode::anchor()
{}

bool Input::outputting()
{
   return false;
}

bool Input::setCurrentDocument()
{
   if (m_docIterator != m_strm->end()) {
      Node *node = m_docIterator->getRoot();
      if (!node) {
         assert(m_strm->failed() && "Root is NULL iff parsing failed");
         m_errorCode = make_error_code(ErrorCode::invalid_argument);
         return false;
      }

      if (isa<NullNode>(node)) {
         // Empty files are allowed and ignored
         ++m_docIterator;
         return setCurrentDocument();
      }
      m_topNode = createHNodes(node);
      m_currentNode = m_topNode.get();
      return true;
   }
   return false;
}

bool Input::nextDocument()
{
   return ++m_docIterator != m_strm->end();
}

const Node *Input::getCurrentNode() const
{
   return m_currentNode ? m_currentNode->m_node : nullptr;
}

bool Input::mapTag(StringRef tag, bool defaultValue)
{
   std::string foundTag = m_currentNode->m_node->getVerbatimTag();
   if (foundTag.empty()) {
      // If no tag found and 'tag' is the default, say it was found.
      return defaultValue;
   }
   // Return true iff found tag matches supplied tag.
   return tag.equals(foundTag);
}

void Input::beginMapping()
{
   if (m_errorCode) {
      return;
   }

   // m_currentNode can be null if the document is empty.
   MapHNode *mapNode = dyn_cast_or_null<MapHNode>(m_currentNode);
   if (mapNode) {
      mapNode->m_validKeys.clear();
   }
}

std::vector<StringRef> Input::getKeys()
{
   MapHNode *mapNode = dyn_cast<MapHNode>(m_currentNode);
   std::vector<StringRef> ret;
   if (!mapNode) {
      setError(m_currentNode, "not a mapping");
      return ret;
   }
   for (auto &map : mapNode->m_mapping) {
      ret.push_back(map.getFirst());
   }
   return ret;
}

bool Input::preflightKey(const char *key, bool required, bool, bool &useDefault,
                         void *&saveInfo)
{
   useDefault = false;
   if (m_errorCode) {
      return false;
   }

   // m_currentNode is null for empty documents, which is an error in case required
   // nodes are present.
   if (!m_currentNode) {
      if (required) {
         m_errorCode = make_error_code(ErrorCode::invalid_argument);
      }
      return false;
   }

   MapHNode *mapNode = dyn_cast<MapHNode>(m_currentNode);
   if (!mapNode) {
      if (required || !isa<EmptyHNode>(m_currentNode)) {
         setError(m_currentNode, "not a mapping");
      }
      return false;
   }
   mapNode->m_validKeys.push_back(key);
   HNode *value = mapNode->m_mapping[key].get();
   if (!value) {
      if (required) {
         setError(m_currentNode, Twine("missing required key '") + key + "'");
      } else {
         useDefault = true;
      }
      return false;
   }
   saveInfo = m_currentNode;
   m_currentNode = value;
   return true;
}

void Input::postflightKey(void *saveInfo)
{
   m_currentNode = reinterpret_cast<HNode *>(saveInfo);
}

void Input::endMapping()
{
   if (m_errorCode) {
      return;
   }
   // m_currentNode can be null if the document is empty.
   MapHNode *mapNode = dyn_cast_or_null<MapHNode>(m_currentNode);
   if (!mapNode) {
      return;
   }

   for (const auto &mapItem : mapNode->m_mapping) {
      if (!is_contained(mapNode->m_validKeys, mapItem.getFirst())) {
         setError(mapItem.m_second.get(), Twine("unknown key '") + mapItem.getFirst() + "'");
         break;
      }
   }
}

void Input::beginFlowMapping()
{
   beginMapping();
}

void Input::endFlowMapping()
{
   endMapping();
}

size_t Input::beginSequence()
{
   if (SequenceHNode *sequence = dyn_cast<SequenceHNode>(m_currentNode)) {
      return sequence->m_entries.size();
   }
   if (isa<EmptyHNode>(m_currentNode)) {
      return 0;
   }
   // Treat case where there's a scalar "null" value as an empty sequence.
   if (ScalarHNode *sn = dyn_cast<ScalarHNode>(m_currentNode)) {
      if (is_null(sn->value())) {
         return 0;
      }
   }
   // Any other type of HNode is an error.
   setError(m_currentNode, "not a sequence");
   return 0;
}

void Input::endSequence()
{
}

bool Input::preflightElement(unsigned index, void *&saveInfo)
{
   if (m_errorCode) {
      return false;
   }
   if (SequenceHNode *sequence = dyn_cast<SequenceHNode>(m_currentNode)) {
      saveInfo = m_currentNode;
      m_currentNode = sequence->m_entries[index].get();
      return true;
   }
   return false;
}

void Input::postflightElement(void *saveInfo)
{
   m_currentNode = reinterpret_cast<HNode *>(saveInfo);
}

size_t Input::beginFlowSequence()
{
   return beginSequence();
}

bool Input::preflightFlowElement(unsigned index, void *&saveInfo)
{
   if (m_errorCode) {
      return false;
   }

   if (SequenceHNode *sequence = dyn_cast<SequenceHNode>(m_currentNode)) {
      saveInfo = m_currentNode;
      m_currentNode = sequence->m_entries[index].get();
      return true;
   }
   return false;
}

void Input::postflightFlowElement(void *saveInfo)
{
   m_currentNode = reinterpret_cast<HNode *>(saveInfo);
}

void Input::endFlowSequence() {
}

void Input::beginEnumScalar()
{
   m_scalarMatchFound = false;
}

bool Input::matchEnumScalar(const char *str, bool)
{
   if (m_scalarMatchFound) {
      return false;
   }

   if (ScalarHNode *sn = dyn_cast<ScalarHNode>(m_currentNode)) {
      if (sn->value().equals(str)) {
         m_scalarMatchFound = true;
         return true;
      }
   }
   return false;
}

bool Input::matchEnumFallback()
{
   if (m_scalarMatchFound) {
      return false;
   }
   m_scalarMatchFound = true;
   return true;
}

void Input::endEnumScalar()
{
   if (!m_scalarMatchFound) {
      setError(m_currentNode, "unknown enumerated scalar");
   }
}

bool Input::beginBitSetScalar(bool &doClear)
{
   m_bitValuesUsed.clear();
   if (SequenceHNode *sequence = dyn_cast<SequenceHNode>(m_currentNode)) {
      m_bitValuesUsed.insert(m_bitValuesUsed.begin(), sequence->m_entries.size(), false);
   } else {
      setError(m_currentNode, "expected sequence of bit values");
   }
   doClear = true;
   return true;
}

bool Input::bitSetMatch(const char *str, bool)
{
   if (m_errorCode)
      return false;
   if (SequenceHNode *sequence = dyn_cast<SequenceHNode>(m_currentNode)) {
      unsigned index = 0;
      for (auto &node : sequence->m_entries) {
         if (ScalarHNode *sn = dyn_cast<ScalarHNode>(node.get())) {
            if (sn->value().equals(str)) {
               m_bitValuesUsed[index] = true;
               return true;
            }
         } else {
            setError(m_currentNode, "unexpected scalar in sequence of bit values");
         }
         ++index;
      }
   } else {
      setError(m_currentNode, "expected sequence of bit values");
   }
   return false;
}

void Input::endBitSetScalar()
{
   if (m_errorCode) {
      return;
   }

   if (SequenceHNode *sequence = dyn_cast<SequenceHNode>(m_currentNode)) {
      assert(m_bitValuesUsed.size() == sequence->m_entries.size());
      for (unsigned i = 0; i < sequence->m_entries.size(); ++i) {
         if (!m_bitValuesUsed[i]) {
            setError(sequence->m_entries[i].get(), "unknown bit value");
            return;
         }
      }
   }
}

void Input::scalarString(StringRef &str, QuotingType)
{
   if (ScalarHNode *sn = dyn_cast<ScalarHNode>(m_currentNode)) {
      str = sn->value();
   } else {
      setError(m_currentNode, "unexpected scalar");
   }
}

void Input::blockScalarString(StringRef &str)
{
   scalarString(str, QuotingType::None);
}

void Input::scalarTag(std::string &tag)
{
   tag = m_currentNode->m_node->getVerbatimTag();
}

void Input::setError(HNode *hnode, const Twine &message)
{
   assert(hnode && "HNode must not be NULL");
   setError(hnode->m_node, message);
}

void Input::setError(Node *node, const Twine &message)
{
   m_strm->printError(node, message);
   m_errorCode = make_error_code(ErrorCode::invalid_argument);
}

NodeKind Input::getNodeKind()
{
   if (isa<ScalarHNode>(m_currentNode)) {
      return NodeKind::Scalar;
   } else if (isa<MapHNode>(m_currentNode)) {
      return NodeKind::Map;
   } else if (isa<SequenceHNode>(m_currentNode)) {
      return NodeKind::Sequence;
   }
   polar_unreachable("Unsupported node kind");
}

std::unique_ptr<Input::HNode> Input::createHNodes(Node *node)
{
   SmallString<128> stringStorage;
   if (ScalarNode *sn = dyn_cast<ScalarNode>(node)) {
      StringRef keyStr = sn->getValue(stringStorage);
      if (!stringStorage.empty()) {
         // Copy string to permanent storage
         keyStr = stringStorage.getStr().copy(m_stringAllocator);
      }
      return std::make_unique<ScalarHNode>(node, keyStr);
   } else if (BlockScalarNode *bsn = dyn_cast<BlockScalarNode>(node)) {
      StringRef valueCopy = bsn->getValue().copy(m_stringAllocator);
      return std::make_unique<ScalarHNode>(node, valueCopy);
   } else if (SequenceNode *sequence = dyn_cast<SequenceNode>(node)) {
      auto sequenceHNode = std::make_unique<SequenceHNode>(node);
      for (Node &sn : *sequence) {
         auto Entry = createHNodes(&sn);
         if (m_errorCode) {
            break;
         }
         sequenceHNode->m_entries.push_back(std::move(Entry));
      }
      return std::move(sequenceHNode);
   } else if (MappingNode *Map = dyn_cast<MappingNode>(node)) {
      auto mapHNode = std::make_unique<MapHNode>(node);
      for (KeyValueNode &kvn : *Map) {
         Node *KeyNode = kvn.getKey();
         ScalarNode *key = dyn_cast<ScalarNode>(KeyNode);
         Node *value = kvn.getValue();
         if (!key || !value) {
            if (!key) {
               setError(KeyNode, "Map key must be a scalar");
            }
            if (!value) {
               setError(KeyNode, "Map value must not be empty");
            }
            break;
         }
         stringStorage.clear();
         StringRef keyStr = key->getValue(stringStorage);
         if (!stringStorage.empty()) {
            // Copy string to permanent storage
            keyStr = stringStorage.getStr().copy(m_stringAllocator);
         }
         auto ValueHNode = createHNodes(value);
         if (m_errorCode) {
            break;
         }
         mapHNode->m_mapping[keyStr] = std::move(ValueHNode);
      }
      return std::move(mapHNode);
   } else if (isa<NullNode>(node)) {
      return std::make_unique<EmptyHNode>(node);
   } else {
      setError(node, "unknown node kind");
      return nullptr;
   }
}

void Input::setError(const Twine &Message)
{
   setError(m_currentNode, Message);
}

bool Input::canElideEmptySequence()
{
   return false;
}

//===----------------------------------------------------------------------===//
//  Output
//===----------------------------------------------------------------------===//

Output::Output(RawOutStream &out, void *context, int wrapColumn)
   : IO(context), m_out(out), m_wrapColumn(wrapColumn)
{}

Output::~Output()
{}

bool Output::outputting()
{
   return true;
}

void Output::beginMapping()
{
   m_stateStack.push_back(inMapFirstKey);
   m_needsNewLine = true;
}

bool Output::mapTag(StringRef tag, bool use)
{
   if (use) {
      // If this tag is being written inside a sequence we should write the start
      // of the sequence before writing the tag, otherwise the tag won't be
      // attached to the element in the sequence, but rather the sequence itself.
      bool sequenceElement = false;
      if (m_stateStack.size() > 1) {
         auto &E = m_stateStack[m_stateStack.size() - 2];
         sequenceElement = inSeqAnyElement(E) || inFlowSeqAnyElement(E);
      }
      if (sequenceElement && m_stateStack.back() == inMapFirstKey) {
         newLineCheck();
      } else {
         output(" ");
      }
      output(tag);
      if (sequenceElement) {
         // If we're writing the tag during the first element of a map, the tag
         // takes the place of the first element in the sequence.
         if (m_stateStack.back() == inMapFirstKey) {
            m_stateStack.pop_back();
            m_stateStack.push_back(inMapOtherKey);
         }
         // Tags inside maps in sequences should act as keys in the map from a
         // formatting perspective, so we always want a newline in a sequence.
         m_needsNewLine = true;
      }
   }
   return use;
}

void Output::endMapping()
{
   // If we did not map anything, we should explicitly emit an empty map
   if (m_stateStack.back() == inMapFirstKey) {
      output("{}");
   }
   m_stateStack.pop_back();
}

std::vector<StringRef> Output::getKeys()
{
   report_fatal_error("invalid call");
}

bool Output::preflightKey(const char *key, bool required, bool sameAsDefault,
                          bool &useDefault, void *&) {
   useDefault = false;
   if (required || !sameAsDefault || m_writeDefaultValues) {
      auto state = m_stateStack.getBack();
      if (state == inFlowMapFirstKey || state == inFlowMapOtherKey) {
         flowKey(key);
      } else {
         newLineCheck();
         paddedKey(key);
      }
      return true;
   }
   return false;
}

void Output::postflightKey(void *)
{
   if (m_stateStack.getBack() == inMapFirstKey) {
      m_stateStack.pop_back();
      m_stateStack.push_back(inMapOtherKey);
   } else if (m_stateStack.getBack() == inFlowMapFirstKey) {
      m_stateStack.pop_back();
      m_stateStack.push_back(inFlowMapOtherKey);
   }
}

void Output::beginFlowMapping()
{
   m_stateStack.push_back(inFlowMapFirstKey);
   newLineCheck();
   m_columnAtMapFlowStart = m_column;
   output("{ ");
}

void Output::endFlowMapping()
{
   m_stateStack.pop_back();
   outputUpToEndOfLine(" }");
}

void Output::beginDocuments()
{
   outputUpToEndOfLine("---");
}

bool Output::preflightDocument(size_t index)
{
   if (index > 0) {
      outputUpToEndOfLine("\n---");
   }
   return true;
}

void Output::postflightDocument()
{
}

void Output::endDocuments()
{
   output("\n...\n");
}

size_t Output::beginSequence()
{
   m_stateStack.push_back(inSeqFirstElement);
   m_needsNewLine = true;
   return 0;
}

void Output::endSequence()
{
   // If we did not emit anything, we should explicitly emit an empty sequence
   if (m_stateStack.back() == inSeqFirstElement) {
      output("[]");
   }
   m_stateStack.pop_back();
}

bool Output::preflightElement(unsigned, void *&)
{
   return true;
}

void Output::postflightElement(void *)
{
   if (m_stateStack.back() == inSeqFirstElement) {
      m_stateStack.pop_back();
      m_stateStack.push_back(inSeqOtherElement);
   } else if (m_stateStack.back() == inFlowSeqFirstElement) {
      m_stateStack.pop_back();
      m_stateStack.push_back(inFlowSeqOtherElement);
   }
}

size_t Output::beginFlowSequence()
{
   m_stateStack.push_back(inFlowSeqFirstElement);
   newLineCheck();
   m_columnAtFlowStart = m_column;
   output("[ ");
   m_needFlowSequenceComma = false;
   return 0;
}

void Output::endFlowSequence()
{
   m_stateStack.pop_back();
   outputUpToEndOfLine(" ]");
}

bool Output::preflightFlowElement(unsigned, void *&)
{
   if (m_needFlowSequenceComma) {
      output(", ");
   }

   if (m_wrapColumn && m_column > m_wrapColumn) {
      output("\n");
      for (int i = 0; i < m_columnAtFlowStart; ++i) {
         output(" ");
      }
      m_column = m_columnAtFlowStart;
      output("  ");
   }
   return true;
}

void Output::postflightFlowElement(void *)
{
   m_needFlowSequenceComma = true;
}

void Output::beginEnumScalar()
{
   m_enumerationMatchFound = false;
}

bool Output::matchEnumScalar(const char *str, bool match)
{
   if (match && !m_enumerationMatchFound) {
      newLineCheck();
      outputUpToEndOfLine(str);
      m_enumerationMatchFound = true;
   }
   return false;
}

bool Output::matchEnumFallback()
{
   if (m_enumerationMatchFound) {
      return false;
   }
   m_enumerationMatchFound = true;
   return true;
}

void Output::endEnumScalar()
{
   if (!m_enumerationMatchFound) {
      polar_unreachable("bad runtime enum value");
   }
}

bool Output::beginBitSetScalar(bool &doClear)
{
   newLineCheck();
   output("[ ");
   m_needBitValueComma = false;
   doClear = false;
   return true;
}

bool Output::bitSetMatch(const char *str, bool matches)
{
   if (matches) {
      if (m_needBitValueComma) {
         output(", ");
      }
      output(str);
      m_needBitValueComma = true;
   }
   return false;
}

void Output::endBitSetScalar()
{
   outputUpToEndOfLine(" ]");
}

void Output::scalarString(StringRef &str, QuotingType mustQuote)
{
   newLineCheck();
   if (str.empty()) {
      // Print '' for the empty string because leaving the field empty is not
      // allowed.
      outputUpToEndOfLine("''");
      return;
   }
   if (mustQuote == QuotingType::None) {
      // Only quote if we must.
      outputUpToEndOfLine(str);
      return;
   }

   unsigned i = 0;
   unsigned j = 0;
   size_t endMark = str.getSize();
   const char *base = str.getData();

   const char *const quote = mustQuote == QuotingType::Single ? "'" : "\"";

   output(quote); // Starting quote.

   // When using double-quoted strings (and only in that case), non-printable characters may be
   // present, and will be escaped using a variety of unicode-scalar and special short-form
   // escapes. This is handled in yaml::escape.
   if (mustQuote == QuotingType::Double) {
      output(yaml::escape(base, /* EscapePrintable= */ false));
      outputUpToEndOfLine(quote);
      return;
   }

   // When using single-quoted strings, any single quote ' must be doubled to be escaped.
   while (j < endMark) {
      if (str[j] == '\'') {                    // Escape quotes.
         output(StringRef(&base[i], j - i));  // "flush".
         output(StringLiteral("''"));         // Print it as ''
         i = j + 1;
      }
      ++j;
   }
   output(StringRef(&base[i], j - i));
   outputUpToEndOfLine(quote); // Ending quote.
}

void Output::blockScalarString(StringRef &str)
{
   if (!m_stateStack.empty()) {
      newLineCheck();
   }
   output(" |");
   outputNewLine();
   size_t indent = m_stateStack.empty() ? 1 : m_stateStack.getSize();
   auto buffer = MemoryBuffer::getMemBuffer(str, "", false);
   for (LineIterator lines(*buffer, false); !lines.isAtEnd(); ++lines) {
      for (unsigned index = 0; index < indent; ++index) {
         output("  ");
      }
      output(*lines);
      outputNewLine();
   }
}


void Output::scalarTag(std::string &tag)
{
   if (tag.empty()) {
      return;
   }
   newLineCheck();
   output(tag);
   output(" ");
}

void Output::setError(const Twine &)
{
}

bool Output::canElideEmptySequence()
{
   // Normally, with an optional key/value where the value is an empty sequence,
   // the whole key/value can be not written.  But, that produces wrong yaml
   // if the key/value is the only thing in the map and the map is used in
   // a sequence.  This detects if the this sequence is the first key/value
   // in map that itself is embedded in a sequnce.
   if (m_stateStack.getSize() < 2) {
      return true;
   }
   if (m_stateStack.getBack() != inMapFirstKey) {
      return true;
   }
   return !inSeqAnyElement(m_stateStack[m_stateStack.size() - 2]);
}

void Output::output(StringRef str)
{
   m_column += str.getSize();
   m_out << str;
}

void Output::outputUpToEndOfLine(StringRef str)
{
   output(str);
   if (m_stateStack.empty() || (!inFlowSeqAnyElement(m_stateStack.back()) &&
                                !inFlowMapAnyKey(m_stateStack.back()))){
      m_needsNewLine = true;
   }
}

void Output::outputNewLine()
{
   m_out << "\n";
   m_column = 0;
}

// if seq at top, indent as if map, then add "- "
// if seq in middle, use "- " if firstKey, else use "  "
//

void Output::newLineCheck()
{
   if (!m_needsNewLine) {
      return;
   }
   m_needsNewLine = false;
   outputNewLine();
   if (m_stateStack.size() == 0) {
      return;
   }
   size_t indent = m_stateStack.size() - 1;
   bool outputDash = false;

   if (m_stateStack.back() == inSeqFirstElement ||
       m_stateStack.back() == inSeqOtherElement) {
      outputDash = true;
   } else if ((m_stateStack.size() > 1) &&
              ((m_stateStack.back() == inMapFirstKey) ||
               inFlowSeqAnyElement(m_stateStack.back()) ||
               (m_stateStack.back() == inFlowMapFirstKey)) &&
              inSeqAnyElement(m_stateStack[m_stateStack.size() - 2])) {
      --indent;
      outputDash = true;
   }

   for (unsigned i = 0; i < indent; ++i) {
      output("  ");
   }
   if (outputDash) {
      output("- ");
   }
}

void Output::paddedKey(StringRef key)
{
   output(key);
   output(":");
   const char *spaces = "                ";
   if (key.getSize() < strlen(spaces)) {
      output(&spaces[key.getSize()]);
   } else {
      output(" ");
   }
}

void Output::flowKey(StringRef key)
{
   if (m_stateStack.getBack() == inFlowMapOtherKey) {
      output(", ");
   }

   if (m_wrapColumn && m_column > m_wrapColumn) {
      output("\n");
      for (int index = 0; index < m_columnAtMapFlowStart; ++index) {
         output(" ");
      }
      m_column = m_columnAtMapFlowStart;
      output("  ");
   }
   output(key);
   output(": ");
}


NodeKind Output::getNodeKind()
{
   report_fatal_error("invalid call");
}

bool Output::inSeqAnyElement(InState state)
{
   return state == inSeqFirstElement || state == inSeqOtherElement;
}

bool Output::inFlowSeqAnyElement(InState state)
{
   return state == inFlowSeqFirstElement || state == inFlowSeqOtherElement;
}

bool Output::inMapAnyKey(InState state)
{
   return state == inMapFirstKey || state == inMapOtherKey;
}

bool Output::inFlowMapAnyKey(InState state)
{
   return state == inFlowMapFirstKey || state == inFlowMapOtherKey;
}

//===----------------------------------------------------------------------===//
//  traits for built-in types
//===----------------------------------------------------------------------===//

void ScalarTraits<bool>::output(const bool &value, void *, RawOutStream &out)
{
   out << (value ? "true" : "false");
}

StringRef ScalarTraits<bool>::input(StringRef scalar, void *, bool &value)
{
   if (scalar.equals("true")) {
      value = true;
      return StringRef();
   } else if (scalar.equals("false")) {
      value = false;
      return StringRef();
   }
   return "invalid boolean";
}

void ScalarTraits<StringRef>::output(const StringRef &value, void *,
                                     RawOutStream &out)
{
   out << value;
}

StringRef ScalarTraits<StringRef>::input(StringRef scalar, void *,
                                         StringRef &value)
{
   value = scalar;
   return StringRef();
}

void ScalarTraits<std::string>::output(const std::string &value, void *,
                                       RawOutStream &out)
{
   out << value;
}

StringRef ScalarTraits<std::string>::input(StringRef scalar, void *,
                                           std::string &value)
{
   value = scalar.getStr();
   return StringRef();
}

void ScalarTraits<uint8_t>::output(const uint8_t &value, void *,
                                   RawOutStream &out)
{
   // use temp uin32_t because ostream thinks uint8_t is a character
   uint32_t num = value;
   out << num;
}

StringRef ScalarTraits<uint8_t>::input(StringRef scalar, void *, uint8_t &value)
{
   unsigned long long n;
   if (get_as_unsigned_integer(scalar, 0, n)) {
      return "invalid number";
   }
   if (n > 0xFF) {
      return "out of range number";
   }
   value = static_cast<uint8_t>(n);
   return StringRef();
}

void ScalarTraits<uint16_t>::output(const uint16_t &value, void *,
                                    RawOutStream &out)
{
   out << value;
}

StringRef ScalarTraits<uint16_t>::input(StringRef scalar, void *,
                                        uint16_t &value)
{
   unsigned long long n;
   if (get_as_unsigned_integer(scalar, 0, n)) {
      return "invalid number";
   }
   if (n > 0xFFFF) {
      return "out of range number";
   }
   value = static_cast<uint16_t>(n);
   return StringRef();
}

void ScalarTraits<uint32_t>::output(const uint32_t &value, void *,
                                    RawOutStream &out)
{
   out << value;
}

StringRef ScalarTraits<uint32_t>::input(StringRef scalar, void *,
                                        uint32_t &value) {
   unsigned long long n;
   if (get_as_unsigned_integer(scalar, 0, n)) {
      return "invalid number";
   }
   if (n > 0xFFFFFFFFUL) {
      return "out of range number";
   }
   value = static_cast<uint32_t>(n);
   return StringRef();
}

void ScalarTraits<uint64_t>::output(const uint64_t &value, void *,
                                    RawOutStream &out)
{
   out << value;
}

StringRef ScalarTraits<uint64_t>::input(StringRef scalar, void *,
                                        uint64_t &value)
{
   unsigned long long node;
   if (get_as_unsigned_integer(scalar, 0, node)) {
      return "invalid number";
   }
   value = node;
   return StringRef();
}

void ScalarTraits<int8_t>::output(const int8_t &value, void *, RawOutStream &out)
{
   // use temp in32_t because ostream thinks int8_t is a character
   int32_t num = value;
   out << num;
}

StringRef ScalarTraits<int8_t>::input(StringRef scalar, void *, int8_t &value)
{
   long long node;
   if (get_as_signed_integer(scalar, 0, node)) {
      return "invalid number";
   }
   if ((node > 127) || (node < -128)) {
      return "out of range number";
   }
   value = static_cast<int8_t>(node);
   return StringRef();
}

void ScalarTraits<int16_t>::output(const int16_t &value, void *,
                                   RawOutStream &out)
{
   out << value;
}

StringRef ScalarTraits<int16_t>::input(StringRef scalar, void *, int16_t &value)
{
   long long node;
   if (get_as_signed_integer(scalar, 0, node)) {
      return "invalid number";
   }
   if ((node > INT16_MAX) || (node < INT16_MIN)) {
      return "out of range number";
   }
   value = static_cast<int16_t>(node);
   return StringRef();
}

void ScalarTraits<int32_t>::output(const int32_t &value, void *,
                                   RawOutStream &out)
{
   out << value;
}

StringRef ScalarTraits<int32_t>::input(StringRef scalar, void *, int32_t &value) {
   long long node;
   if (get_as_signed_integer(scalar, 0, node)) {
      return "invalid number";
   }
   if ((node > INT32_MAX) || (node < INT32_MIN)) {
      return "out of range number";
   }
   value = static_cast<int32_t>(node);
   return StringRef();
}

void ScalarTraits<int64_t>::output(const int64_t &value, void *,
                                   RawOutStream &out)
{
   out << value;
}

StringRef ScalarTraits<int64_t>::input(StringRef scalar, void *, int64_t &value)
{
   long long node;
   if (get_as_signed_integer(scalar, 0, node)) {
      return "invalid number";
   }
   value = node;
   return StringRef();
}

void ScalarTraits<double>::output(const double &value, void *, RawOutStream &out)
{
   out << format("%g", value);
}

StringRef ScalarTraits<double>::input(StringRef scalar, void *, double &value)
{
   if (to_float(scalar, value)) {
      return StringRef();
   }
   return "invalid floating point number";
}

void ScalarTraits<float>::output(const float &value, void *, RawOutStream &out)
{
   out << format("%g", value);
}

StringRef ScalarTraits<float>::input(StringRef scalar, void *, float &value)
{
   if (to_float(scalar, value)) {
      return StringRef();
   }
   return "invalid floating point number";
}

void ScalarTraits<Hex8>::output(const Hex8 &value, void *, RawOutStream &out)
{
   uint8_t num = value;
   out << format("0x%02X", num);
}

StringRef ScalarTraits<Hex8>::input(StringRef scalar, void *, Hex8 &value) {
   unsigned long long n;
   if (get_as_unsigned_integer(scalar, 0, n)){
      return "invalid hex8 number";
   }
   if (n > 0xFF) {
      return "out of range hex8 number";
   }
   value = static_cast<Hex8>(n);
   return StringRef();
}

void ScalarTraits<Hex16>::output(const Hex16 &value, void *, RawOutStream &out)
{
   uint16_t num = value;
   out << format("0x%04X", num);
}

StringRef ScalarTraits<Hex16>::input(StringRef scalar, void *, Hex16 &value)
{
   unsigned long long n;
   if (get_as_unsigned_integer(scalar, 0, n)) {
      return "invalid hex16 number";
   }
   if (n > 0xFFFF) {
      return "out of range hex16 number";
   }
   value = static_cast<Hex16>(n);
   return StringRef();
}

void ScalarTraits<Hex32>::output(const Hex32 &value, void *, RawOutStream &out)
{
   uint32_t num = value;
   out << format("0x%08X", num);
}

StringRef ScalarTraits<Hex32>::input(StringRef scalar, void *, Hex32 &value)
{
   unsigned long long n;
   if (get_as_unsigned_integer(scalar, 0, n)) {
      return "invalid hex32 number";
   }
   if (n > 0xFFFFFFFFUL) {
      return "out of range hex32 number";
   }
   value = static_cast<Hex32>(n);
   return StringRef();
}

void ScalarTraits<Hex64>::output(const Hex64 &value, void *, RawOutStream &out)
{
   uint64_t num = value;
   out << format("0x%016llX", num);
}

StringRef ScalarTraits<Hex64>::input(StringRef scalar, void *, Hex64 &value)
{
   unsigned long long num;
   if (get_as_unsigned_integer(scalar, 0, num)) {
      return "invalid hex64 number";
   }
   value = num;
   return StringRef();
}

} // yaml
} // polar
