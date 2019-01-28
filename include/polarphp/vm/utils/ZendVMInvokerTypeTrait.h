// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/01/26.

#ifndef POLARPHP_VMAPI_UTILS_ZENDVM_INVOKER_TYPE_TRAIT_H
#define POLARPHP_VMAPI_UTILS_ZENDVM_INVOKER_TYPE_TRAIT_H

#include <type_traits>

namespace polar {
namespace vmapi {

// forward declare class
class Parameters;

template <typename CallableType>
struct callable_prototype_checker : public std::false_type
{};

template <typename ReturnType>
struct callable_prototype_checker <ReturnType (*)(Parameters &)> : public std::true_type
{};

template <typename ReturnType>
struct callable_prototype_checker <ReturnType (*)()> : public std::true_type
{};

template <typename CallableType>
struct method_callable_prototype_checker : public std::false_type
{};

template <typename ReturnType>
struct method_callable_prototype_checker <ReturnType (*)(Parameters &)> : public std::true_type
{};

template <typename ReturnType>
struct method_callable_prototype_checker <ReturnType (*)()> : public std::true_type
{};

template <typename Class, typename ReturnType>
struct method_callable_prototype_checker <ReturnType (Class::*)(Parameters &)> : public std::true_type
{};

template <typename Class, typename ReturnType>
struct method_callable_prototype_checker <ReturnType (Class::*)(Parameters &) const> : public std::true_type
{};

template <typename Class, typename ReturnType>
struct method_callable_prototype_checker <ReturnType (Class::*)(Parameters &) volatile> : public std::true_type
{};

template <typename Class, typename ReturnType>
struct method_callable_prototype_checker <ReturnType (Class::*)(Parameters &) const volatile> : public std::true_type
{};

template <typename Class, typename ReturnType>
struct method_callable_prototype_checker <ReturnType (Class::*)(Parameters &) &> : public std::true_type
{};

template <typename Class, typename ReturnType>
struct method_callable_prototype_checker <ReturnType (Class::*)(Parameters &) const &> : public std::true_type
{};

template <typename Class, typename ReturnType>
struct method_callable_prototype_checker <ReturnType (Class::*)(Parameters &) volatile &> : public std::true_type
{};

template <typename Class, typename ReturnType>
struct method_callable_prototype_checker <ReturnType (Class::*)(Parameters &) const volatile &> : public std::true_type
{};

template <typename Class, typename ReturnType>
struct method_callable_prototype_checker <ReturnType (Class::*)(Parameters &) &&> : public std::true_type
{};

template <typename Class, typename ReturnType>
struct method_callable_prototype_checker <ReturnType (Class::*)()> : public std::true_type
{};

template <typename Class, typename ReturnType>
struct method_callable_prototype_checker <ReturnType (Class::*)() const> : public std::true_type
{};

template <typename Class, typename ReturnType>
struct method_callable_prototype_checker <ReturnType (Class::*)() volatile> : public std::true_type
{};

template <typename Class, typename ReturnType>
struct method_callable_prototype_checker <ReturnType (Class::*)() const volatile> : public std::true_type
{};

template <typename Class, typename ReturnType>
struct method_callable_prototype_checker <ReturnType (Class::*)() &> : public std::true_type
{};

template <typename Class, typename ReturnType>
struct method_callable_prototype_checker <ReturnType (Class::*)() const &> : public std::true_type
{};

template <typename Class, typename ReturnType>
struct method_callable_prototype_checker <ReturnType (Class::*)() volatile &> : public std::true_type
{};

template <typename Class, typename ReturnType>
struct method_callable_prototype_checker <ReturnType (Class::*)() const volatile &> : public std::true_type
{};

template <typename Class, typename ReturnType>
struct method_callable_prototype_checker <ReturnType (Class::*)() &&> : public std::true_type
{};

} // vmapi
} // polar

#endif // POLARPHP_VMAPI_UTILS_ZENDVM_INVOKER_TYPE_TRAIT_H
