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

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !ETCPAL_NO_OS_SUPPORT
#include "etcpal/mutex.h"
#endif
#include "etcpal/mempool.h"
#include "etcpal/private/log.h"

#ifdef _MSC_VER
/* Suppress strncpy() warnings on Windows/MSVC. */
#pragma warning(disable : 4996)

/* Provide va_copy() in pre-VS2013 environments. */
#if _MSC_VER < 1800
#define va_copy(dest, src) (dest = src)
#endif
#endif

/*************************** Private constants *******************************/

#define SYSLOG_PROT_VERSION 1 /*RFC 5424, sec. 6.2.2 */
#define NILVALUE_STR "-"
#define MSGID_STR NILVALUE_STR
#define STRUCTURED_DATA_STR NILVALUE_STR
/* LOG_LOCAL1 seems to be one that's lightly used */
#define DEFAULT_FACILITY ETCPAL_LOG_LOCAL1
/* 1-32 alphanumeric characters, terminated by ": " and a null */
#define LEGACY_SYSLOG_TAG_MAX_LEN 35

// clang-format off
static const char* kLogSeverityStrings[] = {
  "EMRG", // ETCPAL_LOG_EMERG
  "ALRT", // ETCPAL_LOG_ALERT
  "CRIT", // ETCPAL_LOG_CRIT
  "ERR ", // ETCPAL_LOG_ERR
  "WARN", // ETCPAL_LOG_WARNING
  "NOTI", // ETCPAL_LOG_NOTICE
  "INFO", // ETCPAL_LOG_INFO
  "DBUG"  // ETCPAL_LOG_DEBUG
};

static const char* kMonthNames[12] = {
  "Jan",
  "Feb",
  "Mar",
  "Apr",
  "May",
  "Jun",
  "Jul",
  "Aug",
  "Sep",
  "Oct",
  "Nov",
  "Dec"
};
// clang-format on

/**************************** Private variables ******************************/

static unsigned int init_count;
#if !ETCPAL_NO_OS_SUPPORT
static etcpal_mutex_t buf_lock;
#endif

/*********************** Private function prototypes *************************/

static char* create_log_str(char*                     buf,
                            size_t                    buflen,
                            const EtcPalLogTimestamp* timestamp,
                            int                       pri,
                            const char*               format,
                            va_list                   args);
static char* create_syslog_str(char*                     buf,
                               size_t                    buflen,
                               const EtcPalLogTimestamp* timestamp,
                               const EtcPalSyslogParams* syslog_params,
                               int                       pri,
                               const char*               format,
                               va_list                   args);
static char* create_legacy_syslog_str(char*                     buf,
                                      size_t                    buflen,
                                      const EtcPalLogTimestamp* timestamp,
                                      const EtcPalSyslogParams* syslog_params,
                                      int                       pri,
                                      const char*               format,
                                      va_list                   args);

static void sanitize_str(char* str);

static void make_iso_timestamp(const EtcPalLogTimestamp* timestamp, char* buf, bool human_readable);
static void make_legacy_syslog_timestamp(const EtcPalLogTimestamp* timestamp, char* buf);
static void make_legacy_syslog_tag_str(const EtcPalSyslogParams* syslog_params, char* buf);
static bool get_time(const EtcPalLogParams* params, EtcPalLogTimestamp* timestamp);

/*************************** Function definitions ****************************/

/* Initialize the etcpal_log module. Creates the mutex which locks the static buffers that log
 * messages are written into. */
etcpal_error_t etcpal_log_init(void)
{
#if !ETCPAL_NO_OS_SUPPORT
  if (init_count == 0)
  {
    if (!etcpal_mutex_create(&buf_lock))
    {
      return kEtcPalErrSys;
    }
  }
#endif
  ++init_count;
  return kEtcPalErrOk;
}

/* Deinitialize the etcpal_log module. */
void etcpal_log_deinit(void)
{
  --init_count;
#if !ETCPAL_NO_OS_SUPPORT
  if (init_count == 0)
  {
    etcpal_mutex_destroy(&buf_lock);
  }
#endif
}

/**
 * @brief Create a log message with a human-readable prefix in the given buffer.
 *
 * Buffer must be at least #ETCPAL_LOG_STR_MIN_LEN in length.
 *
 * @param[out] buf Buffer in which to build the log message.
 * @param[in] buflen Length in bytes of buf.
 * @param[in] timestamp A timestamp representing the current time. If NULL, no timestamp will be
 *                      added to the log message.
 * @param[in] pri Priority of this log message.
 * @param[in] format Log message with printf-style format specifiers. Provide additional arguments
 *                   as appropriate for format specifiers.
 */
bool etcpal_create_log_str(char*                     buf,
                           size_t                    buflen,
                           const EtcPalLogTimestamp* timestamp,
                           int                       pri,
                           const char*               format,
                           ...)
{
  va_list args;
  va_start(args, format);
  bool res = (create_log_str(buf, buflen, timestamp, pri, format, args) != NULL);
  va_end(args);
  return res;
}

/**
 * @brief Create a log a message with a human-readable prefix in the given buffer.
 *
 * Buffer must be at least #ETCPAL_LOG_STR_MIN_LEN in length. This function is useful if you want
 * to create a wrapper function around etcpal_create_log_str() which also takes variable format
 * arguments.
 *
 * @param[out] buf Buffer in which to build the log message.
 * @param[in] buflen Length in bytes of buf.
 * @param[in] timestamp A timestamp representing the current time. If NULL, no timestamp will be
 *                      added to the log message.
 * @param[in] pri Priority of this log message.
 * @param[in] format Log message with printf-style format specifiers. Provide additional arguments
 *                   as appropriate for format specifiers.
 * @param[in] args Argument list for the format specifiers in format.
 */
bool etcpal_vcreate_log_str(char*                     buf,
                            size_t                    buflen,
                            const EtcPalLogTimestamp* timestamp,
                            int                       pri,
                            const char*               format,
                            va_list                   args)
{
  return (create_log_str(buf, buflen, timestamp, pri, format, args) != NULL);
}

/**
 * @brief Create a log message with RFC 5424 syslog header in the given buffer.
 *
 * Buffer must be at least #ETCPAL_SYSLOG_STR_MIN_LEN in length.
 *
 * @param[out] buf Buffer in which to build the syslog message.
 * @param[in] buflen Length in bytes of buf.
 * @param[in] timestamp A timestamp representing the current time. If NULL, no timestamp will be
 *                      added to the log message.
 * @param[in] syslog_params A set of parameters for the syslog header.
 * @param[in] pri Priority of this log message.
 * @param[in] format Log message with printf-style format specifiers. Provide additional arguments
 *                   as appropriate for format specifiers.
 */
bool etcpal_create_syslog_str(char*                     buf,
                              size_t                    buflen,
                              const EtcPalLogTimestamp* timestamp,
                              const EtcPalSyslogParams* syslog_params,
                              int                       pri,
                              const char*               format,
                              ...)
{
  va_list args;
  va_start(args, format);
  bool res = (create_syslog_str(buf, buflen, timestamp, syslog_params, pri, format, args) != NULL);
  va_end(args);
  return res;
}

/**
 * @brief Create a log a message with RFC 5424 syslog header in the given buffer.
 *
 * Buffer must be at least #ETCPAL_SYSLOG_STR_MIN_LEN in length. This function is useful if you
 * want to create a wrapper function around etcpal_create_syslog_str() which also takes variable
 * format arguments.
 *
 * @param[out] buf Buffer in which to build the syslog message.
 * @param[in] buflen Length in bytes of buf.
 * @param[in] timestamp A timestamp representing the current time. If NULL, no timestamp will be
 *                      added to the log message.
 * @param[in] syslog_params A set of parameters for the syslog header.
 * @param[in] pri Priority of this log message.
 * @param[in] format Log message with printf-style format specifiers. Provide additional arguments
 *                   as appropriate for format specifiers.
 * @param[in] args Argument list for the format specifiers in format.
 */
bool etcpal_vcreate_syslog_str(char*                     buf,
                               size_t                    buflen,
                               const EtcPalLogTimestamp* timestamp,
                               const EtcPalSyslogParams* syslog_params,
                               int                       pri,
                               const char*               format,
                               va_list                   args)
{
  return (create_syslog_str(buf, buflen, timestamp, syslog_params, pri, format, args) != NULL);
}

/**
 * @brief Create a log message with RFC 3164 syslog header in the given buffer.
 *
 * Buffer must be at least #ETCPAL_SYSLOG_STR_MIN_LEN in length.
 *
 * @param[out] buf Buffer in which to build the syslog message.
 * @param[in] buflen Length in bytes of buf.
 * @param[in] timestamp A timestamp representing the current time. If NULL, no timestamp will be
 *                      added to the log message.
 * @param[in] syslog_params A set of parameters for the syslog header.
 * @param[in] pri Priority of this log message.
 * @param[in] format Log message with printf-style format specifiers. Provide additional arguments
 *                   as appropriate for format specifiers.
 */
bool etcpal_create_legacy_syslog_str(char*                     buf,
                                     size_t                    buflen,
                                     const EtcPalLogTimestamp* timestamp,
                                     const EtcPalSyslogParams* syslog_params,
                                     int                       pri,
                                     const char*               format,
                                     ...)
{
  va_list args;
  va_start(args, format);
  bool res = (create_legacy_syslog_str(buf, buflen, timestamp, syslog_params, pri, format, args) != NULL);
  va_end(args);
  return res;
}

/**
 * @brief Create a log a message with RFC 3164 syslog header in the given buffer.
 *
 * Buffer must be at least #ETCPAL_SYSLOG_STR_MIN_LEN in length. This function is useful if you
 * want to create a wrapper function around etcpal_create_syslog_str() which also takes variable
 * format arguments.
 *
 * @param[out] buf Buffer in which to build the syslog message.
 * @param[in] buflen Length in bytes of buf.
 * @param[in] timestamp A timestamp representing the current time. If NULL, no timestamp will be
 *                      added to the log message.
 * @param[in] syslog_params A set of parameters for the syslog header.
 * @param[in] pri Priority of this log message.
 * @param[in] format Log message with printf-style format specifiers. Provide additional arguments
 *                   as appropriate for format specifiers.
 * @param[in] args Argument list for the format specifiers in format.
 */
bool etcpal_vcreate_legacy_syslog_str(char*                     buf,
                                      size_t                    buflen,
                                      const EtcPalLogTimestamp* timestamp,
                                      const EtcPalSyslogParams* syslog_params,
                                      int                       pri,
                                      const char*               format,
                                      va_list                   args)
{
  return (create_legacy_syslog_str(buf, buflen, timestamp, syslog_params, pri, format, args) != NULL);
}

/**
 * @brief Ensure that the given syslog parameters are compliant with the syslog RFC (modifying them
 *        if necessary).
 *
 * Sanitizes the three string fields (hostname, app_name and procid) by replacing characters that
 * are not allowed by RFC 5424 with filler characters. Also ensures that the facility value is
 * within the correct range (#ETCPAL_LOG_NFACILITIES).
 *
 * @param[in,out] params Syslog params to sanitize.
 */
void etcpal_sanitize_syslog_params(EtcPalSyslogParams* params)
{
  if (ETCPAL_LOG_FAC(params->facility) >= ETCPAL_LOG_NFACILITIES)
    params->facility = DEFAULT_FACILITY;

  sanitize_str(params->hostname);
  sanitize_str(params->app_name);
  sanitize_str(params->procid);
}

/**
 * @brief Ensure that the given EtcPalLogParams are valid.
 *
 * This also sanitizes the syslog params using etcpal_sanitize_syslog_params() if action is set to
 * kEtcPalLogCreateSyslog or kEtcPalLogCreateBoth.
 *
 * @param[in,out] params etcpal_log_params to validate.
 * @return true (params are valid) or false (params are invalid).
 */
bool etcpal_validate_log_params(EtcPalLogParams* params)
{
  if (!params || !params->log_fn)
  {
    return false;
  }

  if (params->action & (ETCPAL_LOG_CREATE_SYSLOG | ETCPAL_LOG_CREATE_LEGACY_SYSLOG))
  {
    etcpal_sanitize_syslog_params(&params->syslog_params);
  }
  return true;
}

/**
 * @brief Determine whether the given EtcPalLogTimestamp is valid.
 *
 * Enforces the range rules defined in the structure documentation.
 *
 * @param[in] timestamp Timestamp to validate.
 * @return Whether the timestamp represents a valid time.
 */
bool etcpal_validate_log_timestamp(const EtcPalLogTimestamp* timestamp)
{
  return (timestamp->year <= 9999 && timestamp->month >= 1 && timestamp->month <= 12 && timestamp->day >= 1 &&
          timestamp->day <= 31 && timestamp->hour <= 23 && timestamp->minute <= 59 && timestamp->second <= 60 &&
          timestamp->msec <= 999);
}

/**
 * @brief Determine whether a priority level can be logged given the mask present in the log
 *        params.
 *
 * This is useful to use as a guard around doing any conversions that are only for logging, e.g.
 * converting addresses to strings -- if the priority of the message can't be logged, then those
 * conversions are just wasted work.
 *
 * @param[in] params The log parameters to be checked.
 * @param[in] pri Priority to check.
 * @return Whether this priority will be logged with the current mask setting.
 */
bool etcpal_can_log(const EtcPalLogParams* params, int pri)
{
  if (params)
    return ((ETCPAL_LOG_MASK(pri) & params->log_mask) != 0);
  return false;
}

/**
 * @brief Log a message.
 *
 * Takes a printf-style format string which is formatted and passed to the application callback.
 *
 * @param[in] params The log parameters to be used for this message.
 * @param[in] pri Priority of this log message.
 * @param[in] format Log message with printf-style format specifiers. Provide additional arguments
 *                   as appropriate for format specifiers.
 */
void etcpal_log(const EtcPalLogParams* params, int pri, const char* format, ...)
{
  va_list args;
  va_start(args, format);
  etcpal_vlog(params, pri, format, args);
  va_end(args);
}

/**
 * @brief Log a message with the list of format arguments already generated.
 *
 * For normal usage, just use etcpal_log(). However, this function is useful if you want to create a
 * wrapper function around etcpal_log() which also takes variable format arguments.
 *
 * @param[in] params The log parameters to be used for this message.
 * @param[in] pri Priority of this log message.
 * @param[in] format Log message with printf-style format specifiers.
 * @param[in] args Argument list for the format specifiers in format.
 */
void etcpal_vlog(const EtcPalLogParams* params, int pri, const char* format, va_list args)
{
  if (!init_count || !params || !params->log_fn || !format || !(ETCPAL_LOG_MASK(pri) & params->log_mask))
    return;

  EtcPalLogTimestamp timestamp;
  bool               have_time = get_time(params, &timestamp);

#if !ETCPAL_NO_OS_SUPPORT
  if (etcpal_mutex_lock(&buf_lock))
  {
#endif
    static char      syslog_msg[ETCPAL_SYSLOG_STR_MAX_LEN + 1];
    static char      legacy_syslog_msg[ETCPAL_SYSLOG_STR_MAX_LEN + 1];
    static char      human_log_msg[ETCPAL_LOG_STR_MAX_LEN + 1];
    EtcPalLogStrings strings = {NULL, NULL, NULL, NULL, 0};
    strings.priority = pri;

    // In the below blocks, we check if the va_list will need to be reused further down - if so,
    // the va_list must be copied. For more info on using a va_list multiple times, see:
    // https://wiki.sei.cmu.edu/confluence/display/c/MSC39-C.+Do+not+call+va_arg%28%29+on+a+va_list+that+has+an+indeterminate+value
    // https://stackoverflow.com/a/26919307
    if (params->action & ETCPAL_LOG_CREATE_HUMAN_READABLE)
    {
      if (params->action & (ETCPAL_LOG_CREATE_SYSLOG | ETCPAL_LOG_CREATE_LEGACY_SYSLOG))
      {
        va_list args_copy;
        va_copy(args_copy, args);
        strings.raw = create_log_str(human_log_msg, ETCPAL_LOG_STR_MAX_LEN + 1, have_time ? &timestamp : NULL, pri,
                                     format, args_copy);
        va_end(args_copy);
      }
      else
      {
        strings.raw =
            create_log_str(human_log_msg, ETCPAL_LOG_STR_MAX_LEN + 1, have_time ? &timestamp : NULL, pri, format, args);
      }
      if (strings.raw)
        strings.human_readable = human_log_msg;
    }

    if (params->action & ETCPAL_LOG_CREATE_SYSLOG)
    {
      if (params->action & ETCPAL_LOG_CREATE_LEGACY_SYSLOG)
      {
        va_list args_copy;
        va_copy(args_copy, args);
        strings.raw = create_syslog_str(syslog_msg, ETCPAL_SYSLOG_STR_MAX_LEN + 1, have_time ? &timestamp : NULL,
                                        &params->syslog_params, pri, format, args_copy);
        va_end(args_copy);
      }
      else
      {
        strings.raw = create_syslog_str(syslog_msg, ETCPAL_SYSLOG_STR_MAX_LEN + 1, have_time ? &timestamp : NULL,
                                        &params->syslog_params, pri, format, args);
      }
      if (strings.raw)
        strings.syslog = syslog_msg;
    }

    if (params->action & ETCPAL_LOG_CREATE_LEGACY_SYSLOG)
    {
      strings.raw = create_legacy_syslog_str(legacy_syslog_msg, ETCPAL_SYSLOG_STR_MAX_LEN + 1,
                                             have_time ? &timestamp : NULL, &params->syslog_params, pri, format, args);
      if (strings.raw)
        strings.legacy_syslog = legacy_syslog_msg;
    }

    params->log_fn(params->context, &strings);
#if !ETCPAL_NO_OS_SUPPORT
    etcpal_mutex_unlock(&buf_lock);
  }
#endif
}

/*
 * Create a log message with a human-readable header given the appropriate va_list. Returns a
 * pointer to the original message within the log message, or NULL on failure.
 */
char* create_log_str(char*                     buf,
                     size_t                    buflen,
                     const EtcPalLogTimestamp* timestamp,
                     int                       pri,
                     const char*               format,
                     va_list                   args)
{
  if (!buf || buflen < ETCPAL_LOG_TIMESTAMP_LEN + 1 || pri < 0 || pri > ETCPAL_LOG_DEBUG || !format)
    return NULL;

  char timestamp_str[ETCPAL_LOG_TIMESTAMP_LEN];
  make_iso_timestamp(timestamp, timestamp_str, true);

  int header_size = 0;
  if (timestamp_str[0] == '\0')
  {
    header_size = snprintf(buf, ETCPAL_LOG_STR_MIN_LEN + 1, "[%s] ", kLogSeverityStrings[pri]);
  }
  else
  {
    header_size = snprintf(buf, ETCPAL_LOG_STR_MIN_LEN + 1, "%s [%s] ", timestamp_str, kLogSeverityStrings[pri]);
  }

  if (header_size >= 0)
  {
    // Copy in the message. vsnprintf will write up to count - 1 bytes and always null-terminates.
    // This allows ETCPAL_LOG_MSG_MAX_LEN valid bytes to be written.
    vsnprintf(&buf[header_size], buflen - (size_t)header_size, format, args);
    return &buf[header_size];
  }

  return NULL;
}

/*
 * Create a log message with the header specified by RFC 5424, given the appropriate va_list.
 * Returns a pointer to the original message within the syslog message, or NULL on failure.
 */
char* create_syslog_str(char*                     buf,
                        size_t                    buflen,
                        const EtcPalLogTimestamp* timestamp,
                        const EtcPalSyslogParams* syslog_params,
                        int                       pri,
                        const char*               format,
                        va_list                   args)
{
  if (!buf || buflen < ETCPAL_SYSLOG_HEADER_MAX_LEN || !syslog_params || !format)
    return NULL;

  char timestamp_str[ETCPAL_LOG_TIMESTAMP_LEN];
  make_iso_timestamp(timestamp, timestamp_str, false);

  int prival = ETCPAL_LOG_PRI(pri) | syslog_params->facility;
  int syslog_header_size =
      snprintf(buf, ETCPAL_SYSLOG_HEADER_MAX_LEN, "<%d>%d %s %s %s %s %s %s ", prival, SYSLOG_PROT_VERSION,
               timestamp_str, syslog_params->hostname[0] ? syslog_params->hostname : NILVALUE_STR,
               syslog_params->app_name[0] ? syslog_params->app_name : NILVALUE_STR,
               syslog_params->procid[0] ? syslog_params->procid : NILVALUE_STR, MSGID_STR, STRUCTURED_DATA_STR);
  if (syslog_header_size >= 0)
  {
    // Copy in the message. vsnprintf will write up to count - 1 bytes and always null-terminates.
    // This allows ETCPAL_LOG_MSG_MAX_LEN valid bytes to be written.
    vsnprintf(&buf[syslog_header_size], buflen - (size_t)syslog_header_size, format, args);
    return &buf[syslog_header_size];
  }

  return NULL;
}

/*
 * Create a log message with the header specified by RFC 3164, given the appropriate va_list.
 * Returns a pointer to the original message within the syslog message, or NULL on failure.
 */
char* create_legacy_syslog_str(char*                     buf,
                               size_t                    buflen,
                               const EtcPalLogTimestamp* timestamp,
                               const EtcPalSyslogParams* syslog_params,
                               int                       pri,
                               const char*               format,
                               va_list                   args)
{
  if (!buf || buflen < ETCPAL_SYSLOG_HEADER_MAX_LEN || !syslog_params || !format)
    return NULL;

  char timestamp_str[ETCPAL_LOG_TIMESTAMP_LEN];
  make_legacy_syslog_timestamp(timestamp, timestamp_str);
  char tag_str[LEGACY_SYSLOG_TAG_MAX_LEN];
  make_legacy_syslog_tag_str(syslog_params, tag_str);

  // Resolve the hostname
  const char* header_format = "<%d>%s%s %s";
  const char* hostname_str = "";
  if (timestamp_str[0] != '\0')
  {
    if (syslog_params->hostname[0] != '\0')
      hostname_str = syslog_params->hostname;
    else
      hostname_str = NILVALUE_STR;
  }
  else  // Hostname cannot be included without timestamp
  {
    header_format = "<%d>%s%s%s";
  }

  int prival = ETCPAL_LOG_PRI(pri) | syslog_params->facility;
  int syslog_header_size =
      snprintf(buf, ETCPAL_SYSLOG_HEADER_MAX_LEN, header_format, prival, timestamp_str, hostname_str, tag_str);
  if (syslog_header_size >= 0)
  {
    // Copy in the message. vsnprintf will write up to count - 1 bytes and always null-terminates.
    // This allows ETCPAL_LOG_MSG_MAX_LEN valid bytes to be written.
    vsnprintf(&buf[syslog_header_size], buflen - (size_t)syslog_header_size, format, args);
    return &buf[syslog_header_size];
  }

  return NULL;
}

/* Replace non-printing characters and spaces with '_'. Replace characters above 127 with '?'. */
void sanitize_str(char* str)
{
  // C library functions like isprint()/isgraph() are not used here because their behavior is not
  // well-defined in the presence of non-ASCII characters.
  for (unsigned char* cp = (unsigned char*)str; *cp != '\0'; ++cp)
  {
    if (*cp < 33 || *cp == 127)
      *cp = '_';
    else if (*cp > 127)
      *cp = '?';
  }
}

/* Build the current timestamp. Buffer must be of length ETCPAL_LOG_TIMESTAMP_LEN. */
void make_iso_timestamp(const EtcPalLogTimestamp* timestamp, char* buf, bool human_readable)
{
  bool timestamp_created = false;

  if (timestamp && etcpal_validate_log_timestamp(timestamp))
  {
    // Print the basic timestamp
    int print_res = snprintf(
        buf, ETCPAL_LOG_TIMESTAMP_LEN,
        human_readable ? "%04u-%02u-%02u %02u:%02u:%02u.%03u" : "%04u-%02u-%02uT%02u:%02u:%02u.%03u", timestamp->year,
        timestamp->month, timestamp->day, timestamp->hour, timestamp->minute, timestamp->second, timestamp->msec);

    if (print_res > 0 && print_res < ETCPAL_LOG_TIMESTAMP_LEN - 1)
    {
      // Add the UTC offset
      if (timestamp->utc_offset == 0)
      {
        buf[print_res] = 'Z';
        buf[print_res + 1] = '\0';
      }
      else
      {
        snprintf(&buf[print_res], ETCPAL_LOG_TIMESTAMP_LEN - (size_t)print_res, "%s%02d:%02d",
                 timestamp->utc_offset > 0 ? "+" : "-", abs(timestamp->utc_offset) / 60,
                 abs(timestamp->utc_offset) % 60);
      }
      timestamp_created = true;
    }
  }

  if (!timestamp_created)
  {
    if (!human_readable)
      strcpy(buf, NILVALUE_STR);
    else
      buf[0] = '\0';
  }
}

void make_legacy_syslog_timestamp(const EtcPalLogTimestamp* timestamp, char* buf)
{
  bool timestamp_created = false;

  if (timestamp && etcpal_validate_log_timestamp(timestamp))
  {
    // Print the basic timestamp
    int print_res = snprintf(buf, ETCPAL_LOG_TIMESTAMP_LEN, "%s %2d %02u:%02u:%02u ", kMonthNames[timestamp->month - 1],
                             timestamp->day, timestamp->hour, timestamp->minute, timestamp->second);

    if (print_res > 0 && print_res < ETCPAL_LOG_TIMESTAMP_LEN - 1)
    {
      timestamp_created = true;
    }
  }

  if (!timestamp_created)
    buf[0] = '\0';
}

void make_legacy_syslog_tag_str(const EtcPalSyslogParams* syslog_params, char* buf)
{
  int tag_str_len = 0;
  if (syslog_params->app_name[0] != '\0' && syslog_params->procid[0] != '\0')
  {
    tag_str_len = snprintf(buf, LEGACY_SYSLOG_TAG_MAX_LEN, "%s[%s]: ", syslog_params->app_name, syslog_params->procid);
  }
  else if (syslog_params->app_name[0] != '\0')
  {
    tag_str_len = snprintf(buf, LEGACY_SYSLOG_TAG_MAX_LEN, "%s: ", syslog_params->app_name);
  }
  else if (syslog_params->procid[0] != '\0')
  {
    tag_str_len = snprintf(buf, LEGACY_SYSLOG_TAG_MAX_LEN, "%s: ", syslog_params->procid);
  }
  else
    buf[0] = '\0';

  // Intelligent truncation
  if (tag_str_len > LEGACY_SYSLOG_TAG_MAX_LEN - 1)
  {
    buf[LEGACY_SYSLOG_TAG_MAX_LEN - 3] = ':';
    buf[LEGACY_SYSLOG_TAG_MAX_LEN - 2] = ' ';
    buf[LEGACY_SYSLOG_TAG_MAX_LEN - 1] = '\0';
  }
}

/* Attempt to get the current time via a time callback. */
bool get_time(const EtcPalLogParams* params, EtcPalLogTimestamp* timestamp)
{
  if (params->time_fn)
  {
    memset(timestamp, 0, sizeof(EtcPalLogTimestamp));
    params->time_fn(params->context, timestamp);
    return true;
  }

  return false;
}
