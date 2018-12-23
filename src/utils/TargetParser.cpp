// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/19.

//===----------------------------------------------------------------------===//
//
// This file implements a target parser to recognise hardware features such as
// FPU/cpu/ARCH names as well as specific support such as HDIV, etc.
//
//===----------------------------------------------------------------------===//

#include "polarphp/utils/ARMBuildAttributes.h"
#include "polarphp/utils/TargetParser.h"
#include "polarphp/basic/adt/StringSwitch.h"
#include "polarphp/basic/adt/Twine.h"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/basic/adt/ArrayRef.h"
#include <cctype>

namespace polar {

using namespace amdgpu;
using namespace aarch64;
using polar::basic::StringSwitch;
using polar::basic::StringRef;
using polar::basic::StringLiteral;
using polar::basic::ArrayRef;

namespace {

struct GPUInfo
{
   StringLiteral Name;
   StringLiteral CanonicalName;
   amdgpu::GPUKind Kind;
   unsigned Features;
};

constexpr GPUInfo sg_r600GPUs[26] =
{
   // Name       Canonical    Kind        Features
   //            Name
   {{"r600"},    {"r600"},    GK_R600,    FEATURE_NONE },
   {{"rv630"},   {"r600"},    GK_R600,    FEATURE_NONE },
   {{"rv635"},   {"r600"},    GK_R600,    FEATURE_NONE },
   {{"r630"},    {"r630"},    GK_R630,    FEATURE_NONE },
   {{"rs780"},   {"rs880"},   GK_RS880,   FEATURE_NONE },
   {{"rs880"},   {"rs880"},   GK_RS880,   FEATURE_NONE },
   {{"rv610"},   {"rs880"},   GK_RS880,   FEATURE_NONE },
   {{"rv620"},   {"rs880"},   GK_RS880,   FEATURE_NONE },
   {{"rv670"},   {"rv670"},   GK_RV670,   FEATURE_NONE },
   {{"rv710"},   {"rv710"},   GK_RV710,   FEATURE_NONE },
   {{"rv730"},   {"rv730"},   GK_RV730,   FEATURE_NONE },
   {{"rv740"},   {"rv770"},   GK_RV770,   FEATURE_NONE },
   {{"rv770"},   {"rv770"},   GK_RV770,   FEATURE_NONE },
   {{"cedar"},   {"cedar"},   GK_CEDAR,   FEATURE_NONE },
   {{"palm"},    {"cedar"},   GK_CEDAR,   FEATURE_NONE },
   {{"cypress"}, {"cypress"}, GK_CYPRESS, FEATURE_FMA  },
   {{"hemlock"}, {"cypress"}, GK_CYPRESS, FEATURE_FMA  },
   {{"juniper"}, {"juniper"}, GK_JUNIPER, FEATURE_NONE },
   {{"redwood"}, {"redwood"}, GK_REDWOOD, FEATURE_NONE },
   {{"sumo"},    {"sumo"},    GK_SUMO,    FEATURE_NONE },
   {{"sumo2"},   {"sumo"},    GK_SUMO,    FEATURE_NONE },
   {{"barts"},   {"barts"},   GK_BARTS,   FEATURE_NONE },
   {{"caicos"},  {"caicos"},  GK_CAICOS,  FEATURE_NONE },
   {{"aruba"},   {"cayman"},  GK_CAYMAN,  FEATURE_FMA  },
   {{"cayman"},  {"cayman"},  GK_CAYMAN,  FEATURE_FMA  },
   {{"turks"},   {"turks"},   GK_TURKS,   FEATURE_NONE }
};

// This table should be sorted by the value of GPUKind
// Don't bother listing the implicitly true features
constexpr GPUInfo sg_amdGCNGPUs[33] = {
   // Name         Canonical    Kind        Features
   //              Name
   {{"gfx600"},    {"gfx600"},  GK_GFX600,  FEATURE_FAST_FMA_F32},
   {{"tahiti"},    {"gfx600"},  GK_GFX600,  FEATURE_FAST_FMA_F32},
   {{"gfx601"},    {"gfx601"},  GK_GFX601,  FEATURE_NONE},
   {{"hainan"},    {"gfx601"},  GK_GFX601,  FEATURE_NONE},
   {{"oland"},     {"gfx601"},  GK_GFX601,  FEATURE_NONE},
   {{"pitcairn"},  {"gfx601"},  GK_GFX601,  FEATURE_NONE},
   {{"verde"},     {"gfx601"},  GK_GFX601,  FEATURE_NONE},
   {{"gfx700"},    {"gfx700"},  GK_GFX700,  FEATURE_NONE},
   {{"kaveri"},    {"gfx700"},  GK_GFX700,  FEATURE_NONE},
   {{"gfx701"},    {"gfx701"},  GK_GFX701,  FEATURE_FAST_FMA_F32},
   {{"hawaii"},    {"gfx701"},  GK_GFX701,  FEATURE_FAST_FMA_F32},
   {{"gfx702"},    {"gfx702"},  GK_GFX702,  FEATURE_FAST_FMA_F32},
   {{"gfx703"},    {"gfx703"},  GK_GFX703,  FEATURE_NONE},
   {{"kabini"},    {"gfx703"},  GK_GFX703,  FEATURE_NONE},
   {{"mullins"},   {"gfx703"},  GK_GFX703,  FEATURE_NONE},
   {{"gfx704"},    {"gfx704"},  GK_GFX704,  FEATURE_NONE},
   {{"bonaire"},   {"gfx704"},  GK_GFX704,  FEATURE_NONE},
   {{"gfx801"},    {"gfx801"},  GK_GFX801,  FEATURE_FAST_FMA_F32|FEATURE_FAST_DENORMAL_F32},
   {{"carrizo"},   {"gfx801"},  GK_GFX801,  FEATURE_FAST_FMA_F32|FEATURE_FAST_DENORMAL_F32},
   {{"gfx802"},    {"gfx802"},  GK_GFX802,  FEATURE_FAST_DENORMAL_F32},
   {{"iceland"},   {"gfx802"},  GK_GFX802,  FEATURE_FAST_DENORMAL_F32},
   {{"tonga"},     {"gfx802"},  GK_GFX802,  FEATURE_FAST_DENORMAL_F32},
   {{"gfx803"},    {"gfx803"},  GK_GFX803,  FEATURE_FAST_DENORMAL_F32},
   {{"fiji"},      {"gfx803"},  GK_GFX803,  FEATURE_FAST_DENORMAL_F32},
   {{"polaris10"}, {"gfx803"},  GK_GFX803,  FEATURE_FAST_DENORMAL_F32},
   {{"polaris11"}, {"gfx803"},  GK_GFX803,  FEATURE_FAST_DENORMAL_F32},
   {{"gfx810"},    {"gfx810"},  GK_GFX810,  FEATURE_FAST_DENORMAL_F32},
   {{"stoney"},    {"gfx810"},  GK_GFX810,  FEATURE_FAST_DENORMAL_F32},
   {{"gfx900"},    {"gfx900"},  GK_GFX900,  FEATURE_FAST_FMA_F32|FEATURE_FAST_DENORMAL_F32},
   {{"gfx902"},    {"gfx902"},  GK_GFX902,  FEATURE_FAST_FMA_F32|FEATURE_FAST_DENORMAL_F32},
   {{"gfx904"},    {"gfx904"},  GK_GFX904,  FEATURE_FAST_FMA_F32|FEATURE_FAST_DENORMAL_F32},
   {{"gfx906"},    {"gfx906"},  GK_GFX906,  FEATURE_FAST_FMA_F32|FEATURE_FAST_DENORMAL_F32},
   {{"gfx909"},    {"gfx909"},  GK_GFX909,  FEATURE_FAST_FMA_F32|FEATURE_FAST_DENORMAL_F32},
};

const GPUInfo *get_arch_entry(amdgpu::GPUKind archKind, ArrayRef<GPUInfo> table)
{
   GPUInfo search = { {""}, {""}, archKind, amdgpu::FEATURE_NONE };
   auto iter = std::lower_bound(table.begin(), table.end(), search,
                                [](const GPUInfo &lhs, const GPUInfo &rhs) {
      return lhs.Kind < rhs.Kind;
   });
   if (iter == table.end()) {
      return nullptr;
   }
   return iter;
}

} // namespace

StringRef amdgpu::get_arch_name_amd_gcn(GPUKind archKind)
{
   if (const auto *entry = get_arch_entry(archKind, sg_amdGCNGPUs)) {
      return entry->CanonicalName;
   }
   return "";
}

StringRef amdgpu::get_arch_name_r600(GPUKind archKind)
{
   if (const auto *entry = get_arch_entry(archKind, sg_r600GPUs)) {
      return entry->CanonicalName;
   }
   return "";
}

amdgpu::GPUKind amdgpu::parse_arch_amd_gcn(StringRef cpu)
{
   for (const auto gpu : sg_amdGCNGPUs) {
      if (cpu == gpu.Name) {
         return gpu.Kind;
      }
   }
   return amdgpu::GPUKind::GK_NONE;
}

amdgpu::GPUKind amdgpu::parse_arch_r600(StringRef cpu) {
   for (const auto item : sg_r600GPUs) {
      if (cpu == item.Name) {
         return item.Kind;
      }
   }
   return amdgpu::GPUKind::GK_NONE;
}

unsigned amdgpu::get_arch_attr_amd_gcn(GPUKind archKind)
{
   if (const auto *entry = get_arch_entry(archKind, sg_amdGCNGPUs)) {
      return entry->Features;
   }
   return FEATURE_NONE;
}

unsigned amdgpu::get_arch_attr_r600(GPUKind archKind)
{
   if (const auto *entry = get_arch_entry(archKind, sg_r600GPUs)) {
      return entry->Features;
   }
   return FEATURE_NONE;
}

void amdgpu::fill_valid_arch_list_amd_gcn(SmallVectorImpl<StringRef> &values)
{
   // XXX: Should this only report unique canonical names?
   for (const auto item : sg_amdGCNGPUs) {
      values.push_back(item.Name);
   }
}

void amdgpu::fill_valid_arch_list_r600(SmallVectorImpl<StringRef> &values)
{
   for (const auto gpu : sg_r600GPUs)
      values.push_back(gpu.Name);
}

amdgpu::IsaVersion amdgpu::get_isa_version(StringRef gpu)
{
   if (gpu == "generic") {
      return {7, 0, 0};
   }
   amdgpu::GPUKind archKind = parse_arch_amd_gcn(gpu);
   if (archKind == amdgpu::GPUKind::GK_NONE) {
      return {0, 0, 0};
   }
   switch (archKind) {
   case GK_GFX600: return {6, 0, 0};
   case GK_GFX601: return {6, 0, 1};
   case GK_GFX700: return {7, 0, 0};
   case GK_GFX701: return {7, 0, 1};
   case GK_GFX702: return {7, 0, 2};
   case GK_GFX703: return {7, 0, 3};
   case GK_GFX704: return {7, 0, 4};
   case GK_GFX801: return {8, 0, 1};
   case GK_GFX802: return {8, 0, 2};
   case GK_GFX803: return {8, 0, 3};
   case GK_GFX810: return {8, 1, 0};
   case GK_GFX900: return {9, 0, 0};
   case GK_GFX902: return {9, 0, 2};
   case GK_GFX904: return {9, 0, 4};
   case GK_GFX906: return {9, 0, 6};
   case GK_GFX909: return {9, 0, 9};
   default:        return {0, 0, 0};
   }
}

} // polar
