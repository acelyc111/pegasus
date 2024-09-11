/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Microsoft Corporation
 *
 * -=- Robust Distributed System Nucleus (rDSN) -=-
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <algorithm>
#include <functional>
#include <iomanip>
#include <memory>
#include <string>
#include <utility>

#include "runtime/tool_api.h"
#include "utils/api_utilities.h"
#include "utils/factory_store.h"
#include "utils/flags.h"
#include "utils/fmt_logging.h"
#include "utils/join_point.h"
#include "utils/sys_exit_hook.h"

enum log_level_t
{
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL,
    LOG_LEVEL_COUNT,
    LOG_LEVEL_INVALID
};

ENUM_BEGIN(log_level_t, LOG_LEVEL_INVALID)
ENUM_REG(LOG_LEVEL_DEBUG)
ENUM_REG(LOG_LEVEL_INFO)
ENUM_REG(LOG_LEVEL_WARNING)
ENUM_REG(LOG_LEVEL_ERROR)
ENUM_REG(LOG_LEVEL_FATAL)
ENUM_END(log_level_t)

DSN_DEFINE_string(core,
                  logging_start_level,
                  "LOG_LEVEL_INFO",
                  "Logs with level larger than or equal to this level be logged");
DSN_DEFINE_validator(logging_start_level, [](const char *value) -> bool {
    const auto level = enum_from_string(value, LOG_LEVEL_INVALID);
    return LOG_LEVEL_DEBUG <= level && level <= LOG_LEVEL_FATAL;
});

DSN_DEFINE_string(
    tools.simple_logger,
    stderr_start_level,
    "LOG_LEVEL_WARNING",
    "The lowest level of log messages to be copied to stderr in addition to log files");
DSN_DEFINE_validator(stderr_start_level, [](const char *value) -> bool {
    const auto level = enum_from_string(value, LOG_LEVEL_INVALID);
    return LOG_LEVEL_DEBUG <= level && level <= LOG_LEVEL_FATAL;
});

DSN_DEFINE_bool(tools.simple_logger,
                short_header,
                false,
                "Whether to use short header (excluding "
                "file, file number and function name "
                "fields in each line)");

static std::map<log_level_t, google::LogSeverity> to_google_levels = {
    {LOG_LEVEL_DEBUG, google::INFO},
    {LOG_LEVEL_INFO, google::INFO},
    {LOG_LEVEL_WARNING, google::WARNING},
    {LOG_LEVEL_ERROR, google::ERROR},
    {LOG_LEVEL_FATAL, google::FATAL}};

DSN_DEFINE_bool(tools.simple_logger, fast_flush, false, "Whether to flush logs immediately");
DSN_DEFINE_uint64(tools.simple_logger,
                  max_log_file_bytes,
                  64 * 1024 * 1024,
                  "The maximum bytes of a log file. A new log file will be created if the current "
                  "log file exceeds this size.");
// max_number_of_log_files_on_disk is removed.
DSN_DEFINE_uint64(tools.simple_logger,
                  log_file_duration_minutes,
                  1440,
                  "The duration of log files will be kept on disk, in minutes. The overdue log "
                  "files will be removed.");
DSN_DEFINE_validator(max_log_file_bytes,
                     [](uint64_t value) -> bool { return value > (1UL << 20UL); });
DSN_DEFINE_bool(core,
                logging_flush_on_exit,
                true,
                "Whether to flush the logs when the process exits");

std::string log_prefixed_message_func()
{
    const static thread_local int tid = dsn::utils::get_current_tid();
    const auto t = dsn::task::get_current_task_id();
    if (t != 0) {
        if (nullptr != dsn::task::get_current_worker2()) {
            return fmt::format("{}.{}{}.{:016}",
                               dsn::task::get_current_node_name(),
                               dsn::task::get_current_worker2()->pool_spec().name,
                               dsn::task::get_current_worker2()->index(),
                               t);
        }
        return fmt::format("{}.io-thrd.{}.{:016}", dsn::task::get_current_node_name(), tid, t);
    }

    if (nullptr != dsn::task::get_current_worker2()) {
        const static thread_local std::string prefix =
            fmt::format("{}.{}{}",
                        dsn::task::get_current_node_name(),
                        dsn::task::get_current_worker2()->pool_spec().name,
                        dsn::task::get_current_worker2()->index());
        return prefix;
    }
    const static thread_local std::string prefix =
        fmt::format("{}.io-thrd.{}", dsn::task::get_current_node_name(), tid);
    return prefix;
}

static void log_on_sys_exit(::dsn::sys_exit_type) { google::FlushLogFiles(google::INFO); }

// TODO(yingchun): function name is missing!
void LogPrefixFormatter(std::ostream &s, const google::LogMessage &m, void * /*data*/)
{
    s << google::GetLogSeverityName(m.severity())[0] // 'I', 'W', 'E' or 'F'.
      << std::setw(4) << 1900 + m.time().year() << '-' << std::setw(2) << 1 + m.time().month()
      << '-' << std::setw(2) << m.time().day() << ' ' << std::setw(2) << m.time().hour() << ':'
      << std::setw(2) << m.time().min() << ':' << std::setw(2) << m.time().sec() << '.'
      << std::setw(6) << m.time().usec() << ' ' << log_prefixed_message_func() << ' '
      << m.basename() << ':' << m.line() << "]";
}

void ShortLogPrefixFormatter(std::ostream &s, const google::LogMessage &m, void * /*data*/)
{
    s << google::GetLogSeverityName(m.severity())[0] // 'I', 'W', 'E' or 'F'.
      << std::setw(4) << 1900 + m.time().year() << '-' << std::setw(2) << 1 + m.time().month()
      << '-' << std::setw(2) << m.time().day() << ' ' << std::setw(2) << m.time().hour() << ':'
      << std::setw(2) << m.time().min() << ':' << std::setw(2) << m.time().sec() << '.'
      << std::setw(6) << m.time().usec() << ' ' << log_prefixed_message_func() << "]";
}

void dsn_log_init(const std::string &log_dir, const std::string &role_name)
{
    ::dsn::command_manager::instance().add_global_cmd(
        ::dsn::command_manager::instance().register_single_command(
            "flush-log",
            "Flush log to stderr or file",
            "",
            [](const std::vector<std::string> & /*args*/) {
                google::FlushLogFiles(google::INFO);
                return "Flush done.";
            }));

    ::dsn::command_manager::instance().add_global_cmd(
        ::dsn::command_manager::instance().register_single_command(
            "reset-log-start-level",
            "Reset the log start level",
            "[DEBUG | INFO | WARNING | ERROR | FATAL]",
            [](const std::vector<std::string> &args) {
                log_level_t start_level = LOG_LEVEL_INVALID;
                if (args.empty()) {
                    start_level = enum_from_string(FLAGS_logging_start_level, LOG_LEVEL_INVALID);
                } else {
                    std::string level_str = "LOG_LEVEL_" + args[0];
                    start_level = enum_from_string(level_str.c_str(), LOG_LEVEL_INVALID);
                    if (start_level == LOG_LEVEL_INVALID) {
                        return "ERROR: invalid level '" + args[0] + "'";
                    }
                }
                FLAGS_minloglevel = to_google_levels[start_level];
                return std::string("OK, current level is ") + enum_to_string(start_level);
            }));

    FLAGS_minloglevel =
        to_google_levels[enum_from_string(FLAGS_logging_start_level, LOG_LEVEL_INVALID)];
    FLAGS_stderrthreshold =
        to_google_levels[enum_from_string(FLAGS_stderr_start_level, LOG_LEVEL_INVALID)];
    FLAGS_logbuflevel = FLAGS_fast_flush ? -1 : 0;
    FLAGS_max_log_size = FLAGS_max_log_file_bytes >> 20;
    FLAGS_stop_logging_if_full_disk = true;

    google::InstallFailureSignalHandler();
    // TODO(yingchun): extend LogCleaner to support clean log files by count.
    google::EnableLogCleaner(std::chrono::minutes(FLAGS_log_file_duration_minutes));
    static const std::vector<google::LogSeverity> severities = {
        google::INFO, google::WARNING, google::ERROR, google::FATAL};
    for (const auto &severity : severities) {
        google::SetLogSymlink(severity, fmt::format("{}/{}", log_dir, role_name).c_str());
        google::SetLogDestination(severity, fmt::format("{}/{}.", log_dir, role_name).c_str());
    }
    google::SetLogFilenameExtension(".log");
    google::InitGoogleLogging(role_name.c_str());
    if (FLAGS_short_header) {
        google::InstallPrefixFormatter(&ShortLogPrefixFormatter, nullptr);
    } else {
        google::InstallPrefixFormatter(&LogPrefixFormatter, nullptr);
    }

    // register log flush on exit
    if (FLAGS_logging_flush_on_exit) {
        ::dsn::tools::sys_exit.put_back(log_on_sys_exit, "log.flush");
    }
}
