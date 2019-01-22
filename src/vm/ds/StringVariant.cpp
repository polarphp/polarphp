// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/24.

#include "polarphp/vm/ds/StringVariant.h"
#include "polarphp/vm/ds/internal/VariantPrivate.h"
#include "polarphp/vm/ds/ArrayItemProxy.h"
#include "polarphp/runtime/RtDefs.h"

#include <cstring>
#include <stdexcept>
#include <list>

namespace polar {
namespace vmapi {

using internal::VariantPrivate;

#define STR_VARIANT_OVERHEAD   (ZEND_MM_OVERHEAD + _ZSTR_HEADER_SIZE + 1)
#define STR_VARIANT_START_SIZE 256
#define STR_VARIANT_START_LEN  (STR_VARIANT_START_SIZE - STR_VARIANT_OVERHEAD)
#define STR_VARIANT_PAGE       4096

StringVariant::StringVariant()
{
   zval *self = getUnDerefZvalPtr();
   Z_STR_P(self) = nullptr;
   Z_TYPE_INFO_P(self) = IS_STRING_EX;
}

StringVariant::StringVariant(const Variant &other)
{
   // chech the type, if the type is not the string, we just try to convert
   // if the type is string we deploy copy on write idiom
   zval *from = const_cast<zval *>(other.getZvalPtr());
   zval *self = getUnDerefZvalPtr();
   if (other.getType() == Type::String) {
      ZVAL_COPY(self, from);
   } else {
      zval temp;
      // will increase 1 to gc refcount
      ZVAL_DUP(&temp, from);
      // will decrease 1 to gc refcount
      convert_to_string(&temp);
      ZVAL_COPY_VALUE(self, &temp);
   }
   // we just set the capacity equal to default allocator algorithm
   setCapacity(ZEND_MM_ALIGNED_SIZE(_ZSTR_STRUCT_SIZE(Z_STRLEN_P(self))));
}

///
/// Review here wether copy under zend_string or just c++ object
/// TODO
///
StringVariant::StringVariant(const StringVariant &other)
   : StringVariant(other, false)
{
}

StringVariant::StringVariant(StringVariant &other, bool isRef)
{
   zval *self = getUnDerefZvalPtr();
   if (!isRef) {
      /// we don't pass through refrence relationship
      zval *otherPtr = other.getZvalPtr();
      stdCopyZval(self, otherPtr);
   } else {
      ZEND_ASSERT(!ZSTR_IS_INTERNED(Z_STR_P(other.getZvalPtr())));
      zval *source = other.getUnDerefZvalPtr();
      if (!Z_OPT_ISREF_P(source)) {
         ZVAL_MAKE_REF(source);
      }
      ZVAL_COPY(self, source);
   }
   setCapacity(other.getCapacity());
}

StringVariant::StringVariant(Variant &&other)
   : Variant(std::move(other))
{
   if (getType() != Type::String) {
      convert_to_string(getUnDerefZvalPtr());
   }
   setCapacity(Z_STRLEN_P(getZvalPtr()));
}

StringVariant::StringVariant(StringVariant &&other) noexcept
   : Variant(std::move(other))
{}

StringVariant::StringVariant(const std::string &value)
   : StringVariant(value.c_str(), value.size())
{}

StringVariant::StringVariant(const char *value, size_t length)
{
   zval *self = getZvalPtr();
   ZVAL_NEW_STR(self, zend_string_init(value, length, 0));
   ZSTR_CAPACITY(Z_STR_P(self)) = length;
}

StringVariant::StringVariant(const char *value)
   : StringVariant(value, std::strlen(value))
{}

StringVariant::StringVariant(zval &other, bool isRef)
   : StringVariant(&other, isRef)
{}

StringVariant::StringVariant(zval &&other, bool isRef)
   : StringVariant(&other, isRef)
{}

StringVariant::StringVariant(zval *other, bool isRef)
{
   /// we don't accept refence to interned string
   ZEND_ASSERT(!isRef || !(Z_OPT_ISREF_P(other) && Z_TYPE_P(Z_REFVAL_P(other)) == IS_STRING &&
                           ZSTR_IS_INTERNED(Z_STR_P(Z_REFVAL_P(other)))));
   zval *self = getUnDerefZvalPtr();
   if (nullptr != other && Z_TYPE_P(other) != IS_NULL) {
      if ((Z_TYPE_P(other) == IS_STRING) ||
          (Z_OPT_ISREF_P(other) && Z_TYPE_P(Z_REFVAL_P(other)) == IS_STRING)) {
         if (Z_OPT_ISREF_P(other)) {
            if (isRef) {
               ZVAL_COPY(self, other);
            } else {
               ZVAL_COPY(self, Z_REFVAL_P(other));
            }
         } else {
            if (ZSTR_IS_INTERNED(Z_STR_P(other))) {
               ZVAL_COPY_VALUE(self, other);
            } else {
               if (isRef) {
                  // refcount > 1
                  if (Z_REFCOUNT_P(other) > 1) {
                     SEPARATE_STRING(other);
                  }
                  ZVAL_MAKE_REF(other);
                  ZVAL_COPY(self, other);
               } else {
                  ZVAL_COPY(self, other);
               }
            }
         }
      } else {
         /// not string type, we don't consider isRef = true
         ZVAL_DUP(self, other);
         convert_to_string(self);
      }
      setCapacity(Z_STRLEN_P(getZvalPtr()));
   } else {
      ZVAL_EMPTY_STRING(self);
   }
}

///
/// we don't pass through reference relationship
///
StringVariant &StringVariant::operator =(const StringVariant &other)
{
   if (this != &other) {
      zval *self = getZvalPtr();
      if (nullptr != getZendStringPtr() && !ZSTR_IS_INTERNED(Z_STR_P(self))) {
         zval_ptr_dtor_str(self);
      }
      ZVAL_COPY(self, other.getZvalPtr());
      setCapacity(other.getSize());
   }
   return *this;
}

///
/// we don't pass through reference relationship
///
StringVariant &StringVariant::operator =(const Variant &other)
{
   zval *self = getZvalPtr();
   if (nullptr != getZendStringPtr() && !ZSTR_IS_INTERNED(Z_STR_P(self))) {
      zval_ptr_dtor_str(self);
   }
   if (other.getType() == Type::String) {
      ZVAL_COPY(self, other.getZvalPtr());
   } else {
      zval *from = const_cast<zval *>(other.getZvalPtr());
      zval temp;
      ZVAL_DUP(&temp, from);
      convert_to_string(&temp);
      ZVAL_COPY_VALUE(self, &temp);
   }
   setCapacity(getSize());
   return *this;
}

StringVariant &StringVariant::operator =(ArrayItemProxy &&other)
{
   return operator =(other.toStringVariant());
}

StringVariant &StringVariant::operator =(char value)
{
   ValueType buffer[2] = {value, '\0'};
   return operator =(buffer);
}

StringVariant &StringVariant::operator =(const std::string &value)
{
   return operator =(value.c_str());
}

StringVariant &StringVariant::operator =(const char *value)
{
   zval *self = getZvalPtr();
   if (nullptr != getZendStringPtr() && !ZSTR_IS_INTERNED(Z_STR_P(self))) {
      zval_ptr_dtor_str(self);
   }
   size_t len = std::strlen(value);
   ZVAL_NEW_STR(self, zend_string_init(value, len, 0));
   setCapacity(len);
   return *this;
}

StringVariant &StringVariant::operator +=(const char *str)
{
   return append(str);
}

StringVariant &StringVariant::operator +=(const char c)
{
   return append(c);
}

StringVariant &StringVariant::operator +=(const std::string &str)
{
   return append(str);
}

StringVariant &StringVariant::operator +=(const StringVariant &str)
{
   return append(str);
}

StringVariant::Reference StringVariant::operator [](size_t pos)
{
   return const_cast<Reference>(static_cast<const StringVariant &>(*this).operator [](pos));
}

StringVariant::ConstReference StringVariant::operator [](size_t pos) const
{
   Pointer dataPtr = getRawStrPtr();
   return dataPtr[pos];
}

bool StringVariant::operator !=(const char *other) const noexcept
{
   return 0 != std::memcmp(getCStr(), other, std::max(getLength(), std::strlen(other)));
}

bool StringVariant::operator !=(const std::string &other) const noexcept
{
   return 0 != std::memcmp(getCStr(), other.c_str(), std::max(getLength(), other.length()));
}

bool StringVariant::operator !=(const StringVariant &other) const noexcept
{
   return 0 != std::memcmp(getCStr(), other.getCStr(), std::max(getLength(), other.getLength()));
}

bool StringVariant::operator ==(const char *other) const noexcept
{
   return 0 == std::memcmp(getCStr(), other, std::max(getLength(), std::strlen(other)));
}

bool StringVariant::operator ==(const std::string &other) const noexcept
{
   return 0 == std::memcmp(getCStr(), other.c_str(), std::max(getLength(), other.length()));
}

bool StringVariant::operator ==(const StringVariant &other) const noexcept
{
   return 0 == std::memcmp(getCStr(), other.getCStr(), std::max(getLength(), other.getLength()));
}

bool StringVariant::operator <(const char *other) const noexcept
{
   return std::memcmp(getCStr(), other, std::max(getLength(), std::strlen(other))) < 0;
}

bool StringVariant::operator <(const std::string &other) const noexcept
{
   return std::memcmp(getCStr(), other.c_str(), std::max(getLength(), other.length())) < 0;
}

bool StringVariant::operator <(const StringVariant &other) const noexcept
{
   return std::memcmp(getCStr(), other.getCStr(), std::max(getLength(), other.getLength())) < 0;
}

bool StringVariant::operator <=(const char *other) const noexcept
{
   size_t len = std::max(getLength(), std::strlen(other));
   return std::memcmp(getCStr(), other, len) < 0||
         0 == std::memcmp(getCStr(),  other, len);
}

bool StringVariant::operator <=(const std::string &other) const noexcept
{
   size_t len = std::max(getLength(), other.length());
   return std::memcmp(getCStr(), other.c_str(), len) < 0||
         0 == std::memcmp(getCStr(),  other.c_str(), len);
}

bool StringVariant::operator <=(const StringVariant &other) const noexcept
{
   size_t len = std::max(getLength(), other.getLength());
   return std::memcmp(getCStr(), other.getCStr(), len) < 0 ||
         0 == std::memcmp(getCStr(),  other.getCStr(), len);
}

bool StringVariant::operator >(const char *other) const noexcept
{
   return std::memcmp(getCStr(), other, std::max(getLength(), std::strlen(other))) > 0;
}

bool StringVariant::operator >(const std::string &other) const noexcept
{
   return std::memcmp(getCStr(), other.c_str(), std::max(getLength(), other.length())) > 0;
}

bool StringVariant::operator >(const StringVariant &other) const noexcept
{
   return std::memcmp(getCStr(), other.getCStr(), std::max(getLength(), other.getLength())) > 0;
}

bool StringVariant::operator >=(const char *other) const noexcept
{
   size_t len = std::max(getLength(), std::strlen(other));
   return std::memcmp(getCStr(), other, len) > 0||
         0 == std::memcmp(getCStr(),  other, len);
}

bool StringVariant::operator >=(const std::string &other) const noexcept
{
   size_t len = std::max(getLength(), other.length());
   return std::memcmp(getCStr(), other.c_str(), len) > 0||
         0 == std::memcmp(getCStr(),  other.c_str(), len);
}

bool StringVariant::operator >=(const StringVariant &other) const noexcept
{
   size_t len = std::max(getLength(), other.getLength());
   return std::memcmp(getCStr(), other.getCStr(), len) > 0 ||
         0 == std::memcmp(getCStr(),  other.getCStr(), len);
}

bool StringVariant::toBoolean() const noexcept
{
   return getType() != Type::Null && 0 != Z_STRLEN_P(const_cast<zval *>(getZvalPtr()));
}

std::string StringVariant::toString() const noexcept
{
   zend_string *str = getZendStringPtr();
   if (!str) {
      return std::string();
   }
   return std::string(str->val, str->len);
}

std::string StringVariant::toLowerCase() const
{
   if (isEmpty()) {
      return std::string();
   }
   std::string ret(cbegin(), cend());
   return str_tolower(ret);
}

std::string StringVariant::toUpperCase() const
{
   if (isEmpty()) {
      return std::string();
   }
   std::string ret(cbegin(), cend());
   return str_toupper(ret);
}

StringVariant::Reference StringVariant::at(SizeType pos)
{
   return const_cast<StringVariant::Reference>(const_cast<const StringVariant &>(*this).at(pos));
}

StringVariant::ConstReference StringVariant::at(SizeType pos) const
{
   if (pos >= getSize()) {
      throw std::out_of_range("string pos out of range");
   }
   char *str = getRawStrPtr();
   return str[pos];
}

StringVariant::Iterator StringVariant::begin() noexcept
{
   return getRawStrPtr();
}

StringVariant::ConstrIterator StringVariant::begin() const noexcept
{
   return getCStr();
}

StringVariant::ConstrIterator StringVariant::cbegin() const noexcept
{
   return getCStr();
}

StringVariant::ReverseIterator StringVariant::rbegin() noexcept
{
   return ReverseIterator(getRawStrPtr() + getLength());
}

StringVariant::ConstReverseIterator StringVariant::rbegin() const noexcept
{
   return ConstReverseIterator(getCStr() + getLength());
}

StringVariant::ConstReverseIterator StringVariant::crbegin() const noexcept
{
   return ConstReverseIterator(getCStr() + getLength());
}

StringVariant::Iterator StringVariant::end() noexcept
{
   return getRawStrPtr() + getLength();
}

StringVariant::ConstrIterator StringVariant::end() const noexcept
{
   return getCStr() + getLength();
}

StringVariant::ConstrIterator StringVariant::cend() const noexcept
{
   return getCStr() + getLength();
}

StringVariant::ReverseIterator StringVariant::rend() noexcept
{
   return ReverseIterator(getRawStrPtr());
}

StringVariant::ConstReverseIterator StringVariant::rend() const noexcept
{
   return ConstReverseIterator(getCStr());
}

StringVariant::ConstReverseIterator StringVariant::crend() const noexcept
{
   return ConstReverseIterator(getCStr());
}

vmapi_long StringVariant::indexOf(const char *needle, vmapi_long offset,
                                  bool caseSensitive) const noexcept
{
   if (isEmpty()) {
      return -1;
   }
   Pointer haystack = getRawStrPtr();
   size_t haystackLength = getSize();
   size_t needleLength = std::strlen(needle);
   GuardValuePtrType haystackDup(nullptr, std_php_memory_deleter);
   GuardValuePtrType needleDup(nullptr, std_php_memory_deleter);
   ConstPointer found = nullptr;
   if (offset < 0) {
      offset += haystackLength;
   }
   if (offset < 0 || static_cast<size_t>(offset) > haystackLength) {
      // TODO need throw exception here or return -1 ?
      return -1;
   }
   if (0 == needleLength) {
      return -1;
   }
   if (!caseSensitive) {
      haystackDup.reset(static_cast<Pointer>(emalloc(haystackLength)));
      needleDup.reset(static_cast<Pointer>(emalloc(needleLength)));
      std::memcpy(haystackDup.get(), haystack, haystackLength);
      std::memcpy(needleDup.get(), needle, needleLength);
      haystack = haystackDup.get();
      needle = needleDup.get();
      str_tolower(haystackDup.get(), haystackLength);
      str_tolower(needleDup.get(), needleLength);
   }
   found = zend_memnstr(haystack + offset, needle, needleLength,
                        haystack + haystackLength);
   if (nullptr != found) {
      return found - haystack;
   }
   return -1;
}

vmapi_long StringVariant::indexOf(const StringVariant &needle, vmapi_long offset,
                                  bool caseSensitive) const noexcept
{
   return indexOf(needle.getCStr(), offset, caseSensitive);
}

vmapi_long StringVariant::indexOf(const std::string &needle, vmapi_long offset,
                                  bool caseSensitive) const noexcept
{
   return indexOf(needle.c_str(), offset, caseSensitive);
}

vmapi_long StringVariant::indexOf(const char needle, vmapi_long offset,
                                  bool caseSensitive) const noexcept
{
   ValueType buffer[2] = {needle, '\0'};
   return indexOf(reinterpret_cast<Pointer>(buffer), offset, caseSensitive);
}

vmapi_long StringVariant::lastIndexOf(const char *needle, vmapi_long offset,
                                      bool caseSensitive) const noexcept
{
   if (isEmpty()) {
      return -1;
   }
   Pointer haystack = getRawStrPtr();
   size_t haystackLength = getSize();
   size_t needleLength = std::strlen(needle);
   GuardValuePtrType haystackDup(nullptr, std_php_memory_deleter);
   GuardValuePtrType needleDup(nullptr, std_php_memory_deleter);
   Pointer p = nullptr;
   Pointer e = nullptr;
   ConstPointer found = nullptr;
   if (!caseSensitive) {
      haystackDup.reset(static_cast<Pointer>(emalloc(haystackLength)));
      needleDup.reset(static_cast<Pointer>(emalloc(needleLength)));
      std::memcpy(haystackDup.get(), haystack, haystackLength);
      std::memcpy(needleDup.get(), needle, needleLength);
      haystack = haystackDup.get();
      needle = needleDup.get();
      str_tolower(haystackDup.get(), haystackLength);
      str_tolower(needleDup.get(), needleLength);
   }
   if (offset >= 0) {
      if (static_cast<size_t>(offset) > haystackLength) {
         // here we do like php
         php_error_docref(NULL, E_WARNING, "Offset is greater than the length of haystack string");
         return -1;
      }
      p = haystack + static_cast<size_t>(offset);
      e = haystack + haystackLength;
   } else {
      if (offset < -INT_MAX || static_cast<size_t>(-offset) > haystackLength) {
         php_error_docref(NULL, E_WARNING, "Offset is greater than the length of haystack string");
         return -1;
      }
      p = haystack;
      // find the best end pos
      if (static_cast<size_t>(-offset) < needleLength) {
         e = haystack + haystackLength;
      } else {
         e = haystack + haystackLength + offset + needleLength;
      }
   }
   found = zend_memnrstr(p, needle, needleLength, e);
   if (nullptr != found) {
      return found - haystack;
   }
   return -1;
}

vmapi_long StringVariant::lastIndexOf(const StringVariant &needle, vmapi_long offset,
                                      bool caseSensitive) const noexcept
{
   return lastIndexOf(needle.getCStr(), offset, caseSensitive);
}

vmapi_long StringVariant::lastIndexOf(const std::string &needle, vmapi_long offset,
                                      bool caseSensitive) const noexcept
{
   return lastIndexOf(needle.c_str(), offset, caseSensitive);
}

vmapi_long StringVariant::lastIndexOf(const char needle, vmapi_long offset,
                                      bool caseSensitive) const noexcept
{
   ValueType buffer[2] = {needle, '\0'};
   return lastIndexOf(reinterpret_cast<Pointer>(buffer), offset, caseSensitive);
}

bool StringVariant::contains(const StringVariant &needle, bool caseSensitive) const noexcept
{
   return -1 != indexOf(needle.getCStr(), 0, caseSensitive);
}

bool StringVariant::contains(const char *needle, bool caseSensitive) const noexcept
{
   return -1 != indexOf(needle, 0, caseSensitive);
}

bool StringVariant::contains(const std::string &needle, bool caseSensitive) const noexcept
{
   return -1 != indexOf(needle, 0, caseSensitive);
}

bool StringVariant::contains(const char needle, bool caseSensitive) const noexcept
{
   return -1 != indexOf(needle, 0, caseSensitive);
}

bool StringVariant::startsWith(const StringVariant &str, bool caseSensitive) const noexcept
{
   return startsWith(str.getCStr(), caseSensitive);
}

bool StringVariant::startsWith(const char *str, bool caseSensitive) const noexcept
{
   if (isEmpty()) {
      return false;
   }
   Pointer selfStr = getRawStrPtr();
   Pointer otherStrPtr = const_cast<Pointer>(str);
   GuardValuePtrType selfStrLowerCase(nullptr, std_php_memory_deleter);
   GuardValuePtrType otherStrLowerCase(nullptr, std_php_memory_deleter);
   size_t selfStrLength = getSize();
   size_t otherStrLength = std::strlen(str);
   if (selfStrLength < otherStrLength) {
      return false;
   }
   if (!caseSensitive) {
      selfStrLowerCase.reset(static_cast<Pointer>(emalloc(selfStrLength)));
      otherStrLowerCase.reset(static_cast<Pointer>(emalloc(otherStrLength)));
      std::memcpy(selfStrLowerCase.get(), selfStr, selfStrLength);
      std::memcpy(otherStrLowerCase.get(), otherStrPtr, otherStrLength);
      selfStr = selfStrLowerCase.get();
      otherStrPtr = otherStrLowerCase.get();
      str_tolower(selfStr);
      str_tolower(otherStrPtr);
   }
   return 0 == std::memcmp(selfStr, otherStrPtr, otherStrLength);
}

bool StringVariant::startsWith(const std::string &str, bool caseSensitive) const noexcept
{
   return startsWith(str.c_str(), caseSensitive);
}

bool StringVariant::startsWith(char c, bool caseSensitive) const noexcept
{
   ValueType buffer[2] = {c, '\0'};
   return startsWith(reinterpret_cast<Pointer>(buffer), caseSensitive);
}

bool StringVariant::endsWith(const StringVariant &str, bool caseSensitive) const noexcept
{
   return endsWith(str.getCStr(), caseSensitive);
}

bool StringVariant::endsWith(const char *str, bool caseSensitive) const noexcept
{
   if (isEmpty()) {
      return false;
   }
   Pointer selfStr = getRawStrPtr();
   Pointer otherStrPtr = const_cast<Pointer>(str);
   GuardValuePtrType selfStrLowerCase(nullptr, std_php_memory_deleter);
   GuardValuePtrType otherStrLowerCase(nullptr, std_php_memory_deleter);
   size_t selfStrLength = getSize();
   size_t otherStrLength = std::strlen(str);
   if (selfStrLength < otherStrLength) {
      return false;
   }
   if (!caseSensitive) {
      selfStrLowerCase.reset(static_cast<Pointer>(emalloc(selfStrLength)));
      otherStrLowerCase.reset(static_cast<Pointer>(emalloc(otherStrLength)));
      std::memcpy(selfStrLowerCase.get(), selfStr, selfStrLength);
      std::memcpy(otherStrLowerCase.get(), otherStrPtr, otherStrLength);
      selfStr = selfStrLowerCase.get();
      otherStrPtr = otherStrLowerCase.get();
      str_tolower(selfStr);
      str_tolower(otherStrPtr);
   }
   return 0 == std::memcmp(selfStr + selfStrLength - otherStrLength, otherStrPtr, otherStrLength);
}

bool StringVariant::endsWith(const std::string &str, bool caseSensitive) const noexcept
{
   return endsWith(str.c_str(), caseSensitive);
}

bool StringVariant::endsWith(char c, bool caseSensitive) const noexcept
{
   ValueType buffer[2] = {c, '\0'};
   return endsWith(reinterpret_cast<Pointer>(buffer), caseSensitive);
}

StringVariant &StringVariant::prepend(const char *str)
{
   zval *self = getUnDerefZvalPtr();
   size_t newLength;
   size_t length = std::strlen(str);
   size_t requestLength = length + getLength();
   zend_string *destStrPtr = getZendStringPtr();
   if (!Z_OPT_ISREF_P(self)) {
      if (destStrPtr && !ZSTR_IS_INTERNED(destStrPtr)) {
         SEPARATE_STRING(self);
      }
   }
   self = getZvalPtr();
   destStrPtr = getZendStringPtr();
   size_t origLength = getSize();
   newLength = strReAlloc(destStrPtr, requestLength, 0);
   Pointer newRawStr = ZSTR_VAL(destStrPtr);
   // copy backward
   while (origLength--) {
      *(newRawStr + origLength + length) = *(newRawStr + origLength);
   }
   // copy prepend
   std::memcpy(newRawStr, str, length);
   // set self state
   ZSTR_VAL(destStrPtr)[newLength] = '\0';
   ZSTR_LEN(destStrPtr) = newLength;
   Z_STR_P(self) = destStrPtr;
   return *this;
}

StringVariant &StringVariant::prepend(const char c)
{
   ValueType buffer[2] = {c, '\0'};
   return prepend(reinterpret_cast<Pointer>(buffer));
}

StringVariant &StringVariant::prepend(const std::string &str)
{
   return prepend(str.c_str());
}

StringVariant &StringVariant::prepend(const StringVariant &str)
{
   return prepend(str.getCStr());
}

StringVariant &StringVariant::append(const char *str)
{
   zval *self = getUnDerefZvalPtr();
   size_t newLength;
   size_t length = std::strlen(str);
   size_t origLength = getLength();
   size_t requestLength = length + origLength;
   zend_string *destStrPtr = getZendStringPtr();
   if (!Z_OPT_ISREF_P(self)) {
      if (destStrPtr && !ZSTR_IS_INTERNED(destStrPtr)) {
         SEPARATE_STRING(self);
      }
   } else {
      zval *refZval = getZvalPtr();
      if (Z_REFCOUNT_P(refZval) > 1) {
         SEPARATE_STRING(refZval);
      }
   }
   self = getZvalPtr();
   destStrPtr = getZendStringPtr();
   newLength = strReAlloc(destStrPtr, requestLength, 0);
   std::memcpy(ZSTR_VAL(destStrPtr) + origLength, str, length);
   // set self state
   ZSTR_VAL(destStrPtr)[newLength] = '\0';
   ZSTR_LEN(destStrPtr) = newLength;
   Z_STR_P(self) = destStrPtr;
   return *this;
}

StringVariant &StringVariant::append(const char c)
{
   ValueType buffer[2] = {c, '\0'};
   return append(reinterpret_cast<Pointer>(buffer));
}

StringVariant &StringVariant::append(const std::string &str)
{
   return append(str.c_str());
}

StringVariant &StringVariant::append(const StringVariant &str)
{
   return append(str.getCStr());
}

StringVariant &StringVariant::remove(size_t pos, size_t length)
{
   size_t selfLength = getLength();
   if (pos > selfLength) {
      throw std::out_of_range("string pos out of range");
   }
   zval *self = getUnDerefZvalPtr();
   zend_string *destStrPtr = getZendStringPtr();
   // implement php copy on write idiom
   if (!Z_OPT_ISREF_P(self)) {
      if (destStrPtr && !ZSTR_IS_INTERNED(destStrPtr)) {
         SEPARATE_STRING(self);
      }
   }

   Pointer strPtr = getRawStrPtr();
   if (pos + length < selfLength) {
      size_t needCopy = selfLength - pos - length;
      std::memmove(strPtr + pos, strPtr + pos + length, needCopy);
   }
   size_t newLength = selfLength - std::min(length, selfLength - pos);
   *(strPtr + newLength) = '\0';
   ZSTR_LEN(getZendStringPtr()) = newLength;
   return *this;
}

StringVariant &StringVariant::remove(const char *str, bool caseSensitive)
{
   size_t length = std::strlen(str);
   vmapi_long pos = -1;
   while (-1 != (pos = indexOf(str, 0, caseSensitive))) {
      remove(static_cast<size_t>(pos), length);
   }
   return *this;
}

StringVariant &StringVariant::remove(char c, bool caseSensitive)
{
   ValueType buffer[2] = {c, '\0'};
   return remove(reinterpret_cast<Pointer>(buffer), caseSensitive);
}

StringVariant &StringVariant::remove(const std::string &str, bool caseSensitive)
{
   return remove(str.c_str(), caseSensitive);
}

StringVariant &StringVariant::remove(const StringVariant &str, bool caseSensitive)
{
   return remove(str.getCStr(), caseSensitive);
}

StringVariant &StringVariant::insert(size_t pos, const char *str)
{
   size_t origLength = getLength();
   if (pos > origLength) {
      throw std::out_of_range("string pos out of range");
   }

   zval *self = getUnDerefZvalPtr();
   zend_string *destStrPtr = getZendStringPtr();
   // implement php copy on write idiom
   if (!Z_OPT_ISREF_P(self)) {
      if (destStrPtr && !ZSTR_IS_INTERNED(destStrPtr)) {
         SEPARATE_STRING(self);
      }
   }

   self = getZvalPtr();
   destStrPtr = getZendStringPtr();
   size_t length = std::strlen(str);
   size_t requestLength = origLength + length;
   size_t newLength = strReAlloc(destStrPtr, requestLength, 0);

   Pointer dataPtr = ZSTR_VAL(destStrPtr);
   size_t iterator = origLength - pos;
   Pointer newDataEnd = dataPtr + newLength;
   while (iterator--) {
      *(--newDataEnd) = *(dataPtr + pos + iterator);
   }
   std::memcpy(dataPtr + pos, str, length);
   // set self state
   *(dataPtr + newLength) = '\0';
   ZSTR_LEN(destStrPtr) = newLength;
   Z_STR_P(self) = destStrPtr;
   return *this;
}

StringVariant &StringVariant::insert(size_t pos, const char c)
{
   ValueType buffer[2] = {c, '\0'};
   return insert(pos, reinterpret_cast<Pointer>(buffer));
}

StringVariant &StringVariant::insert(size_t pos, const std::string &str)
{
   return insert(pos, str.c_str());
}

StringVariant &StringVariant::insert(size_t pos, const StringVariant &str)
{
   return insert(pos, str.getCStr());
}

StringVariant &StringVariant::replace(size_t pos, size_t length, const char *replace)
{
   remove(pos, length);
   insert(pos, replace);
   return *this;
}

StringVariant &StringVariant::replace(size_t pos, size_t length, const char replace)
{
   remove(pos, length);
   insert(pos, replace);
   return *this;
}

StringVariant &StringVariant::replace(size_t pos, size_t length, const std::string &replace)
{
   remove(pos, length);
   insert(pos, replace);
   return *this;
}

StringVariant &StringVariant::replace(size_t pos, size_t length, const StringVariant &replace)
{
   remove(pos, length);
   insert(pos, replace);
   return *this;
}

StringVariant &StringVariant::replace(const char *search, const char *replaceStr, bool caseSensitive)
{
   size_t length = std::strlen(search);
   vmapi_long pos = -1;
   size_t startPos = 0;
   while (-1 != (pos = indexOf(search, startPos, caseSensitive))) {
      replace(static_cast<size_t>(pos), length, replaceStr);
      startPos = pos + length;
   }
   return *this;
}

StringVariant &StringVariant::replace(char search, char replaceStr, bool caseSensitive)
{
   vmapi_long pos = -1;
   size_t startPos = 0;
   while (-1 != (pos = indexOf(search, startPos, caseSensitive))) {
      replace(static_cast<size_t>(pos), 1, replaceStr);
      startPos = pos + 1;
   }
   return *this;
}

StringVariant &StringVariant::replace(char search, const char *replaceStr, bool caseSensitive)
{
   ValueType buffer[2] = {search, '\0'};
   return replace(reinterpret_cast<Pointer>(buffer), replaceStr, caseSensitive);
}

StringVariant &StringVariant::replace(char search, const std::string &replaceStr, bool caseSensitive)
{
   return replace(search, replaceStr.c_str(), caseSensitive);
}

StringVariant &StringVariant::replace(char search, const StringVariant &replaceStr, bool caseSensitive)
{
   return replace(search, replaceStr.getCStr(), caseSensitive);
}

StringVariant &StringVariant::replace(const char *search, const std::string &replaceStr, bool caseSensitive)
{
   return replace(search, replaceStr.c_str(), caseSensitive);
}

StringVariant &StringVariant::replace(const char *search, const StringVariant &replaceStr, bool caseSensitive)
{
   return replace(search, replaceStr.getCStr(), caseSensitive);
}

StringVariant &StringVariant::replace(const std::string &search, const char *replaceStr, bool caseSensitive)
{
   return replace(search.c_str(), replaceStr, caseSensitive);
}

StringVariant &StringVariant::replace(const std::string &search, const std::string &replaceStr, bool caseSensitive)
{
   return replace(search.c_str(), replaceStr.c_str(), caseSensitive);
}

StringVariant &StringVariant::replace(const std::string &search, const StringVariant &replaceStr, bool caseSensitive)
{
   return replace(search.c_str(), replaceStr.getCStr(), caseSensitive);
}

StringVariant &StringVariant::replace(const StringVariant &search, const char *replaceStr, bool caseSensitive)
{
   return replace(search.getCStr(), replaceStr, caseSensitive);
}

StringVariant &StringVariant::replace(const StringVariant &search, const std::string &replaceStr, bool caseSensitive)
{
   return replace(search.getCStr(), replaceStr.c_str(), caseSensitive);
}

StringVariant &StringVariant::replace(const StringVariant &search, const StringVariant &replaceStr, bool caseSensitive)
{
   return replace(search.getCStr(), replaceStr.getCStr(), caseSensitive);
}

StringVariant &StringVariant::clear()
{
   // here we release zend_string memory
   // and set capacity to zero
   zend_string *destStrPtr = getZendStringPtr();
   if (!destStrPtr) {
      return *this;
   }
   if (!ZSTR_IS_INTERNED(destStrPtr)) {
      if (Z_OPT_ISREF_P(getUnDerefZvalPtr())) {
         zend_string_release(Z_STR_P(getZvalPtr()));
         ZVAL_EMPTY_STRING(getZvalPtr());
      } else {
         zend_string_release(Z_STR_P(getUnDerefZvalPtr()));
         Z_STR_P(getUnDerefZvalPtr()) = nullptr;
      }
   } else {
      Z_STR_P(getUnDerefZvalPtr()) = nullptr;
   }
   return *this;
}

void StringVariant::resize(SizeType size)
{
   if (size == getCapacity()) {
      return;
   }

   zval *self = getUnDerefZvalPtr();
   zend_string *destStrPtr = getZendStringPtr();
   // implement php copy on write idiom
   if (!Z_OPT_ISREF_P(self)) {
      if (destStrPtr && !ZSTR_IS_INTERNED(destStrPtr)) {
         SEPARATE_STRING(self);
      }
   }
   destStrPtr = getZendStringPtr();
   zend_string *newStr = nullptr;
   if (!destStrPtr) {
      newStr = zend_string_alloc(size, 0);
   } else {
      newStr = zend_string_realloc(destStrPtr, size, 0);
   }

   ZSTR_VAL(newStr)[size] = '\0';
   Z_STR_P(getZvalPtr()) = newStr;
   setCapacity(size);
}

void StringVariant::resize(SizeType size, char fillChar)
{
   size_t oldSize = getSize();
   resize(size);
   if (size > oldSize) {
      Pointer strPtr = getRawStrPtr();
      std::memset(strPtr + oldSize, fillChar, size - oldSize);
   }
}

std::string StringVariant::trimmed() const
{
   if (isEmpty()) {
      return std::string();
   }
   ConstPointer start = getRawStrPtr();
   ConstPointer end = start + getLength();
   while (start < end && std::isspace(*start)) {
      ++start;
   }
   if (start < end) {
      while (start < end && std::isspace(*(end - 1))) {
         end--;
      }
   }
   return std::string(start, end);
}

std::string StringVariant::simplified() const
{
   if (isEmpty()) {
      return std::string();
   }
   ConstPointer start = getRawStrPtr();
   ConstPointer end = start + getLength();
   GuardValuePtrType tempStr(static_cast<Pointer>(emalloc(getSize())), std_php_memory_deleter);
   // trim two side space chars
   while (start < end && std::isspace(*start)) {
      ++start;
   }
   if (start < end) {
      while (start < end && std::isspace(*(end - 1))) {
         end--;
      }
   }
   // trim mid space chars
   Pointer dest = tempStr.get();
   bool seeSpace = false;
   while (start < end) {
      if (std::isspace(*start)) {
         ++start;
         if (!seeSpace) {
            *dest++ = ' ';
            seeSpace = true;
         }
      } else {
         *dest++ = *start++;
         seeSpace = false;
      }
   }
   return std::string(tempStr.get(), dest);
}

std::string StringVariant::left(size_t size) const
{
   if (isEmpty()) {
      return std::string();
   }
   size_t needCopyLength = std::min(size, getSize());
   Pointer strPtr = getRawStrPtr();
   return std::string(strPtr, strPtr + needCopyLength);
}

std::string StringVariant::right(size_t size) const
{
   if (isEmpty()) {
      return std::string();
   }
   size_t selfLength = getSize();
   size_t needCopyLength = std::min(size, selfLength);
   Pointer strPtr = getRawStrPtr();
   return std::string(strPtr + selfLength - needCopyLength, strPtr + selfLength);
}

std::string StringVariant::leftJustified(size_t size, char fill) const
{
   if (isEmpty()) {
      return std::string();
   }
   Pointer strPtr = getRawStrPtr();
   size_t selfLength = getLength();
   size_t needCopyLength = std::min(size, selfLength);
   std::string ret(strPtr, strPtr + needCopyLength);
   if (size > selfLength) {
      // need fill
      size_t fillLength = size - selfLength;
      while(fillLength--) {
         ret.push_back(fill);
      }
   }
   return ret;
}

std::string StringVariant::rightJustified(size_t size, char fill) const
{
   if (isEmpty()) {
      return std::string();
   }
   Pointer strPtr = getRawStrPtr();
   size_t selfLength = getLength();
   size_t needCopyLength = std::min(size, selfLength);
   std::string ret;
   ret.reserve(std::max(size, selfLength));
   if (size > selfLength) {
      // need fill
      size_t fillLength = size - selfLength;
      while(fillLength--) {
         ret.push_back(fill);
      }
   }
   ret.append(strPtr, needCopyLength);
   return ret;
}

std::string StringVariant::substring(size_t pos, size_t length) const
{
   if (isEmpty()) {
      return std::string();
   }
   size_t selfLength = getLength();
   if (pos > selfLength) {
      throw std::out_of_range("string pos out of range");
   }
   Pointer strPtr = getRawStrPtr();
   return std::string(strPtr + pos, strPtr + std::min(selfLength, pos + length));
}

std::string StringVariant::substring(size_t pos) const
{
   return substring(pos, getSize());
}

std::string StringVariant::repeated(size_t times) const
{
   if (isEmpty()) {
      return std::string();
   }
   std::string ret;
   Pointer dataPtr = getRawStrPtr();
   Pointer endDataPtr = dataPtr + getLength();
   while (times--) {
      ret.append(dataPtr, endDataPtr);
   }
   return ret;
}

std::vector<std::string> StringVariant::split(char sep, bool keepEmptyParts, bool caseSensitive)
{
   ValueType buffer[2] = {sep, '\0'};
   return split(reinterpret_cast<Pointer>(buffer), keepEmptyParts, caseSensitive);
}

std::vector<std::string> StringVariant::split(const char *sep, bool keepEmptyParts, bool caseSensitive)
{
   std::vector<std::string> list;
   if (isEmpty()) {
      return list;
   }
   std::list<size_t> points;
   vmapi_long pos = -1;
   size_t startPos = 0;
   size_t sepLength = std::strlen(sep);
   while (-1 != (pos = indexOf(sep, startPos, caseSensitive))) {
      points.push_back(pos);
      startPos = pos + sepLength;
   }
   if (points.size() == 0) {
      list.emplace_back(getCStr(), getLength());
      return list;
   }
   // push end point
   points.push_back(getLength());
   startPos = 0;
   ConstPointer dataPtr = getCStr();
   while (!points.empty()) {
      size_t splitPoint = points.front();
      if ((splitPoint - startPos) > 0 || (keepEmptyParts && (splitPoint - startPos) == 0)) {
         list.emplace_back(dataPtr + startPos, dataPtr + splitPoint);
      }
      startPos = splitPoint + sepLength;
      points.pop_front();
   }
   return list;
}

const char *StringVariant::getCStr() const noexcept
{
   return Z_STR_P(getZvalPtr()) ? Z_STRVAL_P(const_cast<zval *>(getZvalPtr())) : nullptr;
}

char *StringVariant::getData() noexcept
{
   return Z_STR_P(getZvalPtr()) ? Z_STRVAL_P(const_cast<zval *>(getZvalPtr())) : nullptr;
}

const char *StringVariant::getData() const noexcept
{
   return Z_STR_P(getZvalPtr()) ? Z_STRVAL_P(const_cast<zval *>(getZvalPtr())) : nullptr;
}

char *StringVariant::getRawStrPtr() const noexcept
{
   return Z_STR_P(getZvalPtr()) ? Z_STRVAL_P(const_cast<zval *>(getZvalPtr())) : nullptr;
}

StringVariant::SizeType StringVariant::getSize() const noexcept
{
   zval *self = const_cast<zval *>(getZvalPtr());
   return !Z_STR_P(self) ? 0 : Z_STRLEN_P(self);
}

bool StringVariant::isEmpty() const noexcept
{
   return 0 == getSize();
}

StringVariant::SizeType StringVariant::getLength() const noexcept
{
   return getSize();
}

StringVariant::SizeType StringVariant::getCapacity() const noexcept
{
   const zend_string *strPtr = getZendStringPtr();
   if (!strPtr) {
      return 0;
   }
   return static_cast<SizeType>(strPtr->capacity);
}

StringVariant::~StringVariant() noexcept
{
}

zend_string *StringVariant::getZendStringPtr() const
{
   return Z_STR_P(getZvalPtr());
}

size_t StringVariant::calculateNewStrSize(size_t length) noexcept
{
   return ZEND_MM_ALIGNED_SIZE_EX(length + STR_VARIANT_OVERHEAD, STR_VARIANT_PAGE) - STR_VARIANT_OVERHEAD;
}

void StringVariant::strStdRealloc(zend_string *&str, size_t length)
{
   size_t newCapacity = length < STR_VARIANT_START_SIZE
         ? STR_VARIANT_START_LEN
         : calculateNewStrSize(length);
   if (UNEXPECTED(!str) || ZSTR_IS_INTERNED(str)) {
      str = zend_string_alloc(newCapacity, 0);
      ZSTR_LEN(str) = 0;
   } else {
      zend_string *newStr = reinterpret_cast<zend_string *>(erealloc2(str, _ZSTR_STRUCT_SIZE(newCapacity),
                                                                      _ZSTR_HEADER_SIZE + ZSTR_LEN(str)));
      VMAPI_ASSERT_X(newStr, "StringVariant::strStdRealloc", "realloc memory error");
      str = newStr;
   }
   str->capacity = newCapacity;
}

void StringVariant::strPersistentRealloc(zend_string *&str, size_t length)
{
   size_t newCapacity = length < STR_VARIANT_START_SIZE
         ? STR_VARIANT_START_LEN
         : calculateNewStrSize(length);
   if (UNEXPECTED(!str) || ZSTR_IS_INTERNED(str)) {
      str = zend_string_alloc(newCapacity, 1);
      ZSTR_LEN(str) = 0;
   } else {
      zend_string *newStr = reinterpret_cast<zend_string *>(perealloc(str, _ZSTR_STRUCT_SIZE(newCapacity), 1));
      VMAPI_ASSERT_X(newStr, "StringVariant::strPersistentRealloc", "realloc memory error");
      str = newStr;
   }
   str->capacity = newCapacity;
}

StringVariant::SizeType StringVariant::strAlloc(zend_string *&str, size_t length, bool persistent)
{
   if (UNEXPECTED(!str)) {
      goto do_smart_str_realloc;
   } else {
      length += ZSTR_LEN(str);
      if (UNEXPECTED(length >= ZSTR_CAPACITY(str))) {
do_smart_str_realloc:
         if (persistent) {
            strPersistentRealloc(str, length);
         } else {
            strStdRealloc(str, length);
         }
      }
   }
   return length;
}

StringVariant::SizeType StringVariant::strReAlloc(zend_string *&str, size_t length, bool persistent)
{
   if (UNEXPECTED(!str)) {
      goto do_smart_str_realloc;
   } else {
      if (UNEXPECTED(length >= ZSTR_CAPACITY(str))) {
do_smart_str_realloc:
         if (persistent) {
            strPersistentRealloc(str, length);
         } else {
            strStdRealloc(str, length);
         }
      }
   }
   return length;
}

void StringVariant::setCapacity(SizeType capacity)
{
   zend_string *strPtr = getZendStringPtr();
   VMAPI_ASSERT(strPtr);
   strPtr->capacity = capacity;
}

bool operator ==(const char *lhs, const StringVariant &rhs)
{
   return 0 == std::memcmp(lhs, rhs.getCStr(), std::max(std::strlen(lhs), rhs.getLength()));
}

bool operator ==(const std::string &lhs, const StringVariant &rhs)
{
   return 0 == std::memcmp(lhs.c_str(), rhs.getCStr(), std::max(lhs.length(), rhs.getLength()));
}

bool operator !=(const char *lhs, const StringVariant &rhs)
{
   return 0 != std::memcmp(lhs, rhs.getCStr(), std::max(std::strlen(lhs), rhs.getLength()));
}

bool operator !=(const std::string &lhs, const StringVariant &rhs)
{
   return 0 != std::memcmp(lhs.c_str(), rhs.getCStr(), std::max(lhs.length(), rhs.getLength()));
}

bool operator <(const char *lhs, const StringVariant &rhs)
{
   return std::memcmp(lhs, rhs.getCStr(), std::max(std::strlen(lhs), rhs.getLength())) < 0;
}

bool operator <(const std::string &lhs, const StringVariant &rhs)
{
   return std::memcmp(lhs.c_str(), rhs.getCStr(), std::max(lhs.length(), rhs.getLength())) < 0;
}

bool operator <=(const char *lhs, const StringVariant &rhs)
{
   return std::memcmp(lhs, rhs.getCStr(), rhs.getLength()) < 0 ||
         0 == std::memcmp(lhs, rhs.getCStr(), rhs.getLength());
}

bool operator <=(const std::string &lhs, const StringVariant &rhs)
{
   size_t len = std::max(lhs.length(), rhs.getLength());
   return std::memcmp(lhs.c_str(), rhs.getCStr(), len) < 0 ||
         0 == std::memcmp(lhs.c_str(), rhs.getCStr(), len);
}

bool operator >(const char *lhs, const StringVariant &rhs)
{
   return std::memcmp(lhs, rhs.getCStr(), std::max(std::strlen(lhs), rhs.getLength())) > 0;
}

bool operator >(const std::string &lhs, const StringVariant &rhs)
{
   return std::memcmp(lhs.c_str(), rhs.getCStr(), std::max(lhs.length(), rhs.getLength())) > 0;
}


bool operator >=(const char *lhs, const StringVariant &rhs)
{
   size_t len = std::max(std::strlen(lhs), rhs.getLength());
   return std::memcmp(lhs, rhs.getCStr(), len) > 0 ||
         0 == std::memcmp(lhs, rhs.getCStr(), len);
}

bool operator >=(const std::string &lhs, const StringVariant &rhs)
{
   size_t len = std::max(lhs.length(), rhs.getLength());
   return std::memcmp(lhs.c_str(), rhs.getCStr(), len) > 0 ||
         0 == std::memcmp(lhs.c_str(), rhs.getCStr(), len);
}

std::string operator +(const StringVariant& lhs, const StringVariant &rhs)
{
   std::string ret(lhs.getCStr());
   ret += rhs.getCStr();
   return ret;
}

std::string operator +(const StringVariant& lhs, const char *rhs)
{
   std::string ret(lhs.getCStr());
   ret += rhs;
   return ret;
}

std::string operator +(const StringVariant& lhs, const std::string &rhs)
{
   std::string ret(lhs.getCStr());
   ret += rhs;
   return ret;
}

std::string operator +(const StringVariant& lhs, char rhs)
{
   std::string ret(lhs.getCStr());
   ret += rhs;
   return ret;
}

std::string operator +(const char *lhs, const StringVariant &rhs)
{
   std::string ret(lhs);
   ret += rhs.getCStr();
   return ret;
}

std::string operator +(const std::string &lhs, const StringVariant &rhs)
{
   std::string ret(lhs);
   ret += rhs.getCStr();
   return ret;
}

std::string operator +(char lhs, const StringVariant &rhs)
{
   std::string ret;
   ret += lhs;
   ret += rhs.getCStr();
   return ret;
}

} // vmapi
} // polar
