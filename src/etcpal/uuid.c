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

#include "etcpal/uuid.h"

#include <stddef.h>
#include "etcpal/common.h"
#include "etcpal/pack.h"
#include "etcpal/thirdparty/md5.h"
#include "etcpal/thirdparty/sha1.h"

/****************************** Private macros *******************************/

#ifdef _MSC_VER
#pragma warning(disable : 4996)
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

const EtcPalUuid kEtcPalNullUuid = {{0}};

/*************************** Function definitions ****************************/

/**
 * @brief Create a string representation of a UUID.
 *
 * The resulting string will be of the form: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx (lowercase is
 * used for hexadecimal letters per RFC 4122 and common convention).
 *
 * @param[in] uuid UUID to convert to a string.
 * @param[out] buf Character buffer to which to write the resulting string. To avoid undefined
 *                 behavior, this buffer must be at least of size #ETCPAL_UUID_STRING_BYTES.
 * @return true (conversion successful) or false (invalid argument).
 */
bool etcpal_uuid_to_string(const EtcPalUuid* uuid, char* buf)
{
  if (!uuid || !buf)
    return false;

  const uint8_t* c = uuid->data;

  /* Minimum buffer size is well documented and cannot be exceeded by this format string. */
  sprintf(buf, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x", c[0], c[1], c[2], c[3], c[4],
          c[5], c[6], c[7], c[8], c[9], c[10], c[11], c[12], c[13], c[14], c[15]);
  return true;
}

/**
 * @brief Create a UUID from a string representation.
 *
 * Parses a string-represented UUID and fills in a EtcPalUuid structure with the result. The input
 * should be of the form: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx (hexadecimal letters can be upper-
 * or lowercase).
 *
 * @param[in] str The null-terminated string to convert.
 * @param[out] uuid UUID to fill in with the parse result.
 * @return true (parse successful) or false (parse failure).
 */
bool etcpal_string_to_uuid(const char* str, EtcPalUuid* uuid)
{
  if (!str || !uuid)
    return false;

  const char* from_ptr = str;
  uint8_t     to_buf[ETCPAL_UUID_BYTES];
  uint8_t*    to_ptr = to_buf;
  bool        first = true; /* Whether we are doing the first or second nibble of the byte */

  while ((to_ptr - to_buf < ETCPAL_UUID_BYTES) && (*from_ptr != '\0'))
  {
    uint8_t offset = 0;
    if ((*from_ptr >= '0') && (*from_ptr <= '9'))
      offset = '0';
    else if ((*from_ptr >= 'A') && (*from_ptr <= 'F'))
      offset = 0x37; /* 0x41('A') - 0x37('7') = 0xa */
    else if ((*from_ptr >= 'a') && (*from_ptr <= 'f'))
      offset = 0x57; /* 0x61('a') - 0x57('W') = 0xa */

    if (offset != 0)
    {
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
    }
    ++from_ptr;
  }

  if (to_ptr == to_buf + ETCPAL_UUID_BYTES)
  {
    memcpy(uuid->data, to_buf, ETCPAL_UUID_BYTES);
    return true;
  }
  return false;
}

#if ETCPAL_NO_OS_SUPPORT || DOXYGEN
/**
 * @brief Generate a Version 1 UUID.
 *
 * This function uses the underlying OS API to create a UUID that is based on a combination of a
 * local MAC address and the current system time. This method guarantees UUID uniqueness across
 * space and time. Some OSes have deprecated this method because it creates a UUID that is
 * traceable to the MAC address of the machine on which it was generated. If this type of security
 * is a concern, etcpal_generate_v4_uuid() should be preferred. If you want to generate UUIDs that
 * are deterministic for a combination of inputs you provide, see etcpal_generate_v3_uuid(),
 * etcpal_generate_v5_uuid() or etcpal_generate_device_uuid().
 *
 * This function may return #kEtcPalErrNotImpl on platforms that do not have this functionality
 * available (this is mostly a concern for RTOS-level embedded platforms).
 *
 * @param[out] uuid UUID to fill in with the generation result.
 * @return #kEtcPalErrOk: UUID generated successfully.
 * @return #kEtcPalErrInvalid: Invalid argument provided.
 * @return #kEtcPalErrNotImpl: This UUID generation method is not available on this platform.
 * @return #kEtcPalErrSys: An internal library of system call error occurred.
 */
etcpal_error_t etcpal_generate_v1_uuid(EtcPalUuid* uuid)
{
  ETCPAL_UNUSED_ARG(uuid);
  return kEtcPalErrNotImpl;
}
#endif

/**
 * @brief Generate a Version 3 UUID.
 *
 * Version 3 UUIDs are "name-based"; they deterministically convert a combination of a "namespace"
 * and "name" to a UUID. A namespace is another UUID of any type which represents a space within
 * which UUIDs generated from different names are unique. A name is opaque data of any length. The
 * same combination of namespace and name will always produce the same Version 3 UUID.
 *
 * Version 3 UUIDs use the MD5 hashing algorithm to convert their inputs into a UUID. Prefer using
 * etcpal_generate_v5_uuid(), which uses the SHA-1 algorithm instead, if backward compatibility is
 * not an issue.
 *
 * @param[in] ns UUID to use as a namespace.
 * @param[in] name The name to convert into a UUID - opaque data.
 * @param[in] name_len The length in bytes of name.
 * @param[out] uuid UUID to fill in with the generation result.
 * @return #kEtcPalErrOk: UUID generated successfully.
 * @return #kEtcPalErrInvalid: Invalid argument provided.
 */
etcpal_error_t etcpal_generate_v3_uuid(const EtcPalUuid* ns, const void* name, size_t name_len, EtcPalUuid* uuid)
{
  if (!ns || !name || name_len == 0 || !uuid)
    return kEtcPalErrInvalid;

  MD5_CTX md5;

  MD5Init(&md5);
  MD5Update(&md5, ns->data, ETCPAL_UUID_BYTES);
  MD5Update(&md5, (const uint8_t*)name, (unsigned int)name_len);
  MD5Final(uuid->data, &md5);

  // Add the UUID flags into the buffer
  // The Version bits to say this is a name-based UUID using MD5
  uuid->data[6] = (uint8_t)(0x30 | (uuid->data[6] & 0x0f));
  // The variant bits to say this is encoded via RFC 4122
  uuid->data[8] = (uint8_t)(0x80 | (uuid->data[8] & 0x3f));

  return kEtcPalErrOk;
}

#if ETCPAL_NO_OS_SUPPORT || DOXYGEN
/**
 * @brief Generate a Version 4 UUID.
 *
 * This function uses the underlying OS API to create a UUID that is based on random data. The
 * quality of the random data used may differ across platforms. If you want to generate UUIDs that
 * are deterministic for a combination of inputs you provide, see etcpal_generate_v3_uuid().
 *
 * This function may return #kEtcPalErrNotImpl on platforms that do not have this functionality
 * available (this is mostly a concern for RTOS-level embedded platforms).
 *
 * @param[out] uuid UUID to fill in with the generation result.
 * @return #kEtcPalErrOk: UUID generated successfully.
 * @return #kEtcPalErrInvalid: Invalid argument provided.
 * @return #kEtcPalErrNotImpl: This UUID generation method is not available on this platform.
 * @return #kEtcPalErrSys: An internal library of system call error occurred.
 */
etcpal_error_t etcpal_generate_v4_uuid(EtcPalUuid* uuid)
{
  ETCPAL_UNUSED_ARG(uuid);
  return kEtcPalErrNotImpl;
}
#endif

/**
 * @brief Generate a Version 5 UUID.
 *
 * Version 5 UUIDs are "name-based"; they deterministically convert a combination of a "namespace"
 * and "name" to a UUID. A namespace is another UUID of any type which represents a space within
 * which UUIDs generated from different names are unique. A name is opaque data of any length. The
 * same combination of namespace and name will always produce the same Version 5 UUID.
 *
 * Version 5 UUIDs use the SHA-1 hashing algorithm to convert their inputs into a UUID.
 *
 * etcpal_generate_device_uuid() provides a specialized form of this function which embedded
 * devices can use to create UUIDs representing themselves.
 *
 * @param[in] ns UUID to use as a namespace.
 * @param[in] name The name to convert into a UUID - opaque data.
 * @param[in] name_len The length in bytes of name.
 * @param[out] uuid UUID to fill in with the generation result.
 * @return #kEtcPalErrOk: UUID generated successfully.
 * @return #kEtcPalErrInvalid: Invalid argument provided.
 */
etcpal_error_t etcpal_generate_v5_uuid(const EtcPalUuid* ns, const void* name, size_t name_len, EtcPalUuid* uuid)
{
  if (!ns || !name || name_len == 0 || !uuid)
    return kEtcPalErrInvalid;

  SHA1_CTX      sha1;
  unsigned char hash[20];

  SHA1Init(&sha1);
  SHA1Update(&sha1, ns->data, ETCPAL_UUID_BYTES);
  SHA1Update(&sha1, name, (uint32_t)name_len);
  SHA1Final(hash, &sha1);

  // Add the UUID flags into the buffer
  // The Version bits to say this is a name-based UUID using SHA-1
  hash[6] = (uint8_t)(0x50 | (hash[6] & 0x0f));
  // The variant bits to say this is encoded via RFC 4122
  hash[8] = (uint8_t)(0x80 | (hash[8] & 0x3f));

  memcpy(uuid->data, hash, ETCPAL_UUID_BYTES);
  return kEtcPalErrOk;
}

#if ETCPAL_NO_OS_SUPPORT || DOXYGEN
/**
 * @brief Generate the preferred UUID version of the underlying OS.
 *
 * This function uses the underlying OS API to create a UUID of the recommended type per the
 * underlying OS API. In practice, this is often a V4 UUID, although this is not guaranteed.
 *
 * This function may return #kEtcPalErrNotImpl on platforms that do not have this functionality
 * available (this is mostly a concern for RTOS-level embedded platforms).
 *
 * @param[out] uuid UUID to fill in with the generation result.
 * @return #kEtcPalErrOk: UUID generated successfully.
 * @return #kEtcPalErrInvalid: Invalid argument provided.
 * @return #kEtcPalErrNotImpl: This UUID generation method is not available on this platform.
 * @return #kEtcPalErrSys: An internal library of system call error occurred.
 */
etcpal_error_t etcpal_generate_os_preferred_uuid(EtcPalUuid* uuid)
{
  ETCPAL_UNUSED_ARG(uuid);
  return kEtcPalErrNotImpl;
}
#endif

/**
 * @brief Generate a UUID from a combination of a custom string and MAC address.
 *
 * This function is for use by embedded devices that want to create UUIDs representing themselves,
 * and create the same UUIDs each time. It creates a Version 5 UUID (defined in RFC 4122) within a
 * constant, hardcoded namespace. The UUID output is deterministic for each combination of dev_str,
 * mac_addr and uuid_num inputs.
 *
 * The namespace UUID used is: 57323103-db01-44b3-bafa-abdee3f37c1a
 *
 * @param[in] dev_str The device-specific string, such as the model name. This should never change
 *                    on the device. It also allows different programs running on the device to
 *                    generate different UUID sets.
 * @param[in] mac_addr The device's MAC address; must be an array of 6 bytes.
 * @param[in] uuid_num Component number. By changing this number, multiple unique UUIDs can be
 *                     generated for the same device string-MAC address combination.
 * @param[out] uuid UUID to fill in with the generation result.
 * @return #kEtcPalErrOk: UUID generated successfully.
 * @return #kEtcPalErrInvalid: Invalid argument provided.
 */
etcpal_error_t etcpal_generate_device_uuid(const char*    dev_str,
                                           const uint8_t* mac_addr,
                                           uint32_t       uuid_num,
                                           EtcPalUuid*    uuid)
{
  if (!dev_str || !mac_addr || !uuid)
    return kEtcPalErrInvalid;

  /* The hardcoded namespace UUID for EtcPal device UUIDs. */
  static const EtcPalUuid kNamespace = {
      {0x57, 0x32, 0x31, 0x03, 0xdb, 0x01, 0x44, 0xb3, 0xba, 0xfa, 0xab, 0xde, 0xe3, 0xf3, 0x7c, 0x1a}};

#define TOTAL_NAME_LEN (ETCPAL_UUID_DEV_STR_MAX_LEN + 6 + 4)

  /* Concatenate the input data into a buffer of the form "[dev_str][mac_addr][uuid_num]" */
  uint8_t name[TOTAL_NAME_LEN];
  strncpy((char*)name, dev_str, ETCPAL_UUID_DEV_STR_MAX_LEN);
  memcpy(&name[ETCPAL_UUID_DEV_STR_MAX_LEN], mac_addr, 6);
  etcpal_pack_u32l(&name[ETCPAL_UUID_DEV_STR_MAX_LEN + 6], uuid_num);

  return etcpal_generate_v5_uuid(&kNamespace, name, TOTAL_NAME_LEN, uuid);
}
