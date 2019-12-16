//===--- Program.h ----------------------------------------------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/12/02.


#ifndef POLARPHP_BASIC_PROGRAM_H
#define POLARPHP_BASIC_PROGRAM_H

namespace polar {

/// This function executes the program using the arguments provided,
/// preferring to reexecute the current process, if supported.
///
/// \param Program Path of the program to be executed
/// \param args A vector of strings that are passed to the program. The first
/// element should be the name of the program. The list *must* be terminated by
/// a null char * entry.
/// \param env An optional vector of strings to use for the program's
/// environment. If not provided, the current program's environment will be
/// used.
///
/// \returns Typically, this function will not return, as the current process
/// will no longer exist, or it will call exit() if the program was successfully
/// executed. In the event of an error, this function will return a negative
/// value indicating a failure to execute.
int execute_in_place(const char *program, const char **args,
                     const char **env = nullptr);

} // polar

#endif // POLARPHP_BASIC_PROGRAM_H
