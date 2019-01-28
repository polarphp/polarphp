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

#include "polarphp/vm/lang/Ini.h"
#include "polarphp/runtime/BuildinIniModifyHandler.h"

#include <iostream>

namespace polar {
namespace vmapi {

using polar::runtime::update_string_handler;

namespace internal {
class IniPrivate
{
public:
   using CfgType = Ini::CfgType;
public:
   IniPrivate(const char *name, const char *value, const CfgType cfgType = CfgType::All)
      :m_name(name), m_value(value), m_cfgType(cfgType)
   {}
   IniPrivate(const char *name, bool value, const CfgType cfgType = CfgType::All)
      :m_name(name), m_value(bool2str(value)), m_cfgType(cfgType)
   {}

   IniPrivate(const char *name, const int16_t value, const CfgType cfgType = CfgType::All)
      :m_name(name), m_value(std::to_string(value)), m_cfgType(cfgType)
   {}

   IniPrivate(const char *name, const int32_t value, const CfgType cfgType = CfgType::All)
      :m_name(name), m_value(std::to_string(value)), m_cfgType(cfgType)
   {}

   IniPrivate(const char *name, const int64_t value, const CfgType cfgType = CfgType::All)
      :m_name(name), m_value(std::to_string(value)), m_cfgType(cfgType)
   {}

   IniPrivate(const char *name, const double value, const CfgType cfgType = CfgType::All)
      :m_name(name), m_value(std::to_string(value)), m_cfgType(cfgType)
   {}

   IniPrivate(const IniPrivate &other)
      : m_name(other.m_name),
        m_value(other.m_value),
        m_cfgType(other.m_cfgType)
   {}

#ifdef POLAR_CC_MSVC
   static const char *bool2str(const bool value)
#else
   static constexpr const char *bool2str(const bool value)
#endif
   {
      return (value ? "On" : "Off");
   }
   std::string m_name;
   std::string m_value;
   CfgType m_cfgType;
};
} // internal

Ini::Ini(const char *name, const char *value, const CfgType cfgType)
   : m_implPtr(new IniPrivate(name, value, cfgType))
{}
Ini::Ini(const char *name, bool value, const CfgType cfgType)
   : m_implPtr(new IniPrivate(name, value, cfgType))
{}

Ini::Ini(const char *name, const int16_t value, const CfgType cfgType)
   : m_implPtr(new IniPrivate(name, value, cfgType))
{}

Ini::Ini(const char *name, const int32_t value, const CfgType cfgType)
   : m_implPtr(new IniPrivate(name, value, cfgType))
{}

Ini::Ini(const char *name, const int64_t value, const CfgType cfgType)
   : m_implPtr(new IniPrivate(name, value, cfgType))
{}

Ini::Ini(const char *name, const double value, const CfgType cfgType)
   : m_implPtr(new IniPrivate(name, value, cfgType))
{}

Ini::Ini(const Ini &other)
   : m_implPtr(new IniPrivate(*other.m_implPtr.get()))
{}

Ini::Ini(Ini &&other)
   : m_implPtr(std::move(other.m_implPtr))
{}

bool Ini::operator==(const Ini &other) const
{
   VMAPI_D(const Ini);
   return implPtr->m_name == other.m_implPtr->m_name &&
         implPtr->m_value == other.m_implPtr->m_value &&
         implPtr->m_cfgType == other.m_implPtr->m_cfgType;
}

Ini::~Ini()
{}

void Ini::setupIniDef(_zend_ini_entry_def *zendIniDef, int)
{
   VMAPI_D(Ini);
   zendIniDef->modifiable = static_cast<int>(implPtr->m_cfgType);
   zendIniDef->name = implPtr->m_name.data();
   zendIniDef->name_length = implPtr->m_name.size();
   zendIniDef->on_modify = nullptr;
   zendIniDef->mh_arg1 = nullptr;
   /// TODO polarphp does not have global data right now
   // zendIniDef->mh_arg2 = (void *) &vmapi_globals_id;
   zendIniDef->mh_arg2 = nullptr;
   zendIniDef->mh_arg3 = nullptr;
   zendIniDef->value = implPtr->m_value.data();
   zendIniDef->value_length = implPtr->m_value.size();
   zendIniDef->displayer = nullptr;
}

std::string IniValue::getStringValue() const
{
   const char *value = getRawValue();
   return std::string(value ? value : "");
}

class IniValuePrivate
{
public:
   IniValuePrivate(const char *name, const bool isOrig)
      : m_name(name), m_isOrig(isOrig)
   {}
   /**
    * @brief ini entry name
    * @var std::string
    */
   std::string m_name;

   /**
    * @brief Is the orig value
    * @var bool
    */
   bool m_isOrig = false;
};

IniValue::IniValue(const char *name, const bool isOrig)
   : m_implPtr(new IniValuePrivate(name, isOrig))
{}

IniValue::IniValue(const IniValue &value)
{
   VMAPI_D(IniValue);
   implPtr->m_name = value.m_implPtr->m_name;
   implPtr->m_isOrig = value.m_implPtr->m_isOrig;
}

IniValue::IniValue(IniValue &&value)
   : m_implPtr(std::move(value.m_implPtr))
{}

IniValue::~IniValue()
{}

/**
 * Cost to a number
 *
 * @return int64_t
 */
int64_t IniValue::getNumericValue() const
{
   VMAPI_D(const IniValue);
   return zend_ini_long(const_cast<char *>(implPtr->m_name.c_str()), implPtr->m_name.size(), implPtr->m_isOrig);
}

const char* IniValue::getRawValue() const
{
   VMAPI_D(const IniValue);
   return zend_ini_string(const_cast<char *>(implPtr->m_name.c_str()), implPtr->m_name.size(), implPtr->m_isOrig);
}

IniValue::operator double() const
{
   VMAPI_D(const IniValue);
   return zend_ini_double(const_cast<char *>(implPtr->m_name.c_str()), implPtr->m_name.size(), implPtr->m_isOrig);
}

std::ostream &operator<<(std::ostream &stream, const IniValue &iniValue)
{
   return stream << static_cast<const char *>(iniValue);
}

} // vmapi
} // polar
