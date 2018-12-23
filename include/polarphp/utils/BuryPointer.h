// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/05.

#ifndef POLARPHP_UTILS_BURY_POINTER_H
#define POLARPHP_UTILS_BURY_POINTER_H

#include <memory>

namespace polar {
namespace utils {

// In tools that will exit soon anyway, going through the process of explicitly
// deallocating resources can be unnecessary - better to leak the resources and
// let the OS clean them up when the process ends. Use this function to ensure
// the memory is not misdiagnosed as an unintentional leak by leak detection
// tools (this is achieved by preserving pointers to the object in a globally
// visible array).
void bury_pointer(const void *ptr);
template <typename T> void bury_pointer(std::unique_ptr<T> ptr)
{
   bury_pointer(ptr.release());
}

} // utils
} // polar

#endif // POLARPHP_UTILS_BURY_POINTER_H
