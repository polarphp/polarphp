// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/10/10.

#include "polarphp/utils/Signals.h"
#include "polarphp/utils/OptionalError.h"
#include "polarphp/utils/MemoryBuffer.h"
#include "polarphp/utils/ManagedStatics.h"
#include "polarphp/utils/ErrorHandling.h"

#include <filesystem>
#include <vector>
#include <string>
#include <mutex>
#include <iostream>

namespace polar {
namespace utils {

// Callbacks to run in signal handler must be lock-free because a signal handler
// could be running as we add new callbacks. We don't add unbounded numbers of
// callbacks, an array is therefore sufficient.
struct CallbackAndCookie
{
   SignalHandlerCallback m_callback;
   void *m_cookie;
   enum class Status { Empty, Initializing, Initialized, Executing };
   std::atomic<Status> m_flag;
};

static constexpr size_t MaxSignalHandlerCallbacks = 8;
static CallbackAndCookie sg_callBacksToRun[MaxSignalHandlerCallbacks];

// Signal-safe.
void run_signal_handlers()
{
   for (size_t index = 0; index < MaxSignalHandlerCallbacks; ++index) {
      auto &callback = sg_callBacksToRun[index];
      auto expected = CallbackAndCookie::Status::Initialized;
      auto desired = CallbackAndCookie::Status::Executing;
      if (!callback.m_flag.compare_exchange_strong(expected, desired)) {
         continue;
      }
      (*callback.m_callback)(callback.m_cookie);
      callback.m_callback = nullptr;
      callback.m_cookie = nullptr;
      callback.m_flag.store(CallbackAndCookie::Status::Empty);
   }
}

// Signal-safe.
void insert_signal_handler(SignalHandlerCallback funcPtr,
                           void *cookie)
{
   for (size_t index = 0; index < MaxSignalHandlerCallbacks; ++index) {
      auto &callback = sg_callBacksToRun[index];
      auto expected = CallbackAndCookie::Status::Empty;
      auto desired = CallbackAndCookie::Status::Initializing;
      if (!callback.m_flag.compare_exchange_strong(expected, desired))
         continue;
      callback.m_callback = funcPtr;
      callback.m_cookie = cookie;
      callback.m_flag.store(CallbackAndCookie::Status::Initialized);
      return;
   }
   report_fatal_error("too many signal callbacks already registered");
}

} // utils
} // polar
