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

#ifndef ETCPAL_OS_QUEUE_H
#define ETCPAL_OS_QUEUE_H

#include "etcpal/common.h"
#include "etcpal/queue.h"
#include "etcpal/sem.h"

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct EtcPalQueueNode
{
  void* data;
} EtcPalQueueNode;

typedef struct
{
  unsigned         head;
  unsigned         tail;
  EtcPalQueueNode* node_list;
  size_t           max_queue_size;
  size_t           queue_size;

  etcpal_sem_t lock;
  etcpal_sem_t spots_available;
  etcpal_sem_t spots_filled;
  size_t       element_size;
} etcpal_queue_t;

#define ETCPAL_QUEUE_HAS_TIMED_FUNCTIONS ETCPAL_SEM_HAS_TIMED_WAIT
#define ETCPAL_QUEUE_HAS_ISR_FUNCTIONS ETCPAL_SEM_HAS_POST_FROM_ISR

bool etcpal_queue_create(etcpal_queue_t* id, size_t size, size_t item_size);
void etcpal_queue_destroy(etcpal_queue_t* id);

bool etcpal_queue_send(etcpal_queue_t* id, const void* data);
bool etcpal_queue_timed_send(etcpal_queue_t* id, const void* data, int timeout_ms);
bool etcpal_queue_send_from_isr(etcpal_queue_t* id, const void* data);

bool etcpal_queue_receive(etcpal_queue_t* id, void* data);
bool etcpal_queue_timed_receive(etcpal_queue_t* id, void* data, int timeout_ms);
bool etcpal_queue_receive_from_isr(etcpal_queue_t* id, void* data);

bool etcpal_queue_reset(etcpal_queue_t* id);

bool etcpal_queue_is_empty(const etcpal_queue_t* id);
bool etcpal_queue_is_empty_from_isr(const etcpal_queue_t* id);

bool etcpal_queue_is_full(const etcpal_queue_t* id);
bool etcpal_queue_is_full_from_isr(const etcpal_queue_t* id);

size_t etcpal_queue_slots_used(const etcpal_queue_t* id);
size_t etcpal_queue_slots_used_from_isr(const etcpal_queue_t* id);

size_t etcpal_queue_slots_available(const etcpal_queue_t* id);

#ifdef __cplusplus
}
#endif

#endif  // ETCPAL_OS_QUEUE_H_
