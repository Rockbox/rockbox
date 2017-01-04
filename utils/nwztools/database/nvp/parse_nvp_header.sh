#!/bin/bash
# usage
# parse_header.sh /path/to/icx_nvp.h
# parse_header.sh /path/to/linux/source [file]
# parse_header.sh /path/to/linux.tgz [file]
# the optional argument is an additional filter when there are several matches
# and is an argument to grep
#
if [ "$#" -lt 1 ]; then
    >&2 echo "usage: parse_header.sh /path/to/icx_nvp.h|/path/to/kernel/source|/path/to/kernel.tgz [filter]"
    exit 1
fi

FILE="$1"
IN_TGZ="0"
FILTER="$2"

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

# apply user filter
if [ "$FILTER" != "" ]; then
    LIST=`echo "$LIST" | grep "$FILTER"`
fi
# if file contains icx_nvp_emmc.h, we want that
if [ "`echo "$LIST" | grep "icx_nvp_emmc.h" | wc -l`" = "1" ]; then
    LIST=`echo "$LIST" | grep "icx_nvp_emmc.h"`
else
    LIST=`echo "$LIST" | grep 'icx[[:digit:]]*_nvp[[:alpha:]_]*.h'`
fi
LIST_CNT=`echo "$LIST" | wc -l`
if [ "$LIST_CNT" = "0" ]; then
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

cat "$FILE" | awk ' \
BEGIN { \
    expr = "#define[[:space:]]+ICX_NVP_NODE_([[:alnum:]]+)[[:space:]]+ICX_NVP_NODE_BASE[[:space:]]*\"([[:digit:]]+)\""; \
} \
{ \
    if($0 ~ expr) \
    { \
        print(tolower(gensub(expr, "\\1,\\2", "g", $0)));
    } \
}'
