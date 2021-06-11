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
#include "etcpal/cpp/timer.h"
#include "unity_fixture.h"
#include "etc_fff_wrapper.h"

extern "C" {

ETC_FAKE_VALUE_FUNC(uint32_t, etcpal_getms);

TEST_GROUP(timer_controlled);

TEST_SETUP(timer_controlled)
{
  RESET_FAKE(etcpal_getms);
}

TEST_TEAR_DOWN(timer_controlled)
{
}

TEST(timer_controlled, timer_wraparound_works_as_expected)
{
  EtcPalTimer t1;

  // Test the wraparound case by forcing a wraparound value returned from etcpal_getms()
  etcpal_getms_fake.return_val = 0xfffffff0u;
  etcpal_timer_start(&t1, 0x20);

  // We've wrapped around but have not exceeded the interval yet
  etcpal_getms_fake.return_val = 0x0f;
  TEST_ASSERT_FALSE(etcpal_timer_is_expired(&t1));
  TEST_ASSERT_EQUAL_UINT32(etcpal_timer_elapsed(&t1), 0x1fu);

  etcpal_getms_fake.return_val = 0x11;
  TEST_ASSERT_TRUE(etcpal_timer_is_expired(&t1));
}

TEST(timer_controlled, elapsed_since_wraparound_works_as_expected)
{
  uint32_t start_time = 0xfffffff0u;
  etcpal_getms_fake.return_val = 0x10;
  TEST_ASSERT_EQUAL_UINT32(ETCPAL_TIME_ELAPSED_SINCE(start_time), 0x20u);
}

TEST(timer_controlled, remaining_works_as_expected)
{
  EtcPalTimer t1;

  etcpal_getms_fake.return_val = 20;
  etcpal_timer_start(&t1, 10);
  TEST_ASSERT_EQUAL_UINT32(etcpal_timer_remaining(&t1), 10u);

  // Almost expired
  etcpal_getms_fake.return_val = 29;
  TEST_ASSERT_EQUAL_UINT32(etcpal_timer_remaining(&t1), 1u);

  // 1 ms past expired
  etcpal_getms_fake.return_val = 31;
  TEST_ASSERT_EQUAL_UINT32(etcpal_timer_remaining(&t1), 0u);

  // Almost wrapped the original time point, remaining should still be 0.
  etcpal_getms_fake.return_val = 19;
  TEST_ASSERT_EQUAL_UINT32(etcpal_timer_remaining(&t1), 0u);
}

TEST(timer_controlled, time_point_now_works)
{
  etcpal_getms_fake.return_val = 0;
  auto tp = etcpal::TimePoint::Now();
  TEST_ASSERT_EQUAL_UINT32(tp.value(), 0u);

  etcpal_getms_fake.return_val = 0xffffffffu;
  tp = etcpal::TimePoint::Now();
  TEST_ASSERT_EQUAL_UINT32(tp.value(), 0xffffffffu);
}

TEST_GROUP_RUNNER(timer_controlled)
{
  RUN_TEST_CASE(timer_controlled, timer_wraparound_works_as_expected);
  RUN_TEST_CASE(timer_controlled, elapsed_since_wraparound_works_as_expected);
  RUN_TEST_CASE(timer_controlled, remaining_works_as_expected);
  RUN_TEST_CASE(timer_controlled, time_point_now_works);
}
}
