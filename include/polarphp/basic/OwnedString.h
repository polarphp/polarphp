// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/01/15.

#ifndef POLARPHP_BASIC_ADT_OWNED_STRING_H
#define POLARPHP_BASIC_ADT_OWNED_STRING_H

#include "llvm/ADT/IntrusiveRefCntPtr.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/TrailingObjects.h"

namespace polar {
namespace basic {

using llvm::StringRef;
using llvm::TrailingObjects;
using llvm::ThreadSafeRefCountedBase;
using llvm::IntrusiveRefCntPtr;

/// Holds a string - either statically allocated or dynamically allocated
/// and owned by this type.
class OwnedString
{
   /// An owner that keeps the buffer of a ref counted \c OwnedString alive.
   class TextOwner final : public ThreadSafeRefCountedBase<TextOwner>,
         public TrailingObjects<TextOwner, char>
   {
      TextOwner(StringRef text)
      {
         std::uninitialized_copy(text.begin(), text.end(),
                                 getTrailingObjects<char>());
      }

   public:
      static TextOwner *make(StringRef text)
      {
         auto size = totalSizeToAlloc<char>(text.size());
         void *data = ::operator new(size);
         return new (data) TextOwner(text);
      }

      const char *getText() const
      {
         return getTrailingObjects<char>();
      }
   };

   /// The text this owned string represents
   StringRef text;

   /// In case of a ref counted string an owner that keeps the buffer \c text
   /// references alive.
   IntrusiveRefCntPtr<TextOwner> m_ownedPtr;

   OwnedString(StringRef text, IntrusiveRefCntPtr<TextOwner> ownedPtr)
      : text(text), m_ownedPtr(ownedPtr)
   {}

public:
   OwnedString() : OwnedString(/*text=*/StringRef(), /*m_ownedPtr=*/nullptr) {}

   /// Create a ref counted \c OwnedString that is initialized with the text of
   /// the given \c StringRef.
   OwnedString(StringRef str) : OwnedString(makeRefCounted(str)) {}

   /// Create a ref counted \c OwnedString that is initialized with the text of
   /// the given buffer.
   OwnedString(const char *str) : OwnedString(StringRef(str)) {}

   /// Create an \c OwnedString that references the given string. The
   /// \c OwnedString will not take ownership of that buffer and will assume that
   /// the buffer outlives its lifetime.
   static OwnedString makeUnowned(StringRef str)
   {
      return OwnedString(str, /*m_ownedPtr=*/nullptr);
   }

   /// Create an \c OwnedString that keeps its contents in a reference counted
   /// buffer. The contents of \p str will be copied initially and are allowed to
   /// be disposed after the \c OwnedString has been created.
   static OwnedString makeRefCounted(StringRef str)
   {
      if (str.empty()) {
         // Copying an empty string doesn't make sense. Just create an unowned
         // string that points to the empty string.
         return makeUnowned(str);
      } else {
         IntrusiveRefCntPtr<TextOwner> m_ownedPtr(TextOwner::make(str));
         // Allocate the StringRef on the stack first.  This is to ensure that the
         // order of evaluation of the arguments is specified.  The specification
         // does not specify the order of evaluation for the arguments.  Itanium
         // chose to evaluate left to right, while Windows evaluates right to left.
         // As such, it is possible that the m_ownedPtr has already been `std::move`d
         // by the time that the StringRef is attempted to be created.  In such a
         // case, the offset of the field (+4) is used instead of the pointer to
         // the text, resulting in invalid memory references.
         StringRef S(m_ownedPtr->getText(), str.size());
         return OwnedString(S, std::move(m_ownedPtr));
      }
   }

   /// Returns the length of the string in bytes.
   size_t size() const
   {
      return text.size();
   }

   size_t getSize() const
   {
      return size();
   }

   /// Returns true if the length is 0.
   bool empty() const
   {
      return size() == 0;
   }

   /// Returns a StringRef to the underlying data. No copy is made and no
   /// ownership changes take place.
   StringRef str() const
   {
      return text;
   }

   StringRef getStr() const
   {
      return text;
   }

   bool operator==(const OwnedString &Right) const
   {
      return str() == Right.str();
   }
};

} // basic
} // polar

#endif // POLARPHP_BASIC_ADT_OWNED_STRING_H
