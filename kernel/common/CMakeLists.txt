# full common library for sedna

add_library(common STATIC)
add_dependencies(common sedna::expat) # find/build expat before common
add_dependencies(common generate_error_codes) # build error codes before common (devenv parallel build freaks out without this)

target_sources( common
      PRIVATE
		argtable.c
		base.cpp
		bit_set.cpp
		gmm.cpp
		ipc_ops.cpp
		pping.cpp
		rcv_test.cpp
		sedna.c
		sp.c
		SSMMsg.cpp
		tr_debug.cpp
		ugc.cpp
		utils.cpp
		version.c
		xptr.cpp
)

# ToDo: two lines below are temporary fix of CMake issue, probably should delete them in 3.20
# see https://gitlab.kitware.com/cmake/cmake/-/issues/18399
set_property(SOURCE ${CMAKE_CURRENT_BINARY_DIR}/errdbg/error_codes.c PROPERTY GENERATED 1)
set_property(SOURCE ${CMAKE_CURRENT_BINARY_DIR}/errdbg/error_codes.h PROPERTY GENERATED 1)

add_subdirectory(errdbg)
add_subdirectory(llcommon)
add_subdirectory(mmgr)
add_subdirectory(st)
add_subdirectory(u)



