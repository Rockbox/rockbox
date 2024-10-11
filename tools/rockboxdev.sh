#!/bin/bash

# Abort execution as soon as an error is encountered
# That way the script do not let the user think the process completed correctly
# and leave the opportunity to fix the problem and restart compilation where
# it stopped
set -e

# this is where this script will store downloaded files and check for already
# downloaded files
dlwhere="${RBDEV_DOWNLOAD:-/tmp/rbdev-dl}"

# will append the target string to the prefix dir mentioned here
# Note that the user running this script must be able to do make install in
# this given prefix directory. Also make sure that this given root dir
# exists.
prefix="${RBDEV_PREFIX:-/usr/local}"

# This directory is used to extract all files and to build everything in. It
# must not exist before this script is invoked (as a security measure).
builddir="${RBDEV_BUILD:-/tmp/rbdev-build}"

# This script needs to use GNU Make. On Linux systems, GNU Make is invoked
# by running the "make" command, on most BSD systems, GNU Make is invoked
# by running the "gmake" command. Set the "make" variable accordingly.
if [ -f "`which gmake 2>/dev/null`" ]; then
    make="gmake"
else
    make="make"
fi

# record version
makever=`$make -v |sed -n '1p' | sed -e 's/.* \([0-9]*\)\.\([0-9]*\).*/\1\2/'`

# This is the absolute path to where the script resides.
rockboxdevdir="$( readlink -f "$( dirname "${BASH_SOURCE[0]}" )" )"
patch_dir="$rockboxdevdir/toolchain-patches"

if [ `uname -s` = "Darwin" ]; then
    parallel=`sysctl -n hw.physicalcpu`
else
    parallel=`nproc`
fi

if [ $parallel -gt 1 ] ; then
  make_parallel=-j$parallel
fi

if [ -z $GNU_MIRROR ] ; then
    GNU_MIRROR=http://mirrors.kernel.org/gnu
fi

if [ -z $LINUX_MIRROR ] ; then
    LINUX_MIRROR=http://www.kernel.org/pub/linux
fi

# These are the tools this script requires and depends upon.
reqtools="gcc g++ xz bzip2 gzip $make patch makeinfo automake libtool autoconf flex bison"

##############################################################################
# Functions:

findtool(){
  file="$1"

  IFS=":"
  for path in $PATH
  do
    # echo "Checks for $file in $path" >&2
    if test -f "$path/$file"; then
      echo "$path/$file"
      return
    fi
  done
}

findlib (){
    lib="$1"
    # on error, gcc will spit out "cannot find -lxxxx", but it may not be in
    # english so grep for -lxxxx
    if ! gcc -l$lib 2>&1 | grep -q -- "-l$lib"; then
        echo "ok"
        return
    fi
}

# check if all the libraries in argument are installed, exit with error if not
checklib() {
    for t in "$@"; do
        lib=`findlib $t`
        if test -z "$lib"; then
            echo "ROCKBOXDEV: library \"$t\" is required for this script to work."
            missingtools="yes"
        fi
    done
    if [ -n "$missingtools" ]; then
        echo "ROCKBOXDEV: Please install the missing libraries and re-run the script."
        exit 1
    fi
}

input() {
    read response
    echo $response
}

# compare two versions, return 1 if first version is strictly before the second
# version_lt ver1 ver2
version_lt() {
    # use sort with natural version sorting
    ltver=`printf "$1\n$2" | sort -V | head -n 1`
    [ "$1" = "$ltver" ] && true || false
}

# Download a file from a server (unless it already exists locally in $dlwhere).
# The output file name is always $dlwhere/$1, and the function will try URLs
# one after the other
# $1 file
# $2 server file name
# $2,$3,... URL root list
getfile_ex() {
    out_file="$dlwhere/$1"
    srv_file="$2"
    if test -f $out_file; then
        echo "ROCKBOXDEV: Skipping download of $1: File already exists"
        return
    fi
    # find tool (curl or wget) and build download command
    tool=`findtool curl`
    if test -z "$tool"; then
        tool=`findtool wget`
        if test -n "$tool"; then
            # wget download
            echo "ROCKBOXDEV: Downloading $1 using wget"
            tool="$tool -T 60 -O "
        else
            echo "ROCKBOXDEV: No downloader tool found!"
            echo "ROCKBOXDEV: Please install curl or wget and re-run the script"
            exit
        fi
    else
        # curl download
        echo "ROCKBOXDEV: Downloading $1 using curl"
        tool="$tool -fLo "
    fi

    # shift arguments so that $@ is the list of URLs
    shift
    shift
    for url in "$@"; do
        echo "ROCKBOXDEV: try $url/$srv_file"
        if $tool "$out_file" "$url/$srv_file"; then
            return
        fi
    done

    echo "ROCKBOXDEV: couldn't download the file!"
    echo "ROCKBOXDEV: check your internet connection"
    exit
}

#$1 file
#$2 URL"root
# Output file name is the same as the server file name ($1)
# Does not download the file if it already exists locally in $dlwhere/
getfile() {
    getfile_ex "$1" "$1" "$2"
}

# wrapper around getfile to abstract url
# getttool tool version
# the file is always downloaded to "$dlwhere/$toolname-$version.tar.bz2"
gettool() {
    toolname="$1"
    version="$2"
    ext="tar.bz2"
    file="$toolname-$version"
    srv_file="$toolname-$version"
    case $toolname in
        gcc)
            # before 4.7, the tarball was named gcc-core
            if version_lt "$version" "4.7"; then
                srv_file="gcc-core-$version"
            fi
            url="$GNU_MIRROR/gcc/gcc-$version"
	    if ! version_lt "$version" "7.2"; then
		ext="tar.xz"
	    fi
            ;;

        binutils)
            url="$GNU_MIRROR/binutils"
	    if ! version_lt "$version" "2.28.1"; then
		ext="tar.xz"
	    fi
            ;;

        glibc)
            url="$GNU_MIRROR/glibc"
	    if ! version_lt "$version" "2.11"; then
		ext="tar.xz"
	    fi
            ;;

        alsa-lib)
            url="ftp://ftp.alsa-project.org/pub/lib"
            ;;

        libffi)
            url="ftp://sourceware.org/pub/libffi"
            ext="tar.gz"
            ;;

        glib)
            url="https://ftp.gnome.org/pub/gnome/sources/glib/2.46"
            ext="tar.xz"
            ;;

        zlib)
            url="https://www.zlib.net/fossils"
            ext="tar.gz"
            ;;

        dbus)
            url="https://dbus.freedesktop.org/releases/dbus"
            ext="tar.gz"
            ;;

        expat)
            url="https://src.fedoraproject.org/repo/pkgs/expat/expat-2.1.0.tar.gz/dd7dab7a5fea97d2a6a43f511449b7cd"
            ext="tar.gz"
            ;;

        linux)
            # FIXME linux kernel server uses an overcomplicated architecture,
            # especially for longterm kernels, so we need to lookup several
            # places. This will need fixing for non-4-part 2.6 versions but it
            # is written in a way that should make it easy
            case "$version" in
                2.6.*.*)
                    # 4-part versions -> remove last digit for longterm
                    longterm_ver="${version%.*}"
                    top_dir="v2.6"
                    ;;
                3.*)
                    longterm_ver=""
                    top_dir="v3.x"
                    ;;
                *)
                    echo "ROCKBOXDEV: I don't know how to handle this kernel version: $version"
                    exit
                ;;
            esac
            base_url="http://www.kernel.org/pub/linux/kernel/$top_dir"
            # we try several URLs, the 2.6 versions are a mess and need that
            url="$base_url $base_url/longterm/v$longterm_ver $base_url/longterm"
            ext="tar.gz"
            ;;

        *)
            echo "ROCKBOXDEV: Bad toolname $toolname"
            exit
            ;;
    esac
    getfile_ex "$file.$ext" "$srv_file.$ext" $url
}

# extract file to the current directory
# extract file
extract() {
    if [ -d "$1" ]; then
        echo "ROCKBOXDEV: Skipping extraction of $1: already done"
        return
    fi
    echo "ROCKBOXDEV: extracting $1"
    if [ -f "$dlwhere/$1.tar.bz2" ]; then
        tar xjf "$dlwhere/$1.tar.bz2"
    elif [ -f "$dlwhere/$1.tar.gz" ]; then
        tar xzf "$dlwhere/$1.tar.gz"
    elif [ -f "$dlwhere/$1.tar.xz" ]; then
        tar xJf "$dlwhere/$1.tar.xz"
    else
        echo "ROCKBOXDEV: unknown compression for $1"
        exit
    fi
}

# run a command, and a log command and output to a file (append)
# exit on error
# run_cmd logfile cmd...
run_cmd() {
    logfile="$1"
    shift
    echo "Running '$@'" >>$logfile
    if ! $@ >> "$logfile" 2>&1; then
        echo "ROCKBOXDEV: an error occured, please see $logfile"
        exit 1
    fi
}

# check if the following should be executed or not, depending on RBDEV_RESTART variable:
# $1=tool
# If RBDEV_RESTART is empty, always perform step, otherwise only perform is there
# is an exact match. On the first match, RBDEV_RESTART is reset to "" so that next step
# are executed
check_restart() {
    if [ "x$RBDEV_RESTART" = "x" ]; then
        return 0
    elif [ "$1" = "$RBDEV_RESTART" ]; then
        RBDEV_RESTART=""
        return 0
    else
        return 1
    fi
}

# advanced tool building: create a build directory, run configure, run make
# and run make install. It is possible to customize all steps or disable them
# $1=tool
# $2=version
# $3=configure options, or "NO_CONFIGURE" to disable step
# $4=make options, or "NO_MAKE" to disable step
# $5=make install options (will be replaced by "install" if left empty)
# By default, the restary step is the toolname, but it can be changed by setting
# RESTART_STEP
buildtool() {
    tool="$1"
    version="$2"
    toolname="$tool-$version"
    config_opt="$3"
    make_opts="$4"
    install_opts="$5"
    logfile="$builddir/build-$toolname.log"

    stepname=${RESTART_STEP:-$tool}
    if ! check_restart "$stepname"; then
        echo "ROCKBOXDEV: Skipping step '$stepname' as requested per RBDEV_RESTART"
        return
    fi
    echo "ROCKBOXDEV: Starting step '$stepname'"

    echo "ROCKBOXDEV: logging to $logfile"
    rm -f "$logfile"

    echo "ROCKBOXDEV: mkdir build-$toolname"
    mkdir "build-$toolname"

    cd build-$toolname

    # in-tree/out-of-tree build
    case "$tool" in
        linux|alsa-lib)
            # in-intree
            echo "ROCKBOXDEV: copy $toolname for in-tree build"
            # copy the source directory to the build directory
            cp -r ../$toolname/* .
            cfg_dir="."
            ;;
        *)
            # out-of-tree
            cfg_dir="../$toolname";
            ;;
    esac

    if [ "$RESTART_STEP" == "gcc-stage1" ] ; then
	CXXFLAGS="-std=gnu++03"
    elif [ "$RESTART_STEP" == "gcc-stage2" ] ; then
	CXXFLAGS="-std=gnu++11"
    else
	CXXFLAGS=""
    fi

    if [ "$tool" == "zlib" ]; then
        echo "ROCKBOXDEV: $toolname/configure"
        # NOTE glibc requires to be compiled with optimization
        CFLAGS='-U_FORTIFY_SOURCE -fgnu89-inline -O2' run_cmd "$logfile" \
            "$cfg_dir/configure" "--prefix=$prefix" \
            $config_opt
    elif [ "$config_opt" != "NO_CONFIGURE" ]; then
        echo "ROCKBOXDEV: $toolname/configure"
        cflags='-U_FORTIFY_SOURCE -fgnu89-inline -O2'
        if [ "$tool" == "glib" ]; then
            run_cmd "$logfile" sed -i -e 's/m4_copy/m4_copy_force/g' "$cfg_dir/m4macros/glib-gettext.m4"
            run_cmd "$logfile" autoreconf -fiv "$cfg_dir"
            cflags="$cflags -Wno-format-nonliteral -Wno-format-overflow"
        fi
        # NOTE glibc requires to be compiled with optimization
        CFLAGS="$cflags" CXXFLAGS="$CXXFLAGS" run_cmd "$logfile" \
            "$cfg_dir/configure" "--prefix=$prefix" \
            --disable-docs $config_opt
    fi

    if [ "$make_opts" != "NO_MAKE" ]; then
        echo "ROCKBOXDEV: $toolname/make"
        run_cmd "$logfile" $make $make_parallel $make_opts
    fi

    if [ "$install_opts" = "" ]; then
        install_opts="install"
    fi
    echo "ROCKBOXDEV: $toolname/make (install)"
    run_cmd "$logfile" $make $install_opts

    cd ..

    echo "ROCKBOXDEV: rm -rf build-$toolname $toolname"
    rm -rf build-$toolname
    if [ "$stepname" != "gcc-stage1" ] ; then
        echo "ROCKBOXDEV: rm -rf $toolname"
        rm -rf $toolname
    fi
}

build() {
    toolname="$1"
    target="$2"
    version="$3"
    patch="$4"
    configure_params="$5"
    needs_libs="$6"
    logfile="$builddir/build-$toolname.log"

    stepname=${RESTART_STEP:-$toolname}
    if ! check_restart "$stepname"; then
        echo "ROCKBOXDEV: Skipping step '$stepname' as requested per RBDEV_RESTART"
        return
    fi
    echo "ROCKBOXDEV: Starting step '$stepname'"

    # create build directory
    if test -d $builddir; then
        if test ! -w $builddir; then
            echo "ROCKBOXDEV: No write permission for $builddir"
            exit
        fi
    else
        mkdir -p $builddir
    fi

    # download source tarball
    gettool "$toolname" "$version"
    file="$toolname-$version"

    cd $builddir

    extract "$toolname-$version"

    # do we have a patch?
    for p in $patch; do
        echo "ROCKBOXDEV: applying patch $p"

        # apply the patch
        (cd $builddir/$toolname-$version && patch -p1 < "$patch_dir/$p")

        # check if the patch applied cleanly
        if [ $? -gt 0 ]; then
            echo "ROCKBOXDEV: failed to apply patch $p"
            exit
        fi
    done

    # kludge to avoid having to install GMP, MPFR, MPC and ISL
    if test -n "$needs_libs"; then
        cd "$toolname-$version"
        if (echo $needs_libs | grep -q gmp && test ! -d gmp); then
            echo "ROCKBOXDEV: Getting GMP"
            getfile "gmp-6.1.2.tar.xz" "$GNU_MIRROR/gmp"
            tar xJf $dlwhere/gmp-6.1.2.tar.xz
            ln -s gmp-6.1.2 gmp
        fi

        if (echo $needs_libs | grep -q mpfr && test ! -d mpfr); then
            echo "ROCKBOXDEV: Getting MPFR"
            getfile "mpfr-3.1.6.tar.xz" "$GNU_MIRROR/mpfr"
            tar xJf $dlwhere/mpfr-3.1.6.tar.xz
            ln -s mpfr-3.1.6 mpfr
        fi

        if (echo $needs_libs | grep -q mpc && test ! -d mpc); then
            echo "ROCKBOXDEV: Getting MPC"
            getfile "mpc-1.0.1.tar.gz" "http://www.multiprecision.org/downloads"
            tar xzf $dlwhere/mpc-1.0.1.tar.gz
            ln -s mpc-1.0.1 mpc
        fi

        if (echo $needs_libs | grep -q isl && test ! -d isl); then
            echo "ROCKBOXDEV: Getting ISL"
            getfile "isl-0.15.tar.bz2" "https://gcc.gnu.org/pub/gcc/infrastructure"
            tar xjf $dlwhere/isl-0.15.tar.bz2
            ln -s isl-0.15 isl
        fi
        cd $builddir
    fi

    # GCC is special
    if [ "$toolname" == "gcc" ] ; then
	configure_params="--enable-languages=c --disable-libssp $configure_params"
    fi

    echo "ROCKBOXDEV: logging to $logfile"
    rm -f "$logfile"

    echo "ROCKBOXDEV: mkdir build-$toolname"
    mkdir build-$toolname

    cd build-$toolname

    echo "ROCKBOXDEV: $toolname/configure"
    CFLAGS='-U_FORTIFY_SOURCE -fgnu89-inline -fcommon -O2' CXXFLAGS='-std=gnu++03' run_cmd "$logfile" ../$toolname-$version/configure --target=$target --prefix=$prefix --disable-docs $configure_params

    echo "ROCKBOXDEV: $toolname/make"
    run_cmd "$logfile" $make $make_parallel

    echo "ROCKBOXDEV: $toolname/make install"
    run_cmd "$logfile" $make install

    cd ..

    echo "ROCKBOXDEV: rm -rf build-$toolname $toolname-$version"
    rm -rf build-$toolname $toolname-$version
}

# build a cross compiler toolchain for linux
# $1=target
# $2=binutils version
# $3=binutils configure extra options
# $4=gcc version
# $5=gcc configure extra options
# $6=linux version
# $7=glibc version
# $8=glibc configure extra options
build_linux_toolchain () {
    target="$1"
    binutils_ver="$2"
    binutils_opts="$3"
    gcc_ver="$4"
    gcc_opts="$5"
    linux_ver="$6"
    glibc_ver="$7"
    glibc_opts="$8"
    glibc_patches="$9"

    # where to put the sysroot
    sysroot="$prefix/$target/sysroot"
    # extract arch from target
    arch=${target%%-*}

    # check libraries:
    # contrary to other toolchains that rely on a hack to avoid installing
    # gmp, mpc and mpfr, we simply require that they are installed on the system
    # this is not a huge requirement since virtually all systems these days
    # provide dev packages for them
    # FIXME: maybe add an option to download and install them automatically
    checklib "mpc" "gmp" "mpfr"

    # create build directory
    if test -d $builddir; then
        if test ! -w $builddir; then
            echo "ROCKBOXDEV: No write permission for $builddir"
            exit
        fi
    else
        mkdir -p $builddir
    fi

    if [ "$makever" -gt 43 ] ; then
	glibc_make_opts="--jobserver-style=pipe --shuffle=none"
#	MAKEFLAGS="--jobserver-style=pipe --shuffle=none"
    fi

    # download all tools
    gettool "binutils" "$binutils_ver"
    gettool "gcc" "$gcc_ver"
    gettool "linux" "$linux_ver"
    gettool "glibc" "$glibc_ver"

    # extract them
    cd $builddir
    extract "binutils-$binutils_ver"
    extract "gcc-$gcc_ver"
    extract "linux-$linux_ver"
    extract "glibc-$glibc_ver"

    # do we have a patch?
    for p in $glibc_patches; do
        echo "ROCKBOXDEV: applying patch $p"
	(cd $builddir/glibc-$glibc_ver ; patch -p1 < "$patch_dir/$p")

	# check if the patch applied cleanly
        if [ $? -gt 0 ]; then
            echo "ROCKBOXDEV: failed to apply patch $p"
            exit
        fi
    done

    # we make it possible to restart a build on error by using the RBDEV_RESTART
    # variable, the format is RBDEV_RESTART="tool" where tool is the toolname at which
    # to restart (binutils, gcc)

    # install binutils, with support for sysroot
    buildtool "binutils" "$binutils_ver" "--target=$target --disable-werror \
        --with-sysroot=$sysroot --disable-nls" "" ""
    # build stage 1 compiler: disable headers, disable threading so that
    # pthread headers are not included, pretend we use newlib so that libgcc
    # doesn't get linked at the end
    # NOTE there is some magic involved here
    RESTART_STEP="gcc-stage1" \
    buildtool "gcc" "$gcc_ver" "$gcc_opts --enable-languages=c --target=$target \
        --without-headers --disable-threads --disable-libgomp --disable-libmudflap \
        --disable-libssp --disable-libquadmath --disable-libquadmath-support \
        --disable-shared --with-newlib --disable-libitm \
        --disable-libsanitizer --disable-libatomic" "" ""
    # install linux headers
    # NOTE: we need to tell make where to put the build files, since buildtool
    # switches to the builddir, "." will be the correct builddir when ran
    if [ "$arch" == "mipsel" ]; then
        arch="mips"
    fi
    linux_opts="O=. ARCH=$arch INSTALL_HDR_PATH=$sysroot/usr/"
    RESTART_STEP="linux-headers" \
    buildtool "linux" "$linux_ver" "NO_CONFIGURE" \
        "$linux_opts headers_install" "$linux_opts headers_check"
    # build glibc using the first stage cross compiler
    # we need to set the prefix to /usr because the glibc runs on the actual
    # target and is indeed installed in /usr
    RESTART_STEP="glibc" \
    prefix="/usr" \
    buildtool "glibc" "$glibc_ver" "--target=$target --host=$target --build=$MACHTYPE \
        --with-__thread --with-headers=$sysroot/usr/include $glibc_opts" \
        "$glibc_make_opts" "install install_root=$sysroot"
    # build stage 2 compiler
    RESTART_STEP="gcc-stage2" \
    buildtool "gcc" "$gcc_ver" "$gcc_opts --enable-languages=c,c++ --target=$target \
        --with-sysroot=$sysroot" "" ""
}

usage () {
    echo "usage: rockboxdev.sh [options]"
    echo "options:"
    echo "  --help              Display this help"
    echo "  --target=LIST       List of targets to build"
    echo "  --restart=STEP      Restart build at given STEP (same as RBDEV_RESTART env var)"
    echo "  --prefix=PREFIX     Set install prefix (same as RBDEV_PREFIX env var)"
    echo "  --dlwhere=DIR       Set download directory (same as RBDEV_DOWNLOAD env var)"
    echo "  --builddir=DIR      Set build directory (same as RBDEV_BUILD env var)"
    echo "  --makeflags=FLAGS   Set make flags (same as MAKEFLAGS env var)"
    exit 1
}

##############################################################################
# Code:

# Parse arguments
for i in "$@"
do
case $i in
    --help)
        usage
        ;;
    --prefix=*)
        prefix="${i#*=}"
        shift
        ;;
    --target=*)
        RBDEV_TARGET="${i#*=}"
        shift
        ;;
    --restart=*)
        RBDEV_RESTART="${i#*=}"
        shift
        ;;
    --dlwhere=*)
        dlwhere="${i#*=}"
        shift
        ;;
    --builddir=*)
        builddir="${i#*=}"
        shift
        ;;
    --makeflags=*)
        export MAKEFLAGS="${i#*=}" # export so it's available in make
        shift
        ;;
    *)
        echo "Unknown option '$i'"
        exit 1
        ;;
esac
done

# Verify required tools and libraries
for t in $reqtools; do
    tool=`findtool $t`
    if test -z "$tool"; then
        echo "ROCKBOXDEV: \"$t\" is required for this script to work."
        missingtools="yes"
    fi
done
if [ -n "$missingtools" ]; then
    echo "ROCKBOXDEV: Please install the missing tools and re-run the script."
    exit 1
fi

if ! $make -v | grep -q GNU ; then
    echo "ROCKBOXDEV: Building the rockbox toolchains requires GNU Make."
    exit 1
fi

dlwhere=$(readlink -f "$dlwhere")
prefix=$(readlink -f "$prefix")
builddir=$(readlink -f "$builddir")

echo "Download directory : $dlwhere (set RBDEV_DOWNLOAD or use --dlwhere= to change)"
echo "Install prefix     : $prefix  (set RBDEV_PREFIX or use --prefix= to change)"
echo "Build dir          : $builddir (set RBDEV_BUILD or use --builddir= to change)"
echo "Make options       : $MAKEFLAGS (set MAKEFLAGS or use --makeflags= to change)"
echo "Restart step       : $RBDEV_RESTART (set RBDEV_RESTART or use --restart= to change)"
echo "Target arch        : $RBDEV_TARGET (set RBDEV_TARGET or use --target= to change)"

# Verify download directory
if test -d "$dlwhere"; then
  if ! test -w "$dlwhere"; then
    echo "ROCKBOXDEV: No write permission for $dlwhere"
    exit
  fi
else
  mkdir $dlwhere
  if test $? -ne 0; then
    echo "ROCKBOXDEV: Failed creating directory $dlwhere"
    exit
  fi
fi


# Verify the prefix dir
if test ! -d $prefix; then
  mkdir -p $prefix
  if test $? -ne 0; then
      echo "ROCKBOXDEV: Failed creating directory $prefix"
      exit
  fi
fi
if test ! -w $prefix; then
  echo "ROCKBOXDEV: No write permission for $prefix"
  exit
fi

if [ -z "$RBDEV_TARGET" ]; then
    echo "Select target arch:"
    echo "m   - m68k     (iriver h1x0/h3x0, iaudio m3/m5/x5 and mpio hd200)"
    echo "a   - arm      (ipods, iriver H10, Sansa, D2, Gigabeat, older Sony NWZ, etc)"
    echo "i   - mips     (Jz47xx and ATJ-based players)"
    echo "x   - arm-linux  (Generic Linux ARM: Samsung ypr0, Linux-based Sony NWZ)"
    echo "y   - mips-linux  (Generic Linux MIPS: eg the many HiBy-OS targets)"
    echo "separate multiple targets with spaces"
    echo "(Example: \"m a i\" will build m68k, arm, and mips)"
    echo ""
    selarch=`input`
else
    selarch=$RBDEV_TARGET
fi
system=`uname -s`

# add target dir to path to ensure the new binutils are used in gcc build
PATH="$prefix/bin:${PATH}"

for arch in $selarch
do
    export MAKEFLAGS=`echo $MAKEFLAGS| sed 's/ -r / /'`  # We don't want -r
    echo ""
    case $arch in
        [Ii])
            build "binutils" "mipsel-elf" "2.26.1" "" "--disable-werror" "isl"
            build "gcc" "mipsel-elf" "4.9.4" "" "" "gmp mpfr mpc isl"
            ;;

        [Mm])
            build "binutils" "m68k-elf" "2.26.1" "" "--disable-werror" "isl"
            build "gcc" "m68k-elf" "4.9.4" "" "--with-arch=cf MAKEINFO=missing" "gmp mpfr mpc isl"
            ;;

        [Aa])
            binopts=""
            gccopts=""
            case $system in
                Darwin)
                    binopts="--disable-nls"
                    gccopts="--disable-nls"
                    ;;
            esac
            build "binutils" "arm-elf-eabi" "2.26.1" "" "$binopts --disable-werror" "isl"
            build "gcc" "arm-elf-eabi" "4.9.4" "rockbox-multilibs-noexceptions-arm-elf-eabi-gcc-4.9.4.diff" "$gccopts MAKEINFO=missing" "gmp mpfr mpc isl"
            ;;
        [Xx])
            # IMPORTANT NOTE
            # This toolchain must support several targets and thus must support
            # the oldest possible configuration.
            #
            # Samsung YP-R0/R1:
            #  ARM1176JZF-S, softfp EABI
            #   gcc: 4.9.4 is the latest 4.9.x stable branch, also the only one that
            #        compiles with GCC >6
            #   kernel: 2.6.27.59 is the same 2.6.x stable kernel as used by the
            #           original ct-ng toolchain, the device runs kernel 2.6.24
            #   glibc: 2.19 is the latest version that supports kernel 2.6.24 which
            #          is used on the device, but we need to support ABI 2.4 because
            #          the device uses glibc 2.4.2
            #
            # Sony NWZ:
            #   gcc: 4.9.4 is the latest 4.9.x stable branch, also the only one that
            #        compiles with GCC >6
            #   kernel: 2.6.32.68 is the latest 2.6.x stable kernel, the device
            #           runs kernel 2.6.23 or 2.6.35 or 3.x for the most recent
            #   glibc: 2.19 is the latest version that supports kernel 2.6.23 which
            #          is used on many Sony players, but we need to support ABI 2.7
            #          because the device uses glibc 2.7
            #
            # Thus the lowest common denominator is to use the latest 2.6.x stable
            # kernel but compile glibc to support kernel 2.6.23 and glibc 2.4.
            # We use a recent 2.26.1 binutils to avoid any build problems and
            # avoid patches/bugs.
            glibcopts="--enable-kernel=2.6.23 --enable-oldest-abi=2.4"
            build_linux_toolchain "arm-rockbox-linux-gnueabi" "2.26.1" "" "4.9.4" \
                "$gccopts" "2.6.32.68" "2.19" "$glibcopts" "glibc-220-make44.patch"
            # build alsa-lib
            # we need to set the prefix to how it is on device (/usr) and then
            # tweak install dir at make install step
            alsalib_ver="1.0.19"
            gettool "alsa-lib" "$alsalib_ver"
            extract "alsa-lib-$alsalib_ver"
            prefix="/usr" buildtool "alsa-lib" "$alsalib_ver" \
                "--host=$target --disable-python" "" "install DESTDIR=$prefix/$target/sysroot"
            ;;
        [Yy])
            # IMPORTANT NOTE
            # This toolchain must support several targets and thus must support
            # the oldest possible configuration.
            #
            # AGPTek Rocker:
            #   XBurst release 1 (something inbetween mips32r1 and mips32r2)
            #   gcc: 4.9.4 is the latest 4.9.x stable branch, also the only one that
            #        compiles with GCC >6
            #   kernel: 3.10.14
            #   glibc: 2.16
            #   alsa: 1.0.29
            #
            # FiiO M3K Linux:
            #   kernel: 3.10.14
            #   glibc: 2.16
            #   alsa: 1.0.26
            #
            # To maximize compatibility, we use kernel 3.2.85 which is the lastest
            # longterm 3.2 kernel and is supported by the latest glibc, and we
            # require support for up to glibc 2.16
            # We use a recent 2.26.1 binutils to avoid any build problems and
            # avoid patches/bugs.
            glibcopts="--enable-kernel=3.2 --enable-oldest-abi=2.16"
            # FIXME: maybe add -mhard-float
            build_linux_toolchain "mipsel-rockbox-linux-gnu" "2.26.1" "" "4.9.4" \
                "$gccopts" "3.2.85" "2.25" "$glibcopts" "glibc-225-make44.patch"
            # build alsa-lib
            # we need to set the prefix to how it is on device (/usr) and then
            # tweak install dir at make install step
            alsalib_ver="1.0.26"
            gettool "alsa-lib" "$alsalib_ver"
            extract "alsa-lib-$alsalib_ver"
            prefix="/usr" buildtool "alsa-lib" "$alsalib_ver" \
                "--host=$target --disable-python" "" "install DESTDIR=$prefix/$target/sysroot"
            # build libffi
	    libffi_ver="3.2.1"
	    gettool "libffi" "$libffi_ver"
	    extract "libffi-$libffi_ver"
	    prefix="/usr" buildtool "libffi" "$libffi_ver" \
               "--includedir=/usr/include --host=$target" "" "install DESTDIR=$prefix/$target/sysroot"
            (cd $prefix/$target/sysroot/usr/include ; ln -sf ../lib/libffi-$libffi_ver/include/ffi.h . ;  ln -sf ../lib/libffi-$libffi_ver/include/ffitarget.h . )

            # build zlib
	    zlib_ver="1.2.13"  # Target uses 1.2.8!
	    gettool "zlib" "$zlib_ver"
	    extract "zlib-$zlib_ver"
	    CHOST=$target prefix="/usr" buildtool "zlib" "$zlib_ver" \
                "" "" "install DESTDIR=$prefix/$target/sysroot"

            # build glib
	    glib_ver="2.46.2"
	    gettool "glib" "$glib_ver"
	    extract "glib-$glib_ver"
	    prefix="/usr" buildtool "glib" "$glib_ver" \
               "--host=$target --with-sysroot=$prefix/$target/sysroot --disable-libelf glib_cv_stack_grows=no glib_cv_uscore=no ac_cv_func_posix_getpwuid_r=yes ac_cv_func_posix_getgrgid_r=yes" "" "install DESTDIR=$prefix/$target/sysroot"

            # build expat
	    expat_ver="2.1.0"
	    gettool "expat" "$expat_ver"
	    extract "expat-$expat_ver"
	    prefix="/usr" buildtool "expat" "$expat_ver" \
               "--host=$target --includedir=/usr/include --enable-abstract-sockets" "" "install DESTDIR=$prefix/$target/sysroot "

            # build dbus
	    dbus_ver="1.10.2"
	    gettool "dbus" "$dbus_ver"
	    extract "dbus-$dbus_ver"
	    prefix="/usr" buildtool "dbus" "$dbus_ver" \
               "--host=$target --with-sysroot=$prefix/$target/sysroot --includedir=/usr/include --enable-abstract-sockets ac_cv_lib_expat_XML_ParserCreate_MM=yes --disable-systemd --disable-launchd --enable-x11-autolaunch=no --with-x=no -disable-selinux --disable-apparmor --disable-doxygen-docs " "" "install DESTDIR=$prefix/$target/sysroot "


            ;;
        *)
            echo "ROCKBOXDEV: Unsupported architecture option: $arch"
            exit
            ;;
    esac
done

echo ""
echo "ROCKBOXDEV: Done!"
echo ""
echo "ROCKBOXDEV: Make sure your PATH includes $prefix/bin"
echo ""
