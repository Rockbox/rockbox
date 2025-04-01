#!/bin/sh

set -e
set -x

tag=$1
version=$2

outdir="output/source"

mkdir -p "${outdir}"
git archive --prefix=rockbox-$version/ -o "${outdir}/rockbox-source-${version}.tar" ${tag}
# either compress with xz... or...
#xz -f "${outdir}/rockbox-source-${version}.tar"
# recompress with 7zip
tar -xf "${outdir}/rockbox-source-${version}.tar"
7z a -mx=9 "${outdir}/rockbox-source-${version}.7z" "rockbox-${version}"
rm -Rf "rockbox-${version}" "${outdir}/rockbox-source-${version}.tar"
