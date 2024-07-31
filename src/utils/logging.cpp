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
#include "spdlog/common.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_sinks.h"
#include "utils/command_manager.h"

DSN_DEFINE_string(core,
                  logging_start_level,
                  "LOG_LEVEL_INFO",
                  "Logs with level larger than or equal to this level be logged");
DSN_DEFINE_validator(logging_start_level, [](const char *value) -> bool {
    const auto level = enum_from_string(value, LOG_LEVEL_INVALID);
    return LOG_LEVEL_DEBUG <= level && level <= LOG_LEVEL_FATAL;
});

DSN_DEFINE_bool(core,
                logging_flush_on_exit,
                true,
                "Whether to flush the logs when the process exits");

DSN_DEFINE_bool(tools.simple_logger,
                short_header,
                false,
                "Whether to use short header (excluding "
                "file, file number and function name "
                "fields in each line)");

DSN_DEFINE_bool(tools.simple_logger, fast_flush, false, "Whether to flush logs every second");

DSN_DEFINE_uint64(tools.simple_logger,
                  max_log_file_bytes,
                  64 * 1024 * 1024,
                  "The maximum bytes of a log file. A new log file will be created if the current "
                  "log file exceeds this size.");
DSN_DEFINE_validator(max_log_file_bytes, [](int32_t value) -> bool { return value > 0; });

DSN_DEFINE_uint64(
    tools.simple_logger,
    max_number_of_log_files_on_disk,
    20,
    "The maximum number of log files to be reserved on disk, older logs are deleted automatically");
DSN_DEFINE_validator(max_number_of_log_files_on_disk,
                     [](int32_t value) -> bool { return value > 0; });

DSN_DEFINE_string(tools.simple_logger, base_name, "pegasus", "The default base name for log file");

DSN_DEFINE_string(
    tools.simple_logger,
    stderr_start_level,
    "LOG_LEVEL_WARNING",
    "The lowest level of log messages to be copied to stderr in addition to log files");
DSN_DEFINE_validator(stderr_start_level, [](const char *value) -> bool {
    const auto level = enum_from_string(value, LOG_LEVEL_INVALID);
    return LOG_LEVEL_DEBUG <= level && level <= LOG_LEVEL_FATAL;
});

std::shared_ptr<spdlog::logger> g_stderr_logger = spdlog::stderr_logger_mt("stderr");
std::shared_ptr<spdlog::logger> g_file_logger;

static std::map<log_level_t, spdlog::level::level_enum> to_spdlog_levels = {
    {LOG_LEVEL_DEBUG, spdlog::level::debug},
    {LOG_LEVEL_INFO, spdlog::level::info},
    {LOG_LEVEL_WARNING, spdlog::level::warn},
    {LOG_LEVEL_ERROR, spdlog::level::err},
    {LOG_LEVEL_FATAL, spdlog::level::critical}};
static std::vector<std::unique_ptr<dsn::command_deregister>> log_cmds;

spdlog::level::level_enum g_stderr_start_level;
spdlog::level::level_enum g_file_log_start_level;

void dsn_log_init(const std::string &log_dir, const std::string &role_name)
{
    g_file_logger = spdlog::rotating_logger_mt(
        "file",
        fmt::format("{}/{}.log", log_dir, role_name.empty() ? FLAGS_base_name : role_name),
        FLAGS_max_log_file_bytes,
        FLAGS_max_number_of_log_files_on_disk);
    //    _symlink_path = utils::filesystem::path_combine(_log_dir, symlink_name);
    const auto spdlog_start_level =
        to_spdlog_levels[enum_from_string(FLAGS_logging_start_level, LOG_LEVEL_INVALID)];
    g_file_log_start_level = spdlog_start_level;
    g_file_logger->set_level(spdlog_start_level);
    g_stderr_start_level =
        to_spdlog_levels[enum_from_string(FLAGS_stderr_start_level, LOG_LEVEL_INVALID)];
    g_file_logger->flush_on(spdlog::level::err);
    g_stderr_logger->flush_on(spdlog::level::err);
    if (FLAGS_fast_flush) {
        spdlog::flush_every(std::chrono::seconds(1));
    }

    // register log flush on exit
    if (FLAGS_logging_flush_on_exit) {
        //        ::dsn::tools::sys_exit.put_back(log_on_sys_exit, "log.flush");
    }

    // See: https://github.com/gabime/spdlog/wiki/3.-Custom-formatting#pattern-flags
    auto formatter = std::make_unique<spdlog::pattern_formatter>();
    if (FLAGS_short_header) {
        static const std::string kPattern = "%L%Y-%m-%d %H:%M:%S.%e %t %* %v";
        formatter->add_flag<pegasus_formatter_flag>('*').set_pattern(kPattern);
    } else {
        static const std::string kPattern = "%L%Y-%m-%d %H:%M:%S.%e %t %* %s:%#:%!(): %v";
        formatter->add_flag<pegasus_formatter_flag>('*').set_pattern(kPattern);
    }
    spdlog::set_formatter(std::move(formatter));

    // Flush when exit
    //    ::fflush(_log);
    //    ::fflush(stderr);
    //    ::fflush(stdout);
    // TODO(yingchun): simple_logger is destroyed after command_manager, so will cause crash like
    //  "assertion expression: [_handlers.empty()] All commands must be deregistered before
    //  command_manager is destroyed, however 'flush-log' is still registered".
    //  We need to fix it.
    log_cmds.emplace_back(::dsn::command_manager::instance().register_single_command(
        "flush-log", "Flush log to stderr or file", "", [](const std::vector<std::string> &args) {
            // TODO(yingchun): not support now!
            return "Flush done.";
        }));

    log_cmds.emplace_back(::dsn::command_manager::instance().register_single_command(
        "reset-log-start-level",
        "Reset the log start level",
        "[DEBUG | INFO | WARNING | ERROR | FATAL]",
        [](const std::vector<std::string> &args) {
            log_level_t start_level;
            if (args.empty()) {
                start_level = enum_from_string(FLAGS_logging_start_level, LOG_LEVEL_INVALID);
            } else {
                std::string level_str = "LOG_LEVEL_" + args[0];
                start_level = enum_from_string(level_str.c_str(), LOG_LEVEL_INVALID);
                if (start_level == LOG_LEVEL_INVALID) {
                    return "ERROR: invalid level '" + args[0] + "'";
                }
            }
            g_file_logger->set_level(to_spdlog_levels[start_level]);
            return std::string("OK, current level is ") + enum_to_string(start_level);
        }));
}

std::string log_prefixed_message_func()
{
    const static thread_local int tid = dsn::utils::get_current_tid();
    const auto t = dsn::task::get_current_task_id();
    if (t) {
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
