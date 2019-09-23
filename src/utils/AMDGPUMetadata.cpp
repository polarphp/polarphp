//===--- AMDGPUMetadata.cpp -------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
/// \file
/// AMDGPU metadata definitions and in-memory representations.
///
//
//===----------------------------------------------------------------------===//

// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/04/23.

#include "polarphp/basic/adt/Twine.h"
#include "polarphp/utils/AMDGPUMetadata.h"
#include "polarphp/utils/yaml/YamlTraits.h"
#include "polarphp/utils/RawOutStream.h"

using namespace polar::amdgpu::hsamd;

POLAR_YAML_IS_SEQUENCE_VECTOR(kernel::arg::Metadata)
POLAR_YAML_IS_SEQUENCE_VECTOR(kernel::Metadata)

namespace polar {
namespace yaml {

template <>
struct ScalarEnumerationTraits<AccessQualifier>
{
   static void enumeration(IO &yio, AccessQualifier &en)
   {
      yio.enumCase(en, "Default", AccessQualifier::Default);
      yio.enumCase(en, "ReadOnly", AccessQualifier::ReadOnly);
      yio.enumCase(en, "WriteOnly", AccessQualifier::WriteOnly);
      yio.enumCase(en, "ReadWrite", AccessQualifier::ReadWrite);
   }
};

template <>
struct ScalarEnumerationTraits<AddressSpaceQualifier>
{
   static void enumeration(IO &yio, AddressSpaceQualifier &en)
   {
      yio.enumCase(en, "Private", AddressSpaceQualifier::Private);
      yio.enumCase(en, "Global", AddressSpaceQualifier::Global);
      yio.enumCase(en, "Constant", AddressSpaceQualifier::Constant);
      yio.enumCase(en, "Local", AddressSpaceQualifier::Local);
      yio.enumCase(en, "Generic", AddressSpaceQualifier::Generic);
      yio.enumCase(en, "Region", AddressSpaceQualifier::Region);
   }
};

template <>
struct ScalarEnumerationTraits<ValueKind>
{
   static void enumeration(IO &yio, ValueKind &en)
   {
      yio.enumCase(en, "ByValue", ValueKind::ByValue);
      yio.enumCase(en, "GlobalBuffer", ValueKind::GlobalBuffer);
      yio.enumCase(en, "DynamicSharedPointer", ValueKind::DynamicSharedPointer);
      yio.enumCase(en, "Sampler", ValueKind::Sampler);
      yio.enumCase(en, "Image", ValueKind::Image);
      yio.enumCase(en, "Pipe", ValueKind::Pipe);
      yio.enumCase(en, "Queue", ValueKind::Queue);
      yio.enumCase(en, "HiddenGlobalOffsetX", ValueKind::HiddenGlobalOffsetX);
      yio.enumCase(en, "HiddenGlobalOffsetY", ValueKind::HiddenGlobalOffsetY);
      yio.enumCase(en, "HiddenGlobalOffsetZ", ValueKind::HiddenGlobalOffsetZ);
      yio.enumCase(en, "HiddenNone", ValueKind::HiddenNone);
      yio.enumCase(en, "HiddenPrintfBuffer", ValueKind::HiddenPrintfBuffer);
      yio.enumCase(en, "HiddenDefaultQueue", ValueKind::HiddenDefaultQueue);
      yio.enumCase(en, "HiddenCompletionAction",
                   ValueKind::HiddenCompletionAction);
   }
};

template <>
struct ScalarEnumerationTraits<ValueType>
{
   static void enumeration(IO &yio, ValueType &en)
   {
      yio.enumCase(en, "Struct", ValueType::Struct);
      yio.enumCase(en, "I8", ValueType::I8);
      yio.enumCase(en, "U8", ValueType::U8);
      yio.enumCase(en, "I16", ValueType::I16);
      yio.enumCase(en, "U16", ValueType::U16);
      yio.enumCase(en, "F16", ValueType::F16);
      yio.enumCase(en, "I32", ValueType::I32);
      yio.enumCase(en, "U32", ValueType::U32);
      yio.enumCase(en, "F32", ValueType::F32);
      yio.enumCase(en, "I64", ValueType::I64);
      yio.enumCase(en, "U64", ValueType::U64);
      yio.enumCase(en, "F64", ValueType::F64);
   }
};

template <>
struct MappingTraits<kernel::attrs::Metadata>
{
   static void mapping(IO &yio, kernel::attrs::Metadata &md) {
      yio.mapOptional(kernel::attrs::key::reqdWorkGroupSize,
                      md.m_reqdWorkGroupSize, std::vector<uint32_t>());
      yio.mapOptional(kernel::attrs::key::workGroupSizeHint,
                      md.m_workGroupSizeHint, std::vector<uint32_t>());
      yio.mapOptional(kernel::attrs::key::vecTypeHint,
                      md.m_vecTypeHint, std::string());
      yio.mapOptional(kernel::attrs::key::runtimeHandle, md.m_runtimeHandle,
                      std::string());
   }
};

template <>
struct MappingTraits<kernel::arg::Metadata>
{
   static void mapping(IO &yio, kernel::arg::Metadata &md)
   {
      yio.mapOptional(kernel::arg::key::name, md.m_name, std::string());
      yio.mapOptional(kernel::arg::key::typeName, md.m_typeName, std::string());
      yio.mapRequired(kernel::arg::key::size, md.m_size);
      yio.mapRequired(kernel::arg::key::align, md.m_align);
      yio.mapRequired(kernel::arg::key::valueKind, md.m_valueKind);
      yio.mapRequired(kernel::arg::key::valueType, md.m_valueType);
      yio.mapOptional(kernel::arg::key::pointeeAlign, md.m_pointeeAlign,
                      uint32_t(0));
      yio.mapOptional(kernel::arg::key::addrSpaceQual, md.m_addrSpaceQual,
                      AddressSpaceQualifier::Unknown);
      yio.mapOptional(kernel::arg::key::accQual, md.m_accQual,
                      AccessQualifier::Unknown);
      yio.mapOptional(kernel::arg::key::actualAccQual, md.m_actualAccQual,
                      AccessQualifier::Unknown);
      yio.mapOptional(kernel::arg::key::isConst, md.m_isConst, false);
      yio.mapOptional(kernel::arg::key::isRestrict, md.m_isRestrict, false);
      yio.mapOptional(kernel::arg::key::isVolatile, md.m_isVolatile, false);
      yio.mapOptional(kernel::arg::key::isPipe, md.m_isPipe, false);
   }
};

template <>
struct MappingTraits<kernel::codeProps::Metadata>
{
   static void mapping(IO &yio, kernel::codeProps::Metadata &md) {
      yio.mapRequired(kernel::codeProps::key::kernargSegmentSize,
                      md.m_kernargSegmentSize);
      yio.mapRequired(kernel::codeProps::key::groupSegmentFixedSize,
                      md.m_groupSegmentFixedSize);
      yio.mapRequired(kernel::codeProps::key::privateSegmentFixedSize,
                      md.m_privateSegmentFixedSize);
      yio.mapRequired(kernel::codeProps::key::kernargSegmentAlign,
                      md.m_kernargSegmentAlign);
      yio.mapRequired(kernel::codeProps::key::wavefrontSize,
                      md.m_wavefrontSize);
      yio.mapOptional(kernel::codeProps::key::numSGPRs,
                      md.m_numSGPRs, uint16_t(0));
      yio.mapOptional(kernel::codeProps::key::numVGPRs,
                      md.m_numVGPRs, uint16_t(0));
      yio.mapOptional(kernel::codeProps::key::maxFlatWorkGroupSize,
                      md.m_maxFlatWorkGroupSize, uint32_t(0));
      yio.mapOptional(kernel::codeProps::key::isDynamicCallStack,
                      md.m_isDynamicCallStack, false);
      yio.mapOptional(kernel::codeProps::key::isXNACKEnabled,
                      md.m_isXNACKEnabled, false);
      yio.mapOptional(kernel::codeProps::key::numSpilledSGPRs,
                      md.m_numSpilledSGPRs, uint16_t(0));
      yio.mapOptional(kernel::codeProps::key::numSpilledVGPRs,
                      md.m_numSpilledVGPRs, uint16_t(0));
   }
};

template <>
struct MappingTraits<kernel::debugprops::Metadata>
{
   static void mapping(IO &yio, kernel::debugprops::Metadata &md)
   {
      yio.mapOptional(kernel::debugprops::key::debuggerABIVersion,
                      md.m_debuggerABIVersion, std::vector<uint32_t>());
      yio.mapOptional(kernel::debugprops::key::reservedNumVGPRs,
                      md.m_reservedNumVGPRs, uint16_t(0));
      yio.mapOptional(kernel::debugprops::key::reservedFirstVGPR,
                      md.m_reservedFirstVGPR, uint16_t(-1));
      yio.mapOptional(kernel::debugprops::key::privateSegmentBufferSGPR,
                      md.m_privateSegmentBufferSGPR, uint16_t(-1));
      yio.mapOptional(kernel::debugprops::key::wavefrontPrivateSegmentOffsetSGPR,
                      md.m_wavefrontPrivateSegmentOffsetSGPR, uint16_t(-1));
   }
};

template <>
struct MappingTraits<kernel::Metadata>
{
   static void mapping(IO &yio, kernel::Metadata &md)
   {
      yio.mapRequired(kernel::key::name, md.m_name);
      yio.mapRequired(kernel::key::symbolName, md.m_symbolName);
      yio.mapOptional(kernel::key::language, md.m_language, std::string());
      yio.mapOptional(kernel::key::languageVersion, md.m_languageVersion,
                      std::vector<uint32_t>());
      if (!md.m_attrs.empty() || !yio.outputting())
         yio.mapOptional(kernel::key::attrs, md.m_attrs);
      if (!md.m_args.empty() || !yio.outputting())
         yio.mapOptional(kernel::key::args, md.m_args);
      if (!md.m_codeProps.empty() || !yio.outputting())
         yio.mapOptional(kernel::key::codeProps, md.m_codeProps);
      if (!md.m_debugProps.empty() || !yio.outputting())
         yio.mapOptional(kernel::key::debugprops, md.m_debugProps);
   }
};

template <>
struct MappingTraits<Metadata>
{
   static void mapping(IO &yio, Metadata &md)
   {
      yio.mapRequired(key::version, md.m_version);
      yio.mapOptional(key::printf, md.m_printf, std::vector<std::string>());
      if (!md.m_kernels.empty() || !yio.outputting())
         yio.mapOptional(key::kernels, md.m_kernels);
   }
};

} // yaml

using polar::utils::RawStringOutStream;
using polar::basic::Twine;

namespace amdgpu {
namespace hsamd {

std::error_code fromString(std::string str, Metadata &hsaMetadata)
{
   yaml::Input YamlInput(str);
   YamlInput >> hsaMetadata;
   return YamlInput.getError();
}

std::error_code toString(Metadata hsaMetadata, std::string &str)
{
   RawStringOutStream YamlStream(str);
   yaml::Output YamlOutput(YamlStream, nullptr, std::numeric_limits<int>::max());
   YamlOutput << hsaMetadata;
   return std::error_code();
}

} // end namespace hsamd
} // end namespace amdgpu
} // end namespace polar
