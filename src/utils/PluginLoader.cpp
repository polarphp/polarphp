// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/04.

#define DONT_GET_PLUGIN_LOADER_OPTION
#include "polarphp/utils/PluginLoader.h"
#include "polarphp/utils/DynamicLibrary.h"
#include "polarphp/utils/ManagedStatics.h"
#include "polarphp/utils/RawOutStream.h"
#include <vector>
#include <mutex>

namespace polar {
namespace utils {

static ManagedStatic<std::vector<std::string> > sg_plugins;
static ManagedStatic<std::mutex> sg_pluginsLock;

void PluginLoader::operator=(const std::string &filename)
{
   std::lock_guard locker(*sg_pluginsLock);
   std::string error;
   if (polar::sys::DynamicLibrary::loadLibraryPermanently(filename.c_str(), &error)) {
      error_stream() << "Error opening '" << filename << "': " << error
                     << "\n  -load request ignored.\n";
   } else {
      sg_plugins->push_back(filename);
   }
}

unsigned PluginLoader::getNumPlugins()
{
   std::lock_guard locker(*sg_pluginsLock);
   return sg_plugins.isConstructed() ? sg_plugins->size() : 0;
}

std::string &PluginLoader::getPlugin(unsigned num)
{
   std::lock_guard locker(*sg_pluginsLock);
   assert(sg_plugins.isConstructed() && num < sg_plugins->size() &&
          "Asking for an out of bounds plugin");
   return (*sg_plugins)[num];
}

} // utils
} // polar
