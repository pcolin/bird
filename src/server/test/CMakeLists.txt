file(GLOB TEST_SOURCE_FILES ${PROJECT_SOURCE_DIR}/test/ "*.cpp")

foreach(SOURCE_FILE ${TEST_SOURCE_FILES})
  get_filename_component(TEST_EXCUTABLE_NAME ${SOURCE_FILE} NAME_WE)
  add_executable(${TEST_EXCUTABLE_NAME} ${SOURCE_FILE})
  target_link_libraries(${TEST_EXCUTABLE_NAME} disruptor model config)

  file(READ "${SOURCE_FILE}" SOURCE_FILE_CONTENTS)
  string(REGEX MATCHALL "BOOST_AUTO_TEST_CASE\\( *([A-Za-z_0-9]+) *\\)" FOUND_TESTS ${SOURCE_FILE_CONTENTS})
  foreach(HIT ${FOUND_TESTS})
    string(REGEX REPLACE ".*\\( *([A-Za-z_0-9]+) *\\).*" "\\1" CASE_NAME ${HIT})
    add_test(NAME "${TEST_EXCUTABLE_NAME}.${CASE_NAME}"
             WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
             COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${TEST_EXCUTABLE_NAME}
                --run_test=${CASE_NAME} --catch_system_error=yes)
  endforeach()
endforeach()

# Use make check to output messages(line number etc) of failed tests. 
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND}
  --force-new-ctest-process --output-on-failure)
