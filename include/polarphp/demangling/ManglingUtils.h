//===--- ManglingUtils.h - Utilities for Swift name mangling ----*- C++ -*-===//
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
// Created by polarboy on 2019/12/04.

#ifndef POLARPHP_DEMANGLING_MANGLINGUTILS_H
#define POLARPHP_DEMANGLING_MANGLINGUTILS_H

#include "llvm/ADT/StringRef.h"
#include "polarphp/demangling/Punycode.h"

namespace polar::demangling {

using llvm::StringRef;

inline bool is_lower_letter(char ch)
{
   return ch >= 'a' && ch <= 'z';
}

inline bool is_upper_letter(char ch)
{
   return ch >= 'A' && ch <= 'Z';
}

inline bool isDigit(char ch)
{
   return ch >= '0' && ch <= '9';
}

inline bool isLetter(char ch)
{
   return is_lower_letter(ch) || is_upper_letter(ch);
}

/// Returns true if \p ch is a character which defines the begin of a
/// substitution word.
inline bool is_word_start(char ch)
{
   return !isDigit(ch) && ch != '_' && ch != 0;
}

/// Returns true if \p ch is a character (following \p prevCh) which defines
/// the end of a substitution word.
inline bool is_word_end(char ch, char prevCh)
{
   if (ch == '_' || ch == 0) {
      return true;
   }
   if (!is_upper_letter(prevCh) && is_upper_letter(ch)) {
      return true;
   }
   return false;
}

/// Returns true if \p ch is a valid character which may appear in a symbol
/// mangling.
inline bool is_valid_symbol_char(char ch)
{
   return isLetter(ch) || isDigit(ch) || ch == '_' || ch == '$';
}

/// Returns true if \p str contains any character which may not appear in a
/// mangled symbol string and therefore must be punycode encoded.
bool needs_punycode_encoding(StringRef str);

/// Returns true if \p str contains any non-ASCII character.
bool is_non_ascii(StringRef str);

/// Describes a word in a mangled identifier.
struct SubstitutionWord
{

   /// The position of the first word character in the mangled string.
   size_t start;

   /// The length of the word.
   size_t length;
};

/// Helper struct which represents a word substitution.
struct WordReplacement
{
   /// The position in the identifier where the word is substituted.
   size_t stringPos;

   /// The index into the mangler's words array (-1 if invalid).
   int wordIdx;
};

/// Translate the given operator character into its mangled form.
///
/// Current operator characters:   @/=-+*%<>!&|^~ and the special operator '..'
char translate_operator_char(char op);

/// Returns a string where all characters of the operator \p Op are translated
/// to their mangled form.
std::string translate_operator(StringRef op);

/// Returns the standard type kind for an 'S' substitution, e.g. 'i' for "Int".
char get_standard_type_subst(StringRef type);

/// Mangles an identifier using a generic Mangler class.
///
/// The Mangler class must provide the following:
/// *) words: An array of SubstitutionWord which holds the current list of
///           found words which can be used for substitutions.
/// *) substWordsInIdent: An array of WordReplacement, which is just used
///           as a temporary storage during mangling. Must be empty.
/// *) buffer: A stream where the mangled identifier is written to.
/// *) getBufferStr(): Returns a StringRef of the current content of buffer.
/// *) UsePunycode: A flag indicating if punycode encoding should be done.
template <typename Mangler>
void mangle_identifier(Mangler &mangler, StringRef ident)
{

   size_t wordsInBuffer = mangler.words.size();
   assert(mangler.substWordsInIdent.empty());
   if (mangler.usePunycode && needs_punycode_encoding(ident)) {
      // If the identifier contains non-ASCII character, we mangle
      // with an initial '00' and Punycode the identifier string.
      std::string punycodeBuf;
      encode_punycode_utf8(ident, punycodeBuf,
                                   /*mapNonSymbolChars*/ true);
      StringRef pcIdent = punycodeBuf;
      mangler.buffer << "00" << pcIdent.size();
      if (isDigit(pcIdent[0]) || pcIdent[0] == '_') {
         mangler.buffer << '_';
      }
      mangler.buffer << pcIdent;
      return;
   }
   // Search for word substitutions and for new words.
   const size_t NotInsideWord = ~0;
   size_t wordStartPos = NotInsideWord;
   for (size_t pos = 0, Len = ident.size(); pos <= Len; ++pos) {
      char ch = (pos < Len ? ident[pos] : 0);
      if (wordStartPos != NotInsideWord && is_word_end(ch, ident[pos - 1])) {
         // This position is the end of a word, i.e. the next character after a
         // word.
         assert(pos > wordStartPos);
         size_t wordLen = pos - wordStartPos;
         StringRef word = ident.substr(wordStartPos, wordLen);
         // Helper function to lookup the word in a string.
         auto lookupWord = [&] (StringRef Str,
               size_t fromWordIdx, size_t ToWordIdx) -> int {
            for (size_t idx = fromWordIdx; idx < ToWordIdx; ++idx) {
               const SubstitutionWord &w = mangler.words[idx];
               StringRef existingWord =  Str.substr(w.start, w.length);
               if (word == existingWord) {
                  return static_cast<int>(idx);
               }
            }
            return -1;
         };

         // Is the word already present in the so far mangled string?
         int wordIdx = lookupWord(mangler.getBufferStr(), 0, wordsInBuffer);
         // Otherwise, is the word already present in this identifier?
         if (wordIdx < 0) {
            wordIdx = lookupWord(ident, wordsInBuffer, mangler.words.size());
         }
         if (wordIdx >= 0) {
            // We found a word substitution!
            assert(wordIdx < 26);
            mangler.addSubstWordsInIdent({wordStartPos, wordIdx});
         } else if (wordLen >= 2 && mangler.words.size() < mangler.MaxNumWords) {
            // It's a new word: remember it.
            // Note: at this time the word's start position is relative to the
            // begin of the identifier. We must update it afterwards so that it is
            // relative to the begin of the whole mangled buffer.
            mangler.addWord({wordStartPos, wordLen});
         }
         wordStartPos = NotInsideWord;
      }
      if (wordStartPos == NotInsideWord && is_word_start(ch)) {
         // This position is the begin of a word.
         wordStartPos = pos;
      }
   }
   // If we have word substitutions mangle an initial '0'.
   if (!mangler.substWordsInIdent.empty()) {
      mangler.buffer << '0';
   }
   size_t pos = 0;
   // Add a dummy-word at the end of the list.
   mangler.addSubstWordsInIdent({ident.size(), -1});
   // Mangle a sequence of word substitutions and sub-strings.
   for (size_t idx = 0, End = mangler.substWordsInIdent.size(); idx < End; ++idx) {
      const WordReplacement &repl = mangler.substWordsInIdent[idx];
      if (pos < repl.stringPos) {
         // Mangle the sub-string up to the next word substitution (or to the end
         // of the identifier - that's why we added the dummy-word).
         // The first thing: we add the encoded sub-string length.
         mangler.buffer << (repl.stringPos - pos);
         assert(!isDigit(ident[pos]) &&
                "first char of sub-string may not be a digit");
         do {
            // Update the start position of new added words, so that they refer to
            // the begin of the whole mangled buffer.
            if (wordsInBuffer < mangler.words.size() &&
                mangler.words[wordsInBuffer].start == pos) {
               mangler.words[wordsInBuffer].start = mangler.getBufferStr().size();
               wordsInBuffer++;
            }
            // Add a literal character of the sub-string.
            mangler.buffer << ident[pos];
            pos++;
         } while (pos < repl.stringPos);
      }
      // Is it a "real" word substitution (and not the dummy-word)?
      if (repl.wordIdx >= 0) {
         assert(repl.wordIdx <= static_cast<int>(wordsInBuffer));
         pos += mangler.words[repl.wordIdx].length;
         if (idx < End - 2) {
            mangler.buffer << static_cast<char>((repl.wordIdx + 'a'));
         } else {
            // The last word substitution is a capital letter.
            mangler.buffer << static_cast<char>((repl.wordIdx + 'A'));
            if (pos == ident.size()) {
                mangler.buffer << '0';
            }
         }
      }
   }
   mangler.substWordsInIdent.clear();
}

/// Utility class for mangling merged substitutions.
///
/// Used in the Mangler and Remangler.
class SubstitutionMerging
{

   /// The position of the last substitution mangling,
   /// e.g. 3 for 'AabC' and 'Aab4C'
   size_t m_lastSubstPosition = 0;

   /// The size of the last substitution mangling,
   /// e.g. 1 for 'AabC' or 2 for 'Aab4C'
   size_t m_lastSubstSize = 0;

   /// The repeat count of the last substitution,
   /// e.g. 1 for 'AabC' or 4 for 'Aab4C'
   size_t m_lastNumSubsts = 0;

   /// True if the last substitution is an 'S' substitution,
   /// false if the last substitution is an 'A' substitution.
   bool m_lastSubstIsStandardSubst = false;

public:

   // The only reason to limit the number of repeated substitutions is that we
   // don't want that the demangler blows up on a bogus substitution, e.g.
   // ...A832456823746582B...
   enum { MaxRepeatCount = 2048 };

   void clear()
   {
      m_lastNumSubsts = 0;
   }

   /// Tries to merge the substitution \p subst with a previously mangled
   /// substitution.
   ///
   /// Returns true on success. In case of false, the caller must mangle the
   /// substitution separately in the form 'S<subst>' or 'A<subst>'.
   ///
   /// The Mangler class must provide the following:
   /// *) buffer: A stream where the mangled identifier is written to.
   /// *) getBufferStr(): Returns a StringRef of the current content of buffer.
   /// *) resetBuffer(size_t): Resets the buffer to an old position.
   template <typename Mangler>
   bool tryMergeSubst(Mangler &mangler, char subst, bool isStandardSubst)
   {
      assert(is_upper_letter(subst) || (isStandardSubst && is_lower_letter(subst)));
      StringRef bufferStr = mangler.getBufferStr();
      if (m_lastNumSubsts > 0 && m_lastNumSubsts < MaxRepeatCount
          && bufferStr.size() == m_lastSubstPosition + m_lastSubstSize
          && m_lastSubstIsStandardSubst == isStandardSubst) {

         // The last mangled thing is a substitution.
         assert(m_lastSubstPosition > 0 && m_lastSubstPosition < bufferStr.size());
         assert(m_lastSubstSize > 0);
         char lastSubst = bufferStr.back();
         assert(is_upper_letter(lastSubst)
                || (isStandardSubst && is_lower_letter(lastSubst)));
         if (lastSubst != subst && !isStandardSubst) {
            // We can merge with a different 'A' substitution,
            // e.g. 'AB' -> 'AbC'.
            m_lastSubstPosition = bufferStr.size();
            m_lastNumSubsts = 1;
            mangler.resetBuffer(bufferStr.size() - 1);
            assert(is_upper_letter(lastSubst));
            mangler.buffer << static_cast<char>((lastSubst - 'A' + 'a')) << subst;
            m_lastSubstSize = 1;
            return true;
         }
         if (lastSubst == subst) {
            // We can merge with the same 'A' or 'S' substitution,
            // e.g. 'AB' -> 'A2B', or 'S3i' -> 'S4i'
            m_lastNumSubsts++;
            mangler.resetBuffer(m_lastSubstPosition);
            mangler.buffer << m_lastNumSubsts;
            mangler.buffer << subst;
            m_lastSubstSize = mangler.getBufferStr().size() - m_lastSubstPosition;
            return true;
         }
      }
      // We can't merge with the previous substitution, but let's remember this
      // substitution which will be mangled by the caller.
      m_lastSubstPosition = bufferStr.size() + 1;
      m_lastSubstSize = 1;
      m_lastNumSubsts = 1;
      m_lastSubstIsStandardSubst = isStandardSubst;
      return false;
   }
};

} // polar::demangling

#endif // POLARPHP_DEMANGLING_MANGLINGUTILS_H

