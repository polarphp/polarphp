// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/15.

#include "polarphp/utils/TarWriter.h"
#include "polarphp/basic/adt/Twine.h"
#include "polarphp/utils/FileSystem.h"
#include "polarphp/utils/MathExtras.h"
#include "polarphp/utils/Path.h"

namespace polar {
namespace utils {

using polar::basic::StringRef;
using polar::basic::Twine;

// Each file in an archive must be aligned to this block size.
static const int sg_blockSize = 512;

struct UstarHeader
{
   char name[100];
   char Mode[8];
   char Uid[8];
   char Gid[8];
   char Size[12];
   char Mtime[12];
   char Checksum[8];
   char TypeFlag;
   char Linkname[100];
   char Magic[6];
   char Version[2];
   char Uname[32];
   char Gname[32];
   char DevMajor[8];
   char DevMinor[8];
   char prefix[155];
   char Pad[12];
};
static_assert(sizeof(UstarHeader) == sg_blockSize, "invalid Ustar header");

namespace {

UstarHeader make_ustar_header()
{
   UstarHeader header = {};
   memcpy(header.Magic, "ustar", 5); // Ustar magic
   memcpy(header.Version, "00", 2);  // Ustar version
   return header;
}

// A PAX attribute is in the form of "<length> <key>=<value>\n"
// where <length> is the length of the entire string including
// the length field itself. An example string is this.
//
//   25 ctime=1084839148.1212\n
//
// This function create such string.
std::string format_pax(StringRef key, StringRef value)
{
   int length = key.size() + value.size() + 3; // +3 for " ", "=" and "\n"

   // We need to compute total size twice because appending
   // a length field could change total size by one.
   int total = length + Twine(length).getStr().size();
   total = length + Twine(total).getStr().size();
   return (Twine(total) + " " + key + "=" + value + "\n").getStr();
}

// Headers in tar files must be aligned to 512 byte boundaries.
// This function forwards the current file position to the next boundary.
void pad(RawFdOutStream &outstream)
{
   uint64_t pos = outstream.tell();
   outstream.seek(align_to(pos, sg_blockSize));
}

// Computes a checksum for a tar header.
void compute_checksum(UstarHeader &header)
{
   // Before computing a checksum, checksum field must be
   // filled with space characters.
   memset(header.Checksum, ' ', sizeof(header.Checksum));

   // Compute a checksum and set it to the checksum field.
   unsigned chksum = 0;
   for (size_t index = 0; index < sizeof(header); ++index) {
      chksum += reinterpret_cast<uint8_t *>(&header)[index];
   }
   snprintf(header.Checksum, sizeof(header.Checksum), "%06o", chksum);
}

// Create a tar header and write it to a given output stream.
void write_pax_header(RawFdOutStream &outstream, StringRef path)
{
   // A PAX header consists of a 512-byte header followed
   // by key-value strings. First, create key-value strings.
   std::string paxAttr = format_pax("path", path);

   // Create a 512-byte header.
   UstarHeader header = make_ustar_header();
   snprintf(header.Size, sizeof(header.Size), "%011zo", paxAttr.size());
   header.TypeFlag = 'x'; // PAX magic
   compute_checksum(header);

   // Write them down.
   outstream << StringRef(reinterpret_cast<char *>(&header), sizeof(header));
   outstream << paxAttr;
   pad(outstream);
}

// path fits in a Ustar header if
//
// - path is less than 100 characters long, or
// - path is in the form of "<prefix>/<name>" where <prefix> is less
//   than or equal to 155 characters long and <name> is less than 100
//   characters long. Both <prefix> and <name> can contain extra '/'.
//
// If path fits in a Ustar header, updates prefix and name and returns true.
// Otherwise, returns false.
bool split_ustar(StringRef path, StringRef &prefix, StringRef &name)
{
   if (path.size() < sizeof(UstarHeader::name)) {
      prefix = "";
      name = path;
      return true;
   }

   size_t sep = path.rfind('/', sizeof(UstarHeader::prefix) + 1);
   if (sep == StringRef::npos) {
      return false;
   }
   if (path.size() - sep - 1 >= sizeof(UstarHeader::name)) {
      return false;
   }
   prefix = path.substr(0, sep);
   name = path.substr(sep + 1);
   return true;
}

// The PAX header is an extended format, so a PAX header needs
// to be followed by a "real" header.
void write_ustar_header(RawFdOutStream &outstream, StringRef prefix,
                        StringRef name, size_t Size)
{
   UstarHeader header = make_ustar_header();
   memcpy(header.name, name.getData(), name.size());
   memcpy(header.Mode, "0000664", 8);
   snprintf(header.Size, sizeof(header.Size), "%011zo", Size);
   memcpy(header.prefix, prefix.getData(), prefix.size());
   compute_checksum(header);
   outstream << StringRef(reinterpret_cast<char *>(&header), sizeof(header));
}

} // anonymous namespace

// Creates a TarWriter instance and returns it.
Expected<std::unique_ptr<TarWriter>> TarWriter::create(StringRef outputPath,
                                                       StringRef baseDir)
{
   int fd;
   if (std::error_code errorCode = fs::open_file_for_write(outputPath, fd, fs::CD_CreateAlways, fs::F_None)) {
      return make_error<StringError>("cannot open " + outputPath, errorCode);
   }
   return std::unique_ptr<TarWriter>(new TarWriter(fd, baseDir));
}

TarWriter::TarWriter(int fd, StringRef baseDir)
   : m_outstream(fd, /*shouldClose=*/true, /*unbuffered=*/false),
     m_baseDir(baseDir)
{}

// Append a given file to an archive.
void TarWriter::append(StringRef path, StringRef data)
{
   // Write path and data.
   std::string fullpath = m_baseDir + "/" + fs::path::convert_to_slash(path);

   // We do not want to include the same file more than once.
   if (!m_files.insert(fullpath).second) {
      return;
   }

   StringRef prefix;
   StringRef name;
   if (split_ustar(fullpath, prefix, name)) {
      write_ustar_header(m_outstream, prefix, name, data.size());
   } else {
      write_pax_header(m_outstream, fullpath);
      write_ustar_header(m_outstream, "", "", data.size());
   }

   m_outstream << data;
   pad(m_outstream);

   // POSIX requires tar archives end with two null blocks.
   // Here, we write the terminator and then seek back, so that
   // the file being output is terminated correctly at any moment.
   uint64_t pos = m_outstream.tell();
   m_outstream << std::string(sg_blockSize * 2, '\0');
   m_outstream.seek(pos);
   m_outstream.flush();
}

} // utils
} // polar
