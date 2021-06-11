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

#include "etcpal/cpp/log.h"
#include "unity_fixture.h"

#include <functional>
#include <string>
#include <vector>

// It would be nice to have GoogleMock available, but alas...
class TestLogMessageHandler : public etcpal::LogMessageHandler
{
public:
  void WillReturnTimestamp(const etcpal::LogTimestamp& timestamp) { timestamp_to_return_ = timestamp; }
  void OnLogEvent(const std::function<void(const EtcPalLogStrings&)>& func) { log_event_ = func; }

  int  LogEventCallCount() const { return log_event_call_count_; }
  void ResetLogEventCallCount() { log_event_call_count_ = 0; }

private:
  etcpal::LogTimestamp GetLogTimestamp() override { return timestamp_to_return_; }
  void                 HandleLogMessage(const EtcPalLogStrings& strings) override;

  etcpal::LogTimestamp                         timestamp_to_return_{1970, 1, 1, 0, 0, 0, 0, 0};
  std::function<void(const EtcPalLogStrings&)> log_event_;
  int                                          log_event_call_count_{0};
};

void TestLogMessageHandler::HandleLogMessage(const EtcPalLogStrings& strings)
{
  if (log_event_)
    log_event_(strings);
  ++log_event_call_count_;
}

static TestLogMessageHandler test_log_handler;
static etcpal::Logger        logger;

extern "C" {
TEST_GROUP(etcpal_cpp_log_timestamp);

TEST_SETUP(etcpal_cpp_log_timestamp)
{
}

TEST_TEAR_DOWN(etcpal_cpp_log_timestamp)
{
}

// Default-constructed LogTimestamp should be invalid.
TEST(etcpal_cpp_log_timestamp, default_constructor_works)
{
  etcpal::LogTimestamp timestamp;
  TEST_ASSERT_FALSE(timestamp.IsValid());
}

// Timestamp returned from the LogTimestamp::Invalid() function should be invalid
TEST(etcpal_cpp_log_timestamp, invalid_works)
{
  auto timestamp = etcpal::LogTimestamp::Invalid();
  TEST_ASSERT_FALSE(timestamp.IsValid());
}

TEST(etcpal_cpp_log_timestamp, value_constructor_works)
{
  etcpal::LogTimestamp timestamp(1970, 1, 1, 2, 3, 4, 500, -60);
  TEST_ASSERT_TRUE(timestamp.IsValid());

  TEST_ASSERT_EQUAL_UINT(timestamp.get().year, 1970);
  TEST_ASSERT_EQUAL_UINT(timestamp.get().month, 1);
  TEST_ASSERT_EQUAL_UINT(timestamp.get().day, 1);
  TEST_ASSERT_EQUAL_UINT(timestamp.get().hour, 2);
  TEST_ASSERT_EQUAL_UINT(timestamp.get().minute, 3);
  TEST_ASSERT_EQUAL_UINT(timestamp.get().second, 4);
  TEST_ASSERT_EQUAL_UINT(timestamp.get().msec, 500);
  TEST_ASSERT_EQUAL_INT(timestamp.get().utc_offset, -60);
}

TEST_GROUP_RUNNER(etcpal_cpp_log_timestamp)
{
  RUN_TEST_CASE(etcpal_cpp_log_timestamp, default_constructor_works);
  RUN_TEST_CASE(etcpal_cpp_log_timestamp, invalid_works);
  RUN_TEST_CASE(etcpal_cpp_log_timestamp, value_constructor_works);
}

TEST_GROUP(etcpal_cpp_log);

TEST_SETUP(etcpal_cpp_log)
{
  logger = etcpal::Logger{};
  test_log_handler = TestLogMessageHandler{};
}

TEST_TEAR_DOWN(etcpal_cpp_log)
{
}

TEST(etcpal_cpp_log, startup_works)
{
  std::string human_str;
  test_log_handler.OnLogEvent([&human_str](const EtcPalLogStrings& strings) { human_str = strings.human_readable; });

  // Logging should not work if the logger has not been started
  logger.Debug("Test Message");
  TEST_ASSERT_EQUAL_INT(test_log_handler.LogEventCallCount(), 0);

  // Startup should work - after starting, logging should work
  TEST_ASSERT_TRUE(logger.SetDispatchPolicy(etcpal::LogDispatchPolicy::kDirect)
                       .SetLogAction(ETCPAL_LOG_CREATE_HUMAN_READABLE)
                       .Startup(test_log_handler));

  logger.Debug("Test Message");
  TEST_ASSERT_EQUAL_INT(test_log_handler.LogEventCallCount(), 1);
  TEST_ASSERT_EQUAL_STRING(human_str.c_str(), "1970-01-01 00:00:00.000Z [DBUG] Test Message");

  logger.Shutdown();
}

// Test the dispatch_policy, log_action and log_params setters/getters
TEST(etcpal_cpp_log, basic_getters_work)
{
  logger.SetDispatchPolicy(etcpal::LogDispatchPolicy::kDirect).SetLogAction(ETCPAL_LOG_CREATE_SYSLOG);

  TEST_ASSERT_EQUAL(logger.dispatch_policy(), etcpal::LogDispatchPolicy::kDirect);
  TEST_ASSERT_EQUAL(logger.log_action(), ETCPAL_LOG_CREATE_SYSLOG);
  TEST_ASSERT_EQUAL(logger.log_params().action, ETCPAL_LOG_CREATE_SYSLOG);
}

// Test the functionality of the log mask.
TEST(etcpal_cpp_log, log_mask_works)
{
  TEST_ASSERT_TRUE(logger.SetDispatchPolicy(etcpal::LogDispatchPolicy::kDirect)
                       .SetLogAction(ETCPAL_LOG_CREATE_HUMAN_READABLE)
                       .Startup(test_log_handler));

  logger.SetLogMask(ETCPAL_LOG_UPTO(ETCPAL_LOG_WARNING));
  TEST_ASSERT_EQUAL_INT(logger.log_mask(), ETCPAL_LOG_UPTO(ETCPAL_LOG_WARNING));

  // Mask should allow levels at or above ETCPAL_LOG_WARNING
  TEST_ASSERT_TRUE(logger.CanLog(ETCPAL_LOG_EMERG));
  TEST_ASSERT_TRUE(logger.CanLog(ETCPAL_LOG_ALERT));
  TEST_ASSERT_TRUE(logger.CanLog(ETCPAL_LOG_CRIT));
  TEST_ASSERT_TRUE(logger.CanLog(ETCPAL_LOG_ERR));
  TEST_ASSERT_TRUE(logger.CanLog(ETCPAL_LOG_WARNING));
  TEST_ASSERT_FALSE(logger.CanLog(ETCPAL_LOG_NOTICE));
  TEST_ASSERT_FALSE(logger.CanLog(ETCPAL_LOG_INFO));
  TEST_ASSERT_FALSE(logger.CanLog(ETCPAL_LOG_DEBUG));

  // Log commands should follow the same logic
  logger.Log(ETCPAL_LOG_DEBUG, "Test Message");
  logger.Debug("Test Message");
  logger.Log(ETCPAL_LOG_INFO, "Test Message");
  logger.Info("Test Message");
  logger.Log(ETCPAL_LOG_NOTICE, "Test Message");
  logger.Notice("Test Message");
  TEST_ASSERT_EQUAL_INT(test_log_handler.LogEventCallCount(), 0);

  // Now the unfiltered ones
  logger.Log(ETCPAL_LOG_WARNING, "Test Message");
  logger.Warning("Test Message");
  logger.Log(ETCPAL_LOG_ERR, "Test Message");
  logger.Error("Test Message");
  logger.Log(ETCPAL_LOG_CRIT, "Test Message");
  logger.Critical("Test Message");
  logger.Log(ETCPAL_LOG_ALERT, "Test Message");
  logger.Alert("Test Message");
  logger.Log(ETCPAL_LOG_EMERG, "Test Message");
  logger.Emergency("Test Message");
  TEST_ASSERT_EQUAL_INT(test_log_handler.LogEventCallCount(), 10);

  logger.Shutdown();
}

// Test the functionality of the Log() and log shortcut functions.
TEST(etcpal_cpp_log, log_functions_work)
{
  TEST_ASSERT_TRUE(logger.SetDispatchPolicy(etcpal::LogDispatchPolicy::kDirect)
                       .SetLogAction(ETCPAL_LOG_CREATE_HUMAN_READABLE)
                       .Startup(test_log_handler));

  std::string human_str;
  test_log_handler.OnLogEvent([&human_str](const EtcPalLogStrings& strings) { human_str = strings.human_readable; });

  // Test the Log() function and corresponding shortcut for each priority:

  // Debug
  logger.Log(ETCPAL_LOG_DEBUG, "Test message %d", 1);
  logger.Debug("Test Message %d", 1);
  TEST_ASSERT_EQUAL_INT(test_log_handler.LogEventCallCount(), 2);
  TEST_ASSERT_EQUAL_STRING(human_str.c_str(), "1970-01-01 00:00:00.000Z [DBUG] Test Message 1");
  test_log_handler.ResetLogEventCallCount();

  // Info
  logger.Log(ETCPAL_LOG_INFO, "Test message %d", 2);
  logger.Info("Test Message %d", 2);
  TEST_ASSERT_EQUAL_INT(test_log_handler.LogEventCallCount(), 2);
  TEST_ASSERT_EQUAL_STRING(human_str.c_str(), "1970-01-01 00:00:00.000Z [INFO] Test Message 2");
  test_log_handler.ResetLogEventCallCount();

  // Notice
  logger.Log(ETCPAL_LOG_NOTICE, "Test message %d", 3);
  logger.Notice("Test Message %d", 3);
  TEST_ASSERT_EQUAL_INT(test_log_handler.LogEventCallCount(), 2);
  TEST_ASSERT_EQUAL_STRING(human_str.c_str(), "1970-01-01 00:00:00.000Z [NOTI] Test Message 3");
  test_log_handler.ResetLogEventCallCount();

  // Warning
  logger.Log(ETCPAL_LOG_WARNING, "Test message %d", 4);
  logger.Warning("Test Message %d", 4);
  TEST_ASSERT_EQUAL_INT(test_log_handler.LogEventCallCount(), 2);
  TEST_ASSERT_EQUAL_STRING(human_str.c_str(), "1970-01-01 00:00:00.000Z [WARN] Test Message 4");
  test_log_handler.ResetLogEventCallCount();

  // Error
  logger.Log(ETCPAL_LOG_ERR, "Test message %d", 5);
  logger.Error("Test Message %d", 5);
  TEST_ASSERT_EQUAL_INT(test_log_handler.LogEventCallCount(), 2);
  TEST_ASSERT_EQUAL_STRING(human_str.c_str(), "1970-01-01 00:00:00.000Z [ERR ] Test Message 5");
  test_log_handler.ResetLogEventCallCount();

  // Critical
  logger.Log(ETCPAL_LOG_CRIT, "Test message %d", 6);
  logger.Critical("Test Message %d", 6);
  TEST_ASSERT_EQUAL_INT(test_log_handler.LogEventCallCount(), 2);
  TEST_ASSERT_EQUAL_STRING(human_str.c_str(), "1970-01-01 00:00:00.000Z [CRIT] Test Message 6");
  test_log_handler.ResetLogEventCallCount();

  // Alert
  logger.Log(ETCPAL_LOG_ALERT, "Test message %d", 7);
  logger.Alert("Test Message %d", 7);
  TEST_ASSERT_EQUAL_INT(test_log_handler.LogEventCallCount(), 2);
  TEST_ASSERT_EQUAL_STRING(human_str.c_str(), "1970-01-01 00:00:00.000Z [ALRT] Test Message 7");
  test_log_handler.ResetLogEventCallCount();

  // Emergency
  logger.Log(ETCPAL_LOG_EMERG, "Test message %d", 8);
  logger.Emergency("Test Message %d", 8);
  TEST_ASSERT_EQUAL_INT(test_log_handler.LogEventCallCount(), 2);
  TEST_ASSERT_EQUAL_STRING(human_str.c_str(), "1970-01-01 00:00:00.000Z [EMRG] Test Message 8");

  logger.Shutdown();
}

TEST(etcpal_cpp_log, timestamps_work)
{
  TEST_ASSERT_TRUE(logger.SetDispatchPolicy(etcpal::LogDispatchPolicy::kDirect)
                       .SetLogAction(ETCPAL_LOG_CREATE_HUMAN_READABLE)
                       .Startup(test_log_handler));

  // January 22, 2020 16:57:33.123 UTC-06:00
  etcpal::LogTimestamp timestamp(2020, 1, 22, 16, 57, 33, 123, -360);
  test_log_handler.WillReturnTimestamp(timestamp);

  std::string human_str;
  test_log_handler.OnLogEvent([&human_str](const EtcPalLogStrings& strings) { human_str = strings.human_readable; });

  logger.Info("Test Message");
  TEST_ASSERT_EQUAL_INT(test_log_handler.LogEventCallCount(), 1);
  TEST_ASSERT_EQUAL_STRING(human_str.c_str(), "2020-01-22 16:57:33.123-06:00 [INFO] Test Message");

  // Returning LogTimestamp::Invalid() should omit the timestamp.
  test_log_handler.WillReturnTimestamp(etcpal::LogTimestamp::Invalid());
  logger.Info("Test Message");
  TEST_ASSERT_EQUAL_INT(test_log_handler.LogEventCallCount(), 2);
  TEST_ASSERT_EQUAL_STRING(human_str.c_str(), "[INFO] Test Message");

  logger.Shutdown();
}

TEST(etcpal_cpp_log, syslog_params_work)
{
  TEST_ASSERT_TRUE(logger.SetSyslogFacility(ETCPAL_LOG_LOCAL1)
                       .SetSyslogHostname("MyHost")
                       .SetSyslogAppName("TestApp")
                       .SetSyslogProcId(200)
                       .SetDispatchPolicy(etcpal::LogDispatchPolicy::kDirect)
                       .SetLogAction(ETCPAL_LOG_CREATE_SYSLOG | ETCPAL_LOG_CREATE_LEGACY_SYSLOG)
                       .Startup(test_log_handler));

  TEST_ASSERT_EQUAL_INT(logger.syslog_facility(), ETCPAL_LOG_LOCAL1);
  TEST_ASSERT_EQUAL_STRING(logger.syslog_hostname(), "MyHost");
  TEST_ASSERT_EQUAL_STRING(logger.syslog_app_name(), "TestApp");
  TEST_ASSERT_EQUAL_STRING(logger.syslog_procid(), "200");

  std::string syslog_str;
  std::string legacy_syslog_str;
  test_log_handler.OnLogEvent([&](const EtcPalLogStrings& strings) {
    syslog_str = strings.syslog;
    legacy_syslog_str = strings.legacy_syslog;
  });
  logger.Log(ETCPAL_LOG_INFO, "Test Message");
  TEST_ASSERT_EQUAL_STRING(syslog_str.c_str(), "<142>1 1970-01-01T00:00:00.000Z MyHost TestApp 200 - - Test Message");
  TEST_ASSERT_EQUAL_STRING(legacy_syslog_str.c_str(), "<142>Jan  1 00:00:00 MyHost TestApp[200]: Test Message");
}

TEST(etcpal_cpp_log, queued_dispatch_works)
{
  std::vector<std::string> log_strs;
  log_strs.reserve(3);
  test_log_handler.OnLogEvent(
      [&log_strs](const EtcPalLogStrings& strings) { log_strs.emplace_back(strings.human_readable); });

  // Startup should work - after starting, logging should work
  TEST_ASSERT_TRUE(logger.SetDispatchPolicy(etcpal::LogDispatchPolicy::kQueued)
                       .SetLogAction(ETCPAL_LOG_CREATE_HUMAN_READABLE)
                       .SetLogMask(ETCPAL_LOG_UPTO(ETCPAL_LOG_DEBUG))
                       .Startup(test_log_handler));

  logger.Debug("Test Message 1");
  logger.Debug("Test Message 2");
  logger.Debug("Test Message 3");

  // Wait for the thread to dispatch the log messages and stop
  logger.Shutdown();

  TEST_ASSERT_EQUAL_INT(test_log_handler.LogEventCallCount(), 3);
  TEST_ASSERT_EQUAL(log_strs.size(), 3);
  TEST_ASSERT_EQUAL_STRING(log_strs[0].c_str(), "1970-01-01 00:00:00.000Z [DBUG] Test Message 1");
  TEST_ASSERT_EQUAL_STRING(log_strs[1].c_str(), "1970-01-01 00:00:00.000Z [DBUG] Test Message 2");
  TEST_ASSERT_EQUAL_STRING(log_strs[2].c_str(), "1970-01-01 00:00:00.000Z [DBUG] Test Message 3");
}

TEST_GROUP_RUNNER(etcpal_cpp_log)
{
  RUN_TEST_CASE(etcpal_cpp_log, startup_works);
  RUN_TEST_CASE(etcpal_cpp_log, basic_getters_work);
  RUN_TEST_CASE(etcpal_cpp_log, log_mask_works);
  RUN_TEST_CASE(etcpal_cpp_log, log_functions_work);
  RUN_TEST_CASE(etcpal_cpp_log, timestamps_work);
  RUN_TEST_CASE(etcpal_cpp_log, syslog_params_work);
  RUN_TEST_CASE(etcpal_cpp_log, queued_dispatch_works);
}
}
