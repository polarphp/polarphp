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

#ifndef POLAR_DEVLTOOLS_LIT_THREAD_POOL_FIXED_FUNCTION_H
#define POLAR_DEVLTOOLS_LIT_THREAD_POOL_FIXED_FUNCTION_H

#include <type_traits>
#include <cstring>
#include <stdexcept>
#include <utility>

namespace polar {
namespace lit {
namespace threadpool {

/**
 * @brief The FixedFunction<R(ARGS...), STORAGE_SIZE> class implements
 * functional object.
 * This function is analog of 'std::function' with limited capabilities:
 *  - It supports only move semantics.
 *  - The size of functional objects is limited to storage size.
 * Due to limitations above it is much faster on creation and copying than
 * std::function.
 */
template <typename SIGNATURE, size_t STORAGE_SIZE = 128>
class FixedFunction;

template <typename R, typename... ARGS, size_t STORAGE_SIZE>
class FixedFunction<R(ARGS...), STORAGE_SIZE>
{
   using FuncPtrType = R (*)(ARGS...);
   public:
   FixedFunction()
      : m_functionPtr(nullptr),
        m_methodPtr(nullptr),
        m_allocPtr(nullptr)
   {
   }

   /**
     * @brief FixedFunction Constructor from functional object.
     * @param object Functor object will be stored in the internal storage
     * using move constructor. Unmovable objects are prohibited explicitly.
     */
   template <typename FUNC>
   FixedFunction(FUNC&& object)
      : FixedFunction()
   {
      using UnrefType = typename std::remove_reference<FUNC>::type ;

      static_assert(sizeof(UnrefType) < STORAGE_SIZE,
                    "functional object doesn't fit into internal storage");
      static_assert(std::is_move_constructible<UnrefType>::value,
                    "Should be of movable type");

      m_methodPtr = [](
            void *objectPtr, FuncPtrType, ARGS... args) -> R
      {
         return static_cast<UnrefType*>(objectPtr)
               ->
               operator()(args...);
      };

      m_allocPtr = [](void *storagePtr, void *objectPtr)
      {
         if(objectPtr)
         {
            UnrefType* x_object = static_cast<UnrefType*>(objectPtr);
            new(storagePtr) UnrefType(std::move(*x_object));
         }
         else
         {
            static_cast<UnrefType*>(storagePtr)->~UnrefType();
         }
      };

      m_allocPtr(&m_storage, &object);
   }

   /**
     * @brief FixedFunction Constructor from free function or static member.
     */
   template <typename RET, typename... PARAMS>
   FixedFunction(RET (*funcPtr)(PARAMS...))
      : FixedFunction()
   {
      m_functionPtr = funcPtr;
      m_methodPtr = [](void*, FuncPtrType funcPtr, ARGS... args) -> R
      {
         return static_cast<RET (*)(PARAMS...)>(funcPtr)(args...);
      };
   }

   FixedFunction(FixedFunction&& o) : FixedFunction()
   {
      moveFromOther(o);
   }

   FixedFunction& operator=(FixedFunction&& o)
   {
      moveFromOther(o);
      return *this;
   }

   ~FixedFunction()
   {
      if(m_allocPtr) {
         m_allocPtr(&m_storage, nullptr);
      }
   }

   /**
     * @brief operator () Execute stored functional object.
     * @throws std::runtime_error if no functional object is stored.
     */
   R operator()(ARGS... args)
   {
      if(!m_methodPtr) {
         throw std::runtime_error("call of empty functor");
      }
      return m_methodPtr(&m_storage, m_functionPtr, args...);
   }

   private:
   FixedFunction& operator=(const FixedFunction&) = delete;
   FixedFunction(const FixedFunction&) = delete;

   union
   {
      typename std::aligned_storage<STORAGE_SIZE, sizeof(size_t)>::type
            m_storage;
      FuncPtrType m_functionPtr;
   };

   using MethodType = R (*)(
   void *objectPtr, FuncPtrType freeFuncPtr, ARGS... args);
   MethodType m_methodPtr;

   using AllocType = void (*)(void *storagePtr, void *objectPtr);
   AllocType m_allocPtr;

   void moveFromOther(FixedFunction &other)
   {
      if(this == &other) {
         return;
      }

      if(m_allocPtr) {
         m_allocPtr(&m_storage, nullptr);
         m_allocPtr = nullptr;
      } else {
         m_functionPtr = nullptr;
      }

      m_methodPtr = other.m_methodPtr;
      other.m_methodPtr = nullptr;

      if(other.m_allocPtr) {
         m_allocPtr = other.m_allocPtr;
         m_allocPtr(&m_storage, &other.m_storage);
      } else {
         m_functionPtr = other.m_functionPtr;
      }
   }
};

} // threadpool
} // lit
} // polar

#endif // POLAR_DEVLTOOLS_LIT_THREAD_POOL_FIXED_FUNCTION_H
