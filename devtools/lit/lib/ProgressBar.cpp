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

} // lit
} // polar
