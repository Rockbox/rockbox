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
cmake_minimum_required(VERSION 3.12)
project(RockboxUtils)

set (CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (NOT CMAKE_BUILD_TYPE)
    message("-- CMAKE_BUILD_TYPE not set, assuming Release")
    set(CMAKE_BUILD_TYPE Release)
endif()
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    message("-- Found ccache, enabling")
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()

# Global compiler options
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Clang and AppleClang
    if (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        # using clang with clang-cl front end
    elseif (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "GNU")
        # using clang with regular front end
        add_compile_options(-Wall -Wextra)
    endif()
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wall -Wextra)
    add_link_options($<$<CONFIG:RELEASE>:-s>)
endif()


enable_testing()

# Qt
find_package(QT NAMES Qt6 Qt5 REQUIRED)
if(QT_VERSION_MAJOR EQUAL 5)
    find_package(Qt5 REQUIRED COMPONENTS
                 Core Widgets Svg Multimedia Network Test LinguistTools)
else()
    find_package(Qt6 REQUIRED COMPONENTS
                 Core Core5Compat Widgets Svg Network LinguistTools SvgWidgets
                 OPTIONAL_COMPONENTS Multimedia Test)
endif()
get_target_property(_moc_executable Qt${QT_VERSION_MAJOR}::moc IMPORTED_LOCATION)
get_filename_component(QT_BINDIR "${_moc_executable}" DIRECTORY)
message("-- Found Qt${QT_VERSION_MAJOR}: ${Qt${QT_VERSION_MAJOR}_DIR}")

# If we're on Linux, try to find the used libs in the system.
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # required system libs
    find_package(PkgConfig)
    pkg_check_modules(libusb QUIET REQUIRED IMPORTED_TARGET libusb-1.0)
    message("-- Found libusb: ${libusb_LINK_LIBRARIES}")

    # optional system libs.
    pkg_check_modules(speex QUIET IMPORTED_TARGET speex speexdsp)
    if (${speex_FOUND})
        message("-- Found speex: ${speex_LINK_LIBRARIES}")
    else()
        message("-- Could not find system speex, using our own copy.")
    endif()
endif()

if(APPLE)
    find_library(FRAMEWORK_IOKIT IOKit)
    find_library(FRAMEWORK_COREFOUNDATION CoreFoundation)
    find_library(FRAMEWORK_CARBON Carbon)
    find_library(FRAMEWORK_SYSTEMCONFIGURATION SystemConfiguration)
    find_library(FRAMEWORK_CORESERVICES CoreServices)
endif()


add_executable(ipodpatcher-bin
    ipodpatcher/main.c
    ipodpatcher/ipodpatcher-aupd.c
    )
target_link_libraries(ipodpatcher-bin ipodpatcher)
target_compile_definitions(ipodpatcher-bin PRIVATE VERSION="none")
set_target_properties(ipodpatcher-bin PROPERTIES OUTPUT_NAME ipodpatcher)
if(APPLE)
    target_link_libraries(ipodpatcher-bin ${FRAMEWORK_IOKIT} ${FRAMEWORK_COREFOUNDATION})
endif()

add_library(ipodpatcher
    ipodpatcher/arc4.h
    ipodpatcher/arc4.c
    ipodpatcher/fat32format.c
    ipodpatcher/ipodpatcher.c
    ipodpatcher/ipodio.h
    ipodpatcher/ipodpatcher.h
    ipodpatcher/parttypes.h
    )
if(WIN32)
    target_sources(ipodpatcher PRIVATE
        ipodpatcher/ipodio-win32.c
        ipodpatcher/ipodio-win32-scsi.c
    )
else()
    target_sources(ipodpatcher PRIVATE
        ipodpatcher/ipodio-posix.c
    )
endif()
target_compile_definitions(ipodpatcher PUBLIC RBUTIL _LARGEFILE64_SOURCE)
target_include_directories(ipodpatcher PUBLIC ${CMAKE_CURRENT_LIST_DIR}/ipodpatcher)

add_executable(sansapatcher-bin
    sansapatcher/main.c
    )
target_link_libraries(sansapatcher-bin sansapatcher)
target_compile_definitions(sansapatcher-bin PRIVATE VERSION="none")
set_target_properties(sansapatcher-bin PROPERTIES OUTPUT_NAME sansapatcher)

add_library(sansapatcher
    sansapatcher/parttypes.h
    sansapatcher/sansaio.h
    sansapatcher/sansapatcher.c
    sansapatcher/sansapatcher.h
    )
if(WIN32)
    target_sources(sansapatcher PRIVATE
        sansapatcher/sansaio-win32.c
    )
else()
    target_sources(sansapatcher PRIVATE
        sansapatcher/sansaio-posix.c
    )
endif()

target_compile_definitions(sansapatcher PUBLIC RBUTIL _LARGEFILE64_SOURCE)
target_include_directories(sansapatcher PUBLIC ${CMAKE_CURRENT_LIST_DIR}/sansapatcher)

add_library(chinachippatcher
    chinachippatcher/chinachip.c
    chinachippatcher/chinachip.h
    )
target_compile_definitions(chinachippatcher PUBLIC RBUTIL _LARGEFILE64_SOURCE)

add_library(mkamsboot
    mkamsboot/dualboot.c
    mkamsboot/dualboot.h
    mkamsboot/md5.c
    mkamsboot/md5.h
    mkamsboot/mkamsboot.c
    mkamsboot/mkamsboot.h
    )

target_compile_definitions(mkamsboot PUBLIC _LARGEFILE64_SOURCE)
target_link_libraries(mkamsboot ucl)

add_library(ucl
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/alloc.c
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/fake16.h
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/getbit.h
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/internal.h
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/io.c
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/Makefile
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/n2_99.ch
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/n2b_99.c
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/n2b_d.c
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/n2b_ds.c
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/n2b_to.c
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/n2d_99.c
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/n2d_d.c
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/n2d_ds.c
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/n2d_to.c
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/n2e_99.c
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/n2e_d.c
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/n2e_ds.c
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/n2e_to.c
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/ucl_conf.h
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/ucl_crc.c
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/ucl_dll.c
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/ucl_init.c
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/ucl_mchw.ch
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/ucl_ptr.c
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/ucl_ptr.h
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/ucl_str.c
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/ucl_swd.ch
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/ucl_util.c
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/ucl_util.h
    )

target_include_directories(ucl PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/include)

add_library(rbspeex
    ${CMAKE_CURRENT_LIST_DIR}/../tools/rbspeex/rbspeex.c
    ${CMAKE_CURRENT_LIST_DIR}/../tools/rbspeex/rbspeex.h
    )
target_compile_definitions(rbspeex PRIVATE HAVE_CONFIG_H ROCKBOX_VOICE_ENCODER STATIC)
target_include_directories(rbspeex PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../tools/rbspeex)


if (${speex_FOUND})
    target_link_libraries(rbspeex PUBLIC PkgConfig::speex)
else()
    target_sources(rbspeex PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/bits.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/cb_search.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/exc_10_16_table.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/exc_10_32_table.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/exc_20_32_table.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/exc_5_256_table.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/exc_5_64_table.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/exc_8_128_table.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/filters.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/gain_table.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/gain_table_lbr.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/hexc_10_32_table.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/hexc_table.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/high_lsp_tables.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/lsp.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/lsp_tables_nb.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/ltp.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/modes.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/modes_wb.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/nb_celp.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/quant_lsp.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/sb_celp.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/speex.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/speex_callbacks.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/lpc.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/vbr.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/vq.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/window.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/resample.c
        )
    target_include_directories(rbspeex PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex)
endif()

add_library(mkimxboot
    mkimxboot/dualboot.c
    mkimxboot/md5.c
    mkimxboot/md5.h
    mkimxboot/mkimxboot.c
    mkimxboot/mkimxboot.h
    )
target_include_directories(mkimxboot PUBLIC ${CMAKE_CURRENT_LIST_DIR}/mkimxboot)
target_link_libraries(mkimxboot sbtools)

add_library(sbtools
    imxtools/sbtools/crc.c
    imxtools/sbtools/crypto.c
    imxtools/sbtools/crypto.h
    imxtools/sbtools/elf.c
    imxtools/sbtools/elf.h
    imxtools/sbtools/misc.c
    imxtools/sbtools/misc.h
    imxtools/sbtools/sb.c
    imxtools/sbtools/sb.h
    )

target_include_directories(sbtools PUBLIC ${CMAKE_CURRENT_LIST_DIR}/imxtools/sbtools)
target_link_libraries(sbtools PUBLIC rbtomcrypt)

add_library(mkmpioboot
    mkmpioboot/mkmpioboot.c
    mkmpioboot/mkmpioboot.h
    )
target_include_directories(mkmpioboot PUBLIC ${CMAKE_CURRENT_LIST_DIR}/mkmpioboot)

add_library(mktccboot
    mktccboot/mktccboot.c
    mktccboot/mktccboot.h
    )
target_include_directories(mktccboot PUBLIC ${CMAKE_CURRENT_LIST_DIR}/mktccboot)
target_include_directories(mktccboot PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../tools)
target_link_libraries(mktccboot PUBLIC telechips)

add_library(mks5lboot
    mks5lboot/dualboot.c
    mks5lboot/dualboot.h
    mks5lboot/ipoddfu.c
    mks5lboot/mkdfu.c
    mks5lboot/mks5lboot.h
    )
target_include_directories(mks5lboot PUBLIC ${CMAKE_CURRENT_LIST_DIR}/mks5lboot)

add_library(bspatch
    bspatch/bspatch.c
    bspatch/bspatch.h
    )
target_include_directories(bspatch PUBLIC ${CMAKE_CURRENT_LIST_DIR}/bspatch)
target_link_libraries(bspatch bz2)

add_library(voicefont
    ${CMAKE_CURRENT_LIST_DIR}/../tools/voicefont.c
    ${CMAKE_CURRENT_LIST_DIR}/../tools/voicefont.h
    )
target_include_directories(voicefont PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../tools)
target_compile_definitions(voicefont PUBLIC RBUTIL)

add_library(telechips
    ${CMAKE_CURRENT_LIST_DIR}/../tools/telechips.c
    ${CMAKE_CURRENT_LIST_DIR}/../tools/telechips.h
    )
target_include_directories(telechips PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../tools)

find_library(LIBBZ2 bz2)
if(${LIBBZ2} STREQUAL "LIBBZ2-NOTFOUND")
    message("-- bz2 not found, building our own")
    add_library(bz2
        bzip2/blocksort.c
        bzip2/bzlib.c
        bzip2/bzlib.h
        bzip2/bzlib_private.h
        bzip2/compress.c
        bzip2/crctable.c
        bzip2/decompress.c
        bzip2/huffman.c
        bzip2/randtable.c
        )
    target_include_directories(bz2 PUBLIC ${CMAKE_CURRENT_LIST_DIR}/bzip2)
else()
    message("-- Found libbz2: ${LIBBZ2}")
endif()

add_library(rbtomcrypt
    tomcrypt/src/ciphers/aes/aes.c
    tomcrypt/src/ciphers/aes/aes_tab.c
    tomcrypt/src/misc/crypt/crypt_argchk.c
    tomcrypt/src/misc/crypt/crypt_register_cipher.c
    tomcrypt/src/misc/crypt/crypt_cipher_is_valid.c
    tomcrypt/src/misc/crypt/crypt_cipher_descriptor.c
    tomcrypt/src/misc/zeromem.c
    tomcrypt/src/misc/compare_testvector.c
    tomcrypt/src/modes/cbc/cbc_start.c
    tomcrypt/src/modes/cbc/cbc_decrypt.c
    tomcrypt/src/modes/cbc/cbc_encrypt.c
    tomcrypt/src/hashes/sha1.c
    tomcrypt/src/headers/tomcrypt_argchk.h
    tomcrypt/src/headers/tomcrypt_cfg.h
    tomcrypt/src/headers/tomcrypt_cipher.h
    tomcrypt/src/headers/tomcrypt_custom.h
    tomcrypt/src/headers/tomcrypt.h
    tomcrypt/src/headers/tomcrypt_hash.h
    tomcrypt/src/headers/tomcrypt_mac.h
    tomcrypt/src/headers/tomcrypt_macros.h
    tomcrypt/src/headers/tomcrypt_math.h
    tomcrypt/src/headers/tomcrypt_misc.h
    tomcrypt/src/headers/tomcrypt_pkcs.h
    tomcrypt/src/headers/tomcrypt_pk.h
    tomcrypt/src/headers/tomcrypt_private.h
    tomcrypt/src/headers/tomcrypt_prng.h
    )
target_include_directories(rbtomcrypt PUBLIC ${CMAKE_CURRENT_LIST_DIR}/tomcrypt/src/headers)

add_library(skin_parser
    ${CMAKE_CURRENT_LIST_DIR}/../lib/skin_parser/skin_buffer.c
    ${CMAKE_CURRENT_LIST_DIR}/../lib/skin_parser/skin_buffer.h
    ${CMAKE_CURRENT_LIST_DIR}/../lib/skin_parser/skin_debug.c
    ${CMAKE_CURRENT_LIST_DIR}/../lib/skin_parser/skin_debug.h
    ${CMAKE_CURRENT_LIST_DIR}/../lib/skin_parser/skin_parser.c
    ${CMAKE_CURRENT_LIST_DIR}/../lib/skin_parser/skin_parser.h
    ${CMAKE_CURRENT_LIST_DIR}/../lib/skin_parser/skin_scan.c
    ${CMAKE_CURRENT_LIST_DIR}/../lib/skin_parser/skin_scan.h
    ${CMAKE_CURRENT_LIST_DIR}/../lib/skin_parser/symbols.h
    ${CMAKE_CURRENT_LIST_DIR}/../lib/skin_parser/tag_table.c
    ${CMAKE_CURRENT_LIST_DIR}/../lib/skin_parser/tag_table.h
    )
target_include_directories(skin_parser PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../lib/skin_parser)
target_compile_definitions(skin_parser PUBLIC __PCTOOL__)

# target will always be build. File is only updated when contents change.
add_custom_target(gitversion.h
    COMMENT "Updating gitversion.h"
    COMMAND ${CMAKE_COMMAND}
            -DOUTFILE=${CMAKE_CURRENT_BINARY_DIR}/gitversion/gitversion.h
            -P ${CMAKE_CURRENT_LIST_DIR}/cmake/gitversion.cmake
    BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/gitversion/gitversion.h)

# link to get git version header.
add_library(gitversion INTERFACE)

add_dependencies(gitversion gitversion.h)
target_include_directories(gitversion INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/gitversion)

include(${CMAKE_CURRENT_LIST_DIR}/cmake/deploy.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/QtTest.cmake)
add_subdirectory(rbutilqt)
add_subdirectory(themeeditor)

