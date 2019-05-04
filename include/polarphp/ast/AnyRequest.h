//===--- AnyRequest.h - Requests Instances ----------------------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//===----------------------------------------------------------------------===//
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/05/05.
//===----------------------------------------------------------------------===//
//
//  This file defines the AnyRequest class, which describes a m_stored m_request.
//
//===----------------------------------------------------------------------===//

#ifndef POLAR_AST_ANYREQUEST_H
#define POLAR_AST_ANYREQUEST_H

#include "polarphp/basic/TypeId.h"
#include "polarphp/basic/adt/DenseMapInfo.h"
#include "polarphp/basic/adt/Hashing.h"
#include "polarphp/basic/adt/IntrusiveRefCountPtr.h"
#include <string>

namespace polar::utils {
class RawOutStream;
} // polar::utils

namespace polar::ast {

using polar::basic::HashCode;
using polar::basic::hash_value;

using polar::basic::DenseMapInfo;
using polar::basic::RefCountedBase;
using polar::basic::IntrusiveRefCountPtr;
using polar::basic::TypeId;
using polar::utils::RawOutStream;

class DiagnosticEngine;

/// Stores a m_request (for the \c Evaluator class) of any kind.
///
/// Requests must be value types and provide the following API to be m_stored in
/// an \c AnyRequest instance:
///
///   - Copy constructor
///   - Equality operator (==)
///   - Hashing support (hash_value)
///   - TypeId support (see swift/Basic/TypeId.h)
///   - Display support (free function):
///       void simple_display(RawOutStream &, const T &);
///   - Cycle diagnostics operations:
///       void diagnoseCycle(DiagnosticEngine &diags) const;
///       void noteCycleStep(DiagnosticEngine &diags) const;
///
class AnyRequest
{
   friend class DenseMapInfo<AnyRequest>;

   static HashCode hashForHolder(uint64_t typeId, HashCode requestHash)
   {
      return hash_combine(hash_value(typeId), requestHash);
   }

   /// Abstract base class used to hold the specific m_request kind.
   class HolderBase : public RefCountedBase<HolderBase>
   {
   public:
      /// The type ID of the m_request being m_stored.
      const uint64_t typeId;

      /// Hash value for the m_request itself.
      const HashCode hash;

   protected:
      /// Initialize base with type ID and hash code.
      HolderBase(uint64_t typeId, HashCode hash)
         : typeId(typeId),
           hash(AnyRequest::hashForHolder(typeId, hash))
      {}

   public:
      virtual ~HolderBase();

      /// Determine whether this m_request is equivalent to the \c other
      /// m_request.
      virtual bool equals(const HolderBase &other) const = 0;

      /// Display.
      virtual void display(RawOutStream &out) const = 0;

      /// Diagnose a cycle detected for this m_request.
      virtual void diagnoseCycle(DiagnosticEngine &diags) const = 0;

      /// Note that this m_request is part of a cycle.
      virtual void noteCycleStep(DiagnosticEngine &diags) const = 0;
   };

   /// Holds a value that can be used as a m_request input/output.
   template<typename Request>
   class Holder final : public HolderBase
   {
   public:
      const Request m_request;

      Holder(const Request &request)
         : HolderBase(TypeId<Request>::value, hash_value(request)),
           m_request(request)
      {}

      Holder(Request &&request)
         : HolderBase(TypeId<Request>::value, hash_value(request)),
           m_request(std::move(request))
      {}

      virtual ~Holder() { }

      /// Determine whether this m_request is equivalent to another.
      ///
      /// The caller guarantees that the typeIDs are the same.
      virtual bool equals(const HolderBase &other) const override
      {
         assert(typeId == other.typeId && "Caller should match typeIDs");
         return m_request == static_cast<const Holder<Request> &>(other).m_request;
      }

      /// Display.
      virtual void display(RawOutStream &out) const override
      {
         simple_display(out, m_request);
      }

      /// Diagnose a cycle detected for this m_request.
      virtual void diagnoseCycle(DiagnosticEngine &diags) const override
      {
         m_request.diagnoseCycle(diags);
      }

      /// Note that this m_request is part of a cycle.
      virtual void noteCycleStep(DiagnosticEngine &diags) const override
      {
         m_request.noteCycleStep(diags);
      }
   };

   /// FIXME: Inefficient. Use the low bits.
   enum class StorageKind
   {
      Normal,
      Empty,
      Tombstone,
   } m_storageKind = StorageKind::Normal;

   /// The data m_stored in this value.
   IntrusiveRefCountPtr<HolderBase> m_stored;

   AnyRequest(StorageKind storageKind)
      : m_storageKind(storageKind)
   {
      assert(m_storageKind != StorageKind::Normal);
   }

public:
   AnyRequest(const AnyRequest &other) = default;
   AnyRequest &operator=(const AnyRequest &other) = default;

   AnyRequest(AnyRequest &&other)
      : m_storageKind(other.m_storageKind),
        m_stored(std::move(other.m_stored))
   {
      other.m_storageKind = StorageKind::Empty;
   }

   AnyRequest &operator=(AnyRequest &&other)
   {
      m_storageKind = other.m_storageKind;
      m_stored = std::move(other.m_stored);
      other.m_storageKind = StorageKind::Empty;
      other.m_stored = nullptr;
      return *this;
   }

   // Create a local template typename `ValueType` in the template specialization
   // so that we can refer to it in the SFINAE condition as well as the body of
   // the template itself.  The SFINAE condition allows us to remove this
   // constructor from candidacy when evaluating explicit construction with an
   // instance of `AnyRequest`.  If we do not do so, we will find ourselves with
   // ambiguity with this constructor and the defined move constructor above.
   /// Construct a new instance with the given value.
   template <typename T,
             typename ValueType = typename std::remove_cv<
                typename std::remove_reference<T>::type>::type,
             typename = typename std::enable_if<
                !std::is_same<ValueType, AnyRequest>::value>::type>
   explicit AnyRequest(T &&value) : m_storageKind(StorageKind::Normal)
   {
      m_stored = IntrusiveRefCountPtr<HolderBase>(
               new Holder<ValueType>(std::forward<T>(value)));
   }

   /// Cast to a specific (known) type.
   template<typename Request>
   const Request &castTo() const
   {
      assert(m_stored->typeId == TypeId<Request>::value && "wrong type in cast");
      return static_cast<const Holder<Request> *>(m_stored.get())->m_request;
   }

   /// Try casting to a specific (known) type, returning \c nullptr on
   /// failure.
   template<typename Request>
   const Request *getAs() const
   {
      if (m_stored->typeId != TypeId<Request>::value) {
         return nullptr;
      }
      return &static_cast<const Holder<Request> *>(m_stored.get())->m_request;
   }

   /// Diagnose a cycle detected for this m_request.
   void diagnoseCycle(DiagnosticEngine &diags) const
   {
      m_stored->diagnoseCycle(diags);
   }

   /// Note that this m_request is part of a cycle.
   void noteCycleStep(DiagnosticEngine &diags) const
   {
      m_stored->noteCycleStep(diags);
   }

   /// Compare two instances for equality.
   friend bool operator==(const AnyRequest &lhs, const AnyRequest &rhs)
   {
      if (lhs.m_storageKind != rhs.m_storageKind) {
         return false;
      }

      if (lhs.m_storageKind != StorageKind::Normal) {
         return true;
      }


      if (lhs.m_stored->typeId != rhs.m_stored->typeId) {
         return false;
      }
      return lhs.m_stored->equals(*rhs.m_stored);
   }

   friend bool operator!=(const AnyRequest &lhs, const AnyRequest &rhs)
   {
      return !(lhs == rhs);
   }

   friend HashCode hash_value(const AnyRequest &any)
   {
      if (any.m_storageKind != StorageKind::Normal) {
         return 1;
      }
      return any.m_stored->hash;
   }

   friend void simple_display(RawOutStream &out, const AnyRequest &any)
   {
      any.m_stored->display(out);
   }

   /// Return the result of calling simple_display as a string.
   std::string getAsString() const;

   static AnyRequest getEmptyKey()
   {
      return AnyRequest(StorageKind::Empty);
   }

   static AnyRequest getTombstoneKey()
   {
      return AnyRequest(StorageKind::Tombstone);
   }
};

} // polar::ast

namespace polar::basic {

using polar::ast::AnyRequest;

template<>
struct DenseMapInfo<AnyRequest>
{
   static inline AnyRequest getEmptyKey()
   {
      return AnyRequest::getEmptyKey();
   }

   static inline AnyRequest getTombstoneKey()
   {
      return AnyRequest::getTombstoneKey();
   }

   static unsigned getHashValue(const AnyRequest &request)
   {
      return hash_value(request);
   }

   template <typename Request>
   static unsigned getHashValue(const Request &request)
   {
      return AnyRequest::hashForHolder(TypeId<Request>::value,
                                              hash_value(request));
   }

   static bool isEqual(const AnyRequest &lhs,
                       const AnyRequest &rhs)
   {
      return lhs == rhs;
   }

   template <typename Request>
   static bool isEqual(const Request &lhs,
                       const AnyRequest &rhs)
   {
      if (rhs == getEmptyKey() || rhs == getTombstoneKey()) {
         return false;
      }
      const Request *rhsRequest = rhs.getAs<Request>();
      if (!rhsRequest) {
         return false;
      }
      return lhs == *rhsRequest;
   }
};

} // polar::basic

#endif // POLAR_AST_ANYREQUEST_H
