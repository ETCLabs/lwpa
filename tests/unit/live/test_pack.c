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

#include "etcpal/pack.h"
#include "etcpal/pack64.h"
#include "unity_fixture.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

// Disable sprintf() warning on Windows/MSVC
#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif

// What we pack and unpack from
uint8_t test_buf[100];

// Test utility functions that are used by the actual tests

static void test_packi16b(uint8_t* buffer, const char* error_msg)
{
  const int16_t  tst = 0x5678;
  const uint8_t* ptst = (const uint8_t*)(&tst);
  memset(buffer, 0, sizeof(int16_t));

  etcpal_pack_i16b(buffer, tst);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ptst[0], buffer[1], error_msg);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ptst[1], buffer[0], error_msg);
}

static void test_packi16l(uint8_t* buffer, const char* error_msg)
{
  const int16_t  tst = 0x5678;
  const uint8_t* ptst = (const uint8_t*)(&tst);
  memset(buffer, 0, sizeof(int16_t));

  etcpal_pack_i16l(buffer, tst);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ptst[0], buffer[0], error_msg);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ptst[1], buffer[1], error_msg);
}

static void test_unpacki16b(uint8_t* buffer, const char* error_msg)
{
  const int16_t tst = 0x5678;
  memset(buffer, 0, sizeof(int16_t));

  etcpal_pack_i16b(buffer, tst);
  int16_t var = etcpal_unpack_i16b(buffer);
  TEST_ASSERT_EQUAL_INT16_MESSAGE(tst, var, error_msg);
}

static void test_unpacki16l(uint8_t* buffer, const char* error_msg)
{
  const int16_t tst = 0x5678;
  memset(buffer, 0, sizeof(int16_t));

  etcpal_pack_i16l(buffer, tst);
  int16_t var = etcpal_unpack_i16l(buffer);
  TEST_ASSERT_EQUAL_INT16_MESSAGE(tst, var, error_msg);
}

static void test_packu16b(uint8_t* buffer, const char* error_msg)
{
  const uint16_t tst = 0x1234;
  const uint8_t* ptst = (const uint8_t*)(&tst);
  memset(buffer, 0, sizeof(uint16_t));

  etcpal_pack_u16b(buffer, tst);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ptst[0], buffer[1], error_msg);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ptst[1], buffer[0], error_msg);
}

static void test_packu16l(uint8_t* buffer, const char* error_msg)
{
  const uint16_t tst = 0x1234;
  const uint8_t* ptst = (const uint8_t*)(&tst);
  memset(buffer, 0, sizeof(uint16_t));

  etcpal_pack_u16l(buffer, tst);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ptst[0], buffer[0], error_msg);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ptst[1], buffer[1], error_msg);
}

static void test_unpacku16b(uint8_t* buffer, const char* error_msg)
{
  const uint16_t tst = 0x1234;
  memset(buffer, 0, sizeof(uint16_t));

  etcpal_pack_u16b(buffer, tst);
  uint16_t var = etcpal_unpack_u16b(buffer);
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(tst, var, error_msg);
}

static void test_unpacku16l(uint8_t* buffer, const char* error_msg)
{
  const uint16_t tst = 0x1234;
  memset(buffer, 0, sizeof(uint16_t));

  etcpal_pack_u16l(buffer, tst);
  uint16_t var = etcpal_unpack_u16l(buffer);
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(tst, var, error_msg);
}

static void test_packi32b(uint8_t* buffer, const char* error_msg)
{
  const int32_t  tst = 0x12345678;
  const uint8_t* ptst = (const uint8_t*)(&tst);
  memset(buffer, 0, sizeof(int32_t));

  etcpal_pack_i32b(buffer, tst);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ptst[0], buffer[3], error_msg);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ptst[1], buffer[2], error_msg);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ptst[2], buffer[1], error_msg);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ptst[3], buffer[0], error_msg);
}

static void test_packi32l(uint8_t* buffer, const char* error_msg)
{
  const int32_t  tst = 0x12345678;
  const uint8_t* ptst = (const uint8_t*)(&tst);
  memset(buffer, 0, sizeof(int32_t));

  etcpal_pack_i32l(buffer, tst);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ptst[0], buffer[0], error_msg);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ptst[1], buffer[1], error_msg);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ptst[2], buffer[2], error_msg);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ptst[3], buffer[3], error_msg);
}

static void test_unpacki32b(uint8_t* buffer, const char* error_msg)
{
  const int32_t tst = 0x12345678;
  memset(buffer, 0, sizeof(int32_t));

  etcpal_pack_i32b(buffer, tst);
  int32_t var = etcpal_unpack_i32b(buffer);
  TEST_ASSERT_EQUAL_INT32_MESSAGE(tst, var, error_msg);
}

static void test_unpacki32l(uint8_t* buffer, const char* error_msg)
{
  const int32_t tst = 0x12345678;
  memset(buffer, 0, sizeof(int32_t));

  etcpal_pack_i32l(buffer, tst);
  int32_t var = etcpal_unpack_i32l(buffer);
  TEST_ASSERT_EQUAL_INT32_MESSAGE(tst, var, error_msg);
}

static void test_packu32b(uint8_t* buffer, const char* error_msg)
{
  const uint32_t tst = 0x87654321;
  const uint8_t* ptst = (const uint8_t*)(&tst);
  memset(buffer, 0, sizeof(uint32_t));

  etcpal_pack_u32b(buffer, tst);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ptst[0], buffer[3], error_msg);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ptst[1], buffer[2], error_msg);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ptst[2], buffer[1], error_msg);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ptst[3], buffer[0], error_msg);
}

static void test_packu32l(uint8_t* buffer, const char* error_msg)
{
  const uint32_t tst = 0x87654321;
  const uint8_t* ptst = (const uint8_t*)(&tst);
  memset(buffer, 0, sizeof(uint32_t));

  etcpal_pack_u32l(buffer, tst);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ptst[0], buffer[0], error_msg);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ptst[1], buffer[1], error_msg);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ptst[2], buffer[2], error_msg);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ptst[3], buffer[3], error_msg);
}

static void test_unpacku32b(uint8_t* buffer, const char* error_msg)
{
  const uint32_t tst = 0x87654321;
  memset(buffer, 0, sizeof(uint32_t));

  etcpal_pack_u32b(buffer, tst);
  uint32_t var = etcpal_unpack_u32b(buffer);
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(tst, var, error_msg);
}

static void test_unpacku32l(uint8_t* buffer, const char* error_msg)
{
  const uint32_t tst = 0x87654321;
  memset(buffer, 0, sizeof(uint32_t));

  etcpal_pack_u32l(buffer, tst);
  uint32_t var = etcpal_unpack_u32l(buffer);
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(tst, var, error_msg);
}

#ifdef UNITY_SUPPORT_64

static void test_packi64b(uint8_t* buffer, const char* error_msg)
{
  const int64_t  tst = 0x0123456789abcdef;
  const uint8_t* ptst = (const uint8_t*)(&tst);
  memset(buffer, 0, sizeof(int64_t));

  etcpal_pack_i64b(buffer, tst);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ptst[0], buffer[7], error_msg);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ptst[1], buffer[6], error_msg);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ptst[2], buffer[5], error_msg);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ptst[3], buffer[4], error_msg);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ptst[4], buffer[3], error_msg);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ptst[5], buffer[2], error_msg);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ptst[6], buffer[1], error_msg);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ptst[7], buffer[0], error_msg);
}

static void test_packi64l(uint8_t* buffer, const char* error_msg)
{
  const int64_t  tst = 0x0123456789abcdef;
  const uint8_t* ptst = (const uint8_t*)(&tst);
  memset(buffer, 0, sizeof(int64_t));

  etcpal_pack_i64l(buffer, tst);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ptst[0], buffer[0], error_msg);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ptst[1], buffer[1], error_msg);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ptst[2], buffer[2], error_msg);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ptst[3], buffer[3], error_msg);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ptst[4], buffer[4], error_msg);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ptst[5], buffer[5], error_msg);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ptst[6], buffer[6], error_msg);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ptst[7], buffer[7], error_msg);
}

static void test_unpacki64b(uint8_t* buffer, const char* error_msg)
{
  const int64_t tst = 0x0123456789abcdef;
  memset(buffer, 0, sizeof(int64_t));

  etcpal_pack_i64b(buffer, tst);
  int64_t var = etcpal_unpack_i64b(buffer);
  TEST_ASSERT_EQUAL_INT64_MESSAGE(tst, var, error_msg);
}

static void test_unpacki64l(uint8_t* buffer, const char* error_msg)
{
  const int64_t tst = 0x0123456789abcdef;
  memset(buffer, 0, sizeof(int64_t));

  etcpal_pack_i64l(buffer, tst);
  int64_t var = etcpal_unpack_i64l(buffer);
  TEST_ASSERT_EQUAL_INT64_MESSAGE(tst, var, error_msg);
}

static void test_packu64b(uint8_t* buffer, const char* error_msg)
{
  const uint64_t tst = 0xfedcba9876543210;
  const uint8_t* ptst = (const uint8_t*)(&tst);
  memset(buffer, 0, sizeof(uint64_t));

  etcpal_pack_u64b(buffer, tst);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ptst[0], buffer[7], error_msg);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ptst[1], buffer[6], error_msg);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ptst[2], buffer[5], error_msg);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ptst[3], buffer[4], error_msg);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ptst[4], buffer[3], error_msg);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ptst[5], buffer[2], error_msg);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ptst[6], buffer[1], error_msg);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ptst[7], buffer[0], error_msg);
}

static void test_packu64l(uint8_t* buffer, const char* error_msg)
{
  const uint64_t tst = 0xfedcba9876543210;
  const uint8_t* ptst = (const uint8_t*)(&tst);
  memset(buffer, 0, sizeof(uint64_t));

  etcpal_pack_u64l(buffer, tst);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ptst[0], buffer[0], error_msg);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ptst[1], buffer[1], error_msg);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ptst[2], buffer[2], error_msg);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ptst[3], buffer[3], error_msg);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ptst[4], buffer[4], error_msg);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ptst[5], buffer[5], error_msg);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ptst[6], buffer[6], error_msg);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ptst[7], buffer[7], error_msg);
}

static void test_unpacku64b(uint8_t* buffer, const char* error_msg)
{
  const uint64_t tst = 0xfedcba9876543210;
  memset(buffer, 0, sizeof(uint64_t));

  etcpal_pack_u64b(buffer, tst);
  uint64_t var = etcpal_unpack_u64b(buffer);
  TEST_ASSERT_EQUAL_UINT64_MESSAGE(tst, var, error_msg);
}

static void test_unpacku64l(uint8_t* buffer, const char* error_msg)
{
  const uint64_t tst = 0xfedcba9876543210;
  memset(buffer, 0, sizeof(uint64_t));

  etcpal_pack_u64l(buffer, tst);
  uint64_t var = etcpal_unpack_u64l(buffer);
  TEST_ASSERT_EQUAL_UINT64_MESSAGE(tst, var, error_msg);
}

#endif  // UNITY_SUPPORT_64

// The tests themselves

TEST_GROUP(etcpal_pack);

TEST_SETUP(etcpal_pack)
{
}

TEST_TEAR_DOWN(etcpal_pack)
{
}

TEST(etcpal_pack, signed_pack_16_functions_work)
{
  // Test packing and unpacking values at varying memory offsets.
  for (size_t offset = 0; offset <= 60; ++offset)
  {
    char error_msg[50];
    sprintf(error_msg, "Failed on offset %zu", offset);

    test_packi16b(test_buf + offset, error_msg);
    test_packi16l(test_buf + offset, error_msg);
    test_unpacki16b(test_buf + offset, error_msg);
    test_unpacki16l(test_buf + offset, error_msg);
  }
}

TEST(etcpal_pack, unsigned_pack_16_functions_work)
{
  // Test packing and unpacking values at varying memory offsets.
  for (size_t offset = 0; offset <= 60; ++offset)
  {
    char error_msg[50];
    sprintf(error_msg, "Failed on offset %zu", offset);

    test_packu16b(test_buf + offset, error_msg);
    test_packu16l(test_buf + offset, error_msg);
    test_unpacku16b(test_buf + offset, error_msg);
    test_unpacku16l(test_buf + offset, error_msg);
  }
}

TEST(etcpal_pack, signed_pack_32_functions_work)
{
  // Test packing and unpacking values at varying memory offsets.
  for (size_t offset = 0; offset <= 60; ++offset)
  {
    char error_msg[50];
    sprintf(error_msg, "Failed on offset %zu", offset);

    test_packi32b(test_buf + offset, error_msg);
    test_packi32l(test_buf + offset, error_msg);
    test_unpacki32b(test_buf + offset, error_msg);
    test_unpacki32l(test_buf + offset, error_msg);
  }
}

TEST(etcpal_pack, unsigned_pack_32_functions_work)
{
  // Test packing and unpacking values at varying memory offsets.
  for (size_t offset = 0; offset <= 60; ++offset)
  {
    char error_msg[50];
    sprintf(error_msg, "Failed on offset %zu", offset);

    test_packu32b(test_buf + offset, error_msg);
    test_packu32l(test_buf + offset, error_msg);
    test_unpacku32b(test_buf + offset, error_msg);
    test_unpacku32l(test_buf + offset, error_msg);
  }
}

#ifdef UNITY_SUPPORT_64

TEST(etcpal_pack, signed_pack_64_functions_work)
{
  // Test packing and unpacking values at varying memory offsets.
  for (size_t offset = 0; offset <= 60; ++offset)
  {
    char error_msg[50];
    sprintf(error_msg, "Failed on offset %zu", offset);

    test_packi64b(test_buf + offset, error_msg);
    test_packi64l(test_buf + offset, error_msg);
    test_unpacki64b(test_buf + offset, error_msg);
    test_unpacki64l(test_buf + offset, error_msg);
  }
}

TEST(etcpal_pack, unsigned_pack_64_functions_work)
{
  // Test packing and unpacking values at varying memory offsets.
  for (size_t offset = 0; offset <= 60; ++offset)
  {
    char error_msg[50];
    sprintf(error_msg, "Failed on offset %zu", offset);

    test_packu64b(test_buf + offset, error_msg);
    test_packu64l(test_buf + offset, error_msg);
    test_unpacku64b(test_buf + offset, error_msg);
    test_unpacku64l(test_buf + offset, error_msg);
  }
}

#endif  // UNITY_SUPPORT_64

TEST_GROUP_RUNNER(etcpal_pack)
{
  RUN_TEST_CASE(etcpal_pack, signed_pack_16_functions_work);
  RUN_TEST_CASE(etcpal_pack, unsigned_pack_16_functions_work);
  RUN_TEST_CASE(etcpal_pack, signed_pack_32_functions_work);
  RUN_TEST_CASE(etcpal_pack, unsigned_pack_32_functions_work);
#ifdef UNITY_SUPPORT_64
  RUN_TEST_CASE(etcpal_pack, signed_pack_64_functions_work);
  RUN_TEST_CASE(etcpal_pack, unsigned_pack_64_functions_work);
#endif  // UNITY_SUPPORT_64
}
