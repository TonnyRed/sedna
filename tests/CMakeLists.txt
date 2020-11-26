add_test(NAME se_starts COMMAND      $<TARGET_FILE:se_gov>)
add_test(NAME se_create_db COMMAND   $<TARGET_FILE:se_cdb> auction)
add_test(NAME se_run_db COMMAND      $<TARGET_FILE:se_sm> auction )
add_test(NAME se_dataload COMMAND $<TARGET_FILE:se_term> -file load_data.xquery auction
              WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/examples/commandline)
add_test(NAME se_term_xqsample07 COMMAND $<TARGET_FILE:se_term> -file sample07.xquery auction
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/examples/commandline)
set_tests_properties(se_term_xqsample07 PROPERTIES
        PASS_REGULAR_EXPRESSION
"<items person=\"Huei Demke\">2</items>
<items person=\"Jarkko Nozawa\">2</items>
<items person=\"Laurian Grass\">2</items>"
        )

add_test(NAME se_shutdown_db COMMAND $<TARGET_FILE:se_smsd> auction)
add_test(NAME se_delete_db COMMAND   $<TARGET_FILE:se_ddb> auction)
add_test(NAME se_stops COMMAND       $<TARGET_FILE:se_stop>)
add_test(NAME se_term COMMAND       $<TARGET_FILE:se_term> -version)

# Setup/cleanup

set_tests_properties(se_dataload PROPERTIES FIXTURE_REQUIRED dbruns)
set_tests_properties(se_starts PROPERTIES FIXTURES_SETUP server)
set_tests_properties(se_stops PROPERTIES FIXTURES_CLEANUP server)
set_tests_properties(se_create_db PROPERTIES FIXTURE_REQUIRED server FIXTURE_SETUP dbcreated)
set_tests_properties(se_delete_db PROPERTIES FIXTURE_REQUIRED server FIXTURE_CLEANUP dbcreated)
set_tests_properties(se_run_db PROPERTIES FIXTURE_REQUIRED "server;dbcreated" FIXTURE_SETUP dbruns)
set_tests_properties(se_shutdown_db PROPERTIES FIXTURE_REQUIRED "server;dbcreated" FIXTURE_CLEANUP dbruns)

