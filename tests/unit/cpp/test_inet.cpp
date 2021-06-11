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

#include "etcpal/cpp/inet.h"
#include "unity_fixture.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <string>

static void ConvertStringToLowercase(std::string& str)
{
  std::transform(str.begin(), str.end(), str.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
}

extern "C" {

TEST_GROUP(etcpal_cpp_inet);

static EtcPalIpAddr ipv4_1;
static EtcPalIpAddr ipv4_2;
static EtcPalIpAddr ipv6_1;
static EtcPalIpAddr ipv6_2;

static EtcPalSockAddr sock_v4_port_1;
static EtcPalSockAddr sock_v4_port_2;
static EtcPalSockAddr sock_v6_port_1;

TEST_SETUP(etcpal_cpp_inet)
{
  ETCPAL_IP_SET_V4_ADDRESS(&ipv4_1, 0x00000001);
  ETCPAL_IP_SET_V4_ADDRESS(&ipv4_2, 0x00000002);
  std::array<uint8_t, 16> v6_data{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
  ETCPAL_IP_SET_V6_ADDRESS(&ipv6_1, v6_data.data());
  v6_data[15] = 2;
  ETCPAL_IP_SET_V6_ADDRESS(&ipv6_2, v6_data.data());

  ETCPAL_IP_SET_V4_ADDRESS(&sock_v4_port_1.ip, 0x00000001);
  sock_v4_port_1.port = 8888;
  ETCPAL_IP_SET_V4_ADDRESS(&sock_v4_port_2.ip, 0x00000001);
  sock_v4_port_2.port = 8889;
  ETCPAL_IP_SET_V6_ADDRESS(&sock_v6_port_1.ip, v6_data.data());
  sock_v6_port_1.port = 8888;
}

TEST_TEAR_DOWN(etcpal_cpp_inet)
{
}

TEST(etcpal_cpp_inet, c_ipaddr_equality_operators_work)
{
  // Items should equal themselves
  TEST_ASSERT_TRUE(ipv4_1 == ipv4_1);
  TEST_ASSERT_TRUE(ipv4_2 == ipv4_2);
  TEST_ASSERT_TRUE(ipv6_1 == ipv6_1);
  TEST_ASSERT_TRUE(ipv6_2 == ipv6_2);
  TEST_ASSERT_FALSE(ipv4_1 != ipv4_1);
  TEST_ASSERT_FALSE(ipv6_1 != ipv6_1);
  TEST_ASSERT_FALSE(ipv4_2 != ipv4_2);
  TEST_ASSERT_FALSE(ipv6_2 != ipv6_2);

  // IPv4 addresses should not equal IPv6 addresses
  TEST_ASSERT_FALSE(ipv4_1 == ipv6_1);
  TEST_ASSERT_FALSE(ipv6_1 == ipv4_1);
  TEST_ASSERT_TRUE(ipv4_1 != ipv6_1);
  TEST_ASSERT_TRUE(ipv6_1 != ipv4_1);

  // IPv4 and IPv6 addresses with different address values should not equal each other
  TEST_ASSERT_FALSE(ipv4_1 == ipv4_2);
  TEST_ASSERT_FALSE(ipv6_1 == ipv6_2);
  TEST_ASSERT_TRUE(ipv4_1 != ipv4_2);
  TEST_ASSERT_TRUE(ipv6_1 != ipv6_2);
}

TEST(etcpal_cpp_inet, c_ipaddr_comparison_operators_work)  // NOLINT(readability-function-cognitive-complexity)
{
  // Items should not be < or > themselves and should be <= and >=
  TEST_ASSERT_FALSE(ipv4_1 < ipv4_1);
  TEST_ASSERT_FALSE(ipv4_1 > ipv4_1);
  TEST_ASSERT_TRUE(ipv4_1 <= ipv4_1);
  TEST_ASSERT_TRUE(ipv4_1 >= ipv4_1);

  // Items of the same IP protocol and different addresses should compare correctly
  TEST_ASSERT_TRUE(ipv4_1 < ipv4_2);
  TEST_ASSERT_FALSE(ipv4_1 > ipv4_2);
  TEST_ASSERT_TRUE(ipv4_1 <= ipv4_2);
  TEST_ASSERT_FALSE(ipv4_1 >= ipv4_2);
  TEST_ASSERT_TRUE(ipv6_1 < ipv6_2);
  TEST_ASSERT_FALSE(ipv6_1 > ipv6_2);
  TEST_ASSERT_TRUE(ipv6_1 <= ipv6_2);
  TEST_ASSERT_FALSE(ipv6_1 >= ipv6_2);

  // All IPv4 addresses should compare less than all IPv6 addresses
  TEST_ASSERT_TRUE(ipv4_1 < ipv6_1);
  TEST_ASSERT_FALSE(ipv4_1 > ipv6_1);
  TEST_ASSERT_TRUE(ipv4_1 <= ipv6_1);
  TEST_ASSERT_FALSE(ipv4_1 >= ipv6_1);
  TEST_ASSERT_TRUE(ipv4_2 < ipv6_1);
  TEST_ASSERT_FALSE(ipv4_2 > ipv6_1);
  TEST_ASSERT_TRUE(ipv4_2 <= ipv6_1);
  TEST_ASSERT_FALSE(ipv4_2 >= ipv6_1);
  TEST_ASSERT_TRUE(ipv4_1 < ipv6_2);
  TEST_ASSERT_FALSE(ipv4_1 > ipv6_2);
  TEST_ASSERT_TRUE(ipv4_1 <= ipv6_2);
  TEST_ASSERT_FALSE(ipv4_1 >= ipv6_2);
  TEST_ASSERT_TRUE(ipv4_2 < ipv6_2);
  TEST_ASSERT_FALSE(ipv4_2 > ipv6_2);
  TEST_ASSERT_TRUE(ipv4_2 <= ipv6_2);
  TEST_ASSERT_FALSE(ipv4_2 >= ipv6_2);
}

TEST(etcpal_cpp_inet, c_sockaddr_equality_operators_work)
{
  // Items should equal themselves
  TEST_ASSERT_TRUE(sock_v4_port_1 == sock_v4_port_1);
  TEST_ASSERT_TRUE(sock_v4_port_2 == sock_v4_port_2);
  TEST_ASSERT_TRUE(sock_v6_port_1 == sock_v6_port_1);
  TEST_ASSERT_FALSE(sock_v4_port_1 != sock_v4_port_1);
  TEST_ASSERT_FALSE(sock_v6_port_1 != sock_v6_port_1);
  TEST_ASSERT_FALSE(sock_v4_port_2 != sock_v4_port_2);

  // Sockaddrs with different ports should not equal each other
  TEST_ASSERT_FALSE(sock_v4_port_1 == sock_v4_port_2);
  TEST_ASSERT_TRUE(sock_v4_port_1 != sock_v4_port_2);

  // Sockaddrs with different address values should not equal each other
  TEST_ASSERT_FALSE(sock_v4_port_1 == sock_v6_port_1);
  TEST_ASSERT_FALSE(sock_v6_port_1 == sock_v4_port_1);
  TEST_ASSERT_TRUE(sock_v4_port_1 != sock_v6_port_1);
  TEST_ASSERT_TRUE(sock_v6_port_1 != sock_v4_port_1);
}

TEST(etcpal_cpp_inet, c_sockaddr_comparison_operators_work)
{
  // Items should not be < or > themselves and should be <= and >=
  TEST_ASSERT_FALSE(sock_v4_port_1 < sock_v4_port_1);
  TEST_ASSERT_FALSE(sock_v4_port_1 > sock_v4_port_1);
  TEST_ASSERT_TRUE(sock_v4_port_1 <= sock_v4_port_1);
  TEST_ASSERT_TRUE(sock_v4_port_1 >= sock_v4_port_1);

  // Items with the same address and different ports should compare correctly
  TEST_ASSERT_TRUE(sock_v4_port_1 < sock_v4_port_2);
  TEST_ASSERT_FALSE(sock_v4_port_1 > sock_v4_port_2);
  TEST_ASSERT_TRUE(sock_v4_port_1 <= sock_v4_port_2);
  TEST_ASSERT_FALSE(sock_v4_port_1 >= sock_v4_port_2);

  // Items with the same port and different addresses should compare correctly
  TEST_ASSERT_TRUE(sock_v4_port_1 < sock_v6_port_1);
  TEST_ASSERT_FALSE(sock_v4_port_1 > sock_v6_port_1);
  TEST_ASSERT_TRUE(sock_v4_port_1 <= sock_v6_port_1);
  TEST_ASSERT_FALSE(sock_v4_port_1 >= sock_v6_port_1);
}

TEST(etcpal_cpp_inet, ip_default_constructor_works)
{
  const etcpal::IpAddr ip;
  TEST_ASSERT_FALSE(ip.IsValid());
  TEST_ASSERT_FALSE(ip.IsV4());
  TEST_ASSERT_FALSE(ip.IsV6());
  TEST_ASSERT_TRUE(ip.type() == etcpal::IpAddrType::kInvalid);
}

TEST(etcpal_cpp_inet, ip_copy_constructors_work)
{
  EtcPalIpAddr c_ip;
  ETCPAL_IP_SET_V4_ADDRESS(&c_ip, 0xdeadbeef);
  const etcpal::IpAddr ip1(c_ip);
  TEST_ASSERT_TRUE(ip1.get() == c_ip);

  // Use the default copy constructor
  const etcpal::IpAddr ip2(ip1);
  TEST_ASSERT_TRUE(ip1 == ip2);
  TEST_ASSERT_TRUE(ip2.IsValid());
}

TEST(etcpal_cpp_inet, ip_assignment_operators_work)
{
  etcpal::IpAddr ip;
  EtcPalIpAddr   c_ip;
  ETCPAL_IP_SET_V4_ADDRESS(&c_ip, 0xdeadbeef);

  // Assign to an EtcPalIpAddr
  ip = c_ip;
  TEST_ASSERT_TRUE(ip.get() == c_ip);
  TEST_ASSERT_TRUE(ip.IsValid());

  // Use the default assignment operator
  const etcpal::IpAddr invalid_ip;
  ip = invalid_ip;
  TEST_ASSERT_TRUE(ip == invalid_ip);
  TEST_ASSERT_TRUE(ip.get() == invalid_ip.get());
  TEST_ASSERT_FALSE(ip.IsValid());
}

TEST(etcpal_cpp_inet, ip_v4_addresses_assigned_properly)
{
  // Using the IPv4 direct data constructor
  const etcpal::IpAddr ip(0xdeadbeef);

  TEST_ASSERT_TRUE(ip.IsValid());
  TEST_ASSERT_TRUE(ip.IsV4());
  TEST_ASSERT_FALSE(ip.IsV6());
  TEST_ASSERT_TRUE(ip.type() == etcpal::IpAddrType::kV4);
  TEST_ASSERT_EQUAL(ip.v4_data(), 0xdeadbeef);

  // Using the SetAddress() function
  etcpal::IpAddr ip2;
  ip2.SetAddress(0xdeadbeef);

  TEST_ASSERT_TRUE(ip2.IsValid());
  TEST_ASSERT_TRUE(ip2.IsV4());
  TEST_ASSERT_FALSE(ip2.IsV6());
  TEST_ASSERT_TRUE(ip2.type() == etcpal::IpAddrType::kV4);
  TEST_ASSERT_EQUAL(ip2.v4_data(), 0xdeadbeef);
}

static const std::array<uint8_t, ETCPAL_IPV6_BYTES> kTestIpv6Data{0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00,
                                                                  0x00, 0x00, 0x00, 0x00, 0x12, 0x34, 0x56, 0x78};

TEST(etcpal_cpp_inet, ip_v6_addresses_assigned_properly)
{
  // Using the IPv6 direct data constructor
  const etcpal::IpAddr ip(kTestIpv6Data.data());

  TEST_ASSERT_TRUE(ip.IsValid());
  TEST_ASSERT_TRUE(ip.IsV6());
  TEST_ASSERT_FALSE(ip.IsV4());
  TEST_ASSERT_TRUE(ip.type() == etcpal::IpAddrType::kV6);
  TEST_ASSERT_EQUAL(0, std::memcmp(kTestIpv6Data.data(), ip.v6_data(), ETCPAL_IPV6_BYTES));
  TEST_ASSERT_TRUE(ip.ToV6Array() == kTestIpv6Data);
  TEST_ASSERT_EQUAL(ip.scope_id(), 0);

  // Using the IPv6 direct data constructor with a scope ID
  const etcpal::IpAddr ip2(kTestIpv6Data.data(), 12);

  TEST_ASSERT_TRUE(ip2.IsValid());
  TEST_ASSERT_TRUE(ip2.IsV6());
  TEST_ASSERT_FALSE(ip2.IsV4());
  TEST_ASSERT_TRUE(ip2.type() == etcpal::IpAddrType::kV6);
  TEST_ASSERT_EQUAL(0, std::memcmp(kTestIpv6Data.data(), ip2.v6_data(), ETCPAL_IPV6_BYTES));
  TEST_ASSERT_TRUE(ip2.ToV6Array() == kTestIpv6Data);
  TEST_ASSERT_EQUAL(ip2.scope_id(), 12);

  // Using the SetAddress() function
  etcpal::IpAddr ip3;
  ip3.SetAddress(kTestIpv6Data.data());

  TEST_ASSERT_TRUE(ip3.IsValid());
  TEST_ASSERT_TRUE(ip3.IsV6());
  TEST_ASSERT_FALSE(ip3.IsV4());
  TEST_ASSERT_TRUE(ip3.type() == etcpal::IpAddrType::kV6);
  TEST_ASSERT_EQUAL(0, std::memcmp(kTestIpv6Data.data(), ip3.v6_data(), ETCPAL_IPV6_BYTES));
  TEST_ASSERT_TRUE(ip3.ToV6Array() == kTestIpv6Data);
  TEST_ASSERT_EQUAL(ip3.scope_id(), 0);

  // Use the SetAddress() function with a scope ID
  etcpal::IpAddr ip4;
  ip4.SetAddress(kTestIpv6Data.data(), 12);
  TEST_ASSERT_TRUE(ip4.IsValid());
  TEST_ASSERT_TRUE(ip4.IsV6());
  TEST_ASSERT_FALSE(ip4.IsV4());
  TEST_ASSERT_TRUE(ip4.type() == etcpal::IpAddrType::kV6);
  TEST_ASSERT_EQUAL(0, std::memcmp(kTestIpv6Data.data(), ip4.v6_data(), ETCPAL_IPV6_BYTES));
  TEST_ASSERT_TRUE(ip4.ToV6Array() == kTestIpv6Data);
  TEST_ASSERT_EQUAL(ip4.scope_id(), 12);
}

TEST(etcpal_cpp_inet, ip_to_string_works)
{
  const etcpal::IpAddr ip(0xdeadbeef);
  TEST_ASSERT_EQUAL_STRING(ip.ToString().c_str(), "222.173.190.239");

  const etcpal::IpAddr ip2(kTestIpv6Data.data());
  auto                 ip_str = ip2.ToString();
  ConvertStringToLowercase(ip_str);
  TEST_ASSERT_EQUAL_STRING(ip_str.c_str(), "2001:db8::1234:5678");
}

// We do more rigorous testing of the string conversion functions in the core C unit tests, so we
// will only test one bad string in this one.
TEST(etcpal_cpp_inet, ip_from_string_works)
{
  auto v4_good = etcpal::IpAddr::FromString("10.101.20.30");
  TEST_ASSERT_TRUE(v4_good.IsValid());
  TEST_ASSERT_TRUE(v4_good.IsV4());
  TEST_ASSERT_FALSE(v4_good.IsV6());
  TEST_ASSERT_EQUAL(v4_good.v4_data(), 0x0a65141e);

  v4_good = etcpal::IpAddr::FromString(std::string{"10.101.20.30"});
  TEST_ASSERT_TRUE(v4_good.IsValid());
  TEST_ASSERT_TRUE(v4_good.IsV4());
  TEST_ASSERT_FALSE(v4_good.IsV6());
  TEST_ASSERT_EQUAL(v4_good.v4_data(), 0x0a65141e);

  auto v6_good = etcpal::IpAddr::FromString("2001:db8::1234:5678");
  TEST_ASSERT_TRUE(v6_good.IsValid());
  TEST_ASSERT_TRUE(v6_good.IsV6());
  TEST_ASSERT_FALSE(v6_good.IsV4());
  TEST_ASSERT_EQUAL(0, std::memcmp(v6_good.v6_data(), kTestIpv6Data.data(), ETCPAL_IPV6_BYTES));

  v6_good = etcpal::IpAddr::FromString(std::string{"2001:db8::1234:5678"});
  TEST_ASSERT_TRUE(v6_good.IsValid());
  TEST_ASSERT_TRUE(v6_good.IsV6());
  TEST_ASSERT_FALSE(v6_good.IsV4());
  TEST_ASSERT_EQUAL(0, std::memcmp(v6_good.v6_data(), kTestIpv6Data.data(), ETCPAL_IPV6_BYTES));

  auto ip_bad = etcpal::IpAddr::FromString("Bad string");
  TEST_ASSERT_FALSE(ip_bad.IsValid());

  ip_bad = etcpal::IpAddr::FromString(std::string{"Bad string"});
  TEST_ASSERT_FALSE(ip_bad.IsValid());
}

// More rigorous testing is done in the C unit tests, we just do one test here
TEST(etcpal_cpp_inet, ip_is_link_local_works)
{
  const auto ip = etcpal::IpAddr::FromString("169.254.1.2");
  TEST_ASSERT_TRUE(ip.IsLinkLocal());
  TEST_ASSERT_FALSE(ip.IsLoopback());
  TEST_ASSERT_FALSE(ip.IsMulticast());
  TEST_ASSERT_FALSE(ip.IsWildcard());
}

// More rigorous testing is done in the C unit tests, we just do one test here
TEST(etcpal_cpp_inet, ip_is_loopback_works)
{
  const auto ip = etcpal::IpAddr::FromString("127.0.0.1");
  TEST_ASSERT_TRUE(ip.IsLoopback());
  TEST_ASSERT_FALSE(ip.IsLinkLocal());
  TEST_ASSERT_FALSE(ip.IsMulticast());
  TEST_ASSERT_FALSE(ip.IsWildcard());
}

// More rigorous testing is done in the C unit tests, we just do one test here
TEST(etcpal_cpp_inet, ip_is_multicast_works)
{
  const auto ip = etcpal::IpAddr::FromString("239.154.2.3");
  TEST_ASSERT_TRUE(ip.IsMulticast());
  TEST_ASSERT_FALSE(ip.IsLinkLocal());
  TEST_ASSERT_FALSE(ip.IsLoopback());
  TEST_ASSERT_FALSE(ip.IsWildcard());
}

// More rigorous testing is done in the C unit tests, we just do a few tests here
TEST(etcpal_cpp_inet, ip_is_wildcard_works)
{
  auto wildcard = etcpal::IpAddr::WildcardV4();
  TEST_ASSERT_TRUE(wildcard.IsWildcard());
  TEST_ASSERT_FALSE(wildcard.IsLinkLocal());
  TEST_ASSERT_FALSE(wildcard.IsLoopback());
  TEST_ASSERT_FALSE(wildcard.IsMulticast());

  wildcard = etcpal::IpAddr::WildcardV6();
  TEST_ASSERT_TRUE(wildcard.IsWildcard());
  TEST_ASSERT_FALSE(wildcard.IsLinkLocal());
  TEST_ASSERT_FALSE(wildcard.IsLoopback());
  TEST_ASSERT_FALSE(wildcard.IsMulticast());

  wildcard = etcpal::IpAddr::Wildcard(etcpal::IpAddrType::kV4);
  TEST_ASSERT_TRUE(wildcard.IsWildcard());
  TEST_ASSERT_FALSE(wildcard.IsLinkLocal());
  TEST_ASSERT_FALSE(wildcard.IsLoopback());
  TEST_ASSERT_FALSE(wildcard.IsMulticast());

  wildcard = etcpal::IpAddr::Wildcard(etcpal::IpAddrType::kV6);
  TEST_ASSERT_TRUE(wildcard.IsWildcard());
  TEST_ASSERT_FALSE(wildcard.IsLinkLocal());
  TEST_ASSERT_FALSE(wildcard.IsLoopback());
  TEST_ASSERT_FALSE(wildcard.IsMulticast());
}

// More rigorous testing is done in the C unit tests, we just do a few tests here
TEST(etcpal_cpp_inet, ip_mask_length_works)
{
  auto ip = etcpal::IpAddr::FromString("255.254.0.0");
  TEST_ASSERT_EQUAL_UINT(ip.MaskLength(), 15);

  ip = etcpal::IpAddr::FromString("ffff:ffff:ffff:fffe::");
  TEST_ASSERT_EQUAL_UINT(ip.MaskLength(), 63);
}

// More rigorous testing is done in the C unit tests, we just do a few tests here
TEST(etcpal_cpp_inet, ip_netmask_from_length_works)
{
  auto ip = etcpal::IpAddr::NetmaskV4(20);
  TEST_ASSERT_EQUAL(ip.MaskLength(), 20);
  ip = etcpal::IpAddr::NetmaskV6(127);
  TEST_ASSERT_EQUAL(ip.MaskLength(), 127);
  ip = etcpal::IpAddr::Netmask(etcpal::IpAddrType::kV4, 32);
  TEST_ASSERT_EQUAL(ip.MaskLength(), 32);
  ip = etcpal::IpAddr::Netmask(etcpal::IpAddrType::kV6, 128);
  TEST_ASSERT_EQUAL(ip.MaskLength(), 128);
}

TEST(etcpal_cpp_inet, sockaddr_default_constructor_works)
{
  etcpal::SockAddr sockaddr;
  TEST_ASSERT_FALSE(sockaddr.IsValid());
  TEST_ASSERT_EQUAL_UINT16(sockaddr.port(), 0);
}

TEST(etcpal_cpp_inet, sockaddr_copy_constructors_work)
{
  EtcPalSockAddr c_sa;
  ETCPAL_IP_SET_V4_ADDRESS(&c_sa.ip, 0xdeadbeef);
  c_sa.port = 0xfeed;

  const etcpal::SockAddr sockaddr1(c_sa);
  TEST_ASSERT_TRUE(sockaddr1.get() == c_sa);
  TEST_ASSERT_TRUE(sockaddr1.IsValid());
  TEST_ASSERT_EQUAL_UINT32(sockaddr1.v4_data(), 0xdeadbeef);
  TEST_ASSERT_EQUAL_UINT16(sockaddr1.port(), 0xfeed);

  // Use the default copy constructor
  const etcpal::SockAddr sockaddr2(sockaddr1);
  TEST_ASSERT_TRUE(sockaddr1 == sockaddr2);
}

TEST(etcpal_cpp_inet, sockaddr_assignment_operators_work)
{
  etcpal::SockAddr sockaddr;
  EtcPalSockAddr   c_sa;
  ETCPAL_IP_SET_V4_ADDRESS(&c_sa.ip, 0xdeadbeef);
  c_sa.port = 0xfeed;

  // Assign to an EtcPalIpAddr
  sockaddr = c_sa;
  TEST_ASSERT_TRUE(sockaddr.get() == c_sa);
  TEST_ASSERT_TRUE(sockaddr.IsValid());

  // Use the default assignment operator
  const etcpal::SockAddr invalid_sa;
  sockaddr = invalid_sa;
  TEST_ASSERT_TRUE(sockaddr == invalid_sa);
  TEST_ASSERT_TRUE(sockaddr.get() == invalid_sa.get());
  TEST_ASSERT_FALSE(sockaddr.IsValid());
}

TEST(etcpal_cpp_inet, sockaddr_custom_constructors_work)
{
  // Construct from raw IPv4 data and port number
  const etcpal::SockAddr sockaddr1(0xdeadbeef, 0xfeed);
  TEST_ASSERT_TRUE(sockaddr1.IsValid());
  TEST_ASSERT_TRUE(sockaddr1.IsV4());
  TEST_ASSERT_EQUAL_UINT32(sockaddr1.v4_data(), 0xdeadbeef);
  TEST_ASSERT_EQUAL_UINT16(sockaddr1.port(), 0xfeed);

  // Construct from raw IPv6 data and port number
  const etcpal::SockAddr sockaddr2(kTestIpv6Data.data(), 0xbeef);
  TEST_ASSERT_TRUE(sockaddr2.IsValid());
  TEST_ASSERT_TRUE(sockaddr2.IsV6());
  TEST_ASSERT_EQUAL_UINT8_ARRAY(sockaddr2.v6_data(), kTestIpv6Data.data(), ETCPAL_IPV6_BYTES);
  TEST_ASSERT_EQUAL_UINT16(sockaddr2.port(), 0xbeef);

  // Construct from raw IPv6 data, a scope ID and port number.
  const etcpal::SockAddr sockaddr3(kTestIpv6Data.data(), 12, 0xdead);

  TEST_ASSERT_TRUE(sockaddr3.IsValid());
  TEST_ASSERT_TRUE(sockaddr3.IsV6());
  TEST_ASSERT_EQUAL_UINT8_ARRAY(sockaddr3.v6_data(), kTestIpv6Data.data(), ETCPAL_IPV6_BYTES);
  TEST_ASSERT_EQUAL(sockaddr3.scope_id(), 12);
  TEST_ASSERT_EQUAL_UINT16(sockaddr3.port(), 0xdead);

  // Construct from an IpAddr and port number
  const etcpal::SockAddr sockaddr4(etcpal::IpAddr(0xdeadbeef), 0xfeed);
  TEST_ASSERT_TRUE(sockaddr4.IsValid());
  TEST_ASSERT_TRUE(sockaddr4.IsV4());
  TEST_ASSERT_EQUAL_UINT32(sockaddr4.v4_data(), 0xdeadbeef);
  TEST_ASSERT_EQUAL_UINT16(sockaddr4.port(), 0xfeed);
}

TEST(etcpal_cpp_inet, sockaddr_to_string_works)
{
  const etcpal::SockAddr sockaddr_v4(etcpal::IpAddr::FromString("10.101.2.3"), 5555);
  TEST_ASSERT_EQUAL_STRING(sockaddr_v4.ToString().c_str(), "10.101.2.3:5555");

  const etcpal::SockAddr sockaddr_v6(etcpal::IpAddr::FromString("2001:db8::2222:3333"), 6666);
  auto                   sockaddr_str = sockaddr_v6.ToString();
  ConvertStringToLowercase(sockaddr_str);
  TEST_ASSERT_EQUAL_STRING(sockaddr_str.c_str(), "[2001:db8::2222:3333]:6666");

  const etcpal::SockAddr sockaddr_invalid;
  TEST_ASSERT_TRUE(sockaddr_invalid.ToString().empty());
}

TEST(etcpal_cpp_inet, sockaddr_is_link_local_works)
{
  const auto sockaddr = etcpal::SockAddr(etcpal::IpAddr::FromString("169.254.20.40"), 8888);
  TEST_ASSERT_TRUE(sockaddr.IsLinkLocal());
  TEST_ASSERT_FALSE(sockaddr.IsLoopback());
  TEST_ASSERT_FALSE(sockaddr.IsMulticast());
  TEST_ASSERT_FALSE(sockaddr.IsWildcard());
}

TEST(etcpal_cpp_inet, sockaddr_is_loopback_works)
{
  const auto sockaddr = etcpal::SockAddr(etcpal::IpAddr::FromString("::1"), 4000);
  TEST_ASSERT_TRUE(sockaddr.IsLoopback());
  TEST_ASSERT_FALSE(sockaddr.IsLinkLocal());
  TEST_ASSERT_FALSE(sockaddr.IsMulticast());
  TEST_ASSERT_FALSE(sockaddr.IsWildcard());
}

TEST(etcpal_cpp_inet, sockaddr_is_multicast_works)
{
  const auto sockaddr = etcpal::SockAddr(etcpal::IpAddr::FromString("224.127.0.1"), 1234);
  TEST_ASSERT_TRUE(sockaddr.IsMulticast());
  TEST_ASSERT_FALSE(sockaddr.IsLinkLocal());
  TEST_ASSERT_FALSE(sockaddr.IsLoopback());
  TEST_ASSERT_FALSE(sockaddr.IsWildcard());
}

TEST(etcpal_cpp_inet, sockaddr_is_wildcard_works)
{
  auto wildcard = etcpal::SockAddr(etcpal::IpAddr::WildcardV4(), 2000);
  TEST_ASSERT_TRUE(wildcard.IsWildcard());
  TEST_ASSERT_FALSE(wildcard.IsLinkLocal());
  TEST_ASSERT_FALSE(wildcard.IsLoopback());
  TEST_ASSERT_FALSE(wildcard.IsMulticast());
  wildcard = etcpal::SockAddr(etcpal::IpAddr::WildcardV6(), 3000);
  TEST_ASSERT_TRUE(wildcard.IsWildcard());
  TEST_ASSERT_FALSE(wildcard.IsLinkLocal());
  TEST_ASSERT_FALSE(wildcard.IsLoopback());
  TEST_ASSERT_FALSE(wildcard.IsMulticast());
}

TEST(etcpal_cpp_inet, sockaddr_to_ip_works)
{
  constexpr uint32_t kIpv4AddrVal = 0x0a0b0c0d;

  const auto sockaddr = etcpal::SockAddr(kIpv4AddrVal, 8888);
  const auto ip = sockaddr.ip();
  TEST_ASSERT_TRUE(ip.IsV4());
  TEST_ASSERT_EQUAL_UINT32(ip.v4_data(), kIpv4AddrVal);
}

TEST(etcpal_cpp_inet, mac_default_constructor_works)
{
  const etcpal::MacAddr mac;
  TEST_ASSERT_TRUE(mac.IsNull());
}

TEST(etcpal_cpp_inet, mac_copy_constructors_work)
{
  const EtcPalMacAddr   c_mac = {{1, 2, 3, 4, 5, 6}};
  const etcpal::MacAddr mac(c_mac);
  TEST_ASSERT_FALSE(mac.IsNull());
  TEST_ASSERT_TRUE(mac.get() == c_mac);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(mac.data(), c_mac.data, ETCPAL_MAC_BYTES);

  const etcpal::MacAddr mac2(mac);
  TEST_ASSERT_FALSE(mac2.IsNull());
  TEST_ASSERT_TRUE(mac == mac2);
  TEST_ASSERT_TRUE(mac.get() == mac2.get());
  TEST_ASSERT_EQUAL_UINT8_ARRAY(mac.data(), mac2.data(), ETCPAL_MAC_BYTES);
}

TEST(etcpal_cpp_inet, mac_assignment_operators_work)
{
  etcpal::MacAddr     mac;
  const EtcPalMacAddr c_mac = {{1, 2, 3, 4, 5, 6}};

  // Assign to an EtcPalMacAddr
  mac = c_mac;
  TEST_ASSERT_FALSE(mac.IsNull());
  TEST_ASSERT_TRUE(mac.get() == c_mac);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(mac.data(), c_mac.data, ETCPAL_MAC_BYTES);

  // Use the default assignment operator
  const etcpal::MacAddr null_mac;
  mac = null_mac;
  TEST_ASSERT_TRUE(mac.IsNull());
  TEST_ASSERT_TRUE(mac == null_mac);
}

TEST(etcpal_cpp_inet, mac_custom_constructors_work)
{
  const std::array<uint8_t, ETCPAL_MAC_BYTES> mac_data{6, 5, 4, 3, 2, 1};
  const etcpal::MacAddr                       mac(mac_data.data());

  TEST_ASSERT_FALSE(mac.IsNull());
  TEST_ASSERT_EQUAL_UINT8_ARRAY(mac.data(), mac_data.data(), ETCPAL_MAC_BYTES);
}

TEST(etcpal_cpp_inet, mac_to_string_works)
{
  const etcpal::MacAddr mac({0x1d, 0xee, 0x03, 0xfa, 0x34, 0x60});
  TEST_ASSERT_EQUAL_STRING(mac.ToString().c_str(), "1d:ee:03:fa:34:60");

  const etcpal::MacAddr null_mac;
  TEST_ASSERT_EQUAL_STRING(null_mac.ToString().c_str(), "00:00:00:00:00:00");
}

// We do more rigorous testing of the string conversion functions in the core C unit tests, so we
// will only test one bad string in this one.
TEST(etcpal_cpp_inet, mac_from_string_works)
{
  const std::array<uint8_t, ETCPAL_MAC_BYTES> mac_data{0, 1, 2, 0xab, 0xcd, 0xef};
  const etcpal::MacAddr                       mac = etcpal::MacAddr::FromString("00:01:02:AB:cd:EF");
  TEST_ASSERT_FALSE(mac.IsNull());
  TEST_ASSERT_EQUAL_UINT8_ARRAY(mac.data(), mac_data.data(), ETCPAL_MAC_BYTES);

  const etcpal::MacAddr bad_mac = etcpal::MacAddr::FromString("Bad string");
  TEST_ASSERT_TRUE(bad_mac.IsNull());
}

TEST(etcpal_cpp_inet, mac_to_array_works)
{
  const std::array<uint8_t, ETCPAL_MAC_BYTES> mac_data{0x11, 0x22, 0x33, 0xaa, 0xbb, 0xcc};
  const etcpal::MacAddr                       mac(mac_data.data());
  TEST_ASSERT_TRUE(mac.ToArray() == mac_data);
}

TEST_GROUP_RUNNER(etcpal_cpp_inet)
{
  RUN_TEST_CASE(etcpal_cpp_inet, c_ipaddr_equality_operators_work);
  RUN_TEST_CASE(etcpal_cpp_inet, c_ipaddr_comparison_operators_work);
  RUN_TEST_CASE(etcpal_cpp_inet, c_sockaddr_equality_operators_work);
  RUN_TEST_CASE(etcpal_cpp_inet, c_sockaddr_comparison_operators_work);
  RUN_TEST_CASE(etcpal_cpp_inet, ip_default_constructor_works);
  RUN_TEST_CASE(etcpal_cpp_inet, ip_copy_constructors_work);
  RUN_TEST_CASE(etcpal_cpp_inet, ip_assignment_operators_work);
  RUN_TEST_CASE(etcpal_cpp_inet, ip_v4_addresses_assigned_properly);
  RUN_TEST_CASE(etcpal_cpp_inet, ip_v6_addresses_assigned_properly);
  RUN_TEST_CASE(etcpal_cpp_inet, ip_to_string_works);
  RUN_TEST_CASE(etcpal_cpp_inet, ip_from_string_works);
  RUN_TEST_CASE(etcpal_cpp_inet, ip_is_link_local_works);
  RUN_TEST_CASE(etcpal_cpp_inet, ip_is_loopback_works);
  RUN_TEST_CASE(etcpal_cpp_inet, ip_is_multicast_works);
  RUN_TEST_CASE(etcpal_cpp_inet, ip_is_wildcard_works);
  RUN_TEST_CASE(etcpal_cpp_inet, ip_mask_length_works)
  RUN_TEST_CASE(etcpal_cpp_inet, ip_netmask_from_length_works)
  RUN_TEST_CASE(etcpal_cpp_inet, sockaddr_default_constructor_works);
  RUN_TEST_CASE(etcpal_cpp_inet, sockaddr_copy_constructors_work);
  RUN_TEST_CASE(etcpal_cpp_inet, sockaddr_assignment_operators_work);
  RUN_TEST_CASE(etcpal_cpp_inet, sockaddr_custom_constructors_work);
  RUN_TEST_CASE(etcpal_cpp_inet, sockaddr_to_string_works);
  RUN_TEST_CASE(etcpal_cpp_inet, sockaddr_is_link_local_works);
  RUN_TEST_CASE(etcpal_cpp_inet, sockaddr_is_loopback_works);
  RUN_TEST_CASE(etcpal_cpp_inet, sockaddr_is_multicast_works);
  RUN_TEST_CASE(etcpal_cpp_inet, sockaddr_is_wildcard_works);
  RUN_TEST_CASE(etcpal_cpp_inet, sockaddr_to_ip_works);
  RUN_TEST_CASE(etcpal_cpp_inet, mac_default_constructor_works);
  RUN_TEST_CASE(etcpal_cpp_inet, mac_copy_constructors_work);
  RUN_TEST_CASE(etcpal_cpp_inet, mac_assignment_operators_work);
  RUN_TEST_CASE(etcpal_cpp_inet, mac_custom_constructors_work);
  RUN_TEST_CASE(etcpal_cpp_inet, mac_to_string_works);
  RUN_TEST_CASE(etcpal_cpp_inet, mac_from_string_works);
  RUN_TEST_CASE(etcpal_cpp_inet, mac_to_array_works);
}
}
