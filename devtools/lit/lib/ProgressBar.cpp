// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://polarphp.org/LICENSE.txt for license information
// See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/09/17.

#include "ProgressBar.h"
#include "Global.h"
#include "Utils.h"
#include <assert.h>
#include <regex>

namespace polar {
namespace lit {

const std::string TerminalController::BOL{"BOL"};
const std::string TerminalController::UP{"UP"};
const std::string TerminalController::DOWN{"DOWN"};
const std::string TerminalController::LEFT{"LEFT"};
const std::string TerminalController::RIGHT{"RIGHT"};

const std::string TerminalController::CLEAR_SCREEN{"CLEAR_SCREEN"};
const std::string TerminalController::CLEAR_EOL{"CLEAR_EOL"};
const std::string TerminalController::CLEAR_BOL{"CLEAR_BOL"};
const std::string TerminalController::CLEAR_EOS{"CLEAR_EOS"};

const std::string TerminalController::BOLD{"BOLD"};
const std::string TerminalController::BLINK{"BLINK"};
const std::string TerminalController::DIM{"DIM"};
const std::string TerminalController::REVERSE{"REVERSE"};
const std::string TerminalController::NORMAL{"NORMAL"};

const std::string TerminalController::HIDE_CURSOR{"HIDE_CURSOR"};
const std::string TerminalController::SHOW_CURSOR{"SHOW_CURSOR"};

int TerminalController::COLS = -1;
int TerminalController::LINES = -1;
bool TerminalController::XN = false;

std::list<std::string> TerminalController::STRING_CAPABILITIES {
   "BOL=cr",
   "UP=cuu1",
   "DOWN=cud1",
   "LEFT=cub1",
   "RIGHT=cuf1",
   "CLEAR_SCREEN=clear",
   "CLEAR_EOL=el",
   "CLEAR_BOL=el1",
   "CLEAR_EOS=ed",
   "BOLD=bold",
   "BLINK=blink",
   "DIM=dim",
   "REVERSE=rev",
   "UNDERLINE=smul",
   "NORMAL=sgr0",
   "HIDE_CURSOR=cinvis",
   "SHOW_CURSOR=cnorm"
};

std::list<std::string> TerminalController::COLORS {
   "BLACK",
   "BLUE",
   "GREEN",
   "CYAN",
   "RED",
   "MAGENTA",
   "YELLOW",
   "WHITE"
};

std::list<std::string> TerminalController::ANSICOLORS {
   "BLACK",
   "RED",
   "GREEN",
   "YELLOW",
   "BLUE",
   "MAGENTA",
   "CYAN",
   "WHITE"
};

/// Create a `TerminalController` and initialize its attributes
/// with appropriate values for the current terminal.
/// `term_stream` is the stream that will be used for terminal
/// output; if this stream is not a tty, then the terminal is
/// assumed to be a dumb terminal (i.e., have no capabilities).
///
TerminalController::TerminalController(std::ostream &stream)
{
   // If the stream isn't a tty, then assume it has no capabilities.
   if (!stdcout_isatty()) {
      throw std::runtime_error("stdcout is not a tty device");
   }
   // Check the terminal type.  If we fail, then assume that the
   // terminal has no capabilities.
   ::initscr();
   // Look up numeric capabilities.
   COLS = ::tigetnum("cols");
   LINES = ::tigetnum("lines");
   XN = ::tigetflag("xenl");
   // Look up string capabilities.
   for (std::string capability : STRING_CAPABILITIES) {
      std::list<std::string> parts = split_string(capability, '=');
      assert(parts.size() == 2);
      std::list<std::string>::iterator iter = parts.begin();
      std::string attribute = *iter++;
      std::string capName = *iter++;
      m_properties[attribute] = tigetStr(capName);
   }
   // init Colors
   char *setFg = ::tigetstr("setf");
   if (nullptr != setFg) {
      auto iter = COLORS.begin();
      int index = 0;
      for (; iter != COLORS.end(); ++iter, ++index) {
         m_properties[*iter] = ::tparm(setFg, index);
      }
   }
   char *setAnsiFg = ::tigetstr("setaf");
   if (nullptr != setAnsiFg) {
      auto iter = ANSICOLORS.begin();
      int index = 0;
      for (; iter != ANSICOLORS.end(); ++iter, ++index) {
         m_properties[*iter] = ::tparm(setAnsiFg, index);
      }
   }

   char *setBg = ::tigetstr("setb");
   if (nullptr != setBg) {
      auto iter = COLORS.begin();
      int index = 0;
      for (; iter != COLORS.end(); ++iter, ++index) {
         m_properties[*iter] = ::tparm(setFg, index);
      }
   }
   char *setAnsiBg = ::tigetstr("setab");
   if (nullptr != setAnsiBg) {
      auto iter = ANSICOLORS.begin();
      int index = 0;
      for (; iter != ANSICOLORS.end(); ++iter, ++index) {
         m_properties[*iter] = ::tparm(setAnsiFg, index);
      }
   }
}

TerminalController::~TerminalController()
{
   ::endwin();
}

std::string TerminalController::tigetStr(const std::string &capName)
{
   // String capabilities can include "delays" of the form "$<2>".
   // For any modern terminal, we should be able to just ignore
   // these, so strip them out.
   std::string cap(::tigetstr(const_cast<char *>(capName.c_str())));
   if (!cap.empty()) {
      std::regex regex("\$<\d+>[/*]?");
      cap = std::regex_replace(cap, regex, "");
   }
   return cap;
}

///
/// Replace each $-substitutions in the given template string with
/// the corresponding terminal control string (if it's defined) or
/// '' (if it's not).
///
std::string TerminalController::render(const std::string &tpl)
{
   std::regex regex(R"(\$\{\w+\})");
   auto tplSearchBegin =
         std::sregex_token_iterator(tpl.begin(), tpl.end(), regex, 0);
   auto tplSearchEnd = std::sregex_token_iterator();
   while (tplSearchBegin != tplSearchEnd) {
      std::sub_match subMatch = *tplSearchBegin;
      if (subMatch.matched) {
         std::string varname = subMatch.str().substr(2, subMatch.length() - 3);
         trim_string(varname);
         if (m_properties.find(varname) != m_properties.end()) {
            tpl.replace(subMatch.first, subMatch.second, m_properties.at(varname));
         }
      }
      ++tplSearchBegin;
   }
}

} // lit
} // polar
