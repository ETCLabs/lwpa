/******************************************************************************
 * Copyright 2020 ETC Inc.
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
#include "fff.h"

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
  RUN_TEST_GROUP(etcpal_lock);
  RUN_TEST_GROUP(etcpal_thread);
  RUN_TEST_GROUP(etcpal_queue);
  RUN_TEST_GROUP(etcpal_timer);
#endif
#if !ETCPAL_NO_NETWORKING_SUPPORT
  RUN_TEST_GROUP(etcpal_netint);
  RUN_TEST_GROUP(etcpal_inet);
  RUN_TEST_GROUP(etcpal_socket);
#endif
}
