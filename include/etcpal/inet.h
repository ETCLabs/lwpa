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

/* etcpal/inet.h: POSIX-like identifiers for IP addresses, network interfaces and related items. */

#ifndef ETCPAL_INET_H_
#define ETCPAL_INET_H_

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "etcpal/error.h"
#include "etcpal/os_inet.h"

/**
 * @defgroup etcpal_inet inet (Internet Addressing)
 * @ingroup etcpal_net
 * @brief Identifiers for IP addresses and network interfaces.
 *
 * ```c
 * #include "etcpal/inet.h"
 * ```
 *
 * Provides types representing internet addressing constructs. Many of these functions and types
 * mimic those found in POSIX's `arpa/inet.h`, with some quality-of-life improvements. Several
 * functions and macros are provided to inspect, compare and manipulate these types.
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Used to identify the type of IP address contained in a EtcPalIpAddr. */
typedef enum
{
  /** This EtcPalIpAddr is not valid. */
  kEtcPalIpTypeInvalid,
  /** This EtcPalIpAddr contains an IPv4 address. */
  kEtcPalIpTypeV4,
  /** This EtcPalIpAddr contains an IPv6 address. */
  kEtcPalIpTypeV6
} etcpal_iptype_t;

/** The number of bytes in an IPv6 address. */
#define ETCPAL_IPV6_BYTES 16

/**
 * @brief An IP address.
 *
 * Can hold either an IPv4 or IPv6 address. Do not manipulate the members directly; use the macros
 * defined in this module to access them. IPv4 addresses are in host byte order. IPv6 addresses
 * also contain a scope ID, which is also sometimes referred to as a zone index (RFC 4007), to
 * help disambiguate link-local addresses, among other uses. In most cases, this field can be left
 * at its default value, which is set by the ETCPAL_IP_SET_V6_ADDRESS() macro.
 */
typedef struct EtcPalIpAddr
{
  /** The IP address type (IPv4 or IPv6) */
  etcpal_iptype_t type;
  /** A union containing either the IPv4 or IPv6 address. */
  union
  {
    /** The IPv4 address value in host byte order. */
    uint32_t v4;
    /** The IPv6 address value containing the address and scope ID. */
    struct EtcPalIpv6Addr
    {
      uint8_t       addr_buf[ETCPAL_IPV6_BYTES]; /**< The IPv6 address. */
      unsigned long scope_id;                    /**< The IPv6 scope ID. */
    } v6;
  } addr;
} EtcPalIpAddr;

/**
 * @name EtcPalIpAddr macros
 * A set of macros for interacting with a EtcPalIpAddr. It is recommended to use these macros where
 * possible rather than accessing the structure members directly; otherwise, it's easy to forget
 * to fill in a field.
 * @{
 */

/**
 * @brief A set of initializer values for an invalid EtcPalIpAddr.
 *
 * Usage:
 * @code
 * EtcPalIpAddr invalid_ip = { ETCPAL_IP_INVALID_INIT_VALUES };
 * @endcode
 *
 * To omit the enclosing brackets, use #ETCPAL_IP_INVALID_INIT.
 */
#define ETCPAL_IP_INVALID_INIT_VALUES \
  kEtcPalIpTypeInvalid, { 0 }

/**
 * @brief An initializer for an invalid EtcPalIpAddr.
 *
 * Usage:
 * @code
 * EtcPalIpAddr invalid_ip = ETCPAL_IP_INVALID_INIT;
 * @endcode
 */
#define ETCPAL_IP_INVALID_INIT    \
  {                               \
    ETCPAL_IP_INVALID_INIT_VALUES \
  }

/**
 * @brief A set of initializer values for an EtcPalIpAddr representing an IPv4 address.
 *
 * Usage:
 * @code
 * EtcPalIpAddr v4 = { ETCPAL_IPV4_INIT_VALUES(0xc0a80101) }; // 192.168.1.1
 * @endcode
 *
 * Note: Using this macro requires support for C99 designated initializers in your compiler.
 *
 * To omit the enclosing brackets, use #ETCPAL_IPV4_INIT().
 */
#define ETCPAL_IPV4_INIT_VALUES(v4_val) \
  kEtcPalIpTypeV4, { .v4 = v4_val }

/**
 * @brief An initializer for an EtcPalIpAddr representing an IPv4 address.
 *
 * Usage:
 * @code
 * EtcPalIpAddr v4 = ETCPAL_IPV4_INIT(0xc0a80101); // 192.168.1.1
 * @endcode
 *
 * Note: Using this macro requires support for C99 designated initializers in your compiler.
 */
#define ETCPAL_IPV4_INIT(addr_val)    \
  {                                   \
    ETCPAL_IPV4_INIT_VALUES(addr_val) \
  }

/**
 * @brief A set of initializer values for an EtcPalIpAddr representing an IPv6 address.
 *
 * Usage:
 * @code
 * // 2001:db8::1234:5678
 * EtcPalIpAddr v6 = {
 *   ETCPAL_IPV6_INIT_VALUES(0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00,
 *                           0x00, 0x00, 0x00, 0x00, 0x12, 0x34, 0x56, 0x78)
 * };
 * @endcode
 *
 * Any number of initializer values can be used (the remaining IPv6 bytes will be initialized to 0).
 * To omit the enclosing brackets, use #ETCPAL_IPV6_INIT().
 *
 * Note: Using this macro requires support for variadic macros and C99 designated initializers in
 * your compiler.
 */
#define ETCPAL_IPV6_INIT_VALUES(...)                  \
  kEtcPalIpTypeV6,                                    \
  {                                                   \
    .v6 = {.addr_buf = {__VA_ARGS__}, .scope_id = 0 } \
  }

/**
 * @brief An initializer for an EtcPalIpAddr representing an IPv6 address.
 *
 * Usage:
 * @code
 * // 2001:db8::1234:5678
 * EtcPalIpAddr v6 = ETCPAL_IPV6_INIT(0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00,
 *                                    0x00, 0x00, 0x00, 0x00, 0x12, 0x34, 0x56, 0x78);
 * @endcode
 *
 * Any number of initializer values can be used (the remaining IPv6 bytes will be initialized to 0).
 *
 * Note: Using this macro requires support for variadic macros and C99 designated initializers in
 * your compiler.
 */
#define ETCPAL_IPV6_INIT(...)            \
  {                                      \
    ETCPAL_IPV6_INIT_VALUES(__VA_ARGS__) \
  }

/**
 * @brief Determine whether a EtcPalIpAddr contains an IPv4 address.
 * @param etcpal_ip_ptr Pointer to a EtcPalIpAddr.
 * @return true (contains an IPv4 address) or false (does not contain an IPv4 address).
 */
#define ETCPAL_IP_IS_V4(etcpal_ip_ptr) ((etcpal_ip_ptr)->type == kEtcPalIpTypeV4)

/**
 * @brief Determine whether a EtcPalIpAddr contains an IPv6 address.
 * @param etcpal_ip_ptr Pointer to a EtcPalIpAddr.
 * @return true (contains an IPv6 address) or false (does not contain an IPv6 address).
 */
#define ETCPAL_IP_IS_V6(etcpal_ip_ptr) ((etcpal_ip_ptr)->type == kEtcPalIpTypeV6)

/**
 * @brief Determine whether a EtcPalIpAddr contains an invalid address.
 * @param etcpal_ip_ptr Pointer to a EtcPalIpAddr.
 * @return true (is invalid) or false (is not invalid).
 */
#define ETCPAL_IP_IS_INVALID(etcpal_ip_ptr) ((etcpal_ip_ptr)->type == kEtcPalIpTypeInvalid)

/**
 * @brief Get the IPv4 address from a EtcPalIpAddr.
 *
 * It is recommended to first use ETCPAL_IP_IS_V4() to make sure this EtcPalIpAddr contains a valid
 * IPv4 address.
 *
 * @param etcpal_ip_ptr Pointer to a EtcPalIpAddr.
 * @return The IPv4 address (uint32_t).
 */
#define ETCPAL_IP_V4_ADDRESS(etcpal_ip_ptr) ((etcpal_ip_ptr)->addr.v4)

/**
 * @brief Get the IPv6 address from a EtcPalIpAddr.
 *
 * It is recommended to first use ETCPAL_IP_IS_V6() to make sure this EtcPalIpAddr contains a valid
 * IPv6 address.
 *
 * @param etcpal_ip_ptr Pointer to a EtcPalIpAddr.
 * @return The IPv6 address (uint8_t[]).
 */
#define ETCPAL_IP_V6_ADDRESS(etcpal_ip_ptr) ((etcpal_ip_ptr)->addr.v6.addr_buf)

/**
 * @brief Get the IPv6 scope ID from an EtcPalIpAddr.
 *
 * See @ref interface_indexes for more information. It is recommended to first use
 * ETCPAL_IP_IS_V6() to make sure this EtcPalIpAddr contains a valid IPv6 address.
 *
 * @param etcpal_ip_ptr Pointer to a EtcPalIpAddr.
 * @return The IPv6 scope ID (unsigned int).
 */
#define ETCPAL_IP_V6_SCOPE_ID(etcpal_ip_ptr) ((etcpal_ip_ptr)->addr.v6.scope_id)

/**
 * @brief Set the IPv4 address in a EtcPalIpAddr.
 *
 * Also sets the type field to indicate that this EtcPalIpAddr contains an IPv4 address.
 *
 * @param etcpal_ip_ptr Pointer to a EtcPalIpAddr.
 * @param val IPv4 address to set (uint32_t).
 */
#define ETCPAL_IP_SET_V4_ADDRESS(etcpal_ip_ptr, val) \
  do                                                 \
  {                                                  \
    (etcpal_ip_ptr)->type = kEtcPalIpTypeV4;         \
    (etcpal_ip_ptr)->addr.v4 = val;                  \
  } while (0)

/**
 * @brief Set the IPv6 address in a EtcPalIpAddr.
 *
 * Also sets the type field to indicate that this EtcPalIpAddr contains an IPv6 address.
 *
 * @param etcpal_ip_ptr Pointer to a EtcPalIpAddr.
 * @param addr_val IPv6 address to set (uint8_t[]). Must be at least of length #ETCPAL_IPV6_BYTES.
 *                 Gets copied into the struct.
 */
#define ETCPAL_IP_SET_V6_ADDRESS(etcpal_ip_ptr, addr_val) \
  ETCPAL_IP_SET_V6_ADDRESS_WITH_SCOPE_ID(etcpal_ip_ptr, addr_val, 0u)

/**
 * @brief Set an IPv6 address with an explicit scope ID in a EtcPalIpAddr.
 *
 * Also sets the type field to indicate that this EtcPalIpAddr contains an IPv6 address.
 *
 * @param etcpal_ip_ptr Pointer to a EtcPalIpAddr.
 * @param addr_val IPv6 address to set (uint8_t[]). Must be at least of length #ETCPAL_IPV6_BYTES.
 *                 Gets copied into the struct.
 * @param scope_id_val IPv6 scope ID to set.
 */
#define ETCPAL_IP_SET_V6_ADDRESS_WITH_SCOPE_ID(etcpal_ip_ptr, addr_val, scope_id_val) \
  do                                                                                  \
  {                                                                                   \
    (etcpal_ip_ptr)->type = kEtcPalIpTypeV6;                                          \
    memcpy((etcpal_ip_ptr)->addr.v6.addr_buf, (addr_val), ETCPAL_IPV6_BYTES);         \
    (etcpal_ip_ptr)->addr.v6.scope_id = (scope_id_val);                               \
  } while (0)

/**
 * @brief Set the type field in a EtcPalIpAddr to indicate that it does not contain a valid address.
 * @param etcpal_ip_ptr Pointer to a EtcPalIpAddr.
 */
#define ETCPAL_IP_SET_INVALID(etcpal_ip_ptr) ((etcpal_ip_ptr)->type = kEtcPalIpTypeInvalid)

/**
 * @}
 */

/** An IP address with associated interface and port. Ports are in host byte order. */
typedef struct EtcPalSockAddr
{
  uint16_t     port; /**< TCP or UDP port. */
  EtcPalIpAddr ip;   /**< IP address. */
} EtcPalSockAddr;

/** The number of bytes in a MAC address. */
#define ETCPAL_MAC_BYTES 6

/** A MAC address. */
typedef struct EtcPalMacAddr
{
  uint8_t data[ETCPAL_MAC_BYTES]; /**< The 6-byte address data. */
} EtcPalMacAddr;

/**
 * @brief Compare two EtcPalMacAddrs numerically.
 * @param[in] mac1ptr Pointer to first EtcPalMacAddr to compare.
 * @param[in] mac2ptr Pointer to second EtcPalMacAddr to compare.
 * @return < 0: mac1 < mac2
 * @return 0: mac1 == mac2
 * @return > 0: mac1 > mac2
 */
#define ETCPAL_MAC_CMP(mac1ptr, mac2ptr) memcmp((mac1ptr)->data, (mac2ptr)->data, ETCPAL_MAC_BYTES)

/** A null (all 0's) MAC address, used by ETCPAL_MAC_IS_NULL() for comparison. */
extern const EtcPalMacAddr kEtcPalNullMacAddr;

/**
 * @brief Determine if a MAC address is null.
 *
 * A MAC address is said to be 'null' when it is made up of all 0's.
 *
 * @param macptr Pointer to EtcPalMacAddr to null-check.
 * @return true (MAC address is null) or false (MAC address is not null).
 */
#define ETCPAL_MAC_IS_NULL(macptr) (memcmp((macptr)->data, kEtcPalNullMacAddr.data, ETCPAL_MAC_BYTES) == 0)

/** The maximum length of a network interface id. */
#define ETCPAL_NETINTINFO_ID_LEN 64
/** The maximum length of a user-friendly network interface name. */
#define ETCPAL_NETINTINFO_FRIENDLY_NAME_LEN 64

/** A description of a single address assigned to a network interface. */
typedef struct EtcPalNetintInfo
{
  /** The OS-specific network interface number. Since interfaces can have multiple IPv4 and IPv6
   *  addresses assigned simultaneously, there can be a one-to-many relationship between physical
   *  network interfaces and EtcPalNetintInfo descriptions on the same system, all of which will have
   *  the same value for this field. It is also used for IPv6, multicast and IP-version-neutral
   *  APIs. See @ref interface_indexes for more information. */
  unsigned int index;
  /** The interface ip address. */
  EtcPalIpAddr addr;
  /** The subnet mask for this interface. */
  EtcPalIpAddr mask;
  /** The adapter MAC address. */
  EtcPalMacAddr mac;
  /** The system name for the interface. This name can be used as a primary key to identify a
   *  single network adapter. It will not change unless the adapter is removed or reconfigured.
   *  Since interfaces can have multiple IPv4 and IPv6 addresses assigned simultaneously, there can
   *  be a one-to-many relationship between physical network interfaces and EtcPalNetintInfo
   *  structures on the same system, all of which have the same value for this field. */
  char id[ETCPAL_NETINTINFO_ID_LEN];
  /** A user-friendly name for the interface. On some systems, this is the same as the id field.
   *  Others allow users to create and change a friendly name for network interfaces that's
   *  different than the system name. This field should be used when printing the adapter list in a
   *  UI. */
  char friendly_name[ETCPAL_NETINTINFO_FRIENDLY_NAME_LEN];
  /** Whether this is the default network interface. The default network interface is defined as
   *  the network interface chosen for the default IP route on a system. */
  bool is_default;
} EtcPalNetintInfo;

/** A set of identifying information for a network interface, for multicast purposes. The primary key for a network
 *  interface is simply a combination of the interface index and the IP protocol used. The interface IP address is not
 *  required for this. */
typedef struct EtcPalMcastNetintId
{
  etcpal_iptype_t ip_type; /*!< The IP protocol used on the network interface. */
  unsigned int    index;   /*!< The OS index of the network interface. */
} EtcPalMcastNetintId;

/** Maximum length of the string representation of an IP address. */
#define ETCPAL_IP_STRING_BYTES 46
/** Maximum length of the string representation of a MAC address. */
#define ETCPAL_MAC_STRING_BYTES 18

bool etcpal_ip_is_link_local(const EtcPalIpAddr* ip);
bool etcpal_ip_is_loopback(const EtcPalIpAddr* ip);
bool etcpal_ip_is_multicast(const EtcPalIpAddr* ip);
bool etcpal_ip_is_wildcard(const EtcPalIpAddr* ip);
void etcpal_ip_set_wildcard(etcpal_iptype_t type, EtcPalIpAddr* ip);

int  etcpal_ip_cmp(const EtcPalIpAddr* ip1, const EtcPalIpAddr* ip2);
bool etcpal_ip_and_port_equal(const EtcPalSockAddr* sock1, const EtcPalSockAddr* sock2);

unsigned int etcpal_ip_mask_length(const EtcPalIpAddr* netmask);
EtcPalIpAddr etcpal_ip_mask_from_length(etcpal_iptype_t type, unsigned int mask_length);
bool etcpal_ip_network_portions_equal(const EtcPalIpAddr* ip1, const EtcPalIpAddr* ip2, const EtcPalIpAddr* netmask);

bool   ip_os_to_etcpal(const etcpal_os_ipaddr_t* os_ip, EtcPalIpAddr* ip);
size_t ip_etcpal_to_os(const EtcPalIpAddr* ip, etcpal_os_ipaddr_t* os_ip);

bool   sockaddr_os_to_etcpal(const etcpal_os_sockaddr_t* os_sa, EtcPalSockAddr* sa);
size_t sockaddr_etcpal_to_os(const EtcPalSockAddr* sa, etcpal_os_sockaddr_t* os_sa);

etcpal_error_t etcpal_ip_to_string(const EtcPalIpAddr* src, char* dest);
etcpal_error_t etcpal_string_to_ip(etcpal_iptype_t type, const char* src, EtcPalIpAddr* dest);
etcpal_error_t etcpal_mac_to_string(const EtcPalMacAddr* src, char* dest);
etcpal_error_t etcpal_string_to_mac(const char* src, EtcPalMacAddr* dest);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

/* C++ utilities */

/* Comparison operators for IpAddrs */

inline bool operator==(const EtcPalIpAddr& a, const EtcPalIpAddr& b)
{
  return (etcpal_ip_cmp(&a, &b) == 0);
}

inline bool operator!=(const EtcPalIpAddr& a, const EtcPalIpAddr& b)
{
  return !(a == b);
}

inline bool operator<(const EtcPalIpAddr& a, const EtcPalIpAddr& b)
{
  return (etcpal_ip_cmp(&a, &b) < 0);
}

inline bool operator>(const EtcPalIpAddr& a, const EtcPalIpAddr& b)
{
  return b < a;
}

inline bool operator<=(const EtcPalIpAddr& a, const EtcPalIpAddr& b)
{
  return !(b < a);
}

inline bool operator>=(const EtcPalIpAddr& a, const EtcPalIpAddr& b)
{
  return !(a < b);
}

/* Comparison operators for SockAddrs */

inline bool operator==(const EtcPalSockAddr& a, const EtcPalSockAddr& b)
{
  return (a.ip == b.ip && a.port == b.port);
}

inline bool operator!=(const EtcPalSockAddr& a, const EtcPalSockAddr& b)
{
  return !(a == b);
}

inline bool operator<(const EtcPalSockAddr& a, const EtcPalSockAddr& b)
{
  if (a.port == b.port)
    return (etcpal_ip_cmp(&a.ip, &b.ip) < 0);
  return (a.port < b.port);
}

inline bool operator>(const EtcPalSockAddr& a, const EtcPalSockAddr& b)
{
  return b < a;
}

inline bool operator<=(const EtcPalSockAddr& a, const EtcPalSockAddr& b)
{
  return !(b < a);
}

inline bool operator>=(const EtcPalSockAddr& a, const EtcPalSockAddr& b)
{
  return !(a < b);
}

/* Comparison operators for MacAddrs */

inline bool operator==(const EtcPalMacAddr& a, const EtcPalMacAddr& b)
{
  return (ETCPAL_MAC_CMP(&a, &b) == 0);
}

inline bool operator!=(const EtcPalMacAddr& a, const EtcPalMacAddr& b)
{
  return !(a == b);
}

inline bool operator<(const EtcPalMacAddr& a, const EtcPalMacAddr& b)
{
  return (ETCPAL_MAC_CMP(&a, &b) < 0);
}

inline bool operator>(const EtcPalMacAddr& a, const EtcPalMacAddr& b)
{
  return b < a;
}

inline bool operator<=(const EtcPalMacAddr& a, const EtcPalMacAddr& b)
{
  return !(b < a);
}

inline bool operator>=(const EtcPalMacAddr& a, const EtcPalMacAddr& b)
{
  return !(a < b);
}

#endif

/**
 * @}
 */

#endif /* ETCPAL_INET_H_ */
