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
#include "simple_logger.h"
#include "utils/api_utilities.h"
#include "utils/factory_store.h"
#include "utils/flags.h"
#include "utils/fmt_logging.h"
#include "utils/join_point.h"
#include "utils/sys_exit_hook.h"

// glog: minloglevel (int, default=0, which is INFO)
// Log messages at or above this level. Again, the numbers of severity levels INFO, WARNING, ERROR,
// and FATAL are 0, 1, 2, and 3, respectively. glog: v (int, default=0) Show all VLOG(m) messages
// for m less or equal the value of this flag. Overridable by --vmodule. Refer to verbose logging
// for more detail. glog: vmodule (string, default="") Per-module verbose level. The argument has to
// contain a comma-separated list of <module name>=<log level>. <module name> is a glob pattern
// (e.g., gfs* for all modules whose name starts with "gfs"), matched against the filename base
// (that is, name ignoring .cc/.h./-inl.h). <log level> overrides any value given by --v. See also
// verbose logging for more details.
DSN_DEFINE_string(core,
                  logging_start_level,
                  "LOG_LEVEL_INFO",
                  "Logs with level larger than or equal to this level be logged");

DSN_DEFINE_bool(core,
                logging_flush_on_exit,
                true,
                "Whether to flush the logs when the process exits");

log_level_t log_start_level = LOG_LEVEL_INFO;

namespace dsn {

using namespace tools;

std::function<std::string()> log_prefixed_message_func = []() -> std::string { return ": "; };

void set_log_prefixed_message_func(std::function<std::string()> func)
{
    log_prefixed_message_func = std::move(func);
}
} // namespace dsn

static void log_on_sys_exit(::dsn::sys_exit_type) {}

void LogPrefixFormatter(std::ostream &s, const google::LogMessage &m, void * /*data*/)
{
    s << google::GetLogSeverityName(m.severity())[0] // 'I', 'W', 'E' or 'F'.
      << std::setw(4) << 1900 + m.time().year() << '-' << std::setw(2) << 1 + m.time().month()
      << '-' << std::setw(2) << m.time().day() << ' ' << std::setw(2) << m.time().hour() << ':'
      << std::setw(2) << m.time().min() << ':' << std::setw(2) << m.time().sec() << '.'
      << std::setw(3) << m.time().usec() << ' ' << std::setfill(' ') << std::setw(5)
      << m.thread_id() << std::setfill('0') << ' ' << m.basename() << ':' << m.line()
      << dsn::log_prefixed_message_func();
}

void dsn_log_init(const std::string &log_dir,
                  const std::string &role_name,
                  const std::function<std::string()> &dsn_log_prefixed_message_func)
{
    google::InstallPrefixFormatter(&LogPrefixFormatter, nullptr);

    log_start_level = enum_from_string(FLAGS_logging_start_level, LOG_LEVEL_INVALID);

    CHECK_NE_MSG(
        log_start_level, LOG_LEVEL_INVALID, "invalid [core] logging_start_level specified");

    // register log flush on exit
    if (FLAGS_logging_flush_on_exit) {
        ::dsn::tools::sys_exit.put_back(log_on_sys_exit, "log.flush");
    }

    if (dsn_log_prefixed_message_func != nullptr) {
        dsn::set_log_prefixed_message_func(dsn_log_prefixed_message_func);
    }
}

log_level_t get_log_start_level() { return log_start_level; }

void set_log_start_level(log_level_t level) { log_start_level = level; }

void global_log(
    const char *file, const char *function, const int line, log_level_t log_level, const char *str)
{
}
