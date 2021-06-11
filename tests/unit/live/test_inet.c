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

#include "etcpal/inet.h"
#include "unity_fixture.h"

#include <limits.h>
#include <string.h>

#ifdef __MQX__
#define MQX_PROVIDES_STDIO !MQX_SUPPRESS_STDIO_MACROS
#else
#define MQX_PROVIDES_STDIO 0
#endif

#if !MQX_PROVIDES_STDIO
#include <stdio.h>
#endif

// Disable sprintf() warning on Windows/MSVC
#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif

TEST_GROUP(etcpal_inet);

TEST_SETUP(etcpal_inet)
{
}

TEST_TEAR_DOWN(etcpal_inet)
{
}

TEST(etcpal_inet, invalid_calls_fail)
{
  // TODO fill this out with the rest of the functions
  char                mac_str_buf[ETCPAL_MAC_STRING_BYTES];
  const EtcPalMacAddr mac = {0};
  TEST_ASSERT_NOT_EQUAL(kEtcPalErrOk, etcpal_mac_to_string(NULL, mac_str_buf));
  TEST_ASSERT_NOT_EQUAL(kEtcPalErrOk, etcpal_mac_to_string(&mac, NULL));
}

TEST(etcpal_inet, ipaddr_init_macros_work)
{
  const EtcPalIpAddr invalid = ETCPAL_IP_INVALID_INIT;
  TEST_ASSERT_TRUE(ETCPAL_IP_IS_INVALID(&invalid));

  const EtcPalIpAddr v4 = ETCPAL_IPV4_INIT(0x12345678);
  TEST_ASSERT_TRUE(ETCPAL_IP_IS_V4(&v4));
  TEST_ASSERT_EQUAL_UINT32(ETCPAL_IP_V4_ADDRESS(&v4), 0x12345678);

#define V6_INITIALIZER 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0

  const uint8_t      v6_val[] = {V6_INITIALIZER};
  const EtcPalIpAddr v6 = ETCPAL_IPV6_INIT(V6_INITIALIZER);

  TEST_ASSERT_TRUE(ETCPAL_IP_IS_V6(&v6));
  TEST_ASSERT_EQUAL_UINT8_ARRAY(ETCPAL_IP_V6_ADDRESS(&v6), v6_val, 16);
}

// Test the EtcPalIpAddr struct and macros
TEST(etcpal_inet, ipaddr_set_macros_work)
{
  EtcPalIpAddr test_addr;

  ETCPAL_IP_SET_V4_ADDRESS(&test_addr, 0x01020304);
  TEST_ASSERT(ETCPAL_IP_IS_V4(&test_addr));
  TEST_ASSERT_EQUAL_UINT32(ETCPAL_IP_V4_ADDRESS(&test_addr), 0x01020304u);

  uint8_t v6_data[ETCPAL_IPV6_BYTES] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
  ETCPAL_IP_SET_V6_ADDRESS_WITH_SCOPE_ID(&test_addr, v6_data, 32);
  TEST_ASSERT(ETCPAL_IP_IS_V6(&test_addr));
  TEST_ASSERT_EQUAL(ETCPAL_IP_V6_SCOPE_ID(&test_addr), 32);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(ETCPAL_IP_V6_ADDRESS(&test_addr), v6_data, ETCPAL_IPV6_BYTES);

  ETCPAL_IP_SET_INVALID(&test_addr);
  TEST_ASSERT(ETCPAL_IP_IS_INVALID(&test_addr));
}

// Test the etcpal_ip_is_loopback() function
TEST(etcpal_inet, ip_is_loopback_works)
{
  EtcPalIpAddr test_addr;

  // An invalid IP is not loopback
  ETCPAL_IP_SET_INVALID(&test_addr);
  TEST_ASSERT_UNLESS(etcpal_ip_is_loopback(&test_addr));

  // Test the edges of the IPv4 loopback range
  ETCPAL_IP_SET_V4_ADDRESS(&test_addr, 0x7effffff);
  TEST_ASSERT_UNLESS(etcpal_ip_is_loopback(&test_addr));

  ETCPAL_IP_SET_V4_ADDRESS(&test_addr, 0x7f000000);
  TEST_ASSERT(etcpal_ip_is_loopback(&test_addr));

  ETCPAL_IP_SET_V4_ADDRESS(&test_addr, 0x7fffffff);
  TEST_ASSERT(etcpal_ip_is_loopback(&test_addr));

  ETCPAL_IP_SET_V4_ADDRESS(&test_addr, 0x80000000);
  TEST_ASSERT_UNLESS(etcpal_ip_is_loopback(&test_addr));

  // Test the IPv6 loopback address
  uint8_t v6_data[ETCPAL_IPV6_BYTES] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
  ETCPAL_IP_SET_V6_ADDRESS(&test_addr, v6_data);
  TEST_ASSERT(etcpal_ip_is_loopback(&test_addr));

  v6_data[15] = 0;
  ETCPAL_IP_SET_V6_ADDRESS(&test_addr, v6_data);
  TEST_ASSERT_UNLESS(etcpal_ip_is_loopback(&test_addr));
}

// Test the etcpal_ip_is_multicast() function
TEST(etcpal_inet, ip_is_multicast_works)
{
  EtcPalIpAddr test_addr;

  // An invalid IP is not multicast
  ETCPAL_IP_SET_INVALID(&test_addr);
  TEST_ASSERT_UNLESS(etcpal_ip_is_multicast(&test_addr));

  // Test the edges of the IPv4 multicast range
  ETCPAL_IP_SET_V4_ADDRESS(&test_addr, 0xdfffffff);
  TEST_ASSERT_UNLESS(etcpal_ip_is_multicast(&test_addr));

  ETCPAL_IP_SET_V4_ADDRESS(&test_addr, 0xe0000000);
  TEST_ASSERT(etcpal_ip_is_multicast(&test_addr));

  ETCPAL_IP_SET_V4_ADDRESS(&test_addr, 0xefffffff);
  TEST_ASSERT(etcpal_ip_is_multicast(&test_addr));

  ETCPAL_IP_SET_V4_ADDRESS(&test_addr, 0xf0000000);
  TEST_ASSERT_UNLESS(etcpal_ip_is_multicast(&test_addr));

  ETCPAL_IP_SET_V4_ADDRESS(&test_addr, 0xffffffff);
  TEST_ASSERT_UNLESS(etcpal_ip_is_multicast(&test_addr));

  // Test the edges of the IPv6 multicast range
  uint8_t v6_data[ETCPAL_IPV6_BYTES] = {0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
  ETCPAL_IP_SET_V6_ADDRESS(&test_addr, v6_data);
  TEST_ASSERT_UNLESS(etcpal_ip_is_multicast(&test_addr));

  memcpy(v6_data, (uint8_t[]){0xff, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, ETCPAL_IPV6_BYTES);
  ETCPAL_IP_SET_V6_ADDRESS(&test_addr, v6_data);
  TEST_ASSERT(etcpal_ip_is_multicast(&test_addr));

  memcpy(v6_data,
         (uint8_t[]){0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
         ETCPAL_IPV6_BYTES);
  ETCPAL_IP_SET_V6_ADDRESS(&test_addr, v6_data);
  TEST_ASSERT(etcpal_ip_is_multicast(&test_addr));
}

// Just tests reflexivity of the wildcard functions, we don't rely on the wildcard address to be
// any specific value.
TEST(etcpal_inet, ip_is_wildcard_works)
{
  EtcPalIpAddr test_addr;

  etcpal_ip_set_wildcard(kEtcPalIpTypeV4, &test_addr);
  TEST_ASSERT(etcpal_ip_is_wildcard(&test_addr));

  etcpal_ip_set_wildcard(kEtcPalIpTypeV6, &test_addr);
  TEST_ASSERT(etcpal_ip_is_wildcard(&test_addr));
}

// Test the etcpal_ip_cmp() function
TEST(etcpal_inet, ip_compare_functions_work)
{
  EtcPalIpAddr v4;
  ETCPAL_IP_SET_V4_ADDRESS(&v4, 0x01020304);

  // Copied address should compare equal
  EtcPalIpAddr v4_copy = v4;
  TEST_ASSERT_EQUAL_INT(0, etcpal_ip_cmp(&v4_copy, &v4));

  EtcPalIpAddr v6;
  uint8_t      v6_data[ETCPAL_IPV6_BYTES] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
  ETCPAL_IP_SET_V6_ADDRESS(&v6, v6_data);

  // Copied address should compare equal
  EtcPalIpAddr v6_copy = v6;
  TEST_ASSERT_EQUAL_INT(0, etcpal_ip_cmp(&v6_copy, &v6));

  // The v4 and v6 should not be equal, and per the comparison algorithm the v4 should be less than
  // the v6
  TEST_ASSERT_LESS_THAN_INT(0, etcpal_ip_cmp(&v4, &v6));

  // Further test the etcpal_ip_cmp() function, with like address types
  EtcPalIpAddr v4_less;
  ETCPAL_IP_SET_V4_ADDRESS(&v4_less, 0x01020303);

  EtcPalIpAddr v6_less;
  uint8_t      v6_data_less[ETCPAL_IPV6_BYTES];
  memcpy(v6_data_less, v6_data, ETCPAL_IPV6_BYTES);
  v6_data_less[15] = 14;
  ETCPAL_IP_SET_V6_ADDRESS(&v6_less, v6_data_less);

  TEST_ASSERT_LESS_THAN_INT(0, etcpal_ip_cmp(&v4_less, &v4));
  TEST_ASSERT_LESS_THAN_INT(0, etcpal_ip_cmp(&v6_less, &v6));

  // Make sure there are no gotchas with 2's complement by setting high bits of addresses
  ETCPAL_IP_SET_V4_ADDRESS(&v4, 0xffffffff);
  ETCPAL_IP_SET_V4_ADDRESS(&v4_less, 0x0);

  TEST_ASSERT_LESS_THAN_INT(0, etcpal_ip_cmp(&v4_less, &v4));
}

// Test the etcpal_ip_mask_length() function
TEST(etcpal_inet, ip_mask_length_works)
{
  EtcPalIpAddr test_mask;

  // Test mask: Invalid or null IP
  TEST_ASSERT_EQUAL_UINT(0u, etcpal_ip_mask_length(NULL));
  ETCPAL_IP_SET_INVALID(&test_mask);
  TEST_ASSERT_EQUAL_UINT(0u, etcpal_ip_mask_length(&test_mask));

  // Test mask: /0, V4
  ETCPAL_IP_SET_V4_ADDRESS(&test_mask, 0);
  TEST_ASSERT_EQUAL_UINT(0u, etcpal_ip_mask_length(&test_mask));

  // Test mask: /7, V4
  ETCPAL_IP_SET_V4_ADDRESS(&test_mask, 0xfe000000u);
  TEST_ASSERT_EQUAL_UINT(7u, etcpal_ip_mask_length(&test_mask));

  // Test mask: /25, V4
  ETCPAL_IP_SET_V4_ADDRESS(&test_mask, 0xffffff80u);
  TEST_ASSERT_EQUAL_UINT(25u, etcpal_ip_mask_length(&test_mask));

  // Test mask: /32, V4
  ETCPAL_IP_SET_V4_ADDRESS(&test_mask, 0xffffffffu);
  TEST_ASSERT_EQUAL_UINT(32u, etcpal_ip_mask_length(&test_mask));

  // Test mask: nonzero but with leading zeroes, V4
  ETCPAL_IP_SET_V4_ADDRESS(&test_mask, 0x7fffffff);
  TEST_ASSERT_EQUAL_UINT(0u, etcpal_ip_mask_length(&test_mask));

  // Test mask: /0, V6
  uint8_t test_v6_val[ETCPAL_IPV6_BYTES] = {0};
  ETCPAL_IP_SET_V6_ADDRESS(&test_mask, test_v6_val);
  TEST_ASSERT_EQUAL_UINT(0u, etcpal_ip_mask_length(&test_mask));

  // Test mask: /64, V6
  memcpy(test_v6_val, (uint8_t[]){0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0, 0, 0, 0, 0, 0, 0, 0},
         ETCPAL_IPV6_BYTES);
  ETCPAL_IP_SET_V6_ADDRESS(&test_mask, test_v6_val);
  TEST_ASSERT_EQUAL_UINT(64u, etcpal_ip_mask_length(&test_mask));

  // Test mask: /77, V6
  memcpy(test_v6_val, (uint8_t[]){0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf8, 0, 0, 0, 0, 0, 0},
         ETCPAL_IPV6_BYTES);
  ETCPAL_IP_SET_V6_ADDRESS(&test_mask, test_v6_val);
  TEST_ASSERT_EQUAL_UINT(77u, etcpal_ip_mask_length(&test_mask));

  // Test mask: /128, V6
  memcpy(test_v6_val,
         (uint8_t[]){0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
         ETCPAL_IPV6_BYTES);
  ETCPAL_IP_SET_V6_ADDRESS(&test_mask, test_v6_val);
  TEST_ASSERT_EQUAL_UINT(128u, etcpal_ip_mask_length(&test_mask));

  // Test mask: nonzero but with leading zeroes, V6
  memcpy(test_v6_val,
         (uint8_t[]){0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
         ETCPAL_IPV6_BYTES);
  ETCPAL_IP_SET_V6_ADDRESS(&test_mask, test_v6_val);
  TEST_ASSERT_EQUAL_UINT(0u, etcpal_ip_mask_length(&test_mask));
}

// Test the etcpal_ip_mask_from_length() function
TEST(etcpal_inet, ip_mask_from_length_works)
{
  // Test mask: /0, V4
  EtcPalIpAddr mask_out = etcpal_ip_mask_from_length(kEtcPalIpTypeV4, 0);
  TEST_ASSERT(ETCPAL_IP_IS_V4(&mask_out));
  TEST_ASSERT_EQUAL_UINT32(ETCPAL_IP_V4_ADDRESS(&mask_out), 0u);

  // Test mask: /7, V4
  mask_out = etcpal_ip_mask_from_length(kEtcPalIpTypeV4, 7);
  TEST_ASSERT(ETCPAL_IP_IS_V4(&mask_out));
  TEST_ASSERT_EQUAL_UINT32(ETCPAL_IP_V4_ADDRESS(&mask_out), 0xfe000000u);

  // Test mask: /25, V4
  mask_out = etcpal_ip_mask_from_length(kEtcPalIpTypeV4, 25);
  TEST_ASSERT(ETCPAL_IP_IS_V4(&mask_out));
  TEST_ASSERT_EQUAL_UINT32(ETCPAL_IP_V4_ADDRESS(&mask_out), 0xffffff80u);

  // Test mask: /32, V4
  mask_out = etcpal_ip_mask_from_length(kEtcPalIpTypeV4, 32);
  TEST_ASSERT(ETCPAL_IP_IS_V4(&mask_out));
  TEST_ASSERT_EQUAL_UINT32(ETCPAL_IP_V4_ADDRESS(&mask_out), 0xffffffffu);

  // Test mask: Greater than /32, V4
  // Should truncate to 32
  mask_out = etcpal_ip_mask_from_length(kEtcPalIpTypeV4, UINT_MAX);
  TEST_ASSERT(ETCPAL_IP_IS_V4(&mask_out));
  TEST_ASSERT_EQUAL_UINT32(ETCPAL_IP_V4_ADDRESS(&mask_out), 0xffffffffu);

  // Test mask: /0, V6
  uint8_t v6_compare_val[ETCPAL_IPV6_BYTES] = {0};
  mask_out = etcpal_ip_mask_from_length(kEtcPalIpTypeV6, 0);
  TEST_ASSERT(ETCPAL_IP_IS_V6(&mask_out));
  TEST_ASSERT_EQUAL_UINT8_ARRAY(ETCPAL_IP_V6_ADDRESS(&mask_out), v6_compare_val, ETCPAL_IPV6_BYTES);

  // Test mask: /64, V6
  memcpy(v6_compare_val, (uint8_t[]){0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0, 0, 0, 0, 0, 0, 0, 0},
         ETCPAL_IPV6_BYTES);
  mask_out = etcpal_ip_mask_from_length(kEtcPalIpTypeV6, 64);
  TEST_ASSERT(ETCPAL_IP_IS_V6(&mask_out));
  TEST_ASSERT_EQUAL_UINT8_ARRAY(ETCPAL_IP_V6_ADDRESS(&mask_out), v6_compare_val, ETCPAL_IPV6_BYTES);

  // Test mask: /77, V6
  memcpy(v6_compare_val, (uint8_t[]){0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf8, 0, 0, 0, 0, 0, 0},
         ETCPAL_IPV6_BYTES);
  mask_out = etcpal_ip_mask_from_length(kEtcPalIpTypeV6, 77);
  TEST_ASSERT(ETCPAL_IP_IS_V6(&mask_out));
  TEST_ASSERT_EQUAL_UINT8_ARRAY(ETCPAL_IP_V6_ADDRESS(&mask_out), v6_compare_val, ETCPAL_IPV6_BYTES);

  // Test mask: /128, V6
  memcpy(v6_compare_val,
         (uint8_t[]){0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
         ETCPAL_IPV6_BYTES);
  mask_out = etcpal_ip_mask_from_length(kEtcPalIpTypeV6, 128);
  TEST_ASSERT(ETCPAL_IP_IS_V6(&mask_out));
  TEST_ASSERT_EQUAL_UINT8_ARRAY(ETCPAL_IP_V6_ADDRESS(&mask_out), v6_compare_val, ETCPAL_IPV6_BYTES);

  // Test mask: Greater than /128, V6
  // Should truncate to 128
  mask_out = etcpal_ip_mask_from_length(kEtcPalIpTypeV6, UINT_MAX);
  TEST_ASSERT(ETCPAL_IP_IS_V6(&mask_out));
  TEST_ASSERT_EQUAL_UINT8_ARRAY(ETCPAL_IP_V6_ADDRESS(&mask_out), v6_compare_val, ETCPAL_IPV6_BYTES);
}

TEST(etcpal_inet, ip_network_portions_equal_invalid_calls_fail)
{
  // NULL parameters
  TEST_ASSERT_FALSE(etcpal_ip_network_portions_equal(NULL, NULL, NULL));

  // Contrasting IP types
  EtcPalIpAddr addr1;
  EtcPalIpAddr addr2;
  ETCPAL_IP_SET_V4_ADDRESS(&addr1, 0xc0a80101);
  ETCPAL_IP_SET_V4_ADDRESS(&addr2, 0xc0a80102);
  EtcPalIpAddr mask = etcpal_ip_mask_from_length(kEtcPalIpTypeV6, 64);

  TEST_ASSERT_FALSE(etcpal_ip_network_portions_equal(&addr1, &addr2, &mask));
}

TEST(etcpal_inet, ip_network_portions_equal_works_ipv4)
{
  EtcPalIpAddr addr1;
  EtcPalIpAddr addr2;
  EtcPalIpAddr mask;

  ETCPAL_IP_SET_V4_ADDRESS(&addr1, 0xa5a50011);
  ETCPAL_IP_SET_V4_ADDRESS(&addr2, 0xa5a51100);
  ETCPAL_IP_SET_V4_ADDRESS(&mask, 0xffff0000);

  TEST_ASSERT_TRUE(etcpal_ip_network_portions_equal(&addr1, &addr2, &mask));

  ETCPAL_IP_SET_V4_ADDRESS(&mask, 0xffffff00);
  TEST_ASSERT_FALSE(etcpal_ip_network_portions_equal(&addr1, &addr2, &mask));

  ETCPAL_IP_SET_V4_ADDRESS(&mask, 0);
  TEST_ASSERT_TRUE(etcpal_ip_network_portions_equal(&addr1, &addr2, &mask));

  ETCPAL_IP_SET_V4_ADDRESS(&mask, 0xffffffff);
  TEST_ASSERT_FALSE(etcpal_ip_network_portions_equal(&addr1, &addr2, &mask));
}

TEST(etcpal_inet, ip_network_portions_equal_works_ipv6)
{
  EtcPalIpAddr addr1;
  EtcPalIpAddr addr2;

  etcpal_string_to_ip(kEtcPalIpTypeV6, "2001:db8::1234:5678", &addr1);
  etcpal_string_to_ip(kEtcPalIpTypeV6, "2001:db8::8765:4321", &addr2);
  EtcPalIpAddr mask = etcpal_ip_mask_from_length(kEtcPalIpTypeV6, 64);

  TEST_ASSERT_TRUE(etcpal_ip_network_portions_equal(&addr1, &addr2, &mask));

  mask = etcpal_ip_mask_from_length(kEtcPalIpTypeV6, 112);
  TEST_ASSERT_FALSE(etcpal_ip_network_portions_equal(&addr1, &addr2, &mask));

  mask = etcpal_ip_mask_from_length(kEtcPalIpTypeV6, 0);
  TEST_ASSERT_TRUE(etcpal_ip_network_portions_equal(&addr1, &addr2, &mask));

  mask = etcpal_ip_mask_from_length(kEtcPalIpTypeV6, 128);
  TEST_ASSERT_FALSE(etcpal_ip_network_portions_equal(&addr1, &addr2, &mask));
}

// For ip/string functions
static char              str[ETCPAL_IP_STRING_BYTES];
static const char* const kTestIp41 = "0.0.0.0";
static const char* const kTestIp42 = "255.255.255.255";
static const char* const kTestIp4Fail = "256.256.256.256";
static const char* const kTestIp61 = "::";
static const uint8_t     kTestIp61Bin[ETCPAL_IPV6_BYTES] = {0};
static const char* const kTestIp62 = "::1";
static const uint8_t     kTestIp62Bin[ETCPAL_IPV6_BYTES] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
static const char* const kTestIp63 = "ffff:FFFF:ffff:FFFF:ffff:FFFF:ffff:FFFF";
static const uint8_t     kTestIp63Bin[ETCPAL_IPV6_BYTES] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                                        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
static const char* const kTestIp6Fail = "abcd::ef01::2345";

TEST(etcpal_inet, ip_to_string_conversion_works)
{
  EtcPalIpAddr addr;

  ETCPAL_IP_SET_V4_ADDRESS(&addr, 0);
  TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_ip_to_string(&addr, str));
  TEST_ASSERT_EQUAL_STRING(str, kTestIp41);
  ETCPAL_IP_SET_V4_ADDRESS(&addr, 0xffffffff);
  TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_ip_to_string(&addr, str));
  TEST_ASSERT_EQUAL_STRING(str, kTestIp42);
  ETCPAL_IP_SET_V6_ADDRESS(&addr, kTestIp61Bin);
  TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_ip_to_string(&addr, str));
  TEST_ASSERT_EQUAL_STRING(str, kTestIp61);
  ETCPAL_IP_SET_V6_ADDRESS(&addr, kTestIp62Bin);
  TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_ip_to_string(&addr, str));
  TEST_ASSERT_EQUAL_STRING(str, kTestIp62);
  ETCPAL_IP_SET_V6_ADDRESS(&addr, kTestIp63Bin);
  TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_ip_to_string(&addr, str));
  TEST_ASSERT((0 == strcmp(str, "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff")) ||
              (0 == strcmp(str, "FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF")));
}

TEST(etcpal_inet, string_to_ip_conversion_works)
{
  EtcPalIpAddr addr;

  TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_string_to_ip(kEtcPalIpTypeV4, kTestIp41, &addr));
  TEST_ASSERT_EQUAL(ETCPAL_IP_V4_ADDRESS(&addr), 0u);
  TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_string_to_ip(kEtcPalIpTypeV4, kTestIp42, &addr));
  TEST_ASSERT_EQUAL(ETCPAL_IP_V4_ADDRESS(&addr), 0xffffffffu);
  TEST_ASSERT_NOT_EQUAL(kEtcPalErrOk, etcpal_string_to_ip(kEtcPalIpTypeV4, kTestIp4Fail, &addr));
  TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_string_to_ip(kEtcPalIpTypeV6, kTestIp61, &addr));
  TEST_ASSERT_EQUAL(0, memcmp(ETCPAL_IP_V6_ADDRESS(&addr), kTestIp61Bin, ETCPAL_IPV6_BYTES));
  TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_string_to_ip(kEtcPalIpTypeV6, kTestIp62, &addr));
  TEST_ASSERT_EQUAL(0, memcmp(ETCPAL_IP_V6_ADDRESS(&addr), kTestIp62Bin, ETCPAL_IPV6_BYTES));
  TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_string_to_ip(kEtcPalIpTypeV6, kTestIp63, &addr));
  TEST_ASSERT_EQUAL(0, memcmp(ETCPAL_IP_V6_ADDRESS(&addr), kTestIp63Bin, ETCPAL_IPV6_BYTES));
  TEST_ASSERT_NOT_EQUAL(kEtcPalErrOk, etcpal_string_to_ip(kEtcPalIpTypeV6, kTestIp6Fail, &addr));
}

TEST(etcpal_inet, mac_is_null_works)
{
  EtcPalMacAddr mac = {{0}};
  TEST_ASSERT_TRUE(ETCPAL_MAC_IS_NULL(&mac));

  mac = kEtcPalNullMacAddr;
  TEST_ASSERT_TRUE(ETCPAL_MAC_IS_NULL(&mac));

  for (uint8_t i = 0; i < ETCPAL_MAC_BYTES; ++i)
    mac.data[i] = i;
  TEST_ASSERT_FALSE(ETCPAL_MAC_IS_NULL(&mac));
}

TEST(etcpal_inet, mac_compare_works)
{
  const EtcPalMacAddr mac1 = {{1, 2, 3, 4, 5, 6}};
  const EtcPalMacAddr mac2 = {{1, 2, 3, 4, 5, 7}};

  TEST_ASSERT_EQUAL(0, ETCPAL_MAC_CMP(&mac1, &mac1));
  TEST_ASSERT_GREATER_THAN(0, ETCPAL_MAC_CMP(&mac2, &mac1));
  TEST_ASSERT_LESS_THAN(0, ETCPAL_MAC_CMP(&mac1, &mac2));
}

TEST(etcpal_inet, mac_to_string_conversion_works)
{
  const EtcPalMacAddr mac = {{1, 2, 3, 4, 5, 6}};
  char                str_buf[ETCPAL_MAC_STRING_BYTES];

  etcpal_mac_to_string(&mac, str_buf);
  TEST_ASSERT_EQUAL_STRING(str_buf, "01:02:03:04:05:06");
}

TEST(etcpal_inet, string_to_mac_conversion_works)
{
  EtcPalMacAddr       mac;
  const char*         good_strings[] = {"0a:0b:cd:01:02:34", "0a0bcd010234"};
  const EtcPalMacAddr good_str_mac = {{0x0a, 0x0b, 0xcd, 0x01, 0x02, 0x34}};
  // clang-format off
  const char* bad_strings[] = {
    "0a:0b:cd:01:02:0", // short by one character
    "0a:0b::cd:01:02:34", // extra colon
    "ga:0b:cd:01:02:34", // bad character at the beginning
    "This isn't a MAC", // Short random string
    "This isn't a MAC but it is a very long string" // Long random string
  };
  // clang-format on

  for (size_t i = 0; i < (sizeof(good_strings) / sizeof(const char*)); ++i)
  {
    char msg_buf[100];
    sprintf(msg_buf, "Failed on input: %s", good_strings[i]);
    TEST_ASSERT_EQUAL_MESSAGE(etcpal_string_to_mac(good_strings[i], &mac), kEtcPalErrOk, msg_buf);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ETCPAL_MAC_CMP(&mac, &good_str_mac), msg_buf);
  }

  for (size_t i = 0; i < (sizeof(bad_strings) / sizeof(const char*)); ++i)
  {
    char msg_buf[100];
    sprintf(msg_buf, "Failed on input: %s", bad_strings[i]);
    TEST_ASSERT_NOT_EQUAL_MESSAGE(etcpal_string_to_mac(bad_strings[i], &mac), kEtcPalErrOk, msg_buf);
  }
}

TEST_GROUP_RUNNER(etcpal_inet)
{
  RUN_TEST_CASE(etcpal_inet, invalid_calls_fail);
  RUN_TEST_CASE(etcpal_inet, ipaddr_init_macros_work);
  RUN_TEST_CASE(etcpal_inet, ipaddr_set_macros_work);
  RUN_TEST_CASE(etcpal_inet, ip_is_loopback_works);
  RUN_TEST_CASE(etcpal_inet, ip_is_multicast_works);
  RUN_TEST_CASE(etcpal_inet, ip_is_wildcard_works);
  RUN_TEST_CASE(etcpal_inet, ip_compare_functions_work);
  RUN_TEST_CASE(etcpal_inet, ip_mask_length_works);
  RUN_TEST_CASE(etcpal_inet, ip_mask_from_length_works);
  RUN_TEST_CASE(etcpal_inet, ip_network_portions_equal_invalid_calls_fail);
  RUN_TEST_CASE(etcpal_inet, ip_network_portions_equal_works_ipv4);
  RUN_TEST_CASE(etcpal_inet, ip_network_portions_equal_works_ipv6);
  RUN_TEST_CASE(etcpal_inet, ip_to_string_conversion_works);
  RUN_TEST_CASE(etcpal_inet, string_to_ip_conversion_works);
  RUN_TEST_CASE(etcpal_inet, mac_is_null_works);
  RUN_TEST_CASE(etcpal_inet, mac_compare_works);
  RUN_TEST_CASE(etcpal_inet, mac_to_string_conversion_works);
  RUN_TEST_CASE(etcpal_inet, string_to_mac_conversion_works);
}
