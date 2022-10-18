#!/bin/sh
# The MIT License (MIT)
#
# Copyright (c) 2015 Microsoft Corporation
#
# -=- Robust Distributed System Nucleus (rDSN) -=-
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.


if [ -z "${REPORT_DIR}" ]; then
    REPORT_DIR="."
fi

./clear.sh
output_xml="${REPORT_DIR}/dsn.meta.test.1.xml"
GTEST_OUTPUT="xml:${output_xml}" ./dsn.meta.test --gtest_filter=meta_bulk_load_http_test.start_compaction_test
if [ $? -ne 0 ]; then
    echo "run dsn.meta.test failed"
    echo "---- ls ----"
    ls -l
    echo "---- ls data ----"
    ls -l data
    echo "---- ls data/log ----"
    ls -l data/log
    if find . -name log.1.txt; then
        echo "---- tail -n 100 log.1.txt ----"
        tail -n 100 `find . -name log.1.txt`
    fi
    if [ -f core ]; then
        echo "---- gdb ./dsn.meta.test core ----"
        gdb ./dsn.meta.test core -ex "thread apply all bt" -ex "set pagination 0" -batch
    fi
    exit 1
fi
