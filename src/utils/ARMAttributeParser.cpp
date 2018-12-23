// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/01.

#include "polarphp/utils/ARMAttributeParser.h"
#include "polarphp/basic/adt/StlExtras.h"
#include "polarphp/basic/adt/StringExtras.h"
#include "polarphp/utils/Leb128.h"
#include "polarphp/utils/ScopedPrinter.h"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/basic/adt/ArrayRef.h"

namespace polar {

using polar::utils::EnumEntry;
using armbuildattrs::AttrType;
using polar::utils::decode_uleb128;
using polar::utils::DictScope;
using polar::basic::array_lengthof;
using polar::basic::SmallVector;
using polar::utils::error_stream;
using polar::basic::utostr;
using polar::basic::make_array_ref;
using polar::basic::Twine;

static const EnumEntry<unsigned> sg_tagNames[] = {
   { "tag_File", armbuildattrs::File },
   { "tag_section", armbuildattrs::Section },
   { "tag_Symbol", armbuildattrs::Symbol },
};

#define ATTRIBUTE_HANDLER(Attr_)                                                \
{ armbuildattrs::Attr_, &ARMAttributeParser::Attr_ }

const ARMAttributeParser::DisplayHandler
ARMAttributeParser::sm_displayRoutines[] = {
   { armbuildattrs::CPU_raw_name, &ARMAttributeParser::stringAttribute, },
   { armbuildattrs::CPU_name, &ARMAttributeParser::stringAttribute },
   ATTRIBUTE_HANDLER(CPU_arch),
   ATTRIBUTE_HANDLER(CPU_arch_profile),
   ATTRIBUTE_HANDLER(ARM_ISA_use),
   ATTRIBUTE_HANDLER(THUMB_ISA_use),
   ATTRIBUTE_HANDLER(FP_arch),
   ATTRIBUTE_HANDLER(WMMX_arch),
   ATTRIBUTE_HANDLER(Advanced_SIMD_arch),
   ATTRIBUTE_HANDLER(PCS_config),
   ATTRIBUTE_HANDLER(ABI_PCS_R9_use),
   ATTRIBUTE_HANDLER(ABI_PCS_RW_data),
   ATTRIBUTE_HANDLER(ABI_PCS_RO_data),
   ATTRIBUTE_HANDLER(ABI_PCS_GOT_use),
   ATTRIBUTE_HANDLER(ABI_PCS_wchar_t),
   ATTRIBUTE_HANDLER(ABI_FP_rounding),
   ATTRIBUTE_HANDLER(ABI_FP_denormal),
   ATTRIBUTE_HANDLER(ABI_FP_exceptions),
   ATTRIBUTE_HANDLER(ABI_FP_user_exceptions),
   ATTRIBUTE_HANDLER(ABI_FP_number_model),
   ATTRIBUTE_HANDLER(ABI_align_needed),
   ATTRIBUTE_HANDLER(ABI_align_preserved),
   ATTRIBUTE_HANDLER(ABI_enum_size),
   ATTRIBUTE_HANDLER(ABI_HardFP_use),
   ATTRIBUTE_HANDLER(ABI_VFP_args),
   ATTRIBUTE_HANDLER(ABI_WMMX_args),
   ATTRIBUTE_HANDLER(ABI_optimization_goals),
   ATTRIBUTE_HANDLER(ABI_FP_optimization_goals),
   ATTRIBUTE_HANDLER(compatibility),
   ATTRIBUTE_HANDLER(CPU_unaligned_access),
   ATTRIBUTE_HANDLER(FP_HP_extension),
   ATTRIBUTE_HANDLER(ABI_FP_16bit_format),
   ATTRIBUTE_HANDLER(MPextension_use),
   ATTRIBUTE_HANDLER(DIV_use),
   ATTRIBUTE_HANDLER(DSP_extension),
   ATTRIBUTE_HANDLER(T2EE_use),
   ATTRIBUTE_HANDLER(Virtualization_use),
   ATTRIBUTE_HANDLER(nodefaults)
};

#undef ATTRIBUTE_HANDLER

uint64_t ARMAttributeParser::parseInteger(const uint8_t *data,
                                          uint32_t &offset)
{
   unsigned length;
   uint64_t value = decode_uleb128(data + offset, &length);
   offset = offset + length;
   return value;
}

StringRef ARMAttributeParser::parseString(const uint8_t *data,
                                          uint32_t &offset)
{
   const char *string = reinterpret_cast<const char*>(data + offset);
   size_t length = std::strlen(string);
   offset = offset + length + 1;
   return StringRef(string, length);
}

void ARMAttributeParser::integerAttribute(AttrType tag, const uint8_t *data,
                                          uint32_t &offset)
{
   uint64_t value = parseInteger(data, offset);
   m_attributes.insert(std::make_pair(tag, value));
   if (m_sw) {
      m_sw->printNumber(armbuildattrs::attr_type_as_string(tag), value);
   }
}

void ARMAttributeParser::stringAttribute(AttrType tag, const uint8_t *data,
                                         uint32_t &offset)
{
   StringRef tagName = armbuildattrs::attr_type_as_string(tag, /*TagPrefix*/false);
   StringRef valueDesc = parseString(data, offset);
   if (m_sw) {
      DictScope AS(*m_sw, "Attribute");
      m_sw->printNumber("Tag", tag);
      if (!tagName.empty()) {
         m_sw->printString("TagName", tagName);
      }
      m_sw->printString("Value", valueDesc);
   }
}

void ARMAttributeParser::printAttribute(unsigned tag, unsigned value,
                                        StringRef valueDesc)
{
   m_attributes.insert(std::make_pair(tag, value));
   if (m_sw) {
      StringRef tagName = armbuildattrs::attr_type_as_string(tag,
                                                             /*TagPrefix*/false);
      DictScope as(*m_sw, "Attribute");
      m_sw->printNumber("Tag", tag);
      m_sw->printNumber("Value", value);
      if (!tagName.empty()) {
         m_sw->printString("TagName", tagName);
      }
      if (!valueDesc.empty()) {
         m_sw->printString("Description", valueDesc);
      }
   }
}

void ARMAttributeParser::CPU_arch(AttrType tag, const uint8_t *data,
                                  uint32_t &offset)
{
   static const char *const strings[] = {
      "Pre-v4", "ARM v4", "ARM v4T", "ARM v5T", "ARM v5TE", "ARM v5TEJ", "ARM v6",
      "ARM v6KZ", "ARM v6T2", "ARM v6K", "ARM v7", "ARM v6-M", "ARM v6S-M",
      "ARM v7E-M", "ARM v8"
   };

   uint64_t value = parseInteger(data, offset);
   StringRef valueDesc =
         (value < array_lengthof(strings)) ? strings[value] : nullptr;
   printAttribute(tag, value, valueDesc);
}

void ARMAttributeParser::CPU_arch_profile(AttrType tag, const uint8_t *data,
                                          uint32_t &offset)
{
   uint64_t encoded = parseInteger(data, offset);

   StringRef profile;
   switch (encoded) {
   default:  profile = "Unknown"; break;
   case 'A': profile = "Application"; break;
   case 'R': profile = "Real-time"; break;
   case 'M': profile = "Microcontroller"; break;
   case 'S': profile = "Classic"; break;
   case 0: profile = "None"; break;
   }

   printAttribute(tag, encoded, profile);
}

void ARMAttributeParser::ARM_ISA_use(AttrType tag, const uint8_t *data,
                                     uint32_t &offset)
{
   static const char *const strings[] = { "Not Permitted", "Permitted" };

   uint64_t value = parseInteger(data, offset);
   StringRef valueDesc =
         (value < array_lengthof(strings)) ? strings[value] : nullptr;
   printAttribute(tag, value, valueDesc);
}

void ARMAttributeParser::THUMB_ISA_use(AttrType tag, const uint8_t *data,
                                       uint32_t &offset)
{
   static const char *const strings[] = { "Not Permitted", "Thumb-1", "Thumb-2" };

   uint64_t value = parseInteger(data, offset);
   StringRef valueDesc =
         (value < array_lengthof(strings)) ? strings[value] : nullptr;
   printAttribute(tag, value, valueDesc);
}

void ARMAttributeParser::FP_arch(AttrType tag, const uint8_t *data,
                                 uint32_t &offset)
{
   static const char *const strings[] = {
      "Not Permitted", "VFPv1", "VFPv2", "VFPv3", "VFPv3-D16", "VFPv4",
      "VFPv4-D16", "ARMv8-a FP", "ARMv8-a FP-D16"
   };

   uint64_t value = parseInteger(data, offset);
   StringRef valueDesc =
         (value < array_lengthof(strings)) ? strings[value] : nullptr;
   printAttribute(tag, value, valueDesc);
}

void ARMAttributeParser::WMMX_arch(AttrType tag, const uint8_t *data,
                                   uint32_t &offset)
{
   static const char *const strings[] = { "Not Permitted", "WMMXv1", "WMMXv2" };

   uint64_t value = parseInteger(data, offset);
   StringRef valueDesc =
         (value < array_lengthof(strings)) ? strings[value] : nullptr;
   printAttribute(tag, value, valueDesc);
}

void ARMAttributeParser::Advanced_SIMD_arch(AttrType tag, const uint8_t *data,
                                            uint32_t &offset)
{
   static const char *const strings[] = {
      "Not Permitted", "NEONv1", "NEONv2+FMA", "ARMv8-a NEON", "ARMv8.1-a NEON"
   };

   uint64_t value = parseInteger(data, offset);
   StringRef valueDesc =
         (value < array_lengthof(strings)) ? strings[value] : nullptr;
   printAttribute(tag, value, valueDesc);
}

void ARMAttributeParser::PCS_config(AttrType tag, const uint8_t *data,
                                    uint32_t &offset)
{
   static const char *const strings[] = {
      "None", "Bare Platform", "Linux Application", "Linux DSO", "Palm OS 2004",
      "Reserved (Palm OS)", "Symbian OS 2004", "Reserved (Symbian OS)"
   };

   uint64_t value = parseInteger(data, offset);
   StringRef valueDesc =
         (value < array_lengthof(strings)) ? strings[value] : nullptr;
   printAttribute(tag, value, valueDesc);
}

void ARMAttributeParser::ABI_PCS_R9_use(AttrType tag, const uint8_t *data,
                                        uint32_t &offset)
{
   static const char *const strings[] = { "v6", "Static Base", "TLS", "Unused" };

   uint64_t value = parseInteger(data, offset);
   StringRef valueDesc =
         (value < array_lengthof(strings)) ? strings[value] : nullptr;
   printAttribute(tag, value, valueDesc);
}

void ARMAttributeParser::ABI_PCS_RW_data(AttrType tag, const uint8_t *data,
                                         uint32_t &offset)
{
   static const char *const strings[] = {
      "Absolute", "PC-relative", "SB-relative", "Not Permitted"
   };

   uint64_t value = parseInteger(data, offset);
   StringRef valueDesc =
         (value < array_lengthof(strings)) ? strings[value] : nullptr;
   printAttribute(tag, value, valueDesc);
}

void ARMAttributeParser::ABI_PCS_RO_data(AttrType tag, const uint8_t *data,
                                         uint32_t &offset)
{
   static const char *const strings[] = {
      "Absolute", "PC-relative", "Not Permitted"
   };

   uint64_t value = parseInteger(data, offset);
   StringRef valueDesc =
         (value < array_lengthof(strings)) ? strings[value] : nullptr;
   printAttribute(tag, value, valueDesc);
}

void ARMAttributeParser::ABI_PCS_GOT_use(AttrType tag, const uint8_t *data,
                                         uint32_t &offset)
{
   static const char *const strings[] = {
      "Not Permitted", "Direct", "GOT-Indirect"
   };

   uint64_t value = parseInteger(data, offset);
   StringRef valueDesc =
         (value < array_lengthof(strings)) ? strings[value] : nullptr;
   printAttribute(tag, value, valueDesc);
}

void ARMAttributeParser::ABI_PCS_wchar_t(AttrType tag, const uint8_t *data,
                                         uint32_t &offset)
{
   static const char *const strings[] = {
      "Not Permitted", "Unknown", "2-byte", "Unknown", "4-byte"
   };

   uint64_t value = parseInteger(data, offset);
   StringRef valueDesc =
         (value < array_lengthof(strings)) ? strings[value] : nullptr;
   printAttribute(tag, value, valueDesc);
}

void ARMAttributeParser::ABI_FP_rounding(AttrType tag, const uint8_t *data,
                                         uint32_t &offset)
{
   static const char *const strings[] = { "IEEE-754", "Runtime" };

   uint64_t value = parseInteger(data, offset);
   StringRef valueDesc =
         (value < array_lengthof(strings)) ? strings[value] : nullptr;
   printAttribute(tag, value, valueDesc);
}

void ARMAttributeParser::ABI_FP_denormal(AttrType tag, const uint8_t *data,
                                         uint32_t &offset)
{
   static const char *const strings[] = {
      "Unsupported", "IEEE-754", "Sign Only"
   };

   uint64_t value = parseInteger(data, offset);
   StringRef valueDesc =
         (value < array_lengthof(strings)) ? strings[value] : nullptr;
   printAttribute(tag, value, valueDesc);
}

void ARMAttributeParser::ABI_FP_exceptions(AttrType tag, const uint8_t *data,
                                           uint32_t &offset)
{
   static const char *const strings[] = { "Not Permitted", "IEEE-754" };

   uint64_t value = parseInteger(data, offset);
   StringRef valueDesc =
         (value < array_lengthof(strings)) ? strings[value] : nullptr;
   printAttribute(tag, value, valueDesc);
}

void ARMAttributeParser::ABI_FP_user_exceptions(AttrType tag,
                                                const uint8_t *data,
                                                uint32_t &offset)
{
   static const char *const strings[] = { "Not Permitted", "IEEE-754" };

   uint64_t value = parseInteger(data, offset);
   StringRef valueDesc =
         (value < array_lengthof(strings)) ? strings[value] : nullptr;
   printAttribute(tag, value, valueDesc);
}

void ARMAttributeParser::ABI_FP_number_model(AttrType tag, const uint8_t *data,
                                             uint32_t &offset)
{
   static const char *const strings[] = {
      "Not Permitted", "Finite Only", "RTABI", "IEEE-754"
   };

   uint64_t value = parseInteger(data, offset);
   StringRef valueDesc =
         (value < array_lengthof(strings)) ? strings[value] : nullptr;
   printAttribute(tag, value, valueDesc);
}

void ARMAttributeParser::ABI_align_needed(AttrType tag, const uint8_t *data,
                                          uint32_t &offset)
{
   static const char *const strings[] = {
      "Not Permitted", "8-byte alignment", "4-byte alignment", "Reserved"
   };

   uint64_t value = parseInteger(data, offset);

   std::string description;
   if (value < array_lengthof(strings)) {
      description = std::string(strings[value]);
   } else if (value <= 12) {
      description = std::string("8-byte alignment, ") + utostr(1ULL << value)
            + std::string("-byte extended alignment");
   } else {
      description = "Invalid";
   }
   printAttribute(tag, value, description);
}

void ARMAttributeParser::ABI_align_preserved(AttrType tag, const uint8_t *data,
                                             uint32_t &offset)
{
   static const char *const strings[] = {
      "Not Required", "8-byte data alignment", "8-byte data and code alignment",
      "Reserved"
   };

   uint64_t value = parseInteger(data, offset);

   std::string description;
   if (value < array_lengthof(strings)) {
      description = std::string(strings[value]);
   } else if (value <= 12) {
      description = std::string("8-byte stack alignment, ") +
            utostr(1ULL << value) + std::string("-byte data alignment");
   } else {
      description = "Invalid";
   }
   printAttribute(tag, value, description);
}

void ARMAttributeParser::ABI_enum_size(AttrType tag, const uint8_t *data,
                                       uint32_t &offset)
{
   static const char *const strings[] = {
      "Not Permitted", "Packed", "Int32", "External Int32"
   };

   uint64_t value = parseInteger(data, offset);
   StringRef valueDesc =
         (value < array_lengthof(strings)) ? strings[value] : nullptr;
   printAttribute(tag, value, valueDesc);
}

void ARMAttributeParser::ABI_HardFP_use(AttrType tag, const uint8_t *data,
                                        uint32_t &offset)
{
   static const char *const strings[] = {
      "tag_FP_arch", "Single-Precision", "Reserved", "tag_FP_arch (deprecated)"
   };

   uint64_t value = parseInteger(data, offset);
   StringRef valueDesc =
         (value < array_lengthof(strings)) ? strings[value] : nullptr;
   printAttribute(tag, value, valueDesc);
}

void ARMAttributeParser::ABI_VFP_args(AttrType tag, const uint8_t *data,
                                      uint32_t &offset)
{
   static const char *const strings[] = {
      "AAPCS", "AAPCS VFP", "Custom", "Not Permitted"
   };

   uint64_t value = parseInteger(data, offset);
   StringRef valueDesc =
         (value < array_lengthof(strings)) ? strings[value] : nullptr;
   printAttribute(tag, value, valueDesc);
}

void ARMAttributeParser::ABI_WMMX_args(AttrType tag, const uint8_t *data,
                                       uint32_t &offset)
{
   static const char *const strings[] = { "AAPCS", "iWMMX", "Custom" };

   uint64_t value = parseInteger(data, offset);
   StringRef valueDesc =
         (value < array_lengthof(strings)) ? strings[value] : nullptr;
   printAttribute(tag, value, valueDesc);
}

void ARMAttributeParser::ABI_optimization_goals(AttrType tag,
                                                const uint8_t *data,
                                                uint32_t &offset)
{
   static const char *const strings[] = {
      "None", "Speed", "Aggressive Speed", "size", "Aggressive size", "Debugging",
      "Best Debugging"
   };

   uint64_t value = parseInteger(data, offset);
   StringRef valueDesc =
         (value < array_lengthof(strings)) ? strings[value] : nullptr;
   printAttribute(tag, value, valueDesc);
}

void ARMAttributeParser::ABI_FP_optimization_goals(AttrType tag,
                                                   const uint8_t *data,
                                                   uint32_t &offset)
{
   static const char *const strings[] = {
      "None", "Speed", "Aggressive Speed", "size", "Aggressive size", "Accuracy",
      "Best Accuracy"
   };

   uint64_t value = parseInteger(data, offset);
   StringRef valueDesc =
         (value < array_lengthof(strings)) ? strings[value] : nullptr;
   printAttribute(tag, value, valueDesc);
}

void ARMAttributeParser::compatibility(AttrType tag, const uint8_t *data,
                                       uint32_t &offset)
{
   uint64_t Integer = parseInteger(data, offset);
   StringRef string = parseString(data, offset);

   if (m_sw) {
      DictScope AS(*m_sw, "Attribute");
      m_sw->printNumber("Tag", tag);
      m_sw->startLine() << "value: " << Integer << ", " << string << '\n';
      m_sw->printString("TagName", attr_type_as_string(tag, /*TagPrefix*/false));
      switch (Integer) {
      case 0:
         m_sw->printString("Description", StringRef("No Specific Requirements"));
         break;
      case 1:
         m_sw->printString("Description", StringRef("AEABI Conformant"));
         break;
      default:
         m_sw->printString("Description", StringRef("AEABI Non-Conformant"));
         break;
      }
   }
}

void ARMAttributeParser::CPU_unaligned_access(AttrType tag, const uint8_t *data,
                                              uint32_t &offset)
{
   static const char *const strings[] = { "Not Permitted", "v6-style" };

   uint64_t value = parseInteger(data, offset);
   StringRef valueDesc =
         (value < array_lengthof(strings)) ? strings[value] : nullptr;
   printAttribute(tag, value, valueDesc);
}

void ARMAttributeParser::FP_HP_extension(AttrType tag, const uint8_t *data,
                                         uint32_t &offset)
{
   static const char *const strings[] = { "If Available", "Permitted" };

   uint64_t value = parseInteger(data, offset);
   StringRef valueDesc =
         (value < array_lengthof(strings)) ? strings[value] : nullptr;
   printAttribute(tag, value, valueDesc);
}

void ARMAttributeParser::ABI_FP_16bit_format(AttrType tag, const uint8_t *data,
                                             uint32_t &offset)
{
   static const char *const strings[] = { "Not Permitted", "IEEE-754", "VFPv3" };

   uint64_t value = parseInteger(data, offset);
   StringRef valueDesc =
         (value < array_lengthof(strings)) ? strings[value] : nullptr;
   printAttribute(tag, value, valueDesc);
}

void ARMAttributeParser::MPextension_use(AttrType tag, const uint8_t *data,
                                         uint32_t &offset)
{
   static const char *const strings[] = { "Not Permitted", "Permitted" };

   uint64_t value = parseInteger(data, offset);
   StringRef valueDesc =
         (value < array_lengthof(strings)) ? strings[value] : nullptr;
   printAttribute(tag, value, valueDesc);
}

void ARMAttributeParser::DIV_use(AttrType tag, const uint8_t *data,
                                 uint32_t &offset)
{
   static const char *const strings[] = {
      "If Available", "Not Permitted", "Permitted"
   };

   uint64_t value = parseInteger(data, offset);
   StringRef valueDesc =
         (value < array_lengthof(strings)) ? strings[value] : nullptr;
   printAttribute(tag, value, valueDesc);
}

void ARMAttributeParser::DSP_extension(AttrType tag, const uint8_t *data,
                                       uint32_t &offset)
{
   static const char *const strings[] = { "Not Permitted", "Permitted" };

   uint64_t value = parseInteger(data, offset);
   StringRef valueDesc =
         (value < array_lengthof(strings)) ? strings[value] : nullptr;
   printAttribute(tag, value, valueDesc);
}

void ARMAttributeParser::T2EE_use(AttrType tag, const uint8_t *data,
                                  uint32_t &offset)
{
   static const char *const strings[] = { "Not Permitted", "Permitted" };

   uint64_t value = parseInteger(data, offset);
   StringRef valueDesc =
         (value < array_lengthof(strings)) ? strings[value] : nullptr;
   printAttribute(tag, value, valueDesc);
}

void ARMAttributeParser::Virtualization_use(AttrType tag, const uint8_t *data,
                                            uint32_t &offset)
{
   static const char *const strings[] = {
      "Not Permitted", "TrustZone", "Virtualization Extensions",
      "TrustZone + Virtualization Extensions"
   };

   uint64_t value = parseInteger(data, offset);
   StringRef valueDesc =
         (value < array_lengthof(strings)) ? strings[value] : nullptr;
   printAttribute(tag, value, valueDesc);
}

void ARMAttributeParser::nodefaults(AttrType tag, const uint8_t *data,
                                    uint32_t &offset)
{
   uint64_t value = parseInteger(data, offset);
   printAttribute(tag, value, "Unspecified Tags UNDEFINED");
}

void ARMAttributeParser::parseIndexList(const uint8_t *data, uint32_t &offset,
                                        SmallVectorImpl<uint8_t> &indexList)
{
   for (;;) {
      unsigned length;
      uint64_t value = decode_uleb128(data + offset, &length);
      offset = offset + length;
      if (value == 0) {
         break;
      }
      indexList.push_back(value);
   }
}

void ARMAttributeParser::parseAttributeList(const uint8_t *data,
                                            uint32_t &offset, uint32_t length)
{
   while (offset < length) {
      unsigned length;
      uint64_t tag = decode_uleb128(data + offset, &length);
      offset += length;

      bool handled = false;
      for (unsigned ahi = 0, ahe = array_lengthof(sm_displayRoutines);
           ahi != ahe && !handled; ++ahi) {
         if (uint64_t(sm_displayRoutines[ahi].m_attribute) == tag) {
            (this->*sm_displayRoutines[ahi].m_routine)(armbuildattrs::AttrType(tag),
                                                  data, offset);
            handled = true;
            break;
         }
      }
      if (!handled) {
         if (tag < 32) {
            error_stream() << "unhandled AEABI tag " << tag
                   << " (" << armbuildattrs::attr_type_as_string(tag) << ")\n";
            continue;
         }

         if (tag % 2 == 0)
            integerAttribute(armbuildattrs::AttrType(tag), data, offset);
         else
            stringAttribute(armbuildattrs::AttrType(tag), data, offset);
      }
   }
}

void ARMAttributeParser::parseSubsection(const uint8_t *data, uint32_t length)
{
   uint32_t offset = sizeof(uint32_t); /* sectionLength */

   const char *vendorName = reinterpret_cast<const char*>(data + offset);
   size_t vendorNameLength = std::strlen(vendorName);
   offset = offset + vendorNameLength + 1;

   if (m_sw) {
      m_sw->printNumber("sectionLength", length);
      m_sw->printString("Vendor", StringRef(vendorName, vendorNameLength));
   }

   if (StringRef(vendorName, vendorNameLength).toLower() != "aeabi") {
      return;
   }

   while (offset < length) {
      /// tag_File | tag_section | tag_Symbol   uleb128:byte-size
      uint8_t tag = data[offset];
      offset = offset + sizeof(tag);

      uint32_t size =
            *reinterpret_cast<const utils::ulittle32_t*>(data + offset);
      offset = offset + sizeof(size);

      if (m_sw) {
         m_sw->printEnum("Tag", tag, make_array_ref(sg_tagNames));
         m_sw->printNumber("size", size);
      }

      if (size > length) {
         error_stream() << "subsection length greater than section length\n";
         return;
      }

      StringRef ScopeName, IndexName;
      SmallVector<uint8_t, 8> Indicies;
      switch (tag) {
      case armbuildattrs::File:
         ScopeName = "FileAttributes";
         break;
      case armbuildattrs::Section:
         ScopeName = "SectionAttributes";
         IndexName = "Sections";
         parseIndexList(data, offset, Indicies);
         break;
      case armbuildattrs::Symbol:
         ScopeName = "SymbolAttributes";
         IndexName = "Symbols";
         parseIndexList(data, offset, Indicies);
         break;
      default:
         error_stream() << "unrecognised tag: 0x" << Twine::utohexstr(tag) << '\n';
         return;
      }

      if (m_sw) {
         DictScope ASS(*m_sw, ScopeName);
         if (!Indicies.empty())
            m_sw->printList(IndexName, Indicies);
         parseAttributeList(data, offset, length);
      } else {
         parseAttributeList(data, offset, length);
      }
   }
}

void ARMAttributeParser::parse(ArrayRef<uint8_t> section, bool isLittle)
{
   size_t offset = 1;
   unsigned sectionNumber = 0;
   while (offset < section.getSize()) {
      uint32_t sectionLength = isLittle ?
               utils::endian::read32le(section.getData() + offset) :
               utils::endian::read32be(section.getData() + offset);

      if (m_sw) {
         m_sw->startLine() << "section " << ++sectionNumber << " {\n";
         m_sw->indent();
      }
      parseSubsection(section.getData() + offset, sectionLength);
      offset = offset + sectionLength;
      if (m_sw) {
         m_sw->unindent();
         m_sw->startLine() << "}\n";
      }
   }
}
} // polar
