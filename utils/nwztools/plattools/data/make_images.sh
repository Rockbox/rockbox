#!/bin/bash
#
# This script contains the code used to produce all the images.
# Because of the variety of tools needed to achieve that, the result is also
# included in the repository but this makes it easier to modify the data
# to add more content
#

# path to root of repository
ROOT_DIR=../../../../

# final resolution
NWZ_WIDTH=130
NWZ_HEIGHT=130

# path to rockbox icon
RB_ICON_PATH=$ROOT_DIR/docs/logo/rockbox-icon.svg
# path to tools icon (currently stolen from KDE Oxygen icon set)
TOOL_ICON_PATH=Oxygen480-categories-preferences-system.svg

# convert_svg width height input output
function convert_svg
{
    local width="$1"
    local height="$2"
    local input="$3"
    local output="$4"
    TMP=tmp.png
    # convert from SVG to PNG
    inkscape -z -e $TMP -w $width -h $height $input
    if [ "$?" != 0 ]; then
        echo "SVG -> PNG conversion failed"
        exit 1
    fi
    # convert from PNG to BMP, force using "version 3" because the OF don't like
    # "recent" BMP
    convert -channel RGB $TMP -define bmp:format=bmp3 ${output}_icon.bmp
    if [ "$?" != 0 ]; then
        rm -f $TMP
        echo "PNG -> BMP conversion failed"
        exit 1
    fi
    # remove temporary
    rm -f $TMP
}

# start by creating the bitmap files from rockbox-icon.svg for all resolutions
# we make a detour by svg because inkscape can only export to SVG
# NOTE: we use image magick to convert to bmp but the OF tools don't like BMPv5
# and contrary to what the documentation says, image magick tends to produce
# those by default unless asked otherwise
convert_svg $NWZ_WIDTH $NWZ_HEIGHT $RB_ICON_PATH rockbox
convert_svg $NWZ_WIDTH $NWZ_HEIGHT $TOOL_ICON_PATH tools
