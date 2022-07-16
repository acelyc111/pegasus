set(MY_PROJ_NAME dsn_nfs_test)

# Source files under CURRENT project directory will be automatically included.
# You can manually set MY_PROJ_SRC to include source files under other directories.
set(MY_PROJ_SRC "")

# Search mode for source files under CURRENT project directory?
# "GLOB_RECURSE" for recursive search
# "GLOB" for non-recursive search
set(MY_SRC_SEARCH_MODE "GLOB")

set(MY_PROJ_LIBS dsn_nfs dsn_runtime gtest dsn_aio)

set(MY_BOOST_LIBS Boost::system Boost::filesystem Boost::regex)

# Extra files that will be installed
set(MY_BINPLACES
    "nfs_test_file1"
    "nfs_test_file2"
    "config.ini"
    "clear.sh"
    "run.sh"
)

dsn_add_test()
