// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/09/17.

#include "ProgressBar.h"
#include "LitGlobal.h"
#include "Utils.h"
#include "CLI/CLI.hpp"
#include "polarphp/basic/adt/StringRef.h"
#include <assert.h>
#include <boost/regex.hpp>

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

int TerminalController::COLUMNS = -1;
int TerminalController::LINE_COUNT = -1;
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

std::list<std::string> TerminalController::COLOR_TYPES {
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

namespace {
class CursesWinUnlocker
{
public:
   CursesWinUnlocker(bool needUnlock)
      : m_needUnlock(needUnlock)
   {
   }
   ~CursesWinUnlocker()
   {
      if (m_needUnlock) {
         ::endwin();
      }
   }
protected:
   bool m_needUnlock;
};
}

/// Create a `TerminalController` and initialize its attributes
/// with appropriate values for the current terminal.
/// `term_stream` is the stream that will be used for terminal
/// output; if this stream is not a tty, then the terminal is
/// assumed to be a dumb terminal (i.e., have no capabilities).
///
TerminalController::TerminalController(std::ostream &)
   : m_initialized(false)
{
   // If the stream isn't a tty, then assume it has no capabilities.
   if (!stdcout_isatty()) {
      throw std::runtime_error("stdcout is not a tty device");
   }
   initTermScreen();
   CursesWinUnlocker winLocker(true);
   // Check the terminal type.  If we fail, then assume that the
   // terminal has no capabilities.
   // Look up numeric capabilities.
   COLUMNS = ::tigetnum(const_cast<char *>("cols"));
   LINE_COUNT = ::tigetnum(const_cast<char *>("lines"));
   XN = ::tigetflag(const_cast<char *>("xenl"));
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
   std::string setFg = tigetStr("setf");
   if (!setFg.empty()) {
      auto iter = COLOR_TYPES.begin();
      int index = 0;
      for (; iter != COLOR_TYPES.end(); ++iter, ++index) {
         m_properties[*iter] = tparm(setFg, index);
      }
   }
   std::string setAnsiFg = tigetStr("setaf");
   if (!setAnsiFg.empty()) {
      auto iter = ANSICOLORS.begin();
      int index = 0;
      for (; iter != ANSICOLORS.end(); ++iter, ++index) {
         m_properties[*iter] = tparm(setAnsiFg, index);
      }
   }

   std::string setBg = tigetStr("setb");
   if (!setBg.empty()) {
      auto iter = COLOR_TYPES.begin();
      int index = 0;
      for (; iter != COLOR_TYPES.end(); ++iter, ++index) {
         m_properties[*iter] = tparm(setFg, index);
      }
   }
   std::string setAnsiBg = tigetStr("setab");
   if (!setAnsiBg.empty()) {
      auto iter = ANSICOLORS.begin();
      int index = 0;
      for (; iter != ANSICOLORS.end(); ++iter, ++index) {
         m_properties[*iter] = tparm(setAnsiFg, index);
      }
   }
}

TerminalController::~TerminalController()
{
}

std::string TerminalController::tigetStr(const std::string &capName)
{
   // String capabilities can include "delays" of the form "$<2>".
   // For any modern terminal, we should be able to just ignore
   // these, so strip them out.
   char *str = ::tigetstr(const_cast<char *>(capName.c_str()));
   std::string cap;
   if (str != nullptr && str != reinterpret_cast<char *>(-1)) {
      cap = str;
   }
   if (!cap.empty()) {
      boost::regex regex("$<\\d+>[/*]?");
      cap = boost::regex_replace(cap, regex, "");
   }
   return cap;
}

std::string TerminalController::tparm(const std::string &arg, int index)
{
   char *str = ::tparm(const_cast<char *>(arg.c_str()), index);
   if (str == nullptr || str == reinterpret_cast<char *>(-1)) {
      return std::string{};
   }
   return str;
}

void TerminalController::initTermScreen()
{
   std::lock_guard locker(m_mutex);
   NCURSES_CONST char *name;
   if ((name = getenv("TERM")) == nullptr
       || *name == '\0')
      name = const_cast<char *>("unknown");
#ifdef __CYGWIN__
   /*
       * 2002/9/21
       * Work around a bug in Cygwin.  Full-screen subprocesses run from
       * bash, in turn spawned from another full-screen process, will dump
       * core when attempting to write to stdout.  Opening /dev/tty
       * explicitly seems to fix the problem.
       */
   if (isatty(fileno(stdout))) {
      FILE *fp = fopen("/dev/tty", "w");
      if (fp != 0 && isatty(fileno(fp))) {
         fclose(stdout);
         dup2(fileno(fp), STDOUT_FILENO);
         stdout = fdopen(STDOUT_FILENO, "w");
      }
   }
#endif
   if (newterm(name, stdout, stdin) == nullptr) {
      throw std::runtime_error(format_string("Error opening terminal: %s.\n", name));
   }
   /* def_shell_mode - done in newterm/_nc_setupscreen */
   def_prog_mode();
}

///
/// Replace each $-substitutions in the given template string with
/// the corresponding terminal control string (if it's defined) or
/// '' (if it's not).
///
std::string TerminalController::render(std::string tpl) const
{
   boost::regex regex(R"(\$\{(\w+)\})");
   boost::smatch varMatch;
   while(boost::regex_search(tpl, varMatch, regex)) {
      std::string varname = varMatch[1];
      if (m_properties.find(varname) != m_properties.end()) {
         tpl.replace(varMatch[0].first, varMatch[0].second, m_properties.at(varname));
      }
   }
   return tpl;
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

void SimpleProgressBar::update(float percent, std::string message)
{
   if (m_atIndex == -1) {
      std::printf("%s\n", m_header.c_str());
      m_atIndex = 0;
   }
   int next = static_cast<int>(percent * 50);
   if (next == m_atIndex) {
      return;
   }
   for (int i = m_atIndex; i < next; ++i) {
      int idx = i % 5;
      if (0 == idx) {
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
     m_header(header),
     m_cleared(true),
     m_useETA(useETA)
{
   if (m_term.getProperty(TerminalController::CLEAR_EOL).empty() ||
       m_term.getProperty(TerminalController::UP).empty() ||
       m_term.getProperty(TerminalController::BOL).empty()) {
      throw ValueError("Terminal isn't capable enough -- you "
                       "should use a simpler progress dispaly.");
   }
   if (m_term.COLUMNS != -1) {
      m_width = static_cast<size_t>(m_term.COLUMNS);
      if (!m_term.XN) {
         BOL = m_term.getProperty(TerminalController::UP) + m_term.getProperty(TerminalController::BOL);
         XNL = ""; // Cursor must be fed to the next line
      }
   } else {
      m_width = 75;
   }
   m_bar = m_term.render(BAR);
   if (m_useETA) {
      m_startTime = std::chrono::system_clock::now();
   }
}

void ProgressBar::update(float percent, std::string message)
{
   if (m_cleared) {
      std::printf("%s\n", m_header.c_str());
      m_cleared = false;
   }
   std::string prefix = format_string("%3d%%", static_cast<int>(percent * 100));
   std::string suffix = "";
   if (m_useETA) {
      auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - m_startTime).count();
      if (percent > 0.0001f && elapsed > 1) {
         int total = static_cast<int>(elapsed / percent);
         int eta = int(total - elapsed);
         int h = eta / 3600;
         int m = (eta / 60) % 60;
         int s = eta % 60;
         suffix = format_string(" ETA: %02d:%02d:%02d", h, m, s);
      }
   }
   size_t barWidth = m_width - prefix.size() - suffix.size() - 2;
   size_t n = static_cast<size_t>(barWidth * percent);
   if (message.size() < m_width) {
      message = message + std::string(m_width - message.size(), ' ');
   } else {
      message = "... " + message.substr(-(m_width - 4));
   }

   std::string output = BOL +
         m_term.getProperty(TerminalController::CLEAR_EOL);

   output += format_string(m_bar, prefix.c_str(), std::string(n, '=').c_str(),
                           std::string(barWidth - n, '-').c_str(), suffix.c_str());
   output += XNL;
   output += m_term.getProperty(TerminalController::CLEAR_EOL);
   output += message;
   std::printf("%s\n", output.c_str());
   std::fflush(stdout);
}

void ProgressBar::clear()
{
   if (!m_cleared) {
      std::printf("%s", (BOL + m_term.getProperty(TerminalController::CLEAR_EOL) +
                           m_term.getProperty(TerminalController::UP) +
                           m_term.getProperty(TerminalController::CLEAR_EOL)).c_str());
      std::fflush(stdout);
      m_cleared = true;
   }
}

TestingProgressDisplay::TestingProgressDisplay(const CLI::App &opts, size_t numTests,
                                               std::shared_ptr<AbstractProgressBar> progressBar)
   : m_opts(opts),
     m_numTests(numTests),
     m_progressBar(progressBar),
     m_completed(0)
{
   m_showAllOutput = m_opts.get_option("--show-all")->count() > 0 ? true : false;
   m_incremental = m_opts.get_option("--incremental")->count() > 0 ? true : false;
   m_quiet = m_opts.get_option("--quiet")->count() > 0 ? true : false;
   m_succinct = m_opts.get_option("--succinct")->count() > 0 ? true : false;
   m_showOutput = m_opts.get_option("--verbose")->count() > 0 ? true : false;
}

void TestingProgressDisplay::finish()
{
   if (m_progressBar) {
      m_progressBar->clear();
   } else if (m_quiet) {
      // TODO
   } else if (m_succinct) {
      std::printf("\n");
   }
}

void update_incremental_cache(TestPointer test)
{
   if (!test->getResult()->getCode()->isFailure()) {
      return;
   }
   polar::lit::modify_file_utime_and_atime(test->getFilePath());
}

void TestingProgressDisplay::update(TestPointer test)
{
   m_completed += 1;
   if (m_incremental) {
      update_incremental_cache(test);
   }
   if (m_progressBar) {
      m_progressBar->update(m_completed / static_cast<float>(m_numTests), test->getFullName());
   }
   assert(test->getResult()->getCode() && "test result code is not set");
   bool shouldShow = test->getResult()->getCode()->isFailure() ||
         m_showAllOutput ||
         (!m_quiet && !m_succinct);

   if (!shouldShow) {
      return;
   }
   if (m_progressBar) {
      m_progressBar->clear();
   }
   // Show the test result line.
   std::string testName = test->getFullName();
   ResultPointer testResult = test->getResult();
   const ResultCode *resultCode = testResult->getCode();
   std::printf("%s: %s (%lu of %lu)\n", resultCode->getName().c_str(),
               testName.c_str(), m_completed, m_numTests);
   // Show the test failure output, if requested.
   if ((resultCode->isFailure() && m_showOutput) ||
       m_showAllOutput) {
      if (resultCode->isFailure()) {
         std::printf("%s TEST '%s' FAILED %s\n", std::string(20, '*').c_str(),
                     test->getFullName().c_str(), std::string(20, '*').c_str());
      }
      std::cout << testResult->getOutput() << std::endl;
      std::cout << std::string(20, '*') << std::endl;
   }
   // Report test metrics, if present.
   if (!testResult->getMetrics().empty()) {
      // @TODO sort the metrics
      std::printf("%s TEST '%s' RESULTS %s\n", std::string(10, '*').c_str(),
                  test->getFullName().c_str(),
                  std::string(10, '*').c_str());
      for (auto &item : testResult->getMetrics()) {
         std::printf("%s: %s \n", item.first.c_str(), item.second->format().c_str());
      }
      std::cout << std::string(10, '*') << std::endl;
   }
   // Report micro-tests, if present
   if (!testResult->getMicroResults().empty()) {
      // @TODO sort the MicroResults
      for (auto &item : testResult->getMicroResults()) {
         std::printf("%s MICRO-TEST: %s\n", std::string(3, '*').c_str(), item.first.c_str());
         ResultPointer microTest = item.second;
         if (!microTest->getMetrics().empty()) {
            // @TODO sort the metrics
            for (auto &microItem : microTest->getMetrics()) {
               std::printf("    %s:  %s \n", microItem.first.c_str(), microItem.second->format().c_str());
            }
         }
      }
   }
   // Ensure the output is flushed.
   std::fflush(stdout);
}

} // lit
} // polar
