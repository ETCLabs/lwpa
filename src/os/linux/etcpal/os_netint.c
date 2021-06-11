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

/* The os_netint implementation for Linux.
 *
 * Uses getifaddrs() to get an initial list of network interfaces. More info:
 * http://man7.org/linux/man-pages/man3/getifaddrs.3.html
 *
 * Then uses netlink sockets (specifically RTNETLINK) to fill out missing information about each
 * interface. More info:
 *
 * Netlink sockets: http://man7.org/linux/man-pages/man7/netlink.7.html
 * Netlink macros, for decoding netlink messages: http://man7.org/linux/man-pages/man3/netlink.3.html
 * RtNetlink sockets: http://man7.org/linux/man-pages/man7/rtnetlink.7.html
 * Some sample RtNetlink code: https://www.linuxjournal.com/article/8498
 */

#include "etcpal/netint.h"

#include <stdlib.h>
#include <limits.h>
#include <string.h>

#ifdef ETCPAL_NETINT_DEBUG_OUTPUT
#include <stdio.h>
#endif

#include <ifaddrs.h>
#include <errno.h>
#include <asm/types.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "etcpal/common.h"
#include "etcpal/socket.h"
#include "etcpal/private/netint.h"
#include "os_error.h"

/***************************** Private types *********************************/

typedef struct RoutingTableEntry
{
  EtcPalIpAddr addr;
  EtcPalIpAddr mask;
  EtcPalIpAddr gateway;
  int          interface_index;
  int          metric;
} RoutingTableEntry;

typedef struct RoutingTable
{
  RoutingTableEntry* entries;
  RoutingTableEntry* default_route;
  size_t             size;
} RoutingTable;

/* A composite struct representing an RT_NETLINK request sent over a netlink socket. */
typedef struct RtNetlinkRequest
{
  struct nlmsghdr nl_header;
  struct rtmsg    rt_msg;
} RtNetlinkRequest;

/**************************** Private variables ******************************/

RoutingTable routing_table_v4;
RoutingTable routing_table_v6;

/*********************** Private function prototypes *************************/

// Functions for building the routing tables
static etcpal_error_t build_routing_tables(void);
static etcpal_error_t build_routing_table(int family, RoutingTable* table);
static void           free_routing_tables(void);
static void           free_routing_table(RoutingTable* table);

// Interacting with RTNETLINK
static etcpal_error_t send_netlink_route_request(int socket, int family);
static etcpal_error_t receive_netlink_route_reply(int sock, int family, size_t buf_size, RoutingTable* table);
static etcpal_error_t parse_netlink_route_reply(int           family,
                                                const char*   buffer,
                                                size_t        nl_msg_size,
                                                RoutingTable* table);

// Manipulating routing table entries
static void init_routing_table_entry(RoutingTableEntry* entry);
static int  compare_routing_table_entries(const void* a, const void* b);

#if ETCPAL_NETINT_DEBUG_OUTPUT
static void debug_print_routing_table(RoutingTable* table);
#endif

/*************************** Function definitions ****************************/

/* Quick helper for enumerate_netints() to determine entries to skip in the linked list. */
static bool should_skip_ifaddr(const struct ifaddrs* ifaddr)
{
  // Skip an entry if it doesn't have an address, or if the address is not IPv4 or IPv6.
  return (!ifaddr->ifa_addr || (ifaddr->ifa_addr->sa_family != AF_INET && ifaddr->ifa_addr->sa_family != AF_INET6));
}

etcpal_error_t os_enumerate_interfaces(CachedNetintInfo* cache)
{
  etcpal_error_t res = build_routing_tables();
  if (res != kEtcPalErrOk)
    return res;

  // Fill in the default index information
  if (routing_table_v4.default_route)
  {
    cache->def.v4_valid = true;
    cache->def.v4_index = routing_table_v4.default_route->interface_index;
  }
  if (routing_table_v6.default_route)
  {
    cache->def.v6_valid = true;
    cache->def.v6_index = routing_table_v6.default_route->interface_index;
  }

  // Create the OS resources necessary to enumerate the interfaces
  int ioctl_sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (ioctl_sock == -1)
    return errno_os_to_etcpal(errno);

  struct ifaddrs* os_addrs = NULL;
  if (getifaddrs(&os_addrs) < 0)
  {
    close(ioctl_sock);
    return errno_os_to_etcpal(errno);
  }

  // Pass 1: Total the number of addresses
  cache->num_netints = 0;
  for (struct ifaddrs* ifaddr = os_addrs; ifaddr; ifaddr = ifaddr->ifa_next)
  {
    if (should_skip_ifaddr(ifaddr))
      continue;

    ++cache->num_netints;
  }

  if (cache->num_netints == 0)
  {
    freeifaddrs(os_addrs);
    close(ioctl_sock);
    return kEtcPalErrNoNetints;
  }

  // Allocate our interface array
  cache->netints = (EtcPalNetintInfo*)calloc(cache->num_netints, sizeof(EtcPalNetintInfo));
  if (!cache->netints)
  {
    freeifaddrs(os_addrs);
    close(ioctl_sock);
    return kEtcPalErrNoMem;
  }

  // Pass 2: Fill in all the info about each address
  size_t current_etcpal_index = 0;
  for (struct ifaddrs* ifaddr = os_addrs; ifaddr; ifaddr = ifaddr->ifa_next)
  {
    if (should_skip_ifaddr(ifaddr))
      continue;

    EtcPalNetintInfo* current_info = &cache->netints[current_etcpal_index];

    // Interface name
    strncpy(current_info->id, ifaddr->ifa_name, ETCPAL_NETINTINFO_ID_LEN);
    current_info->id[ETCPAL_NETINTINFO_ID_LEN - 1] = '\0';
    strncpy(current_info->friendly_name, ifaddr->ifa_name, ETCPAL_NETINTINFO_FRIENDLY_NAME_LEN);
    current_info->friendly_name[ETCPAL_NETINTINFO_FRIENDLY_NAME_LEN - 1] = '\0';

    // Interface address
    ip_os_to_etcpal(ifaddr->ifa_addr, &current_info->addr);

    // Interface netmask
    ip_os_to_etcpal(ifaddr->ifa_netmask, &current_info->mask);

    // Struct ifreq to use with ioctl() calls
    struct ifreq if_req;
    strncpy(if_req.ifr_name, ifaddr->ifa_name, IFNAMSIZ);

    // Hardware address
    int ioctl_res = ioctl(ioctl_sock, SIOCGIFHWADDR, &if_req);
    if (ioctl_res == 0)
      memcpy(current_info->mac.data, if_req.ifr_hwaddr.sa_data, ETCPAL_MAC_BYTES);
    else
      memset(current_info->mac.data, 0, ETCPAL_MAC_BYTES);

    // Interface index
    ioctl_res = ioctl(ioctl_sock, SIOCGIFINDEX, &if_req);
    if (ioctl_res == 0)
      current_info->index = (unsigned int)if_req.ifr_ifindex;
    else
      current_info->index = 0;

    // Is Default
    if (ETCPAL_IP_IS_V4(&current_info->addr) && routing_table_v4.default_route &&
        current_info->index == routing_table_v4.default_route->interface_index)
    {
      current_info->is_default = true;
    }
    else if (ETCPAL_IP_IS_V6(&current_info->addr) && routing_table_v6.default_route &&
             current_info->index == routing_table_v6.default_route->interface_index)
    {
      current_info->is_default = true;
    }

    current_etcpal_index++;
  }

  freeifaddrs(os_addrs);
  close(ioctl_sock);
  return kEtcPalErrOk;
}

void os_free_interfaces(CachedNetintInfo* cache)
{
  if (cache->netints)
  {
    free(cache->netints);
    cache->netints = NULL;
  }

  free_routing_tables();
}

etcpal_error_t os_resolve_route(const EtcPalIpAddr* dest, const CachedNetintInfo* cache, unsigned int* index)
{
  ETCPAL_UNUSED_ARG(cache);

  RoutingTable* table_to_use = (ETCPAL_IP_IS_V6(dest) ? &routing_table_v6 : &routing_table_v4);

  unsigned int index_found = 0;
  for (RoutingTableEntry* entry = table_to_use->entries; entry < table_to_use->entries + table_to_use->size; ++entry)
  {
    if (entry->interface_index <= 0 || ETCPAL_IP_IS_INVALID(&entry->mask))
      continue;

    // Check each route to see if it matches the destination address explicitly
    if (etcpal_ip_network_portions_equal(&entry->addr, dest, &entry->mask))
    {
      index_found = (unsigned int)entry->interface_index;
      break;
    }
  }

  // Fall back to the default route
  if (index_found == 0 && table_to_use->default_route)
    index_found = (unsigned int)table_to_use->default_route->interface_index;

  if (index_found > 0)
  {
    *index = index_found;
    return kEtcPalErrOk;
  }
  return kEtcPalErrNotFound;
}

bool os_netint_is_up(unsigned int index, const CachedNetintInfo* cache)
{
  ETCPAL_UNUSED_ARG(cache);

  int ioctl_sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (ioctl_sock == -1)
    return false;

  // Translate the index to a name
  struct ifreq if_req;
  if_req.ifr_ifindex = (int)index;
  int ioctl_res = ioctl(ioctl_sock, SIOCGIFNAME, &if_req);
  if (ioctl_res != 0)
  {
    close(ioctl_sock);
    return false;
  }

  // Get the flags for the interface with the given name
  ioctl_res = ioctl(ioctl_sock, SIOCGIFFLAGS, &if_req);
  close(ioctl_sock);
  if (ioctl_res == 0)
  {
    return (bool)(if_req.ifr_flags & IFF_UP);
  }
  return false;
}

etcpal_error_t build_routing_tables(void)
{
  etcpal_error_t res = build_routing_table(AF_INET, &routing_table_v4);
  if (res == kEtcPalErrOk)
  {
    res = build_routing_table(AF_INET6, &routing_table_v6);
  }

  if (res != kEtcPalErrOk)
    free_routing_tables();

#if ETCPAL_NETINT_DEBUG_OUTPUT
  debug_print_routing_table(&routing_table_v4);
  debug_print_routing_table(&routing_table_v6);
#endif

  return res;
}

etcpal_error_t build_routing_table(int family, RoutingTable* table)
{
  // Create a netlink socket, send a netlink request to get the routing table, and receive the
  // reply. If the buffer was not big enough, repeat (cannot reuse the same socket because we've
  // often received partial messages that must be discarded)

  etcpal_error_t result = kEtcPalErrOk;
  bool           done = false;
  size_t         recv_buf_size = 2048;  // Tests show this is usually enough for small routing tables
  while (result == kEtcPalErrOk && !done)
  {
    struct sockaddr_nl addr;
    memset(&addr, 0, sizeof(struct sockaddr_nl));
    addr.nl_family = AF_NETLINK;

    int sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (sock == -1)
      result = errno_os_to_etcpal(errno);

    if (result == kEtcPalErrOk)
    {
      if (0 != bind(sock, (struct sockaddr*)&addr, sizeof(addr)))
        result = errno_os_to_etcpal(errno);
    }

    if (result == kEtcPalErrOk)
      result = send_netlink_route_request(sock, family);

    if (result == kEtcPalErrOk)
    {
      result = receive_netlink_route_reply(sock, family, recv_buf_size, table);
      switch (result)
      {
        case kEtcPalErrOk:
          done = true;
          break;
        case kEtcPalErrBufSize:
          recv_buf_size *= 2;  // Double the buffer size and try again.
          result = kEtcPalErrOk;
          break;
        default:
          break;
      }
    }

    close(sock);
  }
  return result;
}

etcpal_error_t send_netlink_route_request(int socket, int family)
{
  // Build the request
  RtNetlinkRequest req;
  memset(&req, 0, sizeof(req));
  req.nl_header.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
  req.nl_header.nlmsg_type = RTM_GETROUTE;
  req.nl_header.nlmsg_flags = (__u16)(NLM_F_REQUEST | NLM_F_DUMP);
  req.nl_header.nlmsg_pid = (__u32)getpid();
  req.rt_msg.rtm_family = (unsigned char)family;
  req.rt_msg.rtm_table = RT_TABLE_MAIN;

  // Send it to the kernel
  struct sockaddr_nl naddr;
  memset(&naddr, 0, sizeof(naddr));
  naddr.nl_family = AF_NETLINK;

  if (sendto(socket, &req.nl_header, req.nl_header.nlmsg_len, 0, (struct sockaddr*)&naddr, sizeof(naddr)) >= 0)
    return kEtcPalErrOk;
  return errno_os_to_etcpal(errno);
}

etcpal_error_t receive_netlink_route_reply(int sock, int family, size_t buf_size, RoutingTable* table)
{
  // Allocate slightly larger than buf_size, so we can detect when more room is needed
  size_t real_size = buf_size + 20;
  char*  buffer = (char*)malloc(real_size);
  if (!buffer)
    return kEtcPalErrNoMem;
  memset(buffer, 0, real_size);

  char*  cur_ptr = buffer;
  size_t nl_msg_size = 0;

  // Read from the socket until the NLMSG_DONE is returned in the type of the RTNETLINK message
  while (1)
  {
    // While we are receiving with real_size, checking against buf_size will detect when we've
    // passed the limit given by the app
    if (buf_size <= nl_msg_size)
    {
      free(buffer);
      return kEtcPalErrBufSize;
    }

    ssize_t recv_res = recv(sock, cur_ptr, real_size - nl_msg_size, 0);
    if (recv_res == -1)
    {
      free(buffer);
      return errno_os_to_etcpal(errno);
    }

    struct nlmsghdr* nl_header = (struct nlmsghdr*)cur_ptr;

    if (nl_header->nlmsg_type == NLMSG_DONE)
      break;

    // Adjust our position in the buffer and size received
    cur_ptr += recv_res;
    nl_msg_size += (size_t)recv_res;
  }

  etcpal_error_t parse_res = parse_netlink_route_reply(family, buffer, nl_msg_size, table);

  free(buffer);
  return parse_res;
}

etcpal_error_t parse_netlink_route_reply(int family, const char* buffer, size_t nl_msg_size, RoutingTable* table)
{
  table->size = 0;
  table->entries = NULL;
  table->default_route = NULL;

  // Parse the result
  // outer loop: loops thru all the NETLINK headers that also include the route entry header
  struct nlmsghdr* nl_header = (struct nlmsghdr*)buffer;
  for (; NLMSG_OK(nl_header, nl_msg_size); nl_header = NLMSG_NEXT(nl_header, nl_msg_size))
  {
    RoutingTableEntry new_entry;
    init_routing_table_entry(&new_entry);

    bool new_entry_valid = true;

    // get route entry header
    struct rtmsg* rt_message = (struct rtmsg*)NLMSG_DATA(nl_header);

    // Filter out entries from the local routing table. Netlink seems to give us those even though
    // we only asked for the main one.
    if (rt_message->rtm_type != RTN_LOCAL && rt_message->rtm_type != RTN_BROADCAST &&
        rt_message->rtm_type != RTN_ANYCAST)
    {
      // inner loop: loop thru all the attributes of one route entry.
      struct rtattr* rt_attributes = (struct rtattr*)RTM_RTA(rt_message);
      unsigned int   rt_attr_size = (unsigned int)RTM_PAYLOAD(nl_header);
      for (; RTA_OK(rt_attributes, rt_attr_size); rt_attributes = RTA_NEXT(rt_attributes, rt_attr_size))
      {
        // We only care about the gateway and DST attribute
        if (rt_attributes->rta_type == RTA_DST)
        {
          if (family == AF_INET6)
            ETCPAL_IP_SET_V6_ADDRESS(&new_entry.addr, ((struct in6_addr*)RTA_DATA(rt_attributes))->s6_addr);
          else
            ETCPAL_IP_SET_V4_ADDRESS(&new_entry.addr, ntohl(((struct in_addr*)RTA_DATA(rt_attributes))->s_addr));
        }
        else if (rt_attributes->rta_type == RTA_GATEWAY)
        {
          if (family == AF_INET6)
            ETCPAL_IP_SET_V6_ADDRESS(&new_entry.gateway, ((struct in6_addr*)RTA_DATA(rt_attributes))->s6_addr);
          else
            ETCPAL_IP_SET_V4_ADDRESS(&new_entry.gateway, ntohl(((struct in_addr*)RTA_DATA(rt_attributes))->s_addr));
        }
        else if (rt_attributes->rta_type == RTA_OIF)
        {
          new_entry.interface_index = *((int*)RTA_DATA(rt_attributes));
        }
        else if (rt_attributes->rta_type == RTA_PRIORITY)
        {
          new_entry.metric = *((int*)RTA_DATA(rt_attributes));
        }
      }
    }
    else
    {
      new_entry_valid = false;
    }

    if (!ETCPAL_IP_IS_INVALID(&new_entry.addr))
    {
      new_entry.mask = etcpal_ip_mask_from_length(new_entry.addr.type, rt_message->rtm_dst_len);
    }

    // Insert the new entry into the list
    if (new_entry_valid)
    {
      ++table->size;
      if (table->entries)
        table->entries = (RoutingTableEntry*)realloc(table->entries, table->size * sizeof(RoutingTableEntry));
      else
        table->entries = (RoutingTableEntry*)malloc(sizeof(RoutingTableEntry));
      table->entries[table->size - 1] = new_entry;
    }
  }

  if (table->size > 0)
  {
    qsort(table->entries, table->size, sizeof(RoutingTableEntry), compare_routing_table_entries);

    // Mark the default route with the lowest metric (the first one we encounter after sorting the
    // table)
    for (RoutingTableEntry* entry = table->entries; entry < table->entries + table->size; ++entry)
    {
      if (ETCPAL_IP_IS_INVALID(&entry->addr) && ETCPAL_IP_IS_INVALID(&entry->mask) &&
          !ETCPAL_IP_IS_INVALID(&entry->gateway))
      {
        table->default_route = entry;
        break;
      }
    }
  }

  return kEtcPalErrOk;
}

void init_routing_table_entry(RoutingTableEntry* entry)
{
  ETCPAL_IP_SET_INVALID(&entry->addr);
  ETCPAL_IP_SET_INVALID(&entry->mask);
  ETCPAL_IP_SET_INVALID(&entry->gateway);
  entry->interface_index = -1;
  entry->metric = INT_MAX;
}

int compare_routing_table_entries(const void* a, const void* b)
{
  RoutingTableEntry* e1 = (RoutingTableEntry*)a;
  RoutingTableEntry* e2 = (RoutingTableEntry*)b;

  unsigned int mask_length_1 = etcpal_ip_mask_length(&e1->mask);
  unsigned int mask_length_2 = etcpal_ip_mask_length(&e2->mask);

  // Sort by mask length in descending order - within the same mask length, sort by metric in
  // ascending order.
  if (mask_length_1 == mask_length_2)
  {
    return (e1->metric > e2->metric) - (e1->metric < e2->metric);
  }
  return (mask_length_1 < mask_length_2) - (mask_length_1 > mask_length_2);
}

void free_routing_tables(void)
{
  free_routing_table(&routing_table_v4);
  free_routing_table(&routing_table_v6);
}

void free_routing_table(RoutingTable* table)
{
  if (table->entries)
  {
    free(table->entries);
    table->entries = NULL;
  }
  table->size = 0;
}

#if ETCPAL_NETINT_DEBUG_OUTPUT
void debug_print_routing_table(RoutingTable* table)
{
  printf("%-40s %-40s %-40s %-10s %s\n", "Address", "Mask", "Gateway", "Metric", "Index");
  for (RoutingTableEntry* entry = table->entries; entry < table->entries + table->size; ++entry)
  {
    char addr_str[ETCPAL_IP_STRING_BYTES];
    char mask_str[ETCPAL_IP_STRING_BYTES];
    char gw_str[ETCPAL_IP_STRING_BYTES];

    if (!ETCPAL_IP_IS_INVALID(&entry->addr))
      etcpal_ip_to_string(&entry->addr, addr_str);
    else
      strcpy(addr_str, "0.0.0.0");

    if (!ETCPAL_IP_IS_INVALID(&entry->mask))
      etcpal_ip_to_string(&entry->mask, mask_str);
    else
      strcpy(mask_str, "0.0.0.0");

    if (!ETCPAL_IP_IS_INVALID(&entry->gateway))
      etcpal_ip_to_string(&entry->gateway, gw_str);
    else
      strcpy(gw_str, "0.0.0.0");

    printf("%-40s %-40s %-40s %-10d %d\n", addr_str, mask_str, gw_str, entry->metric, entry->interface_index);
  }
}
#endif
