#/bin/bash

TMPDIR="./tmp-check-defines"
SEARCH_DIRS="apps/ firmware/ uisimulator/"

function usage()
{
    echo "Usage: $0 <build file> <define 1> [<define 2>...]"
    echo ""
    echo "This tool tries to detect which macros are defined in each build,"
    echo "it uses the www repository build list to find the targets to build."
    echo "By default it ignores wps and sim builds, feel free to hack the code,"
    echo "if you want to change this. The tool can not only detect which macros"
    echo "get define but also print the values and try to pretty print it."
    echo ""
    echo "If the define name is suffixed with @ or ~, the tool will also print its value"
    echo "Furthermore, if a regex follows the @, the tool will try to list all defines"
    echo "matching this regex to pretty print the output"
    echo "If the define name is suffixed with ~, the tool will try to list all definitions"
    echo "of the macro and compare the right hand side value with the actual value"
    echo "to pretty print the value"
    echo ""
    echo "IMPORTANT: the tools needs to be ran from the root directory of the"
    echo "           repository"
    echo ""
    echo "Examples:"
    echo "$0 ../www/buildserver/builds HAVE_USBSTACK "
    echo "$0 ../www/buildserver/builds 'CONFIG_LCD@LCD_[[:alnum:]_]*'"
    echo "$0 ../www/buildserver/builds CONFIG_CPU~"
    exit 1
}

# pattern_check define file
function pattern_check()
{
    cat >>"$2" <<EOF
#ifdef $1
#pragma message "cafebabe-$1"
#else
#pragma message "deadbeef-$1"
#endif
EOF
}

# pattern_check define file
function pattern_value()
{
    cat >>"$2" <<EOF
#ifdef $1
#pragma message "cafebabe-$1@"MYEVAL($1)"@"
#else
#pragma message "deadbeef-$1"
#endif
EOF
}

# pattern_check define regex file
function pattern_regex()
{
    # not implemented yet
    cat >>"$3" <<EOF
#ifdef $1
#pragma message "cafebabe-$1@"MYEVAL($1)"@"
#else
#pragma message "deadbeef-$1"
#endif
EOF
}

# pattern_list define file
function pattern_list()
{
    cat >>"$2" <<EOF
#if !defined($1)
#pragma message "deadbeef-$1"
EOF
    # find all occurences
    grep -REh "^#define[[:space:]]+$1[[:space:]]+" $SEARCH_DIRS |
        awk -v "DEF=$1" '{ match($0,/#define[[:space:]]+[[:alnum:]_]+[[:space:]]+(.*)/,arr);
            print("#elif "DEF" == ("arr[1]")");
            print("#pragma message \"cafebabe-"DEF"@"arr[1]"@\""); }' >> $2
cat >>"$2" <<EOF
#else
#pragma message "cafebabe-$1@"MYEVAL($1)"@"
#endif
EOF
}

# check number of arguments
if [ $# -le 1 ]; then
    usage
fi

# try to check that the tool is ran from the root directory
if [ ! -f "tools/configure" ]; then
    echo "You must run this tool from the root directory of the repository"
    exit 2
fi

# print invocation, for reference
echo "#invocation $0 $*"

# build file
BUILD_FILE="$1"
shift

# check build file
if [ ! -f $BUILD_FILE ]; then
    echo "ERROR: build file doesn't exist"
    exit 2
fi

# fill template test file
TMPLFILE=template.c
cat > "$TMPLFILE" <<EOF
/* Automatically generated. http://www.rockbox.org/ */
#include "system.h"
#include "usb.h"
#include "usb_ch9.h"
/* add more includes here if needed */
#define MYEVAL_(x) #x
#define MYEVAL(x) MYEVAL_(x)
EOF
# generate test pattern for each define
for DEFINE in "$@"
do
    # check if define must be printed (named suffixed by @)
    if [[ "$DEFINE" =~ "@" ]]; then
        REGEX=`echo "$DEFINE" | awk '{ match($0,/@(.*)$/,arr); print(arr[1]); }'`
        DEFINE=`echo "$DEFINE" | awk '{ match($0,/([^@]*)@/,arr); print(arr[1]); }'`
        # if regex is empty, it's simple
        if [ "$REGEX" = "" ]; then
            pattern_value "$DEFINE" "$TMPLFILE"
        else
            pattern_regex "$DEFINE" "$REGEX" "$TMPLFILE"
        fi
    elif [[ "$DEFINE" =~ "~" ]]; then
        DEFINE=`echo "$DEFINE" | awk '{ match($0,/([^~]*)~/,arr); print(arr[1]); }'`
        pattern_list "$DEFINE" "$TMPLFILE"
    else
        pattern_check "$DEFINE" "$TMPLFILE"
    fi
done

# process lines in the build
while read line
do
    # ignore wps and manual lines, also sim
    if `echo "$line" | grep -E "(wps)|(manual)|(sim)" &> /dev/null`; then
        continue
    fi
    # format of each line: compiler:x:name:title:file:hint:command
    NAME=`echo "$line" | awk 'BEGIN { FS=":" } { print $3 }'`
    CMD=`echo "$line" | awk 'BEGIN { FS=":" } { print $7 }'`
    # remove temporary directory
    rm -rf "$TMPDIR"
    # create temporary directory and cd into it
    mkdir "$TMPDIR"
    cd "$TMPDIR"
    # create a fake generated file
    TESTFILE=mytest
    cp "../$TMPLFILE" "$TESTFILE.c"
    # the command extract is of the form
    # ../tools/configure <options> && make <blabla>
    # extract the part before && and replace the whole thing with:
    # ../tools/configure <options> && make $TESTFILE.o
    CFGCMD=`echo "$CMD" | awk '{ i=index($0, "&&"); print(substr($0,1,i-1)); }'`
    # try to run configure
    # it might error if the target compiler is not installed for example
    if eval $CFGCMD &>/dev/null; then
        # pretend dependencies were generated (because it's very long)
        touch make.dep
        # try to build our test file
        OUTPUT=out.txt
        ERROR=err.txt
        if ! make $TESTFILE.o >$OUTPUT 2>$ERROR; then
            # print make error
            echo "$NAME: <make error>"
        else
            # print defines
            echo "$NAME:" `cat "$ERROR" | awk 'BEGIN { list="" }
                { if(match($0,/cafebabe-([[:alnum:]_]+)/,arr) != 0) list = list " " arr[1]; 
                  if(match($0,/@([^@]*)@/,arr)) list = list "=" arr[1]; }
                END { print(list); }'`
        fi
    else
        # print config error
        echo "$NAME: <config error>"
    fi
    # cd back to root
    cd ..
done < "$BUILD_FILE"

# remove everything
rm -rf "$TMPDIR"
rm -f "$TMPLFILE"