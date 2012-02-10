#!/bin/sh
rootdir=`dirname $0`
toolsdir=$rootdir/..
outdir=$rootdir/output
jobs="1"
err="0"

mkdir -p $outdir

print_help() {
  echo "Build Checkwps for every target in targets.txt."
  echo "The binaries are put into in '$outdir'"
  echo ""
  cat <<EOF
  Usage: build-all.sh [OPTION]...
  Options:
    --jobs=NUMBER   Let make use NUMBER jobs (default is 1)

EOF
exit
}

for arg in "$@"; do
	case "$arg" in
        --jobs=*)   jobs=`echo "$arg" | cut -d = -f 2`;;
        -h|--help)  print_help;;
        *)          err="1"; echo "[ERROR] Option '$arg' unsupported";;
    esac
done

if [ -z $jobs ] || [ $jobs -le "0" ]
then
    echo "[ERROR] jobs must be a positive number"
    err="1"
fi

if [ $err -ge "1" ]
then
    echo "An error occured. Aborting"
    exit
fi

awk -f $rootdir/parse_configure.awk $rootdir/../configure | (
    while read target model
    do
        make -j $jobs clean
        $toolsdir/configure --target=$model --type=C --ram=32 --lcdwidth=100 --lcdheight=100 # 32 should always give default RAM, assume 100x100 for RaaA for now
        make -j $jobs
        mv checkwps.$model $outdir
    done
)
