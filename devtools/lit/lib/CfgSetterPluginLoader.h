// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/09/22.

#ifndef POLAR_DEVLTOOLS_LIT_CFG_SETTER_PLUGIN_LOADER_H
#define POLAR_DEVLTOOLS_LIT_CFG_SETTER_PLUGIN_LOADER_H

#include <string>

namespace polar {
namespace lit {

class CfgSetterPlugin
{
public:
   CfgSetterPlugin()
   {}
   CfgSetterPlugin(const std::string &path, void *handle, const std::string &startupPath = "")
      : m_handle(handle),
        m_path(path),
        m_startupPath(startupPath)
   {}

   const std::string &getPath() const
   {
      return m_path;
   }

   const std::string &getStartupPath() const
   {
      return m_startupPath;
   }

   CfgSetterPlugin &setStartupPath(const std::string &path)
   {
      m_startupPath = path;
      return *this;
   }

   template <typename CfgSetterType>
   CfgSetterType getCfgSetter(const std::string &setterName) const
   {
      return reinterpret_cast<CfgSetterType>(getSetterSymbol(setterName));
   }

protected:
   void *getSetterSymbol(const std::string &symbol) const;
private:
   void *m_handle = nullptr;
   std::string m_path;
   std::string m_startupPath;
};

CfgSetterPlugin load_cfg_setter_plugin(const std::string &pluginPath, const std::string &pluginRootDir);
void unload_cfg_setter_plugin(const std::string &pluginPath);

} // lit
} // polar

#endif // POLAR_DEVLTOOLS_LIT_CFG_SETTER_PLUGIN_LOADER_H
