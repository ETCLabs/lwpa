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

#include "unity_fixture.h"
#include "etc_fff_wrapper.h"

DEFINE_FFF_GLOBALS;

void run_all_tests(void)
{
  RUN_TEST_GROUP(etcpal_common);
  RUN_TEST_GROUP(etcpal_log);
  RUN_TEST_GROUP(etcpal_mempool);
  RUN_TEST_GROUP(etcpal_pack);
  RUN_TEST_GROUP(etcpal_rbtree);
  RUN_TEST_GROUP(etcpal_uuid);
#if !ETCPAL_NO_OS_SUPPORT
#if !DISABLE_EVENT_GROUP_TESTS
  RUN_TEST_GROUP(etcpal_event_group);
#endif
  RUN_TEST_GROUP(etcpal_mutex);
#if !DISABLE_RECURSIVE_MUTEX_TESTS
  RUN_TEST_GROUP(etcpal_recursive_mutex);
#endif
  RUN_TEST_GROUP(etcpal_rwlock);
  RUN_TEST_GROUP(etcpal_sem);
  RUN_TEST_GROUP(etcpal_signal);
  RUN_TEST_GROUP(etcpal_thread);
  RUN_TEST_GROUP(etcpal_timer);
#if !DISABLE_QUEUE_TESTS
  RUN_TEST_GROUP(etcpal_queue);
#endif  // DISABLE_QUEUE_TESTS
#endif  // ETCPAL_NO_OS_SUPPORT

#if !ETCPAL_NO_NETWORKING_SUPPORT
  RUN_TEST_GROUP(etcpal_netint);
  RUN_TEST_GROUP(etcpal_inet);
  RUN_TEST_GROUP(etcpal_socket);
#endif
}
