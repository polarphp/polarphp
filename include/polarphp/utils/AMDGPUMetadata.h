//===--- AMDGPUMetadata.h ---------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/10/17.

#ifndef POLARPHP_UTILS_AMD_GPU_METADATA_H
#define POLARPHP_UTILS_AMD_GPU_METADATA_H

#include <cstdint>
#include <string>
#include <system_error>
#include <vector>

namespace polar {
namespace amdgpu {
namespace hsamd {

//===----------------------------------------------------------------------===//
// HSA metadata.
//===----------------------------------------------------------------------===//
/// HSA metadata major version.
constexpr uint32_t versionMajor = 1;
/// HSA metadata minor version.
constexpr uint32_t versionMinor = 0;

/// HSA metadata beginning assembler directive.
constexpr char assemblerDirectiveBegin[] = ".amd_amdgpu_hsa_metadata";
/// HSA metadata ending assembler directive.
constexpr char assemblerDirectiveEnd[] = ".end_amd_amdgpu_hsa_metadata";

/// Access qualifiers.
enum class AccessQualifier : uint8_t
{
   Default   = 0,
   ReadOnly  = 1,
   WriteOnly = 2,
   ReadWrite = 3,
   Unknown   = 0xff
};

/// Address space qualifiers.
enum class AddressSpaceQualifier : uint8_t
{
   Private  = 0,
   Global   = 1,
   Constant = 2,
   Local    = 3,
   Generic  = 4,
   Region   = 5,
   Unknown  = 0xff
};

/// Value kinds.
enum class ValueKind : uint8_t
{
   ByValue                = 0,
   GlobalBuffer           = 1,
   DynamicSharedPointer   = 2,
   Sampler                = 3,
   Image                  = 4,
   Pipe                   = 5,
   Queue                  = 6,
   HiddenGlobalOffsetX    = 7,
   HiddenGlobalOffsetY    = 8,
   HiddenGlobalOffsetZ    = 9,
   HiddenNone             = 10,
   HiddenPrintfBuffer     = 11,
   HiddenDefaultQueue     = 12,
   HiddenCompletionAction = 13,
   Unknown                = 0xff
};

/// Value types.
enum class ValueType : uint8_t
{
   Struct  = 0,
   I8      = 1,
   U8      = 2,
   I16     = 3,
   U16     = 4,
   F16     = 5,
   I32     = 6,
   U32     = 7,
   F32     = 8,
   I64     = 9,
   U64     = 10,
   F64     = 11,
   Unknown = 0xff
};

//===----------------------------------------------------------------------===//
// kernel Metadata.
//===----------------------------------------------------------------------===//
namespace kernel {

//===----------------------------------------------------------------------===//
// kernel Attributes Metadata.
//===----------------------------------------------------------------------===//
namespace attrs {

namespace key {
/// key for kernel::Attr::Metadata::m_reqdWorkGroupSize.
constexpr char reqdWorkGroupSize[] = "ReqdWorkGroupSize";
/// key for kernel::Attr::Metadata::m_workGroupSizeHint.
constexpr char workGroupSizeHint[] = "WorkGroupSizeHint";
/// key for kernel::Attr::Metadata::m_vecTypeHint.
constexpr char vecTypeHint[] = "VecTypeHint";
/// key for kernel::Attr::Metadata::m_runtimeHandle.
constexpr char runtimeHandle[] = "RuntimeHandle";
} // end namespace key

/// In-memory representation of kernel attributes metadata.
struct Metadata final
{
   /// 'reqd_work_group_size' attribute. Optional.
   std::vector<uint32_t> m_reqdWorkGroupSize = std::vector<uint32_t>();
   /// 'work_group_size_hint' attribute. Optional.
   std::vector<uint32_t> m_workGroupSizeHint = std::vector<uint32_t>();
   /// 'vec_type_hint' attribute. Optional.
   std::string m_vecTypeHint = std::string();
   /// External symbol created by runtime to store the kernel address
   /// for enqueued blocks.
   std::string m_runtimeHandle = std::string();

   /// Default constructor.
   Metadata() = default;

   /// \returns True if kernel attributes metadata is empty, false otherwise.
   bool empty() const
   {
      return !notEmpty();
   }

   /// \returns True if kernel attributes metadata is not empty, false otherwise.
   bool notEmpty() const
   {
      return !m_reqdWorkGroupSize.empty() || !m_workGroupSizeHint.empty() ||
            !m_vecTypeHint.empty() || !m_runtimeHandle.empty();
   }
};

} // end namespace attrs

//===----------------------------------------------------------------------===//
// kernel Argument Metadata.
//===----------------------------------------------------------------------===//
namespace arg {

namespace key {
/// key for kernel::arg::Metadata::m_name.
constexpr char name[] = "Name";
/// key for kernel::arg::Metadata::m_typeName.
constexpr char typeName[] = "TypeName";
/// key for kernel::arg::Metadata::m_size.
constexpr char size[] = "Size";
/// key for kernel::arg::Metadata::m_align.
constexpr char align[] = "Align";
/// key for kernel::arg::Metadata::m_valueKind.
constexpr char valueKind[] = "ValueKind";
/// key for kernel::arg::Metadata::m_valueType.
constexpr char valueType[] = "ValueType";
/// key for kernel::arg::Metadata::m_pointeeAlign.
constexpr char pointeeAlign[] = "PointeeAlign";
/// key for kernel::arg::Metadata::m_addrSpaceQual.
constexpr char addrSpaceQual[] = "AddrSpaceQual";
/// key for kernel::arg::Metadata::m_accQual.
constexpr char accQual[] = "AccQual";
/// key for kernel::arg::Metadata::m_actualAccQual.
constexpr char actualAccQual[] = "ActualAccQual";
/// key for kernel::arg::Metadata::m_isConst.
constexpr char isConst[] = "IsConst";
/// key for kernel::arg::Metadata::m_isRestrict.
constexpr char isRestrict[] = "IsRestrict";
/// key for kernel::arg::Metadata::m_isVolatile.
constexpr char isVolatile[] = "IsVolatile";
/// key for kernel::arg::Metadata::m_isPipe.
constexpr char isPipe[] = "IsPipe";
} // end namespace key

/// In-memory representation of kernel argument metadata.
struct Metadata final
{
   /// Name. Optional.
   std::string m_name = std::string();
   /// Type name. Optional.
   std::string m_typeName = std::string();
   /// Size in bytes. Required.
   uint32_t m_size = 0;
   /// Alignment in bytes. Required.
   uint32_t m_align = 0;
   /// Value kind. Required.
   ValueKind m_valueKind = ValueKind::Unknown;
   /// Value type. Required.
   ValueType m_valueType = ValueType::Unknown;
   /// Pointee alignment in bytes. Optional.
   uint32_t m_pointeeAlign = 0;
   /// Address space qualifier. Optional.
   AddressSpaceQualifier m_addrSpaceQual = AddressSpaceQualifier::Unknown;
   /// Access qualifier. Optional.
   AccessQualifier m_accQual = AccessQualifier::Unknown;
   /// Actual access qualifier. Optional.
   AccessQualifier m_actualAccQual = AccessQualifier::Unknown;
   /// True if 'const' qualifier is specified. Optional.
   bool m_isConst = false;
   /// True if 'restrict' qualifier is specified. Optional.
   bool m_isRestrict = false;
   /// True if 'volatile' qualifier is specified. Optional.
   bool m_isVolatile = false;
   /// True if 'pipe' qualifier is specified. Optional.
   bool m_isPipe = false;

   /// Default constructor.
   Metadata() = default;
};

} // end namespace arg

//===----------------------------------------------------------------------===//
// kernel Code Properties Metadata.
//===----------------------------------------------------------------------===//
namespace codeProps {

namespace key {
/// key for kernel::codeProps::Metadata::mKernargSegmentSize.
constexpr char kernargSegmentSize[] = "KernargSegmentSize";
/// key for kernel::codeProps::Metadata::mGroupSegmentFixedSize.
constexpr char groupSegmentFixedSize[] = "GroupSegmentFixedSize";
/// key for kernel::codeProps::Metadata::mPrivateSegmentFixedSize.
constexpr char privateSegmentFixedSize[] = "PrivateSegmentFixedSize";
/// key for kernel::codeProps::Metadata::mKernargSegmentAlign.
constexpr char kernargSegmentAlign[] = "KernargSegmentAlign";
/// key for kernel::codeProps::Metadata::mWavefrontSize.
constexpr char wavefrontSize[] = "WavefrontSize";
/// key for kernel::codeProps::Metadata::mNumSGPRs.
constexpr char numSGPRs[] = "NumSGPRs";
/// key for kernel::codeProps::Metadata::mNumVGPRs.
constexpr char numVGPRs[] = "NumVGPRs";
/// key for kernel::codeProps::Metadata::mMaxFlatWorkGroupSize.
constexpr char maxFlatWorkGroupSize[] = "MaxFlatWorkGroupSize";
/// key for kernel::codeProps::Metadata::mIsDynamicCallStack.
constexpr char isDynamicCallStack[] = "IsDynamicCallStack";
/// key for kernel::codeProps::Metadata::mIsXNACKEnabled.
constexpr char isXNACKEnabled[] = "IsXNACKEnabled";
/// key for kernel::codeProps::Metadata::mNumSpilledSGPRs.
constexpr char numSpilledSGPRs[] = "NumSpilledSGPRs";
/// key for kernel::codeProps::Metadata::mNumSpilledVGPRs.
constexpr char numSpilledVGPRs[] = "NumSpilledVGPRs";
} // end namespace key

/// In-memory representation of kernel code properties metadata.
struct Metadata final
{
   /// Size in bytes of the kernarg segment memory. Kernarg segment memory
   /// holds the values of the arguments to the kernel. Required.
   uint64_t m_kernargSegmentSize = 0;
   /// Size in bytes of the group segment memory required by a workgroup.
   /// This value does not include any dynamically allocated group segment memory
   /// that may be added when the kernel is dispatched. Required.
   uint32_t m_groupSegmentFixedSize = 0;
   /// Size in bytes of the private segment memory required by a workitem.
   /// Private segment memory includes arg, spill and private segments. Required.
   uint32_t m_privateSegmentFixedSize = 0;
   /// Maximum byte alignment of variables used by the kernel in the
   /// kernarg memory segment. Required.
   uint32_t m_kernargSegmentAlign = 0;
   /// Wavefront size. Required.
   uint32_t m_wavefrontSize = 0;
   /// Total number of SGPRs used by a wavefront. Optional.
   uint16_t m_numSGPRs = 0;
   /// Total number of VGPRs used by a workitem. Optional.
   uint16_t m_numVGPRs = 0;
   /// Maximum flat work-group size supported by the kernel. Optional.
   uint32_t m_maxFlatWorkGroupSize = 0;
   /// True if the generated machine code is using a dynamically sized
   /// call stack. Optional.
   bool m_isDynamicCallStack = false;
   /// True if the generated machine code is capable of supporting XNACK.
   /// Optional.
   bool m_isXNACKEnabled = false;
   /// Number of SGPRs spilled by a wavefront. Optional.
   uint16_t m_numSpilledSGPRs = 0;
   /// Number of VGPRs spilled by a workitem. Optional.
   uint16_t m_numSpilledVGPRs = 0;

   /// Default constructor.
   Metadata() = default;

   /// \returns True if kernel code properties metadata is empty, false
   /// otherwise.
   bool empty() const
   {
      return !notEmpty();
   }

   /// \returns True if kernel code properties metadata is not empty, false
   /// otherwise.
   bool notEmpty() const
   {
      return true;
   }
};

} // end namespace codeProps

//===----------------------------------------------------------------------===//
// kernel Debug Properties Metadata.
//===----------------------------------------------------------------------===//
namespace debugprops {
namespace key {
/// key for kernel::debugprops::Metadata::m_debuggerABIVersion.
constexpr char debuggerABIVersion[] = "DebuggerABIVersion";
/// key for kernel::debugprops::Metadata::m_reservedNumVGPRs.
constexpr char reservedNumVGPRs[] = "ReservedNumVGPRs";
/// key for kernel::debugprops::Metadata::m_reservedFirstVGPR.
constexpr char reservedFirstVGPR[] = "ReservedFirstVGPR";
/// key for kernel::debugprops::Metadata::m_privateSegmentBufferSGPR.
constexpr char privateSegmentBufferSGPR[] = "PrivateSegmentBufferSGPR";
/// key for
///     kernel::debugprops::Metadata::m_wavefrontPrivateSegmentOffsetSGPR.
constexpr char wavefrontPrivateSegmentOffsetSGPR[] =
      "WavefrontPrivateSegmentOffsetSGPR";
} // end namespace key

/// In-memory representation of kernel debug properties metadata.
struct Metadata final
{
   /// Debugger ABI version. Optional.
   std::vector<uint32_t> m_debuggerABIVersion = std::vector<uint32_t>();
   /// Consecutive number of VGPRs reserved for debugger use. Must be 0 if
   /// m_debuggerABIVersion is not set. Optional.
   uint16_t m_reservedNumVGPRs = 0;
   /// First fixed VGPR reserved. Must be uint16_t(-1) if
   /// m_debuggerABIVersion is not set or m_reservedFirstVGPR is 0. Optional.
   uint16_t m_reservedFirstVGPR = uint16_t(-1);
   /// Fixed SGPR of the first of 4 SGPRs used to hold the scratch V# used
   /// for the entire kernel execution. Must be uint16_t(-1) if
   /// m_debuggerABIVersion is not set or SGPR not used or not known. Optional.
   uint16_t m_privateSegmentBufferSGPR = uint16_t(-1);
   /// Fixed SGPR used to hold the wave scratch offset for the entire
   /// kernel execution. Must be uint16_t(-1) if m_debuggerABIVersion is not set
   /// or SGPR is not used or not known. Optional.
   uint16_t m_wavefrontPrivateSegmentOffsetSGPR = uint16_t(-1);

   /// Default constructor.
   Metadata() = default;

   /// \returns True if kernel debug properties metadata is empty, false
   /// otherwise.
   bool empty() const
   {
      return !notEmpty();
   }

   /// \returns True if kernel debug properties metadata is not empty, false
   /// otherwise.
   bool notEmpty() const
   {
      return !m_debuggerABIVersion.empty();
   }
};

} // end namespace debugprops

namespace key {
/// key for kernel::Metadata::m_name.
constexpr char name[] = "Name";
/// key for kernel::Metadata::m_symbolName.
constexpr char symbolName[] = "SymbolName";
/// key for kernel::Metadata::m_language.
constexpr char language[] = "Language";
/// key for kernel::Metadata::m_languageVersion.
constexpr char languageVersion[] = "LanguageVersion";
/// key for kernel::Metadata::m_attrs.
constexpr char attrs[] = "Attrs";
/// key for kernel::Metadata::m_args.
constexpr char args[] = "Args";
/// key for kernel::Metadata::m_codeProps.
constexpr char codeProps[] = "CodeProps";
/// key for kernel::Metadata::m_debugProps.
constexpr char debugprops[] = "Debugprops";
} // end namespace key

/// In-memory representation of kernel metadata.
struct Metadata final
{
   /// kernel source name. Required.
   std::string m_name = std::string();
   /// kernel descriptor name. Required.
   std::string m_symbolName = std::string();
   /// Language. Optional.
   std::string m_language = std::string();
   /// Language version. Optional.
   std::vector<uint32_t> m_languageVersion = std::vector<uint32_t>();
   /// Attributes metadata. Optional.
   attrs::Metadata m_attrs = attrs::Metadata();
   /// Arguments metadata. Optional.
   std::vector<arg::Metadata> m_args = std::vector<arg::Metadata>();
   /// Code properties metadata. Optional.
   codeProps::Metadata m_codeProps = codeProps::Metadata();
   /// Debug properties metadata. Optional.
   debugprops::Metadata m_debugProps = debugprops::Metadata();

   /// Default constructor.
   Metadata() = default;
};

} // end namespace kernel

namespace key {
/// key for HSA::Metadata::mVersion.
constexpr char version[] = "Version";
/// key for HSA::Metadata::mPrintf.
constexpr char printf[] = "Printf";
/// key for HSA::Metadata::mKernels.
constexpr char kernels[] = "Kernels";
} // end namespace key

/// In-memory representation of HSA metadata.
struct Metadata final
{
   /// HSA metadata version. Required.
   std::vector<uint32_t> m_version = std::vector<uint32_t>();
   /// Printf metadata. Optional.
   std::vector<std::string> m_printf = std::vector<std::string>();
   /// Kernels metadata. Required.
   std::vector<kernel::Metadata> m_kernels = std::vector<kernel::Metadata>();

   /// Default constructor.
   Metadata() = default;
};

/// Converts \p str to \p hsaMetadata.
std::error_code fromString(std::string str, Metadata &hsaMetadata);

/// Converts \p hsaMetadata to \p str.
std::error_code toString(Metadata hsaMetadata, std::string &str);

//===----------------------------------------------------------------------===//
// HSA metadata for v3 code object.
//===----------------------------------------------------------------------===//
namespace v3 {
/// HSA metadata major version.
constexpr uint32_t versionMajor = 1;
/// HSA metadata minor version.
constexpr uint32_t versionMinor = 0;

/// HSA metadata beginning assembler directive.
constexpr char assemblerDirectiveBegin[] = ".amdgpu_metadata";
/// HSA metadata ending assembler directive.
constexpr char assemblerDirectiveEnd[] = ".end_amdgpu_metadata";
} // end namespace V3

} // end namespace hsamd

//===----------------------------------------------------------------------===//
// PAL metadata.
//===----------------------------------------------------------------------===//
namespace palmd {

/// PAL metadata assembler directive.
constexpr char assemblerDirective[] = ".amd_amdgpu_pal_metadata";

/// PAL metadata keys.
enum key : uint32_t {
   LS_NUM_USED_VGPRS = 0x10000021,
   HS_NUM_USED_VGPRS = 0x10000022,
   ES_NUM_USED_VGPRS = 0x10000023,
   GS_NUM_USED_VGPRS = 0x10000024,
   VS_NUM_USED_VGPRS = 0x10000025,
   PS_NUM_USED_VGPRS = 0x10000026,
   CS_NUM_USED_VGPRS = 0x10000027,

   LS_NUM_USED_SGPRS = 0x10000028,
   HS_NUM_USED_SGPRS = 0x10000029,
   ES_NUM_USED_SGPRS = 0x1000002a,
   GS_NUM_USED_SGPRS = 0x1000002b,
   VS_NUM_USED_SGPRS = 0x1000002c,
   PS_NUM_USED_SGPRS = 0x1000002d,
   CS_NUM_USED_SGPRS = 0x1000002e,

   LS_SCRATCH_SIZE = 0x10000044,
   HS_SCRATCH_SIZE = 0x10000045,
   ES_SCRATCH_SIZE = 0x10000046,
   GS_SCRATCH_SIZE = 0x10000047,
   VS_SCRATCH_SIZE = 0x10000048,
   PS_SCRATCH_SIZE = 0x10000049,
   CS_SCRATCH_SIZE = 0x1000004a
};

/// PAL metadata represented as a vector.
typedef std::vector<uint32_t> Metadata;

/// Converts \p palMetadata to \p str.
std::error_code toString(const Metadata &palMetadata, std::string &string);

} // end namespace palmd
} // amdgpu
} // polar

#endif // POLARPHP_UTILS_AMD_GPU_METADATA_H
