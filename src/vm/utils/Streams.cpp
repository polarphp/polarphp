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

std::ostream &out()
{
   thread_local std::ostream outStream(&sg_bufOut);
   return outStream;
}

std::ostream &error()
{
   thread_local std::ostream errorStream(&sg_bufError);
   return errorStream;
}

std::ostream &warning()
{
   thread_local std::ostream warningStream(&sg_bufWarning);
   return warningStream;
}

std::ostream &notice()
{
   thread_local std::ostream noticeStream(&sg_bufNotice);
   return noticeStream;
}

std::ostream &deprecated()
{
   thread_local std::ostream deprecatedStream(&sg_bufDeprecated);
   return deprecatedStream;
}

} // vmapi
} // polar
