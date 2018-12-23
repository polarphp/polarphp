// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/03.

#ifndef POLARPHP_UTILS_FORMAT_VARIADIC_H
#define POLARPHP_UTILS_FORMAT_VARIADIC_H

#include "polarphp/basic/adt/StlExtras.h"
#include "polarphp/basic/adt/SmallString.h"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/utils/FormatCommon.h"
#include "polarphp/utils/FormatProviders.h"
#include "polarphp/utils/FormatVariadicDetail.h"
#include "polarphp/utils/RawOutStream.h"
#include <cstddef>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
#include <optional>

namespace polar {
namespace utils {

enum class ReplacementType
{
   Empty,
   Format,
   Literal
};

struct ReplacementItem
{
   ReplacementItem() = default;
   explicit ReplacementItem(StringRef literal)
      : m_type(ReplacementType::Literal), m_spec(literal)
   {}

   ReplacementItem(StringRef spec, size_t index, size_t align, AlignStyle where,
                   char pad, StringRef options)
      : m_type(ReplacementType::Format), m_spec(spec),
        m_index(index), m_align(align),
        m_where(where), m_pad(pad),
        m_options(options)
   {}

   ReplacementType m_type = ReplacementType::Empty;
   StringRef m_spec;
   size_t m_index = 0;
   size_t m_align = 0;
   AlignStyle m_where = AlignStyle::Right;
   char m_pad;
   StringRef m_options;
};

class FormatvObjectBase
{
protected:
   // The parameters are stored in a std::tuple, which does not provide runtime
   // indexing capabilities.  In order to enable runtime indexing, we use this
   // structure to put the parameters into a std::vector.  Since the parameters
   // are not all the same type, we use some type-erasure by wrapping the
   // parameters in a template class that derives from a non-template superclass.
   // Essentially, we are converting a std::tuple<Derived<Ts...>> to a
   // std::vector<Base*>.
   struct create_adapters {
      template <typename... Ts>
      std::vector<internal::FormatAdapterImpl *> operator()(Ts &... items)
      {
         return std::vector<internal::FormatAdapterImpl *>{&items...};
      }
   };

   StringRef m_fmt;
   std::vector<internal::FormatAdapterImpl *> m_adapters;
   std::vector<ReplacementItem> m_replacements;

   static bool consumeFieldLayout(StringRef &spec, AlignStyle &where,
                                  size_t &align, char &pad);

   static std::pair<ReplacementItem, StringRef>
   splitLiteralAndReplacement(StringRef fmt);

public:
   FormatvObjectBase(StringRef fmt, std::size_t paramCount)
      : m_fmt(fmt), m_replacements(parseFormatString(fmt))
   {
      m_adapters.reserve(paramCount);
   }

   FormatvObjectBase(FormatvObjectBase const &rhs) = delete;

   FormatvObjectBase(FormatvObjectBase &&rhs)
      : m_fmt(std::move(rhs.m_fmt)),
        m_adapters(), // m_adapters are initialized by FormatvObject
        m_replacements(std::move(rhs.m_replacements))
   {
      m_adapters.reserve(rhs.m_adapters.size());
   }

   void format(RawOutStream &outStream) const
   {
      for (auto &replacement : m_replacements) {
         if (replacement.m_type == ReplacementType::Empty)
            continue;
         if (replacement.m_type == ReplacementType::Literal) {
            outStream << replacement.m_spec;
            continue;
         }
         if (replacement.m_index >= m_adapters.size()) {
            outStream << replacement.m_spec;
            continue;
         }

         auto w = m_adapters[replacement.m_index];

         FmtAlign align(*w, replacement.m_where, replacement.m_align, replacement.m_pad);
         align.format(outStream, replacement.m_options);
      }
   }

   static std::vector<ReplacementItem> parseFormatString(StringRef fmt);

   static std::optional<ReplacementItem> parseReplacementItem(StringRef spec);

   std::string getStr() const
   {
      std::string result;
      RawStringOutStream stream(result);
      stream << *this;
      stream.flush();
      return result;
   }

   template <unsigned N> SmallString<N> getSmallStr() const
   {
      SmallString<N> result;
      RawSvectorOutStream stream(result);
      stream << *this;
      return result;
   }

   template <unsigned N> operator SmallString<N>() const
   {
      return getSmallStr<N>();
   }

   operator std::string() const
   {
      return getStr();
   }
};

template <typename Tuple>
class FormatvObject : public FormatvObjectBase
{
   // Storage for the parameter adapters.  Since the base class erases the type
   // of the parameters, we have to own the storage for the parameters here, and
   // have the base class store type-erased pointers into this tuple.
   Tuple m_parameters;

public:
   FormatvObject(StringRef fmt, Tuple &&params)
      : FormatvObjectBase(fmt, std::tuple_size<Tuple>::value),
        m_parameters(std::move(params))
   {
      m_adapters = polar::basic::apply_tuple(create_adapters(), m_parameters);
   }

   FormatvObject(FormatvObject const &rhs) = delete;

   FormatvObject(FormatvObject &&rhs)
      : FormatvObjectBase(std::move(rhs)),
        m_parameters(std::move(rhs.m_parameters))
   {
      m_adapters = polar::basic::apply_tuple(create_adapters(), m_parameters);
   }
};

// Format text given a format string and replacement parameters.
//
// ===General Description===
//
// Formats textual output.  `Fmt` is a string consisting of one or more
// replacement sequences with the following grammar:
//
// rep_field ::= "{" [index] ["," layout] [":" format] "}"
// index     ::= <non-negative integer>
// layout    ::= [[[char]loc]width]
// format    ::= <any string not containing "{" or "}">
// char      ::= <any character except "{" or "}">
// loc       ::= "-" | "=" | "+"
// width     ::= <positive integer>
//
// index   - A non-negative integer specifying the index of the item in the
//           parameter pack to print.  Any other value is invalid.
// layout  - A string controlling how the field is laid out within the available
//           space.
// format  - A type-dependent string used to provide additional options to
//           the formatting operation.  Refer to the documentation of the
//           various individual format providers for per-type options.
// char    - The padding character.  Defaults to ' ' (space).  Only valid if
//           `loc` is also specified.
// loc     - Where to print the formatted text within the field.  Only valid if
//           `width` is also specified.
//           '-' : The field is left aligned within the available space.
//           '=' : The field is centered within the available space.
//           '+' : The field is right aligned within the available space (this
//                 is the default).
// width   - The width of the field within which to print the formatted text.
//           If this is less than the required length then the `char` and `loc`
//           fields are ignored, and the field is printed with no leading or
//           trailing padding.  If this is greater than the required length,
//           then the text is output according to the value of `loc`, and padded
//           as appropriate on the left and/or right by `char`.
//
// ===Special Characters===
//
// The characters '{' and '}' are reserved and cannot appear anywhere within a
// replacement sequence.  Outside of a replacement sequence, in order to print
// a literal '{' or '}' it must be doubled -- "{{" to print a literal '{' and
// "}}" to print a literal '}'.
//
// ===Parameter Indexing===
// `index` specifies the index of the parameter in the parameter pack to format
// into the output.  Note that it is possible to refer to the same parameter
// index multiple times in a given format string.  This makes it possible to
// output the same value multiple times without passing it multiple times to the
// function. For example:
//
//   formatv("{0} {1} {0}", "a", "bb")
//
// would yield the string "abba".  This can be convenient when it is expensive
// to compute the value of the parameter, and you would otherwise have had to
// save it to a temporary.
//
// ===Formatter Search===
//
// For a given parameter of type T, the following steps are executed in order
// until a match is found:
//
//   1. If the parameter is of class type, and inherits from format_adapter,
//      Then format() is invoked on it to produce the formatted output.  The
//      implementation should write the formatted text into `Stream`.
//   2. If there is a suitable template specialization of FormatProvider<>
//      for type T containing a method whose signature is:
//      void format(const T &Obj, RawOutStream &Stream, StringRef Options)
//      Then this method is invoked as described in Step 1.
//   3. If an appropriate operator<< for raw_ostream exists, it will be used.
//      For this to work, (RawOutStream& << const T&) must return RawOutStream&.
//
// If a match cannot be found through either of the above methods, a compiler
// error is generated.
//
// ===Invalid Format String Handling===
//
// In the case of a format string which does not match the grammar described
// above, the output is undefined.  With asserts enabled, LLVM will trigger an
// assertion.  Otherwise, it will try to do something reasonable, but in general
// the details of what that is are undefined.
//
template <typename... Ts>
inline auto formatv(const char *fmt, Ts &&... values) -> FormatvObject<decltype(
      std::make_tuple(internal::build_format_adapter(std::forward<Ts>(values))...))>
{
   using ParamTuple = decltype(
   std::make_tuple(internal::build_format_adapter(std::forward<Ts>(values))...));
   return FormatvObject<ParamTuple>(
            fmt,
            std::make_tuple(internal::build_format_adapter(std::forward<Ts>(values))...));
}

} // utils
} // polar

#endif // POLARPHP_UTILS_FORMAT_VARIADIC_H
