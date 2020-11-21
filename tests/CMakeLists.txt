add_test(NAME se_starts COMMAND      se_gov   WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
add_test(NAME se_create_db COMMAND   se_cdb testdb   WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
add_test(NAME se_run_db COMMAND      se_sm testdb   WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
add_test(NAME se_shutdown_db COMMAND se_smsd testdb WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
add_test(NAME se_delete_db COMMAND   se_ddb testdb   WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
add_test(NAME se_stops COMMAND       se_stop  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Setup/cleanup
set_tests_properties(se_starts PROPERTIES FIXTURES_SETUP server)
set_tests_properties(se_stops PROPERTIES FIXTURES_CLEANUP server)
set_tests_properties(se_create_db PROPERTIES FIXTURE_REQUIRED server FIXTURE_SETUP dbcreated)
set_tests_properties(se_delete_db PROPERTIES FIXTURE_REQUIRED server FIXTURE_CLEANUP dbcreated)
set_tests_properties(se_run_db PROPERTIES FIXTURE_REQUIRED "server;dbcreated" FIXTURE_SETUP dbruns)
set_tests_properties(se_shutdown_db PROPERTIES FIXTURE_REQUIRED "server;dbcreated" FIXTURE_CLEANUP dbruns)



#add_custom_target(run_tests ALL
#        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
#        )