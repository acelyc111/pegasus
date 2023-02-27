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

// some useful utility functions provided by rDSN,
// such as logging, performance counter, checksum,
// command line interface registration and invocation,
// etc.

#pragma once

#include <stdarg.h>

#include "ports.h"

/*!
@defgroup logging Logging Service
@ingroup service-api-utilities

 Logging Service

 Note developers can plug into rDSN their own logging libraryS
 implementation, so as to integrate rDSN logs into
 their own cluster operation systems.
@{
*/

typedef enum dsn_log_level_t {
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL,
    LOG_LEVEL_COUNT,
    LOG_LEVEL_INVALID
} dsn_log_level_t;

// logs with level smaller than this start_level will not be logged
extern dsn_log_level_t dsn_log_start_level;
extern dsn_log_level_t dsn_log_get_start_level();
extern void dsn_log_set_start_level(dsn_log_level_t level);
extern void dsn_logv(const char *file,
                     const char *function,
                     const int line,
                     dsn_log_level_t log_level,
                     const char *fmt,
                     va_list args);
extern void dsn_logf(const char *file,
                     const char *function,
                     const int line,
                     dsn_log_level_t log_level,
                     const char *fmt,
                     ...);
extern void dsn_log(const char *file,
                    const char *function,
                    const int line,
                    dsn_log_level_t log_level,
                    const char *str);
extern void dsn_coredump();

#ifdef DSN_MOCK_TEST
#define mock_private public
#define mock_virtual virtual
#else
#define mock_private private
#define mock_virtual
#endif

#define RETURN_FALSE(exp)                                                                          \
    if (dsn_unlikely(!(exp)))                                                                      \
    return false

#define RETURN_FALSE_ON_EXCEPTION(exp)                                                             \
    do {                                                                                           \
        try {                                                                                      \
            exp;                                                                                   \
        } catch (...) {                                                                            \
            return false;                                                                          \
        }                                                                                          \
    } while (0)
/*@}*/
