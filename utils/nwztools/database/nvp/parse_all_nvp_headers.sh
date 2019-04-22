#!/bin/bash
# usage: parse_all_nvp_headers /path/to/directory
#
# the expected structure is:
#   nwz-*/<kernel>
#   nw-*/<kernel/
# where <kernel> must be of the form
#   linux-kernel-*.tgz
#
function usage
{
    >&2 echo "usage: parse_all_nvp_header.sh [--new-only] /path/to/directory"
    exit 1
}

if [ "$#" -lt 1 ]; then
    usage
fi

new_only="no"
if [ "$1" = "--new-only" ]; then
    shift
    new_only="yes"
fi

# list interesting directories
for dir in `find "$1" -maxdepth 1 -name "nw-*" -or -name "nwz-*" -or -name "dmp-*"`
do
    # extract codename
    codename=`basename "$dir"`
    # only consider linux based targets
    if [ -e "$dir/not_linux" ]; then
        #echo "$codename: not linux based"
        continue
    fi
    # skip if not new
    if [ $new_only = "yes" ] && [ -e $codename.txt ]; then
        echo "Skip $codename: already exists"
        continue
    fi
    # check if we can find a kernel
    kernel_tgz=`find "$dir" -maxdepth 1 -name "linux-kernel-*.tgz"`
    if [ "$kernel_tgz" == "" ]; then
        echo "$codename: no kernel found"
        continue
    fi
    echo "$codename: found kernel `basename $kernel_tgz`"
    ./parse_nvp_header.sh "$kernel_tgz" > "$codename.txt"
done
