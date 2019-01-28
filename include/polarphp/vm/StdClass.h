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

#ifndef POLARPHP_VMAPI_STD_CLASS_H
#define POLARPHP_VMAPI_STD_CLASS_H

#include "polarphp/vm/ZendApi.h"
#include "polarphp/vm/ds/Variant.h"
#include "polarphp/vm/ds/ObjectVariant.h"

namespace polar {
namespace vmapi {

/// forward declare class with namespace
namespace internal {
class StdClassPrivate;
} // internal

/// forward declare class
class ArrayVariant;
class ObjectBinder;

using internal::StdClassPrivate;

class VMAPI_DECL_EXPORT StdClass
{
protected:
   /**
    * Constructor
    */
   StdClass();

   /**
    * Copy constructor
    *
    * This copy constructor is explicitly defined to make sure that the
    * copied object does not already have an implementation in the zend engine.
    * Otherwise the copied object has the same object handle as the original
    * object.
    *
    * @param object
    */
   StdClass(const StdClass &object);

public:
   virtual ~StdClass();
   /**
    * Get access to a property by name using the [] operator
    * @param  string
    * @return Value
    */
   Variant operator[](const char *name) const;

   /**
    * Alternative way to access a property using the [] operator
    * @param  string
    * @return Value
    */
   Variant operator[](const std::string &name) const;
   /**
    * Retrieve a property by name
    * @param  string
    * @return Value
    */
   Variant property(const char *name) const;

   /**
    * Retrieve a property by name
    * @param  string
    * @return Value
    */
   Variant property(const std::string &name) const;

   /**
    * Overridable method that is called right before an object is destructed
    */
   void __destruct() const;

   /**
    * Overridable method that is called right after an object is cloned
    *
    * The default implementation does nothing
    */
   void __clone();

   /**
    * Overridable method that is called to check if a property is set
    *
    * The default implementation does nothing, and the script will fall back
    * to accessing the regular object properties
    *
    * @param  key
    * @return bool
    */
   bool __isset(const std::string &key) const;

   /**
    * Overridable method that is called to set a new property
    *
    * The default implementation does nothing, and the script will fall back
    * to accessing the regular object properties
    *
    * @param  key
    * @param  value
    */
   void __set(const std::string &key, const Variant &value);

   /**
    * Retrieve a property
    *
    * The default implementation does nothing, and the script will fall back
    * to accessing the regular object properties
    *
    * @param  key
    * @return value
    */
   Variant __get(const std::string &key) const;

   /**
    * Remove a member
    *
    * The default implementation does nothing, and the script will fall back
    * to accessing the regular object properties
    *
    * @param key
    */
   void __unset(const std::string &key);

   /**
    * Call a method
    *
    * This method is called when a method is called from the PHP script that
    * was not explicitly defined. You can use this to catch variable method
    * names, or to support all thinkable method names.
    *
    * @param  method      Name of the method that was called
    * @param  params      The parameters that were passed to the function
    * @return Value       The return value
    */
   Variant __call(const std::string &method, Parameters &params) const;

   /**
    * Call the class as if it was a function
    *
    * This method is called when a an object is used with () operators:
    * $object(). You can override this method to make objects callable.
    *
    * @param  params      The parameters that were passed to the function
    * @return Value       The return value
    */
   Variant __invoke(Parameters &params) const;

   /**
    * Cast the object to a string
    *
    * This method is called when an object is casted to a string, or when
    * it is used in a string context
    *
    * @return Value       The object as a string
    */
   Variant __toString() const;

   /**
    * Cast the object to an integer
    *
    * This method is called when an object is casted to an integer, or when
    * it is used in an integer context
    *
    * @return int Integer value
    */
   Variant __toInteger() const;

   /**
    * Cast the object to a float
    *
    * This method is called when an object is casted to a float, or when it
    * is used in a float context
    *
    * @return double      Floating point value
    */
   Variant __toDouble() const;

   /**
    * Cast the object to a boolean
    *
    * This method is called when an object is casted to a bool, or when it
    * is used in a boolean context
    *
    * @return bool
    */
   Variant __toBool() const;

   /**
    *  Compare the object with a different object
    *
    *  Check how a different object compares to this object
    *
    *  @param  that        Object to compare with
    *  @return int
    */
   int __compare(const StdClass &object) const;

   ArrayVariant __debugInfo() const;
protected:
   ObjectVariant *getObjectZvalPtr() const;
   ObjectVariant *getObjectZvalPtr();
   template <typename ...Args>
   Variant callParent(const char *name, Args &&...args);
   template <typename ...Args>
   Variant callParent(const char *name, Args &&...args) const;
   template <typename ...Args>
   Variant call(const char *name, Args &&...args);
   template <typename ...Args>
   Variant call(const char *name, Args &&...args) const;
private:
   zval *doCallParent(const char *name, const int argc, Variant *argv, zval *retval) const;
protected:
   VMAPI_DECLARE_PRIVATE(StdClass);
   friend class Variant;// for Variant(const StdClass &stdClass);
   friend class ObjectBinder; // for set zend_object pointer
   friend class ObjectVariant; // for initialze from StdClass instance
   std::unique_ptr<StdClassPrivate> m_implPtr;
};


template <typename ...Args>
Variant StdClass::callParent(const char *name, Args&&... args) const
{
   std::array<Variant, sizeof...(args)> vargs{ Variant(std::forward<Args>(args))... };
   zval retval;
   std::memset(&retval, 0, sizeof(retval));
   doCallParent(name, sizeof...(Args), vargs.data(), &retval);
   Variant resultVarint(retval);
   zval_dtor(&retval);
   return resultVarint;
}

template <typename ...Args>
Variant StdClass::callParent(const char *name, Args&&... args)
{
   return const_cast<const StdClass &>(*this).callParent(name, std::forward<Args>(args)...);
}

template <typename ...Args>
Variant StdClass::call(const char *name, Args&&... args) const
{
   Variant vargs[] = { Variant(std::forward<Args>(args))... };
   return getObjectZvalPtr()->exec(name, sizeof...(Args), vargs);
}

template <typename ...Args>
Variant StdClass::call(const char *name, Args&&... args)
{
   return const_cast<const StdClass &>(*this).call(name, std::forward<Args>(args)...);
}

} // vmapi
} // polar

#endif // POLARPHP_VMAPI_STD_CLASS_H
