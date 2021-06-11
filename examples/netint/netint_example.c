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

/*
 * An example application showing usage of the etcpal_netint functions.
 * This example will print information about each network interface on your machine. It is
 * platform-neutral.
 */

#include <assert.h>
#include <stdio.h>
#include "etcpal/netint.h"
#include "etcpal/socket.h"

#define FORMAT_BUF_SIZE 50

static char header_format[FORMAT_BUF_SIZE];
static char line_format[FORMAT_BUF_SIZE];

#define ID_COL_HEADER "ID"
#define ADDR_COL_HEADER "Address"
#define NETMASK_COL_HEADER "Netmask"
#define MAC_COL_HEADER "MAC"
#define INDEX_COL_HEADER "OS Index"
#define STATE_COL_HEADER "State"

// Create a format string for printf based on the column width of the longest entry in each column
void create_format_strings(const EtcPalNetintInfo* netint_arr, size_t num_interfaces)
{
  size_t longest_id = 0;
  size_t longest_addr = 0;
  size_t longest_netmask = 0;

  for (const EtcPalNetintInfo* netint = netint_arr; netint < netint_arr + num_interfaces; ++netint)
  {
    char addr_str[ETCPAL_IP_STRING_BYTES] = {'\0'};
    char netmask_str[ETCPAL_IP_STRING_BYTES] = {'\0'};

    etcpal_ip_to_string(&netint->addr, addr_str);
    etcpal_ip_to_string(&netint->mask, netmask_str);

    size_t id_len = strlen(netint->id);
    size_t addr_len = strlen(addr_str);
    size_t netmask_len = strlen(netmask_str);

    if (id_len > longest_id)
      longest_id = id_len;
    if (addr_len > longest_addr)
      longest_addr = addr_len;
    if (netmask_len > longest_netmask)
      longest_netmask = netmask_len;
  }

  if (longest_id < sizeof(ID_COL_HEADER))
    longest_id = sizeof(ID_COL_HEADER);
  if (longest_addr < sizeof(ADDR_COL_HEADER))
    longest_addr = sizeof(ADDR_COL_HEADER);
  if (longest_netmask < sizeof(NETMASK_COL_HEADER))
    longest_netmask = sizeof(NETMASK_COL_HEADER);

  snprintf(header_format, FORMAT_BUF_SIZE, "%%-%zus %%-%zus %%-%zus %%-17s %%s %%s\n", longest_id, longest_addr,
           longest_netmask);
  snprintf(line_format, FORMAT_BUF_SIZE, "%%-%zus %%-%zus %%-%zus %%-17s %%8u %%s\n", longest_id, longest_addr,
           longest_netmask);
}

int main(void)
{
  etcpal_error_t init_res = etcpal_init(ETCPAL_FEATURE_NETINTS);
  if (init_res != kEtcPalErrOk)
  {
    printf("etcpal_init() failed with error: '%s'\n", etcpal_strerror(init_res));
    return 1;
  }

  size_t num_interfaces = etcpal_netint_get_num_interfaces();
  if (num_interfaces == 0)
  {
    printf("Error: No network interfaces found on system.\n");
    etcpal_deinit(ETCPAL_FEATURE_NETINTS);
    return 1;
  }

  const EtcPalNetintInfo* netint_arr = etcpal_netint_get_interfaces();
  assert(netint_arr);

  create_format_strings(netint_arr, num_interfaces);

  printf("Network interfaces found:\n");
  printf(header_format, ID_COL_HEADER, ADDR_COL_HEADER, NETMASK_COL_HEADER, MAC_COL_HEADER, INDEX_COL_HEADER,
         STATE_COL_HEADER);

  for (const EtcPalNetintInfo* netint = netint_arr; netint < netint_arr + num_interfaces; ++netint)
  {
    char addr_str[ETCPAL_IP_STRING_BYTES] = {'\0'};
    char netmask_str[ETCPAL_IP_STRING_BYTES] = {'\0'};
    char mac_str[ETCPAL_MAC_STRING_BYTES];
    etcpal_ip_to_string(&netint->addr, addr_str);
    etcpal_ip_to_string(&netint->mask, netmask_str);
    etcpal_mac_to_string(&netint->mac, mac_str);
    printf(line_format, netint->id, addr_str, netmask_str, mac_str, netint->index,
           etcpal_netint_is_up(netint->index) ? "Up" : "Down");
  }

  unsigned int default_v4 = 0;
  if (kEtcPalErrOk == etcpal_netint_get_default_interface(kEtcPalIpTypeV4, &default_v4))
  {
    const EtcPalNetintInfo* addr_arr = NULL;
    size_t                  addr_arr_size = 0;
    if (kEtcPalErrOk == etcpal_netint_get_interfaces_by_index(default_v4, &addr_arr, &addr_arr_size))
    {
      printf("Default IPv4 interface: %s (%u)\n", addr_arr->friendly_name, default_v4);
    }
  }

  unsigned int default_v6 = 0;
  if (kEtcPalErrOk == etcpal_netint_get_default_interface(kEtcPalIpTypeV6, &default_v6))
  {
    const EtcPalNetintInfo* addr_arr = NULL;
    size_t                  addr_arr_size = 0;
    if (kEtcPalErrOk == etcpal_netint_get_interfaces_by_index(default_v6, &addr_arr, &addr_arr_size))
    {
      printf("Default IPv6 interface: %s (%u)\n", addr_arr->friendly_name, default_v6);
    }
  }

  etcpal_deinit(ETCPAL_FEATURE_NETINTS);

  return 0;
}
