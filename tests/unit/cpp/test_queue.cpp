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

#include "etcpal/cpp/queue.h"

#include "unity_fixture.h"

extern "C" {

TEST_GROUP(etcpal_cpp_queue);

TEST_SETUP(etcpal_cpp_queue)
{
}

TEST_TEAR_DOWN(etcpal_cpp_queue)
{
}

TEST(etcpal_cpp_queue, check_empty)
{
  etcpal::Queue<unsigned char> q(5);
  TEST_ASSERT_TRUE(q.IsEmpty());
}

TEST(etcpal_cpp_queue, can_send_and_receive)
{
  etcpal::Queue<unsigned char> q(3);
  unsigned char                data = 0xDE;
  TEST_ASSERT_TRUE(q.Send(data));
  unsigned char received_data = 0;
  TEST_ASSERT_TRUE(q.Receive(received_data));
  TEST_ASSERT_EQUAL(data, received_data);
}

TEST(etcpal_cpp_queue, will_timeout_on_send)
{
  // Create queue for 3 chars
  etcpal::Queue<unsigned char> q(3);
  unsigned char                data = 0xDE;
  TEST_ASSERT_TRUE(q.Send(data));
  data = 0xAD;
  TEST_ASSERT_TRUE(q.Send(data));
  data = 0xBE;
  TEST_ASSERT_TRUE(q.Send(data));

  // This one should NOT work because we are over our size
  data = 0xEF;
#if ETCPAL_QUEUE_HAS_TIMED_FUNCTIONS
  TEST_ASSERT_FALSE(q.Send(data, 10));
#else
  TEST_ASSERT_FALSE(q.Send(data, 0));
#endif
}

TEST(etcpal_cpp_queue, will_timeout_on_receive)
{
  // Create queue for 3 chars
  etcpal::Queue<unsigned char> q(3);
  unsigned char                data = 0xDE;
  TEST_ASSERT_TRUE(q.Send(data));
  unsigned char received_data = 0x00;
  TEST_ASSERT_TRUE(q.Receive(received_data, 10));
#if ETCPAL_QUEUE_HAS_TIMED_FUNCTIONS
  TEST_ASSERT_FALSE(q.Receive(received_data, 10));
#else
  TEST_ASSERT_FALSE(q.Receive(received_data, 0));
#endif
}

TEST(etcpal_cpp_queue, can_detect_empty)
{
  // Create queue for 3 chars
  etcpal::Queue<unsigned char> q(3);
  TEST_ASSERT_TRUE(q.IsEmpty());

  unsigned char data = 0xDE;
  TEST_ASSERT_TRUE(q.Send(data));
  TEST_ASSERT_FALSE(q.IsEmpty());

  unsigned char received_data = 0x00;
  TEST_ASSERT_TRUE(q.Receive(received_data));
  TEST_ASSERT_TRUE(q.IsEmpty());

  TEST_ASSERT_TRUE(q.Send(data));
  TEST_ASSERT_FALSE(q.IsEmpty());
}

TEST(etcpal_cpp_queue, can_detect_reset)
{
  // Create queue for 3 chars
  etcpal::Queue<unsigned char> q(3);
  TEST_ASSERT_TRUE(q.IsEmpty());

  unsigned char data = 0xDE;
  TEST_ASSERT_TRUE(q.Send(data));
  TEST_ASSERT_FALSE(q.IsEmpty());

  TEST_ASSERT_TRUE(q.Reset());
  TEST_ASSERT_TRUE(q.IsEmpty());
}

TEST(etcpal_cpp_queue, can_detect_full)
{
  // Create queue for 3 chars
  etcpal::Queue<unsigned char> q(3);
  TEST_ASSERT_FALSE(q.IsFull());

  unsigned char data = 0xDE;
  TEST_ASSERT_TRUE(q.Send(data));
  TEST_ASSERT_TRUE(q.Send(data));
  TEST_ASSERT_TRUE(q.Send(data));
  TEST_ASSERT_TRUE(q.IsFull());

  unsigned char received_data = 0x00;
  TEST_ASSERT_TRUE(q.Receive(received_data));
  TEST_ASSERT_FALSE(q.IsFull());

  TEST_ASSERT_TRUE(q.Send(data));
  TEST_ASSERT_TRUE(q.IsFull());
}

TEST(etcpal_cpp_queue, can_detect_slots_used)
{
  // Create queue for 3 chars
  etcpal::Queue<unsigned char> q(3);
  TEST_ASSERT_TRUE(q.SlotsUsed() == 0);

  unsigned char data = 0xDE;
  TEST_ASSERT_TRUE(q.Send(data));
  TEST_ASSERT_TRUE(q.SlotsUsed() == 1);

  unsigned char received_data = 0x00;
  TEST_ASSERT_TRUE(q.Receive(received_data));
  TEST_ASSERT_TRUE(q.SlotsUsed() == 0);

  TEST_ASSERT_TRUE(q.Send(data));
  TEST_ASSERT_TRUE(q.Send(data));
  TEST_ASSERT_TRUE(q.Send(data));
  TEST_ASSERT_TRUE(q.SlotsUsed() == 3);
}

TEST(etcpal_cpp_queue, can_detect_slots_available)
{
  // Create queue for 3 chars
  etcpal::Queue<unsigned char> q(3);
  TEST_ASSERT_TRUE(q.SlotsAvailable() == 3);

  unsigned char data = 0xDE;
  TEST_ASSERT_TRUE(q.Send(data));
  TEST_ASSERT_TRUE(q.SlotsAvailable() == 2);
  TEST_ASSERT_TRUE(q.Send(data));
  TEST_ASSERT_TRUE(q.SlotsAvailable() == 1);
  TEST_ASSERT_TRUE(q.Send(data));
  TEST_ASSERT_TRUE(q.SlotsAvailable() == 0);

  unsigned char received_data = 0x00;
  TEST_ASSERT_TRUE(q.Receive(received_data));
  TEST_ASSERT_TRUE(q.SlotsAvailable() == 1);
}

TEST_GROUP_RUNNER(etcpal_cpp_queue)
{
  RUN_TEST_CASE(etcpal_cpp_queue, can_send_and_receive);
  RUN_TEST_CASE(etcpal_cpp_queue, will_timeout_on_send);
  RUN_TEST_CASE(etcpal_cpp_queue, will_timeout_on_receive);
  RUN_TEST_CASE(etcpal_cpp_queue, can_detect_empty);
  RUN_TEST_CASE(etcpal_cpp_queue, can_detect_reset);
  RUN_TEST_CASE(etcpal_cpp_queue, can_detect_full);
  RUN_TEST_CASE(etcpal_cpp_queue, can_detect_slots_used);
  RUN_TEST_CASE(etcpal_cpp_queue, can_detect_slots_available);
}

}  // extern "C"
