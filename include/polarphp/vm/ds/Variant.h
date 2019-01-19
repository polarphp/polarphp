// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/23.

#ifndef POLARPHP_VMAPI_DS_VARIANT_H
#define POLARPHP_VMAPI_DS_VARIANT_H

#include "polarphp/vm/ZendApi.h"
#include "polarphp/vm/ds/internal/VariantPrivate.h"

#include <vector>
#include <map>
#include <cstring>
#include <string>

namespace polar {
namespace vmapi {

namespace internal {
class VariantPrivate;
} // internal

using internal::VariantPrivate;
class StdClass;

class StringVariant;
class NumericVariant;
class DoubleVariant;
class ArrayVariant;
class BooleanVariant;
class ObjectVariant;
class CallableVariant;

class VMAPI_DECL_EXPORT Variant
{
public:
   /**
    * Empty constructor (value = NULL)
    */
   Variant();
   /**
    * Constructor for various types
    */
   Variant(const std::nullptr_t value);
   Variant(const std::int8_t value);
   Variant(const std::int16_t value);
   Variant(const std::int32_t value);
#if SIZEOF_ZEND_LONG == 8
   Variant(const std::int64_t value);
#endif
   Variant(const double value);
   Variant(const bool value);
   Variant(const char value);
   Variant(const std::string &value);
   Variant(const char *value, size_t length);
   Variant(const char *value);
   Variant(const StdClass &nativeObject);

   /**
    * Copy constructor
    * @param  value
    */
   Variant(const Variant &other);
   Variant(const Variant &other, bool isRef);
   Variant(BooleanVariant &value, bool isRef);
   Variant(NumericVariant &value, bool isRef);
   Variant(DoubleVariant &value, bool isRef);
   Variant(StringVariant &value, bool isRef);
   Variant(ArrayVariant &value, bool isRef);

   Variant(const BooleanVariant &value);
   Variant(const NumericVariant &value);
   Variant(const StringVariant &value);
   Variant(const DoubleVariant &value);
   Variant(const ArrayVariant &value);
   Variant(const ObjectVariant &value);
   Variant(const CallableVariant &value);

   Variant(BooleanVariant &&value);
   Variant(NumericVariant &&value);
   Variant(StringVariant &&value);
   Variant(DoubleVariant &&value);
   Variant(ArrayVariant &&value);
   Variant(ObjectVariant &&value);
   Variant(CallableVariant &&value);

   template <typename T,
             size_t arrayLength,
             typename std::enable_if<std::is_integral<T>::value>::type>
   Variant(char (&value)[arrayLength], T length);
   template <size_t arrayLength>
   Variant(char (&value)[arrayLength]);
   /**
    * Wrap object around zval
    * @param  zval Zval to wrap
    * @param  ref Force this to be a reference
    */
   Variant(zval *value, bool isRef = false);
   Variant(zval &value, bool isRef = false);
   Variant(zval &&value, bool isRef = false);

   /**
    * Move constructor
    * @param  value
    */
   Variant(Variant &&other) noexcept;

   /**
    * Destructor
    */
   virtual ~Variant() noexcept;

   /**
    * Move assignment
    * @param  value
    * @return Value
    */
   Variant &operator =(const Variant &value);
   Variant &operator =(Variant &&value) noexcept;
   /**
    * Assignment operator for various types
    * @param  value
    * @return Value
    */
   Variant &operator =(std::nullptr_t value);
   Variant &operator =(std::int8_t value);
   Variant &operator =(std::int16_t value);
   Variant &operator =(std::int32_t value);
#if SIZEOF_ZEND_LONG == 8
   Variant &operator =(std::int64_t value);
#endif
   Variant &operator =(bool value);
   Variant &operator =(char value);
   Variant &operator =(const std::string &value);
   Variant &operator =(const char *value);
   Variant &operator =(double value);
   Variant &operator =(zval *value);
   bool operator ==(const Variant &other) const;
   bool operator !=(const Variant &other) const;
   bool operator ==(const zval &other) const;
   bool operator !=(const zval &other) const;
   bool strictEqual(const Variant &other) const;
   bool strictNotEqual(const Variant &other) const;
   /**
    * Cast to a boolean
    * @return boolean
    */
   virtual operator bool () const
   {
      return toBoolean();
   }

   /**
    * Cast to a string
    * @return string
    */
   virtual operator std::string () const
   {
      return toString();
   }

   operator zval * () const;

   /**
    * The type of object
    * @return Type
    */
   Type getType() const noexcept;

   /**
    * The type of object
    * @return Type
    */
   Type getUnDerefType() const noexcept;

   std::string getTypeStr() const noexcept;

   /**
    * Make a clone of the value with the same type
    *
    * @return Value
    */
   Variant clone() const;

   /**
    * Check if the value is of a certain type
    *
    * @return bool
    */
   bool isNull() const noexcept;
   bool isLong() const noexcept;
   bool isBool() const noexcept;
   bool isString() const noexcept;
   bool isDouble() const noexcept;
   bool isObject() const noexcept;
   bool isArray() const noexcept;
   bool isScalar() const noexcept
   {
      return isNull() || isLong() || isBool() || isString() || isDouble();
   }

   /**
    * Retrieve the value as boolean
    *
    * @return bool
    */
   virtual bool toBoolean() const noexcept;

   /**
    * Retrieve the value as a string
    *
    * @return string
    */
   virtual std::string toString() const noexcept;

   zval &getZval() const noexcept;
   zval *getZvalPtr() noexcept;
   const zval *getZvalPtr() const noexcept;
   zval &getUnDerefZval() const noexcept;
   zval *getUnDerefZvalPtr() noexcept;
   const zval *getUnDerefZvalPtr() const noexcept;
   uint32_t getRefCount() const noexcept;
   zval detach(bool keeprefcount);
   Variant makeReferenceByZval();
   bool isReference() const noexcept;
   void invalidate() noexcept;

protected:
   static void stdCopyZval(zval *dest, zval *source);
   static void stdAssignZval(zval *dest, zval *source);
   static void selfDeref(zval *self);

protected:
   friend class StringVariant;
   friend class NumericVariant;
   friend class DoubleVariant;
   friend class ArrayVariant;
   friend class BooleanVariant;
   friend class ObjectVariant;
   friend class CallableVariant;
   VMAPI_DECLARE_PRIVATE(Variant);
   std::shared_ptr<VariantPrivate> m_implPtr = nullptr;
};

std::ostream &operator <<(std::ostream &stream, const Variant &value);

} // vmapi
} // polar

#endif // POLARPHP_VMAPI_DS_VARIANT_H
