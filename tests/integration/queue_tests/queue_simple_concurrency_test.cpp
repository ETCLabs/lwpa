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

#include "queue_tests.h"

#include <array>
#include <cstdio>
#include <functional>
#include <unordered_map>
#include <vector>
#include "etcpal/cpp/thread.h"
#include "etcpal/sem.h"

static const unsigned kLargePrime = 115249;

static bool writer_failure = false;
static bool reader_failure = false;

static unsigned num_writes = 0;
static unsigned num_reads = 0;

std::unordered_map<long long int, unsigned> val_occurrences_map;

etcpal_sem_t occ_map_lock;
void         init_concurrency_test()
{
  etcpal_sem_create(&occ_map_lock, 1, 1);
}

static void register_value_received(long long int val)
{
  etcpal_sem_wait(&occ_map_lock);

  if (val_occurrences_map.find(val) == val_occurrences_map.end())
  {
    val_occurrences_map[val] = 0;
  }
  val_occurrences_map[val]++;

  etcpal_sem_post(&occ_map_lock);
}

static bool check_occurrences(unsigned num_expected_occurrences)
{
  bool true_if_success = true;
  etcpal_sem_wait(&occ_map_lock);
  for (std::pair<long long int, unsigned> val_occ_pair : val_occurrences_map)
  {
    if (val_occ_pair.second != num_expected_occurrences)
    {
      true_if_success = false;
      printf("\r\n\tERROR: The value %lld occurred %d times, expected %d occurrences", val_occ_pair.first,
             val_occ_pair.second, num_expected_occurrences);
    }
  }
  if (!true_if_success)
  {
    printf("\r\n");
  }
  etcpal_sem_post(&occ_map_lock);
  return true_if_success;
}

static void reset_occurrence_counter()
{
  etcpal_sem_wait(&occ_map_lock);
  val_occurrences_map.clear();
  etcpal_sem_post(&occ_map_lock);
}

static void writer_func(etcpal::Queue<long long int>& queue)
{
  for (unsigned i = 0; i < num_writes; i++)
  {
    if (!queue.Send((i + 1) * kLargePrime))
    {
      writer_failure = true;
      break;
    }
  }
}

static void reader_func(etcpal::Queue<long long int>& queue)
{
  for (unsigned i = 0; i < num_reads; i++)
  {
    long long int val = 0;
    if (queue.Receive(val))
    {
      register_value_received(val);
      if (val == 0 || val % kLargePrime != 0)
      {
        reader_failure = true;
        break;
      }
    }
  }
}

std::vector<etcpal::Thread> writer_threads;
std::vector<etcpal::Thread> reader_threads;

static void create_writer_test(etcpal::Queue<long long int>& q, int num_writers)
{
  writer_threads.reserve(num_writers);
  std::array<char, ETCPAL_THREAD_NAME_MAX_LENGTH> thread_name{};

  for (int i = 0; i < num_writers; i++)
  {
    sprintf(thread_name.data(), "Writer %d", i);

    etcpal::Thread thread;
    thread.SetPriority(1).SetStackSize(0x5000).SetName(thread_name.data()).Start(writer_func, std::ref(q));
    writer_threads.push_back(std::move(thread));
  }
}

static void create_reader_test(etcpal::Queue<long long int>& q, int num_readers)
{
  reader_threads.reserve(num_readers);
  std::array<char, ETCPAL_THREAD_NAME_MAX_LENGTH> thread_name{};

  for (int i = 0; i < num_readers; i++)
  {
    sprintf(thread_name.data(), "Reader %d", i);

    etcpal::Thread thread;
    thread.SetPriority(1).SetStackSize(0x5000).SetName(thread_name.data()).Start(reader_func, std::ref(q));
    reader_threads.push_back(std::move(thread));
  }
}

static void wait_for_writer_complete(int num_threads)
{
  for (auto& thread : writer_threads)
    thread.Join();
}

static void wait_for_reader_complete(int num_threads)
{
  for (auto& thread : reader_threads)
    thread.Join();
}

static void cleanup_writers()
{
  writer_threads.clear();
}

static void cleanup_readers()
{
  reader_threads.clear();
}

bool concurrent_small_queue_few_writers()
{
  writer_failure = false;

  // Each of 8 thread writes 1 element, to fill a queue with 8 slots
  int                          num_threads = 8;
  etcpal::Queue<long long int> q(8);
  num_writes = 1;

  create_writer_test(q, num_threads);
  wait_for_writer_complete(num_threads);
  cleanup_writers();
  return !writer_failure;
}

bool concurrent_large_queue_few_writers_many_items()
{
  writer_failure = false;

  // Each of 8 threads writes 10000 elements, to fill a queue with 80000 elements
  num_writes = 10000;
  int                          num_threads = 8;
  etcpal::Queue<long long int> q(80000);

  create_writer_test(q, num_threads);
  wait_for_writer_complete(num_threads);
  cleanup_writers();
  return !writer_failure;
}

bool concurrent_large_queue_many_writers_few_items()
{
  writer_failure = false;

  // Each of 100 threads writes 800 elements, to fill a queue with 80000 elements
  num_writes = 800;
  int                          num_threads = 100;
  etcpal::Queue<long long int> q(80000);

  create_writer_test(q, num_threads);
  wait_for_writer_complete(num_threads);
  cleanup_writers();
  return !writer_failure;
}

bool concurrent_small_queue_one_reader_few_writers_few_items()
{
  bool occurrence_fail = false;

  writer_failure = false;
  reader_failure = false;

  // Each of 8 writers writes 8 elements
  // Meanwhile, Each of 1 readers reads 64 elements
  // Queue only has 8 elements

  num_writes = 8;
  num_reads = 64;
  int                          num_writers = 8;
  int                          num_readers = 1;
  etcpal::Queue<long long int> q(8);

  create_writer_test(q, num_writers);
  create_reader_test(q, num_readers);

  wait_for_writer_complete(num_writers);
  wait_for_reader_complete(num_readers);

  occurrence_fail = !check_occurrences(num_writers);
  reset_occurrence_counter();

  cleanup_writers();
  cleanup_readers();

  return !(writer_failure || reader_failure || occurrence_fail);
}

bool concurrent_small_queue_few_readers_few_writers_few_items()
{
  bool occurrence_fail = false;

  writer_failure = false;
  reader_failure = false;

  // Each of 8 writers writes 8 elements
  // Meanwhile, Each of 8 readers reads 8 elements
  // Queue only has 8 elements

  num_writes = 8;
  num_reads = 8;
  int                          num_writers = 8;
  int                          num_readers = 8;
  etcpal::Queue<long long int> q(8);

  create_writer_test(q, num_writers);
  create_reader_test(q, num_readers);

  wait_for_writer_complete(num_writers);
  wait_for_reader_complete(num_readers);

  occurrence_fail = !check_occurrences(num_writers);
  reset_occurrence_counter();

  cleanup_writers();
  cleanup_readers();

  return !(writer_failure || reader_failure || occurrence_fail);
}

bool concurrent_small_queue_few_readers_many_writers_many_items()
{
  bool occurrence_fail = false;

  writer_failure = false;
  reader_failure = false;

  // Each of 100 writers writes 800 elements
  // Meanwhile, Each of 8 readers reads 10000 elements
  // Queue only has 8 elements

  num_writes = 800;
  num_reads = 10000;
  int                          num_writers = 100;
  int                          num_readers = 8;
  etcpal::Queue<long long int> q(8);

  create_writer_test(q, num_writers);
  create_reader_test(q, num_readers);

  wait_for_writer_complete(num_writers);
  wait_for_reader_complete(num_readers);

  occurrence_fail = !check_occurrences(num_writers);
  reset_occurrence_counter();

  cleanup_writers();
  cleanup_readers();

  return !(writer_failure || reader_failure || occurrence_fail);
}

bool concurrent_small_queue_many_readers_few_writers_many_items()
{
  bool occurrence_fail = false;

  writer_failure = false;
  reader_failure = false;

  // Each of 8 writers writes 10000 elements
  // Meanwhile, Each of 100 readers reads 800 elements
  // Queue only has 8 elements

  num_writes = 10000;
  num_reads = 800;
  int                          num_writers = 8;
  int                          num_readers = 100;
  etcpal::Queue<long long int> q(8);

  create_writer_test(q, num_writers);
  create_reader_test(q, num_readers);

  wait_for_writer_complete(num_writers);
  wait_for_reader_complete(num_readers);

  occurrence_fail = !check_occurrences(num_writers);
  reset_occurrence_counter();

  cleanup_writers();
  cleanup_readers();

  return !(writer_failure || reader_failure || occurrence_fail);
}
