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

#define _GNU_SOURCE  // for pthread_setname_np() - this is a Linux-specific file

#include "etcpal/thread.h"

#include <string.h>
#include "etcpal/common.h"
#include "os_error.h"

#if !defined(ETCPAL_BUILDING_MOCK_LIB)

/*********************** Private function prototypes *************************/

static void* thread_func_internal(void* arg);

/*************************** Function definitions ****************************/

etcpal_error_t etcpal_thread_create(etcpal_thread_t*          id,
                                    const EtcPalThreadParams* params,
                                    void (*thread_fn)(void*),
                                    void* thread_arg)
{
  if (!id || !params || !thread_fn)
    return kEtcPalErrInvalid;

  pthread_attr_t  thread_attr;
  pthread_attr_t* p_thread_attr = NULL;

  if (params && params->stack_size != ETCPAL_THREAD_DEFAULT_STACK)
  {
    pthread_attr_init(&thread_attr);
    pthread_attr_setstacksize(&thread_attr, params->stack_size);
    p_thread_attr = &thread_attr;
  }

  if (params->thread_name)
  {
    strncpy(id->name, params->thread_name, ETCPAL_THREAD_NAME_MAX_LENGTH);
    id->name[ETCPAL_THREAD_NAME_MAX_LENGTH - 1] = '\0';
  }
  else
  {
    id->name[0] = '\0';
  }

  id->fn = thread_fn;
  id->arg = thread_arg;
  etcpal_error_t res = kEtcPalErrOk;
  if (0 != pthread_create(&id->handle, p_thread_attr, thread_func_internal, id))
    res = errno_os_to_etcpal(errno);

  if (p_thread_attr)
    pthread_attr_destroy(p_thread_attr);

  return res;
}

etcpal_error_t etcpal_thread_join(etcpal_thread_t* id)
{
  if (id)
  {
    if (0 == pthread_join(id->handle, NULL))
      return kEtcPalErrOk;
    return errno_os_to_etcpal(errno);
  }
  return kEtcPalErrInvalid;
}

etcpal_error_t etcpal_thread_timed_join(etcpal_thread_t* id, int timeout_ms)
{
  ETCPAL_UNUSED_ARG(timeout_ms);
  return etcpal_thread_join(id);
}

etcpal_error_t etcpal_thread_terminate(etcpal_thread_t* id)
{
  if (!id)
    return kEtcPalErrInvalid;

  if (pthread_cancel(id->handle) != 0)
    return kEtcPalErrSys;

  return kEtcPalErrOk;
}

void* thread_func_internal(void* arg)
{
  etcpal_thread_t* thread_data = (etcpal_thread_t*)arg;
  if (thread_data && thread_data->fn)
  {
    if (thread_data->name[0] != '\0')
      pthread_setname_np(thread_data->handle, thread_data->name);

    thread_data->fn(thread_data->arg);
  }
  return NULL;
}

etcpal_thread_os_handle_t etcpal_thread_get_os_handle(etcpal_thread_t* id)
{
  return (id ? id->handle : ETCPAL_THREAD_OS_HANDLE_INVALID);
}

#endif  // !defined(ETCPAL_BUILDING_MOCK_LIB)
