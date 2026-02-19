# Copyright 2020 Olivier Croquette <ocroquette@free.fr>
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT
# WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
# THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
# LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
# OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
# WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
# This script is run as POST_BUILD step, added by QtTest.cmake. See there
# for more information.
#
# v1.2
#
# Latest version is available from GitHub:
# https://github.com/ocroquette/cmake-qtest-discovery

get_filename_component(working_dir ${TEST_EXECUTABLE} DIRECTORY)

execute_process(
  COMMAND "${TEST_EXECUTABLE}" -datatags
  WORKING_DIR ${working_dir}
  OUTPUT_VARIABLE output_variable
  TIMEOUT ${TEST_DISCOVERY_TIMEOUT}
  ERROR_VARIABLE error_variable
  RESULT_VARIABLE result_variable
)

if(NOT "${result_variable}" EQUAL 0)
  string(REPLACE "\n" "\n    " output "${output}")
  message(FATAL_ERROR
    "Error running test executable.\n"
    "  Path: '${TEST_EXECUTABLE}'\n"
    "  Result: ${result_variable}\n"
    "  Output:\n"
    "    ${output_variable}\n"
    "  Error:\n"
    "    ${error_variable}\n"
  )
endif()

macro(get_and_escape_parameters)
  set(test_class ${CMAKE_MATCH_1})
  set(test_method ${CMAKE_MATCH_2})
  set(test_dataset ${CMAKE_MATCH_3})

  # Test class and method should be safe, but the dataset name might
  # contain problematic characters
  set(test_dataset_escaped "${test_dataset}")
  string(REPLACE "\\" "\\\\" test_dataset_escaped "${test_dataset_escaped}")
  string(REPLACE "\"" "\\\"" test_dataset_escaped "${test_dataset_escaped}")
endmacro()

set(ctest_script_content)
string(REPLACE "\n" ";" output_lines "${output_variable}")
foreach(line ${output_lines})
  set(generated_name)
  if(line MATCHES "^([^ ]*) ([^ ]*) (.*)$")
    # Line contains a data set name, like in:
    #   test_qttestdemo myParameterizedTest data set name 1
    #   test_qttestdemo myParameterizedTest data set name 2
    get_and_escape_parameters()
    set(generated_name "${TEST_PREFIX}${test_class}.${test_method}:${test_dataset_escaped}${TEST_SUFFIX}")
    string(APPEND ctest_script_content "add_test(\"${generated_name}\" \"${TEST_EXECUTABLE}\" \"${test_method}:${test_dataset_escaped}\")\n")
  elseif(line MATCHES "^([^ ]*) ([^ ]*)$")
    # Line doesn't contain a data set name, like in:
    #   test_qttestdemo myFirstTest
    #   test_qttestdemo mySecondTest
    get_and_escape_parameters()
    set(generated_name "${TEST_PREFIX}${test_class}.${test_method}${TEST_SUFFIX}")
    string(APPEND ctest_script_content "add_test(\"${generated_name}\" \"${TEST_EXECUTABLE}\" \"${test_method}\")\n")
  endif()
  if(generated_name)
    # Make ctest aware of tests skipped with QSKIP()
    #    SKIP   : test_qttestdemo::mySkippedTest() Example of skipped test
    string(APPEND ctest_script_content
      "set_tests_properties(\"${generated_name}\" PROPERTIES SKIP_REGULAR_EXPRESSION \"SKIP   : \")\n"
    )
    # Set other properties
    string(APPEND ctest_script_content
      "set_tests_properties(\"${generated_name}\" PROPERTIES WORKING_DIRECTORY \"${TEST_WORKING_DIR}\")\n"
    )
    string(REPLACE ";" " " external_properties "${TEST_PROPERTIES}")
    string(APPEND ctest_script_content
      "set_tests_properties(\"${generated_name}\" PROPERTIES ${external_properties})\n"
    )
  endif()
endforeach()

file(WRITE "${CTEST_FILE}" "${ctest_script_content}")
