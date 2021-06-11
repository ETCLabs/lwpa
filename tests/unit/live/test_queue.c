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

#include "etcpal/queue.h"

#include <stdint.h>
#include "etcpal/timer.h"
#include "unity_fixture.h"

TEST_GROUP(etcpal_queue);

TEST_SETUP(etcpal_queue)
{
}

TEST_TEAR_DOWN(etcpal_queue)
{
}

TEST(etcpal_queue, can_send_and_receive)
{
  etcpal_queue_t queue;

  // Create queue for 10 chars
  TEST_ASSERT_TRUE(etcpal_queue_create(&queue, 10, sizeof(uint8_t)));
  uint8_t data = 0xDE;
  TEST_ASSERT_TRUE(etcpal_queue_send(&queue, &data));
  uint8_t received_data = 0;
  TEST_ASSERT_TRUE(etcpal_queue_receive(&queue, &received_data));
  TEST_ASSERT_EQUAL(data, received_data);
  etcpal_queue_destroy(&queue);
}

TEST(etcpal_queue, will_timeout_on_send)
{
  etcpal_queue_t queue;

  // Create queue for 3 chars
  TEST_ASSERT_TRUE(etcpal_queue_create(&queue, 3, sizeof(uint8_t)));
  uint8_t data = 0xDE;
  TEST_ASSERT_TRUE(etcpal_queue_timed_send(&queue, &data, 0));
  data = 0xAD;
  TEST_ASSERT_TRUE(etcpal_queue_timed_send(&queue, &data, 0));
  data = 0xBE;
  TEST_ASSERT_TRUE(etcpal_queue_timed_send(&queue, &data, 0));

#if ETCPAL_QUEUE_HAS_TIMED_FUNCTIONS
  EtcPalTimer timer;
  etcpal_timer_start(&timer, 100);

  // This one should NOT work because we are over our size
  data = 0xEF;
  TEST_ASSERT_FALSE(etcpal_queue_timed_send(&queue, &data, 10));

  // An unfortunately necessary heuristic - we assert that at least half the specified time has
  // gone by, to account for OS slop.
  TEST_ASSERT_GREATER_THAN_UINT32(5, etcpal_timer_elapsed(&timer));
#else
  // This one should NOT work because we are over our size
  data = 0xEF;
  TEST_ASSERT_FALSE(etcpal_queue_timed_send(&queue, &data, 0));
#endif

  etcpal_queue_destroy(&queue);
}

TEST(etcpal_queue, will_timeout_on_receive)
{
  etcpal_queue_t queue;

  // Create queue for 3 chars
  TEST_ASSERT_TRUE(etcpal_queue_create(&queue, 3, sizeof(uint8_t)));
  uint8_t data = 0xDE;
  TEST_ASSERT_TRUE(etcpal_queue_timed_send(&queue, &data, 0));
  uint8_t received_data = 0x00;
  TEST_ASSERT_TRUE(etcpal_queue_timed_receive(&queue, &received_data, 10));

#if ETCPAL_QUEUE_HAS_TIMED_FUNCTIONS
  EtcPalTimer timer;
  etcpal_timer_start(&timer, 100);

  TEST_ASSERT_FALSE(etcpal_queue_timed_receive(&queue, &received_data, 10));

  // An unfortunately necessary heuristic - we assert that at least half the specified time has
  // gone by, to account for OS slop.
  TEST_ASSERT_GREATER_THAN_UINT32(5, etcpal_timer_elapsed(&timer));
#else
  TEST_ASSERT_FALSE(etcpal_queue_timed_receive(&queue, &received_data, 0));
#endif

  etcpal_queue_destroy(&queue);
}

TEST(etcpal_queue, can_detect_empty)
{
  etcpal_queue_t queue;

  // Create queue for 3 chars
  TEST_ASSERT_TRUE(etcpal_queue_create(&queue, 4, sizeof(uint8_t)));
  TEST_ASSERT_TRUE(etcpal_queue_is_empty(&queue));

  uint8_t data = 0xDE;
  TEST_ASSERT_TRUE(etcpal_queue_timed_send(&queue, &data, 0));
  TEST_ASSERT_FALSE(etcpal_queue_is_empty(&queue));

  data = 0xAD;
  TEST_ASSERT_TRUE(etcpal_queue_timed_receive(&queue, &data, 0));
  TEST_ASSERT_TRUE(etcpal_queue_is_empty(&queue));

  TEST_ASSERT_TRUE(etcpal_queue_timed_send(&queue, &data, 0));
  TEST_ASSERT_FALSE(etcpal_queue_is_empty(&queue));

  etcpal_queue_destroy(&queue);
}

TEST_GROUP_RUNNER(etcpal_queue)
{
  RUN_TEST_CASE(etcpal_queue, can_send_and_receive);
  RUN_TEST_CASE(etcpal_queue, will_timeout_on_send);
  RUN_TEST_CASE(etcpal_queue, will_timeout_on_receive);
  RUN_TEST_CASE(etcpal_queue, can_detect_empty);
}
