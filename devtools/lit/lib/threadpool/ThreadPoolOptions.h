// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/09/10.

#ifndef POLAR_DEVLTOOLS_LIT_THREAD_POOL_POOL_OPTIONS_H
#define POLAR_DEVLTOOLS_LIT_THREAD_POOL_POOL_OPTIONS_H

#include <algorithm>
#include <thread>

namespace polar {
namespace lit {
namespace threadpool {

/**
 * @brief The ThreadPoolOptions class provides creation options for
 * ThreadPool.
 */
class ThreadPoolOptions
{
public:
   /**
     * @brief ThreadPoolOptions Construct default options for thread pool.
     */
   ThreadPoolOptions();

   /**
     * @brief setThreadCount Set thread count.
     * @param count Number of threads to be created.
     */
   void setThreadCount(size_t count);

   /**
     * @brief setQueueSize Set single worker queue size.
     * @param count Maximum length of queue of single worker.
     */
   void setQueueSize(size_t size);

   /**
     * @brief threadCount Return thread count.
     */
   size_t getThreadCount() const;

   /**
     * @brief queueSize Return single worker queue size.
     */
   size_t getQueueSize() const;

private:
   size_t m_threadCount;
   size_t m_queueSize;
};

/// Implementation

inline ThreadPoolOptions::ThreadPoolOptions()
   : m_threadCount(std::max<size_t>(1u, std::thread::hardware_concurrency()))
   , m_queueSize(1024u)
{
}

inline void ThreadPoolOptions::setThreadCount(size_t count)
{
   m_threadCount = std::max<size_t>(1u, count);
}

inline void ThreadPoolOptions::setQueueSize(size_t size)
{
   m_queueSize = std::max<size_t>(1u, size);
}

inline size_t ThreadPoolOptions::getThreadCount() const
{
   return m_threadCount;
}

inline size_t ThreadPoolOptions::getQueueSize() const
{
   return m_queueSize;
}

} // threadpool
} // lit
} // polar

#endif // POLAR_DEVLTOOLS_LIT_THREAD_POOL_POOL_OPTIONS_H
