// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/07.

#include "polarphp/utils/WithColor.h"
#include "polarphp/utils/RawOutStream.h"

namespace polar {
namespace utils {

using polar::cmd::Opt;
using polar::cmd::BoolOrDefault;
using polar::cmd::Category;
using polar::cmd::Desc;

OptionCategory sg_colorCategory("Color Options");

static Opt<BoolOrDefault>
sg_useColor("color", Category(sg_colorCategory),
            Desc("Use colors in output (default=autodetect)"),
            polar::cmd::init(BoolOrDefault::BOU_UNSET));


WithColor::WithColor(RawOutStream &m_outstream, HighlightColor Color, bool disableColors)
   : m_outstream(m_outstream),
     m_disableColors(disableColors)
{
   // Detect color from terminal type unless the user passed the --color option.
   if (colorsEnabled()) {
      switch (Color) {
      case HighlightColor::Address:
         m_outstream.changeColor(RawOutStream::Colors::YELLOW);
         break;
      case HighlightColor::String:
         m_outstream.changeColor(RawOutStream::Colors::GREEN);
         break;
      case HighlightColor::Tag:
         m_outstream.changeColor(RawOutStream::Colors::BLUE);
         break;
      case HighlightColor::Attribute:
         m_outstream.changeColor(RawOutStream::Colors::CYAN);
         break;
      case HighlightColor::Enumerator:
         m_outstream.changeColor(RawOutStream::Colors::MAGENTA);
         break;
      case HighlightColor::Macro:
         m_outstream.changeColor(RawOutStream::Colors::RED);
         break;
      case HighlightColor::Error:
         m_outstream.changeColor(RawOutStream::Colors::RED, true);
         break;
      case HighlightColor::Warning:
         m_outstream.changeColor(RawOutStream::Colors::MAGENTA, true);
         break;
      case HighlightColor::Note:
         m_outstream.changeColor(RawOutStream::Colors::BLACK, true);
         break;
      case HighlightColor::Remark:
         m_outstream.changeColor(RawOutStream::Colors::BLUE, true);
         break;
      }
   }
}

RawOutStream &WithColor::error()
{
   return error(polar::utils::error_stream());
}

RawOutStream &WithColor::warning()
{
   return warning(polar::utils::error_stream());
}

RawOutStream &WithColor::note()
{
   return note(polar::utils::error_stream());
}

RawOutStream &WithColor::remark()
{
   return remark(polar::utils::error_stream());
}

RawOutStream &WithColor::error(RawOutStream &outstream, StringRef prefix,
                               bool disableColors)
{
   if (!prefix.empty()) {
      outstream << prefix << ": ";
   }

   return WithColor(outstream, HighlightColor::Error, disableColors).get()
         << "error: ";
}

RawOutStream &WithColor::warning(RawOutStream &outstream, StringRef prefix,
                                 bool disableColors)
{
   if (!prefix.empty()) {
      outstream << prefix << ": ";
   }
   return WithColor(outstream, HighlightColor::Warning, disableColors).get()
         << "warning: ";
}

RawOutStream &WithColor::note(RawOutStream &outstream, StringRef prefix,
                              bool disableColors)
{
   if (!prefix.empty()) {
      outstream << prefix << ": ";
   }
   return WithColor(outstream, HighlightColor::Note, disableColors).get() << "note: ";
}

RawOutStream &WithColor::remark(RawOutStream &outstream, StringRef prefix,
                                bool disableColors)
{
   if (!prefix.empty()) {
      outstream << prefix << ": ";
   }
   return WithColor(outstream, HighlightColor::Remark, disableColors).get()
         << "remark: ";
}

bool WithColor::colorsEnabled()
{
   if (m_disableColors) {
      return false;
   }
   if (sg_useColor == BoolOrDefault::BOU_UNSET) {
      return m_outstream.hasColors();
   }
   return sg_useColor == BoolOrDefault::BOU_TRUE;
}

WithColor &WithColor::changeColor(RawOutStream::Colors color, bool bold,
                                  bool bg)
{
   if (colorsEnabled()) {
      m_outstream.changeColor(color, bold, bg);
   }
   return *this;
}

WithColor &WithColor::resetColor()
{
   if (colorsEnabled()) {
      m_outstream.resetColor();
   }
   return *this;
}

WithColor::~WithColor()
{
   resetColor();
}

} // utils
} // polar
