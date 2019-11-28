//===--- Mangler.h - Base class for Swift name mangling ---------*- C++ -*-===//
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
// Created by polarboy on 2019/11/28.

#ifndef POLARPHP_BASIC_MANGLER_H
#define POLARPHP_BASIC_MANGLER_H

//#include "polarphp/demangling/ManglingUtils.h"
#include "polarphp/basic/LLVM.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/Support/raw_ostream.h"

namespace polar::basic {

void print_mangling_stats();

/// The basic Swift symbol mangler.
///
/// This class serves as an abstract base class for specific manglers. It
/// provides some basic utilities, like handling of substitutions, mangling of
/// identifiers, etc.
class Mangler
{
protected:
   template <typename Mangler>
   friend void mangleIdentifier(Mangler &M, StringRef ident);
   friend class SubstitutionMerging;

   /// The storage for the mangled symbol.
   llvm::SmallString<128> m_storage;

   /// The output stream for the mangled symbol.
   llvm::raw_svector_ostream m_buffer;

   /// A temporary storage needed by the ::mangleIdentifier() template function.
   llvm::SmallVector<WordReplacement, 8> m_substWordsInIdent;

   /// m_substitutions, except identifier substitutions.
   llvm::DenseMap<const void *, unsigned> m_substitutions;

   /// Identifier substitutions.
   llvm::StringMap<unsigned> m_stringSubstitutions;

   /// Word substitutions in mangled identifiers.
   llvm::SmallVector<SubstitutionWord, 26> m_words;

   /// Used for repeated substitutions and known substitutions, e.g. A3B, S2i.
   SubstitutionMerging m_substMerging;

   size_t m_maxNumWords = 26;

   /// If enabled, non-ASCII names are encoded in modified Punycode.
   bool m_usePunycode = true;

   /// If enabled, repeated entities are mangled using substitutions ('A...').
   bool m_useSubstitutions = true;

   /// A helpful little wrapper for an integer value that should be mangled
   /// in a particular, compressed value.
   class Index
   {
      unsigned N;
   public:
      explicit Index(unsigned n) : N(n) {}
      friend llvm::raw_ostream &operator<<(llvm::raw_ostream &out, Index n) {
         if (n.N != 0) out << (n.N - 1);
         return (out << '_');
      }
   };

   void addSubstWordsInIdent(const WordReplacement &repl)
   {
      m_substWordsInIdent.push_back(repl);
   }

   void addWord(const SubstitutionWord &word)
   {
      m_words.push_back(word);
   }

   /// Returns the buffer as a StringRef, needed by mangleIdentifier().
   StringRef getBufferStr() const
   {
      return StringRef(m_storage.data(), m_storage.size());
   }

   /// Removes the last characters of the buffer by setting it's size to a
   /// smaller value.
   void resetBuffer(size_t toPos)
   {
      assert(toPos <= m_storage.size());
      m_storage.resize(toPos);
   }

protected:

   Mangler() : m_buffer(m_storage) { }

   /// Begins a new mangling but does not add the mangling prefix.
   void beginManglingWithoutPrefix();

   /// Begins a new mangling but and adds the mangling prefix.
   void beginMangling();

   /// Finish the mangling of the symbol and return the mangled name.
   std::string finalize();

   /// Finish the mangling of the symbol and write the mangled name into
   /// \p stream.
   void finalize(llvm::raw_ostream &stream);

   /// Verify that demangling and remangling works.
   static void verify(StringRef mangledName);

   void dump();

   /// Appends a mangled identifier string.
   void appendIdentifier(StringRef ident);

   void addSubstitution(const void *ptr) {
      if (m_useSubstitutions)
         m_substitutions[ptr] = m_substitutions.size() + m_stringSubstitutions.size();
   }
   void addSubstitution(StringRef Str) {
      if (m_useSubstitutions)
         m_stringSubstitutions[Str] = m_substitutions.size() + m_stringSubstitutions.size();
   }

   bool tryMangleSubstitution(const void *ptr);

   void mangleSubstitution(unsigned Index);

#ifndef NDEBUG
   void recordOpStatImpl(StringRef op, size_t OldPos);
#endif

   void recordOpStat(StringRef op, size_t OldPos) {
#ifndef NDEBUG
      recordOpStatImpl(op, OldPos);
#endif
   }

   void appendOperator(StringRef op)
   {
      size_t OldPos = m_storage.size();
      m_buffer << op;
      recordOpStat(op, OldPos);
   }

   void appendOperator(StringRef op, Index index)
   {
      size_t OldPos = m_storage.size();
      m_buffer << op << index;
      recordOpStat(op, OldPos);
   }

   void appendOperator(StringRef op, Index index1, Index index2)
   {
      size_t OldPos = m_storage.size();
      m_buffer << op << index1 << index2;
      recordOpStat(op, OldPos);
   }

   void appendOperator(StringRef op, StringRef arg)
   {
      size_t OldPos = m_storage.size();
      m_buffer << op << arg;
      recordOpStat(op, OldPos);
   }

   void appendListSeparator()
   {
      appendOperator("_");
   }

   void appendListSeparator(bool &isFirstListItem)
   {
      if (isFirstListItem) {
         appendListSeparator();
         isFirstListItem = false;
      }
   }

   void appendOperatorParam(StringRef op)
   {
      m_buffer << op;
   }

   void appendOperatorParam(StringRef op, int natural)
   {
      m_buffer << op << natural << '_';
   }

   void appendOperatorParam(StringRef op, Index index)
   {
      m_buffer << op << index;
   }

   void appendOperatorParam(StringRef op, Index index1, Index index2)
   {
      m_buffer << op << index1 << index2;
   }

   void appendOperatorParam(StringRef op, StringRef arg)
   {
      m_buffer << op << arg;
   }
};

} // polar::basic

#endif // POLARPHP_BASIC_MANGLER_H
