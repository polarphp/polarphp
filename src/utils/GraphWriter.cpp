// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/04.

#include "polarphp/utils/GraphWriter.h"
#include "polarphp/basic/adt/SmallString.h"
#include "polarphp/basic/adt/SmallVector.h"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/utils/CommandLine.h"
#include "polarphp/utils/ErrorHandling.h"
#include "polarphp/utils/OptionalError.h"
#include "polarphp/utils/FileSystem.h"
#include "polarphp/utils/Program.h"
#include "polarphp/utils/RawOutStream.h"
#include <cassert>
#include <system_error>
#include <string>
#include <vector>

namespace polar {
namespace utils {

using polar::basic::StringRef;
using polar::basic::SmallString;

static cmd::Opt<bool> sg_viewBackground("view-background", cmd::Hidden,
                                        cmd::Desc("Execute graph viewer in the background. Creates tmp file litter."));

std::string dot::escape_string(const std::string &label)
{
   std::string str(label);
   for (unsigned i = 0; i != str.length(); ++i)
      switch (str[i]) {
      case '\n':
         str.insert(str.begin()+i, '\\');  // Escape character...
         ++i;
         str[i] = 'n';
         break;
      case '\t':
         str.insert(str.begin()+i, ' ');  // Convert to two spaces
         ++i;
         str[i] = ' ';
         break;
      case '\\':
         if (i+1 != str.length())
            switch (str[i+1]) {
            case 'l': continue; // don't disturb \l
            case '|': case '{': case '}':
               str.erase(str.begin()+i); continue;
            default: break;
            }
         POLAR_FALLTHROUGH;
      case '{': case '}':
      case '<': case '>':
      case '|': case '"':
         str.insert(str.begin()+i, '\\');  // Escape character...
         ++i;  // don't infinite loop
         break;
      }
   return str;
}

/// \brief Get a color string for this node number. Simply round-robin selects
/// from a reasonable number of colors.
StringRef dot::get_color_string(unsigned colorNumber)
{
   static const int numColors = 20;
   static const char* colors[numColors] = {
      "aaaaaa", "aa0000", "00aa00", "aa5500", "0055ff", "aa00aa", "00aaaa",
      "555555", "ff5555", "55ff55", "ffff55", "5555ff", "ff55ff", "55ffff",
      "ffaaaa", "aaffaa", "ffffaa", "aaaaff", "ffaaff", "aaffff"};
   return colors[colorNumber % numColors];
}

std::string create_graph_filename(const Twine &name, int &fd) {
   fd = -1;
   SmallString<128> filename;
   std::error_code errorCode = polar::fs::create_temporary_file(name, "dot", fd, filename);
   if (errorCode) {
      error_stream() << "Error: " << errorCode.message() << "\n";
      return "";
   }

   error_stream() << "Writing '" << filename << "'... ";
   return filename.getStr();
}

// Execute the graph viewer. Return true if there were errors.
static bool exec_graph_viewer(StringRef execPath, std::vector<StringRef> &args,
                              StringRef filename, bool wait,
                              std::string &errMsg)
{
   if (wait) {
      if (sys::execute_and_wait(execPath, args, std::nullopt, std::nullopt, {}, 0, 0,
                                &errMsg)) {
         error_stream() << "Error: " << errMsg << "\n";
         return true;
      }
      polar::fs::remove(filename);
      error_stream() << " done. \n";
   } else {
      sys::execute_no_wait(execPath, args, std::nullopt, std::nullopt, {}, 0, &errMsg);
      error_stream() << "Remember to erase graph file: " << filename << "\n";
   }
   return false;
}

namespace {

struct GraphSession
{
   std::string m_logBuffer;

   bool tryFindProgram(StringRef names, std::string &programPath)
   {
      RawStringOutStream log(m_logBuffer);
      SmallVector<StringRef, 8> parts;
      names.split(parts, '|');
      for (auto name : parts) {
         if (OptionalError<std::string> pname = sys::find_program_by_name(name)) {
            programPath = *pname;
            return true;
         }
         log << "  Tried '" << name << "'\n";
      }
      return false;
   }
};

} // end anonymous namespace

static const char *get_program_name(graphprogram::Name program) {
   switch (program) {
   case graphprogram::DOT:
      return "dot";
   case graphprogram::FDP:
      return "fdp";
   case graphprogram::NEATO:
      return "neato";
   case graphprogram::TWOPI:
      return "twopi";
   case graphprogram::CIRCO:
      return "circo";
   }
   polar_unreachable("bad kind");
}

bool display_graph(StringRef filenameRef, bool wait,
                   graphprogram::Name program) {
   std::string filename = filenameRef;
   std::string errMsg;
   std::string viewerPath;
   GraphSession session;

#ifdef __APPLE__
   wait &= !sg_viewBackground;
   if (session.tryFindProgram("open", viewerPath)) {
      std::vector<StringRef> args;
      args.push_back(viewerPath);
      if (wait) {
         args.push_back("-W");
      }
      args.push_back(filename);
      error_stream() << "Trying 'open' program... ";
      if (!exec_graph_viewer(viewerPath, args, filename, wait, errMsg)) {
         return false;
      }
   }
#endif
   if (session.tryFindProgram("xdg-open", viewerPath)) {
      std::vector<StringRef> args;
      args.push_back(viewerPath);
      args.push_back(filename);
      error_stream() << "Trying 'xdg-open' program... ";
      if (!exec_graph_viewer(viewerPath, args, filename, wait, errMsg)) {
         return false;
      }
   }

   // Graphviz
   if (session.tryFindProgram("Graphviz", viewerPath)) {
      std::vector<StringRef> args;
      args.push_back(viewerPath);
      args.push_back(filename);
      error_stream() << "Running 'Graphviz' program... ";
      return exec_graph_viewer(viewerPath, args, filename, wait, errMsg);
   }

   // xdot
   if (session.tryFindProgram("xdot|xdot.py", viewerPath)) {
      std::vector<StringRef> args;
      args.push_back(viewerPath);
      args.push_back(filename);

      args.push_back("-f");
      args.push_back(get_program_name(program));

      error_stream() << "Running 'xdot.py' program... ";
      return exec_graph_viewer(viewerPath, args, filename, wait, errMsg);
   }

   enum ViewerKind {
      VK_None,
      VK_OSXOpen,
      VK_XDGOpen,
      VK_Ghostview,
      VK_CmdStart
   };
   ViewerKind viewer = VK_None;
#ifdef __APPLE__
   if (!viewer && session.tryFindProgram("open", viewerPath))
      viewer = VK_OSXOpen;
#endif
   if (!viewer && session.tryFindProgram("gv", viewerPath))
      viewer = VK_Ghostview;
   if (!viewer && session.tryFindProgram("xdg-open", viewerPath))
      viewer = VK_XDGOpen;
#ifdef POLAR_ON_WIN32
   if (!viewer && session.tryFindProgram("cmd", viewerPath)) {
      viewer = VK_CmdStart;
   }
#endif

   // PostScript or PDF graph generator + PostScript/PDF viewer
   std::string GeneratorPath;
   if (viewer &&
       (session.tryFindProgram(get_program_name(program), GeneratorPath) ||
        session.tryFindProgram("dot|fdp|neato|twopi|circo", GeneratorPath))) {
      std::string OutputFilename =
            filename + (viewer == VK_CmdStart ? ".pdf" : ".ps");

      std::vector<StringRef> args;
      args.push_back(GeneratorPath.c_str());
      if (viewer == VK_CmdStart) {
         args.push_back("-Tpdf");
      } else {
         args.push_back("-Tps");
      }
      args.push_back("-Nfontname=Courier");
      args.push_back("-Gsize=7.5,10");
      args.push_back(filename);
      args.push_back("-o");
      args.push_back(OutputFilename);

      error_stream() << "Running '" << GeneratorPath << "' program... ";

      if (exec_graph_viewer(GeneratorPath, args, filename, true, errMsg)) {
         return true;
      }

      // The lifetime of StartArg must include the call of exec_graph_viewer
      // because the args are passed as vector of char*.
      std::string StartArg;

      args.clear();
      args.push_back(viewerPath);
      switch (viewer) {
      case VK_OSXOpen:
         args.push_back("-W");
         args.push_back(OutputFilename);
         break;
      case VK_XDGOpen:
         wait = false;
         args.push_back(OutputFilename);
         break;
      case VK_Ghostview:
         args.push_back("--spartan");
         args.push_back(OutputFilename);
         break;
      case VK_CmdStart:
         args.push_back("/session");
         args.push_back("/C");
         StartArg =
               (StringRef("start ") + (wait ? "/WAIT " : "") + OutputFilename).getStr();
         args.push_back(StartArg);
         break;
      case VK_None:
         polar_unreachable("Invalid viewer");
      }

      errMsg.clear();
      return exec_graph_viewer(viewerPath, args, OutputFilename, wait, errMsg);
   }

   // dotty
   if (session.tryFindProgram("dotty", viewerPath)) {
      std::vector<StringRef> args;
      args.push_back(viewerPath);
      args.push_back(filename);
      // Dotty spawns another app and doesn't wait until it returns
#ifdef POLAR_ON_WIN32
      wait = false;
#endif
      error_stream() << "Running 'dotty' program... ";
      return exec_graph_viewer(viewerPath, args, filename, wait, errMsg);
   }

   error_stream() << "Error: Couldn't find a usable graph viewer program:\n";
   error_stream() << session.m_logBuffer << "\n";
   return true;
}

} // utils
} // polar
