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

/***************************** Global variables ******************************/

const EtcPalMacAddr kEtcPalNullMacAddr = {{0}};

/****************************** Private macros *******************************/

#ifdef _MSC_VER
#define ETCPAL_SPRINTF __pragma(warning(suppress : 4996)) sprintf
#else
#define ETCPAL_SPRINTF sprintf
#endif

#ifdef __MQX__
#define MQX_PROVIDES_STDIO !MQX_SUPPRESS_STDIO_MACROS
#else
#define MQX_PROVIDES_STDIO 0
#endif

#if !MQX_PROVIDES_STDIO
#include <stdio.h>
#endif

/**************************** Private variables ******************************/

static const uint8_t kV6Wildcard[ETCPAL_IPV6_BYTES] = {0};
static const uint8_t kV6Loopback[ETCPAL_IPV6_BYTES] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};

/*************************** Function definitions ****************************/

/**
 * @brief Determine whether a EtcPalIpAddr contains a link-local address.
 *
 * Works for both IPv4 and IPv6 addresses.
 *
 * @param[in] ip Address to check.
 * @return true: ip contains a link-local address.
 * @return false: ip does not contain a link-local address.
 */
bool etcpal_ip_is_link_local(const EtcPalIpAddr* ip)
{
  if (ip)
  {
    if (ETCPAL_IP_IS_V4(ip))
    {
      // An IPv4 address is link-local if the first two octets are 0xa9fe (169.254.0.0/16)
      return ((ETCPAL_IP_V4_ADDRESS(ip) & 0xffff0000u) == 0xa9fe0000u);
    }
    if (ETCPAL_IP_IS_V6(ip))
    {
      // An IPv6 address is link-local if the first 10 bits are 1111111010 (fe80::/10)
      const uint8_t* addr_buf = ETCPAL_IP_V6_ADDRESS(ip);
      return (addr_buf[0] == 0xfeu && ((addr_buf[1] & 0xc0u) == 0x80u));
    }
  }
  return false;
}

/**
 * @brief Determine whether a EtcPalIpAddr contains a loopback address.
 *
 * Works for both IPv4 and IPv6 addresses.
 *
 * @param[in] ip Address to check.
 * @return true: ip contains a loopback address.
 * @return false: ip does not contain a loopback address.
 */
bool etcpal_ip_is_loopback(const EtcPalIpAddr* ip)
{
  if (ip)
  {
    if (ETCPAL_IP_IS_V4(ip))
    {
      // An IPv4 address is loopback if the first octet is 0x7f (127.0.0.0/8)
      return ((ETCPAL_IP_V4_ADDRESS(ip) & 0xff000000u) == 0x7f000000u);
    }
    if (ETCPAL_IP_IS_V6(ip))
    {
      // An IPv6 address is loopback if it is equal to the address ::1
      return (0 == memcmp(ETCPAL_IP_V6_ADDRESS(ip), kV6Loopback, ETCPAL_IPV6_BYTES));
    }
  }
  return false;
}

/**
 * @brief Determine whether a EtcPalIpAddr contains a multicast address.
 *
 * Works for both IPv4 and IPv6 addresses.
 *
 * @param[in] ip Address to check.
 * @return true: ip contains a multicast address.
 * @return false: ip does not contain a multicast address.
 */
bool etcpal_ip_is_multicast(const EtcPalIpAddr* ip)
{
  if (ip)
  {
    if (ETCPAL_IP_IS_V4(ip))
    {
      // An IPv4 address is multicast if it is in the range 224.0.0.0 to 239.255.255.255, inclusive
      // (224.0.0.0/4)
      return (ETCPAL_IP_V4_ADDRESS(ip) >= 0xe0000000u && ETCPAL_IP_V4_ADDRESS(ip) <= 0xefffffffu);
    }
    if (ETCPAL_IP_IS_V6(ip))
    {
      // An IPv6 address is multicast if the first octet is 0xff (ff00::/8)
      return (ETCPAL_IP_V6_ADDRESS(ip)[0] == 0xff);
    }
  }
  return false;
}

/**
 * @brief Determine whether a EtcPalIpAddr contains a wildcard address.
 *
 * Works for both IPv4 and IPv6 addresses. The wildcard address is used as an argument to
 * etcpal_bind() to indicate that a socket should be bound to all available network interfaces. It
 * should not be used as a placeholder or invalid address - use ETCPAL_IP_SET_INVALID() and
 * ETCPAL_IP_IS_INVALID() for that.
 *
 * @param[in] ip Address to check.
 * @return true: ip contains a wildcard address.
 * @return false: ip does not contain a wildcard address.
 */
bool etcpal_ip_is_wildcard(const EtcPalIpAddr* ip)
{
  if (ip)
  {
    // Wildcard addresses are all-zeroes
    if (ETCPAL_IP_IS_V4(ip))
    {
      return (ETCPAL_IP_V4_ADDRESS(ip) == 0);
    }
    if (ETCPAL_IP_IS_V6(ip))
    {
      return (0 == memcmp(ETCPAL_IP_V6_ADDRESS(ip), kV6Wildcard, ETCPAL_IPV6_BYTES));
    }
  }
  return false;
}

/**
 * @brief Initialize a EtcPalIpAddr with a wildcard address.
 *
 * Works for both IPv4 and IPv6 addresses. The wildcard address is used as an argument to
 * etcpal_bind() to indicate that a socket should be bound to all available network interfaces. It
 * should not be used as a placeholder or invalid address - use ETCPAL_IP_SET_INVALID() and
 * ETCPAL_IP_IS_INVALID() for that.
 *
 * @param[in] type Type of wildcard to create, either IPv4 or IPv6.
 * @param[out] ip Address in which to store the wildcard value.
 */
void etcpal_ip_set_wildcard(etcpal_iptype_t type, EtcPalIpAddr* ip)
{
  if (ip)
  {
    switch (type)
    {
      case kEtcPalIpTypeV4:
        ETCPAL_IP_SET_V4_ADDRESS(ip, 0);
        break;
      case kEtcPalIpTypeV6:
        ETCPAL_IP_SET_V6_ADDRESS(ip, kV6Wildcard);
        break;
      default:
        ETCPAL_IP_SET_INVALID(ip);
        break;
    }
  }
}

/**
 * @brief Compare two EtcPalIpAddrs.
 *
 * Rules for comparison:
 * * All Invalid addresses are considered to be equal to each other and < all IPv4 and IPv6
 *   addresses
 * * All IPv4 addresses are considered to be < all IPv6 addresses
 * * For two IPv4 or IPv6 addresses, the numerical address value is compared
 *
 * @param[in] ip1 First EtcPalIpAddr to compare.
 * @param[in] ip2 Second EtcPalIpAddr to compare.
 * @return < 0: ip1 < ip2
 * @return 0: ip1 == ip2
 * @return > 0: ip1 > ip2
 */
int etcpal_ip_cmp(const EtcPalIpAddr* ip1, const EtcPalIpAddr* ip2)
{
  if (ip1 && ip2)
  {
    if (ip1->type != ip2->type)
    {
      return (int)(ip1->type - ip2->type);
    }
    if (ip1->type == kEtcPalIpTypeV4)
    {
      return (ETCPAL_IP_V4_ADDRESS(ip1) > ETCPAL_IP_V4_ADDRESS(ip2)) -
             (ETCPAL_IP_V4_ADDRESS(ip1) < ETCPAL_IP_V4_ADDRESS(ip2));
    }
    if (ip1->type == kEtcPalIpTypeV6)
    {
      return memcmp(ETCPAL_IP_V6_ADDRESS(ip1), ETCPAL_IP_V6_ADDRESS(ip2), ETCPAL_IPV6_BYTES);
    }
  }
  return 0;
}

/**
 * @brief Determine whether two instances of EtcPalSockAddr contain identical IP addresses and ports.
 * @param[in] sock1 First EtcPalSockAddr to compare.
 * @param[in] sock2 Second EtcPalSockAddr to compare.
 * @return true: the IP address and port are identical.
 * @return false: the IP address and port are not identical.
 */
bool etcpal_ip_and_port_equal(const EtcPalSockAddr* sock1, const EtcPalSockAddr* sock2)
{
  if (sock1 && sock2)
    return ((etcpal_ip_cmp(&sock1->ip, &sock2->ip) == 0) && sock1->port == sock2->port);
  return false;
}

/**
 * @brief Get the length in bits of a netmask.
 *
 * Counts the number of set ('1') bits in the netmask, starting from the MSB. Works for both IPv4
 * and IPv6 netmasks.
 *
 * For example:
 * netmask = 255.255.0.0 (0xffff0000); result = 16
 * netmask = ffff:ffff:ffff:ffff::; result = 64
 *
 * @param[in] netmask Netmask to count.
 * @return Number of set bits in the netmask.
 */
unsigned int etcpal_ip_mask_length(const EtcPalIpAddr* netmask)
{
  unsigned int length = 0;

  if (netmask)
  {
    if (ETCPAL_IP_IS_V4(netmask))
    {
      uint32_t addr_val = ETCPAL_IP_V4_ADDRESS(netmask);
      uint32_t bit_mask = 0x80000000u;
      while (addr_val & bit_mask)
      {
        ++length;
        bit_mask >>= 1;
      }
    }
    else if (ETCPAL_IP_IS_V6(netmask))
    {
      const uint8_t* addr_buf = ETCPAL_IP_V6_ADDRESS(netmask);
      size_t         addr_index = 0;
      while (addr_index < ETCPAL_IPV6_BYTES && addr_buf[addr_index] == 0xffu)
      {
        length += 8;
        addr_index++;
      }

      if (addr_index < ETCPAL_IPV6_BYTES)
      {
        uint8_t addr_val = addr_buf[addr_index];
        uint8_t bit_mask = 0x80u;
        for (; bit_mask != 0; bit_mask >>= 1, ++length)
        {
          if (!(addr_val & bit_mask))
            break;
        }
      }
    }
  }

  return length;
}

/**
 * @brief Create a netmask given a length in bits.
 *
 * Creates either an IPv4 or IPv6 netmask, setting the most-significant mask_length bits.
 *
 * For example:
 * type = kEtcPalIpTypeV4; mask_length = 16; result = 255.255.0.0
 * type = kEtcPalIpTypeV6; mask_length = 64; result = ffff:ffff:ffff:ffff::
 *
 * @param[in] type Type of netmask to create, either IPv4 or IPv6.
 * @param[in] mask_length Length in bits of the mask, counting from the MSB.
 * @return EtcPalIpAddr containing the netmask.
 */
EtcPalIpAddr etcpal_ip_mask_from_length(etcpal_iptype_t type, unsigned int mask_length)
{
  EtcPalIpAddr result;

  if (type == kEtcPalIpTypeV4)
  {
    uint32_t     mask_val = 0;
    uint32_t     bit_mask = 0x80000000u;
    unsigned int adjusted_length = (mask_length > 32 ? 32 : mask_length);
    for (unsigned int i = 0; i < adjusted_length; ++i, bit_mask >>= 1)
    {
      mask_val |= bit_mask;
    }

    ETCPAL_IP_SET_V4_ADDRESS(&result, mask_val);
  }
  else if (type == kEtcPalIpTypeV6)
  {
    uint8_t mask_buf[ETCPAL_IPV6_BYTES];
    memset(mask_buf, 0, sizeof(mask_buf));
    unsigned int adjusted_length = (mask_length > 128 ? 128 : mask_length);

    size_t mask_index = 0;
    while (adjusted_length / 8 > 0)
    {
      mask_buf[mask_index++] = 0xff;
      adjusted_length -= 8;
    }

    uint8_t bit_mask = 0x80u;
    do
    {
      if (adjusted_length == 0)
        break;
      mask_buf[mask_index] |= bit_mask;
      --adjusted_length;
      bit_mask >>= 1;
    } while (bit_mask != 0);

    ETCPAL_IP_SET_V6_ADDRESS(&result, mask_buf);
  }
  else
  {
    ETCPAL_IP_SET_INVALID(&result);
  }
  return result;
}

/**
 * @brief Compare the network portions of two IP addresses using a netmask.
 *
 * Returns true if the _network_ portions of the IP addresses match. The host portions are
 * disregarded.
 *
 * Some examples:
 * ip1 = 192.168.0.1; ip2 = 192.168.0.2; netmask = 255.255.0.0 (/16); result = true
 * ip1 = 192.168.0.1; ip2 = 192.168.1.1; netmask = 255.255.255.0 (/24); result = false
 * ip1 = 2001:db8::1; ip2 = 2001:db8::2; netmask = ffff:ffff:ffff:ffff:: (/64); result = true
 * ip1 = 2001:db8:1::1; ip2 = 2001:db8:2::1; netmask = ffff:ffff:ffff:ffff:: (/64); result = false
 *
 * @param[in] ip1 First EtcPalIpAddr to compare.
 * @param[in] ip2 Second EtcPalIpAddr to compare.
 * @param[in] netmask The netmask to use for the comparison. This mask is used to determine the
 *                    network portion of each address. The etcpal_ip_mask_from_length() function can
 *                    be used to turn a mask or prefix bit length into a mask.
 * @return true: The network portions of the IP addresses are equal.
 * @return false: The network portions are not equal.
 */
bool etcpal_ip_network_portions_equal(const EtcPalIpAddr* ip1, const EtcPalIpAddr* ip2, const EtcPalIpAddr* netmask)
{
  if (!ip1 || !ip2 || !netmask)
    return false;

  if (ETCPAL_IP_IS_V4(ip1) && ETCPAL_IP_IS_V4(ip2) && ETCPAL_IP_IS_V4(netmask))
  {
    return ((ETCPAL_IP_V4_ADDRESS(ip1) & ETCPAL_IP_V4_ADDRESS(netmask)) ==
            (ETCPAL_IP_V4_ADDRESS(ip2) & ETCPAL_IP_V4_ADDRESS(netmask)));
  }

  if (ETCPAL_IP_IS_V6(ip1) && ETCPAL_IP_IS_V6(ip2) && ETCPAL_IP_IS_V6(netmask))
  {
    const uint8_t* p1 = ETCPAL_IP_V6_ADDRESS(ip1);
    const uint8_t* p2 = ETCPAL_IP_V6_ADDRESS(ip2);
    const uint8_t* pm = ETCPAL_IP_V6_ADDRESS(netmask);

    for (size_t i = 0; i < ETCPAL_IPV6_BYTES; ++i, ++p1, ++p2, ++pm)
    {
      if ((*p1 & *pm) != (*p2 & *pm))
        return false;
    }
    return true;
  }

  return false;
}

/* This documentation appears here; the actual functions are in os/[os name]/etcpal/os_inet.c */

/**
 * @fn bool ip_os_to_etcpal(const etcpal_os_ipaddr_t* os_ip, EtcPalIpAddr* ip)
 * @brief Convert a platform-specific IP address to an EtcPalIpAddr.
 *
 * Most of the time etcpal_os_ipaddr_t is defined as a struct sockaddr. Since most platforms use
 * struct sockaddr (which holds an IP address, port, and additional information) in functions which
 * only require IP addresses, this function will discard some information, including any port value
 * in the sockaddr.
 *
 * @param[in] os_ip Platform-specific IP address to convert from.
 * @param[out] ip Filled in on success with the EtcPalIpAddr.
 * @return true on success, false if an invalid argument was provided.
 */

/**
 * @fn size_t ip_etcpal_to_os(const EtcPalIpAddr* ip, etcpal_os_ipaddr_t* os_ip)
 * @brief Convert an EtcPalIpAddr to a platform-specific IP address.
 *
 * Most of the time etcpal_os_ipaddr_t is defined as a struct sockaddr. Since most platforms use
 * struct sockaddr (which holds an IP address, port, and additional information) in functions which
 * only require IP addresses, this function will fill in only part of the etcpal_os_ipaddr_t
 * (the address family, address, and for IPv6, the scope ID).
 *
 * @param[in] ip The EtcPalIpAddr to convert from.
 * @param[out] os_ip Filled in on success with the platform-specific IP address.
 * @return On success, the size of os_ip, for convenience. Returns 0 if an invalid argument was
 *         provided.
 */

/**
 * @fn bool sockaddr_os_to_etcpal(const etcpal_os_sockaddr_t* os_sa, EtcPalSockAddr* sa)
 * @brief Convert a platform-specific socket address to an EtcPalSockAddr.
 *
 * On most platforms, etcpal_os_sockaddr_t is defined to struct sockaddr.
 *
 * @param[in] os_sa Platform-specific socket address to convert from.
 * @param[out] sa Filled in on success with the EtcPalSockAddr.
 * @return true on success, false if an invalid argument was provided.
 */

/**
 * @fn size_t sockaddr_etcpal_to_os(const EtcPalSockAddr* sa, etcpal_os_sockaddr_t* os_sa)
 * @brief Convert an EtcPalSockAddr to a platform-specific socket address.
 *
 * On most platforms, etcpal_os_sockaddr_t is defined to struct sockaddr.
 *
 * @param[in] sa The EtcPalSockAddr to convert from.
 * @param[out] os_sa Filled in on success with the platform-specific socket address.
 * @return On success, the size of os_sa, for convenience. Returns 0 if an invalid argument was
 *         provided.
 */

/**
 * @fn etcpal_error_t etcpal_ip_to_string(const EtcPalIpAddr* src, char* dest)
 * @brief Convert IPv4 and IPv6 addresses from binary to text form.
 *
 * This function uses each platform's inet_ntop() function under the hood.
 *
 * @param[in] src Address to convert to string form.
 * @param[out] dest Filled in on success with the string-represented address. To avoid undefined
 *                  behavior, this buffer must be at least of size #ETCPAL_IP_STRING_BYTES.
 * @return #kEtcPalErrOk: Success.
 * @return #kEtcPalErrInvalid: Invalid parameter.
 * @return #kEtcPalErrSys: System call failed.
 */

/* This documentation appears here; the actual functions are in os/[os name]/etcpal/os_inet.c */
/**
 * @fn etcpal_error_t etcpal_string_to_ip(etcpal_iptype_t type, const char* src, EtcPalIpAddr* dest)
 * @brief Convert IPv4 and IPv6 addresses from text to binary form.
 *
 * This function uses each platform's inet_pton() function under the hood.
 *
 * @param[in] type Type of string-represented IP address pointed to by src.
 * @param[in] src Character string containing a string-represented IP address.
 * @param[out] dest Filled in on success with the address.
 * @return #kEtcPalErrOk: Success.
 * @return #kEtcPalErrInvalid: Invalid parameter.
 * @return #kEtcPalErrSys: System call failed.
 */

/**
 * @brief Create a string representation of a MAC address.
 *
 * The resulting string will be of the form xx:xx:xx:xx:xx:xx (lowercase is used for hexadecimal
 * letters per common convention).
 *
 * @param[in] src MAC address to convert to a string.
 * @param[out] dest Character buffer to which to write the resulting string. Must be at least of
 *                  size #ETCPAL_MAC_STRING_BYTES.
 * @return #kEtcPalErrOk: Conversion successful.
 * @return #kEtcPalErrInvalid: Invalid argument.
 */
etcpal_error_t etcpal_mac_to_string(const EtcPalMacAddr* src, char* dest)
{
  if (!src || !dest)
    return kEtcPalErrInvalid;

  ETCPAL_SPRINTF(dest, "%02x:%02x:%02x:%02x:%02x:%02x", src->data[0], src->data[1], src->data[2], src->data[3],
                 src->data[4], src->data[5]);
  return kEtcPalErrOk;
}

/**
 * @brief Create a MAC address from a string representation.
 *
 * Parses a string-represented MAC address and fills in an EtcPalMacAddr structure with the result.
 * The input should be of the form: xx:xx:xx:xx:xx:xx, or xxxxxxxxxxxx (hexadecimal letters can be
 * in upper- or lowercase).
 *
 * @param[in] src The null-terminated string to convert.
 * @param[out] dest MAC address structure to fill in with the parse result.
 * @return #kEtcPalErrOk: Parse successful.
 * @return #kEtcPalErrInvalid: Parse failed or invalid argument.
 */
etcpal_error_t etcpal_string_to_mac(const char* src, EtcPalMacAddr* dest)
{
  if (!src || !dest)
    return kEtcPalErrInvalid;

  const char* from_ptr = src;
  uint8_t     to_buf[ETCPAL_MAC_BYTES];
  uint8_t*    to_ptr = to_buf;
  bool        first = true; /* Whether we are doing the first or second nibble of the byte */

  while ((to_ptr - to_buf < ETCPAL_MAC_BYTES) && (*from_ptr != '\0'))
  {
    uint8_t offset = 0;
    if ((*from_ptr >= '0') && (*from_ptr <= '9'))
      offset = '0';
    else if ((*from_ptr >= 'A') && (*from_ptr <= 'F'))
      offset = 0x37; /* 0x41('A') - 0x37('7') = 0xa */
    else if ((*from_ptr >= 'a') && (*from_ptr <= 'f'))
      offset = 0x57; /* 0x61('a') - 0x57('W') = 0xa */

    if (offset == 0)
      break;

    if (first)
    {
      *to_ptr = (uint8_t)(*from_ptr - offset);
      *to_ptr = (uint8_t)(*to_ptr << 4);
      first = false;
    }
    else
    {
      *to_ptr |= (uint8_t)(*from_ptr - offset);
      ++to_ptr;
      first = true;
    }

    if (*(++from_ptr) == ':')
      ++from_ptr;
  }

  if (to_ptr == to_buf + ETCPAL_MAC_BYTES)
  {
    memcpy(dest->data, to_buf, ETCPAL_MAC_BYTES);
    return kEtcPalErrOk;
  }
  return kEtcPalErrInvalid;
}
