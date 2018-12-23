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

#ifndef POLARPHP_UTILS_ARM_ATTRIBUTE_PARSER_H
#define POLARPHP_UTILS_ARM_ATTRIBUTE_PARSER_H

#include "ARMBuildAttributes.h"
#include "ScopedPrinter.h"

#include <map>

namespace polar {

namespace basic {
template <typename T>
class SmallVectorImpl;
class StringRef;
template<typename T>
class ArrayRef;
} // basic

using polar::utils::ScopedPrinter;
using polar::basic::StringRef;
using polar::basic::SmallVectorImpl;
using polar::basic::ArrayRef;

class ARMAttributeParser
{
   ScopedPrinter *m_sw;

   std::map<unsigned, unsigned> m_attributes;

   struct DisplayHandler
   {
      armbuildattrs::AttrType m_attribute;
      void (ARMAttributeParser::*m_routine)(armbuildattrs::AttrType,
                                            const uint8_t *, uint32_t &);
   };
   static const DisplayHandler sm_displayRoutines[];

   uint64_t parseInteger(const uint8_t *data, uint32_t &offset);
   StringRef parseString(const uint8_t *data, uint32_t &offset);

   void integerAttribute(armbuildattrs::AttrType tag, const uint8_t *data,
                         uint32_t &offset);
   void stringAttribute(armbuildattrs::AttrType tag, const uint8_t *data,
                        uint32_t &offset);

   void printAttribute(unsigned tag, unsigned Value, StringRef ValueDesc);

   void CPU_arch(armbuildattrs::AttrType tag, const uint8_t *data,
                 uint32_t &offset);
   void CPU_arch_profile(armbuildattrs::AttrType tag, const uint8_t *data,
                         uint32_t &offset);
   void ARM_ISA_use(armbuildattrs::AttrType tag, const uint8_t *data,
                    uint32_t &offset);
   void THUMB_ISA_use(armbuildattrs::AttrType tag, const uint8_t *data,
                      uint32_t &offset);
   void FP_arch(armbuildattrs::AttrType tag, const uint8_t *data,
                uint32_t &offset);
   void WMMX_arch(armbuildattrs::AttrType tag, const uint8_t *data,
                  uint32_t &offset);
   void Advanced_SIMD_arch(armbuildattrs::AttrType tag, const uint8_t *data,
                           uint32_t &offset);
   void PCS_config(armbuildattrs::AttrType tag, const uint8_t *data,
                   uint32_t &offset);
   void ABI_PCS_R9_use(armbuildattrs::AttrType tag, const uint8_t *data,
                       uint32_t &offset);
   void ABI_PCS_RW_data(armbuildattrs::AttrType tag, const uint8_t *data,
                        uint32_t &offset);
   void ABI_PCS_RO_data(armbuildattrs::AttrType tag, const uint8_t *data,
                        uint32_t &offset);
   void ABI_PCS_GOT_use(armbuildattrs::AttrType tag, const uint8_t *data,
                        uint32_t &offset);
   void ABI_PCS_wchar_t(armbuildattrs::AttrType tag, const uint8_t *data,
                        uint32_t &offset);
   void ABI_FP_rounding(armbuildattrs::AttrType tag, const uint8_t *data,
                        uint32_t &offset);
   void ABI_FP_denormal(armbuildattrs::AttrType tag, const uint8_t *data,
                        uint32_t &offset);
   void ABI_FP_exceptions(armbuildattrs::AttrType tag, const uint8_t *data,
                          uint32_t &offset);
   void ABI_FP_user_exceptions(armbuildattrs::AttrType tag, const uint8_t *data,
                               uint32_t &offset);
   void ABI_FP_number_model(armbuildattrs::AttrType tag, const uint8_t *data,
                            uint32_t &offset);
   void ABI_align_needed(armbuildattrs::AttrType tag, const uint8_t *data,
                         uint32_t &offset);
   void ABI_align_preserved(armbuildattrs::AttrType tag, const uint8_t *data,
                            uint32_t &offset);
   void ABI_enum_size(armbuildattrs::AttrType tag, const uint8_t *data,
                      uint32_t &offset);
   void ABI_HardFP_use(armbuildattrs::AttrType tag, const uint8_t *data,
                       uint32_t &offset);
   void ABI_VFP_args(armbuildattrs::AttrType tag, const uint8_t *data,
                     uint32_t &offset);
   void ABI_WMMX_args(armbuildattrs::AttrType tag, const uint8_t *data,
                      uint32_t &offset);
   void ABI_optimization_goals(armbuildattrs::AttrType tag, const uint8_t *data,
                               uint32_t &offset);
   void ABI_FP_optimization_goals(armbuildattrs::AttrType tag,
                                  const uint8_t *data, uint32_t &offset);
   void compatibility(armbuildattrs::AttrType tag, const uint8_t *data,
                      uint32_t &offset);
   void CPU_unaligned_access(armbuildattrs::AttrType tag, const uint8_t *data,
                             uint32_t &offset);
   void FP_HP_extension(armbuildattrs::AttrType tag, const uint8_t *data,
                        uint32_t &offset);
   void ABI_FP_16bit_format(armbuildattrs::AttrType tag, const uint8_t *data,
                            uint32_t &offset);
   void MPextension_use(armbuildattrs::AttrType tag, const uint8_t *data,
                        uint32_t &offset);
   void DIV_use(armbuildattrs::AttrType tag, const uint8_t *data,
                uint32_t &offset);
   void DSP_extension(armbuildattrs::AttrType tag, const uint8_t *data,
                      uint32_t &offset);
   void T2EE_use(armbuildattrs::AttrType tag, const uint8_t *data,
                 uint32_t &offset);
   void Virtualization_use(armbuildattrs::AttrType tag, const uint8_t *data,
                           uint32_t &offset);
   void nodefaults(armbuildattrs::AttrType tag, const uint8_t *data,
                   uint32_t &offset);

   void parseAttributeList(const uint8_t *data, uint32_t &offset,
                           uint32_t Length);
   void parseIndexList(const uint8_t *data, uint32_t &offset,
                       SmallVectorImpl<uint8_t> &IndexList);
   void parseSubsection(const uint8_t *data, uint32_t Length);
public:
   ARMAttributeParser(ScopedPrinter *sw)
      : m_sw(sw)
   {}

   ARMAttributeParser()
      : m_sw(nullptr)
   {}

   void parse(ArrayRef<uint8_t> Section, bool isLittle);

   bool hasAttribute(unsigned tag) const
   {
      return m_attributes.count(tag);
   }

   unsigned getAttributeValue(unsigned tag) const
   {
      return m_attributes.find(tag)->second;
   }
};

} // polar

#endif // POLARPHP_UTILS_ARM_ATTRIBUTE_PARSER_H
