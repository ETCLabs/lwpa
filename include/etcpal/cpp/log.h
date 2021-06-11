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

/// @file etcpal/cpp/log.h
/// @brief C++ wrapper and utilities for etcpal/log.h

#ifndef ETCPAL_CPP_LOG_H_
#define ETCPAL_CPP_LOG_H_

#include <array>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <memory>
#include <queue>
#include <string>
#include "etcpal/common.h"
#include "etcpal/log.h"
#include "etcpal/cpp/common.h"
#include "etcpal/cpp/mutex.h"
#include "etcpal/cpp/signal.h"
#include "etcpal/cpp/thread.h"

namespace etcpal
{
/// @defgroup etcpal_cpp_log log (Logging)
/// @ingroup etcpal_cpp
/// @brief C++ utilities for the @ref etcpal_log module.
///
/// This module contains a class Logger which is used to gather log messages dispatched from
/// different portions of an application into a single stream with a unified format. It wraps the C
/// logging module, which provides the core of this functionality, in a convenience layer which can
/// also spawn a background thread to handle large log volumes. See the
/// @ref etcpal_log "documentation for the C module" for more detailed information on the log
/// format.
///
/// Generally a single instance per application is sufficient:
///
/// @code
/// // This class handles the log messages gathered by the Logger class.
/// class MyLogHandler : public etcpal::LogMessageHandler
/// {
///   etcpal::LogTimestamp GetLogTimestamp() override
///   {
///     // Grab a timestamp from the local system and return it
///     // This function can also be omitted if you can't easily obtain system time. No timestamps
///     // will be prepended to log messages in that case.
///   }
///
///   void HandleLogMessage(const EtcPalLogStrings& strings) override
///   {
///     // "strings" will contain strings pertaining to what action was requested by
///     // Logger::SetLogAction().
///
///     // Dispatch log messages in an application-defined way.
///
///     // If human-readable logging is enabled, maybe...
///     printf("%s\n", strings.human_readable);
///     // If syslog is enabled, maybe...
///     etcpal_sendto(my_syslog_socket, strings.syslog, strlen(strings.syslog), 0, &my_syslog_server);
///   }
/// };
///
/// int main()
/// {
///   etcpal_init(ETCPAL_FEATURE_LOGGING);
///
///   MyLogHandler log_handler;
///   etcpal::Logger logger;
///
///   // Set parameters and start logging using the method-chaining setters:
///   logger.SetLogMask(ETCPAL_LOG_UPTO(ETCPAL_LOG_INFO))
///         .SetLogAction(kEtcPalLogCreateBoth)
///         .SetSyslogFacility(ETCPAL_LOG_LOCAL1)
///         .SetSyslogHostname(my_hostname)
///         .SetSyslogAppName("My App")
///         .SetSyslogProcId(0)
///         .Startup(log_handler);
///
///   // By default, a background thread is started to dispatch the log messages.
///
///   logger.Log(ETCPAL_LOG_INFO, "Starting up!");
///
///   // Use the shortcuts to log at a specific priority level:
///
///   logger.Warning("Uh oh...");
///   logger.Debug("We've reached line 250");
///
///   // All log functions accept printf-style format specifiers and arguments
///   logger.Info("This module has gone %d days without an accident", -1);
///
///   // Use the log_params() getter to add logging output from other libraries with C APIs that
///   // support it.
///   somelib_init(&logger.log_params());
///
///   // Shut down the logger when done to join the thread, if applicable.
///   logger.Shutdown();
///   etcpal_deinit(ETCPAL_FEATURE_LOGGING);
///   return 0;
/// }
/// @endcode

/// @ingroup etcpal_cpp_log
/// @brief An object representing the current local time with millisecond resolution for logging purposes.
/// @details Can also be invalid, which means it does not hold a valid timestamp.
class LogTimestamp
{
public:
  /// Construct an invalid timestamp by default.
  LogTimestamp() = default;
  constexpr LogTimestamp(unsigned int year,
                         unsigned int month,
                         unsigned int day,
                         unsigned int hour,
                         unsigned int minute,
                         unsigned int second,
                         unsigned int msec = 0,
                         int          utc_offset = 0);

  bool IsValid() const noexcept;

  constexpr const EtcPalLogTimestamp& get() const noexcept;
  ETCPAL_CONSTEXPR_14 EtcPalLogTimestamp& get() noexcept;

  static LogTimestamp Invalid();

private:
  EtcPalLogTimestamp timestamp_{};
};

/// @brief Create a timestamp representing the current time.
/// @param year Absolute year. Valid range 0-9999.
/// @param month Month of the year. Valid range 1-12 (starting with 1 for January).
/// @param day Day of the month. Valid range 1-31.
/// @param hour Hours since midnight. Valid range 0-23.
/// @param minute Minutes past the current hour. Valid range 0-59.
/// @param second Seconds past the current minute. Valid range 0-60 (to handle leap seconds).
/// @param msec Milliseconds past the current second. Valid range 0-999.
/// @param utc_offset The local offset from UTC in minutes.
constexpr LogTimestamp::LogTimestamp(unsigned int year,
                                     unsigned int month,
                                     unsigned int day,
                                     unsigned int hour,
                                     unsigned int minute,
                                     unsigned int second,
                                     unsigned int msec,
                                     int          utc_offset)
    : timestamp_{year, month, day, hour, minute, second, msec, utc_offset}
{
}

/// @brief Whether this timestamp represents a valid time.
inline bool LogTimestamp::IsValid() const noexcept
{
  return etcpal_validate_log_timestamp(&timestamp_);
}

/// @brief Get a const reference to the underlying C type.
constexpr const EtcPalLogTimestamp& LogTimestamp::get() const noexcept
{
  return timestamp_;
}

/// @brief Get a mutable reference to the underlying C type.
ETCPAL_CONSTEXPR_14_OR_INLINE EtcPalLogTimestamp& LogTimestamp::get() noexcept
{
  return timestamp_;
}

/// @brief Construct an invalid timestamp.
inline LogTimestamp LogTimestamp::Invalid()
{
  return LogTimestamp{};
}

/// @ingroup etcpal_cpp_log
/// @brief An interface which handles log messages.
///
/// Users of the Logger class must provide a class that derives from LogMessageHandler to handle
/// the Logger's output. LogMessageHandler has two jobs: getting a current timestamp to apply to
/// log messages, and handling the final dispatched log message.
class LogMessageHandler
{
public:
  virtual LogTimestamp GetLogTimestamp();

  /// @brief Define this function to handle log messages and determine what to do with them.
  ///
  /// If the corresponding Logger has a dispatch policy of LogDispatchPolicy::kDirect, this function
  /// is invoked directly from the context of the corresponding call to Log() or similar. In this
  /// case, be mindful of whether this function implementation has potential to block. If
  /// significant blocking is a possibility while handling log messages, consider using
  /// LogDispatchPolicy::kQueued.
  ///
  /// @param strings Strings associated with the log message. Will contain valid strings
  ///                corresponding to the log actions requested using Logger::SetLogAction().
  virtual void HandleLogMessage(const EtcPalLogStrings& strings) = 0;
};

/// @brief Return a LogTimestamp representing the current local time.
/// @details Optional; the default implementation does not append a timestamp to log messages.
inline LogTimestamp LogMessageHandler::GetLogTimestamp()
{
  return LogTimestamp::Invalid();
}

/// @ingroup etcpal_cpp_log
/// @brief Options for the method by which the Logger dispatches log messages.
enum class LogDispatchPolicy
{
  kDirect,  ///< Log messages propagate directly from Log() calls to output streams (normally only used for testing)
  kQueued   ///< Log messages are queued and dispatched from another thread (recommended)
};

/// @ingroup etcpal_cpp_log
/// @brief A class for dispatching log messages.
///
/// See the long description for the @ref etcpal_cpp_log module for more detailed usage
/// information.
class Logger
{
public:
  Logger();

  bool Startup(LogMessageHandler& message_handler);
  void Shutdown();

  bool CanLog(int pri) const noexcept;
  void Log(int pri, const char* format, ...);

  /// @name Logging Shortcuts
  /// @{
  void Debug(const char* format, ...);
  void Info(const char* format, ...);
  void Notice(const char* format, ...);
  void Warning(const char* format, ...);
  void Error(const char* format, ...);
  void Critical(const char* format, ...);
  void Alert(const char* format, ...);
  void Emergency(const char* format, ...);
  /// @}

  /// @name Getters
  /// @{
  LogDispatchPolicy      dispatch_policy() const noexcept;
  int                    log_mask() const noexcept;
  int                    log_action() const noexcept;
  int                    syslog_facility() const noexcept;
  const char*            syslog_hostname() const noexcept;
  const char*            syslog_app_name() const noexcept;
  const char*            syslog_procid() const noexcept;
  const EtcPalLogParams& log_params() const noexcept;
  /// @}

  /// @name Setters
  /// @{
  Logger& SetDispatchPolicy(LogDispatchPolicy new_policy) noexcept;
  Logger& SetLogMask(int log_mask) noexcept;
  Logger& SetLogAction(int log_action) noexcept;
  Logger& SetSyslogFacility(int facility) noexcept;
  Logger& SetSyslogHostname(const char* hostname) noexcept;
  Logger& SetSyslogHostname(const std::string& hostname) noexcept;
  Logger& SetSyslogAppName(const char* app_name) noexcept;
  Logger& SetSyslogAppName(const std::string& app_name) noexcept;
  Logger& SetSyslogProcId(const char* proc_id) noexcept;
  Logger& SetSyslogProcId(const std::string& proc_id) noexcept;
  Logger& SetSyslogProcId(int proc_id) noexcept;

  Logger& SetThreadPriority(unsigned int priority) noexcept;
  Logger& SetThreadStackSize(unsigned int stack_size) noexcept;
  Logger& SetThreadName(const char* name) noexcept;
  Logger& SetThreadName(const std::string& name) noexcept;
  Logger& SetThreadPlatformData(void* platform_data) noexcept;
  /// @}

private:
  void LogInternal(int pri, const char* format, std::va_list args);
  void LogThreadRun();
  void EmptyLogQueue();

  LogDispatchPolicy dispatch_policy_{LogDispatchPolicy::kQueued};
  EtcPalLogParams   log_params_{};

  struct LogMessage
  {
    int                                          pri;
    std::array<char, ETCPAL_RAW_LOG_MSG_MAX_LEN> buf;
  };

  // Used when dispatch_policy_ == Queued
  // This implementation will become more elegant with the addition of new queue APIs. This is
  // being tracked with ETCPAL-43 and ETCPAL-46.
  std::queue<LogMessage>          msg_q_;
  std::unique_ptr<etcpal::Signal> signal_;
  std::unique_ptr<etcpal::Mutex>  mutex_;
  etcpal::Thread                  thread_;
  bool                            running_{false};
};

/// @cond Internal log callback functions

extern "C" inline void LogCallbackFn(void* context, const EtcPalLogStrings* strings)
{
  if (context && strings)
  {
    static_cast<LogMessageHandler*>(context)->HandleLogMessage(*strings);
  }
}

extern "C" inline void LogTimestampFn(void* context, EtcPalLogTimestamp* timestamp)
{
  if (context && timestamp)
  {
    *timestamp = static_cast<LogMessageHandler*>(context)->GetLogTimestamp().get();
  }
}

/// @endcond

inline Logger::Logger()
{
  // Default logging parameters
  log_params_.action = ETCPAL_LOG_CREATE_HUMAN_READABLE;
  log_params_.log_fn = LogCallbackFn;
  log_params_.log_mask = ETCPAL_LOG_UPTO(ETCPAL_LOG_DEBUG);
  log_params_.time_fn = LogTimestampFn;
  log_params_.context = nullptr;
}

/// @brief Start logging.
///
/// Spawns a thread to dispatch log messages unless SetDispatchPolicy(LogDispatchPolicy::kDirect)
/// has been called. Do not call more than once between calls to Shutdown().
///
/// @param message_handler The class instance that will handle log messages from this logger.
inline bool Logger::Startup(LogMessageHandler& message_handler)
{
  if (etcpal_init(ETCPAL_FEATURE_LOGGING) != kEtcPalErrOk)
    return false;

  if (!etcpal_validate_log_params(&log_params_))
  {
    etcpal_deinit(ETCPAL_FEATURE_LOGGING);
    return false;
  }

  log_params_.context = &message_handler;

  if (dispatch_policy_ == LogDispatchPolicy::kDirect)
  {
    running_ = true;
    return true;
  }

  // kQueued
  // Start the log dispatch thread
  running_ = true;
  signal_ = std::unique_ptr<etcpal::Signal>(new etcpal::Signal);
  mutex_ = std::unique_ptr<etcpal::Mutex>(new etcpal::Mutex);
  if (!thread_.SetName("EtcPalLoggerThread").Start(&Logger::LogThreadRun, this))
  {
    etcpal_deinit(ETCPAL_FEATURE_LOGGING);
    return false;
  }
  return true;
}

/// @brief Stop logging.
///
/// If the dispatch policy is LogDispatchPolicy::kQueued (the default) this will dispatch the rest
/// of the log messages and wait for the dispatch thread to join.
inline void Logger::Shutdown()
{
  if (running_)
  {
    running_ = false;
    if (dispatch_policy_ == LogDispatchPolicy::kQueued)
    {
      signal_->Notify();
      thread_.Join();
    }
    etcpal_deinit(ETCPAL_FEATURE_LOGGING);
    log_params_.context = nullptr;
  }
}

/// @brief Determine whether a priority level can be logged using the mask given via SetLogMask().
///
/// See etcpal_can_log() for more information.
inline bool Logger::CanLog(int pri) const noexcept
{
  return etcpal_can_log(&log_params_, pri);
}

/// @brief Log a message.
///
/// Only dispatches the log message if pri is part of the mask previously passed to SetLogMask().
///
/// @param pri The priority of this log message.
/// @param format Log message with printf-style format specifiers. Provide additional arguments as
///               appropriate.
inline void Logger::Log(int pri, const char* format, ...)
{
  std::va_list args;
  va_start(args, format);
  LogInternal(pri, format, args);
  va_end(args);
}

/// @brief Log a message at debug priority.
inline void Logger::Debug(const char* format, ...)
{
  std::va_list args;
  va_start(args, format);
  LogInternal(ETCPAL_LOG_DEBUG, format, args);
  va_end(args);
}

/// @brief Log a message at informational priority.
inline void Logger::Info(const char* format, ...)
{
  std::va_list args;
  va_start(args, format);
  LogInternal(ETCPAL_LOG_INFO, format, args);
  va_end(args);
}

/// @brief Log a message at notice priority.
inline void Logger::Notice(const char* format, ...)
{
  std::va_list args;
  va_start(args, format);
  LogInternal(ETCPAL_LOG_NOTICE, format, args);
  va_end(args);
}

/// @brief Log a message at warning priority.
inline void Logger::Warning(const char* format, ...)
{
  std::va_list args;
  va_start(args, format);
  LogInternal(ETCPAL_LOG_WARNING, format, args);
  va_end(args);
}

/// @brief Log a message at error priority.
inline void Logger::Error(const char* format, ...)
{
  std::va_list args;
  va_start(args, format);
  LogInternal(ETCPAL_LOG_ERR, format, args);
  va_end(args);
}

/// @brief Log a message at critical priority.
inline void Logger::Critical(const char* format, ...)
{
  std::va_list args;
  va_start(args, format);
  LogInternal(ETCPAL_LOG_CRIT, format, args);
  va_end(args);
}

/// @brief Log a message at alert priority.
inline void Logger::Alert(const char* format, ...)
{
  std::va_list args;
  va_start(args, format);
  LogInternal(ETCPAL_LOG_ALERT, format, args);
  va_end(args);
}

/// @brief Log a message at emergency priority.
inline void Logger::Emergency(const char* format, ...)
{
  std::va_list args;
  va_start(args, format);
  LogInternal(ETCPAL_LOG_EMERG, format, args);
  va_end(args);
}

/// @brief Get the current log dispatch policy.
inline LogDispatchPolicy Logger::dispatch_policy() const noexcept
{
  return dispatch_policy_;
}

/// @brief Get the current log mask.
inline int Logger::log_mask() const noexcept
{
  return log_params_.log_mask;
}

/// @brief Get the current log action.
inline int Logger::log_action() const noexcept
{
  return log_params_.action;
}

/// @brief Get the current Syslog facility value.
inline int Logger::syslog_facility() const noexcept
{
  return log_params_.syslog_params.facility;
}

/// @brief Get the current Syslog HOSTNAME.
inline const char* Logger::syslog_hostname() const noexcept
{
  return log_params_.syslog_params.hostname;
}

/// @brief Get the current Syslog APP-NAME.
inline const char* Logger::syslog_app_name() const noexcept
{
  return log_params_.syslog_params.app_name;
}

/// @brief Get the current Syslog PROCID.
inline const char* Logger::syslog_procid() const noexcept
{
  return log_params_.syslog_params.procid;
}

/// @brief Get the log params used by this logger.
///
/// This is convenient when interacting with C APIs which take an EtcPalLogParams instance to log
/// their own messages. Passing these params to those APIs will gather those log messages into this
/// logger instance.
inline const EtcPalLogParams& Logger::log_params() const noexcept
{
  return log_params_;
}

/// @brief Change the dispatch policy of this logger.
///
/// Only has any effect if the logger has not been started yet.
///
/// @param new_policy The new dispatch policy.
inline Logger& Logger::SetDispatchPolicy(LogDispatchPolicy new_policy) noexcept
{
  dispatch_policy_ = new_policy;
  return *this;
}

/// @brief Set a new log mask.
///
/// Use the ETCPAL_LOG_UPTO() macro to create a mask up to and including a priority level. For
/// example, ETCPAL_LOG_UPTO(ETCPAL_LOG_WARNING) will allow all messages with priorites from
/// emergency through warning, inclusive.
///
/// @param log_mask The new log mask.
inline Logger& Logger::SetLogMask(int log_mask) noexcept
{
  log_params_.log_mask = log_mask;
  return *this;
}

/// @brief Set the types of log messages to create and dispatch to the LogMessageHandler.
inline Logger& Logger::SetLogAction(int log_action) noexcept
{
  log_params_.action = log_action;
  return *this;
}

/// @brief Set the Syslog facility value; see RFC 5424 &sect; 6.2.1.
inline Logger& Logger::SetSyslogFacility(int facility) noexcept
{
  log_params_.syslog_params.facility = facility;
  return *this;
}

/// @brief Set the Syslog HOSTNAME; see RFC 5424 &sect; 6.2.4.
inline Logger& Logger::SetSyslogHostname(const char* hostname) noexcept
{
  ETCPAL_MSVC_NO_DEP_WRN strncpy(log_params_.syslog_params.hostname, hostname, ETCPAL_LOG_HOSTNAME_MAX_LEN - 1);
  log_params_.syslog_params.hostname[ETCPAL_LOG_HOSTNAME_MAX_LEN - 1] = '\0';
  return *this;
}

/// @brief Set the Syslog HOSTNAME; see RFC 5424 &sect; 6.2.4.
inline Logger& Logger::SetSyslogHostname(const std::string& hostname) noexcept
{
  SetSyslogHostname(hostname.c_str());
  return *this;
}

/// @brief Set the Syslog APP-NAME; see RFC 5424 &sect; 6.2.5.
inline Logger& Logger::SetSyslogAppName(const char* app_name) noexcept
{
  ETCPAL_MSVC_NO_DEP_WRN strncpy(log_params_.syslog_params.app_name, app_name, ETCPAL_LOG_APP_NAME_MAX_LEN - 1);
  log_params_.syslog_params.app_name[ETCPAL_LOG_APP_NAME_MAX_LEN - 1] = '\0';
  return *this;
}

/// @brief Set the Syslog APP-NAME; see RFC 5424 &sect; 6.2.5.
inline Logger& Logger::SetSyslogAppName(const std::string& app_name) noexcept
{
  SetSyslogAppName(app_name.c_str());
  return *this;
}

/// @brief Set the Syslog PROCID; see RFC 5424 &sect; 6.2.6.
inline Logger& Logger::SetSyslogProcId(const char* proc_id) noexcept
{
  ETCPAL_MSVC_NO_DEP_WRN strncpy(log_params_.syslog_params.procid, proc_id, ETCPAL_LOG_PROCID_MAX_LEN - 1);
  log_params_.syslog_params.procid[ETCPAL_LOG_PROCID_MAX_LEN - 1] = '\0';
  return *this;
}

/// @brief Set the Syslog PROCID; see RFC 5424 &sect; 6.2.6.
inline Logger& Logger::SetSyslogProcId(const std::string& proc_id) noexcept
{
  SetSyslogProcId(proc_id.c_str());
  return *this;
}

/// @brief Set the priority of the log dispatch thread.
/// @see etcpal::Thread::SetPriority()
/// @note If the dispatch policy is LogDispatchPolicy::kDirect, this has no effect.
inline Logger& Logger::SetThreadPriority(unsigned int priority) noexcept
{
  thread_.SetPriority(priority);
  return *this;
}

/// @brief Set the stack size of the log dispatch thread.
/// @see etcpal::Thread::SetStackSize()
/// @note If the dispatch policy is LogDispatchPolicy::kDirect, this has no effect.
inline Logger& Logger::SetThreadStackSize(unsigned int stack_size) noexcept
{
  thread_.SetStackSize(stack_size);
  return *this;
}

/// @brief Set the name of the log dispatch thread.
/// @see etcpal::Thread::SetName()
/// @note If the dispatch policy is LogDispatchPolicy::kDirect, this has no effect.
inline Logger& Logger::SetThreadName(const char* name) noexcept
{
  thread_.SetName(name);
  return *this;
}

/// @brief Set the name of the log dispatch thread.
/// @see etcpal::Thread::SetName()
/// @note If the dispatch policy is LogDispatchPolicy::kDirect, this has no effect.
inline Logger& Logger::SetThreadName(const std::string& name) noexcept
{
  thread_.SetName(name);
  return *this;
}

/// @brief Set the platform-specific parameter data of the log dispatch thread.
/// @see etcpal::Thread::SetPlatformData()
/// @note If the dispatch policy is LogDispatchPolicy::kDirect, this has no effect.
inline Logger& Logger::SetThreadPlatformData(void* platform_data) noexcept
{
  thread_.SetPlatformData(platform_data);
  return *this;
}

/// @brief Set the Syslog PROCID; see RFC 5424 &sect; 6.2.6.
inline Logger& Logger::SetSyslogProcId(int proc_id) noexcept
{
  ETCPAL_MSVC_NO_DEP_WRN sprintf(log_params_.syslog_params.procid, "%d", proc_id);
  return *this;
}

// Private functions

inline void Logger::LogInternal(int pri, const char* format, std::va_list args)
{
  if (running_)
  {
    if (dispatch_policy_ == LogDispatchPolicy::kDirect)
    {
      etcpal_vlog(&log_params_, pri, format, args);
    }
    else
    {
      {
        etcpal::MutexGuard lock(*mutex_);
        msg_q_.emplace();
        msg_q_.back().pri = pri;
        vsnprintf(msg_q_.back().buf.data(), ETCPAL_RAW_LOG_MSG_MAX_LEN, format, args);
      }
      signal_->Notify();
    }
  }
}

inline void Logger::LogThreadRun()
{
  while (running_)
  {
    EmptyLogQueue();
    signal_->Wait();
  }
  // Bail the queue one last time on exit
  EmptyLogQueue();
}

inline void Logger::EmptyLogQueue()
{
  std::queue<LogMessage> to_log;
  {
    etcpal::MutexGuard lock(*mutex_);
    to_log.swap(msg_q_);
  }

  while (!to_log.empty())
  {
    etcpal_log(&log_params_, to_log.front().pri, "%s", to_log.front().buf.data());
    to_log.pop();
  }
}

};  // namespace etcpal

#endif  // ETCPAL_CPP_LOG_H_
