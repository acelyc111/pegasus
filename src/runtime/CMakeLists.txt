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

set(MY_PROJ_NAME dsn_runtime)

# TODO(zlw) remove perf_counter from dsn_runtime after the refactor by WuTao
set(MY_PROJ_SRC
        $<TARGET_OBJECTS:dsn_ranger>
        $<TARGET_OBJECTS:dsn.security>
        $<TARGET_OBJECTS:dsn.rpc>
        $<TARGET_OBJECTS:dsn.task>
        $<TARGET_OBJECTS:dsn.perf_counter>
        core_main.cpp
        dsn.layer2_types.cpp
        env.sim.cpp
        fault_injector.cpp
        global_config.cpp
        message_utils.cpp
        nativerun.cpp
        profiler.cpp
        providers.common.cpp
        scheduler.cpp
        service_api_c.cpp
        service_engine.cpp
        simulator.cpp
        threadpool_code.cpp
        tool_api.cpp
        tracer.cpp
        zlocks.cpp)

set(MY_PROJ_LIBS
        dsn_meta_server
        dsn_utils
        sasl2
        gssapi_krb5
        krb5)

dsn_add_static_library()

add_subdirectory(rpc)
add_subdirectory(task)
add_subdirectory(security)
add_subdirectory(ranger)
add_subdirectory(test)
