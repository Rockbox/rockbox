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

# This is a separate cmake script, to be invoked as
#   cmake -P -DURL=<url-to-download> -DOUTDIR=<output-folder>
# Downloads the file and store it in OUTDIR, using the file basename as output
# filename.
# The downloaded file gets its executable flag set.

function(gettempdir basedir tmpdir)
    # Create a random filename in current directory.
    # Result stored in tmpdir.
    string(RANDOM LENGTH 24 _tmp)
    while(EXISTS "${basedir}/${_tmp}.tmp")
        string(RANDOM LENGTH 24 _tmp)
    endwhile()
    set("${tmpdir}" "${basedir}/${_tmp}.tmp" PARENT_SCOPE)
endfunction()

get_filename_component(fname "${URL}" NAME)

if(EXISTS "${OUTDIR}/${fname}")
    message("-- Found ${fname}")
else()
    message("-- Downloading ${URL} ...")
    gettempdir(${OUTDIR} tmp)

    # cmake CHOWN is 3.19+, thus download to a temporary folder, then copy.
    file(DOWNLOAD "${URL}" "${tmp}/${fname}")
    file(COPY "${tmp}/${fname}" DESTINATION "${OUTDIR}"
         FILE_PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ)
    file(REMOVE_RECURSE "${tmp}")
endif()


