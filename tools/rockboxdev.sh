#!/bin/sh

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

if [ -z $GNU_MIRROR ] ; then
    GNU_MIRROR=ftp://ftp.gnu.org/pub/gnu
fi

# If detection fails, override the value of make manually:
# make="make"

##############################################################################

#
# These are the tools this script requires and depends upon.
reqtools="gcc bzip2 make patch"


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

input() {
    read response
    echo $response
}

#$1 file
#$2 URL"root
getfile() {
  tool=`findtool curl`
  if test -z "$tool"; then
    tool=`findtool wget`
    if test -n "$tool"; then
      # wget download
      echo "ROCKBOXDEV: Downloading $2/$1 using wget"
      $tool -O $dlwhere/$1 $2/$1
    fi
  else
     # curl download
      echo "ROCKBOXDEV: Downloading $2/$1 using curl"
     $tool -Lo $dlwhere/$1 $2/$1
  fi

  if [ $? -ne 0 ] ; then
      echo "ROCKBOXDEV: couldn't download the file!"
      echo "ROCKBOXDEV: check your internet connection"
      exit
  fi

  if test -z "$tool"; then 
    echo "ROCKBOXDEV: No downloader tool found!"
    echo "ROCKBOXDEV: Please install curl or wget and re-run the script"
    exit
  fi
}

for t in $reqtools; do
  tool=`findtool $t`
  if test -z "$tool"; then
    echo "ROCKBOXDEV: \"$t\" is required for this script to work."
    echo "ROCKBOXDEV: Please install \"$t\" and re-run the script."
    exit
  fi
done

###########################################################################
# Verify download directory or create it
if test -d "$dlwhere"; then
  if ! test -w "$dlwhere"; then
    echo "$dlwhere exists, but doesn't seem to be writable for you"
    exit
  fi
else
  mkdir $dlwhere
  if test $? -ne 0; then
    echo "$dlwhere is missing and we failed to create it!"
    exit
  fi
  echo "$dlwhere has been created to store downloads in"
fi

echo "Download directory: $dlwhere (set RBDEV_DOWNLOAD to change dir)"
echo "Install prefix: $prefix/[target] (set RBDEV_PREFIX to change dir)"
echo "Build dir: $builddir (set RBDEV_BUILD to change dir)"

###########################################################################
# Verify that the prefix dir exists and that we can write to it,
#  as otherwise we will hardly be able to install there!
if test ! -d $prefix; then
  echo "ERROR: The installation destination does not exist."
  echo "Please create it and re-run this script"
  exit
fi
if test ! -w $prefix; then
  echo "ERROR: This script is set to install in $prefix but has no write permissions for it"
  echo "Please fix this and re-run this script"
  exit
fi



cleardir () {
  # $1 is the name of the build dir
  # $2 is the arch
  # delete the build dirs and the source dirs
  echo "Cleaning up build folder"
  rm -rf $1/build-gcc-$2 $1/build-binu-$2
}

buildone () {

arch=$1
gccpatch="" # default is no gcc patch
gccver="4.0.3" # default gcc version
binutils="2.16.1" # The binutils version to use
gccconfigure="" #default is nothing added to configure
binutilsconf="" #default is nothing added to configure
gcctarget="" #default make target
gccinstalltarget="install" #default install target

system=`uname -s`
gccurl="http://www.rockbox.org/gcc"

case $arch in
  [Ss])
    target="sh-elf"
    gccpatch="gcc-4.0.3-rockbox-1.diff"
    ;;
  [Mm])
    target="m68k-elf"
    gccver="3.4.6"
    case $system in
      CYGWIN* | Darwin | FreeBSD | Interix | SunOS)
        gccpatch="gcc-3.4.6.patch"
        ;;
      Linux)
        machine=`uname -m`
        case $machine in
          x86_64)
            gccpatch="gcc-3.4.6-amd64.patch"
            ;;
        esac
        ;;
      *)
        echo "Unknown host system $system detected - check necessity of" \
             "patch for ${target}-gcc $gccver and add it."
        echo "Press enter to continue..."
        read ignore
        ;;
    esac
    ;;
  [Aa])
    target="arm-elf"
    gccpatch="rockbox-multilibs-arm-elf-gcc-4.0.3_3.diff"
    ;;
  [Ee])
    target="arm-elf-eabi"
    gccpatch="rockbox-multilibs-noexceptions-arm-elf-eabi-gcc-4.4.2_1.diff"
    gccver="4.4.3"
    # needed to build a bare-metal gcc-4.4.2
    gcctarget="all-gcc all-target-libgcc"
    gccinstalltarget="install-gcc install-target-libgcc"
    binutils="2.20.1"
    ;;
  [Ii])
    target="mipsel-elf"
    gccver="4.1.2"
    binutils="2.17"
    gccconfigure="--disable-libssp"
    binutilsconf="--disable-werror"
    # necessary to make binutils build on gcc 4.3+
    case $system in
      Interix)
        gccpatch="gcc-4.1.2-interix.diff"
        ;;
      *)
        ;;
    esac
    ;;
  *)
    echo "An unsupported architecture option: $arch"
    exit
    ;;
esac

bindir="$prefix/$target/bin"
if test -n $pathadd; then
  pathadd="$pathadd:$bindir"
else
  pathadd="$bindir"
fi

echo ""
echo "In case you encounter a slow internet connection, you can use an alternative mirror."
echo "A list of other GNU mirrors is available here: http://www.gnu.org/prep/ftp.html"
echo ""
echo "Usage: GNU_MIRROR=[URL] ./rockboxdev.sh"
echo ""
echo "Example:"
echo "$ GNU_MIRROR=http://mirrors.kernel.org/gnu ./rockboxdev.sh"
echo ""

if test -f "$dlwhere/binutils-$binutils.tar.bz2"; then
  echo "binutils $binutils already downloaded"
else
  getfile binutils-$binutils.tar.bz2 $GNU_MIRROR/binutils
fi

if test -f "$dlwhere/gcc-core-$gccver.tar.bz2"; then
  echo "gcc $gccver already downloaded"
else
  getfile gcc-core-$gccver.tar.bz2 $GNU_MIRROR/gcc/gcc-$gccver
fi

if test -n "$gccpatch"; then
  if test -f "$dlwhere/$gccpatch"; then
    echo "$gccpatch already downloaded"
  else
    getfile "$gccpatch" "$gccurl"
  fi
fi

###########################################################################
# If there's already a build dir, we don't overwrite or delete it
if test -d $builddir; then
  if test ! -w $builddir; then
    echo "ERROR: No write permissions for the build directory!"
    exit
  fi
else
  mkdir -p $builddir
fi

cd $builddir

###########################################################################
# Create a summary file for each toolchain, containing info about the version
# and a remainder to append the compiler path to PATH

summary="summary-$1"

echo "============================ Summary ============================" > $summary
echo "target:              $target" >> $summary
echo "gcc version:         $gccver" >> $summary
if test -n "$gccpatch"; then
    echo "gcc patch:           $gccpatch" >> $summary
fi
echo "binutils:            $binutils" >> $summary
echo "installation target: $prefix/$target" >> $summary
echo "" >> $summary
echo "When done, append $bindir to PATH" >> $summary
echo "=================================================================" >> $summary

cat $summary

echo "ROCKBOXDEV: extracting binutils-$binutils in $builddir"
bunzip2 < $dlwhere/binutils-$binutils.tar.bz2 | tar xf -
echo "ROCKBOXDEV: extracting gcc-$gccver in $builddir"
bunzip2 < $dlwhere/gcc-core-$gccver.tar.bz2 | tar xf -

if test -n "$gccpatch"; then
  echo "ROCKBOXDEV: applying gcc patch"
  # apply the patch and hope it runs well - don't be dependant on the
  # exact gcc version, thus strip the gcc folder
  (cd $builddir/gcc-$gccver && patch -p1 < "$dlwhere/$gccpatch")
    if [ $? -gt 0 ]; then # check if the applied cleanly
      echo "ROCKBOXDEV: failed to apply the gcc patch"
      exit
    fi
fi

echo "ROCKBOXDEV: mkdir build-binu-$1"
mkdir build-binu-$1
echo "ROCKBOXDEV: cd build-binu-$1"
cd build-binu-$1
echo "ROCKBOXDEV: binutils/configure"
# we undefine _FORTIFY_SOURCE to make the binutils built run fine on recent
# Ubuntu installations
CFLAGS=-U_FORTIFY_SOURCE ../binutils-$binutils/configure --target=$target --prefix=$prefix/$target $binutilsconf
echo "ROCKBOXDEV: binutils/make"
# We add -r when building binutils to fix compilation on OSX - Apple's make has
# extra built-ins which cause problems.
$make -r
echo "ROCKBOXDEV: binutils/make install to $prefix/$target"
$make install
cd .. # get out of build-binu-$1
PATH="$bindir:${PATH}"
SHELL=/bin/sh # seems to be needed by the gcc build in some cases

echo "ROCKBOXDEV: mkdir build-gcc-$1"
mkdir build-gcc-$1
echo "ROCKBOXDEV: cd build-gcc-$1"
cd build-gcc-$1
echo "ROCKBOXDEV: gcc/configure"
# we undefine _FORTIFY_SOURCE to make the gcc build go through fine on
# recent Ubuntu installations
CFLAGS=-U_FORTIFY_SOURCE ../gcc-$gccver/configure --target=$target --prefix=$prefix/$target --enable-languages=c $gccconfigure
echo "ROCKBOXDEV: gcc/make"
$make $gcctarget
echo "ROCKBOXDEV: gcc/make install to $prefix/$target"
$make $gccinstalltarget
cd .. # get out of build-gcc
cd .. # get out of $builddir

} # buildone()

echo ""
echo "Select target arch:"
echo "s   - sh       (Archos models)"
echo "m   - m68k     (iriver h1x0/h3x0, ifp7x0 and iaudio)"
echo "a   - arm      (ipods, iriver H10, Sansa, etc)"
echo "e   - arm-eabi (same as above, new testing toolchain)"
echo "i   - mips     (Jz4740 and ATJ-based players)"
echo "separate multiple targets with spaces"
echo "(Example: \"s m a\" will build sh, m86k and arm)"
echo ""

selarch=`input`

for arch in $selarch
do
echo ""
case $arch in
  [Ss])
    buildone $arch
    cleardir $builddir $arch
    ;;
  [Ii])
    buildone $arch
    cleardir $builddir $arch
    ;;
  [Mm])
    buildone $arch
    cleardir $builddir $arch
    ;;
  [Aa])
    buildone $arch
    cleardir $builddir $arch
    ;;
  [Ee])
    buildone $arch
    cleardir $builddir $arch
    ;;
  *)
    echo "An unsupported architecture option: $arch"
    exit
    ;;
esac
done

# show the summaries:
cat $builddir/summary-*
rm $builddir/summary-*

echo ""
echo "Done!"
echo ""
echo "Make your PATH include $pathadd"
echo ""
