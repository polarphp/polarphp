// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://polarphp.org/LICENSE.txt for license information
// See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/09/22.

#ifndef POLAR_DEVLTOOLS_LIT_CFG_SETTER_PLUGIN_LOADER_H
#define POLAR_DEVLTOOLS_LIT_CFG_SETTER_PLUGIN_LOADER_H

#include <string>

namespace polar {
namespace lit {

namespace internal {

void *do_load_cfg_setter_plugin(const std::string &pluginPath, const std::string &pluginRootDir);

} // internal namespace

template <typename InterfaceType>
InterfaceType load_cfg_setter_plugin(const std::string &pluginPath, const std::string &pluginRootDir)
{
   void *handle = internal::do_load_cfg_setter_plugin(pluginPath, pluginRootDir);
   return reinterpret_cast<InterfaceType>(handle);
}

void unload_cfg_setter_plugin(const std::string &pluginPath);

} // lit
} // polar

#endif // POLAR_DEVLTOOLS_LIT_CFG_SETTER_PLUGIN_LOADER_H
