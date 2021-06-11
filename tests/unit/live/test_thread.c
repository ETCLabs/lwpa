/******************************************************************************
 * Copyright 2021 ETC Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *******************************************************************************
 * This file is a part of EtcPal. For more information, go to:
 * https://github.com/ETCLabs/EtcPal
 ******************************************************************************/

#include "etcpal/thread.h"

#include <stdbool.h>

#include "etcpal/common.h"
#include "etcpal/timer.h"
#include "unity_fixture.h"

TEST_GROUP(etcpal_thread);

TEST_SETUP(etcpal_thread)
{
}

TEST_TEAR_DOWN(etcpal_thread)
{
}

volatile bool waitthread_run;

void wait_and_exit(void* param)
{
  ETCPAL_UNUSED_ARG(param);

  while (waitthread_run)
    etcpal_thread_sleep(5);
}

// Basic test of the three thread functions.
TEST(etcpal_thread, create_and_destroy_functions_work)
{
  EtcPalThreadParams params = ETCPAL_THREAD_PARAMS_INIT;
  etcpal_thread_t    wait_thread;

  waitthread_run = true;
  TEST_ASSERT_EQUAL(etcpal_thread_create(&wait_thread, &params, wait_and_exit, NULL), kEtcPalErrOk);

  // Join should work if the thread has exited.
  waitthread_run = false;
  TEST_ASSERT_EQUAL(etcpal_thread_join(&wait_thread), kEtcPalErrOk);
}

#if ETCPAL_THREAD_HAS_TIMED_JOIN
TEST(etcpal_thread, timed_join_works)
{
  EtcPalThreadParams params = ETCPAL_THREAD_PARAMS_INIT;
  etcpal_thread_t    wait_thread;

  waitthread_run = true;
  TEST_ASSERT_EQUAL(etcpal_thread_create(&wait_thread, &params, wait_and_exit, NULL), kEtcPalErrOk);

  EtcPalTimer timer;
  etcpal_timer_start(&timer, 100);
  TEST_ASSERT_EQUAL(etcpal_thread_timed_join(&wait_thread, 10), kEtcPalErrTimedOut);

  // An unfortunately necessary heuristic - we assert that at least half the specified time has
  // gone by, to account for OS slop.
  TEST_ASSERT_GREATER_THAN_UINT32(5, etcpal_timer_elapsed(&timer));

  // Timed join should work if the thread has exited.
  waitthread_run = false;
  TEST_ASSERT_EQUAL(etcpal_thread_timed_join(&wait_thread, 100), kEtcPalErrOk);
}
#endif

volatile bool spin_task_run;
volatile bool spin_task_ran;

void increment_and_spin(void* param)
{
  ETCPAL_UNUSED_ARG(param);

  spin_task_ran = true;
  while (spin_task_run)
    ;
}

volatile bool oneshot_task_run;
volatile bool oneshot_task_ran;

void oneshot(void* param)
{
  ETCPAL_UNUSED_ARG(param);

  if (oneshot_task_run)
    oneshot_task_ran = true;
}

// Test time slicing. One thread spins constantly; if time slicing works, the second thread should
// get a chance to set its flag. Failure mode could be the flag not being set or a full test timeout.
TEST(etcpal_thread, threads_are_time_sliced)
{
  EtcPalThreadParams params = {
      ETCPAL_THREAD_DEFAULT_PRIORITY,  // priority
      ETCPAL_THREAD_DEFAULT_STACK,     // stack_size
      "ts_thread",                     // thread_name
      NULL                             // platform_data
  };

  // MQX requires explicit setup to time-slice tasks
#ifdef __MQX__
  EtcPalThreadParamsMqx mqx_params;
  mqx_params.task_attributes = MQX_TIME_SLICE_TASK;
  mqx_params.time_slice = 0;
  params.platform_data = &mqx_params;
#endif

  etcpal_thread_t spin_task;
  spin_task_run = true;
  etcpal_thread_t oneshot_task;
  oneshot_task_run = true;

  // Create the spin task.
  TEST_ASSERT_EQUAL(etcpal_thread_create(&spin_task, &params, increment_and_spin, NULL), kEtcPalErrOk);
  // Create the oneshot task.
  TEST_ASSERT_EQUAL(etcpal_thread_create(&oneshot_task, &params, oneshot, NULL), kEtcPalErrOk);
  // Give both tasks time to run.
  etcpal_thread_sleep(100);
  // Stop the tasks
  oneshot_task_run = false;
  TEST_ASSERT_EQUAL(etcpal_thread_join(&oneshot_task), kEtcPalErrOk);
  spin_task_run = false;
  TEST_ASSERT_EQUAL(etcpal_thread_join(&spin_task), kEtcPalErrOk);
  TEST_ASSERT_TRUE(spin_task_ran);
  TEST_ASSERT_TRUE(oneshot_task_ran);
}

etcpal_thread_os_handle_t thread_handle;

void save_thread_handle(void* param)
{
  ETCPAL_UNUSED_ARG(param);
  thread_handle = etcpal_thread_get_current_os_handle();
}

TEST(etcpal_thread, get_os_handle_works)
{
  thread_handle = ETCPAL_THREAD_OS_HANDLE_INVALID;
  EtcPalThreadParams params = ETCPAL_THREAD_PARAMS_INIT;

  etcpal_thread_t save_handle_thread;
  TEST_ASSERT_EQUAL(etcpal_thread_create(&save_handle_thread, &params, save_thread_handle, NULL), kEtcPalErrOk);
  etcpal_thread_os_handle_t reported_handle = etcpal_thread_get_os_handle(&save_handle_thread);
  etcpal_thread_join(&save_handle_thread);

  // Using == comparison here instead of TEST_ASSERT_EQUAL because of platform-defined type
  TEST_ASSERT_TRUE(thread_handle == reported_handle);
}

TEST_GROUP_RUNNER(etcpal_thread)
{
  RUN_TEST_CASE(etcpal_thread, create_and_destroy_functions_work);
#if ETCPAL_THREAD_HAS_TIMED_JOIN
  RUN_TEST_CASE(etcpal_thread, timed_join_works);
#endif
  RUN_TEST_CASE(etcpal_thread, threads_are_time_sliced);
  RUN_TEST_CASE(etcpal_thread, get_os_handle_works);
}
