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

#include "etcpal/signal.h"

bool etcpal_signal_create(etcpal_signal_t* id)
{
  if (id)
  {
    if (0 == pthread_mutex_init(&id->mutex, NULL))
    {
      if (0 == pthread_cond_init(&id->cond, NULL))
      {
        id->valid = true;
        id->signaled = false;
        return true;
      }
      pthread_mutex_destroy(&id->mutex);
    }
  }
  return false;
}

bool etcpal_signal_wait(etcpal_signal_t* id)
{
  if (id && id->valid)
  {
    if (0 == pthread_mutex_lock(&id->mutex))
    {
      while (!id->signaled)
      {
        if (0 != pthread_cond_wait(&id->cond, &id->mutex))
        {
          // On error, the mutex is not locked
          return false;
        }
      }
      id->signaled = false;
      pthread_mutex_unlock(&id->mutex);
      return true;
    }
  }
  return false;
}

bool etcpal_signal_try_wait(etcpal_signal_t* id)
{
  bool res = false;
  if (id && id->valid)
  {
    if (0 == pthread_mutex_lock(&id->mutex))
    {
      if (id->signaled)
      {
        res = true;
        id->signaled = false;
      }
      pthread_mutex_unlock(&id->mutex);
    }
  }
  return res;
}

bool etcpal_signal_timed_wait(etcpal_signal_t* id, int timeout_ms)
{
  return (timeout_ms == 0 ? etcpal_signal_try_wait(id) : etcpal_signal_wait(id));
}

void etcpal_signal_post(etcpal_signal_t* id)
{
  if (id && id->valid)
  {
    if (0 == pthread_mutex_lock(&id->mutex))
    {
      id->signaled = true;
      pthread_cond_signal(&id->cond);
      pthread_mutex_unlock(&id->mutex);
    }
  }
}

void etcpal_signal_destroy(etcpal_signal_t* id)
{
  if (id && id->valid)
  {
    pthread_cond_destroy(&id->cond);
    pthread_mutex_destroy(&id->mutex);
    id->valid = false;
  }
}
