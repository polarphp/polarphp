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

#ifndef POLAR_DEVLTOOLS_LIT_PROGRESS_BAR_H
#define POLAR_DEVLTOOLS_LIT_PROGRESS_BAR_H

#include "Test.h"
#include <string>
#include <list>
#include <iostream>
#include <map>
#include <chrono>
#include <mutex>

// forward declare class with namespace
namespace CLI {
class App;
} // CLI

namespace polar {
namespace lit {

/// A class that can be used to portably generate formatted output to
/// a terminal.
///
/// TerminalController` defines a set of instance variables whose
/// values are initialized to the control sequence necessary to
/// perform a given action.  These can be simply included in normal
/// output to the terminal:
///   ```cpp
///        TerminalController term = TerminalController()
///        std::cout << "This is " << term.GREEN << "green" << term.NORMAL << std::endl;
///   ```
///   Alternatively, the `render()` method can used, which replaces
///   '${action}' with the string required to perform 'action':
///   ```cpp
///       TerminalController term = TerminalController();
///       std::cout << term.render(std::printf("This is ${GREEN}green${NORMAL}")) << std::endl;
///   ```
///   If the terminal doesn't support a given action, then the value of
///   the corresponding instance variable will be set to ''.  As a
///   result, the above code will still work on terminals that do not
///   support color, except that their output will not be colored.
///   Also, this means that you can test whether the terminal supports a
///   given action by simply testing the truth value of the
///   corresponding instance variable:
///   ```cpp
///      TerminalController term = TerminalController();
///      if term.CLEAR_SCREEN {
///         std::printf("This terminal supports clearning the screen.")
///      }
///  ```
///  Finally, if the width and height of the terminal are known, then
///  they will be stored in the `COLS` and `LINES` attributes.
///
class TerminalController
{
public:
   TerminalController(std::ostream &stream = std::cout);
   ~TerminalController();
   std::string render(std::string tpl) const;
   const std::string &getProperty(const std::string &key) const;
protected:
   std::string tigetStr(const std::string &capName);
   std::string tparm(const std::string &arg, int index);
   void initTermScreen();
public:
   // Cursor movement
   const static std::string BOL; // Move the cursor to the beginning of the line
   const static std::string UP; // Move the cursor up one line
   const static std::string DOWN; // Move the cursor down one line
   const static std::string LEFT; // Move the cursor left one char
   const static std::string RIGHT; // Move the cursor right one char

   // Deletion
   const static std::string CLEAR_SCREEN; // Clear the screen and move to home position
   const static std::string CLEAR_EOL; // Clear to the end of the line.
   const static std::string CLEAR_BOL; // Clear to the beginning of the line.
   const static std::string CLEAR_EOS; // Clear to the end of the screen

   // Output modes
   const static std::string BOLD; // Turn on bold mode
   const static std::string BLINK; // Turn on blink mode
   const static std::string DIM; // Turn on half-bright mode
   const static std::string REVERSE; // Turn on reverse-video mode
   const static std::string NORMAL; // Turn off all modes

   // Cursor display
   const static std::string HIDE_CURSOR; // Make the cursor invisible
   const static std::string SHOW_CURSOR; // Make the cursor visible

   // Terminal size:
   static int COLUMNS; // Width of the terminal (-1 for unknown)
   static int LINE_COUNT; // Height of the terminal (-1 for unknown)
   static bool XN;

protected:
   static std::list<std::string> STRING_CAPABILITIES;
   static std::list<std::string> COLOR_TYPES;
   static std::list<std::string> ANSICOLORS;
   std::map<std::string, std::string> m_properties;
   std::mutex m_mutex;
   bool m_initialized;
};

class AbstractProgressBar
{
public:
   virtual void update(float percent, std::string message) = 0;
   virtual void clear() = 0;
   virtual ~AbstractProgressBar() = default;
};

/// A simple progress bar which doesn't need any terminal support.
///
/// This prints out a progress bar like:
/// 'Header: 0 .. 10.. 20.. ...'
class SimpleProgressBar : public AbstractProgressBar
{
public:
   SimpleProgressBar(const std::string &header);
   void update(float percent, std::string message) override;
   void clear() override;
protected:
   std::string m_header;
   int m_atIndex;
};

/// A 3-line progress bar, which looks like::
///
///                                Header
///        20% [===========----------------------------------]
///                           progress message
///
///    The progress bar is colored, if the terminal supports color
///    output; and adjusts to the width of the terminal.
///
class ProgressBar : public AbstractProgressBar
{
public:
   ProgressBar(const TerminalController &term, const std::string &header,
               bool useETA = true);
   void update(float percent, std::string message) override;
   void clear() override;
protected:
   const static std::string BAR;
   const static std::string HEADER;
   std::string BOL;
   std::string XNL;
   const TerminalController &m_term;
   std::string m_bar;
   std::string m_header;
   bool m_cleared;
   bool m_useETA;
   size_t m_width;
   std::chrono::system_clock::time_point m_startTime;
};

class TestingProgressDisplay
{
public:
   TestingProgressDisplay(const CLI::App &opts, size_t numTests,
                          std::shared_ptr<AbstractProgressBar> progressBar = nullptr);
   void finish();
   void update(TestPointer test);
private:
   const CLI::App &m_opts;
   size_t m_numTests;
   std::shared_ptr<AbstractProgressBar> m_progressBar;
   size_t m_completed;
   bool m_quiet;
   bool m_succinct;
   bool m_showAllOutput;
   bool m_incremental;
   bool m_showOutput;
};

} // lit
} // polar

#endif // POLAR_DEVLTOOLS_LIT_PROGRESS_BAR_H
