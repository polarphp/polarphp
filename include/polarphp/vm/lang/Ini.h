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

#ifndef POLARPHP_VMAPI_LANG_INI_H
#define POLARPHP_VMAPI_LANG_INI_H

#include "polarphp/vm/ZendApi.h"

#include <string>

struct _zend_ini_entry;

namespace polar {
namespace vmapi {

namespace internal {
class IniPrivate;
class IniValuePrivate;
} // internal

using internal::IniPrivate;
using internal::IniValuePrivate;

class VMAPI_DECL_EXPORT Ini
{
public:
   enum class CfgType : int
   {
      User    = ZEND_INI_USER,
      PerDir  = ZEND_INI_PERDIR,
      System  = ZEND_INI_SYSTEM,
      All     = ZEND_INI_USER | ZEND_INI_PERDIR | ZEND_INI_SYSTEM
   };

public:
   Ini(const char *name, const char *value, const CfgType cfgType = CfgType::All);
   Ini(const char *name, bool value, const CfgType cfgType = CfgType::All);
   Ini(const char *name, const int16_t value, const CfgType cfgType = CfgType::All);
   Ini(const char *name, const int32_t value, const CfgType cfgType = CfgType::All);
   Ini(const char *name, const int64_t value, const CfgType cfgType = CfgType::All);
   Ini(const char *name, const double value, const CfgType cfgType = CfgType::All);
   Ini(const Ini &other);
   Ini(Ini &&other);
   ~Ini();

public:
   bool operator==(const Ini &other) const;
   void setupIniDef(struct _zend_ini_entry_def *zendIniDef, int moduleNumber = -1);

private:
   VMAPI_DECLARE_PRIVATE(Ini);
   std::unique_ptr<IniPrivate> m_implPtr;
};

class VMAPI_DECL_EXPORT IniValue
{
public:
   IniValue(const char *name, const bool isOrig);
   IniValue(const IniValue &value);
   IniValue(IniValue &&value);
   ~IniValue();
public:

   int64_t getNumericValue() const;
   const char *getRawValue() const;
   bool getBoolValue() const;
   std::string getStringValue() const;

   /**
    * Cast to a floating point
    *
    * @return double
    */
   operator double () const;

   operator int16_t () const
   {
      return static_cast<int16_t>(getNumericValue());
   }

   operator int32_t () const
   {
      return static_cast<int32_t>(getNumericValue());
   }

   operator int64_t () const
   {
      return getNumericValue();
   }

   operator bool () const
   {
      return getBoolValue();
   }

   operator std::string () const
   {
      return getStringValue();
   }

   operator const char * () const
   {
      return getRawValue();
   }

private:
   VMAPI_DECLARE_PRIVATE(IniValue);
   std::unique_ptr<IniValuePrivate> m_implPtr;
};

VMAPI_DECL_EXPORT std::ostream &operator<<(std::ostream &stream, const IniValue &iniValue);

} // vmapi
} // polar

#endif // POLARPHP_VMAPI_LANG_INI_H
