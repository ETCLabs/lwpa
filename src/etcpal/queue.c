// Author: Noah Meltzer

#include "etcpal/queue.h"
#include <string.h>

static inline bool wait_for_space_timed(const etcpal_queue_t* queue, int timeout_ms)
{
  return etcpal_sem_timed_wait((etcpal_sem_t*)&queue->spots_available, timeout_ms);
}

static inline bool notify_space_available(const etcpal_queue_t* queue)
{
  return etcpal_sem_post((etcpal_sem_t*)&queue->spots_available);
}

static inline bool notify_space_available_from_isr(const etcpal_queue_t* queue)
{
  return etcpal_sem_post_from_isr((etcpal_sem_t*)&queue->spots_available);
}

static inline bool wait_for_data_timed(const etcpal_queue_t* queue, int timeout_ms)
{
  return etcpal_sem_timed_wait((etcpal_sem_t*)&queue->spots_filled, timeout_ms);
}

static inline bool notify_data_available(const etcpal_queue_t* queue)
{
  return etcpal_sem_post((etcpal_sem_t*)&queue->spots_filled);
}

static inline bool notify_data_available_from_isr(const etcpal_queue_t* queue)
{
  return etcpal_sem_post_from_isr((etcpal_sem_t*)&queue->spots_filled);
}

static inline bool lock(const etcpal_queue_t* queue)
{
  bool success = etcpal_sem_wait((etcpal_sem_t*)&queue->lock);
  return success;
}

static inline void unlock(const etcpal_queue_t* queue)
{
  (void)etcpal_sem_post((etcpal_sem_t*)&queue->lock);
}

static inline void unlock_from_isr(const etcpal_queue_t* queue)
{
  (void)etcpal_sem_post_from_isr((etcpal_sem_t*)&queue->lock);
}

static inline bool push_data_timed(etcpal_queue_t* queue, const void* data, int timeout_ms)
{
  bool true_if_success = false;

  if (wait_for_space_timed(queue, timeout_ms))
  {
    lock(queue);

    memcpy(queue->node_list[queue->head].data, data, queue->element_size);

    queue->head++;
    queue->head %= (queue->max_queue_size + 1);
    queue->queue_size++;

    true_if_success = true;

    unlock(queue);
    notify_data_available(queue);
  }

  return true_if_success;
}

static inline bool push_data_from_isr(etcpal_queue_t* queue, const void* data)
{
  bool true_if_success = false;

  if (wait_for_space_timed(queue, 0))
  {
    lock(queue);

    memcpy(queue->node_list[queue->head].data, data, queue->element_size);

    queue->head++;
    queue->head %= (queue->max_queue_size + 1);
    queue->queue_size++;

    true_if_success = true;

    unlock_from_isr(queue);
    notify_data_available_from_isr(queue);
  }

  return true_if_success;
}

static inline bool pop_data_timed(etcpal_queue_t* queue, void* data, int timeout_ms)
{
  bool true_if_success = false;
  if (wait_for_data_timed(queue, timeout_ms))
  {
    lock(queue);
    memcpy(data, queue->node_list[queue->tail].data, queue->element_size);

    queue->tail++;
    queue->tail %= (queue->max_queue_size + 1);

    queue->queue_size--;

    true_if_success = true;

    unlock(queue);
    notify_space_available(queue);
  }

  return true_if_success;
}

static inline bool pop_data_from_isr(etcpal_queue_t* queue, void* data)
{
  bool true_if_success = false;
  if (wait_for_data_timed(queue, 0))
  {
    lock(queue);
    memcpy(data, queue->node_list[queue->tail].data, queue->element_size);

    queue->tail++;
    queue->tail %= (queue->max_queue_size + 1);

    queue->queue_size--;

    true_if_success = true;

    unlock(queue);
    notify_space_available_from_isr(queue);
  }

  return true_if_success;
}

bool etcpal_queue_create(etcpal_queue_t* id, size_t size, size_t item_size)
{
  // Initialize queue
  memset(id, 0, sizeof(etcpal_queue_t));

  if (!id)
  {
    return false;
  }

  id->element_size = item_size;

  // Initialize locks
  if (!etcpal_sem_create(&id->lock, 1, 1))
    return false;
  if (!etcpal_sem_create(&id->spots_available, (unsigned)size, (unsigned)size))
  {
    etcpal_sem_destroy(&id->lock);
    return false;
  }
  if (!etcpal_sem_create(&id->spots_filled, 0, (unsigned)size))
  {
    etcpal_sem_destroy(&id->spots_available);
    etcpal_sem_destroy(&id->spots_filled);
    return false;
  }

  // Circular buffers need space for an extra item
  id->node_list = calloc(sizeof(EtcPalQueueNode), size + 1);
  if (id->node_list)
  {
    for (unsigned item_index = 0; item_index < size + 1; item_index++)
    {
      id->node_list[item_index].data = calloc(item_size, 1);
    }
  }
  else
  {
    return false;
  }

  id->max_queue_size = size;
  id->queue_size = 0;

  id->tail = 0;
  id->head = 0;
  return true;
}

void etcpal_queue_destroy(etcpal_queue_t* id)
{
  lock(id);
  if (id->node_list)
  {
    for (unsigned item_index = 0; item_index < id->max_queue_size + 1; item_index++)
    {
      free(id->node_list[item_index].data);
    }
  }

  free(id->node_list);

#if ETCPAL_SEM_MUST_BE_BALANCED
  // Reset the semaphores to their initial counts before destroying
  for (size_t i = 0; i < id->queue_size; ++i)
  {
    etcpal_sem_wait(&id->spots_filled);
    etcpal_sem_post(&id->spots_available);
  }
#endif

  unlock(id);

  etcpal_sem_destroy(&id->lock);
  etcpal_sem_destroy(&id->spots_available);
  etcpal_sem_destroy(&id->spots_filled);
}

bool etcpal_queue_send(etcpal_queue_t* id, const void* data)
{
  bool true_if_success = false;

  true_if_success = push_data_timed(id, data, ETCPAL_WAIT_FOREVER);

  return true_if_success;
}

bool etcpal_queue_timed_send(etcpal_queue_t* id, const void* data, int timeout_ms)
{
  bool true_if_success = false;

  true_if_success = push_data_timed(id, data, timeout_ms);

  return true_if_success;
}

bool etcpal_queue_receive(etcpal_queue_t* id, void* data)
{
  bool true_if_success = false;

  true_if_success = pop_data_timed(id, data, ETCPAL_WAIT_FOREVER);

  return true_if_success;
}

bool etcpal_queue_timed_receive(etcpal_queue_t* id, void* data, int timeout_ms)
{
  bool true_if_success = false;

  true_if_success = pop_data_timed(id, data, timeout_ms);

  return true_if_success;
}

bool etcpal_queue_send_from_isr(etcpal_queue_t* id, const void* data)
{
  bool true_if_success = false;

  true_if_success = push_data_from_isr(id, data);

  return true_if_success;
}

bool etcpal_queue_receive_from_isr(etcpal_queue_t* id, void* data)
{
  bool true_if_success = false;

  true_if_success = pop_data_from_isr(id, data);

  return true_if_success;
}

bool etcpal_queue_reset(etcpal_queue_t* id)
{
  bool true_if_success = true;

  lock(id);

  id->queue_size = 0;

  id->tail = 0;
  id->head = 0;

  unlock(id);
  return true_if_success;
}

bool etcpal_queue_is_empty(const etcpal_queue_t* id)
{
  bool true_if_empty = false;
  lock(id);
  true_if_empty = (id->queue_size == 0);
  unlock(id);
  return true_if_empty;
}

bool etcpal_queue_is_empty_from_isr(const etcpal_queue_t* id)
{
  bool true_if_empty = true;
  if (lock(id))
  {
    true_if_empty = (id->queue_size == 0);
    unlock_from_isr(id);
  }
  return true_if_empty;
}

bool etcpal_queue_is_full(const etcpal_queue_t* id)
{
  bool true_if_full = false;
  lock(id);
  true_if_full = (id->queue_size == id->max_queue_size);
  unlock(id);
  return true_if_full;
}

bool etcpal_queue_is_full_from_isr(const etcpal_queue_t* id)
{
  bool true_if_full = true;
  if (lock(id))
  {
    true_if_full = (id->queue_size == id->max_queue_size);
    unlock_from_isr(id);
  }
  return true_if_full;
}

size_t etcpal_queue_slots_used(const etcpal_queue_t* id)
{
  size_t size = 0;
  lock(id);
  size = id->queue_size;
  unlock(id);

  return size;
}

size_t etcpal_queue_slots_used_from_isr(const etcpal_queue_t* id)
{
  size_t size = 0;
  if (lock(id))
  {
    unlock_from_isr(id);
  }
  return size;
}

size_t etcpal_queue_slots_available(const etcpal_queue_t* id)
{
  lock(id);
  size_t elements = id->max_queue_size - id->queue_size;
  unlock(id);

  return elements;
}
