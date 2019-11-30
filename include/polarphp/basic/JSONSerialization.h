//===--- JSONSerialization.h - JSON serialization support -------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polar.org/LICENSE.txt for license information
// See https://polar.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/11/27.

#ifndef POLARPHP_BASIC_JSON_SERIALIZATION_H
#define POLARPHP_BASIC_JSON_SERIALIZATION_H

#include "polarphp/basic/LLVM.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/Regex.h"
#include "llvm/Support/raw_ostream.h"

#include <map>
#include <vector>

namespace polar::json {

/// This class should be specialized by any type that needs to be converted
/// to/from a JSON object.  For example:
///
///     struct ObjectTraits<MyStruct> {
///       static void mapping(Output &out, MyStruct &s) {
///         out.mapRequired("name", s.name);
///         out.mapRequired("size", s.size);
///         out.mapOptional("age",  s.age);
///       }
///     };
template<class T>
struct ObjectTraits
{
   // Must provide:
   // static void mapping(Output &out, T &fields);
   // Optionally may provide:
   // static StringRef validate(Output &out, T &fields);
};


/// This class should be specialized by any integral type that converts
/// to/from a JSON scalar where there is a one-to-one mapping between
/// in-memory values and a string in JSON.  For example:
///
///     struct ScalarEnumerationTraits<Colors> {
///         static void enumeration(Output &out, Colors &value) {
///           out.enumCase(value, "red",   cRed);
///           out.enumCase(value, "blue",  cBlue);
///           out.enumCase(value, "green", cGreen);
///         }
///       };
template<typename T>
struct ScalarEnumerationTraits
{
   // Must provide:
   // static void enumeration(Output &out, T &value);
};


/// This class should be specialized by any integer type that is a union
/// of bit values and the JSON representation is an array of
/// strings.  For example:
///
///      struct ScalarBitSetTraits<MyFlags> {
///        static void bitset(Output &out, MyFlags &value) {
///          out.bitSetCase(value, "big",   flagBig);
///          out.bitSetCase(value, "flat",  flagFlat);
///          out.bitSetCase(value, "round", flagRound);
///        }
///      };
template<typename T>
struct ScalarBitSetTraits
{
   // Must provide:
   // static void bitset(Output &out, T &value);
};


/// This class should be specialized by type that requires custom conversion
/// to/from a json scalar.  For example:
///
///    template<>
///    struct ScalarTraits<MyType> {
///      static void output(const MyType &val, llvm::raw_ostream &out) {
///        // stream out custom formatting
///        out << llvm::format("%x", val);
///      }
///      static bool mustQuote(StringRef) { return true; }
///    };
template<typename T>
struct ScalarTraits
{
   // Must provide:
   //
   // Function to write the value as a string:
   //static void output(const T &value, void *ctxt, llvm::raw_ostream &out);
   //
   // Function to determine if the value should be quoted.
   //static bool mustQuote(StringRef);
};

/// This is an optimized form of ScalarTraits in case the scalar value is
/// already present in a memory buffer.  For example:
///
///    template<>
///    struct ScalarReferenceTraits<MyType> {
///      static StringRef stringRef(const MyType &val) {
///        // Retrieve scalar value from memory
///        return value.stringValue;
///      }
///      static bool mustQuote(StringRef) { return true; }
///    };
template<typename T>
struct ScalarReferenceTraits
{
   // Must provide:
   //
   // Function to return a string representation of the value.
   // static StringRef stringRef(const T &value);
   //
   // Function to determine if the value should be quoted.
   // static bool mustQuote(StringRef);
};

/// This class should be specialized by any type that can be 'null' in JSON.
/// For example:
///
///    template<>
///    struct NullableTraits<MyType *> > {
///      static bool isNull(MyType *&ptr) {
///        return !ptr;
///      }
///      static MyType &get(MyType *&ptr) {
///        return *ptr;
///      }
///    };
template<typename T>
struct NullableTraits
{
   // Must provide:
   //
   // Function to return true if the value is 'null'.
   // static bool isNull(const T &value);
   //
   // Function to return a reference to the unwrapped value.
   // static T::value_type &get(const T &value);
};


/// This class should be specialized by any type that needs to be converted
/// to/from a JSON array.  For example:
///
///    template<>
///    struct ArrayTraits< std::vector<MyType> > {
///      static size_t size(Output &out, std::vector<MyType> &seq) {
///        return seq.size();
///      }
///      static MyType& element(Output &, std::vector<MyType> &seq, size_t index) {
///        if (index >= seq.size())
///          seq.resize(index+1);
///        return seq[index];
///      }
///    };
template<typename T>
struct ArrayTraits
{
   // Must provide:
   // static size_t size(Output &out, T &seq);
   // static T::value_type& element(Output &out, T &seq, size_t index);
};

// Only used by compiler if both template types are the same
template <typename T, T>
struct SameType;

// Only used for better diagnostics of missing traits
template <typename T>
struct MissingTrait;

// Test if ScalarEnumerationTraits<T> is defined on type T.
template <class T>
struct HasScalarEnumerationTraits
{
   using SignatureEnumeration = void (*)(class Output &, T &);

   template <typename U>
   static char test(SameType<SignatureEnumeration, &U::enumeration>*);

   template <typename U>
   static double test(...);

public:
   static bool const value =
         (sizeof(test<ScalarEnumerationTraits<T> >(nullptr)) == 1);
};


// Test if ScalarBitSetTraits<T> is defined on type T.
template <class T>
struct HasScalarBitSetTraits
{
   using SignatureBitset = void (*)(class Output &, T &);

   template <typename U>
   static char test(SameType<SignatureBitset, &U::bitset>*);

   template <typename U>
   static double test(...);

public:
   static bool const value = (sizeof(test<ScalarBitSetTraits<T> >(nullptr)) == 1);
};


// Test if ScalarTraits<T> is defined on type T.
template <class T>
struct HasScalarTraits
{
   using SignatureOutput = void (*)(const T &, llvm::raw_ostream &);
   using SignatureMustQuote = bool (*)(StringRef);

   template <typename U>
   static char test(SameType<SignatureOutput, &U::output> *,
                    SameType<SignatureMustQuote, &U::mustQuote> *);

   template <typename U>
   static double test(...);

public:
   static bool const value =
         (sizeof(test<ScalarTraits<T>>(nullptr, nullptr)) == 1);
};

// Test if ScalarReferenceTraits<T> is defined on type T.
template <class T>
struct HasScalarReferenceTraits
{
   using SignatureStringRef = StringRef (*)(const T &);
   using SignatureMustQuote = bool (*)(StringRef);

   template <typename U>
   static char test(SameType<SignatureStringRef, &U::stringRef> *,
                    SameType<SignatureMustQuote, &U::mustQuote> *);

   template <typename U>
   static double test(...);

public:
   static bool const value =
         (sizeof(test<ScalarReferenceTraits<T>>(nullptr, nullptr)) == 1);
};

// Test if ObjectTraits<T> is defined on type T.
template <class T>
struct HasObjectTraits
{
   using SignatureMapping = void (*)(class Output &, T &);

   template <typename U>
   static char test(SameType<SignatureMapping, &U::mapping>*);

   template <typename U>
   static double test(...);

public:
   static bool const value = (sizeof(test<ObjectTraits<T> >(nullptr)) == 1);
};

// Test if ObjectTraits<T>::validate() is defined on type T.
template <class T>
struct HasObjectValidateTraits
{
   using SignatureValidate = StringRef (*)(class Output &, T &);

   template <typename U>
   static char test(SameType<SignatureValidate, &U::validate>*);

   template <typename U>
   static double test(...);

public:
   static bool const value = (sizeof(test<ObjectTraits<T> >(nullptr)) == 1);
};



// Test if ArrayTraits<T> is defined on type T.
template <class T>
struct HasArrayMethodTraits
{
   using SignatureSize = size_t (*)(class Output &, T &);

   template <typename U>
   static char test(SameType<SignatureSize, &U::size>*);

   template <typename U>
   static double test(...);

public:
   static bool const value =  (sizeof(test<ArrayTraits<T> >(nullptr)) == 1);
};

// Test if ArrayTraits<T> is defined on type T
template<typename T>
struct HasArrayTraits : public std::integral_constant<bool,
      HasArrayMethodTraits<T>::value > { };

// Test if NullableTraits<T> is defined on type T.
template <class T>
struct HasNullableTraits
{
   using SignatureIsNull = bool (*)(T &);

   template <typename U>
   static char test(SameType<SignatureIsNull, &U::isNull> *);

   template <typename U>
   static double test(...);

public:
   static bool const value =
         (sizeof(test<NullableTraits<T>>(nullptr)) == 1);
};

inline bool isNumber(StringRef str)
{
   static const char DecChars[] = "0123456789";
   if (str.find_first_not_of(DecChars) == StringRef::npos)
      return true;

   llvm::Regex FloatMatcher(
            "^(\\.[0-9]+|[0-9]+(\\.[0-9]*)?)([eE][-+]?[0-9]+)?$");
   if (FloatMatcher.match(str))
      return true;

   return false;
}

inline bool isNumeric(StringRef str)
{
   if ((str.front() == '-' || str.front() == '+') && isNumber(str.drop_front()))
      return true;

   if (isNumber(str))
      return true;

   return false;
}

inline bool isNull(StringRef str)
{
   return str.equals("null");
}

inline bool isBool(StringRef str)
{
   return str.equals("true") || str.equals("false");
}

template<typename T>
struct MissingTraits : public std::integral_constant<bool,
      !HasScalarEnumerationTraits<T>::value
      && !HasScalarBitSetTraits<T>::value
      && !HasScalarTraits<T>::value
      && !HasScalarReferenceTraits<T>::value
      && !HasNullableTraits<T>::value
      && !HasObjectTraits<T>::value
      && !HasArrayTraits<T>::value> {};

template<typename T>
struct ValidatedObjectTraits : public std::integral_constant<bool,
      HasObjectTraits<T>::value
      && HasObjectValidateTraits<T>::value> {};

template<typename T>
struct UnvalidatedObjectTraits : public std::integral_constant<bool,
      HasObjectTraits<T>::value
      && !HasObjectValidateTraits<T>::value> {};

class Output
{
public:
   using UserInfoMap = std::map<void *, void *>;

private:
   enum State {
      ArrayFirstValue,
      ArrayOtherValue,
      ObjectFirstKey,
      ObjectOtherKey
   };

   llvm::raw_ostream &m_stream;
   SmallVector<State, 8> m_stateStack;
   bool m_prettyPrint;
   bool m_needBitValueComma;
   bool m_enumerationMatchFound;
   UserInfoMap m_userInfo;

public:
   Output(llvm::raw_ostream &os, UserInfoMap userInfo = {},
          bool prettyPrint = true)
      : m_stream(os),
        m_prettyPrint(prettyPrint),
        m_needBitValueComma(false),
        m_enumerationMatchFound(false),
        m_userInfo(userInfo)
   {}

   virtual ~Output() = default;

   UserInfoMap &getUserInfo()
   {
      return m_userInfo;
   }

   unsigned beginArray();
   bool preflightElement(unsigned, void *&);
   void postflightElement(void*);
   void endArray();
   bool canElideEmptyArray();

   void beginObject();
   void endObject();
   bool preflightKey(StringRef, bool, bool, bool &, void *&);
   void postflightKey(void*);

   void beginEnumScalar();
   bool matchEnumScalar(const char*, bool);
   void endEnumScalar();

   bool beginBitSetScalar(bool &);
   bool bitSetMatch(const char*, bool);
   void endBitSetScalar();

   void scalarString(StringRef &, bool);
   void null();

   template <typename T>
   void enumCase(T &value, const char* Str, const T ConstVal)
   {
      if (matchEnumScalar(Str, value == ConstVal)) {
         value = ConstVal;
      }
   }

   template <typename T>
   void bitSetCase(T &value, const char* Str, const T ConstVal)
   {
      if (bitSetMatch(Str, (value & ConstVal) == ConstVal)) {
         value = value | ConstVal;
      }
   }

   template <typename T>
   void maskedBitSetCase(T &value, const char *Str, T ConstVal, T Mask)
   {
      if (bitSetMatch(Str, (value & Mask) == ConstVal))
         value = value | ConstVal;
   }

   template <typename T>
   void maskedBitSetCase(T &value, const char *Str, uint32_t ConstVal,
                         uint32_t Mask)
   {
      if (bitSetMatch(Str, (value & Mask) == ConstVal))
         value = value | ConstVal;
   }

   template <typename T>
   void mapRequired(StringRef key, T& value)
   {
      this->processKey(key, value, true);
   }

   template <typename T>
   typename std::enable_if<HasArrayTraits<T>::value,void>::type
   mapOptional(StringRef key, T& value)
   {
      // omit key/value instead of outputting empty array
      if (this->canElideEmptyArray() && !(value.begin() != value.end()))
         return;
      this->processKey(key, value, false);
   }

   template <typename T>
   void mapOptional(StringRef key, Optional<T> &value)
   {
      processKeyWithDefault(key, value, Optional<T>(), /*required=*/false);
   }

   template <typename T>
   typename std::enable_if<!HasArrayTraits<T>::value,void>::type
   mapOptional(StringRef key, T& value)
   {
      this->processKey(key, value, false);
   }

   template <typename T>
   void mapOptional(StringRef key, T& value, const T& defaultValue)
   {
      this->processKeyWithDefault(key, value, defaultValue, false);
   }

private:
   template <typename T>
   void processKeyWithDefault(StringRef key, Optional<T> &value,
                              const Optional<T> &defaultValue, bool required)
   {
      assert(!defaultValue.hasValue() &&
             "Optional<T> shouldn't have a value!");
      void *SaveInfo;
      bool UseDefault;
      const bool sameAsDefault = !value.hasValue();
      if (!value.hasValue())
         value = T();
      if (this->preflightKey(key, required, sameAsDefault, UseDefault,
                             SaveInfo)) {
         jsonize(*this, value.getValue(), required);
         this->postflightKey(SaveInfo);
      } else {
         if (UseDefault)
            value = defaultValue;
      }
   }

   template <typename T>
   void processKeyWithDefault(StringRef key, T &value, const T &defaultValue,
                              bool required)
   {
      void *SaveInfo;
      bool UseDefault;
      const bool sameAsDefault = value == defaultValue;
      if (this->preflightKey(key, required, sameAsDefault, UseDefault,
                             SaveInfo)) {
         jsonize(*this, value, required);
         this->postflightKey(SaveInfo);
      } else {
         if (UseDefault)
            value = defaultValue;
      }
   }

   template <typename T>
   void processKey(StringRef key, T &value, bool required)
   {
      void *SaveInfo;
      bool UseDefault;
      if (this->preflightKey(key, required, false, UseDefault, SaveInfo)) {
         jsonize(*this, value, required);
         this->postflightKey(SaveInfo);
      }
   }

private:
   void indent();
};

template <typename T> struct ArrayTraits<std::vector<T>>
{
   static size_t size(Output &out, std::vector<T> &seq) { return seq.size(); }

   static T &element(Output &out, std::vector<T> &seq, size_t index) {
      if (index >= seq.size())
         seq.resize(index + 1);
      return seq[index];
   }
};

template<>
struct ScalarReferenceTraits<bool>
{
   static StringRef stringRef(const bool &);
   static bool mustQuote(StringRef) { return false; }
};

template<>
struct ScalarReferenceTraits<StringRef>
{
   static StringRef stringRef(const StringRef &);
   static bool mustQuote(StringRef str) { return true; }
};

template<>
struct ScalarReferenceTraits<std::string>
{
   static StringRef stringRef(const std::string &);
   static bool mustQuote(StringRef str) { return true; }
};

template<>
struct ScalarTraits<uint8_t>
{
   static void output(const uint8_t &, llvm::raw_ostream &);
   static bool mustQuote(StringRef) { return false; }
};

template<>
struct ScalarTraits<uint16_t>
{
   static void output(const uint16_t &, llvm::raw_ostream &);
   static bool mustQuote(StringRef) { return false; }
};

template<>
struct ScalarTraits<uint32_t>
{
   static void output(const uint32_t &, llvm::raw_ostream &);
   static bool mustQuote(StringRef) { return false; }
};

#if defined(_MSC_VER)
// In MSVC, 'unsigned long' is 32bit size and different from uint32_t,
// and it is used to define polar::sys::ProcessId.
template<>
struct ScalarTraits<unsigned long> {
   static void output(const unsigned long &, llvm::raw_ostream &);
   static bool mustQuote(StringRef) { return false; }
};
#endif

template<>
struct ScalarTraits<uint64_t>
{
   static void output(const uint64_t &, llvm::raw_ostream &);
   static bool mustQuote(StringRef) { return false; }
};

template<>
struct ScalarTraits<int8_t>
{
   static void output(const int8_t &, llvm::raw_ostream &);
   static bool mustQuote(StringRef) { return false; }
};

template<>
struct ScalarTraits<int16_t>
{
   static void output(const int16_t &, llvm::raw_ostream &);
   static bool mustQuote(StringRef) { return false; }
};

template<>
struct ScalarTraits<int32_t>
{
   static void output(const int32_t &, llvm::raw_ostream &);
   static bool mustQuote(StringRef) { return false; }
};

template<>
struct ScalarTraits<int64_t>
{
   static void output(const int64_t &, llvm::raw_ostream &);
   static bool mustQuote(StringRef) { return false; }
};

template<>
struct ScalarTraits<float>
{
   static void output(const float &, llvm::raw_ostream &);
   static bool mustQuote(StringRef) { return false; }
};

template<>
struct ScalarTraits<double>
{
   static void output(const double &, llvm::raw_ostream &);
   static bool mustQuote(StringRef) { return false; }
};

template<typename T>
typename std::enable_if<HasScalarEnumerationTraits<T>::value,void>::type
jsonize(Output &out, T &value, bool)
{
   out.beginEnumScalar();
   ScalarEnumerationTraits<T>::enumeration(out, value);
   out.endEnumScalar();
}

template<typename T>
typename std::enable_if<HasScalarBitSetTraits<T>::value,void>::type
jsonize(Output &out, T &value, bool)
{
   bool DoClear;
   if (out.beginBitSetScalar(DoClear)) {
      if (DoClear)
         value = static_cast<T>(0);
      ScalarBitSetTraits<T>::bitset(out, value);
      out.endBitSetScalar();
   }
}

template<typename T>
typename std::enable_if<HasScalarTraits<T>::value,void>::type
jsonize(Output &out, T &value, bool)
{
   {
      SmallString<64> Storage;
      llvm::raw_svector_ostream Buffer(Storage);
      Buffer.SetUnbuffered();
      ScalarTraits<T>::output(value, Buffer);
      StringRef Str = Buffer.str();
      out.scalarString(Str, ScalarTraits<T>::mustQuote(Str));
   }
}

template <typename T>
typename std::enable_if<HasScalarReferenceTraits<T>::value, void>::type
jsonize(Output &out, T &value, bool)
{
   StringRef Str = ScalarReferenceTraits<T>::stringRef(value);
   out.scalarString(Str, ScalarReferenceTraits<T>::mustQuote(Str));
}

template<typename T>
typename std::enable_if<HasNullableTraits<T>::value,void>::type
jsonize(Output &out, T &Obj, bool)
{
   if (NullableTraits<T>::isNull(Obj))
      out.null();
   else
      jsonize(out, NullableTraits<T>::get(Obj), true);
}


template<typename T>
typename std::enable_if<ValidatedObjectTraits<T>::value, void>::type
jsonize(Output &out, T &value, bool)
{
   out.beginObject();
   {
      StringRef Err = ObjectTraits<T>::validate(out, value);
      if (!Err.empty()) {
         llvm::errs() << Err << "\n";
         assert(Err.empty() && "invalid struct trying to be written as json");
      }
   }
   ObjectTraits<T>::mapping(out, value);
   out.endObject();
}

template<typename T>
typename std::enable_if<UnvalidatedObjectTraits<T>::value, void>::type
jsonize(Output &out, T &value, bool)
{
   out.beginObject();
   ObjectTraits<T>::mapping(out, value);
   out.endObject();
}

template<typename T>
typename std::enable_if<MissingTrait<T>::value, void>::type
jsonize(Output &out, T &value, bool)
{
   char missing_json_trait_for_type[sizeof(MissingTrait<T>)];
}

template<typename T>
typename std::enable_if<HasArrayTraits<T>::value,void>::type
jsonize(Output &out, T &Seq, bool)
{
   {
      out.beginArray();
      unsigned count = ArrayTraits<T>::size(out, Seq);
      for (unsigned i=0; i < count; ++i) {
         void *SaveInfo;
         if (out.preflightElement(i, SaveInfo)) {
            jsonize(out, ArrayTraits<T>::element(out, Seq, i), true);
            out.postflightElement(SaveInfo);
         }
      }
      out.endArray();
   }
}

// Define non-member operator<< so that Output can stream out a map.
template <typename T>
inline
typename
std::enable_if<polar::json::HasObjectTraits<T>::value, Output &>::type
operator<<(Output &yout, T &map)
{
   jsonize(yout, map, true);
   return yout;
}

// Define non-member operator<< so that Output can stream out an array.
template <typename T>
inline
typename
std::enable_if<polar::json::HasArrayTraits<T>::value, Output &>::type
operator<<(Output &yout, T &seq)
{
   jsonize(yout, seq, true);
   return yout;
}

} // polar::json

#endif // POLARPHP_BASIC_JSON_SERIALIZATION_H
