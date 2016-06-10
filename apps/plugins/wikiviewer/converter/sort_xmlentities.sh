#!/bin/bash

# Helper script which sorts xmlentities.h

START=$(grep -m 1 -nF 'ENT("' xmlentities.h | sed 's/\([0-9]*\).*/\1/')
END=$(grep -nF 'ENT("' xmlentities.h | tail -n 1 | sed 's/\([0-9]*\).*/\1/')

head -n $(($START - 1)) xmlentities.h > xmlentities.h.new
grep 'ENT("' xmlentities.h | LC_ALL=C sort -b | uniq >> xmlentities.h.new
awk "NR > $END { print \$0 }" xmlentities.h >> xmlentities.h.new

rm xmlentities.h
mv xmlentities.h.new xmlentities.h
