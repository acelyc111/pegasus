set(MY_PROJ_NAME dsn_utils_tests)

# Search mode for source files under CURRENT project directory?
# "GLOB_RECURSE" for recursive search
# "GLOB" for non-recursive search
set(MY_SRC_SEARCH_MODE "GLOB")

set(MY_PROJ_LIBS dsn_runtime
                 dsn_utils
                 gtest
                 )

set(MY_BOOST_LIBS Boost::system Boost::filesystem Boost::regex)

# Extra files that will be installed
set(MY_BINPLACES "config-bad-section.ini"
        "config-dup-key.ini"
        "config-dup-section.ini"
        "config-empty.ini"
        "config-no-key.ini"
        "config-no-section.ini"
        "config-null-section.ini"
        "config-sample.ini"
        "config-unmatch-section.ini"
        "run.sh"
        "clear.sh"
        )
add_subdirectory(nth_element_bench)
add_definitions(-Wno-dangling-else)
dsn_add_test()
