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

#include "etcpal/mempool.h"

#include <stdbool.h>
#include "etcpal/common.h"
#if !ETCPAL_NO_OS_SUPPORT
#include "etcpal/mutex.h"
#endif

#if !ETCPAL_NO_OS_SUPPORT
static bool           mempool_lock_initted = false;
static etcpal_mutex_t mempool_lock;
#endif

etcpal_error_t etcpal_mempool_init_priv(EtcPalMempoolDesc* desc)
{
  if (!desc || desc->pool_size == 0)
    return kEtcPalErrInvalid;

#if !ETCPAL_NO_OS_SUPPORT
  if (!mempool_lock_initted)
  {
    if (etcpal_mutex_create(&mempool_lock))
      mempool_lock_initted = true;
    else
      return kEtcPalErrSys;
  }

  etcpal_error_t res = kEtcPalErrSys;
  if (etcpal_mutex_lock(&mempool_lock))
  {
#endif
    size_t i = 0;
    for (; i < desc->pool_size - 1; ++i)
      desc->list[i].next = &desc->list[i + 1];
    desc->list[i].next = NULL;
    desc->freelist = desc->list;
    desc->current_used = 0;
#if !ETCPAL_NO_OS_SUPPORT
    res = kEtcPalErrOk;
    etcpal_mutex_unlock(&mempool_lock);
  }
  return res;
#else
  return kEtcPalErrOk;
#endif
}

void* etcpal_mempool_alloc_priv(EtcPalMempoolDesc* desc)
{
  if (!desc)
    return NULL;

  void* elem = NULL;

#if !ETCPAL_NO_OS_SUPPORT
  if (etcpal_mutex_lock(&mempool_lock))
  {
#endif
    char*          c_pool = (char*)desc->pool;
    EtcPalMempool* elem_desc = desc->freelist;
    if (elem_desc)
    {
      ptrdiff_t index = elem_desc - desc->list;
      if (index >= 0)
      {
        desc->freelist = elem_desc->next;
        elem = (void*)(c_pool + ((size_t)index * desc->elem_size));
        ++desc->current_used;
      }
    }
#if !ETCPAL_NO_OS_SUPPORT
    etcpal_mutex_unlock(&mempool_lock);
  }
#endif

  return elem;
}

void etcpal_mempool_free_priv(EtcPalMempoolDesc* desc, void* elem)
{
  if (!desc || !elem)
    return;

  char* c_pool = (char*)desc->pool;

  ptrdiff_t offset = (char*)elem - c_pool;
  if (offset >= 0)
  {
    if (((size_t)offset % desc->elem_size == 0))
    {
#if !ETCPAL_NO_OS_SUPPORT
      if (etcpal_mutex_lock(&mempool_lock))
      {
#endif
        size_t         index = (size_t)offset / desc->elem_size;
        EtcPalMempool* elem_desc = &desc->list[index];
        elem_desc->next = desc->freelist;
        desc->freelist = elem_desc;
        --desc->current_used;
#if !ETCPAL_NO_OS_SUPPORT
        etcpal_mutex_unlock(&mempool_lock);
      }
#endif
    }
  }
}

size_t etcpal_mempool_used_priv(EtcPalMempoolDesc* desc)
{
  if (!desc)
    return 0;

#if ETCPAL_NO_OS_SUPPORT
  return desc->current_used;
#else
  size_t res = 0;
  if (etcpal_mutex_lock(&mempool_lock))
  {
    res = desc->current_used;
    etcpal_mutex_unlock(&mempool_lock);
  }
  return res;
#endif
}
