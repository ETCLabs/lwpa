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
#include "unity_fixture.h"

#include <string.h>
#include <stddef.h>

#ifdef __MQX__
#define MQX_PROVIDES_STDIO !MQX_SUPPRESS_STDIO_MACROS
#else
#define MQX_PROVIDES_STDIO 0
#endif

#if !MQX_PROVIDES_STDIO
#include <stdio.h>
#endif

static size_t num_netints;

// Tests to run on the netint module without it being initialized first

TEST_GROUP(etcpal_netint_no_init);

TEST_SETUP(etcpal_netint_no_init)
{
}

TEST_TEAR_DOWN(etcpal_netint_no_init)
{
}

// Test that none of the API functions return successfully before the module is initialized. We
// generate valid data to pass to each one, to be sure they don't return e.g. kEtcPalErrInvalid
// instead.
TEST(etcpal_netint_no_init, api_does_not_work_before_initialization)
{
  TEST_ASSERT_EQUAL(etcpal_netint_get_num_interfaces(), 0u);
  TEST_ASSERT_EQUAL_PTR(etcpal_netint_get_interfaces(), NULL);

  const EtcPalNetintInfo* netint_arr = NULL;
  size_t                  netint_arr_size = 0;
  TEST_ASSERT_EQUAL(etcpal_netint_get_interfaces_by_index(1, &netint_arr, &netint_arr_size), kEtcPalErrNotInit);

  unsigned int index = 0;
  TEST_ASSERT_EQUAL(etcpal_netint_get_default_interface(kEtcPalIpTypeV4, &index), kEtcPalErrNotInit);
  TEST_ASSERT_EQUAL(etcpal_netint_get_default_interface(kEtcPalIpTypeV6, &index), kEtcPalErrNotInit);

  EtcPalIpAddr dest;
  etcpal_string_to_ip(kEtcPalIpTypeV4, "8.8.8.8", &dest);
  TEST_ASSERT_EQUAL(etcpal_netint_get_interface_for_dest(&dest, &index), kEtcPalErrNotInit);
}

// The main test group. This initializes the module before each test and deinits it after.

TEST_GROUP(etcpal_netint);

TEST_SETUP(etcpal_netint)
{
  etcpal_init(ETCPAL_FEATURE_NETINTS);
  num_netints = etcpal_netint_get_num_interfaces();
}

TEST_TEAR_DOWN(etcpal_netint)
{
  etcpal_deinit(ETCPAL_FEATURE_NETINTS);
}

TEST(etcpal_netint, netint_enumeration_works)
{
  TEST_ASSERT_GREATER_THAN_UINT(0u, num_netints);

  const EtcPalNetintInfo* netint_list = etcpal_netint_get_interfaces();
  TEST_ASSERT_NOT_NULL(netint_list);

  for (const EtcPalNetintInfo* netint = netint_list; netint < netint_list + num_netints; ++netint)
  {
    TEST_ASSERT_GREATER_THAN_UINT(0u, netint->index);
    TEST_ASSERT_GREATER_THAN_UINT(0u, strlen(netint->id));
    TEST_ASSERT_GREATER_THAN_UINT(0u, strlen(netint->friendly_name));
  }
}

TEST(etcpal_netint, netints_are_in_index_order)
{
  TEST_ASSERT_GREATER_THAN_UINT(0u, num_netints);

  const EtcPalNetintInfo* netint_list = etcpal_netint_get_interfaces();
  unsigned int            last_index = netint_list->index;
  for (const EtcPalNetintInfo* netint = netint_list + 1; netint < netint_list + num_netints; ++netint)
  {
    TEST_ASSERT_GREATER_OR_EQUAL(last_index, netint->index);
    last_index = netint->index;
  }
}

/* TODO Figure out how we're gonna handle mallocing
TEST(etcpal_netint, copy_interfaces_works)
{
  // Test copying the full array
  auto netint_arr = std::make_unique<EtcPalNetintInfo[]>(num_netints);
  size_t num_netints_returned = etcpal_netint_copy_interfaces(netint_arr.get(), num_netints);
  ASSERT_EQ(num_netints_returned, num_netints);

  // Make sure the array is equal to the list obtained from etcpal_netint_get_interfaces()
  EXPECT_EQ(0, memcmp(netint_arr.get(), etcpal_netint_get_interfaces(), num_netints));
}
*/

TEST(etcpal_netint, get_netints_by_index_works)
{
  TEST_ASSERT_GREATER_THAN_UINT(0u, num_netints);

  const EtcPalNetintInfo* netint_list = etcpal_netint_get_interfaces();
  unsigned int            current_index = 0;
  // There are other tests covering that the netint list should be in order by index and that the
  // indexes must all be greater than 0, which simplifies this code.
  const EtcPalNetintInfo* current_arr_by_index = NULL;
  size_t                  current_index_arr_size = 0;
  for (const EtcPalNetintInfo* netint = netint_list; netint < netint_list + num_netints; ++netint)
  {
    if (netint->index > current_index)
    {
      // The previous get-by-index array should be exhausted
      TEST_ASSERT_EQUAL_UINT(current_index_arr_size, 0u);

      // Get the new one
      current_index = netint->index;
      TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_netint_get_interfaces_by_index(netint->index, &current_arr_by_index,
                                                                            &current_index_arr_size));
      TEST_ASSERT_GREATER_THAN_UINT(0u, current_index_arr_size);
    }

    // Now check that we're still in the get-by-index array and it gives the correct slice of the
    // total netint array.
    TEST_ASSERT_GREATER_THAN_UINT(0u, current_index_arr_size);
    TEST_ASSERT_EQUAL_PTR(current_arr_by_index, netint);
    ++current_arr_by_index;
    --current_index_arr_size;
  }
}

TEST(etcpal_netint, default_netint_is_consistent)
{
  TEST_ASSERT_GREATER_THAN_UINT(0u, num_netints);

  unsigned int def_v4 = 0;
  unsigned int def_v6 = 0;

  bool have_default_v4 = (kEtcPalErrOk == etcpal_netint_get_default_interface(kEtcPalIpTypeV4, &def_v4));
  bool have_default_v6 = (kEtcPalErrOk == etcpal_netint_get_default_interface(kEtcPalIpTypeV6, &def_v6));

  if (have_default_v4)
  {
    TEST_ASSERT_NOT_EQUAL(def_v4, 0u);
  }
  if (have_default_v6)
  {
    TEST_ASSERT_NOT_EQUAL(def_v6, 0u);
  }

  const EtcPalNetintInfo* netint_list = etcpal_netint_get_interfaces();
  TEST_ASSERT_NOT_NULL(netint_list);
  for (const EtcPalNetintInfo* netint = netint_list; netint < netint_list + num_netints; ++netint)
  {
    if (netint->is_default)
    {
      if (netint->addr.type == kEtcPalIpTypeV4)
      {
        TEST_ASSERT_EQUAL_UINT(netint->index, def_v4);
      }
      else if (netint->addr.type == kEtcPalIpTypeV6)
      {
        TEST_ASSERT_EQUAL_UINT(netint->index, def_v6);
      }
    }
  }
}

TEST(etcpal_netint, get_interface_for_dest_works_ipv4)
{
  TEST_ASSERT_GREATER_THAN_UINT(0u, num_netints);

  const EtcPalNetintInfo* netint_list = etcpal_netint_get_interfaces();
  TEST_ASSERT_NOT_NULL(netint_list);

  // For each normally routable (non-loopback, non-link-local) network interface, check to make sure
  // that etcpal_netint_get_interface_for_dest() resolves to that interface when asked for a route to
  // the interface address itself.
  for (const EtcPalNetintInfo* netint = netint_list; netint < netint_list + num_netints; ++netint)
  {
    if (!ETCPAL_IP_IS_V4(&netint->addr) || etcpal_ip_is_loopback(&netint->addr) ||
        etcpal_ip_is_link_local(&netint->addr))
      continue;

    EtcPalIpAddr test_addr = netint->addr;
    unsigned int netint_index_res = 0;
    TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_netint_get_interface_for_dest(&test_addr, &netint_index_res));

    // Put addresses in print form to test meaningful information in case of test failure
    char test_addr_str[ETCPAL_IP_STRING_BYTES];
    etcpal_ip_to_string(&test_addr, test_addr_str);
    char test_msg[150];
    snprintf(test_msg, 150, "Address tried: %s (interface %u), interface returned: %u", test_addr_str, netint->index,
             netint_index_res);

    TEST_ASSERT_EQUAL_UINT_MESSAGE(netint_index_res, netint->index, test_msg);
  }

  EtcPalIpAddr ext_addr;
  ETCPAL_IP_SET_V4_ADDRESS(&ext_addr, 0xc8dc0302);  // 200.220.3.2
  unsigned int netint_index_res = 0;
  unsigned int netint_index_default = 0;
  TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_netint_get_default_interface(kEtcPalIpTypeV4, &netint_index_default));
  TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_netint_get_interface_for_dest(&ext_addr, &netint_index_res));
  TEST_ASSERT_EQUAL_UINT(netint_index_res, netint_index_default);
}

TEST_GROUP_RUNNER(etcpal_netint)
{
  RUN_TEST_CASE(etcpal_netint_no_init, api_does_not_work_before_initialization);
  RUN_TEST_CASE(etcpal_netint, netint_enumeration_works);
  RUN_TEST_CASE(etcpal_netint, netints_are_in_index_order);
  RUN_TEST_CASE(etcpal_netint, get_netints_by_index_works);
  RUN_TEST_CASE(etcpal_netint, default_netint_is_consistent);
  RUN_TEST_CASE(etcpal_netint, get_interface_for_dest_works_ipv4);
}
