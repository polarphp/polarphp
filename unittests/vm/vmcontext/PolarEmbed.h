// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/28.

#ifndef POLARPHP_UNITTEST_VM_VMCONTEXT_POLAR_EMBED_H
#define POLARPHP_UNITTEST_VM_VMCONTEXT_POLAR_EMBED_H

namespace polar {
namespace unittest {

bool begin_vm_context(int argc, char *argv[]);
void end_vm_context();

} // unittest
} // polar

#endif // POLARPHP_UNITTEST_VM_VMCONTEXT_POLAR_EMBED_H
