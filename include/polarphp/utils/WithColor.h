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

#ifndef POLARPHP_UTILS_WITH_COLOR_H
#define POLARPHP_UTILS_WITH_COLOR_H

#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/utils/CommandLine.h"

namespace polar {
namespace utils {

using polar::basic::StringRef;
using polar::cmd::OptionCategory;

extern OptionCategory sg_colorCategory;

class RawOutStream;

// Symbolic names for various syntax elements.
enum class HighlightColor {
   Address,
   String,
   Tag,
   Attribute,
   Enumerator,
   Macro,
   Error,
   Warning,
   Note,
   Remark
};

/// An RAII object that temporarily switches an output stream to a specific
/// color.
class WithColor
{
   RawOutStream &m_outstream;
   bool m_disableColors;

public:
   /// To be used like this: WithColor(m_outstream, HighlightColor::String) << "text";
   /// @param m_outstream The output stream
   /// @param S Symbolic name for syntax element to color
   /// @param m_disableColors Whether to ignore color changes regardless of -color
   /// and support in m_outstream
   WithColor(RawOutStream &outstream, HighlightColor color, bool disableColors = false);
   /// To be used like this: WithColor(m_outstream, RawOutStream::Black) << "text";
   /// @param m_outstream The output stream
   /// @param color ANSI color to use, the special SAVEDCOLOR can be used to
   /// change only the bold attribute, and keep colors untouched
   /// @param bold bold/brighter text, default false
   /// @param bg If true, change the background, default: change foreground
   /// @param m_disableColors Whether to ignore color changes regardless of -color
   /// and support in m_outstream
   WithColor(RawOutStream &outstream,
             RawOutStream::Colors color = RawOutStream::Colors::SAVEDCOLOR,
             bool bold = false, bool bg = false, bool disableColors = false)
      : m_outstream(outstream),
        m_disableColors(disableColors)
   {
      changeColor(color, bold, bg);
   }
   ~WithColor();

   RawOutStream &get()
   {
      return m_outstream;
   }

   operator RawOutStream &()
   {
      return m_outstream;
   }

   template <typename T>
   WithColor &operator<<(T &data)
   {
      m_outstream << data;
      return *this;
   }

   template <typename T>
   WithColor &operator<<(const T &data)
   {
      m_outstream << data;
      return *this;
   }

   /// Convenience method for printing "error: " to stderr.
   static RawOutStream &error();
   /// Convenience method for printing "warning: " to stderr.
   static RawOutStream &warning();
   /// Convenience method for printing "note: " to stderr.
   static RawOutStream &note();
   /// Convenience method for printing "remark: " to stderr.
   static RawOutStream &remark();

   /// Convenience method for printing "error: " to the given stream.
   static RawOutStream &error(RawOutStream &outstream, StringRef prefix = "",
                              bool disableColors = false);
   /// Convenience method for printing "warning: " to the given stream.
   static RawOutStream &warning(RawOutStream &outstream, StringRef prefix = "",
                                bool disableColors = false);
   /// Convenience method for printing "note: " to the given stream.
   static RawOutStream &note(RawOutStream &outstream, StringRef prefix = "",
                             bool disableColors = false);
   /// Convenience method for printing "remark: " to the given stream.
   static RawOutStream &remark(RawOutStream &outstream, StringRef prefix = "",
                               bool disableColors = false);

   /// Determine whether colors are displayed.
   bool colorsEnabled();

   /// Change the color of text that will be output from this point forward.
   /// @param color ANSI color to use, the special SAVEDCOLOR can be used to
   /// change only the bold attribute, and keep colors untouched
   /// @param bold bold/brighter text, default false
   /// @param bg If true, change the background, default: change foreground
   WithColor &changeColor(RawOutStream::Colors color, bool bold = false,
                          bool bg = false);

   /// Reset the colors to terminal defaults. Call this when you are done
   /// outputting colored text, or before program exit.
   WithColor &resetColor();
};

} // utils
} // polar

#endif // POLARPHP_UTILS_WITH_COLOR_H
