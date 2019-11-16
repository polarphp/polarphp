# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2019 polarphp software foundation
# Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2018/11/07.

set(generated_include_sources
    SyntaxKindEnumDefs.h.gyb
    SyntaxNodes.h.gyb
    SyntaxNodeBuilders.h.gyb
    SyntaxNodeFactory.h.gyb
    SyntaxNodeVisitor.h.gyb
    Trivia.h.gyb
    internal/TokenEnumDefs.h.gyb
    serialization/TokenKindTypeSerialization.h.gyb)

add_gyb_target(polar-syntax-generated-headers
     "${generated_include_sources}"
     )

add_dependencies(polar-syntax-generated-headers llvm-project)

set_property(TARGET polar-syntax-generated-headers
     PROPERTY FOLDER "Miscellaneous")
