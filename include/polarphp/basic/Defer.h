//===--- Defer.h - 'defer' helper macro -------------------------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
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
// Created by polarboy on 2019/05/05.
//===----------------------------------------------------------------------===//
//
// This file defines a 'POLAR_DEFER' macro for performing a cleanup on any exit
// out of a scope.
//
//===----------------------------------------------------------------------===//

#ifndef POLAR_BASIC_DEFER_H
#define POLAR_BASIC_DEFER_H

#include <type_traits>

namespace polar {

template <typename F>
class DoAtScopeExit
{
   F m_func;
   void operator=(DoAtScopeExit&) = delete;
public:
   DoAtScopeExit(F &&func)
      : m_func(std::move(func))
   {}

   ~DoAtScopeExit()
   {
      m_func();
   }
};

namespace internal
{
struct DeferTask
{};
template<typename F>
DoAtScopeExit<typename std::decay<F>::type> operator+(DeferTask, F&& func)
{
   return DoAtScopeExit<typename std::decay<F>::type>(std::move(func));
}
} // internal


#define DEFER_CONCAT_IMPL(x, y) x##y
#define DEFER_MACRO_CONCAT(x, y) DEFER_CONCAT_IMPL(x, y)

/// This macro is used to register a function / lambda to be run on exit from a
/// scope.  Its typical use looks like:
///
///   POLAR_DEFER {
///     stuff
///   };
///
#define POLAR_DEFER                                                            \
   auto DEFER_MACRO_CONCAT(defer_func, __COUNTER__) =                           \
   ::polar::internal::DeferTask() + [&]()

} // polar

#endif // POLAR_BASIC_DEFER_H
