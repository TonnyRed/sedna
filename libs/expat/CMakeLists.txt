include(FetchContent)
FetchContent_Declare(custom_expat
        URL ${CMAKE_CURRENT_SOURCE_DIR}/libexpat-R_2_2_10.tar.gz
        #		             SOURCE_SUBDIR expat
        #        URL https://github.com/libexpat/libexpat/archive/R_2_2_10.tar.gz
        )

FetchContent_GetProperties(custom_expat)
if(NOT custom_expat_POPULATED)
    FetchContent_Populate(custom_expat)
    message(STATUS "custom_expat_SOURCE_DIR: ${custom_expat_SOURCE_DIR}  custom_expat_BINARY_DIR: ${custom_expat_BINARY_DIR}")
    # set our options for expat
    set (EXPAT_BUILD_TOOLS OFF CACHE INTERNAL "build the xmlwf tool for expat library")
    set(EXPAT_BUILD_EXAMPLES OFF CACHE INTERNAL "build the examples for expat library")
    set(EXPAT_BUILD_TESTS OFF CACHE INTERNAL "build the tests for expat library")
    set(EXPAT_SHARED_LIBS OFF CACHE INTERNAL "build a shared expat library")
    set(EXPAT_BUILD_DOCS OFF CACHE INTERNAL "build man page for xmlwf")
    set(EXPAT_BUILD_FUZZERS OFF CACHE INTERNAL "build fuzzers for the expat library")
    set(EXPAT_BUILD_PKGCONFIG OFF CACHE INTERNAL "build pkg-config file")
    set(EXPAT_OSSFUZZ_BUILD OFF CACHE INTERNAL "build fuzzers via ossfuzz for the expat library")

    set(EXPAT_ENABLE_INSTALL OFF CACHE INTERNAL "install expat files in cmake install target")
    set(EXPAT_LARGE_SIZE ON CACHE INTERNAL "Make XML_GetCurrent* functions return <(unsigned) long long> rather than <(unsigned) long>")

    add_subdirectory(${custom_expat_SOURCE_DIR}/expat ${custom_expat_BINARY_DIR} EXCLUDE_FROM_ALL )
endif()

add_library(sedna::expat ALIAS expat)