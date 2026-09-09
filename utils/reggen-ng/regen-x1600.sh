#!/bin/sh
# Copyright (C) 2026 Michael McAllister
# Regenerate the X1600 headers and scope their include guards to this SoC.
# Usage: regen-x1600.sh [outdir]; defaults to the committed header directory.
# Requires python3 with lxml and utils/regtools/headergen_v2.
set -e

HERE="$(dirname "$0")"
ROOT="$HERE/../.."
DEST="${1:-$ROOT/firmware/target/mips/ingenic_x1600/x1600}"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

python3 "$HERE/reggen-ng.py" < "$HERE/x1600.reggen" > "$TMP/x1600.xml"
[ -s "$TMP/x1600.xml" ] || { echo "reggen-ng.py produced nothing" >&2; exit 1; }

"$ROOT/utils/regtools/headergen_v2" -g jz -o "$TMP/out" "$TMP/x1600.xml"

# Check every described block, including when only part of the SoC is present.
python3 - "$TMP/x1600.xml" "$TMP/out" <<'PYCHECK'
import pathlib
import sys
import xml.etree.ElementTree as ET

root = ET.parse(sys.argv[1]).getroot()
expected = {node.findtext('name').lower() + '.h' for node in root.findall('node')}
expected.add('macro.h')
out = pathlib.Path(sys.argv[2])
actual = {p.name for p in out.glob('*.h') if p.stat().st_size}
if expected != actual:
    sys.exit('headergen output does not match the register description')
PYCHECK
n=$(ls "$TMP/out"/*.h | wc -l)

# Scope the include guards to this SoC.
#
# headergen builds the guard as "__HEADERGEN_" + toupper(filename) + "__" and
# the jz generator's filenames are flat (aic.h, cpm.h, ...), so every guard it
# emits collides with the X1000's. Nothing includes both in one translation
# unit today -- a build targets one SoC -- but the collision is silent if it
# ever happens, and the second header would simply vanish.
for f in "$TMP/out"/*.h; do
    sed -i.bak 's/__HEADERGEN_/__HEADERGEN_X1600_/g' "$f"
    rm -f "$f.bak"
done

mkdir -p "$DEST"
cp "$TMP/out"/*.h "$DEST/"
echo "regenerated $n headers into $DEST"
