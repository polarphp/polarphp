// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/24.

#ifndef POLARPHP_VMAPI_UTILS_CALLABLE_TRAITS_H
#define POLARPHP_VMAPI_UTILS_CALLABLE_TRAITS_H

#include "polarphp/global/CompilerDetection.h"

#include <type_traits>
#include <tuple>

namespace polar {
namespace vmapi {

template <typename CallableType>
struct CallableInfoTrait
{
   constexpr const static bool isMemberCallable = false;
};

template <typename RetType, typename ...ParamTypes>
struct CallableInfoTrait<RetType (&)(ParamTypes ...args)>
{
   using ReturnType = RetType;
   constexpr const static size_t argNum = sizeof...(ParamTypes);
   constexpr const static bool hasVaridicParams = false;
   constexpr const static bool hasParamDef = argNum != 0 || hasVaridicParams;
   constexpr const static bool isMemberCallable = false;
   constexpr const static bool hasReturn = !std::is_same<RetType, void>::value;
   template <size_t index>
   struct arg
   {
      using type = typename std::tuple_element<index, std::tuple<ParamTypes...>>::type;
   };
};

template <typename RetType, typename ...ParamTypes>
struct CallableInfoTrait<RetType (*)(ParamTypes ...args)>
{
   using ReturnType = RetType;
   constexpr const static size_t argNum = sizeof...(ParamTypes);
   constexpr const static bool hasVaridicParams = false;
   constexpr const static bool hasParamDef = argNum != 0 || hasVaridicParams;
   constexpr const static bool isMemberCallable = false;
   constexpr const static bool hasReturn = !std::is_same<RetType, void>::value;
   template <size_t index>
   struct arg
   {
      using type = typename std::tuple_element<index, std::tuple<ParamTypes...>>::type;
   };
};

template <typename RetType, typename ...ParamTypes>
struct CallableInfoTrait<RetType (ParamTypes ...args)>
{
   using ReturnType = RetType;
   constexpr const static size_t argNum = sizeof...(ParamTypes);
   constexpr const static bool hasVaridicParams = false;
   constexpr const static bool hasParamDef = argNum != 0 || hasVaridicParams;
   constexpr const static bool isMemberCallable = false;
   constexpr const static bool hasReturn = !std::is_same<RetType, void>::value;
   template <size_t index>
   struct arg
   {
      using type = typename std::tuple_element<index, std::tuple<ParamTypes...>>::type;
   };
};

template <typename RetType, typename ...ParamTypes>
struct CallableInfoTrait<RetType (&)(ParamTypes ...args, ...)>
{
   using ReturnType = RetType;
   constexpr const static size_t argNum = sizeof...(ParamTypes);
   constexpr const static bool hasVaridicParams = true;
   constexpr const static bool hasParamDef = argNum != 0 || hasVaridicParams;
   constexpr const static bool isMemberCallable = false;
   constexpr const static bool hasReturn = !std::is_same<RetType, void>::value;
   template <size_t index>
   struct arg
   {
      using type = typename std::tuple_element<index, std::tuple<ParamTypes...>>::type;
   };
};

template <typename RetType, typename ...ParamTypes>
struct CallableInfoTrait<RetType (*)(ParamTypes ...args, ...)>
{
   using ReturnType = RetType;
   constexpr const static size_t argNum = sizeof...(ParamTypes);
   constexpr const static bool hasVaridicParams = true;
   constexpr const static bool hasParamDef = argNum != 0 || hasVaridicParams;
   constexpr const static bool isMemberCallable = false;
   constexpr const static bool hasReturn = !std::is_same<RetType, void>::value;
   template <size_t index>
   struct arg
   {
      using type = typename std::tuple_element<index, std::tuple<ParamTypes...>>::type;
   };
};

template <typename RetType, typename ...ParamTypes>
struct CallableInfoTrait<RetType (ParamTypes ...args, ...)>
{
   using ReturnType = RetType;
   constexpr const static size_t argNum = sizeof...(ParamTypes);
   constexpr const static bool hasVaridicParams = true;
   constexpr const static bool hasParamDef = argNum != 0 || hasVaridicParams;
   constexpr const static bool isMemberCallable = false;
   constexpr const static bool hasReturn = !std::is_same<RetType, void>::value;
   template <size_t index>
   struct arg
   {
      using type = typename std::tuple_element<index, std::tuple<ParamTypes...>>::type;
   };
};

template <typename RetType, typename Class, typename ...ParamTypes>
struct CallableInfoTrait<RetType (Class::*)(ParamTypes... args)>
{
   using ReturnType = RetType;
   constexpr const static size_t argNum = sizeof...(ParamTypes);
   constexpr const static bool hasVaridicParams = false;
   constexpr const static bool hasParamDef = argNum != 0 || hasVaridicParams;
   constexpr const static bool isMemberCallable = true;
   constexpr const static bool hasReturn = !std::is_same<RetType, void>::value;
   template <size_t index>
   struct arg
   {
      using type = typename std::tuple_element<index, std::tuple<ParamTypes...>>::type;
   };
};

template <typename RetType, typename Class, typename ...ParamTypes>
struct CallableInfoTrait<RetType (Class::*)(ParamTypes... args, ...)>
{
   using ReturnType = RetType;
   constexpr const static size_t argNum = sizeof...(ParamTypes);
   constexpr const static bool hasVaridicParams = true;
   constexpr const static bool hasParamDef = argNum != 0 || hasVaridicParams;
   constexpr const static bool isMemberCallable = true;
   constexpr const static bool hasReturn = !std::is_same<RetType, void>::value;
   template <size_t index>
   struct arg
   {
      using type = typename std::tuple_element<index, std::tuple<ParamTypes...>>::type;
   };
};

template <typename RetType, typename Class, typename ...ParamTypes>
struct CallableInfoTrait<RetType (Class::*)(ParamTypes... args) const>
{
   using ReturnType = RetType;
   constexpr const static size_t argNum = sizeof...(ParamTypes);
   constexpr const static bool hasVaridicParams = false;
   constexpr const static bool hasParamDef = argNum != 0 || hasVaridicParams;
   constexpr const static bool isMemberCallable = true;
   constexpr const static bool hasReturn = !std::is_same<RetType, void>::value;
   template <size_t index>
   struct arg
   {
      using type = typename std::tuple_element<index, std::tuple<ParamTypes...>>::type;
   };
};

template <typename RetType, typename Class, typename ...ParamTypes>
struct CallableInfoTrait<RetType (Class::*)(ParamTypes... args, ...) const>
{
   using ReturnType = RetType;
   constexpr const static size_t argNum = sizeof...(ParamTypes);
   constexpr const static bool hasVaridicParams = true;
   constexpr const static bool hasParamDef = argNum != 0 || hasVaridicParams;
   constexpr const static bool isMemberCallable = true;
   constexpr const static bool hasReturn = !std::is_same<RetType, void>::value;
   template <size_t index>
   struct arg
   {
      using type = typename std::tuple_element<index, std::tuple<ParamTypes...>>::type;
   };
};

template <typename RetType, typename Class, typename ...ParamTypes>
struct CallableInfoTrait<RetType (Class::*)(ParamTypes... args) volatile>
{
   using ReturnType = RetType;
   constexpr const static size_t argNum = sizeof...(ParamTypes);
   constexpr const static bool hasVaridicParams = false;
   constexpr const static bool hasParamDef = argNum != 0 || hasVaridicParams;
   constexpr const static bool isMemberCallable = true;
   constexpr const static bool hasReturn = !std::is_same<RetType, void>::value;
   template <size_t index>
   struct arg
   {
      using type = typename std::tuple_element<index, std::tuple<ParamTypes...>>::type;
   };
};

template <typename RetType, typename Class, typename ...ParamTypes>
struct CallableInfoTrait<RetType (Class::*)(ParamTypes... args, ...) volatile>
{
   using ReturnType = RetType;
   constexpr const static size_t argNum = sizeof...(ParamTypes);
   constexpr const static bool hasVaridicParams = true;
   constexpr const static bool hasParamDef = argNum != 0 || hasVaridicParams;
   constexpr const static bool isMemberCallable = true;
   constexpr const static bool hasReturn = !std::is_same<RetType, void>::value;
   template <size_t index>
   struct arg
   {
      using type = typename std::tuple_element<index, std::tuple<ParamTypes...>>::type;
   };
};

template <typename RetType, typename Class, typename ...ParamTypes>
struct CallableInfoTrait<RetType (Class::*)(ParamTypes... args) const volatile>
{
   using ReturnType = RetType;
   constexpr const static size_t argNum = sizeof...(ParamTypes);
   constexpr const static bool hasVaridicParams = false;
   constexpr const static bool hasParamDef = argNum != 0 || hasVaridicParams;
   constexpr const static bool isMemberCallable = true;
   constexpr const static bool hasReturn = !std::is_same<RetType, void>::value;
   template <size_t index>
   struct arg
   {
      using type = typename std::tuple_element<index, std::tuple<ParamTypes...>>::type;
   };
};

template <typename RetType, typename Class, typename ...ParamTypes>
struct CallableInfoTrait<RetType (Class::*)(ParamTypes... args, ...) const volatile>
{
   using ReturnType = RetType;
   constexpr const static size_t argNum = sizeof...(ParamTypes);
   constexpr const static bool hasVaridicParams = true;
   constexpr const static bool hasParamDef = argNum != 0 || hasVaridicParams;
   constexpr const static bool isMemberCallable = true;
   constexpr const static bool hasReturn = !std::is_same<RetType, void>::value;
   template <size_t index>
   struct arg
   {
      using type = typename std::tuple_element<index, std::tuple<ParamTypes...>>::type;
   };
};

template <typename RetType, typename Class, typename ...ParamTypes>
struct CallableInfoTrait<RetType (Class::*)(ParamTypes... args) &>
{
   using ReturnType = RetType;
   constexpr const static size_t argNum = sizeof...(ParamTypes);
   constexpr const static bool hasVaridicParams = false;
   constexpr const static bool hasParamDef = argNum != 0 || hasVaridicParams;
   constexpr const static bool isMemberCallable = true;
   constexpr const static bool hasReturn = !std::is_same<RetType, void>::value;
   template <size_t index>
   struct arg
   {
      using type = typename std::tuple_element<index, std::tuple<ParamTypes...>>::type;
   };
};

template <typename RetType, typename Class, typename ...ParamTypes>
struct CallableInfoTrait<RetType (Class::*)(ParamTypes... args, ...) &>
{
   using ReturnType = RetType;
   constexpr const static size_t argNum = sizeof...(ParamTypes);
   constexpr const static bool hasVaridicParams = true;
   constexpr const static bool hasParamDef = argNum != 0 || hasVaridicParams;
   constexpr const static bool isMemberCallable = true;
   constexpr const static bool hasReturn = !std::is_same<RetType, void>::value;
   template <size_t index>
   struct arg
   {
      using type = typename std::tuple_element<index, std::tuple<ParamTypes...>>::type;
   };
};

template <typename RetType, typename Class, typename ...ParamTypes>
struct CallableInfoTrait<RetType (Class::*)(ParamTypes... args) const&>
{
   using ReturnType = RetType;
   constexpr const static size_t argNum = sizeof...(ParamTypes);
   constexpr const static bool hasVaridicParams = false;
   constexpr const static bool hasParamDef = argNum != 0 || hasVaridicParams;
   constexpr const static bool isMemberCallable = true;
   constexpr const static bool hasReturn = !std::is_same<RetType, void>::value;
   template <size_t index>
   struct arg
   {
      using type = typename std::tuple_element<index, std::tuple<ParamTypes...>>::type;
   };
};

template <typename RetType, typename Class, typename ...ParamTypes>
struct CallableInfoTrait<RetType (Class::*)(ParamTypes... args, ...) const&>
{
   using ReturnType = RetType;
   constexpr const static size_t argNum = sizeof...(ParamTypes);
   constexpr const static bool hasVaridicParams = true;
   constexpr const static bool hasParamDef = argNum != 0 || hasVaridicParams;
   constexpr const static bool isMemberCallable = true;
   constexpr const static bool hasReturn = !std::is_same<RetType, void>::value;
   template <size_t index>
   struct arg
   {
      using type = typename std::tuple_element<index, std::tuple<ParamTypes...>>::type;
   };
};

template <typename RetType, typename Class, typename ...ParamTypes>
struct CallableInfoTrait<RetType (Class::*)(ParamTypes... args) volatile&>
{
   using ReturnType = RetType;
   constexpr const static size_t argNum = sizeof...(ParamTypes);
   constexpr const static bool hasVaridicParams = false;
   constexpr const static bool hasParamDef = argNum != 0 || hasVaridicParams;
   constexpr const static bool isMemberCallable = true;
   constexpr const static bool hasReturn = !std::is_same<RetType, void>::value;
   template <size_t index>
   struct arg
   {
      using type = typename std::tuple_element<index, std::tuple<ParamTypes...>>::type;
   };
};

template <typename RetType, typename Class, typename ...ParamTypes>
struct CallableInfoTrait<RetType (Class::*)(ParamTypes... args, ...) volatile&>
{
   using ReturnType = RetType;
   constexpr const static size_t argNum = sizeof...(ParamTypes);
   constexpr const static bool hasVaridicParams = true;
   constexpr const static bool hasParamDef = argNum != 0 || hasVaridicParams;
   constexpr const static bool isMemberCallable = true;
   constexpr const static bool hasReturn = !std::is_same<RetType, void>::value;
   template <size_t index>
   struct arg
   {
      using type = typename std::tuple_element<index, std::tuple<ParamTypes...>>::type;
   };
};

template <typename RetType, typename Class, typename ...ParamTypes>
struct CallableInfoTrait<RetType (Class::*)(ParamTypes... args) const volatile&>
{
   using ReturnType = RetType;
   constexpr const static size_t argNum = sizeof...(ParamTypes);
   constexpr const static bool hasVaridicParams = false;
   constexpr const static bool hasParamDef = argNum != 0 || hasVaridicParams;
   constexpr const static bool isMemberCallable = true;
   constexpr const static bool hasReturn = !std::is_same<RetType, void>::value;
   template <size_t index>
   struct arg
   {
      using type = typename std::tuple_element<index, std::tuple<ParamTypes...>>::type;
   };
};

template <typename RetType, typename Class, typename ...ParamTypes>
struct CallableInfoTrait<RetType (Class::*)(ParamTypes... args, ...) const volatile&>
{
   using ReturnType = RetType;
   constexpr const static size_t argNum = sizeof...(ParamTypes);
   constexpr const static bool hasVaridicParams = true;
   constexpr const static bool hasParamDef = argNum != 0 || hasVaridicParams;
   constexpr const static bool isMemberCallable = true;
   constexpr const static bool hasReturn = !std::is_same<RetType, void>::value;
   template <size_t index>
   struct arg
   {
      using type = typename std::tuple_element<index, std::tuple<ParamTypes...>>::type;
   };
};

template <typename RetType, typename Class, typename ...ParamTypes>
struct CallableInfoTrait<RetType (Class::*)(ParamTypes... args) &&>
{
   using ReturnType = RetType;
   constexpr const static size_t argNum = sizeof...(ParamTypes);
   constexpr const static bool hasVaridicParams = false;
   constexpr const static bool hasParamDef = argNum != 0 || hasVaridicParams;
   constexpr const static bool isMemberCallable = true;
   constexpr const static bool hasReturn = !std::is_same<RetType, void>::value;
   template <size_t index>
   struct arg
   {
      using type = typename std::tuple_element<index, std::tuple<ParamTypes...>>::type;
   };
};

template <typename RetType, typename Class, typename ...ParamTypes>
struct CallableInfoTrait<RetType (Class::*)(ParamTypes... args, ...) &&>
{
   using ReturnType = RetType;
   constexpr const static size_t argNum = sizeof...(ParamTypes);
   constexpr const static bool hasVaridicParams = true;
   constexpr const static bool hasParamDef = argNum != 0 || hasVaridicParams;
   constexpr const static bool isMemberCallable = true;
   constexpr const static bool hasReturn = !std::is_same<RetType, void>::value;
   template <size_t index>
   struct arg
   {
      using type = typename std::tuple_element<index, std::tuple<ParamTypes...>>::type;
   };
};

template <typename RetType, typename Class, typename ...ParamTypes>
struct CallableInfoTrait<RetType (Class::*)(ParamTypes... args) const&&>
{
   using ReturnType = RetType;
   constexpr const static size_t argNum = sizeof...(ParamTypes);
   constexpr const static bool hasVaridicParams = false;
   constexpr const static bool hasParamDef = argNum != 0 || hasVaridicParams;
   constexpr const static bool isMemberCallable = true;
   constexpr const static bool hasReturn = !std::is_same<RetType, void>::value;
   template <size_t index>
   struct arg
   {
      using type = typename std::tuple_element<index, std::tuple<ParamTypes...>>::type;
   };
};

template <typename RetType, typename Class, typename ...ParamTypes>
struct CallableInfoTrait<RetType (Class::*)(ParamTypes... args, ...) const&&>
{
   using ReturnType = RetType;
   constexpr const static size_t argNum = sizeof...(ParamTypes);
   constexpr const static bool hasVaridicParams = true;
   constexpr const static bool hasParamDef = argNum != 0 || hasVaridicParams;
   constexpr const static bool isMemberCallable = true;
   constexpr const static bool hasReturn = !std::is_same<RetType, void>::value;
   template <size_t index>
   struct arg
   {
      using type = typename std::tuple_element<index, std::tuple<ParamTypes...>>::type;
   };
};

template <typename RetType, typename Class, typename ...ParamTypes>
struct CallableInfoTrait<RetType (Class::*)(ParamTypes... args) volatile&&>
{
   using ReturnType = RetType;
   constexpr const static size_t argNum = sizeof...(ParamTypes);
   constexpr const static bool hasVaridicParams = false;
   constexpr const static bool hasParamDef = argNum != 0 || hasVaridicParams;
   constexpr const static bool isMemberCallable = true;
   constexpr const static bool hasReturn = !std::is_same<RetType, void>::value;
   template <size_t index>
   struct arg
   {
      using type = typename std::tuple_element<index, std::tuple<ParamTypes...>>::type;
   };
};

template <typename RetType, typename Class, typename ...ParamTypes>
struct CallableInfoTrait<RetType (Class::*)(ParamTypes... args, ...) volatile&&>
{
   using ReturnType = RetType;
   constexpr const static size_t argNum = sizeof...(ParamTypes);
   constexpr const static bool hasVaridicParams = true;
   constexpr const static bool hasParamDef = argNum != 0 || hasVaridicParams;
   constexpr const static bool isMemberCallable = true;
   constexpr const static bool hasReturn = !std::is_same<RetType, void>::value;
   template <size_t index>
   struct arg
   {
      using type = typename std::tuple_element<index, std::tuple<ParamTypes...>>::type;
   };
};

template <typename RetType, typename Class, typename ...ParamTypes>
struct CallableInfoTrait<RetType (Class::*)(ParamTypes... args) const volatile&&>
{
   using ReturnType = RetType;
   constexpr const static size_t argNum = sizeof...(ParamTypes);
   constexpr const static bool hasVaridicParams = false;
   constexpr const static bool hasParamDef = argNum != 0 || hasVaridicParams;
   constexpr const static bool isMemberCallable = true;
   constexpr const static bool hasReturn = !std::is_same<RetType, void>::value;
   template <size_t index>
   struct arg
   {
      using type = typename std::tuple_element<index, std::tuple<ParamTypes...>>::type;
   };
};

template <typename RetType, typename Class, typename ...ParamTypes>
struct CallableInfoTrait<RetType (Class::*)(ParamTypes... args, ...) const volatile&&>
{
   using ReturnType = RetType;
   constexpr const static size_t argNum = sizeof...(ParamTypes);
   constexpr const static bool hasVaridicParams = true;
   constexpr const static bool hasParamDef = argNum != 0 || hasVaridicParams;
   constexpr const static bool isMemberCallable = true;
   constexpr const static bool hasReturn = !std::is_same<RetType, void>::value;
   template <size_t index>
   struct arg
   {
      using type = typename std::tuple_element<index, std::tuple<ParamTypes...>>::type;
   };
};

namespace internal
{

template <typename MemberPointerType, bool IsMemberFunctionPtr, bool IsMemberObjectPtr>
struct member_pointer_traits_imp
{  // forward declaration; specializations later
   using ClassType = void; // ? is this ok ?
};

template <typename RetType, typename Class, typename ...ParamTypes>
struct member_pointer_traits_imp<RetType (Class::*)(ParamTypes... args), true, false>
{
   using ClassType = Class;
   using ReturnType = RetType;
   using FuncType = RetType (&) (ParamTypes... args);
   // TODO equal ?
   // typedef RetType (FuncType) (ParamTypes...);
};

template <typename RetType, typename Class, typename ...ParamTypes>
struct member_pointer_traits_imp<RetType (Class::*)(ParamTypes... args, ...), true, false>
{
   using ClassType = Class;
   using ReturnType = RetType;
   using FuncType = RetType (&) (ParamTypes... args, ...);
};

template <typename RetType, typename Class, typename ...ParamTypes>
struct member_pointer_traits_imp<RetType (Class::*)(ParamTypes... args) const, true, false>
{
   using ClassType = Class const;
   using ReturnType = RetType;
   using FuncType = RetType (&) (ParamTypes... args);
};

template <typename RetType, typename Class, typename ...ParamTypes>
struct member_pointer_traits_imp<RetType (Class::*)(ParamTypes... args, ...) const, true, false>
{
   using ClassType = Class const;
   using ReturnType = RetType;
   using FuncType = RetType (&) (ParamTypes... args, ...);
};

template <typename RetType, typename Class, typename ...ParamTypes>
struct member_pointer_traits_imp<RetType (Class::*)(ParamTypes... args) volatile, true, false>
{
   using ClassType = Class volatile;
   using ReturnType = RetType;
   using FuncType = RetType (&) (ParamTypes... args);
};

template <typename RetType, typename Class, typename ...ParamTypes>
struct member_pointer_traits_imp<RetType (Class::*)(ParamTypes... args, ...) volatile, true, false>
{
   using ClassType = Class volatile;
   using ReturnType = RetType;
   using FuncType = RetType (&) (ParamTypes... args, ...);
};

template <typename RetType, typename Class, typename ...ParamTypes>
struct member_pointer_traits_imp<RetType (Class::*)(ParamTypes... args) const volatile, true, false>
{
   using ClassType = Class const volatile;
   using ReturnType = RetType;
   using FuncType = RetType (&) (ParamTypes... args);
};

template <typename RetType, typename Class, typename ...ParamTypes>
struct member_pointer_traits_imp<RetType (Class::*)(ParamTypes... args, ...) const volatile, true, false>
{
   using ClassType = Class const volatile;
   using ReturnType = RetType;
   using FuncType = RetType (&) (ParamTypes... args, ...);
};

template <typename RetType, typename Class, typename ...ParamTypes>
struct member_pointer_traits_imp<RetType (Class::*)(ParamTypes... args) &, true, false>
{
   using ClassType = Class &;
   using ReturnType = RetType;
   using FuncType = RetType (&) (ParamTypes... args);
};

template <typename RetType, typename Class, typename ...ParamTypes>
struct member_pointer_traits_imp<RetType (Class::*)(ParamTypes... args, ...) &, true, false>
{
   using ClassType = Class &;
   using ReturnType = RetType;
   using FuncType = RetType (&) (ParamTypes... args, ...);
};

template <typename RetType, typename Class, typename ...ParamTypes>
struct member_pointer_traits_imp<RetType (Class::*)(ParamTypes... args) const&, true, false>
{
   using ClassType = Class const&;
   using ReturnType = RetType;
   using FuncType = RetType (&) (ParamTypes... args);
};

template <typename RetType, typename Class, typename ...ParamTypes>
struct member_pointer_traits_imp<RetType (Class::*)(ParamTypes... args, ...) const&, true, false>
{
   using ClassType = Class const&;
   using ReturnType = RetType;
   using FuncType = RetType (&) (ParamTypes... args, ...);
};

template <typename RetType, typename Class, typename ...ParamTypes>
struct member_pointer_traits_imp<RetType (Class::*)(ParamTypes... args) volatile&, true, false>
{
   using ClassType = Class volatile&;
   using ReturnType = RetType;
   using FuncType = RetType (&) (ParamTypes... args);
};

template <typename RetType, typename Class, typename ...ParamTypes>
struct member_pointer_traits_imp<RetType (Class::*)(ParamTypes... args, ...) volatile&, true, false>
{
   using ClassType = Class volatile&;
   using ReturnType = RetType;
   using FuncType = RetType (&) (ParamTypes... args, ...);
};

template <typename RetType, typename Class, typename ...ParamTypes>
struct member_pointer_traits_imp<RetType (Class::*)(ParamTypes... args) const volatile&, true, false>
{
   using ClassType = Class const volatile&;
   using ReturnType = RetType;
   using FuncType = RetType (&) (ParamTypes... args);
};

template <typename RetType, typename Class, typename ...ParamTypes>
struct member_pointer_traits_imp<RetType (Class::*)(ParamTypes... args, ...) const volatile&, true, false>
{
   using ClassType = Class const volatile&;
   using ReturnType = RetType;
   using FuncType = RetType (&) (ParamTypes... args, ...);
};

template <typename RetType, typename Class, typename ...ParamTypes>
struct member_pointer_traits_imp<RetType (Class::*)(ParamTypes... args) &&, true, false>
{
   using ClassType = Class &&;
   using ReturnType = RetType;
   using FuncType = RetType (&) (ParamTypes... args);
};

template <typename RetType, typename Class, typename ...ParamTypes>
struct member_pointer_traits_imp<RetType (Class::*)(ParamTypes... args, ...) &&, true, false>
{
   using ClassType = Class &&;
   using ReturnType = RetType;
   using FuncType = RetType (&) (ParamTypes... args, ...);
};

template <typename RetType, typename Class, typename ...ParamTypes>
struct member_pointer_traits_imp<RetType (Class::*)(ParamTypes... args) const&&, true, false>
{
   using ClassType = Class const&&;
   using ReturnType = RetType;
   using FuncType = RetType (&) (ParamTypes... args);
};

template <typename RetType, typename Class, typename ...ParamTypes>
struct member_pointer_traits_imp<RetType (Class::*)(ParamTypes... args, ...) const&&, true, false>
{
   using ClassType = Class const&&;
   using ReturnType = RetType;
   using FuncType = RetType (&) (ParamTypes... args, ...);
};

template <typename RetType, typename Class, typename ...ParamTypes>
struct member_pointer_traits_imp<RetType (Class::*)(ParamTypes... args) volatile&&, true, false>
{
   using ClassType = Class volatile&&;
   using ReturnType = RetType;
   using FuncType = RetType (&) (ParamTypes... args);
};

template <typename RetType, typename Class, typename ...ParamTypes>
struct member_pointer_traits_imp<RetType (Class::*)(ParamTypes... args, ...) volatile&&, true, false>
{
   using ClassType = Class volatile&&;
   using ReturnType = RetType;
   using FuncType = RetType (&) (ParamTypes... args, ...);
};

template <typename RetType, typename Class, typename ...ParamTypes>
struct member_pointer_traits_imp<RetType (Class::*)(ParamTypes... args) const volatile&&, true, false>
{
   using ClassType = Class const volatile&&;
   using ReturnType = RetType;
   using FuncType = RetType (&) (ParamTypes... args);
};

template <typename RetType, typename Class, typename ...ParamTypes>
struct member_pointer_traits_imp<RetType (Class::*)(ParamTypes... args, ...) const volatile&&, true, false>
{
   using ClassType = Class const volatile&&;
   using ReturnType = RetType;
   using FuncType = RetType (&) (ParamTypes... args, ...);
};

template <typename UnderType, typename Class>
struct member_pointer_traits_imp<UnderType Class::*, false, true>
{
   using ClassType = Class;
   using ReturnType = UnderType;
};

template <typename DecayedFuncType>
struct member_pointer_class_type
{};

template <typename ReturnType, typename ClassType>
struct member_pointer_class_type<ReturnType ClassType::*>
{
   using type = ClassType;
};

template <typename TargetType>
struct is_reference_wrapper_impl : public std::false_type
{};

template <typename TargetType>
struct is_reference_wrapper_impl<std::reference_wrapper<TargetType>> : public std::true_type
{};

template <typename TargetType>
struct is_reference_wrapper
      : public is_reference_wrapper_impl<typename std::remove_cv<TargetType>::type>
{};

template <typename FuncType, size_t... Is>
auto gen_tuple_impl(FuncType func, std::index_sequence<Is...> )
-> decltype(std::make_tuple(func(Is)...))
{
   return std::make_tuple(func(Is)...);
}

template <typename CallableType, typename GeneratorType, size_t... Is>
auto gen_tuple_with_type_impl(GeneratorType generator, std::index_sequence<Is...> )
-> decltype(std::make_tuple(generator.template generate<typename CallableInfoTrait<CallableType>::template arg<Is>::type>(Is)...))
{
   return std::make_tuple(generator.template generate<typename CallableInfoTrait<CallableType>::template arg<Is>::type>(Is)...);
}
} // internal

template <typename CallableType>
struct CallableHasReturn
{
   constexpr const static bool value = !std::is_same<typename CallableInfoTrait<CallableType>::ReturnType, void>::value;
};

template <typename CallableType>
struct CallableHasNoReturn
{
   constexpr const static bool value = std::is_same<typename CallableInfoTrait<CallableType>::ReturnType, void>::value;
};

template<typename TargetType>
struct is_function_pointer
{
   static const bool value = std::is_pointer<TargetType>::value ?
            std::is_function<typename std::remove_pointer<TargetType>::type>::value :
            false;
};

template <class TypePointer>
struct IsMemberObjectPointer
      : public std::integral_constant<bool, std::is_member_pointer<TypePointer>::value &&
      !CallableInfoTrait<TypePointer>::isMemberCallable>
{};

template <typename MemberPointer>
struct member_pointer_traits
      : public internal::member_pointer_traits_imp<typename std::remove_cv<MemberPointer>::type,
      CallableInfoTrait<MemberPointer>::isMemberCallable,
      IsMemberObjectPointer<MemberPointer>::value>
{};

template <size_t N, typename Generator>
auto gen_tuple(Generator func)
-> decltype(internal::gen_tuple_impl(func, std::make_index_sequence<N>{}))
{
   return internal::gen_tuple_impl(func, std::make_index_sequence<N>{});
}

template <size_t N, typename CallableType, typename Generator>
auto gen_tuple_with_type(Generator generator)
-> decltype(internal::gen_tuple_with_type_impl<CallableType>(generator, std::make_index_sequence<N>{}))
{
   return internal::gen_tuple_with_type_impl<CallableType>(generator, std::make_index_sequence<N>{});
}

} // vmapi
} // polar

#endif // POLARPHP_VMAPI_UTILS_CALLABLE_TRAITS_H
