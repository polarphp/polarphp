// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/10/08.

#ifndef POLARPHP_UTILS_OPTIONAL_ERROR_H
#define POLARPHP_UTILS_OPTIONAL_ERROR_H

#include <cassert>
#include <system_error>
#include <type_traits>
#include <utility>
#include "polarphp/utils/AlignOf.h"
#include "polarphp/utils/ErrorCode.h"

namespace polar {
namespace utils {

/// Represents either an error or a value T.
///
/// OptionalError<T> is a pointer-like class that represents the result of an
/// operation. The result is either an error, or a value of type T. This is
/// designed to emulate the usage of returning a pointer where nullptr indicates
/// failure. However instead of just knowing that the operation failed, we also
/// have an error_code and optional user data that describes why it failed.
///
/// It is used like the following.
/// \code
///   OptionalError<Buffer> getBuffer();
///
///   auto buffer = getBuffer();
///   if (error_code ec = buffer.getError())
///     return ec;
///   buffer->write("adena");
/// \endcode
///
///
/// Implicit conversion to bool returns true if there is a usable value. The
/// unary * and -> operators provide pointer like access to the value. Accessing
/// the value when there is an error has undefined behavior.
///
/// When T is a reference type the behavior is slightly different. The reference
/// is held in a std::reference_wrapper<std::remove_reference<T>::type>, and
/// there is special handling to make operator -> work as if T was not a
/// reference.
///
/// T cannot be a rvalue reference.
///
template<typename T>
class OptionalError
{
   template <typename OtherType>
   friend class OptionalError;
   static const bool isRef = std::is_reference<T>::value;
   using WrapRef = std::reference_wrapper<typename std::remove_reference<T>::type>;
public:
   using StorageType = typename std::conditional<isRef, WrapRef, T>::type;
private:
   using Reference = typename std::remove_reference<T>::type &;
   using ConstReference = const typename std::remove_reference<T>::type &;
   using Pointer = typename std::remove_reference<T>::type *;
   using ConstPointer = const typename std::remove_reference<T>::type *;
   using reference = Reference;
   using const_reference = ConstReference;
   using pointer = Pointer;
   using const_pointer = ConstPointer;
public:
   template <typename ErrorType>
   OptionalError(ErrorType error, typename std::enable_if<std::is_error_code_enum<ErrorType>::value ||
                 std::is_error_condition_enum<ErrorType>::value,
                 void *>::type = nullptr)
      : m_hasError(true)
   {
      new (getErrorStorage()) std::error_code(make_error_code(error));
   }

   OptionalError(std::error_code errorCode)
      : m_hasError(true)
   {
      new (getErrorStorage()) std::error_code(errorCode);
   }

   template <class OtherType>
   OptionalError(OtherType &&value,
                 typename std::enable_if<std::is_convertible<OtherType, T>::value>::type
                 * = nullptr)
      : m_hasError(false)
   {
      new (getStorage()) StorageType(std::forward<OtherType>(value));
   }

   OptionalError(const OptionalError &other)
   {
      copyConstruct(other);
   }

   template <class OtherT>
   OptionalError(
         const OptionalError<OtherT> &other,
         typename std::enable_if<std::is_convertible<OtherT, T>::value>::type * =
         nullptr)
   {
      copyConstruct(other);
   }

   template <class OtherT>
   explicit OptionalError(
         const OptionalError<OtherT> &other,
         typename std::enable_if<
         !std::is_convertible<OtherT, const T &>::value>::type * = nullptr)
   {
      copyConstruct(other);
   }

   OptionalError(OptionalError &&other)
   {
      moveConstruct(std::move(other));
   }

   template <class OtherType>
   OptionalError(OptionalError<OtherType> &&other, typename std::enable_if<std::is_convertible<OtherType, T>::value>::type * =
         nullptr)
   {
      moveConstruct(std::move(other));
   }

   // This might eventually need SFINAE but it's more complex than is_convertible
   // & I'm too lazy to write it right now. hahahaaha
   template <class OtherType>
   explicit OptionalError(OptionalError<OtherType> &&other, typename std::enable_if<!std::is_convertible<OtherType, T>::value>::type * =
         nullptr)
   {
      moveConstruct(std::move(other));
   }

   OptionalError &operator=(const OptionalError &other)
   {
      copyAssign(other);
      return *this;
   }

   OptionalError &operator=(OptionalError &&other)
   {
      moveAssign(std::move(other));
      return *this;
   }

   ~OptionalError()
   {
      if (!m_hasError) {
         getStorage()->~StorageType();
      }
   }

   /// Return false if there is an error.
   explicit operator bool() const
   {
      return !m_hasError;
   }

   reference get()
   {
      return *getStorage();
   }

   const_reference get() const
   {
      return const_cast<OptionalError<T> *>(this)->get();
   }

   std::error_code getError() const
   {
      return m_hasError ? *getErrorStorage() : std::error_code();
   }

   pointer operator ->()
   {
      return toPointer(getStorage());
   }

   const_pointer operator->() const
   {
      return toPointer(getStorage());
   }

   reference operator *()
   {
      return *getStorage();
   }

   const_reference operator*() const
   {
      return *getStorage();
   }

private:

   template <typename T1>
   static bool compareThisIfSameType(const T1 &lhs, const T1 &rhs)
   {
      return &lhs == &rhs;
   }

   template <typename T1, typename T2>
   static bool compareThisIfSameType(const T1 &lhs, const T2 &rhs)
   {
      return false;
   }

   template <typename OtherType>
   void copyConstruct(const OptionalError<OtherType> &other)
   {
      if (!other.m_hasError) {
         // Get the other value.
         m_hasError = false;
         new (getStorage()) StorageType(*other.getStorage());
      } else {
         // Get other's error.
         m_hasError = true;
         new (getErrorStorage()) std::error_code(other.getError());
      }
   }

   template <typename OtherType>
   void copyAssign(const OptionalError<OtherType> &other)
   {
      if (compareThisIfSameType(*this, other)) {
         return;
      }
      this->~OptionalError();
      new (this) OptionalError(other);
   }

   template <typename OtherType>
   void moveConstruct(OptionalError<OtherType> &&other)
   {
      if (!other.m_hasError) {
         // Get the other value.
         m_hasError = false;
         new (getStorage()) StorageType(std::move(*other.getStorage()));
      } else {
         // Get other's error.
         m_hasError = true;
         new (getErrorStorage()) std::error_code(other.getError());
      }
   }

   template <typename OtherType>
   void moveAssign(OptionalError<OtherType> &&other)
   {
      if (compareThisIfSameType(*this, other)) {
         return;
      }
      this->~OptionalError();
      new (this) OptionalError(std::move(other));
   }

   Pointer toPointer(Pointer pointer)
   {
      return pointer;
   }

   ConstPointer toPointer(const Pointer pointer) const
   {
      return pointer;
   }

   Pointer toPointer(WrapRef *value)
   {
      return value->get();
   }

   ConstPointer toPointer(const WrapRef *value) const
   {
      return &value->get();
   }

   const StorageType *getStorage() const
   {
      assert(!m_hasError && "Cannot get value when an error exists!");
      return reinterpret_cast<const StorageType *>(m_typeStorage.m_buffer);
   }

   StorageType *getStorage()
   {
      return const_cast<StorageType *>(static_cast<const OptionalError *>(this)->getStorage());
   }

   const std::error_code *getErrorStorage() const
   {
      assert(m_hasError && "Cannot get error when a value exists!");
      return reinterpret_cast<const std::error_code *>(m_errorStorage.m_buffer);
   }

   std::error_code *getErrorStorage()
   {
      return const_cast<std::error_code *>(static_cast<const OptionalError *>(this)->getErrorStorage());
   }
private:
   union {
      polar::utils::AlignedCharArrayUnion<StorageType> m_typeStorage;
      polar::utils::AlignedCharArrayUnion<std::error_code> m_errorStorage;
   };
   bool m_hasError : 1;
};

template <class T, class E>
typename std::enable_if<std::is_error_code_enum<E>::value ||
std::is_error_condition_enum<E>::value,
bool>::type
operator==(const OptionalError<T> &error, E code)
{
   return error.getError() == code;
}

} // utils
} // polar

#endif // POLARPHP_UTILS_OPTIONAL_ERROR_H
