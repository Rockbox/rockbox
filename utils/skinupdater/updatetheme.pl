#!/usr/bin/perl
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id: wpsbuild.pl 24813 2010-02-21 19:10:57Z kugel $
#

# usage: updatetheme.pl theme.zip workingdir [options passed to skinupdater]
use File::Basename;

$numArgs = $#ARGV + 1;

die "usage: updatetheme.pl theme.zip workingdir [options passed to skinupdater]" if ($numArgs < 2);
    
$ARGV[0] =~ /.*\/(.*).(zip|ZIP)/; #fix this regex!
$theme_name = $1;
$tmp = $ARGV[1];
$outdir = "$tmp/$theme_name";

if ($numArgs > 2)
{
    $args = $ARGV[2];
} else {
    $args = "";
}


system("mkdir $outdir") and die "couldnt mkdir $outdir";

# step 1, unzip the theme zip
system("unzip $ARGV[0] -d $outdir") and die;

#for each skin in the zip run skinupdater
@files = `find $outdir -iname "*.wps" -o -iname "*.sbs" -o -iname "*.fms" -o -iname "*.rwps" -o -iname "*.rsbs" -o -iname "*.rfms"`;
`touch $tmp/theme_name.diff`;
foreach (@files)
{
    chomp($_);
    $file = $_;
    $out = "$tmp/" . `basename $file`; chomp($out);
    `./skinupdater $args $file $out`;
    `diff -u $file $out >> $tmp/$theme_name.diff`;
    `mv $out $file`;
    
}
`cd $outdir && zip -r $tmp/$theme_name.zip .`;   


system("rm -Rf $outdir");
