#!/bin/sh
#
# This script will prepare your Rockbox source folder for billiards compilation.
#
SCRIPT_PATH="${0%/*}"
if [[ ("$0" != "$SCRIPT_PATH") && ("$SCRIPT_PATH" != "") ]]; then
    cd $SCRIPT_PATH
fi
function print_help
{
    echo "Usage: `basename $0` [options] directory"
    echo "Options:"
    echo " -h, --help Display this information"
    echo " -p, --path <directory> Use <directory> as your Rockbox source path"
}
ROCKBOX_DIR=""
# parse arguments
while [[ $# > 0 ]]
do
    case "$1" in
        -p|--path)
            shift
            ROCKBOX_DIR="$1"
            ;;
        -h|--help)
            print_help
            exit 0
            ;;
        *)
            ;;
    esac
    shift
done
if [[ "$ROCKBOX_DIR" == "" ]]; then
    echo "Error: Rockbox source folder not specified"
    print_help
    exit 1
fi
# stop script on error
set -e
echo "Copying billiards sources to $ROCKBOX_DIR/apps/plugins/billiards ..."
mkdir -p $ROCKBOX_DIR/apps/plugins/billiards/src
mkdir -p $ROCKBOX_DIR/apps/plugins/billiards/platforms/rockbox
cp rockbox.c rockbox.h $ROCKBOX_DIR/apps/plugins/billiards/platforms/rockbox
cp ../../src/*.c ../../src/*.h $ROCKBOX_DIR/apps/plugins/billiards/src
cp ../../SOURCES ../../billiards.make $ROCKBOX_DIR/apps/plugins/billiards/
if [[ `tail $ROCKBOX_DIR/apps/plugins/SUBDIRS -n 1` != "billiards" ]]; then
    echo "Appending billiards to $ROCKBOX_DIR/apps/plugins/SUBDIRS ..."
    echo "billiards" >> $ROCKBOX_DIR/apps/plugins/SUBDIRS
fi
if [[ `tail $ROCKBOX_DIR/apps/plugins/CATEGORIES -n 1` != "billiards,games" ]]; then
    echo "Appending billiards to $ROCKBOX_DIR/apps/plugins/CATEGORIES ..."
    echo "billiards,games" >> $ROCKBOX_DIR/apps/plugins/CATEGORIES
fi
echo "Done."
