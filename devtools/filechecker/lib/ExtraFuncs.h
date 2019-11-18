// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/10/25.

#ifndef POLAR_DEVLTOOLS_FILECHECKER_GLOBAL_H
#define POLAR_DEVLTOOLS_FILECHECKER_GLOBAL_H

#include "FileCheckerConfig.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/Support/FileCheck.h"
#include "llvm/ADT/StringRef.h"
#include <cstddef>
#include <vector>
#include <string>
#include <set>

// forward declare with namespace
namespace CLI {
class App;
} // CLI

namespace polar {
namespace filechecker {

using llvm::ManagedStatic;
using llvm::raw_ostream;
using llvm::FileCheckDiag;
using llvm::FileCheckRequest;
using llvm::StringRef;
using llvm::Check::FileCheckType;

enum class DumpInputValue
{
   Default,
   Help,
   Never,
   Fail,
   Always
};

struct MarkerStyle
{
   /// The starting char (before tildes) for marking the line.
   char lead;
   /// What color to use for this annotation.
   raw_ostream::Colors color;
   /// A note to follow the marker, or empty string if none.
   std::string note;
   MarkerStyle() {}
   MarkerStyle(char lead, raw_ostream::Colors color,
               const std::string &note = "")
      : lead(lead),
        color(color),
        note(note)
   {}
};


/// An annotation for a single input line.
struct InputAnnotation
{
   /// The check file line (one-origin indexing) where the directive that
   /// produced this annotation is located.
   unsigned checkLine;
   /// The index of the match result for this check.
   unsigned checkDiagIndex;
   /// The label for this annotation.
   std::string label;
   /// What input line (one-origin indexing) this annotation marks.  This might
   /// be different from the starting line of the original diagnostic if this is
   /// a non-initial fragment of a diagnostic that has been broken across
   /// multiple lines.
   unsigned inputLine;
   /// The column range (one-origin indexing, open end) in which to to mark the
   /// input line.  If InputEndCol is UINT_MAX, treat it as the last column
   /// before the newline.
   unsigned inputStartCol;
   unsigned inputEndCol;
   /// The marker to use.
   MarkerStyle marker;
   /// Whether this annotation represents a good match for an expected pattern.
   bool foundAndExpectedMatch;
};

extern CLI::App *sg_commandParser;
CLI::App &retrieve_command_parser();
extern std::vector<std::string> sg_checkPrefixes;
extern std::vector<std::string> sg_defines;
extern std::vector<std::string> sg_implicitCheckNot;
std::string dump_input_checker(const std::string &value);
DumpInputValue get_dump_input_type(const std::string &opt);

void dump_command_line(int argc, char **argv);
MarkerStyle get_marker(FileCheckDiag::MatchType matchTy);
void dump_input_annotation_help(raw_ostream &outStream);
std::string get_check_type_abbreviation(FileCheckType type);
void build_input_annotations(const std::vector<FileCheckDiag> &diags,
                             std::vector<InputAnnotation> &annotations,
                             unsigned &labelWidth);
void dump_annotated_input(raw_ostream &outStream, const FileCheckRequest &req,
                          StringRef inputFileText,
                          std::vector<InputAnnotation> &annotations,
                          unsigned labelWidth);

} // filechecker
} // polar

#endif // POLAR_DEVLTOOLS_FILECHECKER_GLOBAL_H
