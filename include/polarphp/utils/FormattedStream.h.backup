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
   mutable char_type* m_hm;
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
FormattedStreamBuffer<_CharT, _Traits, _Allocator>::FormattedStreamBuffer(FormattedStream<_CharT, _Traits, _Traits> &stream,
                                                                          std::ios_base::openmode openMode)
   : m_hm(0),
     m_openMode(openMode),
     m_stream(stream)
{
}

template <class _CharT, class _Traits, class _Allocator>
FormattedStreamBuffer<_CharT, _Traits, _Allocator>::FormattedStreamBuffer(FormattedStreamBuffer &&other)
   : m_openMode(other.m_openMode)
{
   char_type* p = const_cast<char_type*>(other.m_buffer.data);
   ptrdiff_t binp = -1;
   ptrdiff_t ninp = -1;
   ptrdiff_t einp = -1;
   if (other.eback() != nullptr)
   {
      binp = other.eback() - p;
      ninp = other.gptr() - p;
      einp = other.egptr() - p;
   }
   ptrdiff_t bout = -1;
   ptrdiff_t nout = -1;
   ptrdiff_t eout = -1;
   if (other.pbase() != nullptr)
   {
      bout = other.pbase() - p;
      nout = other.pptr() - p;
      eout = other.epptr() - p;
   }
   ptrdiff_t hm = other.m_hm == nullptr ? -1 : other.m_hm - p;
   m_buffer = std::move(other.m_buffer);
   p = const_cast<char_type*>(m_buffer.data());
   if (binp != -1) {
      this->setg(p + binp, p + ninp, p + einp);
   }
   if (bout != -1)
   {
      this->setp(p + bout, p + eout);
      this->__pbump(nout);
   }
   m_hm = hm == -1 ? nullptr : p + hm;
   p = const_cast<char_type*>(other.m_buffer.data());
   other.setg(p, p, p);
   other.setp(p, p);
   other.m_hm = p;
   this->pubimbue(other.getloc());
}

template <class _CharT, class _Traits, class _Allocator>
FormattedStreamBuffer<_CharT, _Traits, _Allocator>&
FormattedStreamBuffer<_CharT, _Traits, _Allocator>::operator=(FormattedStreamBuffer &&other)
{
   char_type* p = const_cast<char_type*>(other.m_buffer.data());
   ptrdiff_t binp = -1;
   ptrdiff_t ninp = -1;
   ptrdiff_t einp = -1;
   if (other.eback() != nullptr)
   {
      binp = other.eback() - p;
      ninp = other.gptr() - p;
      einp = other.egptr() - p;
   }
   ptrdiff_t bout = -1;
   ptrdiff_t nout = -1;
   ptrdiff_t eout = -1;
   if (other.pbase() != nullptr)
   {
      bout = other.pbase() - p;
      nout = other.pptr() - p;
      eout = other.epptr() - p;
   }
   ptrdiff_t hm = other.m_hm == nullptr ? -1 : other.m_hm - p;
   m_buffer = std::move(other.m_buffer);
   p = const_cast<char_type*>(m_buffer.data());
   if (binp != -1) {
      this->setg(p + binp, p + ninp, p + einp);
   } else {
      this->setg(nullptr, nullptr, nullptr);
   }
   if (bout != -1) {
      this->setp(p + bout, p + eout);
      this->__pbump(nout);
   } else {
      this->setp(nullptr, nullptr);
   }

   m_hm = hm == -1 ? nullptr : p + hm;
   m_openMode = other.m_openMode;
   p = const_cast<char_type*>(other.m_buffer.data());
   other.setg(p, p, p);
   other.setp(p, p);
   other.m_hm = p;
   this->pubimbue(other.getloc());
   return *this;
}

template <class _CharT, class _Traits, class _Allocator>
void FormattedStreamBuffer<_CharT, _Traits, _Allocator>::swap(FormattedStreamBuffer &other)
{
   char_type* p = const_cast<char_type*>(other.m_buffer.data());
   ptrdiff_t rbinp = -1;
   ptrdiff_t rninp = -1;
   ptrdiff_t reinp = -1;
   if (other.eback() != nullptr)
   {
      rbinp = other.eback() - p;
      rninp = other.gptr() - p;
      reinp = other.egptr() - p;
   }
   ptrdiff_t rbout = -1;
   ptrdiff_t rnout = -1;
   ptrdiff_t reout = -1;
   if (other.pbase() != nullptr)
   {
      rbout = other.pbase() - p;
      rnout = other.pptr() - p;
      reout = other.epptr() - p;
   }
   ptrdiff_t rhm = other.m_hm == nullptr ? -1 : other.m_hm - p;
   p = const_cast<char_type*>(m_buffer.data());
   ptrdiff_t lbinp = -1;
   ptrdiff_t lninp = -1;
   ptrdiff_t leinp = -1;
   if (this->eback() != nullptr)
   {
      lbinp = this->eback() - p;
      lninp = this->gptr() - p;
      leinp = this->egptr() - p;
   }
   ptrdiff_t lbout = -1;
   ptrdiff_t lnout = -1;
   ptrdiff_t leout = -1;
   if (this->pbase() != nullptr)
   {
      lbout = this->pbase() - p;
      lnout = this->pptr() - p;
      leout = this->epptr() - p;
   }
   ptrdiff_t lhm = m_hm == nullptr ? -1 : m_hm - p;
   std::swap(m_openMode, other.m_openMode);
   m_buffer.swap(other.m_buffer);
   p = const_cast<char_type*>(m_buffer.data());
   if (rbinp != -1) {
      this->setg(p + rbinp, p + rninp, p + reinp);
   } else {
      this->setg(nullptr, nullptr, nullptr);
   }
   if (rbout != -1)
   {
      this->setp(p + rbout, p + reout);
      this->__pbump(rnout);
   } else {
      this->setp(nullptr, nullptr);
   }
   m_hm = rhm == -1 ? nullptr : p + rhm;
   p = const_cast<char_type*>(other.m_buffer.data());
   if (lbinp != -1) {
      other.setg(p + lbinp, p + lninp, p + leinp);
   } else {
      other.setg(nullptr, nullptr, nullptr);
   }

   if (lbout != -1)
   {
      other.setp(p + lbout, p + leout);
      other.__pbump(lnout);
   } else {
      other.setp(nullptr, nullptr);
   }
   other.m_hm = lhm == -1 ? nullptr : p + lhm;
   std::locale tl = other.getloc();
   other.pubimbue(this->getloc());
   this->pubimbue(tl);
}

template <class _CharT, class _Traits, class _Allocator>
inline void swap(FormattedStreamBuffer<_CharT, _Traits, _Allocator> &lhs,
                 FormattedStreamBuffer<_CharT, _Traits, _Allocator> &rhs)
{
   lhs.swap(rhs);
}

template <class _CharT, class _Traits, class _Allocator>
typename FormattedStreamBuffer<_CharT, _Traits, _Allocator>::int_type
FormattedStreamBuffer<_CharT, _Traits, _Allocator>::underflow()
{
   if (m_hm < this->pptr()) {
      m_hm = this->pptr();
   }
   if (m_openMode & std::ios_base::in) {
      if (this->egptr() < m_hm) {
         this->setg(this->eback(), this->gptr(), m_hm);
      }
      if (this->gptr() < this->egptr()) {
         return traits_type::to_int_type(*this->gptr());
      }
   }
   return traits_type::eof();
}

template <class _CharT, class _Traits, class _Allocator>
typename FormattedStreamBuffer<_CharT, _Traits, _Allocator>::int_type
FormattedStreamBuffer<_CharT, _Traits, _Allocator>::pbackfail(int_type c)
{
   if (m_hm < this->pptr()) {
      m_hm = this->pptr();
   }
   if (this->eback() < this->gptr())
   {
      if (traits_type::eq_int_type(c, traits_type::eof()))
      {
         this->setg(this->eback(), this->gptr()-1, m_hm);
         return traits_type::not_eof(c);
      }
      if ((m_openMode & ios_base::out) ||
          traits_type::eq(traits_type::to_char_type(c), this->gptr()[-1]))
      {
         this->setg(this->eback(), this->gptr()-1, m_hm);
         *this->gptr() = traits_type::to_char_type(c);
         return c;
      }
   }
   return traits_type::eof();
}

template <class _CharT, class _Traits, class _Allocator>
typename FormattedStreamBuffer<_CharT, _Traits, _Allocator>::int_type
FormattedStreamBuffer<_CharT, _Traits, _Allocator>::overflow(int_type c)
{
   if (!traits_type::eq_int_type(c, traits_type::eof()))
   {
      ptrdiff_t ninp = this->gptr()  - this->eback();
      if (this->pptr() == this->epptr())
      {
         if (!(m_openMode & ios_base::out)) {
            return traits_type::eof();
         }
         try
         {
            ptrdiff_t nout = this->pptr()  - this->pbase();
            ptrdiff_t hm = m_hm - this->pbase();
            m_buffer.push_back(char_type());
            m_buffer.resize(m_buffer.capacity());
            char_type* p = const_cast<char_type*>(m_buffer.data());
            this->setp(p, p + m_buffer.size());
            this->__pbump(nout);
            m_hm = this->pbase() + hm;
         }
         catch (...)
         {
            return traits_type::eof();
         }
      }
      m_hm = std::max(this->pptr() + 1, m_hm);
      if (m_openMode & std::ios_base::in)
      {
         char_type* p = const_cast<char_type*>(m_buffer.data());
         this->setg(p, p + ninp, m_hm);
      }
      return this->sputc(c);
   }
   return traits_type::not_eof(c);
}

template <class _CharT, class _Traits, class _Allocator>
typename FormattedStreamBuffer<_CharT, _Traits, _Allocator>::pos_type
FormattedStreamBuffer<_CharT, _Traits, _Allocator>::seekoff(off_type offset,
                                                            ios_base::seekdir way,
                                                            ios_base::openmode openMode)
{
   if (m_hm < this->pptr()) {
      m_hm = this->pptr();
   }
   if ((openMode & (std::ios_base::in | std::ios_base::out)) == 0) {
      return pos_type(-1);
   }

   if ((openMode & (std::ios_base::in | std::ios_base::out)) == (std::ios_base::in | std::ios_base::out)
       && way == std::ios_base::cur) {
      return pos_type(-1);
   }
   const ptrdiff_t hm = m_hm == nullptr ? 0 : m_hm - m_buffer.data();
   off_type noff;
   switch (way)
   {
   case std::ios_base::beg:
      noff = 0;
      break;
   case std::ios_base::cur:
      if (openMode & std::ios_base::in) {
         noff = this->gptr() - this->eback();
      } else {
         noff = this->pptr() - this->pbase();
      }
      break;
   case std::ios_base::end:
      noff = hm;
      break;
   default:
      return pos_type(-1);
   }
   noff += offset;
   if (noff < 0 || hm < noff) {
      return pos_type(-1);
   }
   if (noff != 0)
   {
      if ((openMode & std::ios_base::in) && this->gptr() == 0) {
         return pos_type(-1);
      }
      if ((openMode & std::ios_base::out) && this->pptr() == 0) {
         return pos_type(-1);
      }
   }
   if (openMode & std::ios_base::in) {
      this->setg(this->eback(), this->eback() + noff, m_hm);
   }

   if (openMode & std::ios_base::out)
   {
      this->setp(this->pbase(), this->epptr());
      this->pbump(noff);
   }
   return pos_type(noff);
}

template <class _CharT, class _Traits, class _Allocator>
typename FormattedStreamBuffer<_CharT, _Traits, _Allocator>::pos_type
FormattedStreamBuffer<_CharT, _Traits, _Allocator>::seekpos(pos_type sp,
                                                            std::ios_base::openmode openMode)
{
   return seekoff(sp, std::ios_base::beg, openMode);
}

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
