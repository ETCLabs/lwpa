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
 * A platform-neutral example application showing usage of the etcpal_timer C++ wrappers.
 */

#include <chrono>
#include <iostream>
#include "etcpal/common.h"
#include "etcpal/cpp/error.h"
#include "etcpal/cpp/thread.h"
#include "etcpal/cpp/timer.h"

using namespace std::chrono_literals;

int main()
{
  // The timer module requires explicit initialization
  etcpal::Error res = etcpal_init(ETCPAL_FEATURE_TIMERS);
  if (!res)
  {
    std::cout << "Couldn't initialize EtcPal: '" << res.ToCString() << "'\n";
    return 1;
  }

  // Do some busy work until a 1-second timer is expired
  unsigned long i = 0;
  std::cout << "Seeing how many times we can increment an integer in one second...\n";
  etcpal::Timer timer(1s);
  while (!timer.IsExpired())
  {
    ++i;
  }
  std::cout << "Wow, we got up to " << i << "!\n";

  // Sleep for some time, then see how long we actually slept for
  std::cout << "Attempting to sleep for 1 second...\n";
  auto start_time = etcpal::TimePoint::Now();
  etcpal::Thread::Sleep(1s);
  std::cout << "We actually slept for " << etcpal::TimePoint::Now() - start_time << " ms.\n";

  // Must always deinit with the same feature macro combination that was passed to init().
  etcpal_deinit(ETCPAL_FEATURE_TIMERS);
  return 0;
}
