// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://polarphp.org/LICENSE.txt for license information
// See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/10/12.

#ifndef POLAR_DEVLTOOLS_UTILS_FORMATTED_STREAM_H
#define POLAR_DEVLTOOLS_UTILS_FORMATTED_STREAM_H

#include <ostream>
#include <utility>
#include <array>

namespace polar {
namespace utils {

template <typename _CharT, typename _Traits, typename _Allocator>
class FormattedStream;

// basic_stringbuf
template <typename _CharT, typename _Traits, typename _Allocator>
class FormattedStreamBuffer
      : public std::basic_streambuf<_CharT, _Traits>
{
public:
   typedef _CharT                         char_type;
   typedef _Traits                        traits_type;
   typedef typename traits_type::int_type int_type;
   typedef typename traits_type::pos_type pos_type;
   typedef typename traits_type::off_type off_type;
   typedef _Allocator                     allocator_type;

private:
   std::array<char_type, 512> m_buffer;
   mutable char_type* __hm_;
   std::ios_base::openmode m_openMode;

public:
   // 27.8.1.1 Constructors:
   inline explicit FormattedStreamBuffer(FormattedStream<_CharT, _Traits, _Traits> stream,
                                         std::ios_base::openmode openMode = std::ios_base::in | std::ios_base::out);
   FormattedStreamBuffer(FormattedStreamBuffer &&other);
   FormattedStreamBuffer &operator=(FormattedStreamBuffer &&other);
   void swap(FormattedStreamBuffer &other);
protected:
   // 27.8.1.4 Overridden virtual functions:
   virtual int_type underflow();
   virtual int_type pbackfail(int_type c = traits_type::eof());
   virtual int_type overflow (int_type c = traits_type::eof());
   virtual pos_type seekoff(off_type offset, std::ios_base::seekdir way,
                            std::ios_base::openmode openMode = std::ios_base::in | std::ios_base::out);
   inline virtual pos_type seekpos(pos_type pos,
                                   std::ios_base::openmode openMode = std::ios_base::in | std::ios_base::out);
};

template <class _CharT, class _Traits, class _Allocator>
class FormattedStream
      : public std::basic_ostream<_CharT, _Traits>
{
public:
   typedef _CharT                         char_type;
   typedef _Traits                        traits_type;
   typedef typename traits_type::int_type int_type;
   typedef typename traits_type::pos_type pos_type;
   typedef typename traits_type::off_type off_type;
   typedef _Allocator                     allocator_type;
private:
   FormattedStreamBuffer<char_type, traits_type, allocator_type> m_buffer;
public:
   // 27.8.2.1 Constructors:
   inline explicit FormattedStream(std::ios_base::openmode openMode = std::ios_base::out);
   inline FormattedStream(FormattedStream &&other);
   FormattedStream& operator=(FormattedStream &&other);
   inline void swap(FormattedStream& other);
   inline FormattedStreamBuffer<char_type, traits_type, allocator_type>* rdbuf() const;
};

} // utils
} // polar

#endif // POLAR_DEVLTOOLS_UTILS_FORMATTED_STREAM_H
