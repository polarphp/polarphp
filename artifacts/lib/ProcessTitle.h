// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/13.

#ifndef POLARPHP_ARTIFACTS_PROCESS_TITLE_H
#define POLARPHP_ARTIFACTS_PROCESS_TITLE_H

namespace polar {

#define PS_TITLE_SUCCESS 0
#define PS_TITLE_NOT_AVAILABLE 1
#define PS_TITLE_NOT_INITIALIZED 2
#define PS_TITLE_BUFFER_NOT_AVAILABLE 3
#define PS_TITLE_WINDOWS_ERROR 4

char **save_ps_args(int argc, char **argv);
int set_ps_title(const char *newStr);
int get_ps_title(int *displen, const char **string);
const char *ps_title_errno(int rc);
int is_ps_title_available();
void cleanup_ps_args(char **argv);

} // polar

#endif // POLARPHP_ARTIFACTS_PROCESS_TITLE_H
