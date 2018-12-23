// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/05.

//===----------------------------------------------------------------------===//
//
// This is a utility class for instrumentation passes (like AddressSanitizer
// or ThreadSanitizer) to avoid instrumenting some functions or global
// variables, or to instrument some functions or global variables in a specific
// way, based on a user-supplied list.
//
//===----------------------------------------------------------------------===//

#include "polarphp/utils/SpecialCaseList.h"
#include "polarphp/basic/adt/SmallVector.h"
#include "polarphp/basic/adt/StringExtras.h"
#include "polarphp/utils/MemoryBuffer.h"
#include <string>
#include <system_error>
#include <utility>
#include <stdio.h>

namespace polar {
namespace utils {

namespace {

// These are the special characters matched in functions like "p_ere_exp".
static const char sg_regexMetachars[] = "()^$|*+?.[]\\{}";

bool is_literal_regex(const StringRef str)
{
   // Check for regex metacharacters.  This list was derived from our regex
   // implementation in regcomp.c and double checked against the POSIX extended
   // regular expression specification.
   return str.findFirstOf(sg_regexMetachars) == StringRef::npos;
}
} // anonymous namespace

bool SpecialCaseList::Matcher::insert(std::string regexp,
                                      unsigned lineNumber,
                                      std::string &reError)
{
   if (regexp.empty()) {
      reError = "Supplied regexp was blank";
      return false;
   }
   if (is_literal_regex(regexp)) {
      m_strings[regexp] = lineNumber;
      return true;
   }
   m_trigrams.insert(regexp);

   // Replace * with .*
   for (size_t pos = 0; (pos = regexp.find('*', pos)) != std::string::npos;
        pos += strlen(".*")) {
      regexp.replace(pos, strlen("*"), ".*");
   }

   regexp = (Twine("^(") + StringRef(regexp) + ")$").getStr();

   try {
      // Check that the regexp is valid.
      std::regex checkRE(regexp);
      m_regExes.emplace_back(
               std::make_pair(std::make_unique<std::regex>(std::move(checkRE)), lineNumber));
      return true;
   } catch (const std::regex_error& e) {
      reError = e.what();
      return false;
   }
}

unsigned SpecialCaseList::Matcher::match(StringRef query) const
{
   auto iter = m_strings.find(query);
   if (iter != m_strings.end()) {
      return iter->m_second;
   }
   if (m_trigrams.isDefinitelyOut(query)) {
      return false;
   }
   for (auto& regExKV : m_regExes) {
      if (std::regex_match(query.getStr(), *regExKV.first)) {
         return regExKV.second;
      }
   }
   return 0;
}

std::unique_ptr<SpecialCaseList>
SpecialCaseList::create(const std::vector<std::string> &paths,
                        std::string &error)
{
   std::unique_ptr<SpecialCaseList> scl(new SpecialCaseList());
   if (scl->createInternal(paths, error)) {
      return scl;
   }
   return nullptr;
}

std::unique_ptr<SpecialCaseList> SpecialCaseList::create(const MemoryBuffer *memoryBuffer,
                                                         std::string &error)
{
   std::unique_ptr<SpecialCaseList> scl(new SpecialCaseList());
   if (scl->createInternal(memoryBuffer, error)) {
      return scl;
   }
   return nullptr;
}

std::unique_ptr<SpecialCaseList>
SpecialCaseList::createOrDie(const std::vector<std::string> &paths)
{
   std::string error;
   if (auto scl = create(paths, error)) {
      return scl;
   }
   report_fatal_error(error);
}

bool SpecialCaseList::createInternal(const std::vector<std::string> &paths,
                                     std::string &error)
{
   StringMap<size_t> m_sections;
   for (const auto &Path : paths) {
      OptionalError<std::unique_ptr<MemoryBuffer>> fileOrErr =
            MemoryBuffer::getFile(Path);
      if (std::error_code errorCode = fileOrErr.getError()) {
         error = (Twine("can't open file '") + Path + "': " + errorCode.message()).getStr();
         return false;
      }
      std::string ParseError;
      if (!parse(fileOrErr.get().get(), m_sections, ParseError)) {
         error = (Twine("error parsing file '") + Path + "': " + ParseError).getStr();
         return false;
      }
   }
   return true;
}

bool SpecialCaseList::createInternal(const MemoryBuffer *memoryBuffer,
                                     std::string &error)
{
   StringMap<size_t> m_sections;
   if (!parse(memoryBuffer, m_sections, error)) {
      return false;
   }
   return true;
}

bool SpecialCaseList::parse(const MemoryBuffer *memoryBuffer,
                            StringMap<size_t> &sectionsMap,
                            std::string &error)
{
   // Iterate through each line in the blacklist file.
   SmallVector<StringRef, 16> lines;
   memoryBuffer->getBuffer().split(lines, '\n');

   unsigned lineNo = 1;
   StringRef section = "*";

   for (auto iter = lines.begin(), endMark = lines.end(); iter != endMark; ++iter, ++lineNo) {
      *iter = iter->trim();
      // Ignore empty lines and lines starting with "#"
      if (iter->empty() || iter->startsWith("#")) {
         continue;
      }

      // Save section names
      if (iter->startsWith("[")) {
         if (!iter->endsWith("]")) {
            error = (Twine("malformed section header on line ") + Twine(lineNo) +
                     ": " + *iter).getStr();
            return false;
         }

         section = iter->slice(1, iter->getSize() - 1);
         try{
            std::regex checkRE(section.getStr());
            POLAR_UNUSED(checkRE);
            continue;
         } catch (const std::regex_error &exp) {
            error =
                  (Twine("malformed regex for section ") + section + ": '" + exp.what())
                  .getStr();
            return false;
         }
      }

      // Get our prefix and unparsed regexp.
      std::pair<StringRef, StringRef> splitLine = iter->split(":");
      StringRef prefix = splitLine.first;
      if (splitLine.second.empty()) {
         // Missing ':' in the line.
         error = (Twine("malformed line ") + Twine(lineNo) + ": '" +
                  splitLine.first + "'").getStr();
         return false;
      }

      std::pair<StringRef, StringRef> splitRegexp = splitLine.second.split("=");
      std::string regexp = splitRegexp.first;
      StringRef category = splitRegexp.second;

      // Create this section if it has not been seen before.
      if (sectionsMap.find(section) == sectionsMap.end()) {
         std::unique_ptr<Matcher> matcher = std::make_unique<Matcher>();
         std::string reError;
         if (!matcher->insert(section, lineNo, reError)) {
            error = (Twine("malformed section ") + section + ": '" + reError).getStr();
            return false;
         }

         sectionsMap[section] = m_sections.size();
         m_sections.emplace_back(std::move(matcher));
      }

      auto &entry = m_sections[sectionsMap[section]].m_entries[prefix][category];
      std::string reError;
      if (!entry.insert(std::move(regexp), lineNo, reError)) {
         error = (Twine("malformed regex in line ") + Twine(lineNo) + ": '" +
                  splitLine.second + "': " + reError).getStr();
         return false;
      }
   }
   return true;
}

SpecialCaseList::~SpecialCaseList() {}

bool SpecialCaseList::inSection(StringRef section, StringRef prefix,
                                StringRef query, StringRef category) const
{
   return inSectionBlame(section, prefix, query, category);
}

unsigned SpecialCaseList::inSectionBlame(StringRef section, StringRef prefix,
                                         StringRef query,
                                         StringRef category) const
{
   for (auto &sectionIter : m_sections) {
      if (sectionIter.m_sectionMatcher->match(section)) {
         unsigned blame =
               inSectionBlame(sectionIter.m_entries, prefix, query, category);
         if (blame) {
            return blame;
         }
      }
   }
   return 0;
}

unsigned SpecialCaseList::inSectionBlame(const SectionEntries &entries,
                                         StringRef prefix, StringRef query,
                                         StringRef category) const
{
   SectionEntries::const_iterator iter = entries.find(prefix);
   if (iter == entries.end()) {
      return 0;
   }
   StringMap<Matcher>::const_iterator iter1 = iter->m_second.find(category);
   if (iter1 == iter->m_second.end()) {
      return 0;
   }
   return iter1->getValue().match(query);
}

} // utils
} // polar
