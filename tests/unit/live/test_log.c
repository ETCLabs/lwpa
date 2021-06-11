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

#include "etcpal/log.h"

#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "etcpal/common.h"
#include "unity_fixture.h"
#include "etc_fff_wrapper.h"

// Disable strcpy() warning on Windows/MSVC
#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif

// Disable format security warnings in GCC/clang
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
#endif

ETC_FAKE_VOID_FUNC(log_callback, void*, const EtcPalLogStrings*);
ETC_FAKE_VOID_FUNC(time_callback, void*, EtcPalLogTimestamp*);

EtcPalLogTimestamp cur_time;
EtcPalLogStrings   last_log_strings_received;

// Buffers for tests of the create_*_str functions
static char syslog_buf[ETCPAL_SYSLOG_STR_MAX_LEN];
static char legacy_syslog_buf[ETCPAL_SYSLOG_STR_MAX_LEN];
static char human_buf[ETCPAL_LOG_STR_MAX_LEN];

static void fill_timestamp(void* context, EtcPalLogTimestamp* timestamp)
{
  ETCPAL_UNUSED_ARG(context);
  TEST_ASSERT_TRUE(timestamp);
  *timestamp = cur_time;
}

static void save_log_strings(void* context, const EtcPalLogStrings* strings)
{
  ETCPAL_UNUSED_ARG(context);
  TEST_ASSERT_TRUE(strings);
  last_log_strings_received = *strings;
}

static void fill_default_time(EtcPalLogTimestamp* timestamp)
{
  timestamp->year = 1970;     // absolute year
  timestamp->month = 1;       // month of the year - [1, 12]
  timestamp->day = 1;         // day of the month - [1, 31]
  timestamp->hour = 0;        // hours since midnight - [0, 23]
  timestamp->minute = 0;      // minutes after the hour - [0, 59]
  timestamp->second = 0;      // seconds after the minute - [0, 60] including leap second
  timestamp->msec = 0;        // milliseconds after the second - [0, 999]
  timestamp->utc_offset = 0;  // Local offset from UTC in minutes
}

TEST_GROUP(etcpal_log);

TEST_SETUP(etcpal_log)
{
  etcpal_init(ETCPAL_FEATURE_LOGGING);

  fill_default_time(&cur_time);

  RESET_FAKE(log_callback);
  RESET_FAKE(time_callback);
  log_callback_fake.custom_fake = save_log_strings;
  time_callback_fake.custom_fake = fill_timestamp;
}

TEST_TEAR_DOWN(etcpal_log)
{
  etcpal_deinit(ETCPAL_FEATURE_LOGGING);
}

// Test the etcpal_sanitize_syslog_params() function.
TEST(etcpal_log, sanitize_syslog_params_works)
{
  EtcPalSyslogParams  syslog_params = ETCPAL_SYSLOG_PARAMS_INIT;
  const unsigned char special_char_array[] = {
      0x01, 0x10, 0x7f,  // Some non-printing chars
      0x41, 0x42, 0x43,  // "ABC"
      0x80, 0x81, 0xff,  // Some characters out of the ASCII range
      0x00               // Null terminator
  };
  memcpy(syslog_params.app_name, special_char_array, sizeof special_char_array);
  memcpy(syslog_params.hostname, special_char_array, sizeof special_char_array);
  memcpy(syslog_params.procid, special_char_array, sizeof special_char_array);
  syslog_params.facility = -1;

  etcpal_sanitize_syslog_params(&syslog_params);
  TEST_ASSERT_EQUAL_STRING(syslog_params.app_name, "___ABC???");
  TEST_ASSERT_EQUAL_STRING(syslog_params.hostname, "___ABC???");
  TEST_ASSERT_EQUAL_STRING(syslog_params.procid, "___ABC???");
  TEST_ASSERT_LESS_THAN(ETCPAL_LOG_NFACILITIES, ETCPAL_LOG_FAC(syslog_params.facility));
}

// Test the etcpal_validate_log_params() function.
TEST(etcpal_log, validate_log_params_works)
{
  EtcPalLogParams     lparams = ETCPAL_LOG_PARAMS_INIT;
  const unsigned char special_char_array[] = {
      0x01, 0x10, 0x7f,  // Some non-printing chars
      0x41, 0x42, 0x43,  // "ABC"
      0x80, 0x81, 0xff,  // Some characters out of the ASCII range
      0x00               // Null terminator
  };

  // Test some normal params
  lparams.action = ETCPAL_LOG_CREATE_SYSLOG;
  lparams.log_fn = log_callback;
  memset(&lparams.syslog_params, 0, sizeof(EtcPalSyslogParams));
  memcpy(lparams.syslog_params.app_name, special_char_array, sizeof special_char_array);
  lparams.log_mask = 0;
  lparams.time_fn = time_callback;
  lparams.context = NULL;

  // Make sure the validation returned true and the syslog field got sanitized
  TEST_ASSERT_TRUE(etcpal_validate_log_params(&lparams));
  TEST_ASSERT_EQUAL_STRING(lparams.syslog_params.app_name, "___ABC???");

  // Invalid log callback
  lparams.log_fn = NULL;
  TEST_ASSERT_FALSE(etcpal_validate_log_params(&lparams));

  // It's valid for time_fn to be NULL
  lparams.log_fn = log_callback;
  lparams.time_fn = NULL;
  TEST_ASSERT_TRUE(etcpal_validate_log_params(&lparams));
}

// Make sure etcpal_validate_log_timestamp() returns correct values for valid and invalid time
TEST(etcpal_log, validate_log_timestamp_works)
{
  // Start with a valid time
  EtcPalLogTimestamp timestamp;
  fill_default_time(&timestamp);
  TEST_ASSERT_TRUE(etcpal_validate_log_timestamp(&timestamp));

  // Invalid years
  timestamp.year = 10000;
  TEST_ASSERT_FALSE(etcpal_validate_log_timestamp(&timestamp));
  timestamp.year = UINT_MAX;
  TEST_ASSERT_FALSE(etcpal_validate_log_timestamp(&timestamp));
  timestamp.year = 1970;

  // Invalid months
  timestamp.month = 0;
  TEST_ASSERT_FALSE(etcpal_validate_log_timestamp(&timestamp));
  timestamp.month = 13;
  TEST_ASSERT_FALSE(etcpal_validate_log_timestamp(&timestamp));
  timestamp.month = 1;

  // Invalid days
  timestamp.day = 0;
  TEST_ASSERT_FALSE(etcpal_validate_log_timestamp(&timestamp));
  timestamp.day = 32;
  TEST_ASSERT_FALSE(etcpal_validate_log_timestamp(&timestamp));
  timestamp.day = 1;

  // Invalid hours
  timestamp.hour = 24;
  TEST_ASSERT_FALSE(etcpal_validate_log_timestamp(&timestamp));
  timestamp.hour = 0;

  // Invalid minutes
  timestamp.minute = 60;
  TEST_ASSERT_FALSE(etcpal_validate_log_timestamp(&timestamp));
  timestamp.minute = 0;

  // Invalid seconds
  timestamp.second = 61;
  TEST_ASSERT_FALSE(etcpal_validate_log_timestamp(&timestamp));
  timestamp.second = 0;

  // Invalid milliseconds
  timestamp.msec = 1000;
  TEST_ASSERT_FALSE(etcpal_validate_log_timestamp(&timestamp));
}

// clang-format off
EtcPalLogParams log_action_test_params = {
  0,
  log_callback,
  {
    ETCPAL_LOG_KERN,
    "10.101.17.38",
    "My_App",
    "\0"
  },
  ETCPAL_LOG_UPTO(ETCPAL_LOG_DEBUG),
  time_callback,
  NULL
};
// clang-format on

#define LOG_ACTION_TEST_MESSAGE "Test Message"
#define LOG_ACTION_TEST_HUMAN_STR "1970-01-01 00:00:00.000Z [EMRG] " LOG_ACTION_TEST_MESSAGE
#define LOG_ACTION_TEST_SYSLOG_STR "<0>1 1970-01-01T00:00:00.000Z 10.101.17.38 My_App - - - " LOG_ACTION_TEST_MESSAGE
#define LOG_ACTION_TEST_LEGACY_SYSLOG_STR "<0>Jan  1 00:00:00 10.101.17.38 My_App: " LOG_ACTION_TEST_MESSAGE
#define LOG_ACTION_TEST_RAW_STR LOG_ACTION_TEST_MESSAGE

// Make sure the "action" member in the EtcPalLogParams struct works as expected.
TEST(etcpal_log, action_human_only_is_honored)
{
  log_action_test_params.action = ETCPAL_LOG_CREATE_HUMAN_READABLE;
  etcpal_log(&log_action_test_params, ETCPAL_LOG_EMERG, LOG_ACTION_TEST_MESSAGE);

  // Make sure the callback was called with the human-readable and raw strings, but not the syslog
  // strings.
  TEST_ASSERT_EQUAL_UINT(log_callback_fake.call_count, 1);
  TEST_ASSERT_FALSE(last_log_strings_received.syslog);
  TEST_ASSERT_FALSE(last_log_strings_received.legacy_syslog);
  TEST_ASSERT_TRUE(last_log_strings_received.human_readable);
  TEST_ASSERT_TRUE(last_log_strings_received.raw);
  TEST_ASSERT_EQUAL_STRING(last_log_strings_received.human_readable, LOG_ACTION_TEST_HUMAN_STR);
  TEST_ASSERT_EQUAL_STRING(last_log_strings_received.raw, LOG_ACTION_TEST_RAW_STR);
}

TEST(etcpal_log, action_syslog_only_is_honored)
{
  log_action_test_params.action = ETCPAL_LOG_CREATE_SYSLOG;
  etcpal_log(&log_action_test_params, ETCPAL_LOG_EMERG, "Test Message");

  // Make sure the callback was called with the syslog and raw strings only.
  TEST_ASSERT_EQUAL_UINT(log_callback_fake.call_count, 1);
  TEST_ASSERT_TRUE(last_log_strings_received.syslog);
  TEST_ASSERT_FALSE(last_log_strings_received.legacy_syslog);
  TEST_ASSERT_FALSE(last_log_strings_received.human_readable);
  TEST_ASSERT_TRUE(last_log_strings_received.raw);
  TEST_ASSERT_EQUAL_STRING(last_log_strings_received.syslog, LOG_ACTION_TEST_SYSLOG_STR);
  TEST_ASSERT_EQUAL_STRING(last_log_strings_received.raw, LOG_ACTION_TEST_RAW_STR);
}

TEST(etcpal_log, action_legacy_syslog_only_is_honored)
{
  log_action_test_params.action = ETCPAL_LOG_CREATE_LEGACY_SYSLOG;
  etcpal_log(&log_action_test_params, ETCPAL_LOG_EMERG, LOG_ACTION_TEST_MESSAGE);

  // Make sure the callback was called with the legacy syslog and raw strings only.
  TEST_ASSERT_EQUAL_UINT(log_callback_fake.call_count, 1);
  TEST_ASSERT_FALSE(last_log_strings_received.syslog);
  TEST_ASSERT_TRUE(last_log_strings_received.legacy_syslog);
  TEST_ASSERT_FALSE(last_log_strings_received.human_readable);
  TEST_ASSERT_TRUE(last_log_strings_received.raw);
  TEST_ASSERT_EQUAL_STRING(last_log_strings_received.legacy_syslog, LOG_ACTION_TEST_LEGACY_SYSLOG_STR);
  TEST_ASSERT_EQUAL_STRING(last_log_strings_received.raw, LOG_ACTION_TEST_RAW_STR);
}

TEST(etcpal_log, action_all_is_honored)
{
  log_action_test_params.action =
      ETCPAL_LOG_CREATE_SYSLOG | ETCPAL_LOG_CREATE_LEGACY_SYSLOG | ETCPAL_LOG_CREATE_HUMAN_READABLE;
  etcpal_log(&log_action_test_params, ETCPAL_LOG_EMERG, LOG_ACTION_TEST_MESSAGE);

  // Make sure the callback was called with all strings.
  TEST_ASSERT_EQUAL_UINT(log_callback_fake.call_count, 1);
  TEST_ASSERT_TRUE(last_log_strings_received.syslog);
  TEST_ASSERT_TRUE(last_log_strings_received.legacy_syslog);
  TEST_ASSERT_TRUE(last_log_strings_received.human_readable);
  TEST_ASSERT_TRUE(last_log_strings_received.raw);
  TEST_ASSERT_EQUAL_STRING(last_log_strings_received.syslog, LOG_ACTION_TEST_SYSLOG_STR);
  TEST_ASSERT_EQUAL_STRING(last_log_strings_received.legacy_syslog, LOG_ACTION_TEST_LEGACY_SYSLOG_STR);
  TEST_ASSERT_EQUAL_STRING(last_log_strings_received.human_readable, LOG_ACTION_TEST_HUMAN_STR);
  TEST_ASSERT_EQUAL_STRING(last_log_strings_received.raw, LOG_ACTION_TEST_RAW_STR);
}

TEST(etcpal_log, context_pointer_is_passed_unmodified)
{
  EtcPalLogParams lparams = ETCPAL_LOG_PARAMS_INIT;
  lparams.action = ETCPAL_LOG_CREATE_HUMAN_READABLE;
  lparams.log_fn = log_callback;
  lparams.log_mask = ETCPAL_LOG_UPTO(ETCPAL_LOG_DEBUG);

  // Test a null pointer
  lparams.context = NULL;
  etcpal_log(&lparams, ETCPAL_LOG_INFO, "Test message");
  TEST_ASSERT_EQUAL_UINT(log_callback_fake.call_count, 1);
  TEST_ASSERT_EQUAL_PTR(log_callback_fake.arg0_val, NULL);

  // Test a random value
  lparams.context = (void*)0x01020304;
  etcpal_log(&lparams, ETCPAL_LOG_INFO, "Test message");
  TEST_ASSERT_EQUAL_UINT(log_callback_fake.call_count, 2);
  TEST_ASSERT_EQUAL_PTR(log_callback_fake.arg0_val, 0x01020304);
}

TEST(etcpal_log, priority_is_passed_unmodified)
{
  EtcPalLogParams lparams = ETCPAL_LOG_PARAMS_INIT;
  lparams.action = ETCPAL_LOG_CREATE_HUMAN_READABLE;
  lparams.log_fn = log_callback;
  lparams.log_mask = ETCPAL_LOG_UPTO(ETCPAL_LOG_DEBUG);

  // Test each priority value
  for (int pri = 0; pri < ETCPAL_LOG_DEBUG + 1; ++pri)
  {
    etcpal_log(&lparams, pri, "Test message");
    TEST_ASSERT_EQUAL(last_log_strings_received.priority, pri);
  }
}

// A string with a non-ASCII character: "host\xC8name"
// Should be sanitized to "host?name"
// NOLINTNEXTLINE(cppcoreguidelines-narrowing-conversions)
const char kWeirdHostname[] = {0x68, 0x6f, 0x73, 0x74, 0xc8, 0x6e, 0x61, 0x6d, 0x65, 0x00};
// A string with some non-printing and non-ASCII characters: "\x012\x034\xff"
// NOLINTNEXTLINE(cppcoreguidelines-narrowing-conversions)
const char kWeirdProcId[] = {0x01, 0x32, 0x03, 0x34, 0xff, 0x00};
// A string with a non-printing character: "My\x001App"
const char kWeirdAppName[] = {0x4d, 0x79, 0x01, 0x41, 0x70, 0x70, 0x00};

#define SYSLOG_HEADER_TEST_MESSAGE "Test Message"

// For each set of parameters, test both the etcpal_log() and etcpal_create_syslog_str() functions.
void test_syslog_header(const EtcPalLogParams*    lparams,
                        const EtcPalLogTimestamp* timestamp,
                        const char*               expected_string)
{
  etcpal_log(lparams, ETCPAL_LOG_EMERG, SYSLOG_HEADER_TEST_MESSAGE);
  etcpal_create_syslog_str(syslog_buf, ETCPAL_SYSLOG_STR_MAX_LEN, timestamp, &lparams->syslog_params, ETCPAL_LOG_EMERG,
                           SYSLOG_HEADER_TEST_MESSAGE);
  TEST_ASSERT_EQUAL_UINT(log_callback_fake.call_count, 1);
  TEST_ASSERT_EQUAL_STRING(last_log_strings_received.syslog, expected_string);
  TEST_ASSERT_EQUAL_STRING(syslog_buf, expected_string);
  RESET_FAKE(log_callback)
}

// Test valid, weird, and missing values in the syslog header
TEST(etcpal_log, syslog_header_with_all_parts)
{
  EtcPalLogParams lparams = ETCPAL_LOG_PARAMS_INIT;
  lparams.action = ETCPAL_LOG_CREATE_SYSLOG;
  lparams.log_fn = log_callback;
  memcpy(lparams.syslog_params.hostname, kWeirdHostname, sizeof kWeirdHostname);
  memcpy(lparams.syslog_params.procid, kWeirdProcId, sizeof kWeirdProcId);
  memcpy(lparams.syslog_params.app_name, kWeirdAppName, sizeof kWeirdAppName);
  lparams.syslog_params.facility = ETCPAL_LOG_KERN;
  lparams.log_mask = ETCPAL_LOG_UPTO(ETCPAL_LOG_DEBUG);
  lparams.time_fn = time_callback;

  // Validate (and also sanitize) the log params
  TEST_ASSERT_TRUE(etcpal_validate_log_params(&lparams));
  test_syslog_header(&lparams, &cur_time,
                     "<0>1 1970-01-01T00:00:00.000Z host?name My_App _2_4? - - " SYSLOG_HEADER_TEST_MESSAGE);
}

TEST(etcpal_log, syslog_header_minus_time)
{
  EtcPalLogParams lparams = ETCPAL_LOG_PARAMS_INIT;
  lparams.action = ETCPAL_LOG_CREATE_SYSLOG;
  lparams.log_fn = log_callback;
  memcpy(lparams.syslog_params.hostname, kWeirdHostname, sizeof kWeirdHostname);
  memcpy(lparams.syslog_params.procid, kWeirdProcId, sizeof kWeirdProcId);
  memcpy(lparams.syslog_params.app_name, kWeirdAppName, sizeof kWeirdAppName);
  lparams.syslog_params.facility = ETCPAL_LOG_KERN;
  lparams.log_mask = ETCPAL_LOG_UPTO(ETCPAL_LOG_DEBUG);

  TEST_ASSERT_TRUE(etcpal_validate_log_params(&lparams));
  test_syslog_header(&lparams, NULL, "<0>1 - host?name My_App _2_4? - - " SYSLOG_HEADER_TEST_MESSAGE);
}

TEST(etcpal_log, syslog_header_minus_hostname)
{
  EtcPalLogParams lparams = ETCPAL_LOG_PARAMS_INIT;
  lparams.action = ETCPAL_LOG_CREATE_SYSLOG;
  lparams.log_fn = log_callback;
  memcpy(lparams.syslog_params.procid, kWeirdProcId, sizeof kWeirdProcId);
  memcpy(lparams.syslog_params.app_name, kWeirdAppName, sizeof kWeirdAppName);
  lparams.syslog_params.facility = ETCPAL_LOG_KERN;
  lparams.log_mask = ETCPAL_LOG_UPTO(ETCPAL_LOG_DEBUG);
  lparams.time_fn = time_callback;

  TEST_ASSERT_TRUE(etcpal_validate_log_params(&lparams));
  test_syslog_header(&lparams, &cur_time,
                     "<0>1 1970-01-01T00:00:00.000Z - My_App _2_4? - - " SYSLOG_HEADER_TEST_MESSAGE);
}

TEST(etcpal_log, syslog_header_minus_app_name)
{
  EtcPalLogParams lparams = ETCPAL_LOG_PARAMS_INIT;
  lparams.action = ETCPAL_LOG_CREATE_SYSLOG;
  lparams.log_fn = log_callback;
  memcpy(lparams.syslog_params.hostname, kWeirdHostname, sizeof kWeirdHostname);
  memcpy(lparams.syslog_params.procid, kWeirdProcId, sizeof kWeirdProcId);
  lparams.syslog_params.facility = ETCPAL_LOG_KERN;
  lparams.log_mask = ETCPAL_LOG_UPTO(ETCPAL_LOG_DEBUG);
  lparams.time_fn = time_callback;

  TEST_ASSERT_TRUE(etcpal_validate_log_params(&lparams));
  test_syslog_header(&lparams, &cur_time,
                     "<0>1 1970-01-01T00:00:00.000Z host?name - _2_4? - - " SYSLOG_HEADER_TEST_MESSAGE);
}

TEST(etcpal_log, syslog_header_minus_procid)
{
  EtcPalLogParams lparams = ETCPAL_LOG_PARAMS_INIT;
  lparams.action = ETCPAL_LOG_CREATE_SYSLOG;
  lparams.log_fn = log_callback;
  memcpy(lparams.syslog_params.hostname, kWeirdHostname, sizeof kWeirdHostname);
  memcpy(lparams.syslog_params.app_name, kWeirdAppName, sizeof kWeirdAppName);
  lparams.syslog_params.facility = ETCPAL_LOG_KERN;
  lparams.log_mask = ETCPAL_LOG_UPTO(ETCPAL_LOG_DEBUG);
  lparams.time_fn = time_callback;

  TEST_ASSERT_TRUE(etcpal_validate_log_params(&lparams));
  test_syslog_header(&lparams, &cur_time,
                     "<0>1 1970-01-01T00:00:00.000Z host?name My_App - - - " SYSLOG_HEADER_TEST_MESSAGE);
}

TEST(etcpal_log, syslog_header_with_no_values)
{
  EtcPalLogParams lparams = ETCPAL_LOG_PARAMS_INIT;
  lparams.action = ETCPAL_LOG_CREATE_SYSLOG;
  lparams.log_fn = log_callback;
  lparams.syslog_params.facility = ETCPAL_LOG_KERN;
  lparams.log_mask = ETCPAL_LOG_UPTO(ETCPAL_LOG_DEBUG);

  TEST_ASSERT_TRUE(etcpal_validate_log_params(&lparams));
  test_syslog_header(&lparams, NULL, "<0>1 - - - - - - " SYSLOG_HEADER_TEST_MESSAGE);
}

// For each set of parameters, test both the etcpal_log() and etcpal_create_legacy_syslog_str() functions.
void test_legacy_syslog_header(const EtcPalLogParams*    lparams,
                               const EtcPalLogTimestamp* timestamp,
                               const char*               expected_string)
{
  etcpal_log(lparams, ETCPAL_LOG_EMERG, SYSLOG_HEADER_TEST_MESSAGE);
  etcpal_create_legacy_syslog_str(syslog_buf, ETCPAL_SYSLOG_STR_MAX_LEN, timestamp, &lparams->syslog_params,
                                  ETCPAL_LOG_EMERG, SYSLOG_HEADER_TEST_MESSAGE);
  TEST_ASSERT_EQUAL_UINT(log_callback_fake.call_count, 1);
  TEST_ASSERT_EQUAL_STRING(last_log_strings_received.legacy_syslog, expected_string);
  TEST_ASSERT_EQUAL_STRING(syslog_buf, expected_string);
  RESET_FAKE(log_callback)
}

// Test valid, weird, and missing values in the legacy syslog header
TEST(etcpal_log, legacy_syslog_header_with_all_parts)
{
  EtcPalLogParams lparams = ETCPAL_LOG_PARAMS_INIT;
  lparams.action = ETCPAL_LOG_CREATE_LEGACY_SYSLOG;
  lparams.log_fn = log_callback;
  memcpy(lparams.syslog_params.hostname, kWeirdHostname, sizeof kWeirdHostname);
  memcpy(lparams.syslog_params.procid, kWeirdProcId, sizeof kWeirdProcId);
  memcpy(lparams.syslog_params.app_name, kWeirdAppName, sizeof kWeirdAppName);
  lparams.syslog_params.facility = ETCPAL_LOG_KERN;
  lparams.log_mask = ETCPAL_LOG_UPTO(ETCPAL_LOG_DEBUG);
  lparams.time_fn = time_callback;

  // Validate (and also sanitize) the log params
  TEST_ASSERT_TRUE(etcpal_validate_log_params(&lparams));
  test_legacy_syslog_header(&lparams, &cur_time,
                            "<0>Jan  1 00:00:00 host?name My_App[_2_4?]: " SYSLOG_HEADER_TEST_MESSAGE);
}

// Test valid, weird, and missing values in the legacy syslog header
TEST(etcpal_log, legacy_syslog_header_minus_timestamp)
{
  EtcPalLogParams lparams = ETCPAL_LOG_PARAMS_INIT;
  lparams.action = ETCPAL_LOG_CREATE_LEGACY_SYSLOG;
  lparams.log_fn = log_callback;
  memcpy(lparams.syslog_params.hostname, kWeirdHostname, sizeof kWeirdHostname);
  memcpy(lparams.syslog_params.procid, kWeirdProcId, sizeof kWeirdProcId);
  memcpy(lparams.syslog_params.app_name, kWeirdAppName, sizeof kWeirdAppName);
  lparams.syslog_params.facility = ETCPAL_LOG_KERN;
  lparams.log_mask = ETCPAL_LOG_UPTO(ETCPAL_LOG_DEBUG);

  // Validate (and also sanitize) the log params
  TEST_ASSERT_TRUE(etcpal_validate_log_params(&lparams));
  test_legacy_syslog_header(&lparams, NULL, "<0>My_App[_2_4?]: " SYSLOG_HEADER_TEST_MESSAGE);
}

TEST(etcpal_log, legacy_syslog_header_minus_hostname)
{
  EtcPalLogParams lparams = ETCPAL_LOG_PARAMS_INIT;
  lparams.action = ETCPAL_LOG_CREATE_LEGACY_SYSLOG;
  lparams.log_fn = log_callback;
  memcpy(lparams.syslog_params.procid, kWeirdProcId, sizeof kWeirdProcId);
  memcpy(lparams.syslog_params.app_name, kWeirdAppName, sizeof kWeirdAppName);
  lparams.syslog_params.facility = ETCPAL_LOG_KERN;
  lparams.log_mask = ETCPAL_LOG_UPTO(ETCPAL_LOG_DEBUG);
  lparams.time_fn = time_callback;

  // Validate (and also sanitize) the log params
  TEST_ASSERT_TRUE(etcpal_validate_log_params(&lparams));
  test_legacy_syslog_header(&lparams, &cur_time, "<0>Jan  1 00:00:00 - My_App[_2_4?]: " SYSLOG_HEADER_TEST_MESSAGE);
}

TEST(etcpal_log, legacy_syslog_header_minus_app_name)
{
  EtcPalLogParams lparams = ETCPAL_LOG_PARAMS_INIT;
  lparams.action = ETCPAL_LOG_CREATE_LEGACY_SYSLOG;
  lparams.log_fn = log_callback;
  memcpy(lparams.syslog_params.hostname, kWeirdHostname, sizeof kWeirdHostname);
  memcpy(lparams.syslog_params.procid, kWeirdProcId, sizeof kWeirdProcId);
  lparams.syslog_params.facility = ETCPAL_LOG_KERN;
  lparams.log_mask = ETCPAL_LOG_UPTO(ETCPAL_LOG_DEBUG);
  lparams.time_fn = time_callback;

  // Validate (and also sanitize) the log params
  TEST_ASSERT_TRUE(etcpal_validate_log_params(&lparams));
  test_legacy_syslog_header(&lparams, &cur_time, "<0>Jan  1 00:00:00 host?name _2_4?: " SYSLOG_HEADER_TEST_MESSAGE);
}

TEST(etcpal_log, legacy_syslog_header_minus_procid)
{
  EtcPalLogParams lparams = ETCPAL_LOG_PARAMS_INIT;
  lparams.action = ETCPAL_LOG_CREATE_LEGACY_SYSLOG;
  lparams.log_fn = log_callback;
  memcpy(lparams.syslog_params.hostname, kWeirdHostname, sizeof kWeirdHostname);
  memcpy(lparams.syslog_params.app_name, kWeirdAppName, sizeof kWeirdAppName);
  lparams.syslog_params.facility = ETCPAL_LOG_KERN;
  lparams.log_mask = ETCPAL_LOG_UPTO(ETCPAL_LOG_DEBUG);
  lparams.time_fn = time_callback;

  // Validate (and also sanitize) the log params
  TEST_ASSERT_TRUE(etcpal_validate_log_params(&lparams));
  test_legacy_syslog_header(&lparams, &cur_time, "<0>Jan  1 00:00:00 host?name My_App: " SYSLOG_HEADER_TEST_MESSAGE);
}

TEST(etcpal_log, syslog_prival_is_correct)
{
  EtcPalSyslogParams syslog_params = ETCPAL_SYSLOG_PARAMS_INIT;

#define SYSLOG_PRIVAL_TEST_MESSAGE "Test Message"

  for (int facility = 0; facility < ETCPAL_LOG_NFACILITIES; ++facility)
  {
    syslog_params.facility = facility << 3;
    for (int priority = 0; priority < 8; ++priority)
    {
      const char error_format[] = "Testing facility %d, priority %d";
      char       error_msg[sizeof error_format + 20];
      sprintf(error_msg, error_format, facility, priority);

      TEST_ASSERT_TRUE(etcpal_create_syslog_str(syslog_buf, ETCPAL_SYSLOG_STR_MAX_LEN, NULL, &syslog_params, priority,
                                                SYSLOG_PRIVAL_TEST_MESSAGE));
      TEST_ASSERT_EQUAL(syslog_buf[0], '<');
      TEST_ASSERT_EQUAL_MESSAGE(atoi(&syslog_buf[1]), ((facility << 3) + priority), error_msg);

      TEST_ASSERT_TRUE(etcpal_create_legacy_syslog_str(syslog_buf, ETCPAL_SYSLOG_STR_MAX_LEN, NULL, &syslog_params,
                                                       priority, SYSLOG_PRIVAL_TEST_MESSAGE));
      TEST_ASSERT_EQUAL(syslog_buf[0], '<');
      TEST_ASSERT_EQUAL_MESSAGE(atoi(&syslog_buf[1]), ((facility << 3) + priority), error_msg);
    }
  }
}

// Make sure the log mask member in the EtcPalLogParams struct is honored properly.
TEST(etcpal_log, log_mask_is_honored)
{
  EtcPalLogParams lparams = ETCPAL_LOG_PARAMS_INIT;
  lparams.action = ETCPAL_LOG_CREATE_SYSLOG;
  lparams.log_fn = log_callback;
  lparams.log_mask = 0;

#define LOG_MASK_TEST_MESSAGE "Test Message"

  // Try logging with the log mask set to 0, should not work.
  for (int pri = 0; pri < 8; ++pri)
  {
    TEST_ASSERT_FALSE(etcpal_can_log(&lparams, pri));
    etcpal_log(&lparams, pri, LOG_MASK_TEST_MESSAGE);
  }
  TEST_ASSERT_EQUAL_UINT(log_callback_fake.call_count, 0);

  // Use the ETCPAL_LOG_UPTO macro to test logging below and above a given priority
  for (int mask_pri = 0; mask_pri < 8; ++mask_pri)
  {
    lparams.log_mask = ETCPAL_LOG_UPTO(mask_pri);
    for (int test_pri = 0; test_pri < 8; ++test_pri)
    {
      if (test_pri <= mask_pri)
      {
        TEST_ASSERT_TRUE(etcpal_can_log(&lparams, test_pri));
      }
      else
      {
        TEST_ASSERT_FALSE(etcpal_can_log(&lparams, test_pri));
      }
      etcpal_log(&lparams, test_pri, LOG_MASK_TEST_MESSAGE);
    }
    TEST_ASSERT_EQUAL_UINT(log_callback_fake.call_count, mask_pri + 1);
    RESET_FAKE(log_callback);
  }

  // Test some other, random log masks
  lparams.log_mask = ETCPAL_LOG_MASK(ETCPAL_LOG_ALERT) | ETCPAL_LOG_MASK(ETCPAL_LOG_WARNING);
  for (int test_pri = 0; test_pri < 8; ++test_pri)
  {
    if (test_pri == ETCPAL_LOG_ALERT || test_pri == ETCPAL_LOG_WARNING)
    {
      TEST_ASSERT_TRUE(etcpal_can_log(&lparams, test_pri));
    }
    else
    {
      TEST_ASSERT_FALSE(etcpal_can_log(&lparams, test_pri));
    }
    etcpal_log(&lparams, test_pri, LOG_MASK_TEST_MESSAGE);
  }
  // The callback should only have been called for our two masked priorities.
  TEST_ASSERT_EQUAL_UINT(log_callback_fake.call_count, 2);
}

// Make sure the time header is properly present (or absent) as necessary
TEST(etcpal_log, human_time_header_is_well_formed)
{
  TEST_ASSERT_TRUE(
      etcpal_create_log_str(human_buf, ETCPAL_LOG_STR_MAX_LEN, &cur_time, ETCPAL_LOG_CRIT, "Test Message"));
  TEST_ASSERT_TRUE(strstr(human_buf, "1970-01-01 00:00:00.000Z"));

  TEST_ASSERT_TRUE(etcpal_create_log_str(human_buf, ETCPAL_LOG_STR_MAX_LEN, NULL, ETCPAL_LOG_CRIT, "Test Message"));
  TEST_ASSERT_EQUAL_STRING(human_buf, "[CRIT] Test Message");

  // Test the addition of UTC offsets
  cur_time.utc_offset = 30;
  TEST_ASSERT_TRUE(
      etcpal_create_log_str(human_buf, ETCPAL_LOG_STR_MAX_LEN, &cur_time, ETCPAL_LOG_CRIT, "Test Message"));
  TEST_ASSERT_TRUE(strstr(human_buf, "1970-01-01 00:00:00.000+00:30"));

  cur_time.utc_offset = -120;
  TEST_ASSERT_TRUE(
      etcpal_create_log_str(human_buf, ETCPAL_LOG_STR_MAX_LEN, &cur_time, ETCPAL_LOG_CRIT, "Test Message"));
  TEST_ASSERT_TRUE(strstr(human_buf, "1970-01-01 00:00:00.000-02:00"));
}

TEST(etcpal_log, syslog_time_header_is_well_formed)
{
  EtcPalSyslogParams syslog_params = ETCPAL_SYSLOG_PARAMS_INIT;

  TEST_ASSERT_TRUE(etcpal_create_syslog_str(syslog_buf, ETCPAL_SYSLOG_STR_MAX_LEN, &cur_time, &syslog_params,
                                            ETCPAL_LOG_EMERG, "Test Message"));
  TEST_ASSERT_TRUE(strstr(syslog_buf, "1970-01-01T00:00:00.000Z"));

  // Test the addition of UTC offsets
  cur_time.utc_offset = 30;
  TEST_ASSERT_TRUE(etcpal_create_syslog_str(syslog_buf, ETCPAL_SYSLOG_STR_MAX_LEN, &cur_time, &syslog_params,
                                            ETCPAL_LOG_EMERG, "Test Message"));
  TEST_ASSERT_TRUE(strstr(syslog_buf, "1970-01-01T00:00:00.000+00:30"));

  cur_time.utc_offset = -120;
  TEST_ASSERT_TRUE(etcpal_create_syslog_str(syslog_buf, ETCPAL_SYSLOG_STR_MAX_LEN, &cur_time, &syslog_params,
                                            ETCPAL_LOG_EMERG, "Test Message"));
  TEST_ASSERT_TRUE(strstr(syslog_buf, "1970-01-01T00:00:00.000-02:00"));
}

TEST(etcpal_log, legacy_syslog_time_header_is_well_formed)
{
  EtcPalSyslogParams syslog_params = ETCPAL_SYSLOG_PARAMS_INIT;

  TEST_ASSERT_TRUE(etcpal_create_legacy_syslog_str(syslog_buf, ETCPAL_SYSLOG_STR_MAX_LEN, &cur_time, &syslog_params,
                                                   ETCPAL_LOG_EMERG, "Test Message"));
  TEST_ASSERT_TRUE(strstr(syslog_buf, "Jan  1 00:00:00"));

  // Make sure two digits are handled correctly
  cur_time.month = 8;
  cur_time.day = 15;
  cur_time.hour = 8;
  TEST_ASSERT_TRUE(etcpal_create_legacy_syslog_str(syslog_buf, ETCPAL_SYSLOG_STR_MAX_LEN, &cur_time, &syslog_params,
                                                   ETCPAL_LOG_EMERG, "Test Message"));
  TEST_ASSERT_TRUE(strstr(syslog_buf, "Aug 15 08:00:00"));
}

// clang-format off
static const EtcPalLogParams kFormatTestLogParams = {
  (ETCPAL_LOG_CREATE_HUMAN_READABLE | ETCPAL_LOG_CREATE_SYSLOG | ETCPAL_LOG_CREATE_LEGACY_SYSLOG),
  log_callback,
  ETCPAL_SYSLOG_PARAMS_INIT,
  ETCPAL_LOG_UPTO(ETCPAL_LOG_DEBUG),
  NULL,
  NULL
};
// clang-format on

#define DO_FORMAT_TEST(expected_str, ...)                                                                         \
  TEST_ASSERT_TRUE(etcpal_create_syslog_str(syslog_buf, ETCPAL_SYSLOG_STR_MAX_LEN, &cur_time,                     \
                                            &kFormatTestLogParams.syslog_params, ETCPAL_LOG_EMERG, __VA_ARGS__)); \
  TEST_ASSERT_TRUE(strstr(syslog_buf, expected_str));                                                             \
  TEST_ASSERT_TRUE(etcpal_create_legacy_syslog_str(legacy_syslog_buf, ETCPAL_SYSLOG_STR_MAX_LEN, &cur_time,       \
                                                   &kFormatTestLogParams.syslog_params, ETCPAL_LOG_EMERG,         \
                                                   __VA_ARGS__));                                                 \
  TEST_ASSERT_TRUE(strstr(legacy_syslog_buf, expected_str));                                                      \
  TEST_ASSERT_TRUE(                                                                                               \
      etcpal_create_log_str(human_buf, ETCPAL_LOG_STR_MAX_LEN, &cur_time, ETCPAL_LOG_EMERG, __VA_ARGS__));        \
  TEST_ASSERT_TRUE(strstr(human_buf, expected_str));                                                              \
  etcpal_log(&kFormatTestLogParams, ETCPAL_LOG_EMERG, __VA_ARGS__);                                               \
  TEST_ASSERT_EQUAL_UINT(log_callback_fake.call_count, 1);                                                        \
  TEST_ASSERT_TRUE(last_log_strings_received.syslog);                                                             \
  TEST_ASSERT_TRUE(last_log_strings_received.legacy_syslog);                                                      \
  TEST_ASSERT_TRUE(last_log_strings_received.human_readable);                                                     \
  TEST_ASSERT_TRUE(last_log_strings_received.raw);                                                                \
  TEST_ASSERT_TRUE(strstr(last_log_strings_received.syslog, expected_str));                                       \
  TEST_ASSERT_TRUE(strstr(last_log_strings_received.legacy_syslog, expected_str));                                \
  TEST_ASSERT_TRUE(strstr(last_log_strings_received.human_readable, expected_str));                               \
  TEST_ASSERT_EQUAL_STRING(last_log_strings_received.raw, expected_str)

// Test logging of int values in the format string.
TEST(etcpal_log, formatting_int_values_works)
{
  DO_FORMAT_TEST("Here are some int values: 1 42 4294967295", "Here are some int values: %d %d %u", 1, 42, 0xffffffffu);
}

// Test logging of string values in the format string.
TEST(etcpal_log, formatting_string_values_works)
{
  DO_FORMAT_TEST("Here are some string values: hey wassup hello", "Here are some string values: %s %s %s", "hey",
                 "wassup", "hello");
}

// Helper to get the proper sanitized character from a loop counter for the
// max length string test
static char get_sanitized_char(size_t i)
{
  unsigned char ret = (i % 128 == 0 ? 1 : i % 128);

  // We also need to avoid '%' because it is the printf format specifier.
  if (ret < 33 || ret == 127 || ret == '%')
    ret = '_';
  else if (ret > 127)
    ret = '?';
  return (char)ret;
}

// Test logging a maximum length string.
TEST(etcpal_log, maximum_length_human_string_works)
{
  EtcPalLogParams lparams = ETCPAL_LOG_PARAMS_INIT;
  lparams.action = ETCPAL_LOG_CREATE_HUMAN_READABLE;
  lparams.log_fn = log_callback;
  lparams.syslog_params.facility = ETCPAL_LOG_LOCAL7;
  lparams.log_mask = ETCPAL_LOG_UPTO(ETCPAL_LOG_DEBUG);
  lparams.time_fn = time_callback;

  TEST_ASSERT_TRUE(etcpal_validate_log_params(&lparams));

  static char expect_human_str[ETCPAL_LOG_STR_MAX_LEN];
  static char expect_raw_str[ETCPAL_LOG_STR_MAX_LEN];
  strcpy(expect_human_str, "1970-01-01 00:00:00.000-12:00 [DBUG] ");
  size_t str_pos = strlen(expect_human_str);

  // Now build our actual log message
  static char to_log_str[ETCPAL_RAW_LOG_MSG_MAX_LEN];
  size_t      i = 0;
  for (; i < ETCPAL_RAW_LOG_MSG_MAX_LEN - 1; ++i)
  {
    char to_add = get_sanitized_char(i);
    expect_human_str[str_pos++] = to_add;
    expect_raw_str[i] = to_add;
    to_log_str[i] = to_add;
  }
  expect_human_str[str_pos++] = '\0';
  expect_raw_str[i] = '\0';
  to_log_str[i] = '\0';

  TEST_ASSERT_LESS_OR_EQUAL(ETCPAL_LOG_STR_MAX_LEN, str_pos);

  // We want to have a non-zero, two-digit UTC offset for the maximum length possible.
  cur_time.utc_offset = -720;

  TEST_ASSERT_TRUE(etcpal_create_log_str(human_buf, ETCPAL_LOG_STR_MAX_LEN, &cur_time, ETCPAL_LOG_DEBUG, to_log_str));
  TEST_ASSERT_EQUAL_STRING(human_buf, expect_human_str);

  // Now test the etcpal_log function
  etcpal_log(&lparams, ETCPAL_LOG_DEBUG, to_log_str);
  // Make sure the callback was called with all three strings, with the correct format.
  TEST_ASSERT_EQUAL_UINT(log_callback_fake.call_count, 1);
  TEST_ASSERT_TRUE(last_log_strings_received.human_readable);
  TEST_ASSERT_TRUE(last_log_strings_received.raw);
  TEST_ASSERT_EQUAL_STRING(last_log_strings_received.human_readable, expect_human_str);
  TEST_ASSERT_EQUAL_STRING(last_log_strings_received.raw, expect_raw_str);
}

TEST(etcpal_log, maximum_length_syslog_string_works)
{
  EtcPalLogParams lparams = ETCPAL_LOG_PARAMS_INIT;
  lparams.action = ETCPAL_LOG_CREATE_SYSLOG;
  lparams.log_fn = log_callback;
  lparams.syslog_params.facility = ETCPAL_LOG_LOCAL7;
  lparams.log_mask = ETCPAL_LOG_UPTO(ETCPAL_LOG_DEBUG);
  lparams.time_fn = time_callback;

  static char expect_syslog_str[ETCPAL_SYSLOG_STR_MAX_LEN];
  static char expect_raw_str[ETCPAL_LOG_STR_MAX_LEN];
  strcpy(expect_syslog_str, "<191>1 1970-01-01T00:00:00.000-12:00 ");

  size_t str_pos = strlen(expect_syslog_str);

  // Create our very long syslog header components
  size_t i = 0;
  for (; i < ETCPAL_LOG_HOSTNAME_MAX_LEN - 1; ++i)
  {
    char to_add = get_sanitized_char(i);
    lparams.syslog_params.hostname[i] = to_add;
    expect_syslog_str[str_pos++] = to_add;
  }
  lparams.syslog_params.hostname[i] = '\0';
  expect_syslog_str[str_pos++] = ' ';

  TEST_ASSERT_LESS_THAN(ETCPAL_SYSLOG_STR_MAX_LEN, str_pos);

  for (i = 0; i < ETCPAL_LOG_APP_NAME_MAX_LEN - 1; ++i)
  {
    char to_add = get_sanitized_char(i);
    lparams.syslog_params.app_name[i] = to_add;
    expect_syslog_str[str_pos++] = to_add;
  }
  lparams.syslog_params.app_name[i] = '\0';
  expect_syslog_str[str_pos++] = ' ';

  TEST_ASSERT_LESS_THAN(ETCPAL_SYSLOG_STR_MAX_LEN, str_pos);

  for (i = 0; i < ETCPAL_LOG_PROCID_MAX_LEN - 1; ++i)
  {
    char to_add = get_sanitized_char(i);
    lparams.syslog_params.procid[i] = to_add;
    expect_syslog_str[str_pos++] = to_add;
  }
  lparams.syslog_params.procid[i] = '\0';
  strcat(expect_syslog_str, " - - ");
  str_pos += 5;

  TEST_ASSERT_LESS_THAN(ETCPAL_SYSLOG_STR_MAX_LEN, str_pos);

  // Now build our actual log message
  char to_log_str[ETCPAL_RAW_LOG_MSG_MAX_LEN];
  for (i = 0; i < ETCPAL_RAW_LOG_MSG_MAX_LEN - 1; ++i)
  {
    char to_add = get_sanitized_char(i);
    expect_syslog_str[str_pos++] = to_add;
    expect_raw_str[i] = to_add;
    to_log_str[i] = to_add;
  }
  expect_syslog_str[str_pos++] = '\0';
  expect_raw_str[i] = '\0';
  to_log_str[i] = '\0';

  TEST_ASSERT_LESS_OR_EQUAL(ETCPAL_SYSLOG_STR_MAX_LEN, str_pos);

  TEST_ASSERT_TRUE(etcpal_validate_log_params(&lparams));
  // We want to have a non-zero, two-digit UTC offset for the maximum length possible.
  cur_time.utc_offset = -720;

  TEST_ASSERT_TRUE(etcpal_create_syslog_str(syslog_buf, ETCPAL_SYSLOG_STR_MAX_LEN, &cur_time, &lparams.syslog_params,
                                            ETCPAL_LOG_DEBUG, to_log_str));
  TEST_ASSERT_EQUAL_STRING(syslog_buf, expect_syslog_str);

  // Now test the etcpal_log function
  etcpal_log(&lparams, ETCPAL_LOG_DEBUG, to_log_str);
  // Make sure the callback was called with all three strings, with the correct format.
  TEST_ASSERT_EQUAL_UINT(log_callback_fake.call_count, 1);
  TEST_ASSERT_TRUE(last_log_strings_received.syslog);
  TEST_ASSERT_TRUE(last_log_strings_received.raw);
  TEST_ASSERT_EQUAL_STRING(last_log_strings_received.syslog, expect_syslog_str);
  TEST_ASSERT_EQUAL_STRING(last_log_strings_received.raw, expect_raw_str);
}

TEST(etcpal_log, maximum_length_legacy_syslog_string_works)
{
  EtcPalLogParams lparams = ETCPAL_LOG_PARAMS_INIT;
  lparams.action = ETCPAL_LOG_CREATE_LEGACY_SYSLOG;
  lparams.log_fn = log_callback;
  lparams.syslog_params.facility = ETCPAL_LOG_LOCAL7;
  lparams.log_mask = ETCPAL_LOG_UPTO(ETCPAL_LOG_DEBUG);
  lparams.time_fn = time_callback;

  static char expect_syslog_str[ETCPAL_SYSLOG_STR_MAX_LEN];
  static char expect_raw_str[ETCPAL_LOG_STR_MAX_LEN];
  strcpy(expect_syslog_str, "<191>Jan  1 00:00:00 ");
  size_t str_pos = strlen(expect_syslog_str);

  // Create our very long syslog header components
  size_t i = 0;
  for (; i < ETCPAL_LOG_HOSTNAME_MAX_LEN - 1; ++i)
  {
    char to_add = get_sanitized_char(i);
    lparams.syslog_params.hostname[i] = to_add;
    expect_syslog_str[str_pos++] = to_add;
  }
  lparams.syslog_params.hostname[i] = '\0';
  expect_syslog_str[str_pos++] = ' ';

  TEST_ASSERT_LESS_THAN(ETCPAL_SYSLOG_STR_MAX_LEN, str_pos);

  for (i = 0; i < ETCPAL_LOG_APP_NAME_MAX_LEN - 1; ++i)
  {
    char to_add = get_sanitized_char(i);
    lparams.syslog_params.app_name[i] = to_add;
    if (i < 32)
      expect_syslog_str[str_pos++] = to_add;
  }
  lparams.syslog_params.app_name[i] = '\0';

  TEST_ASSERT_LESS_THAN(ETCPAL_SYSLOG_STR_MAX_LEN, str_pos);

  for (i = 0; i < ETCPAL_LOG_PROCID_MAX_LEN - 1; ++i)
  {
    char to_add = get_sanitized_char(i);
    lparams.syslog_params.procid[i] = to_add;
    // APP_NAME/PROCID pair will be truncated to 32 characters
    // expect_syslog_str[str_pos++] = to_add;
  }
  lparams.syslog_params.procid[i] = '\0';
  strcat(expect_syslog_str, ": ");
  str_pos += 2;

  TEST_ASSERT_LESS_THAN(ETCPAL_SYSLOG_STR_MAX_LEN, str_pos);

  // Now build our actual log message
  char to_log_str[ETCPAL_RAW_LOG_MSG_MAX_LEN];
  for (i = 0; i < ETCPAL_RAW_LOG_MSG_MAX_LEN - 1; ++i)
  {
    char to_add = get_sanitized_char(i);
    expect_syslog_str[str_pos++] = to_add;
    expect_raw_str[i] = to_add;
    to_log_str[i] = to_add;
  }
  expect_syslog_str[str_pos++] = '\0';
  expect_raw_str[i] = '\0';
  to_log_str[i] = '\0';

  TEST_ASSERT_LESS_OR_EQUAL(ETCPAL_SYSLOG_STR_MAX_LEN, str_pos);

  TEST_ASSERT_TRUE(etcpal_validate_log_params(&lparams));
  // We want to have a non-zero, two-digit UTC offset for the maximum length possible.
  cur_time.utc_offset = -720;

  TEST_ASSERT_TRUE(etcpal_create_legacy_syslog_str(syslog_buf, ETCPAL_SYSLOG_STR_MAX_LEN, &cur_time,
                                                   &lparams.syslog_params, ETCPAL_LOG_DEBUG, to_log_str));
  TEST_ASSERT_EQUAL_STRING(syslog_buf, expect_syslog_str);

  // Now test the etcpal_log function
  etcpal_log(&lparams, ETCPAL_LOG_DEBUG, to_log_str);
  // Make sure the callback was called with all three strings, with the correct format.
  TEST_ASSERT_EQUAL_UINT(log_callback_fake.call_count, 1);
  TEST_ASSERT_TRUE(last_log_strings_received.legacy_syslog);
  TEST_ASSERT_TRUE(last_log_strings_received.raw);
  TEST_ASSERT_EQUAL_STRING(last_log_strings_received.legacy_syslog, expect_syslog_str);
  TEST_ASSERT_EQUAL_STRING(last_log_strings_received.raw, expect_raw_str);
}

#define VCREATE_TEST_PRI ETCPAL_LOG_WARNING
#define VCREATE_TEST_FORMAT_STR_AND_ARGS "Here are some random values: %s %d %x", "hey", 25, 0x3d5
#define VCREATE_TEST_EXPECTED_RESULT "1970-01-01 00:00:00.000Z [WARN] Here are some random values: hey 25 3d5"

static bool vcreate_test_helper(const char* format, ...)
{
  va_list args;
  va_start(args, format);
  bool res = etcpal_vcreate_log_str(human_buf, ETCPAL_LOG_STR_MAX_LEN, &cur_time, VCREATE_TEST_PRI, format, args);
  va_end(args);
  return res;
}

TEST(etcpal_log, vcreate_log_str_works)
{
  TEST_ASSERT_TRUE(vcreate_test_helper(VCREATE_TEST_FORMAT_STR_AND_ARGS));
  TEST_ASSERT_EQUAL_STRING(human_buf, VCREATE_TEST_EXPECTED_RESULT);
}

#define VCREATE_SYSLOG_TEST_PRI ETCPAL_LOG_EMERG
#define VCREATE_SYSLOG_TEST_FORMAT_STR_AND_ARGS "Here are some random values: %s %d %x", "hey", 25, 0x3d5

static const EtcPalSyslogParams kVcreateTestSyslogParams = {ETCPAL_LOG_DAEMON, "test_host", "test_app", "test_proc"};

#define VCREATE_SYSLOG_TEST_EXPECTED_RESULT \
  "<24>1 1970-01-01T00:00:00.000Z test_host test_app test_proc - - Here are some random values: hey 25 3d5"

static bool vcreate_syslog_test_helper(const char* format, ...)
{
  va_list args;
  va_start(args, format);
  bool res = etcpal_vcreate_syslog_str(syslog_buf, ETCPAL_SYSLOG_STR_MAX_LEN, &cur_time, &kVcreateTestSyslogParams,
                                       VCREATE_SYSLOG_TEST_PRI, format, args);
  va_end(args);
  return res;
}

TEST(etcpal_log, vcreate_syslog_str_works)
{
  TEST_ASSERT_TRUE(vcreate_syslog_test_helper(VCREATE_SYSLOG_TEST_FORMAT_STR_AND_ARGS));
  TEST_ASSERT_EQUAL_STRING(syslog_buf, VCREATE_SYSLOG_TEST_EXPECTED_RESULT);
}

#define VCREATE_LEGACY_SYSLOG_TEST_EXPECTED_RESULT \
  "<24>Jan  1 00:00:00 test_host test_app[test_proc]: Here are some random values: hey 25 3d5"

static bool vcreate_legacy_syslog_test_helper(const char* format, ...)
{
  va_list args;
  va_start(args, format);
  bool res = etcpal_vcreate_legacy_syslog_str(syslog_buf, ETCPAL_SYSLOG_STR_MAX_LEN, &cur_time,
                                              &kVcreateTestSyslogParams, VCREATE_SYSLOG_TEST_PRI, format, args);
  va_end(args);
  return res;
}

TEST(etcpal_log, vcreate_legacy_syslog_str_works)
{
  TEST_ASSERT_TRUE(vcreate_legacy_syslog_test_helper(VCREATE_SYSLOG_TEST_FORMAT_STR_AND_ARGS));
  TEST_ASSERT_EQUAL_STRING(syslog_buf, VCREATE_LEGACY_SYSLOG_TEST_EXPECTED_RESULT);
}

TEST_GROUP_RUNNER(etcpal_log)
{
  RUN_TEST_CASE(etcpal_log, sanitize_syslog_params_works);
  RUN_TEST_CASE(etcpal_log, validate_log_params_works);
  RUN_TEST_CASE(etcpal_log, validate_log_timestamp_works);
  RUN_TEST_CASE(etcpal_log, action_human_only_is_honored);
  RUN_TEST_CASE(etcpal_log, action_syslog_only_is_honored);
  RUN_TEST_CASE(etcpal_log, action_legacy_syslog_only_is_honored);
  RUN_TEST_CASE(etcpal_log, action_all_is_honored);
  RUN_TEST_CASE(etcpal_log, context_pointer_is_passed_unmodified);
  RUN_TEST_CASE(etcpal_log, priority_is_passed_unmodified);
  RUN_TEST_CASE(etcpal_log, syslog_header_with_all_parts);
  RUN_TEST_CASE(etcpal_log, syslog_header_minus_time);
  RUN_TEST_CASE(etcpal_log, syslog_header_minus_hostname);
  RUN_TEST_CASE(etcpal_log, syslog_header_minus_app_name);
  RUN_TEST_CASE(etcpal_log, syslog_header_minus_procid);
  RUN_TEST_CASE(etcpal_log, syslog_header_with_no_values);
  RUN_TEST_CASE(etcpal_log, legacy_syslog_header_with_all_parts);
  RUN_TEST_CASE(etcpal_log, legacy_syslog_header_minus_timestamp);
  RUN_TEST_CASE(etcpal_log, legacy_syslog_header_minus_hostname);
  RUN_TEST_CASE(etcpal_log, legacy_syslog_header_minus_app_name);
  RUN_TEST_CASE(etcpal_log, legacy_syslog_header_minus_procid);
  RUN_TEST_CASE(etcpal_log, syslog_prival_is_correct);
  RUN_TEST_CASE(etcpal_log, log_mask_is_honored);
  RUN_TEST_CASE(etcpal_log, human_time_header_is_well_formed);
  RUN_TEST_CASE(etcpal_log, syslog_time_header_is_well_formed);
  RUN_TEST_CASE(etcpal_log, legacy_syslog_time_header_is_well_formed);
  RUN_TEST_CASE(etcpal_log, formatting_int_values_works);
  RUN_TEST_CASE(etcpal_log, formatting_string_values_works);
  RUN_TEST_CASE(etcpal_log, maximum_length_human_string_works);
  RUN_TEST_CASE(etcpal_log, maximum_length_syslog_string_works);
  RUN_TEST_CASE(etcpal_log, maximum_length_legacy_syslog_string_works);
  RUN_TEST_CASE(etcpal_log, vcreate_log_str_works);
  RUN_TEST_CASE(etcpal_log, vcreate_syslog_str_works);
  RUN_TEST_CASE(etcpal_log, vcreate_legacy_syslog_str_works);
}
