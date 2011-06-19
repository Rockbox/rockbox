#!/bin/sh
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#
# Spell check files from the Rockbox manual.
#
# Call the script with a single file to spellcheck. Call it without arguments
# to spellcheck all files in the manual tree except preamble.tex.
# This will invoke aspell interactively.

MANDIR=$(dirname $0)
if [ $# -gt 0 ]; then
    TEX_FILES="$1"
else
    TEX_FILES=$(find "$MANDIR" -name "*.tex" | sed -e "s/\S*preamble.tex//")
fi

for file in $TEX_FILES; do
    aspell -t -l en_UK-ise --add-tex-command="opt pp" \
    --add-tex-command="nopt pp" --add-tex-command="screenshot ppo" \
    --add-tex-command="reference p" --add-tex-command="fname p" \
    --add-tex-command="wikilink p" --add-tex-command="IfFileExists p" \
    --add-tex-command="newcommand pp" --add-tex-command="renewcommand pp" \
    --add-tex-command="download p" \
    -c $file
done
