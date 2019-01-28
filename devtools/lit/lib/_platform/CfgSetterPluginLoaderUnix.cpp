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

#include "../CfgSetterPluginLoader.h"
#include "../Utils.h"
#include <map>
#include <filesystem>
#include <dlfcn.h>

namespace polar {
namespace lit {

namespace fs = std::filesystem;

static std::map<const std::string, void *> sg_pluginPool{};

CfgSetterPlugin load_cfg_setter_plugin(const std::string &pluginPath, const std::string &pluginRootDir)
{
   // @TODO now hard code
   fs::path pluginFilepath = fs::path(pluginRootDir) / (pluginPath + ".so");
   void *handle = nullptr;
   if (sg_pluginPool.find(pluginPath) != sg_pluginPool.end()) {
      handle = sg_pluginPool.at(pluginPath);
   } else {
      if (!fs::exists(pluginFilepath)) {
         throw std::runtime_error(format_string("cfg setter plugin %s is not exist", pluginFilepath.c_str()));
      }
      handle = dlopen(pluginFilepath.string().c_str(), RTLD_NOW | RTLD_GLOBAL);
      if (!handle) {
         throw std::runtime_error(format_string("dlopen error: %s\n", dlerror()));
      }
      dlerror();
      sg_pluginPool[pluginPath] = handle;
   }
   return CfgSetterPlugin(pluginPath, handle);
}

void unload_cfg_setter_plugin(const std::string &pluginPath)
{
   if (sg_pluginPool.find(pluginPath) != sg_pluginPool.end()) {
      return;
   }
   if (dlclose(sg_pluginPool.at(pluginPath))) {
      throw std::runtime_error(format_string("dlopen error: %s\n", dlerror()));
   }
}

void *CfgSetterPlugin::getSetterSymbol(const std::string &symbol) const
{
   void *funcPtr = dlsym(m_handle, symbol.c_str());
   if (!funcPtr) {
      throw std::runtime_error(format_string("dlsym error: %s\n", dlerror()));
   }
   dlerror();
   return funcPtr;
}

} // lit
} // polar
