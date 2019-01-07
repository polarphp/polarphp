// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/28.

#include "polarphp/vm/lang/Type.h"
#include "gtest/gtest.h"

using polar::vmapi::Modifier;

TEST(TypeTest, testModifierOperator)
{
   {
      Modifier modifier = Modifier::Abstract;
      ASSERT_TRUE(modifier == Modifier::Abstract);
      modifier |= Modifier::Public;
      ASSERT_TRUE(modifier == (0x02 | 0x100));
   }
   ASSERT_EQ(Modifier::Abstract | Modifier::Public, (0x02 | 0x100));
   ASSERT_EQ(Modifier::MethodModifiers, Modifier::Final | Modifier::Public | Modifier::Protected | Modifier::Private | Modifier::Static);
   ASSERT_EQ(Modifier::PropertyModifiers, Modifier::Final | Modifier::Public | Modifier::Protected | Modifier::Private | Modifier::Const | Modifier::Static);
   {
      Modifier modifier = Modifier::Public;
      ASSERT_TRUE((modifier & Modifier::Public) == Modifier::Public);
      modifier |= Modifier::Const;
      ASSERT_TRUE((modifier & Modifier::Const) == Modifier::Const);
      ASSERT_FALSE((modifier & Modifier::Const) == Modifier::Protected);
      modifier &= Modifier::Const;
      ASSERT_TRUE(modifier == Modifier::Const);
      ASSERT_TRUE((Modifier::MethodModifiers & Modifier::Final) == Modifier::Final);
      ASSERT_TRUE((Modifier::MethodModifiers & Modifier::Public) == Modifier::Public);
      ASSERT_FALSE((Modifier::MethodModifiers & Modifier::Const) == Modifier::Const);
   }
}
