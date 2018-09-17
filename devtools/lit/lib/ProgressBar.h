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

#ifndef POLAR_DEVLTOOLS_LIT_PROGRESS_BAR_H
#define POLAR_DEVLTOOLS_LIT_PROGRESS_BAR_H

#include <string>
#include <list>
#include <iostream>

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
///       std::cout << term.render(std::printf("This is %sgreen%s", term.GREEN, term.NORMAL)) << std::endl;
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
class TerminalController
{
public:
   TerminalController(std::ostream &stream = std::cout);
   void render(const std::string &tpl);
protected:
   void tparam(const std::string &arg, int index);
   void tigetStr(const std::string &capName);
   void renderSub(const std::string &match);

public:
   // Cursor movement
   static std::string BOL; // Move the cursor to the beginning of the line
   static std::string UP; // Move the cursor up one line
   static std::string DOWN; // Move the cursor down one line
   static std::string LEFT; // Move the cursor left one char
   static std::string RIGHT; // Move the cursor right one char

   // Deletion
   static std::string CLEAR_SCREEN; // Clear the screen and move to home position
   static std::string CLEAR_EOL; // Clear to the end of the line.
   static std::string CLEAR_BOL; // Clear to the beginning of the line.
   static std::string CLEAR_EOS; // Clear to the end of the screen

   // Output modes
   static std::string BOLD; // Turn on bold mode
   static std::string BLINK; // Turn on blink mode
   static std::string DIM; // Turn on half-bright mode
   static std::string REVERSE; // Turn on reverse-video mode
   static std::string NORMAL; // Turn off all modes

   // Cursor display
   static std::string HIDE_CURSOR; // Make the cursor invisible
   static std::string SHOW_CURSOR; // Make the cursor visible

   // Terminal size:
   static int COLS; // Width of the terminal (-1 for unknown)
   static int LINES; // Height of the terminal (-1 for unknown)

   // Foreground colors
   static std::string BLACK;
   static std::string BLUE;
   static std::string GREEN;
   static std::string CYAN;
   static std::string RED;
   static std::string MAGENTA;
   static std::string YELLOW;
   static std::string WHITE;

   // Background colors
   static std::string BG_BLACK;
   static std::string BG_BLUE;
   static std::string BG_GREEN;
   static std::string BG_CYAN;
   static std::string BG_RED;
   static std::string BG_MAGENTA;
   static std::string BG_YELLOW;
   static std::string BG_WHITE;

protected:
   static std::list<std::string> STRING_CAPABILITIES;
   static std::list<std::string> COLORS;
   static std::list<std::string> ANSICOLORS;
};

} // lit
} // polar

#endif // POLAR_DEVLTOOLS_LIT_PROGRESS_BAR_H
