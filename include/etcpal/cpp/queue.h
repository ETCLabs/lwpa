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

/// \file etcpal/cpp/queue.h
/// \brief C++ wrapper and utilities for etcpal/queue.h

#ifndef ETCPAL_CPP_QUEUE_H
#define ETCPAL_CPP_QUEUE_H

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <limits>
#include "etcpal/common.h"
#include "etcpal/queue.h"
#include "etcpal/cpp/common.h"

namespace etcpal
{
/// @defgroup etcpal_cpp_queue queue (RTOS queues)
/// @ingroup etcpal_cpp
/// @brief C++ utilities for the @ref etcpal_queue module.
///
/// Provides a template class Queue which can be used to create blocking OS queues of arbitrary
/// objects.
///
/// @code
/// #include "etcpal/cpp/queue.h"
///
/// struct Foo
/// {
///   Foo(int new_value) : value(new_value) {}
///
///   int value{0};
/// };
///
/// // Create a queue big enough to hold 15 Foo instances.
/// etcpal::Queue<Foo> queue(15);
/// @endcode
///
/// Use the Send() and Receive() functions to add items to and remove items from the queue.
///
/// @code
/// queue.Send(Foo(42));
///
/// Foo received_foo;
/// queue.Receive(received_foo);
/// EXPECT_EQ(received_foo.value, 42);
/// @endcode
///
/// By default, the Send() and Receive() functions will block indefinitely. You can also specify
/// timeouts to these functions:
///
/// @code
/// Foo received_foo;
/// queue.Receive(received_foo, 50); // Block up to 50 milliseconds waiting for a Foo
/// queue.Receive(received_foo, 0); // Return immediately if a Foo is not available
/// queue.Receive(received_foo, ETCPAL_WAIT_FOREVER); // Block indefinitely waiting for a Foo
///
/// // In a C++14 or greater environment...
/// using namespace std::chrono_literals;
///
/// queue.Receive(received_foo, 2s); // Block up to 2 seconds waiting for a Foo
/// @endcode
///
/// In these cases, the Send() and Receive() functions will return false if the timeout was reached
/// while waiting.
///
/// OS queues are only available on RTOS platforms. See the @ref etcpal_queue module for details on
/// what platforms this class is available on.

/// @ingroup etcpal_cpp_queue
/// @brief An RTOS queue class.
///
/// See the module description for @ref etcpal_cpp_queue for usage information.
template <class T>
class Queue
{
public:
  explicit Queue(size_t size);
  ~Queue();

  Queue(const Queue& other) = delete;
  Queue& operator=(const Queue& other) = delete;
  Queue(Queue&& other) = delete;
  Queue& operator=(Queue&& other) = delete;

  bool Send(const T& data, int timeout_ms = ETCPAL_WAIT_FOREVER);
  bool SendFromIsr(const T& data);

  bool Receive(T& data, int timeout_ms = ETCPAL_WAIT_FOREVER);
  template <class Rep, class Period>
  bool Receive(T& data, const std::chrono::duration<Rep, Period>& timeout);
  bool ReceiveFromIsr(T& data);
  bool Reset();
  bool IsEmpty() const;
  bool IsEmptyFromIsr() const;
  bool IsFull() const;
  bool IsFullFromIsr() const;
  size_t SlotsUsed() const;
  size_t SlotsUsedFromIsr() const;
  size_t SlotsAvailable() const;

private:
  etcpal_queue_t queue_{};
};

/// @brief Create a new queue.
///
/// @param size The size of the queue.
template <class T>
inline Queue<T>::Queue(size_t size)
{
  etcpal_queue_create(&queue_, size, sizeof(T));
}

/// @brief Destroy a queue.
template <class T>
inline Queue<T>::~Queue()
{
  etcpal_queue_destroy(&queue_);
}

/// @brief Add some data to the queue.
///
/// Returns when either the timeout expires or the add was attempted. It is still possible for the
/// attempt to be made and the add to not work. Check the return value for confirmation.
///
/// @param data A reference to the data.
/// @param timeout_ms How long to wait to add to the queue.
/// @return The result of the attempt to add to the queue.
template <class T>
inline bool Queue<T>::Send(const T& data, int timeout_ms)
{
  return etcpal_queue_timed_send(&queue_, &data, timeout_ms);
}

/// @brief Add to a queue from an interrupt service routine.
/// @param data A reference to the data to be added to the queue.
/// @return The result of the attempt to add to the queue.
template <class T>
inline bool Queue<T>::SendFromIsr(const T& data)
{
  return etcpal_queue_send_from_isr(&queue_, &data);
}

/// @brief Get an item from the queue.
/// @param data A reference to the data that will receive the item from the queue.
/// @param timeout_ms Amount of time to wait for data.
/// @return The result of the attempt to get an item from the queue.
template <class T>
inline bool Queue<T>::Receive(T& data, int timeout_ms)
{
  return etcpal_queue_timed_receive(&queue_, &data, timeout_ms);
}

/// @brief Get an item from the queue.
///
/// @param data A reference to the data that will receive the item from the queue.
/// @param timeout Amount of time to wait for data.
///
/// @return The result of the attempt to get an item from the queue.
template <class T>
template <class Rep, class Period>
inline bool Queue<T>::Receive(T& data, const std::chrono::duration<Rep, Period>& timeout)
{
  int timeout_ms_clamped =
      static_cast<int>(std::min(std::chrono::milliseconds(timeout).count(),
                                static_cast<std::chrono::milliseconds::rep>(std::numeric_limits<int>::max())));
  return etcpal_queue_timed_receive(&queue_, &data, timeout_ms_clamped);
}

/// @brief Get an item from the queue from an interrupt context.
/// @param data A reference to the data that will receive the item from the queue.
/// @return The result of the attempt to get an item from the queue.
template <class T>
inline bool Queue<T>::ReceiveFromIsr(T& data)
{
  return etcpal_queue_receive_from_isr(&queue_, &data);
}

/// @brief Resets queue to empty state.
///
/// @return true on success, false otherwise.
template <class T>
inline bool Queue<T>::Reset()
{
  return etcpal_queue_reset(&queue_);
}

/// @brief Check if a queue is empty.
///
/// @return true if queue is empty, false otherwise.
template <class T>
inline bool Queue<T>::IsEmpty() const
{
  return etcpal_queue_is_empty(&queue_);
};

/// @brief Check if a queue is empty from an interrupt service routine.
///
/// @return true if queue is empty, false otherwise.
template <class T>
inline bool Queue<T>::IsEmptyFromIsr() const
{
  return etcpal_queue_is_empty_from_isr(&queue_);
};

/// @brief Check if a queue is full.
///
/// @return true if queue is full, false otherwise.
template <class T>
inline bool Queue<T>::IsFull() const
{
  return etcpal_queue_is_full(&queue_);
};

/// @brief Check if a queue is full from an interrupt service routine.
///
/// @return true if queue is full, false otherwise.
template <class T>
inline bool Queue<T>::IsFullFromIsr() const
{
  return etcpal_queue_is_full_from_isr(&queue_);
};

/// @brief Get number of slots being stored in the queue.
///
/// @return number of slots in queue.
template <class T>
inline size_t Queue<T>::SlotsUsed() const
{
  return etcpal_queue_slots_used(&queue_);
};

/// @brief Get number of slots being stored in the queue from an interrupt service routine.
///
/// @return number of slots in queue.
template <class T>
inline size_t Queue<T>::SlotsUsedFromIsr() const
{
  return etcpal_queue_slots_used_from_isr(&queue_);
};

/// @brief Get number of remaining slots in the queue.
///
/// @return number of remaining slots in queue.
template <class T>
inline size_t Queue<T>::SlotsAvailable() const
{
  return etcpal_queue_slots_available(&queue_);
};

};  // namespace etcpal

#endif  // ETCPAL_CPP_RTOS_ERROR_H
