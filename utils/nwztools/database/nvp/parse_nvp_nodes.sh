#!/bin/bash
# usage
# parse_nodes.sh /path/to/icx_nvp_emmc.ko output_file
# parse_nodes.sh /path/to/rootfs/dir output_file
# parse_nodes.sh /path/to/rootfs.tgz output_file
#
if [ "$#" -lt 2 ]; then
    >&2 echo "usage: parse_header.sh /path/to/icx_nvp.ko|/path/to/rootfs/dir|/path/to/rootfs.tgz output_file"
    exit 1
fi

FILE="$1"
IN_TGZ="0"
OUTPUT="$2"

# for directories, list all files
if [ -d "$FILE" ]; then
    LIST=`find "$FILE"`
else
    # otherwise try un unpack it using tar, to see if it's an archive
    LIST=`tar -tzf "$FILE"`
    if [ "$?" == 0 ]; then
        IN_TGZ="1"
        TGZ="$FILE"
    else
        # assume the input is just the right file
        LIST="$FILE"
    fi
fi

# look for icx_nvp_emmc.ko
LIST=`echo "$LIST" | grep "icx[[:digit:]]*_nvp[[:alpha:]_]*.ko"`
LIST_CNT=`echo "$LIST" | wc -l`
if [ "$LIST" == "" ]; then
    >&2 echo "No icx nvp file found" 
    exit 1
elif [ "$LIST_CNT" != "1" ]; then
    >&2 echo "Several matches for icx nvp:"
    >&2 echo "$LIST"
    exit 1
else
    FILE="$LIST"
fi

# if file is in archive, we need to extract it
if [ "$IN_TGZ" = "1" ]; then
    >&2 echo "Extracting $FILE from $TGZ"
    TMP=`mktemp`
    tar -Ozxf "$TGZ" "$FILE" > $TMP
    if [ "$?" != 0 ]; then
        >&2 echo "Extraction failed"
        exit 1
    fi
    FILE="$TMP"
else
    >&2 echo "Analyzing $FILE"
fi

./nvptool -x "$FILE" -o "$OUTPUT" >/dev/null
