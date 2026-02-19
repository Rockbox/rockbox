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
# v1.2
#
# Latest version is available from GitHub:
# https://github.com/ocroquette/cmake-qtest-discovery

#[=======================================================================[.rst:
QtTest
----------

This module defines functions to help use the Qt Test infrastructure.
The main function is :command:`qtest_discover_tests`.

.. command:: qtest_discover_tests

  Automatically add tests with CTest by querying the compiled test executable
  for available tests::

    qtest_discover_tests(target
                         [WORKING_DIRECTORY dir]
                         [TEST_PREFIX prefix]
                         [TEST_SUFFIX suffix]
                         [PROPERTIES name1 value1...]
                         [DISCOVERY_TIMEOUT seconds])

  ``qtest_discover_tests`` sets up a post-build command on the test executable
  that generates the list of tests by parsing the output from running the test
  with the ``-datatags`` argument. This ensures that the full list of
  tests, including instantiations of parameterized tests, is obtained.  Since
  test discovery occurs at build time, it is not necessary to re-run CMake when
  the list of tests changes.

  The options are:

  ``target``
    Specifies the Google Test executable, which must be a known CMake
    executable target.  CMake will substitute the location of the built
    executable when running the test.

  ``WORKING_DIRECTORY dir``
    Specifies the directory in which to run the discovered test cases. If this
    option is not provided, the current binary directory is used.

  ``TEST_PREFIX prefix``
    Specifies a ``prefix`` to be prepended to the name of each discovered test
    case.

  ``TEST_SUFFIX suffix``
    Similar to ``TEST_PREFIX`` except the ``suffix`` is appended to the name of
    every discovered test case.  Both ``TEST_PREFIX`` and ``TEST_SUFFIX`` may
    be specified.

  ``PROPERTIES name1 value1...``
    Specifies additional properties to be set on all tests discovered by this
    invocation of ``qtest_discover_tests``. You can specify a timeout for the
    test execution by setting the TIMEOUT property here, as supported by ctest.

  ``DISCOVERY_TIMEOUT sec``
    Specifies how long (in seconds) CMake will wait for the test to enumerate
    available tests. If the test takes longer than this, discovery (and your
    build) will fail. Most test executables will enumerate their tests very
    quickly, but under some exceptional circumstances, a test may require a
    longer timeout.  The default is 5.  See also the ``TIMEOUT`` option of
    :command:`execute_process`.

#]=======================================================================]

function(qtest_discover_tests TARGET)
if(NOT CMAKE_CROSSCOMPILING)
  cmake_parse_arguments(
    ""
    ""
    "TEST_PREFIX;TEST_SUFFIX;WORKING_DIRECTORY;DISCOVERY_TIMEOUT"
    "PROPERTIES"
    ${ARGN}
  )

  if(NOT _WORKING_DIRECTORY)
    set(_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
  endif()
  if(NOT _DISCOVERY_TIMEOUT)
    set(_DISCOVERY_TIMEOUT 5)
  endif()

  set(ctest_file_base "${CMAKE_CURRENT_BINARY_DIR}/${TARGET}")
  set(ctest_include_file "${ctest_file_base}_tests.cmake")
  add_custom_command(TARGET ${TARGET}
    POST_BUILD
    COMMAND "${CMAKE_COMMAND}"
      -D "\"TEST_EXECUTABLE:FILEPATH=$<TARGET_FILE:${TARGET}>\""
      -D "\"CTEST_FILE:FILEPATH=${ctest_include_file}\""
      -D "\"TEST_PROPERTIES=${_PROPERTIES}\""
      -D "\"TEST_DISCOVERY_TIMEOUT=${_DISCOVERY_TIMEOUT}\""
      -D "\"TEST_PREFIX=${_TEST_PREFIX}\""
      -D "\"TEST_SUFFIX=${_TEST_SUFFIX}\""
      -D "\"TEST_WORKING_DIR=${_WORKING_DIRECTORY}\""
      -P "\"${_QTTEST_DISCOVER_TESTS_SCRIPT}\""
    BYPRODUCTS "${ctest_include_file}"
  )

  set_property(DIRECTORY APPEND PROPERTY TEST_INCLUDE_FILES
    "${ctest_include_file}"
  )
else()
  message("-- Cross compiling, discovering unit tests disabled.")
endif()
endfunction()


###############################################################################

set(_QTTEST_DISCOVER_TESTS_SCRIPT
  ${CMAKE_CURRENT_LIST_DIR}/QtTestAddTests.cmake
)
