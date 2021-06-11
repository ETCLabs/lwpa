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

#include "etcpal/netint.h"
#include "etcpal/private/netint.h"
#include "unity_fixture.h"
#include "etc_fff_wrapper.h"

ETC_FAKE_VALUE_FUNC(etcpal_error_t, os_enumerate_interfaces, CachedNetintInfo*);
ETC_FAKE_VOID_FUNC(os_free_interfaces, CachedNetintInfo*);
ETC_FAKE_VALUE_FUNC(etcpal_error_t, os_resolve_route, const EtcPalIpAddr*, const CachedNetintInfo*, unsigned int*);
ETC_FAKE_VALUE_FUNC(bool, os_netint_is_up, unsigned int, const CachedNetintInfo*);

TEST_GROUP(netint_controlled);

TEST_SETUP(netint_controlled)
{
  TEST_ASSERT_EQUAL(etcpal_netint_init(), kEtcPalErrOk);

  RESET_FAKE(os_enumerate_interfaces);
  RESET_FAKE(os_free_interfaces);
  RESET_FAKE(os_resolve_route);
  RESET_FAKE(os_netint_is_up);
}

TEST_TEAR_DOWN(netint_controlled)
{
  etcpal_netint_deinit();
}

TEST(netint_controlled, netint_is_up_works)
{
  os_netint_is_up_fake.return_val = true;
  TEST_ASSERT_TRUE(etcpal_netint_is_up(1));
  TEST_ASSERT_EQUAL_UINT(os_netint_is_up_fake.arg0_val, 1);

  os_netint_is_up_fake.return_val = false;
  TEST_ASSERT_FALSE(etcpal_netint_is_up(2));
  TEST_ASSERT_EQUAL_UINT(os_netint_is_up_fake.arg0_val, 2);
}

TEST_GROUP_RUNNER(netint_controlled)
{
  RUN_TEST_CASE(netint_controlled, netint_is_up_works);
}
