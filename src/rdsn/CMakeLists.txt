add_definitions(-DGTEST_HAS_TR1_TUPLE=0 -DGTEST_USE_OWN_TR1_TUPLE=0)

# Users don't have to configure CMAKE_INSTALL_PREFIX unless they want to customize
# the destination.
set(CMAKE_INSTALL_PREFIX ${DSN_ROOT} CACHE STRING "" FORCE)
message (STATUS "dsn Installation directory: CMAKE_INSTALL_PREFIX = " ${CMAKE_INSTALL_PREFIX})

# install rdsn/include to ${CMAKE_INCLUDE_PREFIX}/include
install(DIRECTORY include DESTINATION include)

# NOTE: dsn_types.h is not actually a thrift-generated file. As pegasus may reference it,
# we should copy also this file to the path the same as other thrift-generated files.
install(FILES include/dsn/cpp/serialization_helper/dsn_types.h DESTINATION include)
# TODO(wutao1): remove this line after dsn.layer2_types.h is removed from repo.
install(FILES include/dsn/cpp/serialization_helper/dsn.layer2_types.h DESTINATION include)

include_directories(${DSN_PROJECT_DIR}/include)
include_directories(${DSN_PROJECT_DIR}/include/dsn/cpp/serialization_helper)
include_directories(${DSN_PROJECT_DIR}/src)

add_subdirectory(src/runtime)
add_subdirectory(src/aio)
add_subdirectory(src/zookeeper)
add_subdirectory(src/perf_counter)
add_subdirectory(src/failure_detector)
add_subdirectory(src/remote_cmd)
add_subdirectory(src/nfs)
add_subdirectory(src/block_service)
add_subdirectory(src/http)
add_subdirectory(src/client)
add_subdirectory(src/common)
add_subdirectory(src/replica)
add_subdirectory(src/meta)
add_subdirectory(src/tools)
add_subdirectory(src/utils)
