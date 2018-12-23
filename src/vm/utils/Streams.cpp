// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/23.

#include "polarphp/vm/ZendApi.h"
#include "polarphp/vm/utils/StreamBuffer.h"
#include "polarphp/vm/internal/DepsZendVmHeaders.h"

namespace polar {
namespace vmapi {

static thread_local StreamBuffer sg_bufOut        (0);
static thread_local StreamBuffer sg_bufError      (E_ERROR);
static thread_local StreamBuffer sg_bufWarning    (E_WARNING);
static thread_local StreamBuffer sg_bufNotice     (E_NOTICE);
static thread_local StreamBuffer sg_bufDeprecated (E_DEPRECATED);

thread_local std::ostream out               (&sg_bufOut);
thread_local std::ostream error             (&sg_bufError);
thread_local std::ostream warning           (&sg_bufWarning);
thread_local std::ostream notice            (&sg_bufNotice);
thread_local std::ostream deprecated        (&sg_bufDeprecated);

} // vmapi
} // polar
