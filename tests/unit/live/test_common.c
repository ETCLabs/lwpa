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

#include "etcpal/common.h"
#include "unity_fixture.h"
#include "etc_fff_wrapper.h"

#if !ETCPAL_NO_NETWORKING_SUPPORT
#include "etcpal/netint.h"
#endif
#include "etcpal/log.h"

TEST_GROUP(etcpal_common);

TEST_SETUP(etcpal_common)
{
}

TEST_TEAR_DOWN(etcpal_common)
{
}

// Test the ETCPAL_FEATURES_ALL_BUT() macro
TEST(etcpal_common, features_all_but_macro_works)
{
  etcpal_features_t mask = ETCPAL_FEATURES_ALL_BUT(ETCPAL_FEATURE_SOCKETS);
  TEST_ASSERT(mask & ETCPAL_FEATURE_NETINTS);
  TEST_ASSERT(mask & ETCPAL_FEATURE_TIMERS);
  TEST_ASSERT(mask & ETCPAL_FEATURE_LOGGING);
  TEST_ASSERT_UNLESS(mask & ETCPAL_FEATURE_SOCKETS);

  mask = ETCPAL_FEATURES_ALL_BUT(ETCPAL_FEATURE_LOGGING);
  TEST_ASSERT(mask & ETCPAL_FEATURE_SOCKETS);
  TEST_ASSERT(mask & ETCPAL_FEATURE_NETINTS);
  TEST_ASSERT(mask & ETCPAL_FEATURE_TIMERS);
  TEST_ASSERT_UNLESS(mask & ETCPAL_FEATURE_LOGGING);
}

#if !ETCPAL_NO_NETWORKING_SUPPORT
// Test multiple calls of etcpal_init() for the netint module.
TEST(etcpal_common, netint_double_init_works)
{
  TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_init(ETCPAL_FEATURE_NETINTS));
  TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_init(ETCPAL_FEATURE_NETINTS));

  etcpal_deinit(ETCPAL_FEATURE_NETINTS);

  // After 2 inits and one deinit, we should still be able to make valid calls to the module.
  unsigned int def_netint = 0;
  TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_netint_get_default_interface(kEtcPalIpTypeV4, &def_netint));

  etcpal_deinit(ETCPAL_FEATURE_NETINTS);
}
#endif

// A shim from the etcpal_log module to fff.
ETC_FAKE_VOID_FUNC(common_test_log_callback, void*, const EtcPalLogStrings*);

// Test multiple calls of etcpal_init() for the log module.
TEST(etcpal_common, log_double_init_works)
{
  TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_init(ETCPAL_FEATURE_LOGGING));
  TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_init(ETCPAL_FEATURE_LOGGING));

  EtcPalLogParams params = ETCPAL_LOG_PARAMS_INIT;
  params.action = ETCPAL_LOG_CREATE_HUMAN_READABLE;
  params.log_fn = common_test_log_callback;
  params.log_mask = ETCPAL_LOG_UPTO(ETCPAL_LOG_DEBUG);

  TEST_ASSERT_TRUE(etcpal_validate_log_params(&params));

  etcpal_log(&params, ETCPAL_LOG_INFO, "Log message");
  TEST_ASSERT_EQUAL(common_test_log_callback_fake.call_count, 1);

  etcpal_deinit(ETCPAL_FEATURE_LOGGING);

  // After 2 inits and one deinit, we should still be able to use the etcpal_log() function and get
  // callbacks.
  etcpal_log(&params, ETCPAL_LOG_INFO, "Log message");
  TEST_ASSERT_EQUAL(common_test_log_callback_fake.call_count, 2);

  etcpal_deinit(ETCPAL_FEATURE_LOGGING);
}

TEST_GROUP_RUNNER(etcpal_common)
{
  RUN_TEST_CASE(etcpal_common, features_all_but_macro_works);
#if !ETCPAL_NO_NETWORKING_SUPPORT
  RUN_TEST_CASE(etcpal_common, netint_double_init_works);
#endif
  RUN_TEST_CASE(etcpal_common, log_double_init_works);
}
