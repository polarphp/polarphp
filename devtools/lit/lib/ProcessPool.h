// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://polarphp.org/LICENSE.txt for license information
// See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/09/04.

#ifndef POLAR_DEVLTOOLS_LIT_PROCESS_POOL_H
#define POLAR_DEVLTOOLS_LIT_PROCESS_POOL_H

#include <functional>

namespace polar {
namespace lit {

class ProcessPool
{
public:
   template <typename FuncType, typename... ArgTypes>
   ProcessPool(int jobs = 0, FuncType func = nullptr, ArgTypes&&... args)
   {

   }
   void apply();
   void applyAsync();
   void join();
   void close();
   void terminate();
};

class AsyncResult
{
public:
   void wait(int timeout);
   bool ready();
};

} // lit
} // polar

#endif // POLAR_DEVLTOOLS_LIT_PROCESS_POOL_H
