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

/*
 * A platform-neutral example application showing usage of the etcpal_thread C++ wrappers. Starts a
 * few threads that loop at different rates, then waits for them all to finish.
 */

#include <chrono>
#include <functional>
#include <iostream>
#include "etcpal/cpp/thread.h"

using namespace std::chrono_literals;

// Iterates once and exits.
void Thread1Func(int& value)
{
  std::cout << "Thread 1 executing\n";
  ++value;
}

// Iterates 5 times, sleeping for half a second between each iteration.
void Thread2Func(int& value)
{
  for (int i = 0; i < 5; ++i)
  {
    std::cout << "Thread 2 executing\n";
    ++value;
    etcpal::Thread::Sleep(500ms);
  }
}

// Thread 3 uses a lambda below

int main()
{
  // No initialization is necessary for the thread module.

  int thread_1_count = 0;
  int thread_2_count = 0;
  int thread_3_count = 0;

  // You can start a thread using the value constructor...
  etcpal::Thread thread_1(Thread1Func, std::ref(thread_1_count));
  // This throws etcpal::Error if the thread fails to start

  // Or default construct and call start
  etcpal::Thread thread_2;
  auto           start_res = thread_2.SetName("Thread 2").Start(Thread2Func, std::ref(thread_2_count));
  // This returns a result in the form of an etcpal::Error
  if (!start_res)
  {
    std::cout << "Error starting thread 2: " << start_res.ToCString() << '\n';
    return 1;
  }

  // You can also use a lambda as the thread function
  etcpal::Thread thread_3([&]() {
    for (int i = 0; i < 10; ++i)
    {
      std::cout << "Thread 3 executing\n";
      ++thread_3_count;
      etcpal::Thread::Sleep(250ms);
    }
  });

  // Wait for all threads to finish
  thread_1.Join();
  thread_2.Join();
  thread_3.Join();

  // Print results
  std::cout << "Thread 1 ran " << thread_1_count << " times.\n";
  std::cout << "Thread 2 ran " << thread_2_count << " times.\n";
  std::cout << "Thread 3 ran " << thread_3_count << " times.\n";

  return 0;
}
