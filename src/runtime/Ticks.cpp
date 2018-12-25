// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/13.

#include "polarphp/runtime/Ticks.h"
#include "polarphp/runtime/ExecEnv.h"
#include "polarphp/runtime/internal/DepsZendVmHeaders.h"

namespace polar {
namespace runtime {

struct st_tick_function
{
   void (*func)(int, void *);
   void *arg;
};

namespace {
int compare_tick_functions(void *elem1, void *elem2)
{
   st_tick_function *e1 = reinterpret_cast<st_tick_function *>(elem1);
   st_tick_function *e2 = reinterpret_cast<st_tick_function *>(elem2);
   return e1->func == e2->func && e1->arg == e2->arg;
}

void tick_iterator(void *d, void *arg)
{
   st_tick_function *data = (st_tick_function *)d;
   data->func(*((int *)arg), data->arg);
}
} // anonymous namespace

bool startup_ticks(void)
{
   ExecEnvInfo &execEnvInfo = retrieve_global_execenv_runtime_info();
   zend_llist &tickFunctions = execEnvInfo.tickFunctions;
   zend_llist_init(&tickFunctions, sizeof(st_tick_function), nullptr, 1);
   return true;
}

void deactivate_ticks(void)
{
   ExecEnvInfo &execEnvInfo = retrieve_global_execenv_runtime_info();
   zend_llist &tickFunctions =  execEnvInfo.tickFunctions;
   zend_llist_clean(&tickFunctions);
}

void shutdown_ticks(void)
{
   ExecEnvInfo &execEnvInfo = retrieve_global_execenv_runtime_info();
   zend_llist &tickFunctions =  execEnvInfo.tickFunctions;
   zend_llist_destroy(&tickFunctions);
}

void add_tick_function(void (*func)(int, void*), void * arg)
{
   ExecEnvInfo &execEnvInfo = retrieve_global_execenv_runtime_info();
   zend_llist &tickFunctions =  execEnvInfo.tickFunctions;
   st_tick_function tmp = {func, arg};
   zend_llist_add_element(&tickFunctions, (void *)&tmp);
}

void remove_tick_function(void (*func)(int, void *), void * arg)
{
   ExecEnvInfo &execEnvInfo = retrieve_global_execenv_runtime_info();
   zend_llist &tickFunctions =  execEnvInfo.tickFunctions;
   st_tick_function tmp = {func, arg};
   zend_llist_del_element(&tickFunctions, (void *)&tmp, (int(*)(void*, void*))compare_tick_functions);
}

void run_ticks(int count)
{
   ExecEnvInfo &execEnvInfo = retrieve_global_execenv_runtime_info();
   zend_llist &tickFunctions =  execEnvInfo.tickFunctions;
   zend_llist_apply_with_argument(&tickFunctions, (llist_apply_with_arg_func_t) tick_iterator, &count);
}

} // runtime
} // polar
