// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/10/09.

#include "polarphp/utils/ManagedStatics.h"
#include <mutex>
#include <cassert>
#include <thread>

namespace polar {
namespace utils {

static const ManagedStaticBase *sg_staticList = nullptr;
static std::recursive_mutex *sg_managedStaticMutex = nullptr;
static std::once_flag sg_mutexInitFlag;

namespace {

void initialize_mutex()
{
   sg_managedStaticMutex = new std::recursive_mutex();
}

std::recursive_mutex &get_managed_static_mutex()
{
   std::call_once(sg_mutexInitFlag, initialize_mutex);
   return *sg_managedStaticMutex;
}
} // anoymous namespace

void ManagedStaticBase::registerManagedStatic(void *(*creator)(), void (*deleter)(void *)) const
{
   assert(creator);
   std::lock_guard locker(get_managed_static_mutex());
   if (!m_ptr.load(std::memory_order_relaxed)) {
      void *temp = creator();
      m_ptr.store(temp, std::memory_order_release);
      m_deleterFunc = deleter;
      // Add to list of managed statics.
      m_next = sg_staticList;
      sg_staticList = this;
   }
}

void ManagedStaticBase::destroy() const
{
   assert(m_deleterFunc && "ManagedStatic not initialized correctly!");
   assert(sg_staticList == this &&
          "Not destroyed in reverse order of construction?");
   sg_staticList = m_next;
   m_deleterFunc(m_ptr);
   m_ptr = nullptr;
   m_deleterFunc = nullptr;
}

void managed_statics_shutdown()
{
   //std::lock_guard locker(get_managed_static_mutex());
   while (sg_staticList) {
      sg_staticList->destroy();
   }
}

} // utils
} // polar
