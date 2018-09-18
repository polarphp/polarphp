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

namespace polar {
namespace lit {

std::string TerminalController::BOL{};
std::string TerminalController::UP{};
std::string TerminalController::DOWN{};
std::string TerminalController::LEFT{};
std::string TerminalController::RIGHT{};

std::string TerminalController::CLEAR_SCREEN{};
std::string TerminalController::CLEAR_EOL{};
std::string TerminalController::CLEAR_BOL{};
std::string TerminalController::CLEAR_EOS{};

std::string TerminalController::BOLD{};
std::string TerminalController::BLINK{};
std::string TerminalController::DIM{};
std::string TerminalController::REVERSE{};
std::string TerminalController::NORMAL{};

std::string TerminalController::HIDE_CURSOR{};
std::string TerminalController::SHOW_CURSOR{};

int TerminalController::COLS = -1;
int TerminalController::LINES = -1;
bool TerminalController::XN = false;

std::string TerminalController::BLACK{};
std::string TerminalController::BLUE{};
std::string TerminalController::GREEN{};
std::string TerminalController::CYAN{};
std::string TerminalController::RED{};
std::string TerminalController::MAGENTA{};
std::string TerminalController::YELLOW{};
std::string TerminalController::WHITE{};

std::string TerminalController::BG_BLACK{};
std::string TerminalController::BG_BLUE{};
std::string TerminalController::BG_GREEN{};
std::string TerminalController::BG_CYAN{};
std::string TerminalController::BG_RED{};
std::string TerminalController::BG_MAGENTA{};
std::string TerminalController::BG_YELLOW{};
std::string TerminalController::BG_WHITE{};

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
      m_capabilities[attribute] = tigetStr(capName);
   }
   // init Colors
   char *setFg = ::tigetstr("setf");
   if (nullptr != setFg) {
      auto iter = COLORS.begin();
      int index = 0;
      for (; iter != COLORS.end(); ++iter, ++index) {
         m_fgColors[*iter] = ::tparm(setFg, index);
      }
   }
   char *setAnsiFg = ::tigetstr("setaf");
   if (nullptr != setAnsiFg) {
      auto iter = ANSICOLORS.begin();
      int index = 0;
      for (; iter != ANSICOLORS.end(); ++iter, ++index) {
         m_fgAnsiColors[*iter] = ::tparm(setAnsiFg, index);
      }
   }

   char *setBg = ::tigetstr("setb");
   if (nullptr != setBg) {
      auto iter = COLORS.begin();
      int index = 0;
      for (; iter != COLORS.end(); ++iter, ++index) {
         m_bgColors[*iter] = ::tparm(setFg, index);
      }
   }
   char *setAnsiBg = ::tigetstr("setab");
   if (nullptr != setAnsiBg) {
      auto iter = ANSICOLORS.begin();
      int index = 0;
      for (; iter != ANSICOLORS.end(); ++iter, ++index) {
         m_bgAnsiColors[*iter] = ::tparm(setAnsiFg, index);
      }
   }
}

TerminalController::~TerminalController()
{
   ::endwin();
}

std::string TerminalController::tigetStr(const std::string &capName)
{

}

} // lit
} // polar
