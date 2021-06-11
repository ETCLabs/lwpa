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

/// @file etcpal/cpp/inet.h
/// @brief C++ wrapper and utilities for etcpal/inet.h

#ifndef ETCPAL_CPP_INET_H_
#define ETCPAL_CPP_INET_H_

#include <algorithm>
#include <array>
#include <cstring>
#include <iterator>
#include <string>
#include "etcpal/inet.h"
#include "etcpal/cpp/common.h"

namespace etcpal
{
/// @defgroup etcpal_cpp_inet inet (Internet Addressing)
/// @ingroup etcpal_cpp
/// @brief C++ utilities for the @ref etcpal_inet module.
///
/// Provides C++ wrappers for IP addresses (IpAddr), socket addresses (SockAddr) and MAC addresses
/// (MacAddr).

/// @ingroup etcpal_cpp_inet
/// @brief Indicates an IP address family, or an invalid IP address.
enum class IpAddrType
{
  kInvalid = kEtcPalIpTypeInvalid,
  kV4 = kEtcPalIpTypeV4,
  kV6 = kEtcPalIpTypeV6
};

/// @ingroup etcpal_cpp_inet
/// @brief A wrapper class for the EtcPal IP address type.
///
/// Provides C++ syntactic sugar for working with IP addresses.
class IpAddr
{
public:
  ETCPAL_CONSTEXPR_14 IpAddr() noexcept;
  // Note: this constructor is not explicit by design, to allow implicit conversions e.g.
  //   etcpal::IpAddr ip = etcpal_c_function_that_returns_ip();
  constexpr IpAddr(const EtcPalIpAddr& c_ip) noexcept;
  IpAddr& operator=(const EtcPalIpAddr& c_ip) noexcept;

  ETCPAL_CONSTEXPR_14 IpAddr(uint32_t v4_data) noexcept;
  explicit IpAddr(const uint8_t* v6_data) noexcept;
  IpAddr(const uint8_t* v6_data, unsigned long scope_id) noexcept;

  constexpr const EtcPalIpAddr& get() const noexcept;
  ETCPAL_CONSTEXPR_14 EtcPalIpAddr&      get() noexcept;
  std::string                            ToString() const;
  constexpr uint32_t                     v4_data() const noexcept;
  constexpr const uint8_t*               v6_data() const noexcept;
  std::array<uint8_t, ETCPAL_IPV6_BYTES> ToV6Array() const;
  constexpr unsigned long                scope_id() const noexcept;

  constexpr bool       IsValid() const noexcept;
  constexpr IpAddrType type() const noexcept;
  constexpr bool       IsV4() const noexcept;
  constexpr bool       IsV6() const noexcept;
  bool                 IsLinkLocal() const noexcept;
  bool                 IsLoopback() const noexcept;
  bool                 IsMulticast() const noexcept;
  bool                 IsWildcard() const noexcept;
  unsigned int         MaskLength() const noexcept;

  void SetAddress(uint32_t v4_data) noexcept;
  void SetAddress(const uint8_t* v6_data) noexcept;
  void SetAddress(const uint8_t* v6_data, unsigned long scope_id) noexcept;

  static IpAddr FromString(const char* ip_str) noexcept;
  static IpAddr FromString(const std::string& ip_str) noexcept;
  static IpAddr WildcardV4() noexcept;
  static IpAddr WildcardV6() noexcept;
  static IpAddr Wildcard(IpAddrType type) noexcept;
  static IpAddr NetmaskV4(unsigned int mask_length) noexcept;
  static IpAddr NetmaskV6(unsigned int mask_length) noexcept;
  static IpAddr Netmask(IpAddrType type, unsigned int mask_length) noexcept;

private:
  EtcPalIpAddr addr_{ETCPAL_IP_INVALID_INIT_VALUES};
};

/// @brief Constructs an invalid IP address by default.
ETCPAL_CONSTEXPR_14_OR_INLINE IpAddr::IpAddr() noexcept
{
  ETCPAL_IP_SET_INVALID(&addr_);
}

/// @brief Construct an IP address copied from an instance of the C EtcPalIpAddr type.
constexpr IpAddr::IpAddr(const EtcPalIpAddr& c_ip) noexcept : addr_(c_ip)
{
}

/// @brief Assign an instance of the C EtcPalIpAddr type to an instance of this class.
inline IpAddr& IpAddr::operator=(const EtcPalIpAddr& c_ip) noexcept
{
  addr_ = c_ip;
  return *this;
}

/// @brief Construct an IPv4 address from its raw 32-bit representation.
/// @param[in] v4_data The address data in host byte order.
ETCPAL_CONSTEXPR_14_OR_INLINE IpAddr::IpAddr(uint32_t v4_data) noexcept
{
  ETCPAL_IP_SET_V4_ADDRESS(&addr_, v4_data);
}

/// @brief Construct an IPv6 address from its raw 16-byte array representation.
/// @param[in] v6_data Pointer to the address data, an array of length #ETCPAL_IPV6_BYTES.
inline IpAddr::IpAddr(const uint8_t* v6_data) noexcept
{
  ETCPAL_IP_SET_V6_ADDRESS(&addr_, v6_data);
}

/// @brief Construct an IPv6 address from its raw 16-bytes array representation and scope ID.
///
/// See scope_id() for more information.
///
/// @param[in] v6_data Pointer to the address data, an array of length #ETCPAL_IPV6_BYTES.
/// @param[in] scope_id The address's scope ID.
inline IpAddr::IpAddr(const uint8_t* v6_data, unsigned long scope_id) noexcept
{
  ETCPAL_IP_SET_V6_ADDRESS_WITH_SCOPE_ID(&addr_, v6_data, scope_id);
}

/// @brief Get a const reference to the underlying C type.
constexpr const EtcPalIpAddr& IpAddr::get() const noexcept
{
  return addr_;
}

/// @brief Get a mutable reference to the underlying C type.
ETCPAL_CONSTEXPR_14_OR_INLINE EtcPalIpAddr& IpAddr::get() noexcept
{
  return addr_;
}

/// @brief Convert the IP address to a string representation.
///
/// See etcpal_ip_to_string() for more information.
inline std::string IpAddr::ToString() const
{
  std::array<char, ETCPAL_IP_STRING_BYTES> str_buf;  // NOLINT(cppcoreguidelines-pro-type-member-init)

  auto result = etcpal_ip_to_string(&addr_, str_buf.data());
  if (result == kEtcPalErrOk)
    return {str_buf.data()};
  return {};
}

/// @brief Get the raw 32-bit representation of an IPv4 address.
///
/// This function will return undefined data if the IpAddr's type is V6 or Invalid.
///
/// @return The address data in host byte order.
constexpr uint32_t IpAddr::v4_data() const noexcept
{
  return ETCPAL_IP_V4_ADDRESS(&addr_);
}

/// @brief Get the raw 16-byte array representation of an IPv6 address.
///
/// The returned pointer has the same lifetime as this IpAddr instance. This function will return
/// undefined data if the IpAddr's type is V4 or Invalid.
constexpr const uint8_t* IpAddr::v6_data() const noexcept
{
  return ETCPAL_IP_V6_ADDRESS(&addr_);
}

/// @brief Get a 16-byte std::array representation of an IPv6 address.
///
/// Copies the data into a new array. This function will return undefined data if the IpAddr's type
/// is V4 or Invalid.
inline std::array<uint8_t, ETCPAL_IPV6_BYTES> IpAddr::ToV6Array() const
{
  // RVO should hopefully make this only a single copy
  std::array<uint8_t, ETCPAL_IPV6_BYTES> arr;  // NOLINT(cppcoreguidelines-pro-type-member-init)
  std::memcpy(arr.data(), ETCPAL_IP_V6_ADDRESS(&addr_), ETCPAL_IPV6_BYTES);
  return arr;
}

/// @brief Get the scope ID of an IPv6 address.
///
/// Scope IDs (sometimes referred to as Zone IDs or Zone Indices) are a lightly-documented portion
/// of IPv6 addressing, but one concrete use is to indicate the network interface index of a
/// link-local IPv6 address. See also @ref interface_indexes.
///
/// This function will return undefined data if the IpAddr's type is V4 or Invalid.
///
/// @return The scope ID.
constexpr unsigned long IpAddr::scope_id() const noexcept
{
  return ETCPAL_IP_V6_SCOPE_ID(&addr_);
}

/// @brief Whether an IpAddr contains a valid IPv4 or IPv6 address.
constexpr bool IpAddr::IsValid() const noexcept
{
  return !ETCPAL_IP_IS_INVALID(&addr_);
}

/// @brief Get the type of the IP address.
constexpr IpAddrType IpAddr::type() const noexcept
{
  return static_cast<IpAddrType>(addr_.type);
}

/// @brief Whether an IpAddr contains a valid IPv4 address.
constexpr bool IpAddr::IsV4() const noexcept
{
  return ETCPAL_IP_IS_V4(&addr_);
}

/// @brief Whether an IpAddr contains a valid IPv6 address.
constexpr bool IpAddr::IsV6() const noexcept
{
  return ETCPAL_IP_IS_V6(&addr_);
}

/// @brief Whether an IpAddr contains a link-local address.
inline bool IpAddr::IsLinkLocal() const noexcept
{
  return etcpal_ip_is_link_local(&addr_);
}

/// @brief Whether an IpAddr contains a loopback address.
inline bool IpAddr::IsLoopback() const noexcept
{
  return etcpal_ip_is_loopback(&addr_);
}

/// @brief Whether an IpAddr contains a multicast address.
inline bool IpAddr::IsMulticast() const noexcept
{
  return etcpal_ip_is_multicast(&addr_);
}

/// @brief Whether an IpAddr contains a wildcard address.
///
/// See etcpal_ip_is_wildcard() for more information.
inline bool IpAddr::IsWildcard() const noexcept
{
  return etcpal_ip_is_wildcard(&addr_);
}

/// @brief The number of consecutive set bits in a netmask.
///
/// See etcpal_ip_mask_length() for more information.
inline unsigned int IpAddr::MaskLength() const noexcept
{
  return etcpal_ip_mask_length(&addr_);
}

/// @brief Set the IPv4 address data.
///
/// Automatically converts this address's type to V4.
///
/// @param[in] v4_data The address data in host byte order.
inline void IpAddr::SetAddress(uint32_t v4_data) noexcept
{
  ETCPAL_IP_SET_V4_ADDRESS(&addr_, v4_data);
}

/// @brief Set the IPv6 address data.
///
/// Automatically converts this address's type to V6.
///
/// @param[in] v6_data Pointer to the address data, an array of length #ETCPAL_IPV6_BYTES.
inline void IpAddr::SetAddress(const uint8_t* v6_data) noexcept
{
  ETCPAL_IP_SET_V6_ADDRESS(&addr_, v6_data);
}

/// @brief Set the IPv6 address data and scope ID.
///
/// See scope_id() for more information.
///
/// @param[in] v6_data Pointer to the address data, an array of length #ETCPAL_IPV6_BYTES.
/// @param[in] scope_id The address's scope ID.
inline void IpAddr::SetAddress(const uint8_t* v6_data, unsigned long scope_id) noexcept
{
  ETCPAL_IP_SET_V6_ADDRESS_WITH_SCOPE_ID(&addr_, v6_data, scope_id);
}

/// @brief Construct an IpAddr from a string representation.
///
/// See etcpal_string_to_ip() for more information.
inline IpAddr IpAddr::FromString(const char* ip_str) noexcept
{
  IpAddr result;
  if (etcpal_string_to_ip(kEtcPalIpTypeV4, ip_str, &result.addr_) != kEtcPalErrOk)
  {
    etcpal_string_to_ip(kEtcPalIpTypeV6, ip_str, &result.addr_);
  }
  return result;
}

/// @brief Construct an IpAddr from a string representation.
///
/// See etcpal_string_to_ip() for more information.
inline IpAddr IpAddr::FromString(const std::string& ip_str) noexcept
{
  return FromString(ip_str.c_str());
}

/// @brief Construct a wildcard IPv4 address.
///
/// See etcpal_ip_set_wildcard() for more information.
inline IpAddr IpAddr::WildcardV4() noexcept
{
  return Wildcard(IpAddrType::kV4);
}

/// @brief Construct a wildcard IPv6 address.
///
/// See etcpal_ip_set_wildcard() for more information.
inline IpAddr IpAddr::WildcardV6() noexcept
{
  return Wildcard(IpAddrType::kV6);
}

/// @brief Construct a wildcard address of the type specified.
///
/// See etcpal_ip_set_wildcard() for more information.
inline IpAddr IpAddr::Wildcard(IpAddrType type) noexcept
{
  IpAddr result;
  etcpal_ip_set_wildcard(static_cast<etcpal_iptype_t>(type), &result.addr_);
  return result;
}

/// @brief Construct an IPv4 netmask given a length in bits.
///
/// See etcpal_ip_mask_from_length() for more information.
inline IpAddr IpAddr::NetmaskV4(unsigned int mask_length) noexcept
{
  return Netmask(IpAddrType::kV4, mask_length);
}

/// @brief Construct an IPv6 netmask given a length in bits.
///
/// See etcpal_ip_mask_from_length() for more information.
inline IpAddr IpAddr::NetmaskV6(unsigned int mask_length) noexcept
{
  return Netmask(IpAddrType::kV6, mask_length);
}

/// @brief Construct a netmask of the type specifed given a length in bits.
///
/// See etcpal_ip_mask_from_length() for more information.
inline IpAddr IpAddr::Netmask(IpAddrType type, unsigned int mask_length) noexcept
{
  return etcpal_ip_mask_from_length(static_cast<etcpal_iptype_t>(type), mask_length);
}

/// @ingroup etcpal_cpp_inet
/// @brief A wrapper for the EtcPal socket address type.
///
/// Provides C++ syntactic sugar for working with socket addresses, which contain an IP address and
/// port.
class SockAddr
{
public:
  ETCPAL_CONSTEXPR_14 SockAddr() noexcept;
  // Note: this constructor is not explicit by design, to allow implicit conversions e.g.
  //   etcpal::SockAddr sa = etcpal_c_function_that_returns_sockaddr();
  constexpr SockAddr(const EtcPalSockAddr& c_sa) noexcept;
  SockAddr& operator=(const EtcPalSockAddr& c_sa) noexcept;

  ETCPAL_CONSTEXPR_14 SockAddr(uint32_t v4_data, uint16_t port) noexcept;
  SockAddr(const uint8_t* v6_data, uint16_t port) noexcept;
  SockAddr(const uint8_t* v6_data, unsigned long scope_id, uint16_t port) noexcept;
  ETCPAL_CONSTEXPR_14 SockAddr(IpAddr ip, uint16_t port) noexcept;

  constexpr const EtcPalSockAddr& get() const noexcept;
  ETCPAL_CONSTEXPR_14 EtcPalSockAddr& get() noexcept;
  std::string                         ToString() const;
  constexpr IpAddr                    ip() const noexcept;
  constexpr uint16_t                  port() const noexcept;

  constexpr uint32_t                     v4_data() const noexcept;
  constexpr const uint8_t*               v6_data() const noexcept;
  std::array<uint8_t, ETCPAL_IPV6_BYTES> ToV6Array() const;
  constexpr unsigned long                scope_id() const noexcept;

  constexpr bool       IsValid() const noexcept;
  constexpr IpAddrType type() const noexcept;
  constexpr bool       IsV4() const noexcept;
  constexpr bool       IsV6() const noexcept;
  bool                 IsLinkLocal() const noexcept;
  bool                 IsLoopback() const noexcept;
  bool                 IsMulticast() const noexcept;
  bool                 IsWildcard() const noexcept;

  void SetAddress(uint32_t v4_data) noexcept;
  void SetAddress(const uint8_t* v6_data) noexcept;
  void SetAddress(const uint8_t* v6_data, unsigned long scope_id) noexcept;
  void SetAddress(const IpAddr& ip) noexcept;
  void SetPort(uint16_t port) noexcept;

private:
  EtcPalSockAddr addr_{0, ETCPAL_IP_INVALID_INIT};
};

/// @brief Constructs an invalid SockAddr by default.
ETCPAL_CONSTEXPR_14_OR_INLINE SockAddr::SockAddr() noexcept
{
  ETCPAL_IP_SET_INVALID(&addr_.ip);
}

/// @brief Construct a SockAddr copied from an instance of the C EtcPalSockAddr type.
constexpr SockAddr::SockAddr(const EtcPalSockAddr& c_sa) noexcept : addr_(c_sa)
{
}

/// @brief Assign an instance of the C EtcPalSockAddr type to an instance of this class.
inline SockAddr& SockAddr::operator=(const EtcPalSockAddr& c_sa) noexcept
{
  addr_ = c_sa;
  return *this;
}

/// @brief Construct a SockAddr from a raw 32-bit IPv4 representation and port number.
/// @param[in] v4_data The address data in host byte order.
/// @param[in] port The port number in host byte order.
ETCPAL_CONSTEXPR_14_OR_INLINE SockAddr::SockAddr(uint32_t v4_data, uint16_t port) noexcept
{
  ETCPAL_IP_SET_V4_ADDRESS(&addr_.ip, v4_data);
  addr_.port = port;
}

/// @brief Construct a SockAddr from a raw 16-byte IPv6 representation and port number.
/// @param[in] v6_data Pointer to the address data, an array of length #ETCPAL_IPV6_BYTES.
/// @param[in] port The port number in host byte order.
inline SockAddr::SockAddr(const uint8_t* v6_data, uint16_t port) noexcept
{
  ETCPAL_IP_SET_V6_ADDRESS(&addr_.ip, v6_data);
  addr_.port = port;
}

/// @brief Construct a Sockaddr from a raw 16-byte IPv6 representation with scope ID and port number.
///
/// See IpAddr::scope_id() for more information on scope IDs.
///
/// @param[in] v6_data Pointer to the address data, an array of length #ETCPAL_IPV6_BYTES.
/// @param[in] scope_id The Scope ID value.
/// @param[in] port The port number in host byte order.
inline SockAddr::SockAddr(const uint8_t* v6_data, unsigned long scope_id, uint16_t port) noexcept
{
  ETCPAL_IP_SET_V6_ADDRESS_WITH_SCOPE_ID(&addr_.ip, v6_data, scope_id);
  addr_.port = port;
}

/// @brief Construct a SockAddr from an IpAddr and port number.
/// @param[in] ip The IP address.
/// @param[in] port The port number in host byte order.
ETCPAL_CONSTEXPR_14_OR_INLINE SockAddr::SockAddr(IpAddr ip, uint16_t port) noexcept
{
  addr_.ip = ip.get();
  addr_.port = port;
}

/// @brief Get a const reference to the underlying C type.
constexpr const EtcPalSockAddr& SockAddr::get() const noexcept
{
  return addr_;
}

/// @brief Get a mutable reference to the underlying C type.
ETCPAL_CONSTEXPR_14_OR_INLINE EtcPalSockAddr& SockAddr::get() noexcept
{
  return addr_;
}

/// @brief Convert the IP address and port to a string representation.
///
/// The string will be of the form ddd.ddd.ddd.ddd:ppppp for IPv4 addresses, and
/// [xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx]:ppppp for IPv6 addresses (common conventions and
/// rules for compressing these representations will also apply).
inline std::string SockAddr::ToString() const
{
  if (ETCPAL_IP_IS_V4(&addr_.ip))
    return ip().ToString() + ':' + std::to_string(addr_.port);
  if (ETCPAL_IP_IS_V6(&addr_.ip))
    return '[' + ip().ToString() + "]:" + std::to_string(addr_.port);
  return {};
}

/// @brief Get the IP address from the SockAddr.
constexpr IpAddr SockAddr::ip() const noexcept
{
  return addr_.ip;
}

/// @brief Get the port number from the SockAddr.
constexpr uint16_t SockAddr::port() const noexcept
{
  return addr_.port;
}

/// @brief Get the raw 32-bit representation of the SockAddr's IPv4 address.
///
/// This function will return undefined data if the SockAddr's type is V6 or Invalid.
///
/// @return The address data in host byte order.
constexpr uint32_t SockAddr::v4_data() const noexcept
{
  return ETCPAL_IP_V4_ADDRESS(&addr_.ip);
}

/// @brief Get the raw 16-byte array representation of the SockAddr's IPv6 address.
///
/// The returned pointer has the same lifetime as this SockAddr instance. This function will return
/// undefined data if the SockAddr's type is V4 or Invalid.
constexpr const uint8_t* SockAddr::v6_data() const noexcept
{
  return ETCPAL_IP_V6_ADDRESS(&addr_.ip);
}

/// @brief Get a 16-byte std::array representation of the SockAddr's IPv6 address.
///
/// Copies the data into a new array. This function will return undefined data if the SockAddr's
/// type is V4 or Invalid.
inline std::array<uint8_t, ETCPAL_IPV6_BYTES> SockAddr::ToV6Array() const
{
  // RVO should hopefully make this only a single copy
  std::array<uint8_t, ETCPAL_IPV6_BYTES> arr;  // NOLINT(cppcoreguidelines-pro-type-member-init)
  std::memcpy(arr.data(), ETCPAL_IP_V6_ADDRESS(&addr_.ip), ETCPAL_IPV6_BYTES);
  return arr;
}

/// @brief Get the scope ID of the SockAddr's IPv6 address.
///
/// Scope IDs (sometimes referred to as Zone IDs or Zone Indices) are a lightly-documented portion
/// of IPv6 addressing, but one concrete use is to indicate the network interface index of a
/// link-local IPv6 address. See also @ref interface_indexes.
///
/// This function will return undefined data if the SockAddr's type is V4 or Invalid.
///
/// @return The scope ID.
constexpr unsigned long SockAddr::scope_id() const noexcept
{
  return ETCPAL_IP_V6_SCOPE_ID(&addr_.ip);
}

/// @brief Whether a SockAddr contains a valid IPv4 or IPv6 address.
constexpr bool SockAddr::IsValid() const noexcept
{
  return !ETCPAL_IP_IS_INVALID(&addr_.ip);
}

/// @brief Get the type of the SockAddr's IP address.
constexpr IpAddrType SockAddr::type() const noexcept
{
  return static_cast<IpAddrType>(addr_.ip.type);
}

/// @brief Whether a SockAddr contains a valid IPv4 address.
constexpr bool SockAddr::IsV4() const noexcept
{
  return ETCPAL_IP_IS_V4(&addr_.ip);
}

/// @brief Whether a SockAddr contains a valid IPv6 address.
constexpr bool SockAddr::IsV6() const noexcept
{
  return ETCPAL_IP_IS_V6(&addr_.ip);
}

/// @brief Whether a SockAddr contains a link-local address.
inline bool SockAddr::IsLinkLocal() const noexcept
{
  return etcpal_ip_is_link_local(&addr_.ip);
}

/// @brief Whether a SockAddr contains a loopback address.
inline bool SockAddr::IsLoopback() const noexcept
{
  return etcpal_ip_is_loopback(&addr_.ip);
}

/// @brief Whether a SockAddr contains a multicast address.
inline bool SockAddr::IsMulticast() const noexcept
{
  return etcpal_ip_is_multicast(&addr_.ip);
}

/// @brief Whether a SockAddr contains a wildcard address.
///
/// See etcpal_ip_is_wildcard() for more information.
inline bool SockAddr::IsWildcard() const noexcept
{
  return etcpal_ip_is_wildcard(&addr_.ip);
}

/// @brief Set the IPv4 address data.
///
/// Automatically converts this address's type to V4.
///
/// @param[in] v4_data The address data in host byte order.
inline void SockAddr::SetAddress(uint32_t v4_data) noexcept
{
  ETCPAL_IP_SET_V4_ADDRESS(&addr_.ip, v4_data);
}

/// @brief Set the IPv6 address data.
///
/// Automatically converts this address's type to V6.
///
/// @param[in] v6_data Pointer to the address data, an array of length #ETCPAL_IPV6_BYTES.
inline void SockAddr::SetAddress(const uint8_t* v6_data) noexcept
{
  ETCPAL_IP_SET_V6_ADDRESS(&addr_.ip, v6_data);
}

/// @brief Set the IPv6 address data and scope ID.
///
/// See IpAddr::scope_id() for more information.
///
/// @param[in] v6_data Pointer to the address data, an array of length #ETCPAL_IPV6_BYTES.
/// @param[in] scope_id The address's scope ID.
inline void SockAddr::SetAddress(const uint8_t* v6_data, unsigned long scope_id) noexcept
{
  ETCPAL_IP_SET_V6_ADDRESS_WITH_SCOPE_ID(&addr_.ip, v6_data, scope_id);
}

/// @brief Set the IP address using an IpAddr instance.
/// @param[in] ip IpAddr to copy into this SockAddr's underlying IP address.
inline void SockAddr::SetAddress(const IpAddr& ip) noexcept
{
  addr_.ip = ip.get();
}

/// @brief Set the port.
/// @param[in] port The port number.
inline void SockAddr::SetPort(uint16_t port) noexcept
{
  addr_.port = port;
}

/// @ingroup etcpal_cpp_inet
/// @brief A wrapper for the EtcPal MAC address type.
///
/// Provides C++ syntactic sugar for working with MAC addresses.
class MacAddr
{
public:
  /// @brief Constructs a null MAC address by default.
  MacAddr() = default;
  // Note: this constructor is not explicit by design, to allow implicit conversions e.g.
  //   etcpal::MacAddr sa = etcpal_c_function_that_returns_macaddr();
  constexpr MacAddr(const EtcPalMacAddr& c_mac) noexcept;
  MacAddr& operator=(const EtcPalMacAddr& c_mac) noexcept;
  explicit MacAddr(const uint8_t* mac_data) noexcept;

  constexpr const EtcPalMacAddr& get() const noexcept;
  ETCPAL_CONSTEXPR_14 EtcPalMacAddr&    get() noexcept;
  std::string                           ToString() const;
  constexpr const uint8_t*              data() const noexcept;
  std::array<uint8_t, ETCPAL_MAC_BYTES> ToArray() const noexcept;

  bool IsNull() const noexcept;

  static MacAddr FromString(const char* mac_str) noexcept;
  static MacAddr FromString(const std::string& mac_str) noexcept;

private:
  EtcPalMacAddr addr_{};
};

/// @brief Construct a MAC address copied from an instance of the C EtcPalMacAddr type.
constexpr MacAddr::MacAddr(const EtcPalMacAddr& c_mac) noexcept : addr_(c_mac)
{
}

/// @brief Assign an instance of the C EtcPalMacAddr type to an instance of this class.
inline MacAddr& MacAddr::operator=(const EtcPalMacAddr& c_mac) noexcept
{
  addr_ = c_mac;
  return *this;
}

/// @brief Construct a MAC address from its raw 6-byte array representation.
/// @param[in] mac_data Pointer to the MAC data, an array of length #ETCPAL_MAC_BYTES.
inline MacAddr::MacAddr(const uint8_t* mac_data) noexcept
{
  std::memcpy(addr_.data, mac_data, ETCPAL_MAC_BYTES);
}

/// @brief Get a const reference to the underlying C type.
constexpr const EtcPalMacAddr& MacAddr::get() const noexcept
{
  return addr_;
}

/// @brief Get a mutable reference to the underlying C type.
ETCPAL_CONSTEXPR_14_OR_INLINE EtcPalMacAddr& MacAddr::get() noexcept
{
  return addr_;
}

/// @brief Convert the MAC address to a string representation.
///
/// See etcpal_mac_to_string() for more information.
inline std::string MacAddr::ToString() const
{
  std::array<char, ETCPAL_MAC_STRING_BYTES> str_buf;  // NOLINT(cppcoreguidelines-pro-type-member-init)
  etcpal_mac_to_string(&addr_, str_buf.data());
  return {str_buf.data()};
}

/// @brief Get the raw 6-byte array representation of a MAC address.
/// @return Pointer to an array of length #ETCPAL_MAC_BYTES containing the address data.
constexpr const uint8_t* MacAddr::data() const noexcept
{
  return addr_.data;
}

/// @brief Get a 6-byte std::array representation of a MAC address.
///
/// Copies the data into a new array.
inline std::array<uint8_t, ETCPAL_MAC_BYTES> MacAddr::ToArray() const noexcept
{
  // RVO should hopefully make this only a single copy
  std::array<uint8_t, ETCPAL_MAC_BYTES> arr;  // NOLINT(cppcoreguidelines-pro-type-member-init)
  std::memcpy(arr.data(), addr_.data, ETCPAL_MAC_BYTES);
  return arr;
}

/// @brief Whether this MacAddr represents a null (all 0's) MAC address.
inline bool MacAddr::IsNull() const noexcept
{
  return ETCPAL_MAC_IS_NULL(&addr_);
}

/// @brief Construct a MacAddr from a string representation.
///
/// See etcpal_string_to_mac() for more information.
inline MacAddr MacAddr::FromString(const char* mac_str) noexcept
{
  MacAddr result;
  etcpal_string_to_mac(mac_str, &result.addr_);
  return result;
}

/// @brief Construct a MacAddr from a string representation.
///
/// See etcpal_string_to_mac() for more information.
inline MacAddr MacAddr::FromString(const std::string& mac_str) noexcept
{
  return FromString(mac_str.c_str());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Operators
////////////////////////////////////////////////////////////////////////////////////////////////////

/// @addtogroup etcpal_cpp_inet
/// @{

/// @name Inet Relational Operators
/// @{

// Special operators for comparing with the underlying types

inline bool operator==(const EtcPalIpAddr& c_ip, const IpAddr& ip) noexcept
{
  return c_ip == ip.get();
}

inline bool operator!=(const EtcPalIpAddr& c_ip, const IpAddr& ip) noexcept
{
  return !(c_ip == ip);
}

inline bool operator==(const IpAddr& ip, const EtcPalIpAddr& c_ip) noexcept
{
  return ip.get() == c_ip;
}

inline bool operator!=(const IpAddr& ip, const EtcPalIpAddr& c_ip) noexcept
{
  return !(ip == c_ip);
}

inline bool operator==(const EtcPalSockAddr& c_ip, const SockAddr& ip) noexcept
{
  return c_ip == ip.get();
}

inline bool operator!=(const EtcPalSockAddr& c_ip, const SockAddr& ip) noexcept
{
  return !(c_ip == ip);
}

inline bool operator==(const SockAddr& ip, const EtcPalSockAddr& c_ip) noexcept
{
  return ip.get() == c_ip;
}

inline bool operator!=(const SockAddr& ip, const EtcPalSockAddr& c_ip) noexcept
{
  return !(ip == c_ip);
}

inline bool operator==(const EtcPalMacAddr& c_mac, const MacAddr& mac) noexcept
{
  return c_mac == mac.get();
}

inline bool operator!=(const EtcPalMacAddr& c_mac, const MacAddr& mac) noexcept
{
  return !(c_mac == mac);
}

inline bool operator==(const MacAddr& mac, const EtcPalMacAddr& c_mac) noexcept
{
  return mac.get() == c_mac;
}

inline bool operator!=(const MacAddr& mac, const EtcPalMacAddr& c_mac) noexcept
{
  return !(mac == c_mac);
}

// Standard operators

inline bool operator==(const IpAddr& a, const IpAddr& b) noexcept
{
  return a.get() == b.get();
}

inline bool operator!=(const IpAddr& a, const IpAddr& b) noexcept
{
  return !(a == b);
}

inline bool operator<(const IpAddr& a, const IpAddr& b) noexcept
{
  return a.get() < b.get();
}

inline bool operator>(const IpAddr& a, const IpAddr& b) noexcept
{
  return b < a;
}

inline bool operator<=(const IpAddr& a, const IpAddr& b) noexcept
{
  return !(b < a);
}

inline bool operator>=(const IpAddr& a, const IpAddr& b) noexcept
{
  return !(a < b);
}

inline bool operator==(const SockAddr& a, const SockAddr& b) noexcept
{
  return a.get() == b.get();
}

inline bool operator!=(const SockAddr& a, const SockAddr& b) noexcept
{
  return !(a == b);
}

inline bool operator<(const SockAddr& a, const SockAddr& b) noexcept
{
  return a.get() < b.get();
}

inline bool operator>(const SockAddr& a, const SockAddr& b) noexcept
{
  return b < a;
}

inline bool operator<=(const SockAddr& a, const SockAddr& b) noexcept
{
  return !(b < a);
}

inline bool operator>=(const SockAddr& a, const SockAddr& b) noexcept
{
  return !(a < b);
}

inline bool operator==(const MacAddr& a, const MacAddr& b) noexcept
{
  return a.get() == b.get();
}

inline bool operator!=(const MacAddr& a, const MacAddr& b) noexcept
{
  return !(a == b);
}

inline bool operator<(const MacAddr& a, const MacAddr& b) noexcept
{
  return a.get() < b.get();
}

inline bool operator>(const MacAddr& a, const MacAddr& b) noexcept
{
  return b < a;
}

inline bool operator<=(const MacAddr& a, const MacAddr& b) noexcept
{
  return !(b < a);
}

inline bool operator>=(const MacAddr& a, const MacAddr& b) noexcept
{
  return !(a < b);
}

/// @}
/// @}

};  // namespace etcpal

#endif  // ETCPAL_CPP_LOCK_H_
