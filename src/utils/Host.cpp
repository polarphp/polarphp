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

//===----------------------------------------------------------------------===//
//
//  This file implements the operating system Host concept.
//
//===----------------------------------------------------------------------===//

#include "polarphp/global/Config.h"
#include "polarphp/utils/Host.h"
#include "polarphp/utils/Debug.h"
#include "polarphp/utils/FileSystem.h"
#include "polarphp/utils/MemoryBuffer.h"
#include "polarphp/utils/RawOutStream.h"
#include "polarphp/utils/TargetParser.h"
#include "polarphp/basic/adt/SmallSet.h"
#include "polarphp/basic/adt/SmallVector.h"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/basic/adt/StringSwitch.h"
#include "polarphp/basic/adt/Triple.h"
#include <assert.h>
#include <string.h>
#include <unistd.h>

// Include the platform-specific parts of this class.
#ifdef _MSC_VER
#include <intrin.h>
#endif
#if defined(__APPLE__) && (defined(__ppc__) || defined(__powerpc__))
#include <mach/host_info.h>
#include <mach/mach.h>
#include <mach/mach_host.h>
#include <mach/machine.h>
#endif

#define DEBUG_TYPE "host-detection"

namespace polar {
namespace sys {

using polar::utils::OptionalError;
using polar::utils::MemoryBuffer;
using polar::utils::error_stream;
using polar::utils::StringSwitch;
using polar::utils::SmallVector;
using polar::basic::SmallSet;

namespace {
std::unique_ptr<polar::utils::MemoryBuffer>
POLAR_ATTRIBUTE_UNUSED get_proc_cpuinfo_content()
{
   OptionalError<std::unique_ptr<MemoryBuffer>> text =
         MemoryBuffer::getFileAsStream("/proc/cpuinfo");
   if (std::error_code errorCode = text.getError()) {
      error_stream() << "Can't read "
                     << "/proc/cpuinfo: " << errorCode.message() << "\n";
      return nullptr;
   }
   return std::move(*text);
}
} // anonymous namespace

namespace internal {

StringRef get_host_cpu_name_for_power_pc(StringRef procCpuinfoContent)
{
   // Access to the Processor Version Register (PVR) on PowerPC is privileged,
   // and so we must use an operating-system interface to determine the current
   // processor type. On Linux, this is exposed through the /proc/cpuinfo file.
   const char *generic = "generic";

   // The cpu line is second (after the 'processor: 0' line), so if this
   // buffer is too small then something has changed (or is wrong).
   StringRef::const_iterator cpuInfoStart = procCpuinfoContent.begin();
   StringRef::const_iterator cpuInfoEnd = procCpuinfoContent.end();

   StringRef::const_iterator iter = cpuInfoStart;

   StringRef::const_iterator cpuStart = 0;
   size_t cpuLen = 0;

   // We need to find the first line which starts with cpu, spaces, and a colon.
   // After the colon, there may be some additional spaces and then the cpu type.
   while (iter < cpuInfoEnd && cpuStart == 0) {
      if (iter < cpuInfoEnd && *iter == '\n') {
         ++iter;
      }
      if (iter < cpuInfoEnd && *iter == 'c') {
         ++iter;
         if (iter < cpuInfoEnd && *iter == 'p') {
            ++iter;
            if (iter < cpuInfoEnd && *iter == 'u') {
               ++iter;
               while (iter < cpuInfoEnd && (*iter == ' ' || *iter == '\t')) {
                  ++iter;
               }
               if (iter < cpuInfoEnd && *iter == ':') {
                  ++iter;
                  while (iter < cpuInfoEnd && (*iter == ' ' || *iter == '\t')) {
                     ++iter;
                  }
                  if (iter < cpuInfoEnd) {
                     cpuStart = iter;
                     while (iter < cpuInfoEnd && (*iter != ' ' && *iter != '\t' &&
                                                  *iter != ',' && *iter != '\n')) {
                        ++iter;
                     }
                     cpuLen = iter - cpuStart;
                  }
               }
            }
         }
      }

      if (cpuStart == 0) {
         while (iter < cpuInfoEnd && *iter != '\n') {
            ++iter;
         }
      }
   }

   if (cpuStart == 0) {
      return generic;
   }

   return StringSwitch<const char *>(StringRef(cpuStart, cpuLen))
         .cond("604e", "604e")
         .cond("604", "604")
         .cond("7400", "7400")
         .cond("7410", "7400")
         .cond("7447", "7400")
         .cond("7455", "7450")
         .cond("G4", "g4")
         .cond("POWER4", "970")
         .cond("PPC970FX", "970")
         .cond("PPC970MP", "970")
         .cond("G5", "g5")
         .cond("POWER5", "g5")
         .cond("A2", "a2")
         .cond("POWER6", "pwr6")
         .cond("POWER7", "pwr7")
         .cond("POWER8", "pwr8")
         .cond("POWER8E", "pwr8")
         .cond("POWER8NVL", "pwr8")
         .cond("POWER9", "pwr9")
         .defaultCond(generic);
}

StringRef get_host_cpu_name_for_arm(StringRef procCpuinfoContent)
{
   // The cpuid register on arm is not accessible from user space. On Linux,
   // it is exposed through the /proc/cpuinfo file.

   // Read 32 lines from /proc/cpuinfo, which should contain the CPU part line
   // in all cases.
   SmallVector<StringRef, 32> lines;
   procCpuinfoContent.split(lines, "\n");

   // Look for the CPU implementer line.
   StringRef implementer;
   StringRef hardware;
   for (unsigned index = 0, end = lines.getSize(); index != end; ++index) {
      if (lines[index].startsWith("CPU implementer")) {
         implementer = lines[index].substr(15).ltrim("\t :");
      }
      if (lines[index].startsWith("Hardware")) {
         hardware = lines[index].substr(8).ltrim("\t :");
      }
   }
   if (implementer == "0x41") { // ARM Ltd.
      // MSM8992/8994 may give cpu part for the core that the kernel is running on,
      // which is undeterministic and wrong. Always return cortex-a53 for these SoC.
      if (hardware.endsWith("MSM8994") || hardware.endsWith("MSM8996")) {
         return "cortex-a53";
      }
      // Look for the CPU part line.
      for (unsigned index = 0, end = lines.getSize(); index != end; ++index)
         if (lines[index].startsWith("CPU part"))
            // The CPU part is a 3 digit hexadecimal number with a 0x prefix. The
            // values correspond to the "Part number" in the CP15/c0 register. The
            // contents are specified in the various processor manuals.
            return StringSwitch<const char *>(lines[index].substr(8).ltrim("\t :"))
                  .cond("0x926", "arm926ej-s")
                  .cond("0xb02", "mpcore")
                  .cond("0xb36", "arm1136j-s")
                  .cond("0xb56", "arm1156t2-s")
                  .cond("0xb76", "arm1176jz-s")
                  .cond("0xc08", "cortex-a8")
                  .cond("0xc09", "cortex-a9")
                  .cond("0xc0f", "cortex-a15")
                  .cond("0xc20", "cortex-m0")
                  .cond("0xc23", "cortex-m3")
                  .cond("0xc24", "cortex-m4")
                  .cond("0xd04", "cortex-a35")
                  .cond("0xd03", "cortex-a53")
                  .cond("0xd07", "cortex-a57")
                  .cond("0xd08", "cortex-a72")
                  .cond("0xd09", "cortex-a73")
                  .defaultCond("generic");
   }

   if (implementer == "0x42" || implementer == "0x43") { // Broadcom | Cavium.
      for (unsigned i = 0, end = lines.size(); i != end; ++i) {
         if (lines[i].startsWith("CPU part")) {
            return StringSwitch<const char *>(lines[i].substr(8).ltrim("\t :"))
                  .cond("0x516", "thunderx2t99")
                  .cond("0x0516", "thunderx2t99")
                  .cond("0xaf", "thunderx2t99")
                  .cond("0x0af", "thunderx2t99")
                  .cond("0xa1", "thunderxt88")
                  .cond("0x0a1", "thunderxt88")
                  .defaultCond("generic");
         }
      }
   }

   if (implementer == "0x48") {
      // HiSilicon Technologies, Inc.
      // Look for the CPU part line.
      for (unsigned i = 0, end = lines.size(); i != end; ++i) {
         if (lines[i].startsWith("CPU part")) {
            // The CPU part is a 3 digit hexadecimal number with a 0x prefix. The
            // values correspond to the "Part number" in the CP15/c0 register. The
            // contents are specified in the various processor manuals.
            return StringSwitch<const char *>(lines[i].substr(8).ltrim("\t :"))
                  .cond("0xd01", "tsv110")
                  .defaultCond("generic");
         }
      }
   }

   if (implementer == "0x51") // Qualcomm Technologies, Inc.
      // Look for the CPU part line.
      for (unsigned index = 0, end = lines.getSize(); index != end; ++index)
         if (lines[index].startsWith("CPU part"))
            // The CPU part is a 3 digit hexadecimal number with a 0x prefix. The
            // values correspond to the "Part number" in the CP15/c0 register. The
            // contents are specified in the various processor manuals.
            return StringSwitch<const char *>(lines[index].substr(8).ltrim("\t :"))
                  .cond("0x06f", "krait") // APQ8064
                  .cond("0x201", "kryo")
                  .cond("0x205", "kryo")
                  .cond("0x211", "kryo")
                  .cond("0x800", "cortex-a73")
                  .cond("0x801", "cortex-a73")
                  .cond("0xc00", "falkor")
                  .cond("0xc01", "saphira")
                  .defaultCond("generic");

   if (implementer == "0x53") { // Samsung Electronics Co., Ltd.
      // The exynos chips have a convoluted ID scheme that doesn't seem to follow
      // any predictive pattern across variants and parts.
      unsigned variant = 0, part = 0;

      // Look for the CPU variant line, whose value is a 1 digit hexadecimal
      // number, corresponding to the Variant bits in the CP15/C0 register.
      for (auto line : lines) {
         if (line.consumeFront("CPU variant")) {
            line.ltrim("\t :").getAsInteger(0, variant);
         }
      }

      // Look for the CPU part line, whose value is a 3 digit hexadecimal
      // number, corresponding to the PartNum bits in the CP15/C0 register.
      for (auto line : lines) {
         if (line.consumeFront("CPU part")) {
            line.ltrim("\t :").getAsInteger(0, part);
         }
      }

      unsigned exynos = (variant << 12) | part;
      switch (exynos) {
      default:
         // Default by falling through to exynos M1.
         POLAR_FALLTHROUGH;

      case 0x1001:
         return "exynos-m1";

      case 0x4001:
         return "exynos-m2";
      }
   }

   return "generic";
}

StringRef get_host_cpu_name_for_s390x(StringRef procCpuinfoContent)
{
   // STIDP is a privileged operation, so use /proc/cpuinfo instead.

   // The "processor 0:" line comes after a fair amount of other information,
   // including a cache breakdown, but this should be plenty.
   SmallVector<StringRef, 32> lines;
   procCpuinfoContent.split(lines, "\n");

   // Look for the CPU features.
   SmallVector<StringRef, 32> cpuFeatures;
   for (unsigned index = 0, end = lines.getSize(); index != end; ++index) {
      if (lines[index].startsWith("features")) {
         size_t pos = lines[index].find(":");
         if (pos != StringRef::npos) {
            lines[index].dropFront(pos + 1).split(cpuFeatures, ' ');
            break;
         }
      }
   }

   // We need to check for the presence of vector support independently of
   // the machine type, since we may only use the vector register set when
   // supported by the kernel (and hypervisor).
   bool haveVectorSupport = false;
   for (unsigned index = 0, end = cpuFeatures.getSize(); index != end; ++index) {
      if (cpuFeatures[index] == "vx") {
         haveVectorSupport = true;
      }
   }

   // Now check the processor machine type.
   for (unsigned index = 0, end = lines.getSize(); index != end; ++index) {
      if (lines[index].startsWith("processor ")) {
         size_t pos = lines[index].find("machine = ");
         if (pos != StringRef::npos) {
            pos += sizeof("machine = ") - 1;
            unsigned int id;
            if (!lines[index].dropFront(pos).getAsInteger(10, id)) {
               if (id >= 3906 && haveVectorSupport) {
                  return "z14";
               }
               if (id >= 2964 && haveVectorSupport) {
                  return "z13";
               }
               if (id >= 2827) {
                  return "zEC12";
               }
               if (id >= 2817) {
                  return "z196";
               }
            }
         }
         break;
      }
   }
   return "generic";
}

StringRef get_host_cpu_name_for_bpf()
{
#if !defined(__linux__) || !defined(__x86_64__)
   return "generic";
#else
   uint8_t insns[40] __attribute__ ((aligned (8))) =
         /* BPF_MOV64_IMM(BPF_REG_0, 0) */
   { 0xb7, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
         /* BPF_MOV64_IMM(BPF_REG_2, 1) */
         0xb7, 0x2, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0,
         /* BPF_JMP_REG(BPF_JLT, BPF_REG_0, BPF_REG_2, 1) */
         0xad, 0x20, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0,
         /* BPF_MOV64_IMM(BPF_REG_0, 1) */
         0xb7, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0,
         /* BPF_EXIT_INSN() */
         0x95, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };

   struct bpf_prog_load_attr {
      uint32_t prog_type;
      uint32_t insn_cnt;
      uint64_t insns;
      uint64_t license;
      uint32_t log_level;
      uint32_t log_size;
      uint64_t log_buf;
      uint32_t kern_version;
      uint32_t prog_flags;
   } attr = {};
   attr.prog_type = 1; /* BPF_PROG_TYPE_SOCKET_FILTER */
   attr.insn_cnt = 5;
   attr.insns = (uint64_t)insns;
   attr.license = (uint64_t)"DUMMY";

   int fd = syscall(321 /* __NR_bpf */, 5 /* BPF_PROG_LOAD */, &attr, sizeof(attr));
   if (fd >= 0) {
      close(fd);
      return "v2";
   }
   return "v1";
#endif
}

} // internal

#if defined(__i386__) || defined(_M_IX86) || \
   defined(__x86_64__) || defined(_M_X64)

enum VendorSignatures {
   SIG_INTEL = 0x756e6547 /* Genu */,
   SIG_AMD = 0x68747541 /* Auth */
};

namespace {

// The check below for i386 was copied from clang's cpuid.h (__get_cpuid_max).
// Check motivated by bug reports for OpenSSL crashing on CPUs without CPUID
// support. Consequently, for i386, the presence of CPUID is checked first
// via the corresponding eflags bit.
// Removal of cpuid.h header motivated by PR30384
// Header cpuid.h and method __get_cpuid_max are not used in llvm, clang, openmp
// or test-suite, but are used in external projects e.g. libstdcxx
bool is_cpuid_supported()
{
#if defined(__GNUC__) || defined(__clang__)
#if defined(__i386__)
   int __cpuid_supported;
   __asm__("  pushfl\n"
   "  popl   %%eax\n"
   "  movl   %%eax,%%ecx\n"
   "  xorl   $0x00200000,%%eax\n"
   "  pushl  %%eax\n"
   "  popfl\n"
   "  pushfl\n"
   "  popl   %%eax\n"
   "  movl   $0,%0\n"
   "  cmpl   %%eax,%%ecx\n"
   "  je     1f\n"
   "  movl   $1,%0\n"
   "1:"
   : "=r"(__cpuid_supported)
         :
           : "eax", "ecx");
   if (!__cpuid_supported)
      return false;
#endif
   return true;
#endif
   return true;
}

/// get_x86_cpuid_and_info - Execute the specified cpuid and return the 4 values in
/// the specified arguments.  If we can't run cpuid on the host, return true.
bool get_x86_cpuid_and_info(unsigned value, unsigned *rEAX, unsigned *rEBX,
                            unsigned *rECX, unsigned *rEDX)
{
#if defined(__GNUC__) || defined(__clang__)
#if defined(__x86_64__)
   // gcc doesn't know cpuid would clobber ebx/rbx. Preserve it manually.
   // FIXME: should we save this for Clang?
   __asm__("movq\t%%rbx, %%rsi\n\t"
   "cpuid\n\t"
   "xchgq\t%%rbx, %%rsi\n\t"
   : "=a"(*rEAX), "=S"(*rEBX), "=c"(*rECX), "=d"(*rEDX)
      : "a"(value));
   return false;
#elif defined(__i386__)
   __asm__("movl\t%%ebx, %%esi\n\t"
   "cpuid\n\t"
   "xchgl\t%%ebx, %%esi\n\t"
   : "=a"(*rEAX), "=S"(*rEBX), "=c"(*rECX), "=d"(*rEDX)
      : "a"(value));
   return false;
#else
   return true;
#endif
#elif defined(_MSC_VER)
   // The MSVC intrinsic is portable across x86 and x64.
   int registers[4];
   __cpuid(registers, value);
   *rEAX = registers[0];
   *rEBX = registers[1];
   *rECX = registers[2];
   *rEDX = registers[3];
   return false;
#else
   return true;
#endif
}

/// get_x86_cpuid_and_info_ex - Execute the specified cpuid with subleaf and return
/// the 4 values in the specified arguments.  If we can't run cpuid on the host,
/// return true.
bool get_x86_cpuid_and_info_ex(unsigned value, unsigned subleaf,
                               unsigned *rEAX, unsigned *rEBX, unsigned *rECX,
                               unsigned *rEDX)
{
#if defined(__GNUC__) || defined(__clang__)
#if defined(__x86_64__)
   // gcc doesn't know cpuid would clobber ebx/rbx. Preserve it manually.
   // FIXME: should we save this for Clang?
   __asm__("movq\t%%rbx, %%rsi\n\t"
   "cpuid\n\t"
   "xchgq\t%%rbx, %%rsi\n\t"
   : "=a"(*rEAX), "=S"(*rEBX), "=c"(*rECX), "=d"(*rEDX)
      : "a"(value), "c"(subleaf));
   return false;
#elif defined(__i386__)
   __asm__("movl\t%%ebx, %%esi\n\t"
   "cpuid\n\t"
   "xchgl\t%%ebx, %%esi\n\t"
   : "=a"(*rEAX), "=S"(*rEBX), "=c"(*rECX), "=d"(*rEDX)
      : "a"(value), "c"(subleaf));
   return false;
#else
   return true;
#endif
#elif defined(_MSC_VER)
   int registers[4];
   __cpuidex(registers, value, subleaf);
   *rEAX = registers[0];
   *rEBX = registers[1];
   *rECX = registers[2];
   *rEDX = registers[3];
   return false;
#else
   return true;
#endif
}

// Read control register 0 (XCR0). Used to detect features such as AVX.
bool get_x86_xcr0(unsigned *rEAX, unsigned *rEDX)
{
#if defined(__GNUC__) || defined(__clang__)
   // Check xgetbv; this uses a .byte sequence instead of the instruction
   // directly because older assemblers do not include support for xgetbv and
   // there is no easy way to conditionally compile based on the assembler used.
   __asm__(".byte 0x0f, 0x01, 0xd0" : "=a"(*rEAX), "=d"(*rEDX) : "c"(0));
   return false;
#elif defined(_MSC_FULL_VER) && defined(_XCR_XFEATURE_ENABLED_MASK)
   unsigned long long Result = _xgetbv(_XCR_XFEATURE_ENABLED_MASK);
   *rEAX = Result;
   *rEDX = Result >> 32;
   return false;
#else
   return true;
#endif
}

void detect_x86_family_model(unsigned eax, unsigned *family,
                             unsigned *model)
{
   *family = (eax >> 8) & 0xf; // Bits 8 - 11
   *model = (eax >> 4) & 0xf;  // Bits 4 - 7
   if (*family == 6 || *family == 0xf) {
      if (*family == 0xf) {
         // Examine extended family ID if family ID is F.
         *family += (eax >> 20) & 0xff; // Bits 20 - 27
      }
      // Examine extended model ID if family ID is 6 or F.
      *model += ((eax >> 16) & 0xf) << 4; // Bits 16 - 19
   }
}

void
get_intel_processor_type_and_subtype(unsigned family, unsigned model,
                                     unsigned brandId, unsigned features,
                                     unsigned features2, unsigned features3,
                                     unsigned *type, unsigned *subtype) {
   if (brandId != 0)
      return;
   switch (family) {
   case 3:
      *type = x86::INTEL_i386;
      break;
   case 4:
      *type = x86::INTEL_i486;
      break;
   case 5:
      if (features & (1 << x86::FEATURE_MMX)) {
         *type = x86::INTEL_PENTIUM_MMX;
         break;
      }
      *type = x86::INTEL_PENTIUM;
      break;
   case 6:
      switch (model) {
      case 0x01: // Pentium Pro processor
         *type = x86::INTEL_PENTIUM_PRO;
         break;
      case 0x03: // Intel Pentium II OverDrive processor, Pentium II processor,
         // model 03
      case 0x05: // Pentium II processor, model 05, Pentium II Xeon processor,
         // model 05, and Intel Celeron processor, model 05
      case 0x06: // Celeron processor, model 06
         *type = x86::INTEL_PENTIUM_II;
         break;
      case 0x07: // Pentium III processor, model 07, and Pentium III Xeon
         // processor, model 07
      case 0x08: // Pentium III processor, model 08, Pentium III Xeon processor,
         // model 08, and Celeron processor, model 08
      case 0x0a: // Pentium III Xeon processor, model 0Ah
      case 0x0b: // Pentium III processor, model 0Bh
         *type = x86::INTEL_PENTIUM_III;
         break;
      case 0x09: // Intel Pentium M processor, Intel Celeron M processor model 09.
      case 0x0d: // Intel Pentium M processor, Intel Celeron M processor, model
         // 0Dh. All processors are manufactured using the 90 nm process.
      case 0x15: // Intel EP80579 Integrated Processor and Intel EP80579
         // Integrated Processor with Intel QuickAssist Technology
         *type = x86::INTEL_PENTIUM_M;
         break;
      case 0x0e: // Intel Core Duo processor, Intel Core Solo processor, model
         // 0Eh. All processors are manufactured using the 65 nm process.
         *type = x86::INTEL_CORE_DUO;
         break;   // yonah
      case 0x0f: // Intel Core 2 Duo processor, Intel Core 2 Duo mobile
         // processor, Intel Core 2 Quad processor, Intel Core 2 Quad
         // mobile processor, Intel Core 2 Extreme processor, Intel
         // Pentium Dual-Core processor, Intel Xeon processor, model
         // 0Fh. All processors are manufactured using the 65 nm process.
      case 0x16: // Intel Celeron processor model 16h. All processors are
         // manufactured using the 65 nm process
         *type = x86::INTEL_CORE2; // "core2"
         *subtype = x86::INTEL_CORE2_65;
         break;
      case 0x17: // Intel Core 2 Extreme processor, Intel Xeon processor, model
         // 17h. All processors are manufactured using the 45 nm process.
         //
         // 45nm: Penryn , Wolfdale, Yorkfield (XE)
      case 0x1d: // Intel Xeon processor MP. All processors are manufactured using
         // the 45 nm process.
         *type = x86::INTEL_CORE2; // "penryn"
         *subtype = x86::INTEL_CORE2_45;
         break;
      case 0x1a: // Intel Core i7 processor and Intel Xeon processor. All
         // processors are manufactured using the 45 nm process.
      case 0x1e: // Intel(R) Core(TM) i7 CPU         870  @ 2.93GHz.
         // As found in a Summer 2010 model iMac.
      case 0x1f:
      case 0x2e:             // Nehalem EX
         *type = x86::INTEL_COREI7; // "nehalem"
         *subtype = x86::INTEL_COREI7_NEHALEM;
         break;
      case 0x25: // Intel Core i7, laptop version.
      case 0x2c: // Intel Core i7 processor and Intel Xeon processor. All
         // processors are manufactured using the 32 nm process.
      case 0x2f: // Westmere EX
         *type = x86::INTEL_COREI7; // "westmere"
         *subtype = x86::INTEL_COREI7_WESTMERE;
         break;
      case 0x2a: // Intel Core i7 processor. All processors are manufactured
         // using the 32 nm process.
      case 0x2d:
         *type = x86::INTEL_COREI7; //"sandybridge"
         *subtype = x86::INTEL_COREI7_SANDYBRIDGE;
         break;
      case 0x3a:
      case 0x3e:             // Ivy Bridge EP
         *type = x86::INTEL_COREI7; // "ivybridge"
         *subtype = x86::INTEL_COREI7_IVYBRIDGE;
         break;

         // Haswell:
      case 0x3c:
      case 0x3f:
      case 0x45:
      case 0x46:
         *type = x86::INTEL_COREI7; // "haswell"
         *subtype = x86::INTEL_COREI7_HASWELL;
         break;

         // Broadwell:
      case 0x3d:
      case 0x47:
      case 0x4f:
      case 0x56:
         *type = x86::INTEL_COREI7; // "broadwell"
         *subtype = x86::INTEL_COREI7_BROADWELL;
         break;

         // Skylake:
      case 0x4e: // Skylake mobile
      case 0x5e: // Skylake desktop
      case 0x8e: // Kaby Lake mobile
      case 0x9e: // Kaby Lake desktop
         *type = x86::INTEL_COREI7; // "skylake"
         *subtype = x86::INTEL_COREI7_SKYLAKE;
         break;

         // Skylake Xeon:
      case 0x55:
         *type = x86::INTEL_COREI7;
         *subtype = x86::INTEL_COREI7_SKYLAKE_AVX512; // "skylake-avx512"
         break;

         // Cannonlake:
      case 0x66:
         *type = x86::INTEL_COREI7;
         *subtype = x86::INTEL_COREI7_CANNONLAKE; // "cannonlake"
         break;

      case 0x1c: // Most 45 nm Intel Atom processors
      case 0x26: // 45 nm Atom Lincroft
      case 0x27: // 32 nm Atom Medfield
      case 0x35: // 32 nm Atom Midview
      case 0x36: // 32 nm Atom Midview
         *type = x86::INTEL_BONNELL;
         break; // "bonnell"

         // Atom Silvermont codes from the Intel software optimization guide.
      case 0x37:
      case 0x4a:
      case 0x4d:
      case 0x5a:
      case 0x5d:
      case 0x4c: // really airmont
         *type = x86::INTEL_SILVERMONT;
         break; // "silvermont"
         // Goldmont:
      case 0x5c: // Apollo Lake
      case 0x5f: // Denverton
         *type = x86::INTEL_GOLDMONT;
         break; // "goldmont"
      case 0x7a:
         *type = x86::INTEL_GOLDMONT_PLUS;
         break;
      case 0x57:
         *type = x86::INTEL_KNL; // knl
         break;
      case 0x85:
         *type = x86::INTEL_KNM; // knm
         break;

      default: // Unknown family 6 CPU, try to guess.
         if (features & (1 << x86::FEATURE_AVX512VBMI2)) {
            *type = x86::INTEL_COREI7;
            *subtype = x86::INTEL_COREI7_ICELAKE_CLIENT;
            break;
         }

         if (features & (1 << x86::FEATURE_AVX512VBMI)) {
            *type = x86::INTEL_COREI7;
            *subtype = x86::INTEL_COREI7_CANNONLAKE;
            break;
         }

         if (features2 & (1 << (x86::FEATURE_AVX512VNNI - 32))) {
            *type = x86::INTEL_COREI7;
            *subtype = x86::INTEL_COREI7_CASCADELAKE;
            break;
         }

         if (features & (1 << x86::FEATURE_AVX512VL)) {
            *type = x86::INTEL_COREI7;
            *subtype = x86::INTEL_COREI7_SKYLAKE_AVX512;
            break;
         }

         if (features & (1 << x86::FEATURE_AVX512ER)) {
            *type = x86::INTEL_KNL; // knl
            break;
         }

         if (features3 & (1 << (x86::FEATURE_CLFLUSHOPT - 64))) {
            if (features3 & (1 << (x86::FEATURE_SHA - 64))) {
               *type = x86::INTEL_GOLDMONT;
            } else {
               *type = x86::INTEL_COREI7;
               *subtype = x86::INTEL_COREI7_SKYLAKE;
            }
            break;
         }

         if (features3 & (1 << (x86::FEATURE_ADX - 64))) {
            *type = x86::INTEL_COREI7;
            *subtype = x86::INTEL_COREI7_BROADWELL;
            break;
         }

         if (features & (1 << x86::FEATURE_AVX2)) {
            *type = x86::INTEL_COREI7;
            *subtype = x86::INTEL_COREI7_HASWELL;
            break;
         }
         if (features & (1 << x86::FEATURE_AVX)) {
            *type = x86::INTEL_COREI7;
            *subtype = x86::INTEL_COREI7_SANDYBRIDGE;
            break;
         }
         if (features & (1 << x86::FEATURE_SSE4_2)) {
            if (features3 & (1 << (x86::FEATURE_MOVBE - 64))) {
               *type = x86::INTEL_SILVERMONT;
            } else {
               *type = x86::INTEL_COREI7;
               *subtype = x86::INTEL_COREI7_NEHALEM;
            }
            break;
         }
         if (features & (1 << x86::FEATURE_SSE4_1)) {
            *type = x86::INTEL_CORE2; // "penryn"
            *subtype = x86::INTEL_CORE2_45;
            break;
         }
         if (features & (1 << x86::FEATURE_SSSE3)) {
            if (features3 & (1 << (x86::FEATURE_MOVBE - 64))) {
               *type = x86::INTEL_BONNELL; // "bonnell"
            } else {
               *type = x86::INTEL_CORE2; // "core2"
               *subtype = x86::INTEL_CORE2_65;
            }
            break;
         }
         if (features3 & (1 << (x86::FEATURE_EM64T - 64))) {
            *type = x86::INTEL_CORE2; // "core2"
            *subtype = x86::INTEL_CORE2_65;
            break;
         }
         if (features & (1 << x86::FEATURE_SSE3)) {
            *type = x86::INTEL_CORE_DUO;
            break;
         }
         if (features & (1 << x86::FEATURE_SSE2)) {
            *type = x86::INTEL_PENTIUM_M;
            break;
         }
         if (features & (1 << x86::FEATURE_SSE)) {
            *type = x86::INTEL_PENTIUM_III;
            break;
         }
         if (features & (1 << x86::FEATURE_MMX)) {
            *type = x86::INTEL_PENTIUM_II;
            break;
         }
         *type = x86::INTEL_PENTIUM_PRO;
         break;
      }
      break;
   case 15: {
      if (features3 & (1 << (x86::FEATURE_EM64T - 64))) {
         *type = x86::INTEL_NOCONA;
         break;
      }
      if (features & (1 << x86::FEATURE_SSE3)) {
         *type = x86::INTEL_PRESCOTT;
         break;
      }
      *type = x86::INTEL_PENTIUM_IV;
      break;
   }
   default:
      break; /*"generic"*/
   }
}

void get_amd_processor_type_and_subtype(unsigned family, unsigned model,
                                        unsigned features, unsigned *type,
                                        unsigned *subtype)
{
   // FIXME: this poorly matches the generated SubtargetFeatureKV table.  There
   // appears to be no way to generate the wide variety of AMD-specific targets
   // from the information returned from CPUID.
   switch (family) {
   case 4:
      *type = x86::AMD_i486;
      break;
   case 5:
      *type = x86::AMDPENTIUM;
      switch (model) {
      case 6:
      case 7:
         *subtype = x86::AMDPENTIUM_K6;
         break; // "k6"
      case 8:
         *subtype = x86::AMDPENTIUM_K62;
         break; // "k6-2"
      case 9:
      case 13:
         *subtype = x86::AMDPENTIUM_K63;
         break; // "k6-3"
      case 10:
         *subtype = x86::AMDPENTIUM_GEODE;
         break; // "geode"
      }
      break;
   case 6:
      if (features & (1 << x86::FEATURE_SSE)) {
         *type = x86::AMD_ATHLON_XP;
         break; // "athlon-xp"
      }
      *type = x86::AMD_ATHLON;
      break; // "athlon"
   case 15:
      if (features & (1 << x86::FEATURE_SSE3)) {
         *type = x86::AMD_K8SSE3;
         break; // "k8-sse3"
      }
      *type = x86::AMD_K8;
      break; // "k8"
   case 16:
      *type = x86::AMDFAM10H; // "amdfam10"
      switch (model) {
      case 2:
         *subtype = x86::AMDFAM10H_BARCELONA;
         break;
      case 4:
         *subtype = x86::AMDFAM10H_SHANGHAI;
         break;
      case 8:
         *subtype = x86::AMDFAM10H_ISTANBUL;
         break;
      }
      break;
   case 20:
      *type = x86::AMD_BTVER1;
      break; // "btver1";
   case 21:
      *type = x86::AMDFAM15H;
      if (model >= 0x60 && model <= 0x7f) {
         *subtype = x86::AMDFAM15H_BDVER4;
         break; // "bdver4"; 60h-7Fh: Excavator
      }
      if (model >= 0x30 && model <= 0x3f) {
         *subtype = x86::AMDFAM15H_BDVER3;
         break; // "bdver3"; 30h-3Fh: Steamroller
      }
      if ((model >= 0x10 && model <= 0x1f) || model == 0x02) {
         *subtype = x86::AMDFAM15H_BDVER2;
         break; // "bdver2"; 02h, 10h-1Fh: Piledriver
      }
      if (model <= 0x0f) {
         *subtype = x86::AMDFAM15H_BDVER1;
         break; // "bdver1"; 00h-0Fh: Bulldozer
      }
      break;
   case 22:
      *type = x86::AMD_BTVER2;
      break; // "btver2"
   case 23:
      *type = x86::AMDFAM17H;
      *subtype = x86::AMDFAM17H_ZNVER1;
      break;
   default:
      break; // "generic"
   }
}

void get_available_features(unsigned ecx, unsigned edx,
                            unsigned maxLeaf, unsigned *featuresOut,
                            unsigned *features2Out, unsigned *features3Out)
{
   unsigned features = 0;
   unsigned features2 = 0;
   unsigned features3 = 0;
   unsigned eax, ebx;

   auto setFeature = [&](unsigned feature) {
      if (feature < 32) {
         features |= 1U << (feature & 0x1f);
      } else if (feature < 64) {
         features2 |= 1U << ((feature - 32) & 0x1f);
      } else if (feature < 96) {
         features3 |= 1U << ((feature - 64) & 0x1f);
      } else {
         polar_unreachable("Unexpected FeatureBit");
      }
   };

   if ((edx >> 15) & 1) {
      setFeature(x86::FEATURE_CMOV);
   }
   if ((edx >> 23) & 1) {
      setFeature(x86::FEATURE_MMX);
   }
   if ((edx >> 25) & 1) {
      setFeature(x86::FEATURE_SSE);
   }
   if ((edx >> 26) & 1) {
      setFeature(x86::FEATURE_SSE2);
   }
   if ((ecx >> 0) & 1) {
      setFeature(x86::FEATURE_SSE3);
   }
   if ((ecx >> 1) & 1) {
      setFeature(x86::FEATURE_PCLMUL);
   }
   if ((ecx >> 9) & 1) {
      setFeature(x86::FEATURE_SSSE3);
   }
   if ((ecx >> 12) & 1) {
      setFeature(x86::FEATURE_FMA);
   }
   if ((ecx >> 19) & 1) {
      setFeature(x86::FEATURE_SSE4_1);
   }
   if ((ecx >> 20) & 1) {
      setFeature(x86::FEATURE_SSE4_2);
   }
   if ((ecx >> 23) & 1) {
      setFeature(x86::FEATURE_POPCNT);
   }
   if ((ecx >> 25) & 1) {
      setFeature(x86::FEATURE_AES);
   }
   if ((ecx >> 22) & 1) {
      setFeature(x86::FEATURE_MOVBE);
   }

   // If CPUID indicates support for XSAVE, XRESTORE and AVX, and XGETBV
   // indicates that the AVX registers will be saved and restored on context
   // switch, then we have full AVX support.
   const unsigned avxBits = (1 << 27) | (1 << 28);
   bool hasAVX = ((ecx & avxBits) == avxBits) && !get_x86_xcr0(&eax, &edx) &&
         ((eax & 0x6) == 0x6);
   bool hasAVX512Save = hasAVX && ((eax & 0xe0) == 0xe0);

   if (hasAVX) {
      setFeature(x86::FEATURE_AVX);
   }
   bool hasLeaf7 =
         maxLeaf >= 0x7 && !get_x86_cpuid_and_info_ex(0x7, 0x0, &eax, &ebx, &ecx, &edx);

   if (hasLeaf7 && ((ebx >> 3) & 1)) {
      setFeature(x86::FEATURE_BMI);
   }
   if (hasLeaf7 && ((ebx >> 5) & 1) && hasAVX) {
      setFeature(x86::FEATURE_AVX2);
   }
   if (hasLeaf7 && ((ebx >> 9) & 1)) {
      setFeature(x86::FEATURE_BMI2);
   }
   if (hasLeaf7 && ((ebx >> 16) & 1) && hasAVX512Save) {
      setFeature(x86::FEATURE_AVX512F);
   }
   if (hasLeaf7 && ((ebx >> 17) & 1) && hasAVX512Save) {
      setFeature(x86::FEATURE_AVX512DQ);
   }
   if (hasLeaf7 && ((ebx >> 19) & 1)) {
      setFeature(x86::FEATURE_ADX);
   }
   if (hasLeaf7 && ((ebx >> 21) & 1) && hasAVX512Save) {
      setFeature(x86::FEATURE_AVX512IFMA);
   }
   if (hasLeaf7 && ((ebx >> 23) & 1)) {
      setFeature(x86::FEATURE_CLFLUSHOPT);
   }
   if (hasLeaf7 && ((ebx >> 26) & 1) && hasAVX512Save) {
      setFeature(x86::FEATURE_AVX512PF);
   }
   if (hasLeaf7 && ((ebx >> 27) & 1) && hasAVX512Save) {
      setFeature(x86::FEATURE_AVX512ER);
   }
   if (hasLeaf7 && ((ebx >> 28) & 1) && hasAVX512Save) {
      setFeature(x86::FEATURE_AVX512CD);
   }
   if (hasLeaf7 && ((ebx >> 29) & 1)) {
      setFeature(x86::FEATURE_SHA);
   }
   if (hasLeaf7 && ((ebx >> 30) & 1) && hasAVX512Save) {
      setFeature(x86::FEATURE_AVX512BW);
   }
   if (hasLeaf7 && ((ebx >> 31) & 1) && hasAVX512Save) {
      setFeature(x86::FEATURE_AVX512VL);
   }
   if (hasLeaf7 && ((ecx >> 1) & 1) && hasAVX512Save) {
      setFeature(x86::FEATURE_AVX512VBMI);
   }

   if (hasLeaf7 && ((ecx >> 6) & 1) && hasAVX512Save) {
      setFeature(x86::FEATURE_AVX512VBMI2);
   }

   if (hasLeaf7 && ((ecx >> 8) & 1)) {
      setFeature(x86::FEATURE_GFNI);
   }

   if (hasLeaf7 && ((ecx >> 10) & 1) && hasAVX) {
      setFeature(x86::FEATURE_VPCLMULQDQ);
   }

   if (hasLeaf7 && ((ecx >> 11) & 1) && hasAVX512Save) {
      setFeature(x86::FEATURE_AVX512VNNI);
   }

   if (hasLeaf7 && ((ecx >> 12) & 1) && hasAVX512Save) {
      setFeature(x86::FEATURE_AVX512BITALG);
   }

   if (hasLeaf7 && ((ecx >> 14) & 1) && hasAVX512Save) {
      setFeature(x86::FEATURE_AVX512VPOPCNTDQ);
   }

   if (hasLeaf7 && ((edx >> 2) & 1) && hasAVX512Save) {
      setFeature(x86::FEATURE_AVX5124VNNIW);
   }

   if (hasLeaf7 && ((edx >> 3) & 1) && hasAVX512Save) {
      setFeature(x86::FEATURE_AVX5124FMAPS);
   }

   unsigned maxExtLevel;
   get_x86_cpuid_and_info(0x80000000, &maxExtLevel, &ebx, &ecx, &edx);

   bool hasExtLeaf1 = maxExtLevel >= 0x80000001 &&
         !get_x86_cpuid_and_info(0x80000001, &eax, &ebx, &ecx, &edx);
   if (hasExtLeaf1 && ((ecx >> 6) & 1)) {
      setFeature(x86::FEATURE_SSE4_A);
   }
   if (hasExtLeaf1 && ((ecx >> 11) & 1)) {
      setFeature(x86::FEATURE_XOP);
   }
   if (hasExtLeaf1 && ((ecx >> 16) & 1)) {
      setFeature(x86::FEATURE_FMA4);
   }
   if (hasExtLeaf1 && ((edx >> 29) & 1)) {
      setFeature(x86::FEATURE_EM64T);
   }
   *featuresOut  = features;
   *features2Out = features2;
   *features3Out = features3;
}

} // anonymous namespace

StringRef get_host_cpu_name()
{
   unsigned eax = 0, ebx = 0, ecx = 0, edx = 0;
   unsigned maxLeaf, vendor;

#if defined(__GNUC__) || defined(__clang__)
   //FIXME: include cpuid.h from clang or copy __get_cpuid_max here
   // and simplify it to not invoke __cpuid (like cpu_model.c in
   // compiler-rt/lib/builtins/cpu_model.c?
   // Opting for the second option.
   if(!is_cpuid_supported()) {
      return "generic";
   }
#endif
   if (get_x86_cpuid_and_info(0, &maxLeaf, &vendor, &ecx, &edx) || maxLeaf < 1) {
      return "generic";
   }
   get_x86_cpuid_and_info(0x1, &eax, &ebx, &ecx, &edx);
   unsigned brandId = ebx & 0xff;
   unsigned family = 0, model = 0;
   unsigned features = 0, features2 = 0, features3 = 0;;
   detect_x86_family_model(eax, &family, &model);
   get_available_features(ecx, edx, maxLeaf, &features, &features2, &features3);
   unsigned type = 0;
   unsigned subtype = 0;
   if (vendor == SIG_INTEL) {
      get_intel_processor_type_and_subtype(family, model, brandId, features,
                                           features2, features3, &type, &subtype);
   } else if (vendor == SIG_AMD) {
      get_amd_processor_type_and_subtype(family, model, features, &type, &subtype);
   }

   // Check subtypes first since those are more specific.
#define x86_CPU_SUBTYPE(ARCHNAME, ENUM) \
   if (subtype == x86::ENUM) \
   return ARCHNAME;
#include "polarphp/utils/X86TargetParserDefs.h"

   // Now check types.
#define x86_CPU_TYPE(ARCHNAME, ENUM) \
   if (type == x86::ENUM) \
   return ARCHNAME;
#include "polarphp/utils/X86TargetParserDefs.h"
   return "generic";
}

#elif defined(__APPLE__) && (defined(__ppc__) || defined(__powerpc__))
StringRef get_host_cpu_name()
{
   host_basic_info_data_t hostInfo;
   mach_msg_type_number_t infoCount;

   infoCount = HOST_BASIC_INFO_COUNT;
   mach_port_t hostPort = mach_host_self();
   host_info(hostPort, HOST_BASIC_INFO, (host_info_t)&hostInfo,
             &infoCount);
   mach_port_deallocate(mach_task_self(), hostPort);

   if (hostInfo.cpu_type != CPU_TYPE_POWERPC)
      return "generic";

   switch (hostInfo.cpu_subtype) {
   case CPU_SUBTYPE_POWERPC_601:
      return "601";
   case CPU_SUBTYPE_POWERPC_602:
      return "602";
   case CPU_SUBTYPE_POWERPC_603:
      return "603";
   case CPU_SUBTYPE_POWERPC_603e:
      return "603e";
   case CPU_SUBTYPE_POWERPC_603ev:
      return "603ev";
   case CPU_SUBTYPE_POWERPC_604:
      return "604";
   case CPU_SUBTYPE_POWERPC_604e:
      return "604e";
   case CPU_SUBTYPE_POWERPC_620:
      return "620";
   case CPU_SUBTYPE_POWERPC_750:
      return "750";
   case CPU_SUBTYPE_POWERPC_7400:
      return "7400";
   case CPU_SUBTYPE_POWERPC_7450:
      return "7450";
   case CPU_SUBTYPE_POWERPC_970:
      return "970";
   default:;
   }

   return "generic";
}
#elif defined(__linux__) && (defined(__ppc__) || defined(__powerpc__))
StringRef sys::get_host_cpu_name() {
   std::unique_ptr<MemoryBuffer> P = get_proc_cpuinfo_content();
   StringRef content = P ? P->getBuffer() : "";
   return detail::get_host_cpu_name_for_power_pc(content);
}
#elif defined(__linux__) && (defined(__arm__) || defined(__aarch64__))
StringRef sys::get_host_cpu_name() {
   std::unique_ptr<MemoryBuffer> P = get_proc_cpuinfo_content();
   StringRef content = P ? P->getBuffer() : "";
   return detail::get_host_cpu_name_for_arm(content);
}
#elif defined(__linux__) && defined(__s390x__)
StringRef sys::get_host_cpu_name() {
   std::unique_ptr<MemoryBuffer> P = get_proc_cpuinfo_content();
   StringRef content = P ? P->getBuffer() : "";
   return detail::get_host_cpu_name_for_s390x(content);
}
#else
StringRef get_host_cpu_name() { return "generic"; }
#endif

namespace {

#if defined(__linux__) && defined(__x86_64__)
// On Linux, the number of physical cores can be computed from /proc/cpuinfo,
// using the number of unique physical/core id pairs. The following
// implementation reads the /proc/cpuinfo format on an x86_64 system.
int compute_host_num_physical_cores()
{
   // Read /proc/cpuinfo as a stream (until EOF reached). It cannot be
   // mmapped because it appears to have 0 size.
   OptionalError<std::unique_ptr<MemoryBuffer>> text =
         MemoryBuffer::getFileAsStream("/proc/cpuinfo");
   if (std::error_code errorCode = text.getError()) {
      error_stream() << "Can't read "
                     << "/proc/cpuinfo: " << errorCode.message() << "\n";
      return -1;
   }
   SmallVector<StringRef, 8> strs;
   (*text)->getBuffer().split(strs, "\n", /*MaxSplit=*/-1,
                              /*KeepEmpty=*/false);
   int curPhysicalId = -1;
   int curCoreId = -1;
   SmallSet<std::pair<int, int>, 32> UniqueItems;
   for (auto &line : strs) {
      line = line.trim();
      if (!line.startsWith("physical id") && !line.startsWith("core id"))
         continue;
      std::pair<StringRef, StringRef> data = line.split(':');
      auto name = data.first.trim();
      auto val = data.second.trim();
      if (name == "physical id") {
         assert(curPhysicalId == -1 &&
                "Expected a core id before seeing another physical id");
         val.getAsInteger(10, curPhysicalId);
      }
      if (name == "core id") {
         assert(curCoreId == -1 &&
                "Expected a physical id before seeing another core id");
         val.getAsInteger(10, curCoreId);
      }
      if (curPhysicalId != -1 && curCoreId != -1) {
         UniqueItems.insert(std::make_pair(curPhysicalId, curCoreId));
         curPhysicalId = -1;
         curCoreId = -1;
      }
   }
   return UniqueItems.size();
}
#elif defined(__APPLE__) && defined(__x86_64__)
#include <sys/param.h>
#include <sys/sysctl.h>

// Gets the number of *physical cores* on the machine.
int compute_host_num_physical_cores() {
   uint32_t count;
   size_t len = sizeof(count);
   sysctlbyname("hw.physicalcpu", &count, &len, NULL, 0);
   if (count < 1) {
      int nm[2];
      nm[0] = CTL_HW;
      nm[1] = HW_AVAILCPU;
      sysctl(nm, 2, &count, &len, NULL, 0);
      if (count < 1)
         return -1;
   }
   return count;
}
#else
// On other systems, return -1 to indicate unknown.
static int compute_host_num_physical_cores() { return -1; }
#endif

} // anonymous namespace

int get_host_num_physical_cores()
{
   static int numCores = compute_host_num_physical_cores();
   return numCores;
}

#if defined(__i386__) || defined(_M_IX86) || \
   defined(__x86_64__) || defined(_M_X64)
bool get_host_cpu_features(StringMap<bool> &features)
{
   unsigned eax = 0, ebx = 0, ecx = 0, edx = 0;
   unsigned maxLevel;
   union {
      unsigned u[3];
      char c[12];
   } text;

   if (get_x86_cpuid_and_info(0, &maxLevel, text.u + 0, text.u + 2, text.u + 1) ||
       maxLevel < 1)
      return false;

   get_x86_cpuid_and_info(1, &eax, &ebx, &ecx, &edx);

   features["cmov"]   = (edx >> 15) & 1;
   features["mmx"]    = (edx >> 23) & 1;
   features["sse"]    = (edx >> 25) & 1;
   features["sse2"]   = (edx >> 26) & 1;

   features["sse3"]   = (ecx >>  0) & 1;
   features["pclmul"] = (ecx >>  1) & 1;
   features["ssse3"]  = (ecx >>  9) & 1;
   features["cx16"]   = (ecx >> 13) & 1;
   features["sse4.1"] = (ecx >> 19) & 1;
   features["sse4.2"] = (ecx >> 20) & 1;
   features["movbe"]  = (ecx >> 22) & 1;
   features["popcnt"] = (ecx >> 23) & 1;
   features["aes"]    = (ecx >> 25) & 1;
   features["rdrnd"]  = (ecx >> 30) & 1;

   // If CPUID indicates support for XSAVE, XRESTORE and AVX, and XGETBV
   // indicates that the AVX registers will be saved and restored on context
   // switch, then we have full AVX support.
   bool hasAVXSave = ((ecx >> 27) & 1) && ((ecx >> 28) & 1) &&
         !get_x86_xcr0(&eax, &edx) && ((eax & 0x6) == 0x6);
   // AVX512 requires additional context to be saved by the OS.
   bool HasAVX512Save = hasAVXSave && ((eax & 0xe0) == 0xe0);

   features["avx"]   = hasAVXSave;
   features["fma"]   = ((ecx >> 12) & 1) && hasAVXSave;
   // Only enable XSAVE if OS has enabled support for saving YMM state.
   features["xsave"] = ((ecx >> 26) & 1) && hasAVXSave;
   features["f16c"]  = ((ecx >> 29) & 1) && hasAVXSave;

   unsigned maxExtLevel;
   get_x86_cpuid_and_info(0x80000000, &maxExtLevel, &ebx, &ecx, &edx);

   bool hasExtLeaf1 = maxExtLevel >= 0x80000001 &&
         !get_x86_cpuid_and_info(0x80000001, &eax, &ebx, &ecx, &edx);
   features["sahf"]   = hasExtLeaf1 && ((ecx >>  0) & 1);
   features["lzcnt"]  = hasExtLeaf1 && ((ecx >>  5) & 1);
   features["sse4a"]  = hasExtLeaf1 && ((ecx >>  6) & 1);
   features["prfchw"] = hasExtLeaf1 && ((ecx >>  8) & 1);
   features["xop"]    = hasExtLeaf1 && ((ecx >> 11) & 1) && hasAVXSave;
   features["lwp"]    = hasExtLeaf1 && ((ecx >> 15) & 1);
   features["fma4"]   = hasExtLeaf1 && ((ecx >> 16) & 1) && hasAVXSave;
   features["tbm"]    = hasExtLeaf1 && ((ecx >> 21) & 1);
   features["mwaitx"] = hasExtLeaf1 && ((ecx >> 29) & 1);
   features["64bit"]  = hasExtLeaf1 && ((edx >> 29) & 1);

   // Miscellaneous memory related features, detected by
   // using the 0x80000008 leaf of the CPUID instruction
   bool HasExtLeaf8 = maxExtLevel >= 0x80000008 &&
         !get_x86_cpuid_and_info(0x80000008, &eax, &ebx, &ecx, &edx);
   features["clzero"]   = HasExtLeaf8 && ((ebx >> 0) & 1);
   features["wbnoinvd"] = HasExtLeaf8 && ((ebx >> 9) & 1);

   bool hasLeaf7 =
         maxLevel >= 7 && !get_x86_cpuid_and_info_ex(0x7, 0x0, &eax, &ebx, &ecx, &edx);

   features["fsgsbase"]   = hasLeaf7 && ((ebx >>  0) & 1);
   features["sgx"]        = hasLeaf7 && ((ebx >>  2) & 1);
   features["bmi"]        = hasLeaf7 && ((ebx >>  3) & 1);
   // AVX2 is only supported if we have the OS save support from AVX.
   features["avx2"]       = hasLeaf7 && ((ebx >>  5) & 1) && hasAVXSave;
   features["bmi2"]       = hasLeaf7 && ((ebx >>  8) & 1);
   features["invpcid"]    = hasLeaf7 && ((ebx >> 10) & 1);
   features["rtm"]        = hasLeaf7 && ((ebx >> 11) & 1);
   // AVX512 is only supported if the OS supports the context save for it.
   features["avx512f"]    = hasLeaf7 && ((ebx >> 16) & 1) && HasAVX512Save;
   features["avx512dq"]   = hasLeaf7 && ((ebx >> 17) & 1) && HasAVX512Save;
   features["rdseed"]     = hasLeaf7 && ((ebx >> 18) & 1);
   features["adx"]        = hasLeaf7 && ((ebx >> 19) & 1);
   features["avx512ifma"] = hasLeaf7 && ((ebx >> 21) & 1) && HasAVX512Save;
   features["clflushopt"] = hasLeaf7 && ((ebx >> 23) & 1);
   features["clwb"]       = hasLeaf7 && ((ebx >> 24) & 1);
   features["avx512pf"]   = hasLeaf7 && ((ebx >> 26) & 1) && HasAVX512Save;
   features["avx512er"]   = hasLeaf7 && ((ebx >> 27) & 1) && HasAVX512Save;
   features["avx512cd"]   = hasLeaf7 && ((ebx >> 28) & 1) && HasAVX512Save;
   features["sha"]        = hasLeaf7 && ((ebx >> 29) & 1);
   features["avx512bw"]   = hasLeaf7 && ((ebx >> 30) & 1) && HasAVX512Save;
   features["avx512vl"]   = hasLeaf7 && ((ebx >> 31) & 1) && HasAVX512Save;

   features["prefetchwt1"]     = hasLeaf7 && ((ecx >>  0) & 1);
   features["avx512vbmi"]      = hasLeaf7 && ((ecx >>  1) & 1) && HasAVX512Save;
   features["pku"]             = hasLeaf7 && ((ecx >>  4) & 1);
   features["waitpkg"]         = hasLeaf7 && ((ecx >>  5) & 1);
   features["avx512vbmi2"]     = hasLeaf7 && ((ecx >>  6) & 1) && HasAVX512Save;
   features["shstk"]           = hasLeaf7 && ((ecx >>  7) & 1);
   features["gfni"]            = hasLeaf7 && ((ecx >>  8) & 1);
   features["vaes"]            = hasLeaf7 && ((ecx >>  9) & 1) && hasAVXSave;
   features["vpclmulqdq"]      = hasLeaf7 && ((ecx >> 10) & 1) && hasAVXSave;
   features["avx512vnni"]      = hasLeaf7 && ((ecx >> 11) & 1) && HasAVX512Save;
   features["avx512bitalg"]    = hasLeaf7 && ((ecx >> 12) & 1) && HasAVX512Save;
   features["avx512vpopcntdq"] = hasLeaf7 && ((ecx >> 14) & 1) && HasAVX512Save;
   features["rdpid"]           = hasLeaf7 && ((ecx >> 22) & 1);
   features["cldemote"]        = hasLeaf7 && ((ecx >> 25) & 1);
   features["movdiri"]         = hasLeaf7 && ((ecx >> 27) & 1);
   features["movdir64b"]       = hasLeaf7 && ((ecx >> 28) & 1);

   // There are two CPUID leafs which information associated with the pconfig
   // instruction:
   // eax=0x7, ecx=0x0 indicates the availability of the instruction (via the 18th
   // bit of edx), while the eax=0x1b leaf returns information on the
   // availability of specific pconfig leafs.
   // The target feature here only refers to the the first of these two.
   // Users might need to check for the availability of specific pconfig
   // leaves using cpuid, since that information is ignored while
   // detecting features using the "-march=native" flag.
   // For more info, see x86 ISA docs.
   features["pconfig"] = hasLeaf7 && ((edx >> 18) & 1);

   bool hasLeafD = maxLevel >= 0xd &&
         !get_x86_cpuid_and_info_ex(0xd, 0x1, &eax, &ebx, &ecx, &edx);

   // Only enable XSAVE if OS has enabled support for saving YMM state.
   features["xsaveopt"] = hasLeafD && ((eax >> 0) & 1) && hasAVXSave;
   features["xsavec"]   = hasLeafD && ((eax >> 1) & 1) && hasAVXSave;
   features["xsaves"]   = hasLeafD && ((eax >> 3) & 1) && hasAVXSave;

   bool hasLeaf14 = maxLevel >= 0x14 &&
         !get_x86_cpuid_and_info_ex(0x14, 0x0, &eax, &ebx, &ecx, &edx);

   features["ptwrite"] = hasLeaf14 && ((ebx >> 4) & 1);

   return true;
}
#elif defined(__linux__) && (defined(__arm__) || defined(__aarch64__))

bool get_host_cpu_features(StringMap<bool> &features)
{
   std::unique_ptr<MemoryBuffer> ptr = get_proc_cpuinfo_content();
   if (!ptr) {
      return false;
   }
   SmallVector<StringRef, 32> lines;
   ptr->getBuffer().split(lines, "\n");

   SmallVector<StringRef, 32> cpuFeatures;

   // Look for the CPU features.
   for (unsigned index = 0, end = lines.getSize(); index != end; ++index) {
      if (lines[index].startsWith("features")) {
         lines[index].split(cpuFeatures, ' ');
         break;
      }
   }

#if defined(__aarch64__)
   // Keep track of which crypto features we have seen
   enum { CAP_AES = 0x1, CAP_PMULL = 0x2, CAP_SHA1 = 0x4, CAP_SHA2 = 0x8 };
   uint32_t crypto = 0;
#endif

   for (unsigned index = 0, end = cpuFeatures.getSize(); index != end; ++index) {
      StringRef polarVMFeatureStr = StringSwitch<StringRef>(cpuFeatures[I])
      #if defined(__aarch64__)
            .cond("asimd", "neon")
            .cond("fp", "fp-armv8")
            .cond("crc32", "crc")
      #else
            .cond("half", "fp16")
            .cond("neon", "neon")
            .cond("vfpv3", "vfp3")
            .cond("vfpv3d16", "d16")
            .cond("vfpv4", "vfp4")
            .cond("idiva", "hwdiv-arm")
            .cond("idivt", "hwdiv")
      #endif
            .Default("");

#if defined(__aarch64__)
      // We need to check crypto separately since we need all of the crypto
      // extensions to enable the subtarget feature
      if (cpuFeatures[index] == "aes") {
         crypto |= CAP_AES;
      } else if (cpuFeatures[index] == "pmull") {
         crypto |= CAP_PMULL;
      } else if (cpuFeatures[index] == "sha1") {
         crypto |= CAP_SHA1;
      } else if (cpuFeatures[index] == "sha2") {
         crypto |= CAP_SHA2;
      }
#endif

      if (polarVMFeatureStr != "") {
         features[polarVMFeatureStr] = true;
      }
   }

#if defined(__aarch64__)
   // If we have all crypto bits we can add the feature
   if (crypto == (CAP_AES | CAP_PMULL | CAP_SHA1 | CAP_SHA2)) {
      features["crypto"] = true;
   }
#endif
   return true;
}
#else
bool get_host_cpu_features(StringMap<bool> &features) { return false; }
#endif

extern std::string update_triple_os_version(std::string targetTripleString);

std::string get_process_triple()
{
   std::string targetTripleString = update_triple_os_version(POLAR_HOST_TRIPLE);
   Triple triple(Triple::normalize(targetTripleString));
   if (sizeof(void *) == 8 && triple.isArch32Bit()) {
      triple = triple.get64BitArchVariant();
   }
   if (sizeof(void *) == 4 && triple.isArch64Bit()) {
      triple = triple.get32BitArchVariant();
   }
   return triple.getStr();
}

} // sys
} // polar
