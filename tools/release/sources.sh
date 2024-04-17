#!/bin/sh

set -e
set -x

tag=$1
version=$2

outdir="output/source"

mkdir -p "${outdir}"
git archive --prefix=rockbox-$version/ -o "${outdir}/rockbox-source-${version}.tar" ${tag}
xz -f "${outdir}/rockbox-source-${version}.tar"
