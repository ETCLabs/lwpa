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

/// @file etcpal/cpp/mutex.h
/// @brief C++ wrapper and utilities for etcpal/mutex.h

#ifndef ETCPAL_CPP_MUTEX_H_
#define ETCPAL_CPP_MUTEX_H_

#include <stdexcept>
#include "etcpal/cpp/common.h"
#include "etcpal/mutex.h"

namespace etcpal
{
/// @defgroup etcpal_cpp_mutex mutex (Mutexes)
/// @ingroup etcpal_cpp
/// @brief C++ utilities for the @ref etcpal_mutex module.
///
/// Provides a C++ wrapper for the EtcPal mutex type. Also provides an RAII guard classes for this
/// type (MutexGuard), which should be the primary constructs used to interact with the mutex
/// class.
///
/// This class is similar to std::mutex, with the key advantage that it will work on any threaded
/// platform that EtcPal is ported for, including the embedded RTOS platforms. If your application
/// or library does not need to run on these platforms, consider using std::mutex instead.

/// @ingroup etcpal_cpp_mutex
/// @brief A wrapper class for the EtcPal mutex type.
///
/// Note: The etcpal::Mutex functions are not normally used directly - prefer usage of the RAII
/// type etcpal::MutexGuard to manage locking and Unlocking of an etcpal::Mutex.
///
/// Example usage:
/// @code
/// static int shared_counter;
/// etcpal::Mutex mutex;
///
/// void CounterThread()
/// {
///   for (int i = 0; i < 10000; ++i)
///   {
///     etcpal::MutexGuard lock(mutex);
///     ++shared_counter;
///   }
/// }
///
/// int main()
/// {
///   std::vector<etcpal::Thread> counter_threads;
///   counter_threads.reserve(5);
///   for (int i = 0; i < 5; ++i)
///   {
///     counter_threads.emplace_back(CounterThread);
///   }
///
///   // Wait for the threads to finish
///   etcpal::Thread::Sleep(1000);
///   for (auto& thread : counter_threads)
///   {
///     thread.Join();
///   }
///
///   std::cout << "The counter is: " << shared_counter << '\n'; // Prints "the counter is: 50000"
///
///   return 0;
/// }
/// @endcode
///
/// See @ref etcpal_mutex for more information.
class Mutex
{
public:
  Mutex();
  ~Mutex();

  Mutex(const Mutex& other) = delete;
  Mutex& operator=(const Mutex& other) = delete;
  Mutex(Mutex&& other) = delete;
  Mutex& operator=(Mutex&& other) = delete;

  bool Lock();
  bool TryLock(int timeout_ms = 0);
  void Unlock();

  etcpal_mutex_t& get();

private:
  etcpal_mutex_t mutex_{};
};

/// @brief Create a new mutex.
inline Mutex::Mutex()
{
  (void)etcpal_mutex_create(&mutex_);
}

/// @brief Destroy the mutex.
inline Mutex::~Mutex()
{
  etcpal_mutex_destroy(&mutex_);
}

/// @brief Lock the mutex.
/// @return The result of etcpal_mutex_lock() on the underlying mutex.
inline bool Mutex::Lock()
{
  return etcpal_mutex_lock(&mutex_);
}

/// @brief Attempt to lock the mutex.
///
/// Returns when either the mutex is acquired or the timeout expires. **NOTE**: Timeout values
/// other than 0 or ETCPAL_WAIT_FOREVER are typically only honored on real-time platforms. See the
/// table in @ref etcpal_mutex for more information. On platforms where timeouts are not honored,
/// passing 0 for timeout_ms executes a poll for the mutex returning immediately, while any other
/// value executes the equivalent of Lock().
///
/// @param timeout_ms How long to wait to acquire the mutex, in milliseconds. Default is to poll
///                   and return immediately.
/// @return The result of etcpal_mutex_timed_lock() on the underlying mutex.
inline bool Mutex::TryLock(int timeout_ms)
{
  return etcpal_mutex_timed_lock(&mutex_, timeout_ms);
}

/// @brief Unlock the mutex.
///
/// See etcpal_mutex_Unlock().
inline void Mutex::Unlock()
{
  etcpal_mutex_unlock(&mutex_);
}

/// @brief Get a reference to the underlying etcpal_mutex_t type.
inline etcpal_mutex_t& Mutex::get()
{
  return mutex_;
}

/// @ingroup etcpal_cpp_mutex
/// @brief Lock guard around a mutex.
///
/// Lock is taken when this class is instantiated, and released when it goes out of scope.
/// Example usage:
///
/// @code
/// void Class::CriticalFunction()
/// {
///   etcpal::MutexGuard lock(mutex);
///   // Do critical section operations here
///
///   // Lock is released when it goes out of scope
/// }
/// @endcode
class MutexGuard
{
public:
  explicit MutexGuard(Mutex& mutex);
  explicit MutexGuard(etcpal_mutex_t& mutex);
  ~MutexGuard();

  MutexGuard(const MutexGuard& other) = delete;
  MutexGuard& operator=(const MutexGuard& other) = delete;
  MutexGuard(MutexGuard&& other) = delete;
  MutexGuard& operator=(MutexGuard&& other) = delete;

private:
  etcpal_mutex_t& mutex_;

  void GetLock();
};

/// @brief Lock an etcpal::Mutex.
/// @throw std::runtime_error if locking the mutex failed.
inline MutexGuard::MutexGuard(Mutex& mutex) : mutex_(mutex.get())
{
  GetLock();
}

/// @brief Lock an @ref etcpal_mutex_t.
/// @throw std::runtime_error if locking the mutex failed.
inline MutexGuard::MutexGuard(etcpal_mutex_t& mutex) : mutex_(mutex)
{
  GetLock();
}

/// @brief Release the lock upon going out-of-scope.
inline MutexGuard::~MutexGuard()
{
  etcpal_mutex_unlock(&mutex_);
}

inline void MutexGuard::GetLock()
{
  if (!etcpal_mutex_lock(&mutex_))
    ETCPAL_THROW(std::runtime_error("etcpal_mutex_lock failed."));
}

};  // namespace etcpal

#endif  // ETCPAL_CPP_MUTEX_H_
