// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/11.

#ifndef POLARPHP_BASIC_ADT_TRIPLE_H
#define POLARPHP_BASIC_ADT_TRIPLE_H

#include "polarphp/basic/adt/Twine.h"
#include "polarphp/basic/adt/StringRef.h"

// Some system headers or GCC predefined macros conflict with identifiers in
// this file.  Undefine them here.
#undef NetBSD
#undef mips
#undef sparc

namespace polar {
namespace basic {

/// Triple - Helper class for working with autoconf configuration names. For
/// historical reasons, we also call these 'triples' (they used to contain
/// exactly three fields).
///
/// Configuration names are strings in the canonical form:
///   ARCHITECTURE-VENDOR-OPERATING_SYSTEM
/// or
///   ARCHITECTURE-VENDOR-OPERATING_SYSTEM-ENVIRONMENT
///
/// This class is used for clients which want to support arbitrary
/// configuration names, but also want to implement certain special
/// behavior for particular configurations. This class isolates the mapping
/// from the components of the configuration name to well known IDs.
///
/// At its core the Triple class is designed to be a wrapper for a triple
/// string; the constructor does not change or normalize the triple string.
/// Clients that need to handle the non-canonical triples that users often
/// specify should use the normalize method.
///
/// See autoconf/config.guess for a glimpse into what configuration names
/// look like in practice.
///
class Triple
{
public:
   enum class ArchType
   {
      UnknownArch,

      arm,            // ARM (little endian): arm, armv.*, xscale
      armeb,          // ARM (big endian): armeb
      aarch64,        // AArch64 (little endian): aarch64
      aarch64_be,     // AArch64 (big endian): aarch64_be
      arc,            // ARC: Synopsys ARC
      avr,            // AVR: Atmel AVR microcontroller
      bpfel,          // eBPF or extended BPF or 64-bit BPF (little endian)
      bpfeb,          // eBPF or extended BPF or 64-bit BPF (big endian)
      hexagon,        // Hexagon: hexagon
      mips,           // MIPS: mips, mipsallegrex, mipsr6
      mipsel,         // MIPSEL: mipsel, mipsallegrexe, mipsr6el
      mips64,         // MIPS64: mips64, mips64r6, mipsn32, mipsn32r6
      mips64el,       // MIPS64EL: mips64el, mips64r6el, mipsn32el, mipsn32r6el
      msp430,         // MSP430: msp430
      nios2,          // NIOSII: nios2
      ppc,            // PPC: powerpc
      ppc64,          // PPC64: powerpc64, ppu
      ppc64le,        // PPC64LE: powerpc64le
      r600,           // R600: AMD GPUs HD2XXX - HD6XXX
      amdgcn,         // AMDGCN: AMD GCN GPUs
      riscv32,        // RISC-V (32-bit): riscv32
      riscv64,        // RISC-V (64-bit): riscv64
      sparc,          // Sparc: sparc
      sparcv9,        // Sparcv9: Sparcv9
      sparcel,        // Sparc: (endianness = little). NB: 'Sparcle' is a CPU variant
      systemz,        // SystemZ: s390x
      tce,            // TCE (http://tce.cs.tut.fi/): tce
      tcele,          // TCE little endian (http://tce.cs.tut.fi/): tcele
      thumb,          // Thumb (little endian): thumb, thumbv.*
      thumbeb,        // Thumb (big endian): thumbeb
      x86,            // X86: i[3-9]86
      x86_64,         // X86-64: amd64, x86_64
      xcore,          // XCore: xcore
      nvptx,          // NVPTX: 32-bit
      nvptx64,        // NVPTX: 64-bit
      le32,           // le32: generic little-endian 32-bit CPU (PNaCl)
      le64,           // le64: generic little-endian 64-bit CPU (PNaCl)
      amdil,          // AMDIL
      amdil64,        // AMDIL with 64-bit pointers
      hsail,          // AMD HSAIL
      hsail64,        // AMD HSAIL with 64-bit pointers
      spir,           // SPIR: standard portable IR for OpenCL 32-bit version
      spir64,         // SPIR: standard portable IR for OpenCL 64-bit version
      kalimba,        // Kalimba: generic kalimba
      shave,          // SHAVE: Movidius vector VLIW processors
      lanai,          // Lanai: Lanai 32-bit
      wasm32,         // WebAssembly with 32-bit pointers
      wasm64,         // WebAssembly with 64-bit pointers
      renderscript32, // 32-bit RenderScript
      renderscript64, // 64-bit RenderScript
      LastArchType = renderscript64
   };

   enum class SubArchType
   {
      NoSubArch,

      ARMSubArch_v8_5a,
      ARMSubArch_v8_4a,
      ARMSubArch_v8_3a,
      ARMSubArch_v8_2a,
      ARMSubArch_v8_1a,
      ARMSubArch_v8,
      ARMSubArch_v8r,
      ARMSubArch_v8m_baseline,
      ARMSubArch_v8m_mainline,
      ARMSubArch_v7,
      ARMSubArch_v7em,
      ARMSubArch_v7m,
      ARMSubArch_v7s,
      ARMSubArch_v7k,
      ARMSubArch_v7ve,
      ARMSubArch_v6,
      ARMSubArch_v6m,
      ARMSubArch_v6k,
      ARMSubArch_v6t2,
      ARMSubArch_v5,
      ARMSubArch_v5te,
      ARMSubArch_v4t,

      KalimbaSubArch_v3,
      KalimbaSubArch_v4,
      KalimbaSubArch_v5,

      MipsSubArch_r6
   };

   enum class VendorType
   {
      UnknownVendor,

      Apple,
      PC,
      SCEI,
      BGP,
      BGQ,
      Freescale,
      IBM,
      ImaginationTechnologies,
      MipsTechnologies,
      NVIDIA,
      CSR,
      Myriad,
      AMD,
      Mesa,
      SUSE,
      OpenEmbedded,
      LastVendorType = OpenEmbedded
   };

   enum class OSType
   {
      UnknownOS,

      Ananas,
      CloudABI,
      Darwin,
      DragonFly,
      FreeBSD,
      Fuchsia,
      IOS,
      KFreeBSD,
      Linux,
      Lv2,        // PS3
      MacOSX,
      NetBSD,
      OpenBSD,
      Solaris,
      Win32,
      Haiku,
      Minix,
      RTEMS,
      NaCl,       // Native Client
      CNK,        // BG/P Compute-Node Kernel
      AIX,
      CUDA,       // NVIDIA CUDA
      NVCL,       // NVIDIA OpenCL
      AMDHSA,     // AMD HSA Runtime
      PS4,
      ELFIAMCU,
      TvOS,       // Apple tvOS
      WatchOS,    // Apple watchOS
      Mesa3D,
      Contiki,
      AMDPAL,     // AMD PAL Runtime
      HermitCore, // HermitCore Unikernel/Multikernel
      Hurd,       // GNU/Hurd
      LastOSType = Hurd
   };

   enum class EnvironmentType
   {
      UnknownEnvironment,

      GNU,
      GNUABIN32,
      GNUABI64,
      GNUEABI,
      GNUEABIHF,
      GNUX32,
      CODE16,
      EABI,
      EABIHF,
      Android,
      Musl,
      MuslEABI,
      MuslEABIHF,

      MSVC,
      Itanium,
      Cygnus,
      CoreCLR,
      Simulator,  // Simulator variants of other systems, e.g., Apple's iOS
      LastEnvironmentType = Simulator
   };

   enum class ObjectFormatType
   {
      UnknownObjectFormat,

      COFF,
      ELF,
      MachO,
      Wasm,
   };

private:

   std::string m_data;

   /// The parsed arch type.
   ArchType m_arch;

   /// The parsed subarchitecture type.
   SubArchType m_subArch;

   /// The parsed vendor type.
   VendorType m_vendor;

   /// The parsed OS type.
   OSType m_os;

   /// The parsed Environment type.
   EnvironmentType m_environment;

   /// The object format type.
   ObjectFormatType m_objectFormat;

public:
   /// @name Constructors
   /// @{

   /// Default constructor is the same as an empty string and leaves all
   /// triple fields unknown.
   Triple()
      : m_data(),
        m_arch(),
        m_subArch(),
        m_vendor(),
        m_os(),
        m_environment(),
        m_objectFormat()
   {}

   explicit Triple(const Twine &str);
   Triple(const Twine &archStr, const Twine &vendorStr, const Twine &osStr);
   Triple(const Twine &archStr, const Twine &vendorStr, const Twine &osStr,
          const Twine &environmentStr);

   bool operator==(const Triple &other) const
   {
      return m_arch == other.m_arch && m_subArch == other.m_subArch &&
            m_vendor == other.m_vendor && m_os == other.m_os &&
            m_environment == other.m_environment &&
            m_objectFormat == other.m_objectFormat;
   }

   bool operator!=(const Triple &other) const
   {
      return !(*this == other);
   }

   /// @}
   /// @name Normalization
   /// @{

   /// normalize - Turn an arbitrary machine specification into the canonical
   /// triple form (or something sensible that the Triple class understands if
   /// nothing better can reasonably be done).  In particular, it handles the
   /// common case in which otherwise valid components are in the wrong order.
   static std::string normalize(StringRef str);

   /// Return the normalized form of this triple's string.
   std::string normalize() const
   {
      return normalize(m_data);
   }

   /// @}
   /// @name Typed Component Access
   /// @{

   /// getArch - Get the parsed architecture type of this triple.
   ArchType getArch() const
   {
      return m_arch;
   }

   /// getSubArch - get the parsed subarchitecture type for this triple.
   SubArchType getSubArch() const
   {
      return m_subArch;
   }

   /// getVendor - Get the parsed vendor type of this triple.
   VendorType getVendor() const
   {
      return m_vendor;
   }

   /// getOS - Get the parsed operating system type of this triple.
   OSType getOS() const
   {
      return m_os;
   }

   /// hasEnvironment - Does this triple have the optional environment
   /// (fourth) component?
   bool hasEnvironment() const
   {
      return getEnvironmentName() != "";
   }

   /// getEnvironment - Get the parsed environment type of this triple.
   EnvironmentType getEnvironment() const
   {
      return m_environment;
   }

   /// Parse the version number from the OS name component of the
   /// triple, if present.
   ///
   /// For example, "fooos1.2.3" would return (1, 2, 3).
   ///
   /// If an entry is not defined, it will be returned as 0.
   void getEnvironmentVersion(unsigned &major, unsigned &minor,
                              unsigned &micro) const;

   /// getFormat - Get the object format for this triple.
   ObjectFormatType getObjectFormat() const
   {
      return m_objectFormat;
   }

   /// getOSVersion - Parse the version number from the OS name component of the
   /// triple, if present.
   ///
   /// For example, "fooos1.2.3" would return (1, 2, 3).
   ///
   /// If an entry is not defined, it will be returned as 0.
   void getOSVersion(unsigned &major, unsigned &minor, unsigned &micro) const;

   /// getOSMajorVersion - Return just the major version number, this is
   /// specialized because it is a common query.
   unsigned getOSMajorVersion() const
   {
      unsigned major, minor, micro;
      getOSVersion(major, minor, micro);
      return major;
   }

   /// getMacOSXVersion - Parse the version number as with getOSVersion and then
   /// translate generic "darwin" versions to the corresponding OS X versions.
   /// This may also be called with IOS triples but the OS X version number is
   /// just set to a constant 10.4.0 in that case.  Returns true if successful.
   bool getMacOSXVersion(unsigned &major, unsigned &minor,
                         unsigned &micro) const;

   /// getiOSVersion - Parse the version number as with getOSVersion.  This should
   /// only be called with IOS or generic triples.
   void getiOSVersion(unsigned &major, unsigned &minor,
                      unsigned &micro) const;

   /// getWatchOSVersion - Parse the version number as with getOSVersion.  This
   /// should only be called with WatchOS or generic triples.
   void getWatchOSVersion(unsigned &Major, unsigned &Minor,
                          unsigned &Micro) const;

   /// @}
   /// @name Direct Component Access
   /// @{

   const std::string &getStr() const
   {
      return m_data;
   }

   const std::string &getTriple() const
   {
      return m_data;
   }

   /// getArchName - Get the architecture (first) component of the
   /// triple.
   StringRef getArchName() const;

   /// getVendorName - Get the vendor (second) component of the triple.
   StringRef getVendorName() const;

   /// getOSName - Get the operating system (third) component of the
   /// triple.
   StringRef getOSName() const;

   /// getEnvironmentName - Get the optional environment (fourth)
   /// component of the triple, or "" if empty.
   StringRef getEnvironmentName() const;

   /// getOSAndEnvironmentName - Get the operating system and optional
   /// environment components as a single string (separated by a '-'
   /// if the environment component is present).
   StringRef getOSAndEnvironmentName() const;

   /// @}
   /// @name Convenience Predicates
   /// @{

   /// Test whether the architecture is 64-bit
   ///
   /// Note that this tests for 64-bit pointer width, and nothing else. Note
   /// that we intentionally expose only three predicates, 64-bit, 32-bit, and
   /// 16-bit. The inner details of pointer width for particular architectures
   /// is not summed up in the triple, and so only a coarse grained predicate
   /// system is provided.
   bool isArch64Bit() const;

   /// Test whether the architecture is 32-bit
   ///
   /// Note that this tests for 32-bit pointer width, and nothing else.
   bool isArch32Bit() const;

   /// Test whether the architecture is 16-bit
   ///
   /// Note that this tests for 16-bit pointer width, and nothing else.
   bool isArch16Bit() const;

   /// isOSVersionLT - Helper function for doing comparisons against version
   /// numbers included in the target triple.
   bool isOSVersionLT(unsigned major, unsigned minor = 0,
                      unsigned micro = 0) const
   {
      unsigned lhs[3];
      getOSVersion(lhs[0], lhs[1], lhs[2]);

      if (lhs[0] != major) {
         return lhs[0] < major;
      }

      if (lhs[1] != minor) {
         return lhs[1] < minor;
      }

      if (lhs[2] != micro) {
         return lhs[1] < micro;
      }
      return false;
   }

   bool isOSVersionLT(const Triple &other) const
   {
      unsigned rhs[3];
      other.getOSVersion(rhs[0], rhs[1], rhs[2]);
      return isOSVersionLT(rhs[0], rhs[1], rhs[2]);
   }

   /// isMacOSXVersionLT - Comparison function for checking OS X version
   /// compatibility, which handles supporting skewed version numbering schemes
   /// used by the "darwin" triples.
   bool isMacOSXVersionLT(unsigned major, unsigned minor = 0,
                          unsigned micro = 0) const
   {
      assert(isMacOSX() && "Not an OS X triple!");

      // If this is OS X, expect a sane version number.
      if (getOS() == Triple::OSType::MacOSX) {
         return isOSVersionLT(major, minor, micro);
      }
      // Otherwise, compare to the "Darwin" number.
      assert(major == 10 && "Unexpected major version");
      return isOSVersionLT(minor + 4, micro, 0);
   }

   /// isMacOSX - Is this a Mac OS X triple. For legacy reasons, we support both
   /// "darwin" and "osx" as OS X triples.
   bool isMacOSX() const
   {
      return getOS() == Triple::OSType::Darwin || getOS() == Triple::OSType::MacOSX;
   }

   /// Is this an iOS triple.
   /// Note: This identifies tvOS as a variant of iOS. If that ever
   /// changes, i.e., if the two operating systems diverge or their version
   /// numbers get out of sync, that will need to be changed.
   /// watchOS has completely different version numbers so it is not included.
   bool isiOS() const
   {
      return getOS() == Triple::OSType::IOS || isTvOS();
   }

   /// Is this an Apple tvOS triple.
   bool isTvOS() const
   {
      return getOS() == Triple::OSType::TvOS;
   }

   /// Is this an Apple watchOS triple.
   bool isWatchOS() const
   {
      return getOS() == Triple::OSType::WatchOS;
   }

   bool isWatchABI() const
   {
      return getSubArch() == Triple::SubArchType::ARMSubArch_v7k;
   }

   /// isOSDarwin - Is this a "Darwin" OS (OS X, iOS, or watchOS).
   bool isOSDarwin() const
   {
      return isMacOSX() || isiOS() || isWatchOS();
   }

   bool isSimulatorEnvironment() const
   {
      return getEnvironment() == Triple::EnvironmentType::Simulator;
   }

   bool isOSNetBSD() const
   {
      return getOS() == Triple::OSType::NetBSD;
   }

   bool isOSOpenBSD() const
   {
      return getOS() == Triple::OSType::OpenBSD;
   }

   bool isOSFreeBSD() const
   {
      return getOS() == Triple::OSType::FreeBSD;
   }

   bool isOSFuchsia() const
   {
      return getOS() == Triple::OSType::Fuchsia;
   }

   bool isOSDragonFly() const
   {
      return getOS() == Triple::OSType::DragonFly;
   }

   bool isOSSolaris() const
   {
      return getOS() == Triple::OSType::Solaris;
   }

   bool isOSIAMCU() const
   {
      return getOS() == Triple::OSType::ELFIAMCU;
   }

   bool isOSUnknown() const
   {
      return getOS() == Triple::OSType::UnknownOS;
   }

   bool isGNUEnvironment() const
   {
      EnvironmentType env = getEnvironment();
      return env == Triple::EnvironmentType::GNU || env == Triple::EnvironmentType::GNUABIN32 ||
            env == Triple::EnvironmentType::GNUABI64 || env == Triple::EnvironmentType::GNUEABI ||
            env == Triple::EnvironmentType::GNUEABIHF || env == Triple::EnvironmentType::GNUX32;
   }

   bool isOSContiki() const
   {
      return getOS() == Triple::OSType::Contiki;
   }

   /// Tests whether the OS is Haiku.
   bool isOSHaiku() const
   {
      return getOS() == Triple::OSType::Haiku;
   }

   /// Checks if the environment could be MSVC.
   bool isWindowsMSVCEnvironment() const
   {
      return getOS() == Triple::OSType::Win32 &&
            (getEnvironment() == Triple::EnvironmentType::UnknownEnvironment ||
             getEnvironment() == Triple::EnvironmentType::MSVC);
   }

   /// Checks if the environment is MSVC.
   bool isKnownWindowsMSVCEnvironment() const
   {
      return getOS() == Triple::OSType::Win32 && getEnvironment() == Triple::EnvironmentType::MSVC;
   }

   bool isWindowsCoreCLREnvironment() const
   {
      return getOS() == Triple::OSType::Win32 && getEnvironment() == Triple::EnvironmentType::CoreCLR;
   }

   bool isWindowsItaniumEnvironment() const
   {
      return getOS() == Triple::OSType::Win32 && getEnvironment() == Triple::EnvironmentType::Itanium;
   }

   bool isWindowsCygwinEnvironment() const
   {
      return getOS() == Triple::OSType::Win32 && getEnvironment() == Triple::EnvironmentType::Cygnus;
   }

   bool isWindowsGNUEnvironment() const
   {
      return getOS() == Triple::OSType::Win32 && getEnvironment() == Triple::EnvironmentType::GNU;
   }

   /// Tests for either Cygwin or MinGW OS
   bool isOSCygMing() const {
      return isWindowsCygwinEnvironment() || isWindowsGNUEnvironment();
   }

   /// Is this a "Windows" OS targeting a "MSVCRT.dll" environment.
   bool isOSMSVCRT() const
   {
      return isWindowsMSVCEnvironment() || isWindowsGNUEnvironment() ||
            isWindowsItaniumEnvironment();
   }

   /// Tests whether the OS is Windows.
   bool isOSWindows() const
   {
      return getOS() == Triple::OSType::Win32;
   }

   /// Tests whether the OS is NaCl (Native Client)
   bool isOSNaCl() const
   {
      return getOS() == Triple::OSType::NaCl;
   }

   /// Tests whether the OS is Linux.
   bool isOSLinux() const
   {
      return getOS() == Triple::OSType::Linux;
   }

   /// Tests whether the OS is kFreeBSD.
   bool isOSKFreeBSD() const
   {
      return getOS() == Triple::OSType::KFreeBSD;
   }


   /// Tests whether the OS is Hurd.
   bool isOSHurd() const
   {
      return getOS() == Triple::OSType::Hurd;
   }

   /// Tests whether the OS uses glibc.
   bool isOSGlibc() const
   {
      return (getOS() == Triple::OSType::Linux || getOS() == Triple::OSType::KFreeBSD ||
              getOS() == Triple::OSType::Hurd) &&
            !isAndroid();
   }

   /// Tests whether the OS uses the ELF binary format.
   bool isOSBinFormatELF() const
   {
      return getObjectFormat() == Triple::ObjectFormatType::ELF;
   }

   /// Tests whether the OS uses the COFF binary format.
   bool isOSBinFormatCOFF() const
   {
      return getObjectFormat() == Triple::ObjectFormatType::COFF;
   }

   /// Tests whether the environment is MachO.
   bool isOSBinFormatMachO() const
   {
      return getObjectFormat() == Triple::ObjectFormatType::MachO;
   }

   /// Tests whether the OS uses the Wasm binary format.
   bool isOSBinFormatWasm() const
   {
      return getObjectFormat() == Triple::ObjectFormatType::Wasm;
   }

   /// Tests whether the target is the PS4 CPU
   bool isPS4CPU() const
   {
      return getArch() == Triple::ArchType::x86_64 &&
            getVendor() == Triple::VendorType::SCEI &&
            getOS() == Triple::OSType::PS4;
   }

   /// Tests whether the target is the PS4 platform
   bool isPS4() const
   {
      return getVendor() == Triple::VendorType::SCEI &&
            getOS() == Triple::OSType::PS4;
   }

   /// Tests whether the target is Android
   bool isAndroid() const
   {
      return getEnvironment() == Triple::EnvironmentType::Android;
   }

   bool isAndroidVersionLT(unsigned major) const
   {
      assert(isAndroid() && "Not an Android triple!");
      unsigned env[3];
      getEnvironmentVersion(env[0], env[1], env[2]);
      // 64-bit targets did not exist before API level 21 (Lollipop).
      if (isArch64Bit() && env[0] < 21) {
         env[0] = 21;
      }
      return env[0] < major;
   }

   /// Tests whether the environment is musl-libc
   bool isMusl() const
   {
      return getEnvironment() == Triple::EnvironmentType::Musl ||
            getEnvironment() == Triple::EnvironmentType::MuslEABI ||
            getEnvironment() == Triple::EnvironmentType::MuslEABIHF;
   }

   /// Tests whether the target is NVPTX (32- or 64-bit).
   bool isNVPTX() const
   {
      return getArch() == Triple::ArchType::nvptx || getArch() == Triple::ArchType::nvptx64;
   }

   /// Tests whether the target is Thumb (little and big endian).
   bool isThumb() const
   {
      return getArch() == Triple::ArchType::thumb || getArch() == Triple::ArchType::thumbeb;
   }

   /// Tests whether the target is ARM (little and big endian).
   bool isARM() const
   {
      return getArch() == Triple::ArchType::arm || getArch() == Triple::ArchType::armeb;
   }

   /// Tests whether the target is AArch64 (little and big endian).
   bool isAArch64() const
   {
      return getArch() == Triple::ArchType::aarch64 || getArch() == Triple::ArchType::aarch64_be;
   }

   /// Tests whether the target supports comdat
   bool supportsCOMDAT() const
   {
      return !isOSBinFormatMachO();
   }

   /// Tests whether the target uses emulated TLS as default.
   bool hasDefaultEmulatedTLS() const
   {
      return isAndroid() || isOSOpenBSD() || isWindowsCygwinEnvironment();
   }

   /// @}
   /// @name Mutators
   /// @{

   /// setArch - Set the architecture (first) component of the triple
   /// to a known type.
   void setArch(ArchType kind);

   /// setVendor - Set the vendor (second) component of the triple to a
   /// known type.
   void setVendor(VendorType kind);

   /// setOS - Set the operating system (third) component of the triple
   /// to a known type.
   void setOS(OSType kind);

   /// setEnvironment - Set the environment (fourth) component of the triple
   /// to a known type.
   void setEnvironment(EnvironmentType kind);

   /// setObjectFormat - Set the object file format
   void setObjectFormat(ObjectFormatType kind);

   /// setTriple - Set all components to the new triple \p Str.
   void setTriple(const Twine &str);

   /// setArchName - Set the architecture (first) component of the
   /// triple by name.
   void setArchName(StringRef str);

   /// setVendorName - Set the vendor (second) component of the triple
   /// by name.
   void setVendorName(StringRef str);

   /// setOSName - Set the operating system (third) component of the
   /// triple by name.
   void setOSName(StringRef str);

   /// setEnvironmentName - Set the optional environment (fourth)
   /// component of the triple by name.
   void setEnvironmentName(StringRef str);

   /// setOSAndEnvironmentName - Set the operating system and optional
   /// environment components with a single string.
   void setOSAndEnvironmentName(StringRef str);

   /// @}
   /// @name Helpers to build variants of a particular triple.
   /// @{

   /// Form a triple with a 32-bit variant of the current architecture.
   ///
   /// This can be used to move across "families" of architectures where useful.
   ///
   /// \returns A new triple with a 32-bit architecture or an unknown
   ///          architecture if no such variant can be found.
   Triple get32BitArchVariant() const;

   /// Form a triple with a 64-bit variant of the current architecture.
   ///
   /// This can be used to move across "families" of architectures where useful.
   ///
   /// \returns A new triple with a 64-bit architecture or an unknown
   ///          architecture if no such variant can be found.
   Triple get64BitArchVariant() const;

   /// Form a triple with a big endian variant of the current architecture.
   ///
   /// This can be used to move across "families" of architectures where useful.
   ///
   /// \returns A new triple with a big endian architecture or an unknown
   ///          architecture if no such variant can be found.
   Triple getBigEndianArchVariant() const;

   /// Form a triple with a little endian variant of the current architecture.
   ///
   /// This can be used to move across "families" of architectures where useful.
   ///
   /// \returns A new triple with a little endian architecture or an unknown
   ///          architecture if no such variant can be found.
   Triple getLittleEndianArchVariant() const;

   /// Get the (LLVM) name of the minimum ARM CPU for the arch we are targeting.
   ///
   /// \param Arch the architecture name (e.g., "armv7s"). If it is an empty
   /// string then the triple's arch name is used.
   StringRef getARMCPUForArch(StringRef arch = StringRef()) const;

   /// Tests whether the target triple is little endian.
   ///
   /// \returns true if the triple is little endian, false otherwise.
   bool isLittleEndian() const;

   /// Test whether target triples are compatible.
   bool isCompatibleWith(const Triple &other) const;

   /// Merge target triples.
   std::string merge(const Triple &other) const;

   /// @}
   /// @name Static helpers for IDs.
   /// @{

   /// getArchTypeName - Get the canonical name for the \p Kind architecture.
   static StringRef getArchTypeName(ArchType kind);

   /// getArchTypePrefix - Get the "prefix" canonical name for the \p Kind
   /// architecture. This is the prefix used by the architecture specific
   /// builtins, and is suitable for passing to \see
   /// Intrinsic::getIntrinsicForGCCBuiltin().
   ///
   /// \return - The architecture prefix, or 0 if none is defined.
   static StringRef getArchTypePrefix(ArchType kind);

   /// getVendorTypeName - Get the canonical name for the \p Kind vendor.
   static StringRef getVendorTypeName(VendorType kind);

   /// getOSTypeName - Get the canonical name for the \p Kind operating system.
   static StringRef getOSTypeName(OSType kind);

   /// getEnvironmentTypeName - Get the canonical name for the \p Kind
   /// environment.
   static StringRef getEnvironmentTypeName(EnvironmentType kind);

   /// @}
   /// @name Static helpers for converting alternate architecture names.
   /// @{

   /// getArchTypeForLLVMName - The canonical type for the given LLVM
   /// architecture name (e.g., "x86").
   static ArchType getArchTypeForPolarName(StringRef str);

   /// @}
};

} // basic
} // polar

#endif // POLARPHP_BASIC_ADT_TRIPLE_H
