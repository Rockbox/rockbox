#!/usr/bin/perl
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

use File::Copy; # For move() and copy()
use File::Find; # For find()
use File::Path; # For rmtree()
use Cwd 'abs_path';

sub glob_copy {
    my ($pattern, $destination) = @_;
    foreach my $path (glob($pattern)) {
        copy($path, $destination);
    }
}

sub glob_move {
    my ($pattern, $destination) = @_;
    foreach my $path (glob($pattern)) {
        move($path, $destination);
    }
}

sub glob_unlink {
    my ($pattern) = @_;
    foreach my $path (glob($pattern)) {
        unlink($path);
    }
}

sub find_copyfile {
    my ($pattern, $destination) = @_;
    return sub {
        $path = $_;
        if ($path =~ $pattern && filesize($path) > 0 && !($path =~ /\.rockbox/)) {
            copy($path, $destination);
            chmod(0755, $destination.'/'.$path);
        }
    }
}

$ROOT="..";

my $ziptool="zip -r";
my $output="rockbox.zip";
my $verbose;
my $exe;
my $target;
my $archos;
my $incfonts;

while(1) {
    if($ARGV[0] eq "-r") {
        $ROOT=$ARGV[1];
        shift @ARGV;
        shift @ARGV;    
    }

    elsif($ARGV[0] eq "-z") {
        $ziptool=$ARGV[1];
        shift @ARGV;
        shift @ARGV;    
    }

    elsif($ARGV[0] eq "-t") {
        # The target name as used in ARCHOS in the root makefile
        $archos=$ARGV[1];
        shift @ARGV;
        shift @ARGV;    
    }
    elsif($ARGV[0] eq "-i") {
        # The target id name as used in TARGET_ID in the root makefile
        $target_id=$ARGV[1];
        shift @ARGV;
        shift @ARGV;    
    }
    elsif($ARGV[0] eq "-o") {
        $output=$ARGV[1];
        shift @ARGV;
        shift @ARGV;    
    }
    elsif($ARGV[0] eq "-f") {
        $incfonts=$ARGV[1]; # 0 - no fonts, 1 - fonts only 2 - fonts and package
        shift @ARGV;
        shift @ARGV;    
    }

    elsif($ARGV[0] eq "-v") {
        $verbose =1;
        shift @ARGV;
    }
    else {
        $target = $ARGV[0];
        $exe = $ARGV[1];
        last;
    }
}


my $firmdir="$ROOT/firmware";
my $appsdir="$ROOT/apps";
my $viewer_bmpdir="$ROOT/apps/plugins/bitmaps/viewer_defaults";

my $cppdef = $target;

sub gettargetinfo {
    open(GCC, ">gcctemp");
    # Get the LCD screen depth and graphical status
    print GCC <<STOP
\#include "config.h"
#ifdef HAVE_LCD_BITMAP
Bitmap: yes
Depth: LCD_DEPTH
Icon Width: CONFIG_DEFAULT_ICON_WIDTH
Icon Height: CONFIG_DEFAULT_ICON_HEIGHT
#endif
Codec: CONFIG_CODEC
#ifdef HAVE_REMOTE_LCD
Remote Depth: LCD_REMOTE_DEPTH
Remote Icon Width: CONFIG_REMOTE_DEFAULT_ICON_WIDTH
Remote Icon Height: CONFIG_REMOTE_DEFAULT_ICON_HEIGHT
#else
Remote Depth: 0
#endif
#ifdef HAVE_RECORDING
Recording: yes
#endif
STOP
;
    close(GCC);

    my $c="cat gcctemp | gcc $cppdef -I. -I$firmdir/export -E -P -";

    # print STDERR "CMD $c\n";

    open(TARGET, "$c|");

    my ($bitmap, $depth, $swcodec, $icon_h, $icon_w);
    my ($remote_depth, $remote_icon_h, $remote_icon_w);
    my ($recording);
    $icon_count = 1;
    while(<TARGET>) {
        # print STDERR "DATA: $_";
        if($_ =~ /^Bitmap: (.*)/) {
            $bitmap = $1;
        }
        elsif($_ =~ /^Depth: (\d*)/) {
            $depth = $1;
        }
        elsif($_ =~ /^Icon Width: (\d*)/) {
            $icon_w = $1;
        }
        elsif($_ =~ /^Icon Height: (\d*)/) {
            $icon_h = $1;
        }
        elsif($_ =~ /^Codec: (\d*)/) {
            # SWCODEC is 1, the others are HWCODEC
            $swcodec = ($1 == 1);
        }
        elsif($_ =~ /^Remote Depth: (\d*)/) {
            $remote_depth = $1;
        }
        elsif($_ =~ /^Remote Icon Width: (\d*)/) {
            $remote_icon_w = $1;
        }
        elsif($_ =~ /^Remote Icon Height: (\d*)/) {
            $remote_icon_h = $1;
        }
        if($_ =~ /^Recording: (.*)/) {
            $recording = $1;
        }
    }
    close(TARGET);
    unlink("gcctemp");

    return ($bitmap, $depth, $icon_w, $icon_h, $recording,
            $swcodec, $remote_depth, $remote_icon_w, $remote_icon_h);
}

sub filesize {
    my ($filename)=@_;
    my ($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size,
        $atime,$mtime,$ctime,$blksize,$blocks)
        = stat($filename);
    return $size;
}

sub buildzip {
    my ($zip, $image, $fonts)=@_;

    my ($bitmap, $depth, $icon_w, $icon_h, $recording, $swcodec,
        $remote_depth, $remote_icon_w, $remote_icon_h) = &gettargetinfo();

    # print "Bitmap: $bitmap\nDepth: $depth\nSwcodec: $swcodec\n";

    # remove old traces
    rmtree('.rockbox');

    mkdir ".rockbox", 0777;

    if(!$bitmap) {
        # always disable fonts on non-bitmap targets
        $fonts = 0;
    }
    # create the file so the database does not try indexing a folder
    open(IGNORE, ">.rockbox/database.ignore")  || die "can't open database.ignore";
    close(IGNORE);
    
    if($fonts) {
        mkdir ".rockbox/fonts", 0777;
        chdir(".rockbox/fonts");
        $cmd = "$ROOT/tools/convbdf -f $ROOT/fonts/*bdf >/dev/null 2>&1";
        print($cmd."\n") if $verbose;
        system($cmd);
        chdir("../../");

        if($fonts < 2) {
          # fonts-only package, return
          return;
        }
    }

    mkdir ".rockbox/langs", 0777;
    mkdir ".rockbox/rocks", 0777;
    mkdir ".rockbox/rocks/games", 0777;
    mkdir ".rockbox/rocks/apps", 0777;
    mkdir ".rockbox/rocks/demos", 0777;
    mkdir ".rockbox/rocks/viewers", 0777;

    if ($recording) {
        mkdir ".rockbox/recpresets", 0777;
    }

    if($swcodec) {
        mkdir ".rockbox/eqs", 0777;

        glob_copy("$ROOT/apps/eqs/*.cfg", '.rockbox/eqs/'); # equalizer presets
    }

    mkdir ".rockbox/wps", 0777;
    mkdir ".rockbox/themes", 0777;
    if ($bitmap) {
        open(THEME, ">.rockbox/themes/rockbox_default_icons.cfg");
        print THEME <<STOP
# this config file was auto-generated to make it
# easy to reset the icons back to default
iconset: -
# taken from apps/gui/icon.c
viewers iconset: /.rockbox/icons/viewers.bmp
remote iconset: -
# taken from apps/gui/icon.c
remote viewers iconset: /.rockbox/icons/remote_viewers.bmp

STOP
;
        close(THEME);
    }
    
    mkdir ".rockbox/codepages", 0777;

    if($bitmap) {
        system("$ROOT/tools/codepages");
    }
    else {
        system("$ROOT/tools/codepages -m");
    }

    glob_move('*.cp', '.rockbox/codepages/');

    if($bitmap) {
        mkdir ".rockbox/codecs", 0777;
        if($depth > 1) {
            mkdir ".rockbox/backdrops", 0777;
        }

        find(find_copyfile(qr/.*\.codec/, abs_path('.rockbox/codecs/')), 'apps/codecs');

        # remove directory again if no codec was copied
        rmdir(".rockbox/codecs");
    }

    find(find_copyfile(qr/\.(rock|ovl)/, abs_path('.rockbox/rocks/')), 'apps/plugins');

    open VIEWERS, "$ROOT/apps/plugins/viewers.config" or
        die "can't open viewers.config";
    @viewers = <VIEWERS>;
    close VIEWERS;

    open VIEWERS, ">.rockbox/viewers.config" or
        die "can't create .rockbox/viewers.config";

    foreach my $line (@viewers) {
        if ($line =~ /([^,]*),([^,]*),/) {
            my ($ext, $plugin)=($1, $2);
            my $r = "${plugin}.rock";
            my $oname;

            my $dir = $r;
            my $name;

            # strip off the last slash and file name part
            $dir =~ s/(.*)\/(.*)/$1/;
            # store the file name part
            $name = $2;

            # get .ovl name (file part only)
            $oname = $name;
            $oname =~ s/\.rock$/.ovl/;

            # print STDERR "$ext $plugin $dir $name $r\n";

            if(-e ".rockbox/rocks/$name") {
                if($dir ne "rocks") {
                    # target is not 'rocks' but the plugins are always in that
                    # dir at first!
                    move(".rockbox/rocks/$name", ".rockbox/rocks/$r");
                }
                print VIEWERS $line;
            }
            elsif(-e ".rockbox/rocks/$r") {
                # in case the same plugin works for multiple extensions, it
                # was already moved to the viewers dir
                print VIEWERS $line;
            }

            if(-e ".rockbox/rocks/$oname") {
                # if there's an "overlay" file for the .rock, move that as
                # well
                move(".rockbox/rocks/$oname", ".rockbox/rocks/$dir");
            }
        }
    }
    close VIEWERS;
                
    open CATEGORIES, "$ROOT/apps/plugins/CATEGORIES" or
        die "can't open CATEGORIES";
    @rock_targetdirs = <CATEGORIES>;
    close CATEGORIES;
    foreach my $line (@rock_targetdirs) {
        if ($line =~ /([^,]*),(.*)/) {
            my ($plugin, $dir)=($1, $2);
            move(".rockbox/rocks/${plugin}.rock", ".rockbox/rocks/$dir/${plugin}.rock");
        }
    }
    
    if ($bitmap) {
        mkdir ".rockbox/icons", 0777;
        copy("$viewer_bmpdir/viewers.${icon_w}x${icon_h}x$depth.bmp", ".rockbox/icons/viewers.bmp");
        if ($remote_depth) {
            copy("$viewer_bmpdir/remote_viewers.${remote_icon_w}x${remote_icon_h}x$remote_depth.bmp", ".rockbox/icons/remote_viewers.bmp");
        }
    }
    
    copy("$ROOT/apps/tagnavi.config", ".rockbox/");
    copy("$ROOT/apps/plugins/disktidy.config", ".rockbox/rocks/apps/");
      
    if($bitmap) {
        copy("$ROOT/apps/plugins/sokoban.levels", ".rockbox/rocks/games/sokoban.levels"); # sokoban levels
        copy("$ROOT/apps/plugins/snake2.levels", ".rockbox/rocks/games/snake2.levels"); # snake2 levels
    }

    if($image) {
        # image is blank when this is a simulator
        if( filesize("rockbox.ucl") > 1000 ) {
            copy("rockbox.ucl", ".rockbox/rockbox.ucl");  # UCL for flashing
        }
        if( filesize("rombox.ucl") > 1000) {
            copy("rombox.ucl", ".rockbox/rombox.ucl");  # UCL for flashing
        }
        
        # Check for rombox.target
        if ($image=~/(.*)\.(\w+)$/)
        {
            my $romfile = "rombox.$2";
            if (filesize($romfile) > 1000)
            {
                copy($romfile, ".rockbox/$romfile");
            }
        }
    }

    mkdir ".rockbox/docs", 0777;
    for(("COPYING",
         "LICENSES",
         "KNOWN_ISSUES"
        )) {
        copy("$ROOT/docs/$_", ".rockbox/docs/$_.txt");
    }
    if ($fonts) {
        copy("$ROOT/docs/profontdoc.txt", ".rockbox/docs/profontdoc.txt");
    }
    for(("sample.colours",
         "sample.icons"
        )) {
        copy("$ROOT/docs/$_", ".rockbox/docs/$_");
    }

    # Now do the WPS dance
    if(-d "$ROOT/wps") {
        system("perl $ROOT/wps/wpsbuild.pl -r $ROOT $ROOT/wps/WPSLIST $target");
    }
    else {
        print STDERR "No wps module present, can't do the WPS magic!\n";
    }

    # and the info file
    copy("rockbox-info.txt", ".rockbox/rockbox-info.txt");

    # copy the already built lng files
    glob_copy('apps/lang/*lng', '.rockbox/langs/');

}

my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) =
 localtime(time);

$mon+=1;
$year+=1900;

#$date=sprintf("%04d%02d%02d", $year,$mon, $mday);
#$shortdate=sprintf("%02d%02d%02d", $year%100,$mon, $mday);

# made once for all targets
sub runone {
    my ($target, $fonts)=@_;

    # build a full install .rockbox directory
    buildzip($output, $target, $fonts);

    # create a zip file from the .rockbox dfir

    unlink($output);
    if($verbose) {
      print "$ziptool $output .rockbox >/dev/null\n";
    }
    system("$ziptool $output .rockbox >/dev/null");

    if($target && ($fonts != 1)) {
        # On some targets, rockbox.* is inside .rockbox
        if($target !~ /(mod|ajz|wma)\z/i) {
            copy("$target", ".rockbox/$target");
            $target = ".rockbox/".$target;
        }
        
        if($verbose) {
            print "$ziptool $output $target >/dev/null\n";
        }
        system("$ziptool $output $target >/dev/null");
    }

    # remove the .rockbox afterwards
    rmtree('.rockbox');
};

if(!$exe) {
    # not specified, guess!
    if($target =~ /(recorder|ondio)/i) {
        $exe = "ajbrec.ajz";
    }
    elsif($target =~ /iriver/i) {
        $exe = "rockbox.iriver";
    }
    else {
        $exe = "archos.mod";
    }
}
elsif($exe =~ /rockboxui/) {
    # simulator, exclude the exe file
    $exe = "";
}

runone($exe, $incfonts);


