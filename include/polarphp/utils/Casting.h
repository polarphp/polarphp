// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/29.

//===----------------------------------------------------------------------===//
//
// This file defines the isa<X>(), cast<X>(), dyn_cast<X>(), cast_or_null<X>(),
// and dyn_cast_or_null<X>() templates.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_UTILS_CASTING_H
#define POLARPHP_UTILS_CASTING_H

#include "polarphp/global/Global.h"
#include "polarphp/utils/TypeTraits.h"
#include <cassert>
#include <memory>
#include <type_traits>

namespace polar {
namespace utils {

//===----------------------------------------------------------------------===//
//                          isa<x> Support Templates
//===----------------------------------------------------------------------===//

// Define a template that can be specialized by smart pointers to reflect the
// fact that they are automatically dereferenced, and are not involved with the
// template selection process...  the default implementation is a noop.
//
template<typename From>
struct SmplifyType
{
   using SimpleType = From; // The real type this represents...

   // An accessor to get the real value...
   static SimpleType &getSimplifiedValue(From &value)
   {
      return value;
   }
};

template<typename From>
struct SmplifyType<const From>
{
   using NonConstSimpleType = typename SmplifyType<From>::SimpleType;
   using SimpleType =
   typename add_const_past_pointer<NonConstSimpleType>::type;
   using RetType =
   typename add_lvalue_reference_if_not_pointer<SimpleType>::type;

   static RetType getSimplifiedValue(const From& value)
   {
      return SmplifyType<From>::getSimplifiedValue(const_cast<From&>(value));
   }
};

// The core of the implementation of isa<X> is here; To and From should be
// the names of classes.  This template can be specialized to customize the
// implementation of isa<> without rewriting it from scratch.
template <typename To, typename From, typename Enabler = void>
struct IsaImpl
{
   static inline bool doit(const From &value)
   {
      return To::classof(&value);
   }
};

/// \brief Always allow upcasts, and perform no dynamic check for them.
template <typename To, typename From>
struct IsaImpl<
      To, From, typename std::enable_if<std::is_base_of<To, From>::value>::type> {
   static inline bool doit(const From &)
   {
      return true;
   }
};

template <typename To, typename From> struct IsaImplCl
{
   static inline bool doit(const From &value)
   {
      return IsaImpl<To, From>::doit(value);
   }
};

template <typename To, typename From> struct IsaImplCl<To, const From>
{
   static inline bool doit(const From &value)
   {
      return IsaImpl<To, From>::doit(value);
   }
};

template <typename To, typename From>
struct IsaImplCl<To, const std::unique_ptr<From>>
{
   static inline bool doit(const std::unique_ptr<From> &value)
   {
      assert(value && "isa<> used on a null pointer");
      return IsaImplCl<To, From>::doit(*value);
   }
};

template <typename To, typename From> struct IsaImplCl<To, From*>
{
   static inline bool doit(const From *value)
   {
      assert(value && "isa<> used on a null pointer");
      return IsaImpl<To, From>::doit(*value);
   }
};

template <typename To, typename From> struct IsaImplCl<To, From*const>
{
   static inline bool doit(const From *value)
   {
      assert(value && "isa<> used on a null pointer");
      return IsaImpl<To, From>::doit(*value);
   }
};

template <typename To, typename From> struct IsaImplCl<To, const From*>
{
   static inline bool doit(const From *value)
   {
      assert(value && "isa<> used on a null pointer");
      return IsaImpl<To, From>::doit(*value);
   }
};

template <typename To, typename From> struct IsaImplCl<To, const From*const>
{
   static inline bool doit(const From *value)
   {
      assert(value && "isa<> used on a null pointer");
      return IsaImpl<To, From>::doit(*value);
   }
};

template<typename To, typename From, typename SimpleFrom>
struct IsaImplWrap
{
   // When From != SimplifiedType, we can simplify the type some more by using
   // the SmplifyType template.
   static bool doit(const From &value)
   {
      return IsaImplWrap<To, SimpleFrom,
            typename SmplifyType<SimpleFrom>::SimpleType>::doit(
               SmplifyType<const From>::getSimplifiedValue(value));
   }
};

template<typename To, typename FromTy>
struct IsaImplWrap<To, FromTy, FromTy>
{
   // When From == SimpleType, we are as simple as we are going to get.
   static bool doit(const FromTy &value)
   {
      return IsaImplCl<To,FromTy>::doit(value);
   }
};

// isa<X> - Return true if the parameter to the template is an instance of the
// template type argument.  Used like this:
//
//  if (isa<Type>(myVal)) { ... }
//
template <typename X, typename Y>
POLAR_NODISCARD inline bool isa(const Y &value)
{
   return IsaImplWrap<X, const Y,
         typename SmplifyType<const Y>::SimpleType>::doit(value);
}

//===----------------------------------------------------------------------===//
//                          cast<x> Support Templates
//===----------------------------------------------------------------------===//


template <typename To, typename From>
struct CastRetty;

// Calculate what type the 'cast' function should return, based on a requested
// type of To and a source type of From.

template <typename To, typename From>
struct CastRettyImpl
{
   using RetType = To &;       // Normal case, return Ty&
};


template <typename To, typename From>
struct CastRettyImpl<To, const From>
{
   using RetType = const To &; // Normal case, return Ty&
};


template <typename To, typename From>
struct CastRettyImpl<To, From*>
{
   using RetType = To *;       // Pointer arg case, return Ty*
};


template <typename To, typename From>
struct CastRettyImpl<To, const From*>
{
   using RetType = const To *; // Constant pointer arg case, return const Ty*
};


template <typename To, typename From>
struct CastRettyImpl<To, const From*const>
{
   using RetType = const To *; // Constant pointer arg case, return const Ty*
};

template <typename To, typename From>
struct CastRettyImpl<To, std::unique_ptr<From>>
{
private:
   using PointerType = typename CastRettyImpl<To, From *>::RetType;
   using ResultType = typename std::remove_pointer<PointerType>::type;

public:
   using RetType = std::unique_ptr<ResultType>;
};


template <typename To, typename From, typename SimpleFrom>
struct CastRettyWrap
{
   // When the simplified type and the from type are not the same, use the type
   // simplifier to reduce the type, then reuse CastRettyImpl to get the
   // resultant type.
   using RetType = typename CastRetty<To, SimpleFrom>::RetType;
};


template <typename To, typename FromTy>
struct CastRettyWrap<To, FromTy, FromTy>
{
   // When the simplified type is equal to the from type, use it directly.
   using RetType = typename CastRettyImpl<To,FromTy>::RetType;
};


template <typename To, typename From>
struct CastRetty
{
   using RetType = typename CastRettyWrap<
   To, From, typename SmplifyType<From>::SimpleType>::RetType;
};

// Ensure the non-simple values are converted using the SmplifyType template
// that may be specialized by smart pointers...
//

template <typename To, typename From, typename SimpleFrom>
struct CastConvertVal
{
   // This is not a simple type, use the template to simplify it...
   static typename CastRetty<To, From>::RetType doit(From &value)
   {
      return CastConvertVal<To, SimpleFrom,
            typename SmplifyType<SimpleFrom>::SimpleType>::doit(
               SmplifyType<From>::getSimplifiedValue(value));
   }
};


template <typename To, typename FromTy>
struct CastConvertVal<To,FromTy,FromTy>
{
   // This _is_ a simple type, just cast it.
   static typename CastRetty<To, FromTy>::RetType doit(const FromTy &value)
   {
      typename CastRetty<To, FromTy>::RetType res2
            = (typename CastRetty<To, FromTy>::RetType)const_cast<FromTy&>(value);
      return res2;
   }
};

template <typename X>
struct IsSimpleType
{
   static const bool value =
         std::is_same<X, typename SmplifyType<X>::SimpleType>::value;
};

// cast<X> - Return the argument parameter cast to the specified type.  This
// casting operator asserts that the type is correct, so it does not return null
// on failure.  It does not allow a null argument (use cast_or_null for that).
// It is typically used like this:
//
//  cast<Instruction>(myVal)->getParent()
//
template <typename X, typename Y>
inline typename std::enable_if<!IsSimpleType<Y>::value,
typename CastRetty<X, const Y>::RetType>::type
cast(const Y &value)
{
   assert(isa<X>(value) && "cast<Ty>() argument of incompatible type!");
   return CastConvertVal<
         X, const Y, typename SmplifyType<const Y>::SimpleType>::doit(value);
}

template <typename X, typename Y>
inline typename CastRetty<X, Y>::RetType cast(Y &value)
{
   assert(isa<X>(value) && "cast<Ty>() argument of incompatible type!");
   return CastConvertVal<X, Y,
         typename SmplifyType<Y>::SimpleType>::doit(value);
}

template <typename X, typename Y>
inline typename CastRetty<X, Y *>::RetType cast(Y *value)
{
   assert(isa<X>(value) && "cast<Ty>() argument of incompatible type!");
   return CastConvertVal<X, Y*,
         typename SmplifyType<Y*>::SimpleType>::doit(value);
}

template <typename X, typename Y>
inline typename CastRetty<X, std::unique_ptr<Y>>::RetType
cast(std::unique_ptr<Y> &&value) {
   assert(isa<X>(value.get()) && "cast<Ty>() argument of incompatible type!");
   using RetType = typename CastRetty<X, std::unique_ptr<Y>>::RetType;
   return RetType(
            CastConvertVal<X, Y *, typename SmplifyType<Y *>::SimpleType>::doit(
               value.release()));
}

// cast_or_null<X> - Functionally identical to cast, except that a null value is
// accepted.
//
template <typename X, typename Y>
POLAR_NODISCARD inline
typename std::enable_if<!IsSimpleType<Y>::value,
typename CastRetty<X, const Y>::RetType>::type
cast_or_null(const Y &value)
{
   if (!value)
      return nullptr;
   assert(isa<X>(value) && "cast_or_null<Ty>() argument of incompatible type!");
   return cast<X>(value);
}

template <typename X, typename Y>
POLAR_NODISCARD inline
typename std::enable_if<!IsSimpleType<Y>::value,
typename CastRetty<X, Y>::RetType>::type
cast_or_null(Y &value)
{
   if (!value)
      return nullptr;
   assert(isa<X>(value) && "cast_or_null<Ty>() argument of incompatible type!");
   return cast<X>(value);
}

template <typename X, typename Y>
POLAR_NODISCARD inline typename CastRetty<X, Y *>::RetType
cast_or_null(Y *value)
{
   if (!value) return nullptr;
   assert(isa<X>(value) && "cast_or_null<Ty>() argument of incompatible type!");
   return cast<X>(value);
}

template <typename X, typename Y>
inline typename CastRetty<X, std::unique_ptr<Y>>::RetType
cast_or_null(std::unique_ptr<Y> &&value)
{
   if (!value)
      return nullptr;
   return cast<X>(std::move(value));
}

// dyn_cast<X> - Return the argument parameter cast to the specified type.  This
// casting operator returns null if the argument is of the wrong type, so it can
// be used to test for a type as well as cast if successful.  This should be
// used in the context of an if statement like this:
//
//  if (const Instruction *I = dyn_cast<Instruction>(myVal)) { ... }
//

template <typename X, typename Y>
POLAR_NODISCARD inline
typename std::enable_if<!IsSimpleType<Y>::value,
typename CastRetty<X, const Y>::RetType>::type
dyn_cast(const Y &value)
{
   return isa<X>(value) ? cast<X>(value) : nullptr;
}

template <typename X, typename Y>
POLAR_NODISCARD inline typename CastRetty<X, Y>::RetType dyn_cast(Y &value)
{
   return isa<X>(value) ? cast<X>(value) : nullptr;
}

template <typename X, typename Y>
POLAR_NODISCARD inline typename CastRetty<X, Y *>::RetType dyn_cast(Y *value) {
   return isa<X>(value) ? cast<X>(value) : nullptr;
}

// dyn_cast_or_null<X> - Functionally identical to dyn_cast, except that a null
// value is accepted.
//
template <typename X, typename Y>
POLAR_NODISCARD inline
typename std::enable_if<!IsSimpleType<Y>::value,
typename CastRetty<X, const Y>::RetType>::type
dyn_cast_or_null(const Y &value)
{
   return (value && isa<X>(value)) ? cast<X>(value) : nullptr;
}

template <typename X, typename Y>
POLAR_NODISCARD inline
typename std::enable_if<!IsSimpleType<Y>::value,
typename CastRetty<X, Y>::RetType>::type
dyn_cast_or_null(Y &value)
{
   return (value && isa<X>(value)) ? cast<X>(value) : nullptr;
}

template <typename X, typename Y>
POLAR_NODISCARD inline typename CastRetty<X, Y *>::RetType
dyn_cast_or_null(Y *value)
{
   return (value && isa<X>(value)) ? cast<X>(value) : nullptr;
}

// unique_dyn_cast<X> - Given a unique_ptr<Y>, try to return a unique_ptr<X>,
// taking ownership of the input pointer iff isa<X>(value) is true.  If the
// cast is successful, From refers to nullptr on exit and the casted value
// is returned.  If the cast is unsuccessful, the function returns nullptr
// and From is unchanged.
template <typename X, typename Y>
POLAR_NODISCARD inline auto unique_dyn_cast(std::unique_ptr<Y> &value)
-> decltype(cast<X>(value))
{
   if (!isa<X>(value)) {
      return nullptr;
   }
   return cast<X>(std::move(value));
}

template <typename X, typename Y>
POLAR_NODISCARD inline auto unique_dyn_cast(std::unique_ptr<Y> &&value)
-> decltype(cast<X>(value))
{
   return unique_dyn_cast<X, Y>(value);
}

// dyn_cast_or_null<X> - Functionally identical to unique_dyn_cast, except that
// a null value is accepted.
template <typename X, typename Y>
POLAR_NODISCARD inline auto unique_dyn_cast_or_null(std::unique_ptr<Y> &value)
-> decltype(cast<X>(value))
{
   if (!value) {
      return nullptr;
   }
   return unique_dyn_cast<X, Y>(value);
}

template <typename X, typename Y>
POLAR_NODISCARD inline auto unique_dyn_cast_or_null(std::unique_ptr<Y> &&value)
-> decltype(cast<X>(value))
{
   return unique_dyn_cast_or_null<X, Y>(value);
}

} // utils
} // polar

#endif // POLARPHP_UTILS_CASTING_H
