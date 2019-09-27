// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/15.

#include "polarphp/utils/ARMAttributeParser.h"
#include "polarphp/utils/ARMBuildAttributes.h"
#include "gtest/gtest.h"
#include <string>

using namespace polar::basic;
using namespace polar::utils;
using namespace polar;

struct AttributeSection {
  unsigned Tag;
  unsigned Value;

  AttributeSection(unsigned tag, unsigned value) : Tag(tag), Value(value) { }

  void write(RawOutStream &OS) {
    OS.flush();
    // length = length + "aeabi\0" + TagFile + ByteSize + Tag + Value;
    // length = 17 bytes

    OS << 'A' << (uint8_t)17 << (uint8_t)0 << (uint8_t)0 << (uint8_t)0;
    OS << "aeabi" << '\0';
    OS << (uint8_t)1 << (uint8_t)7 << (uint8_t)0 << (uint8_t)0 << (uint8_t)0;
    OS << (uint8_t)Tag << (uint8_t)Value;

  }
};

bool testBuildAttr(unsigned Tag, unsigned Value,
                   unsigned ExpectedTag, unsigned ExpectedValue) {
  std::string buffer;
  RawStringOutStream OS(buffer);
  AttributeSection Section(Tag, Value);
  Section.write(OS);
  ArrayRef<uint8_t> Bytes(
    reinterpret_cast<const uint8_t*>(OS.getStr().c_str()), OS.getStr().size());

  ARMAttributeParser Parser;
  Parser.parse(Bytes, true);

  return (Parser.hasAttribute(ExpectedTag) &&
    Parser.getAttributeValue(ExpectedTag) == ExpectedValue);
}

bool testTagString(unsigned Tag, const char *name) {
  return armbuildattrs::attr_type_as_string(Tag).getStr() == name;
}

TEST(CPUArchBuildAttr, testBuildAttr) {
  EXPECT_TRUE(testTagString(6, "Tag_CPU_arch"));

  EXPECT_TRUE(testBuildAttr(6, 0, armbuildattrs::CPU_arch,
                            armbuildattrs::Pre_v4));
  EXPECT_TRUE(testBuildAttr(6, 1, armbuildattrs::CPU_arch,
                            armbuildattrs::v4));
  EXPECT_TRUE(testBuildAttr(6, 2, armbuildattrs::CPU_arch,
                               armbuildattrs::v4T));
  EXPECT_TRUE(testBuildAttr(6, 3, armbuildattrs::CPU_arch,
                               armbuildattrs::v5T));
  EXPECT_TRUE(testBuildAttr(6, 4, armbuildattrs::CPU_arch,
                               armbuildattrs::v5TE));
  EXPECT_TRUE(testBuildAttr(6, 5, armbuildattrs::CPU_arch,
                               armbuildattrs::v5TEJ));
  EXPECT_TRUE(testBuildAttr(6, 6, armbuildattrs::CPU_arch,
                               armbuildattrs::v6));
  EXPECT_TRUE(testBuildAttr(6, 7, armbuildattrs::CPU_arch,
                               armbuildattrs::v6KZ));
  EXPECT_TRUE(testBuildAttr(6, 8, armbuildattrs::CPU_arch,
                               armbuildattrs::v6T2));
  EXPECT_TRUE(testBuildAttr(6, 9, armbuildattrs::CPU_arch,
                               armbuildattrs::v6K));
  EXPECT_TRUE(testBuildAttr(6, 10, armbuildattrs::CPU_arch,
                               armbuildattrs::v7));
  EXPECT_TRUE(testBuildAttr(6, 11, armbuildattrs::CPU_arch,
                               armbuildattrs::v6_M));
  EXPECT_TRUE(testBuildAttr(6, 12, armbuildattrs::CPU_arch,
                               armbuildattrs::v6S_M));
  EXPECT_TRUE(testBuildAttr(6, 13, armbuildattrs::CPU_arch,
                               armbuildattrs::v7E_M));
  EXPECT_TRUE(testBuildAttr(6, 14, armbuildattrs::CPU_arch,
                               armbuildattrs::v8_A));
  EXPECT_TRUE(testBuildAttr(6, 15, armbuildattrs::CPU_arch,
                               armbuildattrs::v8_R));
  EXPECT_TRUE(testBuildAttr(6, 16, armbuildattrs::CPU_arch,
                               armbuildattrs::v8_M_Base));
  EXPECT_TRUE(testBuildAttr(6, 17, armbuildattrs::CPU_arch,
                               armbuildattrs::v8_M_Main));
  EXPECT_TRUE(testBuildAttr(6, 21, armbuildattrs::CPU_arch,
                               armbuildattrs::v8_1_M_Main));
}

TEST(CPUArchProfileBuildAttr, testBuildAttr) {
  EXPECT_TRUE(testTagString(7, "Tag_CPU_arch_profile"));
  EXPECT_TRUE(testBuildAttr(7, 'A', armbuildattrs::CPU_arch_profile,
                               armbuildattrs::ApplicationProfile));
  EXPECT_TRUE(testBuildAttr(7, 'R', armbuildattrs::CPU_arch_profile,
                               armbuildattrs::RealTimeProfile));
  EXPECT_TRUE(testBuildAttr(7, 'M', armbuildattrs::CPU_arch_profile,
                               armbuildattrs::MicroControllerProfile));
  EXPECT_TRUE(testBuildAttr(7, 'S', armbuildattrs::CPU_arch_profile,
                               armbuildattrs::SystemProfile));
}

TEST(ARMISABuildAttr, testBuildAttr) {
  EXPECT_TRUE(testTagString(8, "Tag_ARM_ISA_use"));
  EXPECT_TRUE(testBuildAttr(8, 0, armbuildattrs::ARM_ISA_use,
                               armbuildattrs::Not_Allowed));
  EXPECT_TRUE(testBuildAttr(8, 1, armbuildattrs::ARM_ISA_use,
                               armbuildattrs::Allowed));
}

TEST(ThumbISABuildAttr, testBuildAttr) {
  EXPECT_TRUE(testTagString(9, "Tag_THUMB_ISA_use"));
  EXPECT_TRUE(testBuildAttr(9, 0, armbuildattrs::THUMB_ISA_use,
                               armbuildattrs::Not_Allowed));
  EXPECT_TRUE(testBuildAttr(9, 1, armbuildattrs::THUMB_ISA_use,
                               armbuildattrs::Allowed));
}

TEST(FPArchBuildAttr, testBuildAttr) {
  EXPECT_TRUE(testTagString(10, "Tag_FP_arch"));
  EXPECT_TRUE(testBuildAttr(10, 0, armbuildattrs::FP_arch,
                               armbuildattrs::Not_Allowed));
  EXPECT_TRUE(testBuildAttr(10, 1, armbuildattrs::FP_arch,
                               armbuildattrs::Allowed));
  EXPECT_TRUE(testBuildAttr(10, 2, armbuildattrs::FP_arch,
                               armbuildattrs::AllowFPv2));
  EXPECT_TRUE(testBuildAttr(10, 3, armbuildattrs::FP_arch,
                               armbuildattrs::AllowFPv3A));
  EXPECT_TRUE(testBuildAttr(10, 4, armbuildattrs::FP_arch,
                               armbuildattrs::AllowFPv3B));
  EXPECT_TRUE(testBuildAttr(10, 5, armbuildattrs::FP_arch,
                               armbuildattrs::AllowFPv4A));
  EXPECT_TRUE(testBuildAttr(10, 6, armbuildattrs::FP_arch,
                               armbuildattrs::AllowFPv4B));
  EXPECT_TRUE(testBuildAttr(10, 7, armbuildattrs::FP_arch,
                               armbuildattrs::AllowFPARMv8A));
  EXPECT_TRUE(testBuildAttr(10, 8, armbuildattrs::FP_arch,
                               armbuildattrs::AllowFPARMv8B));
}

TEST(WMMXBuildAttr, testBuildAttr) {
  EXPECT_TRUE(testTagString(11, "Tag_WMMX_arch"));
  EXPECT_TRUE(testBuildAttr(11, 0, armbuildattrs::WMMX_arch,
                            armbuildattrs::Not_Allowed));
  EXPECT_TRUE(testBuildAttr(11, 1, armbuildattrs::WMMX_arch,
                            armbuildattrs::AllowWMMXv1));
  EXPECT_TRUE(testBuildAttr(11, 2, armbuildattrs::WMMX_arch,
                            armbuildattrs::AllowWMMXv2));
}

TEST(SIMDBuildAttr, testBuildAttr) {
  EXPECT_TRUE(testTagString(12, "Tag_Advanced_SIMD_arch"));
  EXPECT_TRUE(testBuildAttr(12, 0, armbuildattrs::Advanced_SIMD_arch,
                            armbuildattrs::Not_Allowed));
  EXPECT_TRUE(testBuildAttr(12, 1, armbuildattrs::Advanced_SIMD_arch,
                            armbuildattrs::AllowNeon));
  EXPECT_TRUE(testBuildAttr(12, 2, armbuildattrs::Advanced_SIMD_arch,
                            armbuildattrs::AllowNeon2));
  EXPECT_TRUE(testBuildAttr(12, 3, armbuildattrs::Advanced_SIMD_arch,
                            armbuildattrs::AllowNeonARMv8));
  EXPECT_TRUE(testBuildAttr(12, 4, armbuildattrs::Advanced_SIMD_arch,
                            armbuildattrs::AllowNeonARMv8_1a));
}

TEST(FPHPBuildAttr, testBuildAttr) {
  EXPECT_TRUE(testTagString(36, "Tag_FP_HP_extension"));
  EXPECT_TRUE(testBuildAttr(36, 0, armbuildattrs::FP_HP_extension,
                            armbuildattrs::Not_Allowed));
  EXPECT_TRUE(testBuildAttr(36, 1, armbuildattrs::FP_HP_extension,
                            armbuildattrs::AllowHPFP));
}

TEST(MVEBuildAttr, testBuildAttr) {
  EXPECT_TRUE(testTagString(48, "Tag_MVE_arch"));
  EXPECT_TRUE(testBuildAttr(48, 0, armbuildattrs::MVE_arch,
                            armbuildattrs::Not_Allowed));
  EXPECT_TRUE(testBuildAttr(48, 1, armbuildattrs::MVE_arch,
                            armbuildattrs::AllowMVEInteger));
  EXPECT_TRUE(testBuildAttr(48, 2, armbuildattrs::MVE_arch,
                            armbuildattrs::AllowMVEIntegerAndFloat));
}

TEST(CPUAlignBuildAttr, testBuildAttr) {
  EXPECT_TRUE(testTagString(34, "Tag_CPU_unaligned_access"));
  EXPECT_TRUE(testBuildAttr(34, 0, armbuildattrs::CPU_unaligned_access,
                            armbuildattrs::Not_Allowed));
  EXPECT_TRUE(testBuildAttr(34, 1, armbuildattrs::CPU_unaligned_access,
                            armbuildattrs::Allowed));
}

TEST(T2EEBuildAttr, testBuildAttr) {
  EXPECT_TRUE(testTagString(66, "Tag_T2EE_use"));
  EXPECT_TRUE(testBuildAttr(66, 0, armbuildattrs::T2EE_use,
                            armbuildattrs::Not_Allowed));
  EXPECT_TRUE(testBuildAttr(66, 1, armbuildattrs::T2EE_use,
                            armbuildattrs::Allowed));
}

TEST(VirtualizationBuildAttr, testBuildAttr) {
  EXPECT_TRUE(testTagString(68, "Tag_Virtualization_use"));
  EXPECT_TRUE(testBuildAttr(68, 0, armbuildattrs::Virtualization_use,
                            armbuildattrs::Not_Allowed));
  EXPECT_TRUE(testBuildAttr(68, 1, armbuildattrs::Virtualization_use,
                            armbuildattrs::AllowTZ));
  EXPECT_TRUE(testBuildAttr(68, 2, armbuildattrs::Virtualization_use,
                            armbuildattrs::AllowVirtualization));
  EXPECT_TRUE(testBuildAttr(68, 3, armbuildattrs::Virtualization_use,
                            armbuildattrs::AllowTZVirtualization));
}

TEST(MPBuildAttr, testBuildAttr) {
  EXPECT_TRUE(testTagString(42, "Tag_MPextension_use"));
  EXPECT_TRUE(testBuildAttr(42, 0, armbuildattrs::MPextension_use,
                            armbuildattrs::Not_Allowed));
  EXPECT_TRUE(testBuildAttr(42, 1, armbuildattrs::MPextension_use,
                            armbuildattrs::AllowMP));
}

TEST(DivBuildAttr, testBuildAttr) {
  EXPECT_TRUE(testTagString(44, "Tag_DIV_use"));
  EXPECT_TRUE(testBuildAttr(44, 0, armbuildattrs::DIV_use,
                            armbuildattrs::AllowDIVIfExists));
  EXPECT_TRUE(testBuildAttr(44, 1, armbuildattrs::DIV_use,
                            armbuildattrs::DisallowDIV));
  EXPECT_TRUE(testBuildAttr(44, 2, armbuildattrs::DIV_use,
                            armbuildattrs::AllowDIVExt));
}

TEST(PCS_ConfigBuildAttr, testBuildAttr) {
  EXPECT_TRUE(testTagString(13, "Tag_PCS_config"));
  EXPECT_TRUE(testBuildAttr(13, 0, armbuildattrs::PCS_config, 0));
  EXPECT_TRUE(testBuildAttr(13, 1, armbuildattrs::PCS_config, 1));
  EXPECT_TRUE(testBuildAttr(13, 2, armbuildattrs::PCS_config, 2));
  EXPECT_TRUE(testBuildAttr(13, 3, armbuildattrs::PCS_config, 3));
  EXPECT_TRUE(testBuildAttr(13, 4, armbuildattrs::PCS_config, 4));
  EXPECT_TRUE(testBuildAttr(13, 5, armbuildattrs::PCS_config, 5));
  EXPECT_TRUE(testBuildAttr(13, 6, armbuildattrs::PCS_config, 6));
  EXPECT_TRUE(testBuildAttr(13, 7, armbuildattrs::PCS_config, 7));
}

TEST(PCS_R9BuildAttr, testBuildAttr) {
  EXPECT_TRUE(testTagString(14, "Tag_ABI_PCS_R9_use"));
  EXPECT_TRUE(testBuildAttr(14, 0, armbuildattrs::ABI_PCS_R9_use,
                            armbuildattrs::R9IsGPR));
  EXPECT_TRUE(testBuildAttr(14, 1, armbuildattrs::ABI_PCS_R9_use,
                            armbuildattrs::R9IsSB));
  EXPECT_TRUE(testBuildAttr(14, 2, armbuildattrs::ABI_PCS_R9_use,
                            armbuildattrs::R9IsTLSPointer));
  EXPECT_TRUE(testBuildAttr(14, 3, armbuildattrs::ABI_PCS_R9_use,
                            armbuildattrs::R9Reserved));
}

TEST(PCS_RWBuildAttr, testBuildAttr) {
  EXPECT_TRUE(testTagString(15, "Tag_ABI_PCS_RW_data"));
  EXPECT_TRUE(testBuildAttr(15, 0, armbuildattrs::ABI_PCS_RW_data,
                            armbuildattrs::Not_Allowed));
  EXPECT_TRUE(testBuildAttr(15, 1, armbuildattrs::ABI_PCS_RW_data,
                            armbuildattrs::AddressRWPCRel));
  EXPECT_TRUE(testBuildAttr(15, 2, armbuildattrs::ABI_PCS_RW_data,
                            armbuildattrs::AddressRWSBRel));
  EXPECT_TRUE(testBuildAttr(15, 3, armbuildattrs::ABI_PCS_RW_data,
                            armbuildattrs::AddressRWNone));
}

TEST(PCS_ROBuildAttr, testBuildAttr) {
  EXPECT_TRUE(testTagString(16, "Tag_ABI_PCS_RO_data"));
  EXPECT_TRUE(testBuildAttr(16, 0, armbuildattrs::ABI_PCS_RO_data,
                            armbuildattrs::Not_Allowed));
  EXPECT_TRUE(testBuildAttr(16, 1, armbuildattrs::ABI_PCS_RO_data,
                            armbuildattrs::AddressROPCRel));
  EXPECT_TRUE(testBuildAttr(16, 2, armbuildattrs::ABI_PCS_RO_data,
                            armbuildattrs::AddressRONone));
}

TEST(PCS_GOTBuildAttr, testBuildAttr) {
  EXPECT_TRUE(testTagString(17, "Tag_ABI_PCS_GOT_use"));
  EXPECT_TRUE(testBuildAttr(17, 0, armbuildattrs::ABI_PCS_GOT_use,
                            armbuildattrs::Not_Allowed));
  EXPECT_TRUE(testBuildAttr(17, 1, armbuildattrs::ABI_PCS_GOT_use,
                            armbuildattrs::AddressDirect));
  EXPECT_TRUE(testBuildAttr(17, 2, armbuildattrs::ABI_PCS_GOT_use,
                            armbuildattrs::AddressGOT));
}

TEST(PCS_WCharBuildAttr, testBuildAttr) {
  EXPECT_TRUE(testTagString(18, "Tag_ABI_PCS_wchar_t"));
  EXPECT_TRUE(testBuildAttr(18, 0, armbuildattrs::ABI_PCS_wchar_t,
                            armbuildattrs::WCharProhibited));
  EXPECT_TRUE(testBuildAttr(18, 2, armbuildattrs::ABI_PCS_wchar_t,
                            armbuildattrs::WCharWidth2Bytes));
  EXPECT_TRUE(testBuildAttr(18, 4, armbuildattrs::ABI_PCS_wchar_t,
                            armbuildattrs::WCharWidth4Bytes));
}

TEST(EnumSizeBuildAttr, testBuildAttr) {
  EXPECT_TRUE(testTagString(26, "Tag_ABI_enum_size"));
  EXPECT_TRUE(testBuildAttr(26, 0, armbuildattrs::ABI_enum_size,
                            armbuildattrs::EnumProhibited));
  EXPECT_TRUE(testBuildAttr(26, 1, armbuildattrs::ABI_enum_size,
                            armbuildattrs::EnumSmallest));
  EXPECT_TRUE(testBuildAttr(26, 2, armbuildattrs::ABI_enum_size,
                            armbuildattrs::Enum32Bit));
  EXPECT_TRUE(testBuildAttr(26, 3, armbuildattrs::ABI_enum_size,
                            armbuildattrs::Enum32BitABI));
}

TEST(AlignNeededBuildAttr, testBuildAttr) {
  EXPECT_TRUE(testTagString(24, "Tag_ABI_align_needed"));
  EXPECT_TRUE(testBuildAttr(24, 0, armbuildattrs::ABI_align_needed,
                            armbuildattrs::Not_Allowed));
  EXPECT_TRUE(testBuildAttr(24, 1, armbuildattrs::ABI_align_needed,
                            armbuildattrs::Align8Byte));
  EXPECT_TRUE(testBuildAttr(24, 2, armbuildattrs::ABI_align_needed,
                            armbuildattrs::Align4Byte));
  EXPECT_TRUE(testBuildAttr(24, 3, armbuildattrs::ABI_align_needed,
                            armbuildattrs::AlignReserved));
}

TEST(AlignPreservedBuildAttr, testBuildAttr) {
  EXPECT_TRUE(testTagString(25, "Tag_ABI_align_preserved"));
  EXPECT_TRUE(testBuildAttr(25, 0, armbuildattrs::ABI_align_preserved,
                            armbuildattrs::AlignNotPreserved));
  EXPECT_TRUE(testBuildAttr(25, 1, armbuildattrs::ABI_align_preserved,
                            armbuildattrs::AlignPreserve8Byte));
  EXPECT_TRUE(testBuildAttr(25, 2, armbuildattrs::ABI_align_preserved,
                            armbuildattrs::AlignPreserveAll));
  EXPECT_TRUE(testBuildAttr(25, 3, armbuildattrs::ABI_align_preserved,
                            armbuildattrs::AlignReserved));
}

TEST(FPRoundingBuildAttr, testBuildAttr) {
  EXPECT_TRUE(testTagString(19, "Tag_ABI_FP_rounding"));
  EXPECT_TRUE(testBuildAttr(19, 0, armbuildattrs::ABI_FP_rounding, 0));
  EXPECT_TRUE(testBuildAttr(19, 1, armbuildattrs::ABI_FP_rounding, 1));
}

TEST(FPDenormalBuildAttr, testBuildAttr) {
  EXPECT_TRUE(testTagString(20, "Tag_ABI_FP_denormal"));
  EXPECT_TRUE(testBuildAttr(20, 0, armbuildattrs::ABI_FP_denormal,
                            armbuildattrs::PositiveZero));
  EXPECT_TRUE(testBuildAttr(20, 1, armbuildattrs::ABI_FP_denormal,
                            armbuildattrs::IEEEDenormals));
  EXPECT_TRUE(testBuildAttr(20, 2, armbuildattrs::ABI_FP_denormal,
                            armbuildattrs::PreserveFPSign));
}

TEST(FPExceptionsBuildAttr, testBuildAttr) {
  EXPECT_TRUE(testTagString(21, "Tag_ABI_FP_exceptions"));
  EXPECT_TRUE(testBuildAttr(21, 0, armbuildattrs::ABI_FP_exceptions, 0));
  EXPECT_TRUE(testBuildAttr(21, 1, armbuildattrs::ABI_FP_exceptions, 1));
}

TEST(FPUserExceptionsBuildAttr, testBuildAttr) {
  EXPECT_TRUE(testTagString(22, "Tag_ABI_FP_user_exceptions"));
  EXPECT_TRUE(testBuildAttr(22, 0, armbuildattrs::ABI_FP_user_exceptions, 0));
  EXPECT_TRUE(testBuildAttr(22, 1, armbuildattrs::ABI_FP_user_exceptions, 1));
}

TEST(FPNumberModelBuildAttr, testBuildAttr) {
  EXPECT_TRUE(testTagString(23, "Tag_ABI_FP_number_model"));
  EXPECT_TRUE(testBuildAttr(23, 0, armbuildattrs::ABI_FP_number_model,
                            armbuildattrs::Not_Allowed));
  EXPECT_TRUE(testBuildAttr(23, 1, armbuildattrs::ABI_FP_number_model,
                            armbuildattrs::AllowIEEENormal));
  EXPECT_TRUE(testBuildAttr(23, 2, armbuildattrs::ABI_FP_number_model,
                            armbuildattrs::AllowRTABI));
  EXPECT_TRUE(testBuildAttr(23, 3, armbuildattrs::ABI_FP_number_model,
                            armbuildattrs::AllowIEEE754));
}

TEST(FP16BuildAttr, testBuildAttr) {
  EXPECT_TRUE(testTagString(38, "Tag_ABI_FP_16bit_format"));
  EXPECT_TRUE(testBuildAttr(38, 0, armbuildattrs::ABI_FP_16bit_format,
                            armbuildattrs::Not_Allowed));
  EXPECT_TRUE(testBuildAttr(38, 1, armbuildattrs::ABI_FP_16bit_format,
                            armbuildattrs::FP16FormatIEEE));
  EXPECT_TRUE(testBuildAttr(38, 2, armbuildattrs::ABI_FP_16bit_format,
                            armbuildattrs::FP16VFP3));
}

TEST(HardFPBuildAttr, testBuildAttr) {
  EXPECT_TRUE(testTagString(27, "Tag_ABI_HardFP_use"));
  EXPECT_TRUE(testBuildAttr(27, 0, armbuildattrs::ABI_HardFP_use,
                            armbuildattrs::HardFPImplied));
  EXPECT_TRUE(testBuildAttr(27, 1, armbuildattrs::ABI_HardFP_use,
                            armbuildattrs::HardFPSinglePrecision));
  EXPECT_TRUE(testBuildAttr(27, 2, armbuildattrs::ABI_HardFP_use, 2));
}

TEST(VFPArgsBuildAttr, testBuildAttr) {
  EXPECT_TRUE(testTagString(28, "Tag_ABI_VFP_args"));
  EXPECT_TRUE(testBuildAttr(28, 0, armbuildattrs::ABI_VFP_args,
                            armbuildattrs::BaseAAPCS));
  EXPECT_TRUE(testBuildAttr(28, 1, armbuildattrs::ABI_VFP_args,
                            armbuildattrs::HardFPAAPCS));
  EXPECT_TRUE(testBuildAttr(28, 2, armbuildattrs::ABI_VFP_args, 2));
  EXPECT_TRUE(testBuildAttr(28, 3, armbuildattrs::ABI_VFP_args, 3));
}

TEST(WMMXArgsBuildAttr, testBuildAttr) {
  EXPECT_TRUE(testTagString(29, "Tag_ABI_WMMX_args"));
  EXPECT_TRUE(testBuildAttr(29, 0, armbuildattrs::ABI_WMMX_args, 0));
  EXPECT_TRUE(testBuildAttr(29, 1, armbuildattrs::ABI_WMMX_args, 1));
  EXPECT_TRUE(testBuildAttr(29, 2, armbuildattrs::ABI_WMMX_args, 2));
}
