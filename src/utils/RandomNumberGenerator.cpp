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

//===----------------------------------------------------------------------===//
//
// This file implements deterministic random number generation (RNG).
// The current implementation is NOT cryptographically secure as it uses
// the C++11 <random> facilities.
//
//===----------------------------------------------------------------------===//

#include "polarphp/utils/RandomNumberGenerator.h"
#include "polarphp/utils/CommandLine.h"
#include "polarphp/utils/Debug.h"
#include "polarphp/utils/RawOutStream.h"
#include "polarphp/utils/unix/Unix.h"

namespace polar {
namespace utils {

#define DEBUG_TYPE "rng"

// Tracking BUG: 19665
// http://llvm.org/bugs/show_bug.cgi?id=19665
//
// Do not change to cmd::Opt<uint64_t> since this silently breaks argument parsing.
static cmd::Opt<unsigned long long>
sg_seed("rng-seed", cmd::ValueDesc("seed"), cmd::Hidden,
        cmd::Desc("sg_seed for the random number generator"), cmd::init(0));

RandomNumberGenerator::RandomNumberGenerator(StringRef salt)
{
   POLAR_DEBUG(
            if (sg_seed == 0)
            debug_stream() << "Warning! Using unseeded random number generator.\n"
            );

   // Combine seed and salts using std::seed_seq.
   // data: sg_seed-low, sg_seed-high, salt
   // Note: std::seed_seq can only store 32-bit values, even though we
   // are using a 64-bit RNG. This isn't a problem since the Mersenne
   // twister constructor copies these correctly into its initial state.
   std::vector<uint32_t> data;
   data.resize(2 + salt.getSize());
   data[0] = sg_seed;
   data[1] = sg_seed >> 32;

   polar::basic::copy(salt, data.begin() + 2);

   std::seed_seq SeedSeq(data.begin(), data.end());
   m_generator.seed(SeedSeq);
}

RandomNumberGenerator::result_type RandomNumberGenerator::operator()()
{
   return m_generator();
}

// Get random vector of specified size
std::error_code get_random_bytes(void *buffer, size_t size) {
#ifdef POLAR_OS_WIN32
   HCRYPTPROV hProvider;
   if (CryptAcquireContext(&hProvider, 0, 0, PROV_RSA_FULL,
                           CRYPT_VERIFYCONTEXT | CRYPT_SILENT)) {
      ScopedCryptContext ScopedHandle(hProvider);
      if (CryptGenRandom(hProvider, size, static_cast<BYTE *>(buffer))) {
         return std::error_code();
      }
   }
   return std::error_code(GetLastError(), std::system_category());
#else
   int fd = open("/dev/urandom", O_RDONLY);
   if (fd != -1) {
      std::error_code ret;
      ssize_t bytesRead = read(fd, buffer, size);
      if (bytesRead == -1) {
         ret = std::error_code(errno, std::system_category());
      } else if (bytesRead != static_cast<ssize_t>(size)) {
         ret = std::error_code(EIO, std::system_category());
      }
      if (close(fd) == -1) {
         ret = std::error_code(errno, std::system_category());
      }
      return ret;
   }
   return std::error_code(errno, std::system_category());
#endif
}

} // utils
} // polar
