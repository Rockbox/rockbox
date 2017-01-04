#!/bin/bash
# usage: parse_all_nvp_nodes /path/to/directory
#
# the expected structure is:
#   nwz-*/rootfs.tar
#   nwz-*/rootfs.tar
#
if [ "$#" -lt 1 ]; then
    >&2 echo "usage: parse_all_nvp_header.sh /path/to/directory"
    exit 1
fi

# list interesting directories
for dir in `find "$1" -maxdepth 1 -name "nw-*" -or -name "nwz-*"`
do
    # extract codename
    codename=`basename "$dir"`
    # only consider linux based targets
    if [ -e "$dir/not_linux" ]; then
        #echo "$codename: not linux based"
        continue
    fi
    # check if we can find a rootfs
    rootfs_tgz="$dir/rootfs.tgz"
    if [ ! -e "$rootfs_tgz" ]; then
        echo "$codename: no rootfs found"
        continue
    fi
    echo "$codename: found rootfs `basename $rootfs_tgz`"
    ./parse_nvp_nodes.sh "$rootfs_tgz" "nodes-$codename.txt"
done

