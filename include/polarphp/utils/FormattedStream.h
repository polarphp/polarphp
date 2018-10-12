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

#ifndef POLARPHP_UTILS_FORMATTED_STREAM_H
#define POLARPHP_UTILS_FORMATTED_STREAM_H

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
   inline explicit FormattedStreamBuffer(FormattedStream<_CharT, _Traits, _Traits> &stream,
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
protected:
   FormattedStream<_CharT, _Traits, _Traits> &m_stream;
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
public:
   // 27.8.2.1 Constructors:
   inline explicit FormattedStream(std::ostream &stream,
                                   std::ios_base::openmode openMode = std::ios_base::out);
   inline FormattedStream(FormattedStream &&other);
   FormattedStream& operator=(FormattedStream &&other);
   inline void swap(FormattedStream& other);
   inline FormattedStreamBuffer<char_type, traits_type, allocator_type>* rdbuf() const;
private:
   FormattedStreamBuffer<char_type, traits_type, allocator_type> m_buffer;
   std::ostream &m_upstream;
};

template <class _CharT, class _Traits, class _Allocator>
FormattedStream<_CharT, _Traits, _Allocator>::FormattedStream(std::ostream &stream, std::ios_base::openmode openMode)
   : basic_ostream<_CharT, _Traits>(&m_buffer),
     m_buffer(*this, openMode | ios_base::out),
     m_upstream(stream)
{
}

template <class _CharT, class _Traits, class _Allocator>
FormattedStream<_CharT, _Traits, _Allocator>::FormattedStream(FormattedStream &&other)
   : basic_ostream<_CharT, _Traits>(_VSTD::move(other)),
     m_buffer(std::move(other.m_buffer))
{
   basic_ostream<_CharT, _Traits>::set_rdbuf(&m_buffer);
}

template <class _CharT, class _Traits, class _Allocator>
FormattedStream<_CharT, _Traits, _Allocator>&
FormattedStream<_CharT, _Traits, _Allocator>::operator=(FormattedStream &&other)
{
   basic_ostream<char_type, traits_type>::operator=(std::move(other));
   m_buffer = std::move(other.m_buffer);
   return *this;
}

template <class _CharT, class _Traits, class _Allocator>
void FormattedStream<_CharT, _Traits, _Allocator>::swap(FormattedStream &other)
{
   basic_ostream<char_type, traits_type>::swap(other);
   m_buffer.swap(other.m_buffer);
}

template <class _CharT, class _Traits, class _Allocator>
inline void swap(FormattedStream<_CharT, _Traits, _Allocator>& lhs,
                 FormattedStream<_CharT, _Traits, _Allocator>& rhs)
{
   lhs.swap(rhs);
}

template <class _CharT, class _Traits, class _Allocator>
FormattedStreamBuffer<_CharT, _Traits, _Allocator>*
FormattedStream<_CharT, _Traits, _Allocator>::rdbuf() const
{
   return const_cast<FormattedStreamBuffer<char_type, traits_type, allocator_type>*>(&m_buffer);
}

} // utils
} // polar

#endif // POLARPHP_UTILS_FORMATTED_STREAM_H
