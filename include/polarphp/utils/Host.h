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

#ifndef POLARPHP_UTILS_HOST_H
#define POLARPHP_UTILS_HOST_H

#include "polarphp/basic/adt/StringMap.h"
#include "polarphp/basic/adt/StringRef.h"

#if defined(__linux__) || defined(__GNU__) || defined(__HAIKU__)
#include <endian.h>
#elif defined(_AIX)
#include <sys/machine.h>
#elif defined(__sun)
/* Solaris provides _BIG_ENDIAN/_LITTLE_ENDIAN selector in sys/types.h */
#include <sys/types.h>
#define BIG_ENDIAN 4321
#define LITTLE_ENDIAN 1234
#if defined(_BIG_ENDIAN)
#define BYTE_ORDER BIG_ENDIAN
#else
#define BYTE_ORDER LITTLE_ENDIAN
#endif
#else
#if !defined(BYTE_ORDER) && !defined(_WIN32)
#include <machine/endian.h>
#endif
#endif

#include <string>

namespace polar {
namespace sys {

using polar::basic::StringRef;
using polar::basic::StringMap;

#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && BYTE_ORDER == BIG_ENDIAN
static constexpr bool sg_isBigEndianHost = true;
#else
static constexpr bool sg_isBigEndianHost = false;
#endif

static constexpr bool sg_isLittleEndianHost = !sg_isBigEndianHost;

/// getDefaultTargetTriple() - Return the default target triple the compiler
/// has been configured to produce code for.
///
/// The target triple is a string in the format of:
///   CPU_TYPE-VENDOR-OPERATING_SYSTEM
/// or
///   CPU_TYPE-VENDOR-KERNEL-OPERATING_SYSTEM
std::string get_default_target_triple();

/// getProcessTriple() - Return an appropriate target triple for generating
/// code to be loaded into the current process, e.g. when using the JIT.
std::string get_process_triple();

/// getHostCPUName - Get the LLVM name for the host CPU. The particular format
/// of the name is target dependent, and suitable for passing as -mcpu to the
/// target which matches the host.
///
/// \return - The host CPU name, or empty if the CPU could not be determined.
StringRef get_host_cpu_name();

/// getHostCPUFeatures - Get the LLVM names for the host CPU features.
/// The particular format of the names are target dependent, and suitable for
/// passing as -mattr to the target which matches the host.
///
/// \param Features - A string mapping feature names to either
/// true (if enabled) or false (if disabled). This routine makes no guarantees
/// about exactly which features may appear in this map, except that they are
/// all valid LLVM feature names.
///
/// \return - True on success.
bool get_host_cpu_features(StringMap<bool> &features);

/// Get the number of physical cores (as opposed to logical cores returned
/// from thread::hardware_concurrency(), which includes hyperthreads).
/// Returns -1 if unknown for the current host system.
int get_host_num_physical_cores();

namespace internal {
/// Helper functions to extract HostCPUName from /proc/cpuinfo on linux.
StringRef get_host_cpu_name_for_powerpc(StringRef procCpuinfoContent);
StringRef get_host_cpu_name_for_arm(StringRef procCpuinfoContent);
StringRef get_host_cpu_name_for_s390x(StringRef procCpuinfoContent);
StringRef get_host_cpu_name_for_bpf();
} // internal

} // sys
} // polar

#endif // POLARPHP_UTILS_HOST_H
