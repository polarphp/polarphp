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
std::string TerminalController::render(std::string tpl) const
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

const std::string &TerminalController::getProperty(const std::string &key) const
{
   return m_properties.at(key);
}

SimpleProgressBar::SimpleProgressBar(const std::string &header)
   : m_header(header),
     m_atIndex(-1)
{
}

void SimpleProgressBar::update(float percent, const std::string &message)
{
   if (m_atIndex == -1) {
      std::printf(m_header.c_str());
      m_atIndex = 0;
   }
   int next = int(percent * 50);
   if (next == m_atIndex) {
      return;
   }
   for (int i = m_atIndex; i < next; ++i) {
      int idx = i % 5;
      if (5 == idx) {
         std::printf("%-2d", i * 2);
      } else if (1 == idx) {
         // skip
      } else if (idx < 4) {
         std::printf(".");
      } else {
         std::printf(" ");
      }
   }
   std::fflush(stdout);
   m_atIndex = next;
}

void SimpleProgressBar::clear()
{
   if (m_atIndex != -1) {
      std::cout << std::endl;
      std::cout.flush();
      m_atIndex = -1;
   }
}

const std::string ProgressBar::BAR ="%s${GREEN}[${BOLD}%s%s${NORMAL}${GREEN}]${NORMAL}%s";
const std::string ProgressBar::HEADER = "${BOLD}${CYAN}%s${NORMAL}\n\n";

ProgressBar::ProgressBar(const TerminalController &term, const std::string &header,
                         bool useETA)
   : BOL(term.getProperty(TerminalController::BOL)),
     XNL("\n"),
     m_term(term),
     m_cleared(true),
     m_useETA(useETA)
{
   if (m_term.getProperty(TerminalController::CLEAR_EOL).empty() ||
       m_term.getProperty(TerminalController::UP).empty() ||
       m_term.getProperty(TerminalController::BOL).empty()) {
      throw ValueError("Terminal isn't capable enough -- you "
                       "should use a simpler progress dispaly.");
   }
   if (m_term.COLS != -1) {
      m_width = m_term.COLS;
      if (!m_term.XN) {
         BOL = m_term.getProperty(TerminalController::UP) + m_term.getProperty(TerminalController::BOL);
         XNL = ""; // Cursor must be fed to the next line
      }
   } else {
      m_width = 75;
   }
   m_bar = m_term.render(BAR);
   m_useETA = useETA;
   if (m_useETA) {
      m_startTime = std::chrono::system_clock::now();
   }
   update(0, "");
}

void ProgressBar::update(float percent, std::string message)
{
   if (m_cleared) {
      std::printf(m_header.c_str());
      m_cleared = false;
   }
   std::string prefix = format_string("%3d%%", percent * 100);
   std::string suffix = "";
   if (m_useETA) {
      int elapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - m_startTime).count();
      if (percent > 0.0001 && elapsed > 1) {
         int total = elapsed / percent;
         int eta = int(total - elapsed);
         int h = eta / 3600;
         int m = (eta / 60) % 60;
         int s = eta % 60;
         suffix = format_string(" ETA: %02d:%02d:%02d", h, m, s);
      }
   }
   int barWidth = m_width - prefix.size() - suffix.size() - 2;
   int n = barWidth * percent;
   if (message.size() < m_width) {
      message = message + std::string(m_width - message.size(), ' ');
   } else {
      message = "... " + message.substr(-(m_width - 4));
   }
   std::string output = BOL + m_term.getProperty(TerminalController::UP) +
         m_term.getProperty(TerminalController::CLEAR_EOL);
   output += format_string(m_bar, prefix.c_str(), std::string(n, '=').c_str(),
                           std::string(barWidth - n, '-'), suffix);
   output += XNL;
   output += m_term.getProperty(TerminalController::CLEAR_EOL);
   output += message;
   std::printf(message.c_str());
   if (!m_term.XN) {
      std::fflush(stdout);
   }
}

void ProgressBar::clear()
{
   if (!m_cleared) {
      std::printf((BOL + m_term.getProperty(TerminalController::CLEAR_EOL) +
                   m_term.getProperty(TerminalController::UP) +
                   m_term.getProperty(TerminalController::CLEAR_EOL) +
                   m_term.getProperty(TerminalController::UP) +
                   m_term.getProperty(TerminalController::CLEAR_EOL)).c_str());
      std::fflush(stdout);
      m_cleared = true;
   }
}

} // lit
} // polar
