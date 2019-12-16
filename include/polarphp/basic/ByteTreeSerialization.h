//===--- ByteTreeSerialization.h - ByteTree serialization -------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2018 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
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
// Created by polarboy on 2019/05/08.
//
/// \file
/// Provides an interface for serializing an object tree to a custom
/// binary format called ByteTree.
///
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_BASIC_BYTETREESERIALIZATION_H
#define POLARPHP_BASIC_BYTETREESERIALIZATION_H

#include "llvm/Support/BinaryStreamError.h"
#include "llvm/Support/BinaryStreamWriter.h"
#include "polarphp/basic/ExponentialGrowthAppendingBinaryByteStream.h"
#include <map>

namespace polar::bytetree {

using llvm::BinaryStreamWriter;
using llvm::StringRef;

namespace {
// Only used by compiler if both template types are the same
template <typename T, T>
struct SameType;
} // anonymous namespace

class ByteTreeWriter;

using UserInfoMap = std::map<void *, void *>;

/// Add a template specialization of \c ObjectTraits for any type that
/// serializes as an object consisting of multiple fields.
template <class T>
struct ObjectTraits
{
   // Must provide:

   /// Return the number of fields that will be written in \c write when
   /// \p object gets serialized. \p m_userInfo can contain arbitrary values that
   /// can modify the serialization behaviour and gets passed down from the
   /// serialization invocation.
   // static unsigned getNumFields(const T &object, UserInfoMap &m_userInfo);

   /// Serialize \p object by calling \c writer.write for all the fields of
   /// \p object. \p m_userInfo can contain arbitrary values that can modify the
   /// serialization behaviour and gets passed down from the serialization
   /// invocation.
   // static void write(ByteTreeWriter &writer, const T &object,
   //                   UserInfoMap &m_userInfo);
};

/// Add a template specialization of \c ScalarTraits for any type that
/// serializes into a raw set of bytes.
template <class T>
struct ScalarTraits
{
   // Must provide:

   /// Return the number of bytes the serialized format of \p value will take up.
   // static unsigned size(const T &value);

   /// Serialize \p value by writing its binary format into \p writer. Any errors
   /// that may be returned by \p writer can be returned by this function and
   /// will be handled on the call-side.
   // static llvm::Error write(BinaryStreamWriter &writer, const T &value);
};

/// Add a template specialization of \c DirectlyEncodable for any type whose
/// serialized form is equal to its binary representation on the serializing
/// machine.
template <class T>
struct DirectlyEncodable
{
   // Must provide:

   // static bool const value = true;
};

/// Add a template specialization of \c WrapperTypeTraits for any type that
/// serializes as a type that already has a specialization of \c ScalarTypes.
/// This will typically be useful for types like enums that have a 1-to-1
/// mapping to e.g. an integer.
template <class T>
struct WrapperTypeTraits
{
   // Must provide:

   /// Write the serializable representation of \p value to \p writer. This will
   /// typically take the form \c writer.write(convertedValue(value), index)
   /// where \c convertedValue has to be defined.
   // static void write(ByteTreeWriter &writer, const T &value, unsigned index);
};

// Test if ObjectTraits<T> is defined on type T.
template <class T>
struct HasObjectTraits
{
   using Signature_numFields = unsigned (*)(const T &, UserInfoMap &m_userInfo);
   using SignatureWrite = void (*)(ByteTreeWriter &writer, const T &object,
   UserInfoMap &m_userInfo);

   template <typename U>
   static char test(SameType<Signature_numFields, &U::getNumFields> *,
                    SameType<SignatureWrite, &U::write> *);

   template <typename U>
   static double test(...);

public:
   static bool const value =
         (sizeof(test<ObjectTraits<T>>(nullptr, nullptr)) == 1);
};

// Test if ScalarTraits<T> is defined on type T.
template <class T>
struct has_ScalarTraits
{
   using SignatureSize = unsigned (*)(const T &object);
   using SignatureWrite = llvm::Error (*)(BinaryStreamWriter &writer,
   const T &object);

   template <typename U>
   static char test(SameType<SignatureSize, &U::size> *,
                    SameType<SignatureWrite, &U::write> *);

   template <typename U>
   static double test(...);

public:
   static bool const value =
         (sizeof(test<ScalarTraits<T>>(nullptr, nullptr)) == 1);
};

// Test if WrapperTypeTraits<T> is defined on type T.
template <class T>
struct HasWrapperTypeTraits
{
   using SignatureWrite = void (*)(ByteTreeWriter &writer, const T &object,
   unsigned index);

   template <typename U>
   static char test(SameType<SignatureWrite, &U::write> *);

   template <typename U>
   static double test(...);

public:
   static bool const value = (sizeof(test<WrapperTypeTraits<T>>(nullptr)) == 1);
};

class ByteTreeWriter
{
private:
   /// The writer to which the binary data is written.
   BinaryStreamWriter &m_streamWriter;

   /// The underlying m_stream of the m_streamWriter. We need this reference so that
   /// we can call \c ExponentialGrowthAppendingBinaryByteStream.writeRaw
   /// which is more efficient than the generic \c writeBytes of
   /// \c BinaryStreamWriter since it avoids the arbitrary size memcopy.
   ExponentialGrowthAppendingBinaryByteStream &m_stream;

   /// The number of fields this object contains. \c UINT_MAX if it has not been
   /// set yet. No member may be written to the object if expected number of
   /// fields has not been set yet.
   unsigned m_numFields = UINT_MAX;

   /// The index of the next field to write. Used in assertion builds to keep
   /// track that no indicies are jumped and that the object contains the
   /// expected number of fields.
   unsigned m_currentFieldIndex = 0;

   UserInfoMap &m_userInfo;

   /// The \c ByteTreeWriter can only be constructed internally. Use
   /// \c ByteTreeWriter.write to serialize a new object.
   /// \p m_stream must be the underlying m_stream of \p SteamWriter.
   ByteTreeWriter(ExponentialGrowthAppendingBinaryByteStream &stream,
                  BinaryStreamWriter &streamWriter, UserInfoMap &userInfo)
      : m_streamWriter(streamWriter),
        m_stream(stream),
        m_userInfo(userInfo)
   {}

   /// Write the given value to the ByteTree in the same form in which it is
   /// represented on the serializing machine.
   template <typename T>
   llvm::Error writeRaw(T value)
   {
      // FIXME: We implicitly inherit the endianess of the serializing machine.
      // Since we're currently only supporting macOS that's not a problem for now.
      auto error = m_stream.writeRaw(m_streamWriter.getOffset(), value);
      m_streamWriter.setOffset(m_streamWriter.getOffset() + sizeof(T));
      return error;
   }

   /// Set the expected number of fields the object written by this writer is
   /// expected to have.
   void setNumFields(uint32_t getNumFields) {
      assert(getNumFields != UINT_MAX &&
            "getNumFields may not be reset since it has already been written to "
            "the byte m_stream");
      assert((m_numFields == UINT_MAX) && "m_numFields has already been set");
      // Num fields cannot exceed (1 << 31) since it would otherwise interfere
      // with the bitflag that indicates if the next construct in the tree is an
      // object or a scalar.
      assert((getNumFields & ((uint32_t)1 << 31)) == 0 && "Field size too large");

      // Set the most significant bit to indicate that the next construct is an
      // object and not a scalar.
      uint32_t toWrite = getNumFields | (1 << 31);
      auto error = writeRaw(toWrite);
      (void)error;
      assert(!error);

      this->m_numFields = getNumFields;
   }

   /// Validate that \p index is the next field that is expected to be written,
   /// does not exceed the number of fields in this object and that
   /// \c setNumFields has already been called.
   void validateAndIncreaseFieldIndex(unsigned index)
   {
      assert((m_numFields != UINT_MAX) &&
             "setNumFields must be called before writing any value");
      assert(index == m_currentFieldIndex && "Writing index out of order");
      assert(index < m_numFields &&
             "Writing more fields than object is expected to have");

      m_currentFieldIndex++;
   }

   ~ByteTreeWriter()
   {
      assert(m_currentFieldIndex == m_numFields &&
             "object had more or less elements than specified");
   }

public:
   /// Write a binary serialization of \p object to \p m_streamWriter, prefixing
   /// the m_stream by the specified protocolVersion.
   template <typename T>
   typename std::enable_if<HasObjectTraits<T>::value, void>::type
   static write(ExponentialGrowthAppendingBinaryByteStream &stream,
                uint32_t protocolVersion, const T &object,
                UserInfoMap &userInfo)
   {
      BinaryStreamWriter streamWriter(stream);
      ByteTreeWriter writer(stream, streamWriter, userInfo);

      auto error = writer.writeRaw(protocolVersion);
      (void)error;
      assert(!error);

      // There always is one root. We need to set m_numFields so that index
      // validation succeeds, but we don't want to serialize this.
      writer.m_numFields = 1;
      writer.write(object, /*index=*/0);
   }

   template <typename T>
   typename std::enable_if<HasObjectTraits<T>::value, void>::type
   write(const T &object, unsigned index)
   {
      validateAndIncreaseFieldIndex(index);
      auto objectWriter = ByteTreeWriter(m_stream, m_streamWriter, m_userInfo);
      objectWriter.setNumFields(ObjectTraits<T>::getNumFields(object, m_userInfo));

      ObjectTraits<T>::write(objectWriter, object, m_userInfo);
   }

   template <typename T>
   typename std::enable_if<has_ScalarTraits<T>::value, void>::type
   write(const T &value, unsigned index)
   {
      validateAndIncreaseFieldIndex(index);
      uint32_t valueSize = ScalarTraits<T>::size(value);
      // Size cannot exceed (1 << 31) since it would otherwise interfere with the
      // bitflag that indicates if the next construct in the tree is an object
      // or a scalar.
      assert((valueSize & ((uint32_t)1 << 31)) == 0 && "value size too large");
      auto sizeError = writeRaw(valueSize);
      (void)sizeError;
      assert(!sizeError);

      auto startOffset = m_streamWriter.getOffset();
      auto contentError = ScalarTraits<T>::write(m_streamWriter, value);
      (void)contentError;
      assert(!contentError);
      (void)startOffset;
      assert((m_streamWriter.getOffset() - startOffset == valueSize) &&
             "Number of written bytes does not match size returned by "
             "ScalarTraits<T>::size");
   }

   template <typename T>
   typename std::enable_if<DirectlyEncodable<T>::value, void>::type
   write(const T &value, unsigned index)
   {
      validateAndIncreaseFieldIndex(index);
      uint32_t valueSize = sizeof(T);
      auto sizeError = writeRaw(valueSize);
      (void)sizeError;
      assert(!sizeError);
      auto contentError = writeRaw(value);
      (void)contentError;
      assert(!contentError);
   }

   template <typename T>
   typename std::enable_if<HasWrapperTypeTraits<T>::value, void>::type
   write(const T &value, unsigned index)
   {
      auto lengthBeforeWrite = m_currentFieldIndex;
      WrapperTypeTraits<T>::write(*this, value, index);
      (void)lengthBeforeWrite;
      assert(m_currentFieldIndex == lengthBeforeWrite + 1 &&
             "WrapperTypeTraits did not call BinaryWriter.write");
   }
};

// Define serialization schemes for common types

template <>
struct DirectlyEncodable<uint8_t>
{
   static bool const value = true;
};

template <>
struct DirectlyEncodable<uint16_t>
{
   static bool const value = true;
};

template <>
struct DirectlyEncodable<uint32_t>
{
   static bool const value = true;
};

template <>
struct WrapperTypeTraits<bool>
{
   static void write(ByteTreeWriter &writer, const bool &value,
                     unsigned index) {
      writer.write(static_cast<uint8_t>(value), index);
   }
};

template <>
struct ScalarTraits<StringRef>
{
   static unsigned size(const StringRef &str)
   {
      return str.size();
   }

   static llvm::Error write(BinaryStreamWriter &writer,
                            const StringRef &str)
   {
      return writer.writeFixedString(str);
   }
};

template <>
struct ObjectTraits<std::nullopt_t>
{
   // Serialize llvm::None as an object without any elements
   static unsigned getNumFields(const std::nullopt_t &object,
                                UserInfoMap &userInfo)
   {
      return 0;
   }

   static void write(ByteTreeWriter &writer, const std::nullopt_t &object,
                     UserInfoMap &userInfo)
   {
      // Nothing to write
   }
};

} // polar::bytetree

#endif // POLARPHP_BASIC_BYTETREESERIALIZATION_H
