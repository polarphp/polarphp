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

#ifndef POLARPHP_VMAPI_LANG_CONSTANT_H
#define POLARPHP_VMAPI_LANG_CONSTANT_H

#include "polarphp/vm/ZendApi.h"
#include "polarphp/basic/adt/StringRef.h"
#include <any>

struct _zend_constant;

namespace polar {
namespace vmapi {

namespace internal
{
class ModulePrivate;
class ConstantPrivate;
class NamespacePrivate;
} // internal

using internal::ConstantPrivate;
using polar::basic::StringRef;

class VMAPI_DECL_EXPORT Constant final
{
public:
   Constant(StringRef name, std::nullptr_t value = nullptr);
   Constant(StringRef name, bool value);
   Constant(StringRef name, int32_t value);
   Constant(StringRef name, int64_t value);
   Constant(StringRef name, double value);
   Constant(StringRef name, const char *value);
   Constant(StringRef name, StringRef value);
   Constant(const Constant &other);
   Constant(Constant &&other) noexcept;
   Constant &operator=(const Constant &other);
   Constant &operator=(Constant &&other) noexcept;
   const std::string &getName() const;
   const _zend_constant &getZendConstant() const;
   Variant getValueVariant() const;
   std::any getValue() const;
   virtual ~Constant();
private:
   void initialize(const std::string &prefix, int moduleNumber);
   void initialize(int moduleNumber);
private:
   VMAPI_DECLARE_PRIVATE(Constant);
   std::shared_ptr<ConstantPrivate> m_implPtr;
   friend class internal::ModulePrivate;
   friend class internal::NamespacePrivate;
};

} // vmapi
} // polar

#endif // POLARPHP_VMAPI_LANG_CONSTANT_H
