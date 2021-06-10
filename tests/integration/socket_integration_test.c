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

#include "etcpal/socket.h"
#include "unity_fixture.h"

#include <string.h>
#include "etcpal/common.h"
#include "etcpal/netint.h"
#include "etcpal/thread.h"

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

#ifndef ETCPAL_BULK_POLL_TEST_NUM_SOCKETS
#if ETCPAL_SOCKET_MAX_POLL_SIZE <= 0 || ETCPAL_SOCKET_MAX_POLL_SIZE > 1024
// Limit the bulk socket test to a reasonable number
#define ETCPAL_BULK_POLL_TEST_NUM_SOCKETS 512
#else
#define ETCPAL_BULK_POLL_TEST_NUM_SOCKETS (ETCPAL_SOCKET_MAX_POLL_SIZE - 1)
#endif
#endif

#define NUM_TEST_PACKETS 1000

etcpal_error_t      etcpal_init_result;
static unsigned int v4_netint;
bool                run_ipv4_mcast_test;
#if ETCPAL_TEST_IPV6
static unsigned int v6_netint;
bool                run_ipv6_mcast_test;
#endif

static const char kSocketTestMessage[] = "testtesttest";
#define SOCKET_TEST_MESSAGE_LENGTH (sizeof(kSocketTestMessage))
static const uint32_t kTestMcastAddrIPv4 = 0xec02054d;  // 236.2.5.77
// ff02::7465:7374:7465:7374
#if ETCPAL_TEST_IPV6
static const uint8_t kTestMcastAddrIPv6[ETCPAL_IPV6_BYTES] = {0xff, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                              0x74, 0x65, 0x73, 0x74, 0x74, 0x65, 0x73, 0x74};
#endif

static etcpal_socket_t send_sock;
static EtcPalSockAddr  send_addr;
static etcpal_socket_t recv_socks[ETCPAL_BULK_POLL_TEST_NUM_SOCKETS];

#if !ETCPAL_TEST_DISABLE_MCAST_INTEGRATION_TESTS
// Select the default interface if available, the very first non-loopback, non-link-local interface
// if not.
static void select_network_interface_v4()
{
  if (kEtcPalErrOk == etcpal_netint_get_default_interface(kEtcPalIpTypeV4, &v4_netint))
  {
    const EtcPalNetintInfo* netint_arr = NULL;
    size_t                  netint_arr_size = 0;
    if (kEtcPalErrOk == etcpal_netint_get_interfaces_by_index(v4_netint, &netint_arr, &netint_arr_size) &&
        NULL == strstr(netint_arr->id, "utun"))
    {
      run_ipv4_mcast_test = true;
    }
  }
  else
  {
    const EtcPalNetintInfo* arr = etcpal_netint_get_interfaces();
    if (arr)
    {
      for (const EtcPalNetintInfo* netint = arr; netint < arr + etcpal_netint_get_num_interfaces(); ++netint)
      {
        if (ETCPAL_IP_IS_V4(&netint->addr) && !etcpal_ip_is_link_local(&netint->addr) &&
            !etcpal_ip_is_loopback(&netint->addr) && NULL == strstr(netint->id, "utun"))
        {
          v4_netint = netint->index;
          run_ipv4_mcast_test = true;
          return;
        }
      }
      // We haven't found a network interface...
      TEST_MESSAGE("No IPv4 non-loopback, non-link-local network interfaces found. Disabling multicast IPv4 test...");
      run_ipv4_mcast_test = false;
    }
  }
}

#if ETCPAL_TEST_IPV6
// Select the default interface if available, the very first non-loopback interface if not.
static void select_network_interface_v6()
{
  if (kEtcPalErrOk == etcpal_netint_get_default_interface(kEtcPalIpTypeV6, &v6_netint))
  {
    const EtcPalNetintInfo* netint_arr = NULL;
    size_t                  netint_arr_size = 0;
    if (kEtcPalErrOk == etcpal_netint_get_interfaces_by_index(v6_netint, &netint_arr, &netint_arr_size) &&
        NULL == strstr(netint_arr->id, "utun"))
    {
      run_ipv6_mcast_test = true;
    }
  }
  else
  {
    const EtcPalNetintInfo* arr = etcpal_netint_get_interfaces();
    if (arr)
    {
      for (const EtcPalNetintInfo* netint = arr; netint < arr + etcpal_netint_get_num_interfaces(); ++netint)
      {
        if (ETCPAL_IP_IS_V6(&netint->addr) && !etcpal_ip_is_loopback(&netint->addr) &&
            NULL == strstr(netint->id, "utun"))
        {
          v6_netint = netint->index;
          run_ipv6_mcast_test = true;
          return;
        }
      }
      // We haven't found a network interface...
      TEST_MESSAGE("WARNING: No IPv6 non-loopback network interfaces found. Disabling multicast IPv6 test...");
      run_ipv6_mcast_test = false;
    }
  }
}
#endif
#endif

TEST_GROUP(socket_integration_udp);

TEST_SETUP(socket_integration_udp)
{
  if (etcpal_init_result != kEtcPalErrOk)
  {
    char buf[100];
    sprintf(buf, "This test could not be run because etcpal_init() failed with error '%s'", etcpal_strerror(etcpal_init_result));
    TEST_FAIL_MESSAGE(buf);
  }

  send_sock = ETCPAL_SOCKET_INVALID;
  for (size_t i = 0; i < ETCPAL_BULK_POLL_TEST_NUM_SOCKETS; ++i)
    recv_socks[i] = ETCPAL_SOCKET_INVALID;
}

TEST_TEAR_DOWN(socket_integration_udp)
{
  etcpal_close(send_sock);
  for (size_t i = 0; i < ETCPAL_BULK_POLL_TEST_NUM_SOCKETS; ++i)
    etcpal_close(recv_socks[i]);
}

// For UDP testing we apply strength in numbers to get around the inherent unreliability of UDP.
// Packets are sent in bulk from another thread, and pass if we receive some of them.

static void send_thread(void* arg)
{
  ETCPAL_UNUSED_ARG(arg);

  for (size_t i = 0; i < NUM_TEST_PACKETS; ++i)
    etcpal_sendto(send_sock, (const uint8_t*)kSocketTestMessage, SOCKET_TEST_MESSAGE_LENGTH, 0, &send_addr);
}

#define UNICAST_UDP_PORT_BASE 6000

void unicast_udp_test(etcpal_iptype_t ip_type)
{
  int intval = 500;
  TEST_ASSERT_EQUAL(kEtcPalErrOk,
                    etcpal_setsockopt(recv_socks[0], ETCPAL_SOL_SOCKET, ETCPAL_SO_RCVTIMEO, &intval, sizeof(int)));
  intval = 1;
  TEST_ASSERT_EQUAL(kEtcPalErrOk,
                    etcpal_setsockopt(recv_socks[1], ETCPAL_SOL_SOCKET, ETCPAL_SO_RCVTIMEO, &intval, sizeof(int)));

  EtcPalSockAddr bind_addr;
  etcpal_ip_set_wildcard(ip_type, &bind_addr.ip);
  bind_addr.port = UNICAST_UDP_PORT_BASE;
  // Bind socket 1 to the wildcard address and a specific port.
  TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_bind(recv_socks[0], &bind_addr));
  // Bind socket 2 to the wildcard address and a different port.
  bind_addr.port = UNICAST_UDP_PORT_BASE + 1;
  TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_bind(recv_socks[1], &bind_addr));

  send_addr.port = UNICAST_UDP_PORT_BASE;

  // Start the send thread
  EtcPalThreadParams thread_params;
  ETCPAL_THREAD_SET_DEFAULT_PARAMS(&thread_params);
  etcpal_thread_t send_thr_handle;
  TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_thread_create(&send_thr_handle, &thread_params, send_thread, NULL));

  size_t num_packets_received = 0;
  for (size_t i = 0; i < NUM_TEST_PACKETS; ++i)
  {
    EtcPalSockAddr from_addr;
    uint8_t        buf[SOCKET_TEST_MESSAGE_LENGTH + 1];

    int res = etcpal_recvfrom(recv_socks[0], buf, SOCKET_TEST_MESSAGE_LENGTH, 0, &from_addr);
    if (res == SOCKET_TEST_MESSAGE_LENGTH)
    {
      ++num_packets_received;
    }
    else
    {
      TEST_ASSERT(res == kEtcPalErrTimedOut || res == kEtcPalErrWouldBlock);
      break;
    }

    TEST_ASSERT_EQUAL(etcpal_ip_cmp(&send_addr.ip, &from_addr.ip), 0);
    TEST_ASSERT_NOT_EQUAL(from_addr.port, UNICAST_UDP_PORT_BASE);

    buf[SOCKET_TEST_MESSAGE_LENGTH] = '\0';
    TEST_ASSERT_EQUAL_STRING((char*)buf, kSocketTestMessage);
  }

  TEST_ASSERT_GREATER_THAN_UINT(0u, num_packets_received);

  // recvfrom should time out because this socket is bound to a different port and we set the
  // timeout option on this socket.
  EtcPalSockAddr from_addr;
  uint8_t        buf[SOCKET_TEST_MESSAGE_LENGTH + 1];
  TEST_ASSERT_LESS_OR_EQUAL_INT(0, etcpal_recvfrom(recv_socks[1], buf, SOCKET_TEST_MESSAGE_LENGTH, 0, &from_addr));

  // Let the send thread end
  TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_thread_join(&send_thr_handle));
}

TEST(socket_integration_udp, unicast_udp_ipv4)
{
  TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_socket(ETCPAL_AF_INET, ETCPAL_SOCK_DGRAM, &recv_socks[0]));
  TEST_ASSERT_NOT_EQUAL(recv_socks[0], ETCPAL_SOCKET_INVALID);

  TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_socket(ETCPAL_AF_INET, ETCPAL_SOCK_DGRAM, &recv_socks[1]));
  TEST_ASSERT_NOT_EQUAL(recv_socks[1], ETCPAL_SOCKET_INVALID);

  TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_socket(ETCPAL_AF_INET, ETCPAL_SOCK_DGRAM, &send_sock));
  TEST_ASSERT_NOT_EQUAL(send_sock, ETCPAL_SOCKET_INVALID);

  ETCPAL_IP_SET_V4_ADDRESS(&send_addr.ip, 0x7f000001u);

  unicast_udp_test(kEtcPalIpTypeV4);
}

#if ETCPAL_TEST_IPV6
TEST(socket_integration_udp, unicast_udp_ipv6)
{
  TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_socket(ETCPAL_AF_INET6, ETCPAL_SOCK_DGRAM, &recv_socks[0]));
  TEST_ASSERT_NOT_EQUAL(recv_socks[0], ETCPAL_SOCKET_INVALID);

  TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_socket(ETCPAL_AF_INET6, ETCPAL_SOCK_DGRAM, &recv_socks[1]));
  TEST_ASSERT_NOT_EQUAL(recv_socks[1], ETCPAL_SOCKET_INVALID);

  TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_socket(ETCPAL_AF_INET6, ETCPAL_SOCK_DGRAM, &send_sock));
  TEST_ASSERT_NOT_EQUAL(send_sock, ETCPAL_SOCKET_INVALID);

  uint8_t v6_loopback[ETCPAL_IPV6_BYTES];
  memset(&v6_loopback, 0, ETCPAL_IPV6_BYTES);
  v6_loopback[15] = 1;
  ETCPAL_IP_SET_V6_ADDRESS(&send_addr.ip, &v6_loopback);

  unicast_udp_test(kEtcPalIpTypeV6);
}
#endif  // ETCPAL_TEST_IPV6

#define MULTICAST_UDP_PORT_BASE 7000

void multicast_udp_test(void)
{
  int intval = 500;
  TEST_ASSERT_EQUAL(kEtcPalErrOk,
                    etcpal_setsockopt(recv_socks[0], ETCPAL_SOL_SOCKET, ETCPAL_SO_RCVTIMEO, &intval, sizeof(int)));
  intval = 1;
  TEST_ASSERT_EQUAL(kEtcPalErrOk,
                    etcpal_setsockopt(recv_socks[1], ETCPAL_SOL_SOCKET, ETCPAL_SO_RCVTIMEO, &intval, sizeof(int)));

  // Start the send thread
  EtcPalThreadParams thread_params;
  ETCPAL_THREAD_SET_DEFAULT_PARAMS(&thread_params);
  etcpal_thread_t send_thr_handle;
  TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_thread_create(&send_thr_handle, &thread_params, send_thread, NULL));

  size_t num_packets_received = 0;
  for (size_t i = 0; i < NUM_TEST_PACKETS; ++i)
  {
    EtcPalSockAddr from_addr;
    uint8_t        buf[SOCKET_TEST_MESSAGE_LENGTH + 1];

    int res = etcpal_recvfrom(recv_socks[0], buf, SOCKET_TEST_MESSAGE_LENGTH, 0, &from_addr);
    if (res == SOCKET_TEST_MESSAGE_LENGTH)
    {
      ++num_packets_received;
    }
    else
    {
      TEST_ASSERT(res == kEtcPalErrTimedOut || res == kEtcPalErrWouldBlock);
      break;
    }

    TEST_ASSERT_NOT_EQUAL(from_addr.port, MULTICAST_UDP_PORT_BASE);

    buf[SOCKET_TEST_MESSAGE_LENGTH] = '\0';
    TEST_ASSERT_EQUAL_STRING((char*)buf, kSocketTestMessage);
  }
  TEST_ASSERT_GREATER_THAN(0u, num_packets_received);

  EtcPalSockAddr from_addr;
  uint8_t        buf[SOCKET_TEST_MESSAGE_LENGTH + 1];
  // recvfrom should time out because this socket is bound to a different port and we set the
  // timeout option on this socket.
  TEST_ASSERT_LESS_OR_EQUAL_INT(0, etcpal_recvfrom(recv_socks[1], buf, SOCKET_TEST_MESSAGE_LENGTH, 0, &from_addr));

  // Let the send thread end
  TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_thread_join(&send_thr_handle));
}

TEST(socket_integration_udp, multicast_udp_ipv4)
{
  EtcPalSockAddr bind_addr;

  TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_socket(ETCPAL_AF_INET, ETCPAL_SOCK_DGRAM, &recv_socks[0]));
  TEST_ASSERT_NOT_EQUAL(recv_socks[0], ETCPAL_SOCKET_INVALID);

  TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_socket(ETCPAL_AF_INET, ETCPAL_SOCK_DGRAM, &recv_socks[1]));
  TEST_ASSERT_NOT_EQUAL(recv_socks[1], ETCPAL_SOCKET_INVALID);

  int intval = 1;
  TEST_ASSERT_EQUAL(kEtcPalErrOk,
                    etcpal_setsockopt(recv_socks[0], ETCPAL_SOL_SOCKET, ETCPAL_SO_REUSEADDR, &intval, sizeof(int)));
  TEST_ASSERT_EQUAL(kEtcPalErrOk,
                    etcpal_setsockopt(recv_socks[1], ETCPAL_SOL_SOCKET, ETCPAL_SO_REUSEADDR, &intval, sizeof(int)));

  TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_socket(ETCPAL_AF_INET, ETCPAL_SOCK_DGRAM, &send_sock));
  TEST_ASSERT_NOT_EQUAL(send_sock, ETCPAL_SOCKET_INVALID);

  TEST_ASSERT_EQUAL(kEtcPalErrOk,
                    etcpal_setsockopt(send_sock, ETCPAL_IPPROTO_IP, ETCPAL_IP_MULTICAST_LOOP, &intval, sizeof(int)));
  TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_setsockopt(send_sock, ETCPAL_IPPROTO_IP, ETCPAL_IP_MULTICAST_IF, &v4_netint,
                                                    sizeof v4_netint));

  // Bind socket 1 to the wildcard address and a specific port.
  etcpal_ip_set_wildcard(kEtcPalIpTypeV4, &bind_addr.ip);
  bind_addr.port = MULTICAST_UDP_PORT_BASE;
  TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_bind(recv_socks[0], &bind_addr));

  // Bind socket 2 to the wildcard address and a different port.
  bind_addr.port = MULTICAST_UDP_PORT_BASE + 1;
  TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_bind(recv_socks[1], &bind_addr));

  // Subscribe socket 1 to the multicast address.
  EtcPalGroupReq greq;
  greq.ifindex = v4_netint;
  ETCPAL_IP_SET_V4_ADDRESS(&greq.group, kTestMcastAddrIPv4);
  TEST_ASSERT_EQUAL(kEtcPalErrOk,
                    etcpal_setsockopt(recv_socks[0], ETCPAL_IPPROTO_IP, ETCPAL_MCAST_JOIN_GROUP, &greq, sizeof greq));

  // Subscribe socket 2 to the multicast address
  TEST_ASSERT_EQUAL(kEtcPalErrOk,
                    etcpal_setsockopt(recv_socks[1], ETCPAL_IPPROTO_IP, ETCPAL_MCAST_JOIN_GROUP, &greq, sizeof greq));

  ETCPAL_IP_SET_V4_ADDRESS(&send_addr.ip, kTestMcastAddrIPv4);
  send_addr.port = MULTICAST_UDP_PORT_BASE;

  multicast_udp_test();
}

#if ETCPAL_TEST_IPV6
TEST(socket_integration_udp, multicast_udp_ipv6)
{
  EtcPalSockAddr bind_addr;

  TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_socket(ETCPAL_AF_INET6, ETCPAL_SOCK_DGRAM, &recv_socks[0]));
  TEST_ASSERT_NOT_EQUAL(recv_socks[0], ETCPAL_SOCKET_INVALID);

  TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_socket(ETCPAL_AF_INET6, ETCPAL_SOCK_DGRAM, &recv_socks[1]));
  TEST_ASSERT_NOT_EQUAL(recv_socks[1], ETCPAL_SOCKET_INVALID);

  int intval = 1;
  TEST_ASSERT_EQUAL(kEtcPalErrOk,
                    etcpal_setsockopt(recv_socks[0], ETCPAL_SOL_SOCKET, ETCPAL_SO_REUSEADDR, &intval, sizeof(int)));
  TEST_ASSERT_EQUAL(kEtcPalErrOk,
                    etcpal_setsockopt(recv_socks[1], ETCPAL_SOL_SOCKET, ETCPAL_SO_REUSEADDR, &intval, sizeof(int)));

  TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_socket(ETCPAL_AF_INET6, ETCPAL_SOCK_DGRAM, &send_sock));
  TEST_ASSERT_NOT_EQUAL(send_sock, ETCPAL_SOCKET_INVALID);

  // TEST_ASSERT_EQUAL(kEtcPalErrOk,
  etcpal_setsockopt(send_sock, ETCPAL_IPPROTO_IPV6, ETCPAL_IP_MULTICAST_LOOP, &intval, sizeof(int));  //);
  // TEST_ASSERT_EQUAL(kEtcPalErrOk,
  etcpal_setsockopt(send_sock, ETCPAL_IPPROTO_IPV6, ETCPAL_IP_MULTICAST_IF, &v6_netint, sizeof v6_netint);  //);

  // Bind socket 1 to the wildcard address and a specific port.
  etcpal_ip_set_wildcard(kEtcPalIpTypeV6, &bind_addr.ip);
  bind_addr.port = MULTICAST_UDP_PORT_BASE;
  TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_bind(recv_socks[0], &bind_addr));

  // Bind socket 2 to the wildcard address and a different port.
  bind_addr.port = MULTICAST_UDP_PORT_BASE + 1;
  TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_bind(recv_socks[1], &bind_addr));

  // Subscribe socket 1 to the multicast address.
  EtcPalGroupReq greq;
  greq.ifindex = v6_netint;
  ETCPAL_IP_SET_V6_ADDRESS(&greq.group, kTestMcastAddrIPv6);
  TEST_ASSERT_EQUAL(kEtcPalErrOk,
                    etcpal_setsockopt(recv_socks[0], ETCPAL_IPPROTO_IPV6, ETCPAL_MCAST_JOIN_GROUP, &greq, sizeof greq));

  // Subscribe socket 2 to the multicast address
  TEST_ASSERT_EQUAL(kEtcPalErrOk,
                    etcpal_setsockopt(recv_socks[1], ETCPAL_IPPROTO_IPV6, ETCPAL_MCAST_JOIN_GROUP, &greq, sizeof greq));

  ETCPAL_IP_SET_V6_ADDRESS_WITH_SCOPE_ID(&send_addr.ip, kTestMcastAddrIPv6, v6_netint);
  send_addr.port = MULTICAST_UDP_PORT_BASE;

  multicast_udp_test();
}
#endif  // ETCPAL_TEST_IPV6

// Test to make sure etcpal_poll_* functions work properly with a large number of sockets.
// (Tests the maximum number defined by ETCPAL_SOCKET_MAX_POLL_SIZE if that number is well-defined and
// reasonable).
TEST(socket_integration_udp, bulk_poll)
{
  EtcPalPollContext context;
  TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_poll_context_init(&context));

  static uint16_t bind_ports[ETCPAL_BULK_POLL_TEST_NUM_SOCKETS];
  for (size_t i = 0; i < ETCPAL_BULK_POLL_TEST_NUM_SOCKETS; ++i)
  {
    char error_msg[50];
    sprintf(error_msg, "Failed on iteration %zu", i);

    TEST_ASSERT_EQUAL_MESSAGE(kEtcPalErrOk, etcpal_socket(ETCPAL_AF_INET, ETCPAL_SOCK_DGRAM, &recv_socks[i]),
                              error_msg);
    TEST_ASSERT_NOT_EQUAL_MESSAGE(recv_socks[i], ETCPAL_SOCKET_INVALID, error_msg);

    EtcPalSockAddr bind_addr;
    etcpal_ip_set_wildcard(kEtcPalIpTypeV4, &bind_addr.ip);
    bind_addr.port = 0;
    TEST_ASSERT_EQUAL_MESSAGE(kEtcPalErrOk, etcpal_bind(recv_socks[i], &bind_addr), error_msg);
    TEST_ASSERT_EQUAL_MESSAGE(kEtcPalErrOk, etcpal_getsockname(recv_socks[i], &bind_addr), error_msg);
    bind_ports[i] = bind_addr.port;

    TEST_ASSERT_EQUAL_MESSAGE(kEtcPalErrOk, etcpal_poll_add_socket(&context, recv_socks[i], ETCPAL_POLL_IN, NULL),
                              error_msg);
  }

  TEST_ASSERT_EQUAL(kEtcPalErrOk, etcpal_socket(ETCPAL_AF_INET, ETCPAL_SOCK_DGRAM, &send_sock));

  ETCPAL_IP_SET_V4_ADDRESS(&send_addr.ip, 0x7f000001);
  for (size_t i = 0; i < ETCPAL_BULK_POLL_TEST_NUM_SOCKETS; ++i)
  {
    char error_msg[50];
    sprintf(error_msg, "Failed on iteration %zu", i);

    // Send to each socket in turn and make sure poll works for it
    send_addr.port = bind_ports[i];
    etcpal_sendto(send_sock, kSocketTestMessage, SOCKET_TEST_MESSAGE_LENGTH, 0, &send_addr);

    EtcPalPollEvent event;
    TEST_ASSERT_EQUAL_MESSAGE(kEtcPalErrOk, etcpal_poll_wait(&context, &event, 100), error_msg);
    TEST_ASSERT_EQUAL_MESSAGE(event.socket, recv_socks[i], error_msg);
    TEST_ASSERT_EQUAL_MESSAGE(event.events, ETCPAL_POLL_IN, error_msg);
    TEST_ASSERT_EQUAL_MESSAGE(event.err, kEtcPalErrOk, error_msg);

    uint8_t recv_buf[SOCKET_TEST_MESSAGE_LENGTH];
    TEST_ASSERT_GREATER_THAN(0, etcpal_recvfrom(recv_socks[i], recv_buf, SOCKET_TEST_MESSAGE_LENGTH, 0, NULL));
  }

  etcpal_poll_context_deinit(&context);
}

TEST_GROUP_RUNNER(socket_integration_udp)
{
  etcpal_init_result = etcpal_init(ETCPAL_FEATURE_SOCKETS | ETCPAL_FEATURE_NETINTS);

  if (etcpal_init_result == kEtcPalErrOk)
  {
#if !ETCPAL_TEST_DISABLE_MCAST_INTEGRATION_TESTS
    select_network_interface_v4();
#if ETCPAL_TEST_IPV6
    select_network_interface_v6();
#endif
#endif
  }

  RUN_TEST_CASE(socket_integration_udp, unicast_udp_ipv4);
  if (run_ipv4_mcast_test)
    RUN_TEST_CASE(socket_integration_udp, multicast_udp_ipv4);
#if ETCPAL_TEST_IPV6
  RUN_TEST_CASE(socket_integration_udp, unicast_udp_ipv6);
  if (run_ipv6_mcast_test)
    RUN_TEST_CASE(socket_integration_udp, multicast_udp_ipv6);
#endif

  RUN_TEST_CASE(socket_integration_udp, bulk_poll);

  if (etcpal_init_result == kEtcPalErrOk)
    etcpal_deinit(ETCPAL_FEATURE_SOCKETS | ETCPAL_FEATURE_NETINTS);
}
