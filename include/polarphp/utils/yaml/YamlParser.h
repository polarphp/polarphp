// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/27.

//===----------------------------------------------------------------------===//
//
//  This is a YAML 1.2 parser.
//
//  See http://www.yaml.org/spec/1.2/spec.html for the full standard.
//
//  This currently does not implement the following:
//    * Multi-line literal folding.
//    * Tag resolution.
//    * UTF-16.
//    * BOMs anywhere other than the first Unicode scalar value in the file.
//
//  The most important class here is Stream. This represents a YAML stream with
//  0, 1, or many documents.
//
//  SourceMgr sm;
//  StringRef input = getInput();
//  yaml::Stream stream(input, sm);
//
//  for (yaml::DocumentIterator di = stream.begin(), de = stream.end();
//       di != de; ++di) {
//    yaml::Node *n = di->getRoot();
//    if (n) {
//      // Do something with n...
//    } else
//      break;
//  }
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_UTILS_YAML_PARSER_H
#define POLARPHP_UTILS_YAML_PARSER_H

#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/utils/Allocator.h"
#include "polarphp/utils/SourceLocation.h"
#include <cassert>
#include <cstddef>
#include <iterator>
#include <map>
#include <memory>
#include <string>
#include <system_error>

namespace polar {

// forward declare with namespace
namespace basic {
class Twine;
} // basic
namespace utils {
class MemoryBufferRef;
class SourceMgr;
class RawOutStream;
} // utils

namespace yaml {

class Document;
class DocumentIterator;
class Node;
class Scanner;
struct Token;

using polar::basic::Twine;
using polar::basic::StringRef;
using polar::basic::SmallVectorImpl;
using polar::utils::MemoryBufferRef;
using polar::utils::SourceMgr;
using polar::utils::RawOutStream;
using polar::utils::BumpPtrAllocator;
using polar::utils::SMLocation;
using polar::utils::SMRange;

/// Dump all the tokens in this stream to OS.
/// \returns true if there was an error, false otherwise.
bool dump_tokens(StringRef input, RawOutStream &);

/// Scans all tokens in input without outputting anything. This is used
///        for benchmarking the tokenizer.
/// \returns true if there was an error, false otherwise.
bool scan_tokens(StringRef input);

/// Escape \a input for a double quoted scalar; if \p escapePrintable
/// is true, all UTF8 sequences will be escaped, if \p escapePrintable is
/// false, those UTF8 sequences encoding printable unicode scalars will not be
/// escaped, but emitted verbatim.
std::string escape(StringRef input, bool escapePrintable = true);

/// This class represents a YAML stream potentially containing multiple
///        documents.
class Stream
{
public:
   /// This keeps a reference to the string referenced by \p input.
   Stream(StringRef input, SourceMgr &, bool showColors = true,
          std::error_code *errorCode = nullptr);

   Stream(MemoryBufferRef inputBuffer, SourceMgr &, bool showColors = true,
          std::error_code *errorCode = nullptr);
   ~Stream();

   DocumentIterator begin();
   DocumentIterator end();
   void skip();
   bool failed();

   bool validate()
   {
      skip();
      return !failed();
   }

   void printError(Node *node, const Twine &msg);

private:
   friend class Document;

   std::unique_ptr<Scanner> m_scanner;
   std::unique_ptr<Document> m_currentDoc;
};

/// Abstract base class for all Nodes.
class Node
{
   virtual void getAnchor();

public:
   enum NodeKind
   {
      NK_Null,
      NK_Scalar,
      NK_BlockScalar,
      NK_KeyValue,
      NK_Mapping,
      NK_Sequence,
      NK_Alias
   };

   Node(unsigned int type, std::unique_ptr<Document> &, StringRef anchor,
        StringRef tag);

   // It's not safe to copy YAML nodes; the document is streamed and the position
   // is part of the state.
   Node(const Node &) = delete;
   void operator=(const Node &) = delete;

   void *operator new(size_t size, BumpPtrAllocator &alloc,
                      size_t alignment = 16) noexcept
   {
      return alloc.allocate(size, alignment);
   }

   void operator delete(void *ptr, BumpPtrAllocator &alloc,
                        size_t size) noexcept
   {
      alloc.deallocate(ptr, size);
   }

   void operator delete(void *) noexcept = delete;

   /// Get the value of the anchor attached to this node. If it does not
   ///        have one, getAnchor().size() will be 0.
   StringRef getAnchor() const
   {
      return m_anchor;
   }

   /// Get the tag as it was written in the document. This does not
   ///   perform tag resolution.
   StringRef getRawTag() const
   {
      return m_tag;
   }

   /// Get the verbatium tag for a given Node. This performs tag resoluton
   ///   and substitution.
   std::string getVerbatimTag() const;

   SMRange getSourceRange() const
   {
      return m_sourceRange;
   }

   void setSourceRange(SMRange range)
   {
      m_sourceRange = range;
   }

   // These functions forward to Document and Scanner.
   Token &peekNext();
   Token getNext();
   Node *parseBlockNode();
   BumpPtrAllocator &getAllocator();
   void setError(const Twine &message, Token &location) const;
   bool failed() const;

   virtual void skip() {}

   unsigned int getType() const
   {
      return m_typeId;
   }

protected:
   std::unique_ptr<Document> &m_doc;
   SMRange m_sourceRange;

   ~Node() = default;

private:
   unsigned int m_typeId;
   StringRef m_anchor;
   /// The tag as typed in the document.
   StringRef m_tag;
};

/// A null value.
///
/// Example:
///   !!null null
class NullNode final : public Node
{
   void getAnchor() override;

public:
   NullNode(std::unique_ptr<Document> &doc)
      : Node(NK_Null, doc, StringRef(), StringRef())
   {}

   static bool classof(const Node *node)
   {
      return node->getType() == NK_Null;
   }
};

/// A scalar node is an opaque datum that can be presented as a
///        series of zero or more Unicode scalar values.
///
/// Example:
///   Adena
class ScalarNode final : public Node
{
   void getAnchor() override;

public:
   ScalarNode(std::unique_ptr<Document> &doc, StringRef anchor, StringRef tag,
              StringRef value)
      : Node(NK_Scalar, doc, anchor, tag), m_value(value)
   {
      SMLocation start = SMLocation::getFromPointer(value.begin());
      SMLocation end = SMLocation::getFromPointer(value.end());
      m_sourceRange = SMRange(start, end);
   }

   // Return Value without any escaping or folding or other fun YAML stuff. This
   // is the exact bytes that are contained in the file (after conversion to
   // utf8).
   StringRef getRawValue() const
   {
      return m_value;
   }

   /// Gets the value of this node as a StringRef.
   ///
   /// \param Storage is used to store the content of the returned StringRef iff
   ///        it requires any modification from how it appeared in the source.
   ///        This happens with escaped characters and multi-line literals.
   StringRef getValue(SmallVectorImpl<char> &Storage) const;

   static bool classof(const Node *N) {
      return N->getType() == NK_Scalar;
   }

private:
   StringRef m_value;

   StringRef unescapeDoubleQuoted(StringRef unquotedValue,
                                  StringRef::size_type start,
                                  SmallVectorImpl<char> &storage) const;
};

/// A block scalar node is an opaque datum that can be presented as a
///        series of zero or more Unicode scalar values.
///
/// Example:
///   |
///     Hello
///     World
class BlockScalarNode final : public Node
{
   void getAnchor() override;

public:
   BlockScalarNode(std::unique_ptr<Document> &doc, StringRef anchor, StringRef tag,
                   StringRef value, StringRef rawVal)
      : Node(NK_BlockScalar, doc, anchor, tag), m_value(value)
   {
      SMLocation start = SMLocation::getFromPointer(rawVal.begin());
      SMLocation end = SMLocation::getFromPointer(rawVal.end());
      m_sourceRange = SMRange(start, end);
   }

   /// Gets the value of this node as a StringRef.
   StringRef getValue() const
   {
      return m_value;
   }

   static bool classof(const Node *node)
   {
      return node->getType() == NK_BlockScalar;
   }

private:
   StringRef m_value;
};

/// A key and value pair. While not technically a Node under the YAML
///        representation graph, it is easier to treat them this way.
///
/// TODO: Consider making this not a child of Node.
///
/// Example:
///   Section: .text
class KeyValueNode final : public Node
{
   void getAnchor() override;

public:
   KeyValueNode(std::unique_ptr<Document> &doc)
      : Node(NK_KeyValue, doc, StringRef(), StringRef())
   {}

   /// Parse and return the key.
   ///
   /// This may be called multiple times.
   ///
   /// \returns The key, or nullptr if failed() == true.
   Node *getKey();

   /// Parse and return the value.
   ///
   /// This may be called multiple times.
   ///
   /// \returns The value, or nullptr if failed() == true.
   Node *getValue();

   void skip() override
   {
      if (Node *key = getKey()) {
         key->skip();
         if (Node *value = getValue()) {
            value->skip();
         }
      }
   }

   static bool classof(const Node *node)
   {
      return node->getType() == NK_KeyValue;
   }

private:
   Node *m_key = nullptr;
   Node *m_value = nullptr;
};

/// This is an iterator abstraction over YAML collections shared by both
///        sequences and maps.
///
/// BaseT must have a ValueT* member named m_currentEntry and a member function
/// increment() which must set m_currentEntry to 0 to create an end iterator.
template <class BaseT, class ValueT>
class BasicCollectionIterator
      : public std::iterator<std::input_iterator_tag, ValueT>
{
public:
   BasicCollectionIterator() = default;
   BasicCollectionIterator(BaseT *base) : m_base(base)
   {}

   ValueT *operator->() const
   {
      assert(m_base && m_base->m_currentEntry && "Attempted to access end iterator!");
      return m_base->m_currentEntry;
   }

   ValueT &operator*() const
   {
      assert(m_base && m_base->m_currentEntry &&
             "Attempted to dereference end iterator!");
      return *m_base->m_currentEntry;
   }

   operator ValueT *() const
   {
      assert(m_base && m_base->m_currentEntry && "Attempted to access end iterator!");
      return m_base->m_currentEntry;
   }

   /// Note on EqualityComparable:
   ///
   /// The iterator is not re-entrant,
   /// it is meant to be used for parsing YAML on-demand
   /// Once iteration started - it can point only to one entry at a time
   /// hence m_base.m_currentEntry and other.m_base.m_currentEntry are equal
   /// iff m_base and other.m_base are equal.
   bool operator==(const BasicCollectionIterator &other) const
   {
      if (m_base && (m_base == other.m_base)) {
         assert((m_base->m_currentEntry == other.m_base->m_currentEntry)
                && "Equal Bases expected to point to equal Entries");
      }

      return m_base == other.m_base;
   }

   bool operator!=(const BasicCollectionIterator &other) const
   {
      return !(m_base == other.m_base);
   }

   BasicCollectionIterator &operator++()
   {
      assert(m_base && "Attempted to advance iterator past end!");
      m_base->increment();
      // Create an end iterator.
      if (!m_base->m_currentEntry) {
         m_base = nullptr;
      }
      return *this;
   }

private:
   BaseT *m_base = nullptr;
};

// The following two templates are used for both MappingNode and Sequence Node.
template <class CollectionType>
typename CollectionType::iterator begin(CollectionType &collection)
{
   assert(collection.m_isAtBeginning && "You may only iterate over a collection once!");
   collection.m_isAtBeginning = false;
   typename CollectionType::iterator ret(&collection);
   ++ret;
   return ret;
}

template <class CollectionType> void skip(CollectionType &collection)
{
   // TODO: support skipping from the middle of a parsed collection ;/
   assert((collection.m_isAtBeginning || collection.m_isAtEnd) && "Cannot skip mid parse!");
   if (collection.m_isAtBeginning)
      for (typename CollectionType::iterator i = begin(collection), e = collection.end(); i != e;
           ++i) {
         i->skip();
      }
}

/// Represents a YAML map created from either a block map for a flow map.
///
/// This parses the YAML stream as increment() is called.
///
/// Example:
///   Name: _main
///   Scope: Global
class MappingNode final : public Node
{
   void getAnchor() override;

public:
   enum MappingType
   {
      MT_Block,
      MT_Flow,
      MT_Inline ///< An inline mapping node is used for "[key: value]".
   };

   MappingNode(std::unique_ptr<Document> &doc, StringRef anchor, StringRef tag,
               MappingType mapType)
      : Node(NK_Mapping, doc, anchor, tag), m_type(mapType)
   {}

   friend class BasicCollectionIterator<MappingNode, KeyValueNode>;

   using iterator = BasicCollectionIterator<MappingNode, KeyValueNode>;

   template <class T> friend typename T::iterator yaml::begin(T &);
   template <class T> friend void yaml::skip(T &);

   iterator begin()
   {
      return yaml::begin(*this);
   }

   iterator end()
   {
      return iterator();
   }

   void skip() override
   {
      yaml::skip(*this);
   }

   static bool classof(const Node *node)
   {
      return node->getType() == NK_Mapping;
   }

private:
   MappingType m_type;
   bool m_isAtBeginning = true;
   bool m_isAtEnd = false;
   KeyValueNode *m_currentEntry = nullptr;

   void increment();
};

/// Represents a YAML sequence created from either a block sequence for a
///        flow sequence.
///
/// This parses the YAML stream as increment() is called.
///
/// Example:
///   - Hello
///   - World
class SequenceNode final : public Node
{
   void getAnchor() override;

public:
   enum SequenceType
   {
      ST_Block,
      ST_Flow,
      // Use for:
      //
      // key:
      // - val1
      // - val2
      //
      // As a BlockMappingEntry and BlockEnd are not created in this case.
      ST_Indentless
   };

   SequenceNode(std::unique_ptr<Document> &doc, StringRef anchor, StringRef tag,
                SequenceType sourceType)
      : Node(NK_Sequence, doc, anchor, tag), m_seqType(sourceType) {}

   friend class BasicCollectionIterator<SequenceNode, Node>;

   using iterator = BasicCollectionIterator<SequenceNode, Node>;

   template <class T> friend typename T::iterator yaml::begin(T &);
   template <class T> friend void yaml::skip(T &);

   void increment();

   iterator begin()
   {
      return yaml::begin(*this);
   }

   iterator end()
   {
      return iterator();
   }

   void skip() override
   {
      yaml::skip(*this);
   }

   static bool classof(const Node *node)
   {
      return node->getType() == NK_Sequence;
   }

private:
   SequenceType m_seqType;
   bool m_isAtBeginning = true;
   bool m_isAtEnd = false;
   bool m_wasPreviousTokenFlowEntry = true; // Start with an imaginary ','.
   Node *m_currentEntry = nullptr;
};

/// Represents an alias to a Node with an anchor.
///
/// Example:
///   *AnchorName
class AliasNode final : public Node
{
   void getAnchor() override;

public:
   AliasNode(std::unique_ptr<Document> &doc, StringRef value)
      : Node(NK_Alias, doc, StringRef(), StringRef()), m_name(value)
   {}

   StringRef getName() const
   {
      return m_name;
   }

   Node *getTarget();

   static bool classof(const Node *node)
   {
      return node->getType() == NK_Alias;
   }

private:
   StringRef m_name;
};

/// A YAML Stream is a sequence of Documents. A document contains a root
///        node.
class Document
{
public:
   Document(Stream &parentStream);

   /// m_root for parsing a node. Returns a single node.
   Node *parseBlockNode();

   /// Finish parsing the current document and return true if there are
   ///        more. Return false otherwise.
   bool skip();

   /// Parse and return the root level node.
   Node *getRoot()
   {
      if (m_root) {
         return m_root;
      }
      return m_root = parseBlockNode();
   }

   const std::map<StringRef, StringRef> &getTagMap() const
   {
      return m_tagMap;
   }

private:
   friend class Node;
   friend class DocumentIterator;

   /// Stream to read tokens from.
   Stream &m_stream;

   /// Used to allocate nodes to. All are destroyed without calling their
   ///        destructor when the document is destroyed.
   BumpPtrAllocator m_nodeAllocator;

   /// The root node. Used to support skipping a partially parsed
   ///        document.
   Node *m_root;

   /// Maps tag prefixes to their expansion.
   std::map<StringRef, StringRef> m_tagMap;

   Token &peekNext();
   Token getNext();
   void setError(const Twine &message, Token &location) const;
   bool failed() const;

   /// Parse %BLAH directives and return true if any were encountered.
   bool parseDirectives();

   /// Parse %YAML
   void parseYAMLDirective();

   /// Parse %TAG
   void parseTAGDirective();

   /// Consume the next token and error if it is not \a TK.
   bool expectToken(int token);
};

/// Iterator abstraction for Documents over a Stream.
class DocumentIterator
{
public:
   DocumentIterator() = default;
   DocumentIterator(std::unique_ptr<Document> &doc) : m_doc(&doc)
   {}

   bool operator==(const DocumentIterator &other) const
   {
      if (isAtEnd() || other.isAtEnd()) {
         return isAtEnd() && other.isAtEnd();
      }
      return m_doc == other.m_doc;
   }

   bool operator!=(const DocumentIterator &other) const
   {
      return !(*this == other);
   }

   DocumentIterator operator++()
   {
      assert(m_doc && "incrementing iterator past the end.");
      if (!(*m_doc)->skip()) {
         m_doc->reset(nullptr);
      } else {
         Stream &S = (*m_doc)->m_stream;
         m_doc->reset(new Document(S));
      }
      return *this;
   }

   Document &operator*()
   {
      return *m_doc->get();
   }

   std::unique_ptr<Document> &operator->()
   {
      return *m_doc;
   }

private:
   bool isAtEnd() const
   {
      return !m_doc || !*m_doc;
   }

   std::unique_ptr<Document> *m_doc = nullptr;
};

} // yaml
} // polar

#endif // POLARPHP_UTILS_YAML_PARSER_H
