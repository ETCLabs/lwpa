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

#include "etcpal/timer.h"
#include "unity_fixture.h"

// Interdependency is unavoidable here, we need a platform-neutral sleep
#include "etcpal/common.h"
#include "etcpal/thread.h"

TEST_GROUP(etcpal_timer);

TEST_SETUP(etcpal_timer)
{
  TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_init(ETCPAL_FEATURE_TIMERS));
}

TEST_TEAR_DOWN(etcpal_timer)
{
  etcpal_deinit(ETCPAL_FEATURE_TIMERS);
}

TEST(etcpal_timer, getms_gets_increasing_values)
{
  uint32_t t1 = etcpal_getms();
  etcpal_thread_sleep(10);
  uint32_t t2 = etcpal_getms();

  TEST_ASSERT_NOT_EQUAL(t1, 0u);
  TEST_ASSERT_NOT_EQUAL(t2, 0u);
  TEST_ASSERT_GREATER_OR_EQUAL_INT32(0, (int32_t)t2 - (int32_t)t1);
}

TEST(etcpal_timer, elapsed_since_works)
{
  uint32_t start_point = etcpal_getms();
  etcpal_thread_sleep(110);
  TEST_ASSERT_GREATER_OR_EQUAL_UINT32(100u, ETCPAL_TIME_ELAPSED_SINCE(start_point));
}

TEST(etcpal_timer, timers_report_expired_properly)
{
  EtcPalTimer t1, t2;

  etcpal_timer_start(&t1, 0);
  etcpal_timer_start(&t2, 100);

  // A timer with a timeout of 0 should start expired.
  TEST_ASSERT(etcpal_timer_is_expired(&t1));

  // The nonzero timeout should not be expired yet.
  TEST_ASSERT_UNLESS(etcpal_timer_is_expired(&t2));

  etcpal_thread_sleep(110);

  // Now it should.
  TEST_ASSERT(etcpal_timer_is_expired(&t2));
  TEST_ASSERT_GREATER_OR_EQUAL_UINT32(100u, etcpal_timer_elapsed(&t2));

  // Test resetting the timer.
  etcpal_timer_reset(&t2);
  TEST_ASSERT_UNLESS(etcpal_timer_is_expired(&t2));

  // And test the timeout one more time.
  etcpal_thread_sleep(110);
  TEST_ASSERT(etcpal_timer_is_expired(&t2));
  TEST_ASSERT_GREATER_OR_EQUAL_UINT32(100u, etcpal_timer_elapsed(&t2));
}

TEST_GROUP_RUNNER(etcpal_timer)
{
  RUN_TEST_CASE(etcpal_timer, getms_gets_increasing_values);
  RUN_TEST_CASE(etcpal_timer, elapsed_since_works);
  RUN_TEST_CASE(etcpal_timer, timers_report_expired_properly);
}
