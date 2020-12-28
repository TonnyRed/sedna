# additional sub-libraries
set(lib_SUBDIRS bufmgr llsm lm wu)

# common sources for our executables
set(common_SRC sm_globals.cpp)
foreach (SUB ${lib_SUBDIRS})
    ADD_SUBFOLDER_SRC(common_SRC ${SUB})
endforeach (SUB ${lib_SUBDIRS})

# libraries for our executables
#list(APPEND common_libs common expat ${sysaux_LIB})

# se_sm
set(sm_SRC sm.cpp sm_functions.cpp trmgr.cpp hb_utils.cpp ${common_SRC})
SET_SOURCES(sm_SRC 1)

add_executable(se_sm ${sm_SRC})
target_link_libraries(se_sm PRIVATE common_libs)

# se_cdb
set(cdb_SRC cdb.cpp cdb_globals.cpp sm_functions.cpp db_utils.cpp trmgr.cpp ${common_SRC})
SET_SOURCES(cdb_SRC 1)

add_executable(se_cdb ${cdb_SRC})
target_link_libraries(se_cdb PRIVATE common_libs)

# se_ddb
set(ddb_SRC db_utils.cpp ddb.cpp ${common_SRC})
SET_SOURCES(ddb_SRC 1)

add_executable(se_ddb ${ddb_SRC})
target_link_libraries(se_ddb PRIVATE common_libs)

# se_smsd
set(smsd_SRC smsd.cpp ${common_SRC})
SET_SOURCES(smsd_SRC 1)

add_executable(se_smsd ${smsd_SRC})
target_link_libraries(se_smsd PRIVATE common_libs)

# place binaries in build/bin directory
set(sm_execs se_sm se_cdb se_ddb se_smsd)
FinalizeBinaries(${sm_execs})


# msvc-specific option
if (MSVC)
    set_target_properties(${sm_execs} PROPERTIES LINK_FLAGS "/LARGEADDRESSAWARE")
endif (MSVC)

# install in bin
install(TARGETS ${sm_execs} RUNTIME DESTINATION bin)

