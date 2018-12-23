// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/13.

#include "polarphp/basic/adt/Twine.h"
#include "polarphp/basic/adt/SmallString.h"
#include "polarphp/utils/Debug.h"
#include "polarphp/utils/FormatVariadic.h"
#include "polarphp/utils/RawOutStream.h"

namespace polar {
namespace basic {

using polar::utils::RawSvectorOutStream;
using polar::utils::RawOutStream;
using polar::debug_stream;

std::string Twine::getStr() const
{
   // If we're storing only a std::string, just return it.
   if (m_lhsKind == NodeKind::StdStringKind && m_rhsKind == NodeKind::EmptyKind) {
      return *m_lhs.m_stdString;
   }

   // If we're storing a formatv_object, we can avoid an extra copy by formatting
   // it immediately and returning the result.
   if (m_lhsKind == NodeKind::FormatvObjectKind && m_rhsKind == NodeKind::EmptyKind) {
      return m_lhs.m_formatvObject->getStr();
   }
   // Otherwise, flatten and copy the contents first.
   SmallString<256> vector;
   return toStringRef(vector).getStr();
}

void Twine::toVector(SmallVectorImpl<char> &out) const
{
   RawSvectorOutStream outstream(out);
   print(outstream);
}

StringRef Twine::toNullTerminatedStringRef(SmallVectorImpl<char> &out) const
{
   if (isUnary()) {
      switch (getLhsKind()) {
      case NodeKind::CStringKind:
         // Already null terminated, yay!
         return StringRef(m_lhs.m_CString);
      case NodeKind::StdStringKind: {
         const std::string *str = m_lhs.m_stdString;
         return StringRef(str->c_str(), str->size());
      }
      default:
         break;
      }
   }
   toVector(out);
   out.push_back(0);
   out.pop_back();
   return StringRef(out.getData(), out.getSize());
}

void Twine::printOneChild(RawOutStream &outstream, Child ptr,
                          NodeKind kind) const
{
   switch (kind) {
   case NodeKind::NullKind: break;
   case NodeKind::EmptyKind: break;
   case NodeKind::TwineKind:
      ptr.m_twine->print(outstream);
      break;
   case NodeKind::CStringKind:
      outstream << ptr.m_CString;
      break;
   case NodeKind::StdStringKind:
      outstream << *ptr.m_stdString;
      break;
   case NodeKind::StringRefKind:
      outstream << *ptr.m_stringRef;
      break;
   case NodeKind::SmallStringKind:
      outstream << *ptr.m_smallString;
      break;
   case NodeKind::FormatvObjectKind:
      outstream << *ptr.m_formatvObject;
      break;
   case NodeKind::CharKind:
      outstream << ptr.m_character;
      break;
   case NodeKind::DecUIKind:
      outstream << ptr.m_decUInt;
      break;
   case NodeKind::DecIKind:
      outstream << ptr.m_decInt;
      break;
   case NodeKind::DecULKind:
      outstream << *ptr.m_decULong;
      break;
   case NodeKind::DecLKind:
      outstream << *ptr.m_decLong;
      break;
   case NodeKind::DecULLKind:
      outstream << *ptr.m_decULongLong;
      break;
   case NodeKind::DecLLKind:
      outstream << *ptr.m_decLongLong;
      break;
   case NodeKind::UHexKind:
      outstream.writeHex(*ptr.m_uHex);
      break;
   }
}

void Twine::printOneChildRepr(RawOutStream &outstream, Child ptr,
                              NodeKind kind) const
{
   switch (kind) {
   case NodeKind::NullKind:
      outstream << "null"; break;
   case NodeKind::EmptyKind:
      outstream << "empty"; break;
   case NodeKind::TwineKind:
      outstream << "rope:";
      ptr.m_twine->printRepr(outstream);
      break;
   case NodeKind::CStringKind:
      outstream << "cstring:\""
                << ptr.m_CString << "\"";
      break;
   case NodeKind::StdStringKind:
      outstream << "std::string:\""
                << ptr.m_stdString << "\"";
      break;
   case NodeKind::StringRefKind:
      outstream << "stringref:\""
                << ptr.m_stringRef << "\"";
      break;
   case NodeKind::SmallStringKind:
      outstream << "smallstring:\"" << *ptr.m_smallString << "\"";
      break;
   case NodeKind::FormatvObjectKind:
      outstream << "formatv:\"" << *ptr.m_formatvObject << "\"";
      break;
   case NodeKind::CharKind:
      outstream << "char:\"" << ptr.m_character << "\"";
      break;
   case NodeKind::DecUIKind:
      outstream << "decUI:\"" << ptr.m_decUInt << "\"";
      break;
   case NodeKind::DecIKind:
      outstream << "decI:\"" << ptr.m_decInt << "\"";
      break;
   case NodeKind::DecULKind:
      outstream << "decUL:\"" << *ptr.m_decULong << "\"";
      break;
   case NodeKind::DecLKind:
      outstream << "decL:\"" << *ptr.m_decLong << "\"";
      break;
   case NodeKind::DecULLKind:
      outstream << "decULL:\"" << *ptr.m_decULongLong << "\"";
      break;
   case NodeKind::DecLLKind:
      outstream << "decLL:\"" << *ptr.m_decLongLong << "\"";
      break;
   case NodeKind::UHexKind:
      outstream << "uhex:\"" << ptr.m_uHex << "\"";
      break;
   }
}

void Twine::print(RawOutStream &outstream) const
{
   printOneChild(outstream, m_lhs, getLhsKind());
   printOneChild(outstream, m_rhs, getRhsKind());
}

void Twine::printRepr(RawOutStream &outstream) const
{
   outstream << "(Twine ";
   printOneChildRepr(outstream, m_lhs, getLhsKind());
   outstream << " ";
   printOneChildRepr(outstream, m_rhs, getRhsKind());
   outstream << ")";
}

#if !defined(NDEBUG) || defined(POLAR_ENABLE_DUMP)
POLAR_DUMP_METHOD void Twine::dump() const
{
   print(debug_stream());
}

POLAR_DUMP_METHOD void Twine::dumpRepr() const
{
   printRepr(debug_stream());
}
#endif

} // basic
} // polar
