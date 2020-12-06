# pcre library
add_subdirectory(pcre)

# expat library
add_subdirectory(expat)



# libedit "external" library
# we use internal cached variables to determine directories for -I and -L
#set(LIBEDIT_VER "20100424-3.0")
set(LIBEDIT_VER "20191231-3.1")
message(STATUS "Will build libedit from scratch when building the project, if needed...")

set(LIBEDIT_TAR_DIR ${CMAKE_BINARY_DIR}/libs/libedit-${LIBEDIT_VER})
set(libedit_files ${LIBEDIT_TAR_DIR}/include/editline/readline.h ${LIBEDIT_TAR_DIR}/lib/libedit${CMAKE_STATIC_LIBRARY_SUFFIX})

set(CUSTOM_LIBEDIT_INCLUDE ${LIBEDIT_TAR_DIR}/include/editline CACHE INTERNAL "Internal libedit include path")
set(CUSTOM_LIBEDIT_LIB ${LIBEDIT_TAR_DIR}/lib CACHE INTERNAL "Internal libedit library path")

if (UNIX)
    find_program(PATCH_COMMAND patch)
    if (NOT PATCH_COMMAND)
        message(STATUS "Cannot find patch -- it'll not be possible to build libedit")
    endif (NOT PATCH_COMMAND)
endif (UNIX)


add_custom_command(OUTPUT ${libedit_files}
                   COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_SOURCE_DIR}/libs/libedit-${LIBEDIT_VER}.tar
                   COMMAND ${CMAKE_COMMAND} -E chdir ${LIBEDIT_TAR_DIR} sh configure --enable-shared=no --prefix=${LIBEDIT_TAR_DIR}
                   COMMAND ${CMAKE_COMMAND} -E chdir ${LIBEDIT_TAR_DIR} make install
                   WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                   VERBATIM
                  )

add_custom_target(custom_libedit DEPENDS ${libedit_files})

MESSAGE(STATUS "Custom libedit includes in -- ${CUSTOM_LIBEDIT_INCLUDE}")
MESSAGE(STATUS "Custom libedit library in  -- ${CUSTOM_LIBEDIT_LIB}")
