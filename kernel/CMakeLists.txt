add_subdirectory(common)

#Interface library to build binaries
add_library(common_libs INTERFACE)
target_link_libraries(common_libs INTERFACE common sedna::expat ${sysaux_LIB})

add_subdirectory(sm)
add_subdirectory(tr)
add_subdirectory(gov)
