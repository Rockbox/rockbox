#
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#
# All files in this archive are subject to the GNU General Public License.
# See the file COPYING in the source tree root for full license agreement.
#
# This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
# KIND, either express or implied.
#

find_package(Git QUIET)

execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --verify --short=10 HEAD
                OUTPUT_VARIABLE GIT_HASH
                ERROR_QUIET)

# Check whether we got any revision (which isn't
# always the case, e.g. when someone downloaded a zip
# file from Github instead of a checkout)
if ("${GIT_HASH}" STREQUAL "")
    set(GIT_HASH "N/A")
else()
    execute_process(
        COMMAND git diff --quiet --exit-code
        RESULT_VARIABLE GIT_DIFF_EXITCODE)

    string(STRIP "${GIT_HASH}" GIT_HASH)
    if (${GIT_DIFF_EXITCODE})
        set(GIT_DIFF "M")
    endif()
endif()

string(TIMESTAMP TODAY "%y%m%d")
set(VERSION "
#ifndef GITVERSION
#define GITVERSION \"${GIT_HASH}${GIT_DIFF}-${TODAY}\"
#define GITHASH \"${GIT_HASH}${GIT_DIFF}\"
#define BUILDDATE \"${TODAY}\"
#endif
")

if(EXISTS ${OUTFILE})
    file(READ "${OUTFILE}" _version)
else()
    set(_version "")
endif()

if (NOT "${VERSION}" STREQUAL "${_version}")
    file(WRITE "${OUTFILE}" "${VERSION}")
endif()

message("-- Revision: ${GIT_HASH}${GIT_DIFF}")

