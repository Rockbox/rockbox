#!/usr/bin/perl
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

use strict;

use File::Copy; # For move() and copy()
use File::Find; # For find()
use File::Path qw(mkpath rmtree); # For rmtree()
use Cwd;
use Cwd 'abs_path';
use Getopt::Long qw(:config pass_through);    # pass_through so not confused by -DTYPE_STUFF

my $ROOT="..";

my $ziptool;
my $output;
my $verbose;
my $install;
my $exe;
my $target;
my $modelname;
my $incfonts;
my $target_id; # passed in, not currently used
my $rbdir; # can be non-.rockbox for special builds
my $app;
my $mklinks;

sub glob_mkdir {
    my ($dir) = @_;
    mkpath ($dir, $verbose, 0777);
    return 1;
}

sub glob_install {
    my ($_src, $dest, $opts) = @_;

    unless ($opts) {
        $opts = "-m 0664";
    }

    foreach my $src (glob($_src)) {
        unless ( -d $src || !(-e $src)) {
            system("install $opts \"$src\" \"$dest\"");
            print "install $opts \"$src\" -> \"$dest\"\n" if $verbose;
        }
    }
    return 1;
}

sub glob_copy {
    my ($pattern, $destination) = @_;
    print "glob_copy: $pattern -> $destination\n" if $verbose;
    foreach my $path (glob($pattern)) {
        copy($path, $destination);
    }
}

sub glob_move {
    my ($pattern, $destination) = @_;
    print "glob_move: $pattern -> $destination\n" if $verbose;
    foreach my $path (glob($pattern)) {
        move($path, $destination);
    }
}

sub glob_unlink {
    my ($pattern) = @_;
    print "glob_unlink: $pattern\n" if $verbose;
    foreach my $path (glob($pattern)) {
        unlink($path);
    }
}

sub find_copyfile {
    my ($pattern, $destination) = @_;
    print "find_copyfile: $pattern -> $destination\n" if $verbose;
    return sub {
        my $path = $_;
        my $source = getcwd();
        if ($path =~ $pattern && filesize($path) > 0 && !($path =~ /$rbdir/)) {
            if($mklinks) {
                print "link $path $destination\n" if $verbose;
                symlink($source.'/'.$path, $destination.'/'.$path);
            } else {
                print "cp $path $destination\n" if $verbose;
                copy($path, $destination);
                chmod(0755, $destination.'/'.$path);
            } 
        }
    }
}

sub find_installfile {
    my ($pattern, $destination) = @_;
    print "find_installfile: $pattern -> $destination\n" if $verbose;
    return sub {
        my $path = $_;
        if ($path =~ $pattern) {
        print "FIND_INSTALLFILE: $path\n";
            glob_install($path, $destination);
        }
    }
}


sub make_install {
    my ($src, $dest) = @_;

    my $bindir = $dest;
    my $libdir = $dest;
    my $userdir = $dest;

    my @plugins = ( "games", "apps", "demos", "viewers" );
    my @userstuff = ( "backdrops", "codepages", "docs", "fonts", "langs", "themes", "wps", "eqs", "icons" );
    my @files = ();

    if ($app) {
        $bindir .= "/bin";
        $libdir .= "/lib/rockbox";
        $userdir .= "/share/rockbox";
    } else {
        # for non-app builds we expect the prefix to be the dir above .rockbox
        $bindir .= "/$rbdir";
        $libdir = $userdir = $bindir;
    }
    if ($dest =~ /\/dev\/null/) {
        die "ERROR: No PREFIX given\n"
    }

    if ((!$app) && -e $bindir && -e $src && (abs_path($bindir) eq abs_path($src))) {
        return 1;
    }

    # binary
    unless ($exe eq "") {
        unless (glob_mkdir($bindir)) {
            return 0;
        }
        glob_install($exe, $bindir, "-m 0775");
    }

    # codecs
    unless (glob_mkdir("$libdir/codecs")) {
        return 0;
    }
    # Android has codecs installed as native libraries so they are not needed
    # in the zip.
    if ($modelname !~ /android/) {
        glob_install("$src/codecs/*", "$libdir/codecs", "-m 0755");
    }

    # plugins
    unless (glob_mkdir("$libdir/rocks")) {
        return 0;
    }
    foreach my $t (@plugins) {
        unless (glob_mkdir("$libdir/rocks/$t")) {
            return 0;
        }
        glob_install("$src/rocks/$t/*", "$libdir/rocks/$t", "-m 0755");
    }

    # rocks/viewers/lua
    unless (glob_mkdir("$libdir/rocks/viewers/lua")) {
        return 0;
    }
    glob_install("$src/rocks/viewers/lua/*", "$libdir/rocks/viewers/lua");

    # all the rest directories
    foreach my $t (@userstuff) {
        unless (glob_mkdir("$userdir/$t")) {
            return 0;
        }
        glob_install("$src/$t/*", "$userdir/$t");
    }

    # wps/ subfolders and bitmaps
    opendir(DIR, $src . "/wps");
    @files = readdir(DIR);
    closedir(DIR);

    foreach my $_dir (@files) {
        my $dir = "wps/" . $_dir;
        if ( -d "$src/$dir" && $_dir !~ /\.\.?/) {
            unless (glob_mkdir("$userdir/$dir")) {
                return 0;
            }
            glob_install("$src/$dir/*", "$userdir/$dir");
        }
    }

    # rest of the files, excluding the binary
    opendir(DIR,$src);
    @files = readdir(DIR);
    closedir(DIR);

    foreach my $file (grep (/[a-zA-Z]+\.(txt|config|ignore|sh)/,@files)) {
        glob_install("$src/$file", "$userdir/");
    }
    return 1;
}

# Get options
GetOptions ( 'r|root=s'      => \$ROOT,
             'z|ziptool:s'   => \$ziptool,
             'm|modelname=s' => \$modelname,  # The model name as used in ARCHOS in the root makefile
             'i|id=s'        => \$target_id,  # The target id name as used in TARGET_ID in the root makefile
             'o|output:s'    => \$output,
             'f|fonts=s'     => \$incfonts,   # 0 - no fonts, 1 - fonts only 2 - fonts and package
             'v|verbose'     => \$verbose,
             'install=s'     => \$install, # install destination
             'rbdir:s'       => \$rbdir, # If we want to put in a different directory
             'l|link'        => \$mklinks, # If we want to create links instead of copying files
             'a|app:s'       => \$app, # Is this an Application build?
    );

# GetOptions() doesn't remove the params from @ARGV if their value was ""
# Thus we use the ":" for those for which we use a default value in case of ""
# and assign the default value afterwards
if ($ziptool eq '') {
    $ziptool = "zip -r9";
}
if ($output eq '') {
    $output = "rockbox.zip"
}
if ($rbdir eq '') {
    $rbdir = ".rockbox";
}

# Now @ARGV shuold be free of any left-overs GetOptions left
($target, $exe) = @ARGV;

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
LCD Width: LCD_WIDTH
LCD Height: LCD_HEIGHT
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

    my ($bitmap, $width, $height, $depth, $swcodec, $icon_h, $icon_w);
    my ($remote_depth, $remote_icon_h, $remote_icon_w);
    my ($recording);
    my $icon_count = 1;
    while(<TARGET>) {
        # print STDERR "DATA: $_";
        if($_ =~ /^Bitmap: (.*)/) {
            $bitmap = $1;
        }
        elsif($_ =~ /^Depth: (\d*)/) {
            $depth = $1;
        }
        elsif($_ =~ /^LCD Width: (\d*)/) {
            $width = $1;
        }
        elsif($_ =~ /^LCD Height: (\d*)/) {
            $height = $1;
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

    return ($bitmap, $depth, $width, $height, $icon_w, $icon_h, $recording,
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
    my ($image, $fonts)=@_;    
    my $libdir = $install;
    my $temp_dir = ".rockbox";

    print "buildzip: image=$image fonts=$fonts\n" if $verbose;
    
    my ($bitmap, $depth, $width, $height, $icon_w, $icon_h, $recording,
        $swcodec, $remote_depth, $remote_icon_w, $remote_icon_h) =
      &gettargetinfo();

    # print "Bitmap: $bitmap\nDepth: $depth\nSwcodec: $swcodec\n";

    # remove old traces
    rmtree($temp_dir);

    glob_mkdir($temp_dir);

    if(!$bitmap) {
        # always disable fonts on non-bitmap targets
        $fonts = 0;
    }
    if($fonts) {
        glob_mkdir("$temp_dir/fonts");
        chdir "$temp_dir/fonts";
        my $cmd = "$ROOT/tools/convbdf -f $ROOT/fonts/*bdf >/dev/null 2>&1";
        print($cmd."\n") if $verbose;
        system($cmd);
        chdir("../../");

        if($fonts < 2) {
          # fonts-only package, return
          return;
        }
    }

    # create the file so the database does not try indexing a folder
    open(IGNORE, ">$temp_dir/database.ignore")  || die "can't open database.ignore";
    close(IGNORE);

    # the samsung ypr0 has a loader script that's needed in the zip
    if ($modelname =~ /samsungypr0/) {
        glob_copy("$ROOT/utils/ypr0tools/rockbox.sh", "$temp_dir/");
    }
    # add .nomedia on Android
    # in the zip.
    if ($modelname =~ /android/) {
        open(NOMEDIA, ">$temp_dir/.nomedia")  || die "can't open .nomedia";
        close(NOMEDIA);
    }

    glob_mkdir("$temp_dir/langs");
    glob_mkdir("$temp_dir/rocks");
    glob_mkdir("$temp_dir/rocks/games");
    glob_mkdir("$temp_dir/rocks/apps");
    glob_mkdir("$temp_dir/rocks/demos");
    glob_mkdir("$temp_dir/rocks/viewers");

    if ($recording) {
        glob_mkdir("$temp_dir/recpresets");
    }

    if($swcodec) {
        glob_mkdir("$temp_dir/eqs");

        glob_copy("$ROOT/lib/rbcodec/dsp/eqs/*.cfg", "$temp_dir/eqs/"); # equalizer presets
    }

    glob_mkdir("$temp_dir/wps");
    glob_mkdir("$temp_dir/icons");
    glob_mkdir("$temp_dir/themes");
    glob_mkdir("$temp_dir/codepages");

    if($bitmap) {
        system("$ROOT/tools/codepages");
    }
    else {
        system("$ROOT/tools/codepages -m");
    }

    glob_move('*.cp', "$temp_dir/codepages/");

    if($bitmap && $depth > 1) {
        glob_mkdir("$temp_dir/backdrops");
    }

    glob_mkdir("$temp_dir/codecs");

    # Android has codecs installed as native libraries so they are not needed
    # in the zip.
    if ($modelname !~ /android/) {
        find(find_copyfile(qr/.*\.codec/, abs_path("$temp_dir/codecs/")), 'lib/rbcodec/codecs');
    }

    # remove directory again if no codec was copied
    rmdir("$temp_dir/codecs");

    find(find_copyfile(qr/\.(rock|ovl|lua)/, abs_path("$temp_dir/rocks/")), 'apps/plugins');

    # exclude entries for the image file types not supported by the imageviewer for the target.
    my $viewers = "$ROOT/apps/plugins/viewers.config";
    my $c="cat $viewers | gcc $cppdef -I. -I$firmdir/export -E -P -include config.h -";

    open VIEWERS, "$c|" or die "can't open viewers.config";
    my @viewers = <VIEWERS>;
    close VIEWERS;

    open VIEWERS, ">$temp_dir/viewers.config" or
        die "can't create $temp_dir/viewers.config";

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

            if(-e "$temp_dir/rocks/$name") {
                if($dir ne "rocks") {
                    # target is not 'rocks' but the plugins are always in that
                    # dir at first!
                    move("$temp_dir/rocks/$name", "$temp_dir/rocks/$r");
                }
                print VIEWERS $line;
            }
            elsif(-e "$temp_dir/rocks/$r") {
                # in case the same plugin works for multiple extensions, it
                # was already moved to the viewers dir
                print VIEWERS $line;
            }

            if(-e "$temp_dir/rocks/$oname") {
                # if there's an "overlay" file for the .rock, move that as
                # well
                move("$temp_dir/rocks/$oname", "$temp_dir/rocks/$dir");
            }
        }
    }
    close VIEWERS;

    open CATEGORIES, "$ROOT/apps/plugins/CATEGORIES" or
        die "can't open CATEGORIES";
    my @rock_targetdirs = <CATEGORIES>;
    close CATEGORIES;
    foreach my $line (@rock_targetdirs) {
        if ($line =~ /([^,]*),(.*)/) {
            my ($plugin, $dir)=($1, $2);
            move("$temp_dir/rocks/${plugin}.rock", "$temp_dir/rocks/$dir/${plugin}.rock");
            if(-e "$temp_dir/rocks/${plugin}.ovl") {
                # if there's an "overlay" file for the .rock, move that as
                # well
                move("$temp_dir/rocks/${plugin}.ovl", "$temp_dir/rocks/$dir");
            }
            if(-e "$temp_dir/rocks/${plugin}.lua") {
                # if this is a lua script, move it to the appropriate dir
                move("$temp_dir/rocks/${plugin}.lua", "$temp_dir/rocks/$dir/");
            }
        }
    }

    glob_unlink("$temp_dir/rocks/*.lua"); # Clean up unwanted *.lua files (e.g. actions.lua, buttons.lua)

    copy("$ROOT/apps/tagnavi.config", "$temp_dir/");
    copy("$ROOT/apps/plugins/disktidy.config", "$temp_dir/rocks/apps/");

    if($bitmap) {
        copy("$ROOT/apps/plugins/sokoban.levels", "$temp_dir/rocks/games/sokoban.levels"); # sokoban levels
        copy("$ROOT/apps/plugins/snake2.levels", "$temp_dir/rocks/games/snake2.levels"); # snake2 levels
        copy("$ROOT/apps/plugins/rockbox-fonts.config", "$temp_dir/rocks/viewers/");
    }

    if(-e "$temp_dir/rocks/demos/pictureflow.rock") {
        copy("$ROOT/apps/plugins/bitmaps/native/pictureflow_emptyslide.100x100x16.bmp",
             "$temp_dir/rocks/demos/pictureflow_emptyslide.bmp");
        my ($pf_logo);
        if ($width < 200) {
            $pf_logo = "pictureflow_logo.100x18x16.bmp";
        } else {
            $pf_logo = "pictureflow_logo.193x34x16.bmp";
        }
        copy("$ROOT/apps/plugins/bitmaps/native/$pf_logo",
             "$temp_dir/rocks/demos/pictureflow_splash.bmp");

    }

    if($image) {
        # image is blank when this is a simulator
        if( filesize("rockbox.ucl") > 1000 ) {
            copy("rockbox.ucl", "$temp_dir/rockbox.ucl");  # UCL for flashing
        }
        if( filesize("rombox.ucl") > 1000) {
            copy("rombox.ucl", "$temp_dir/rombox.ucl");  # UCL for flashing
        }
        
        # Check for rombox.target
        if ($image=~/(.*)\.(\w+)$/)
        {
            my $romfile = "rombox.$2";
            if (filesize($romfile) > 1000)
            {
                copy($romfile, "$temp_dir/$romfile");
            }
        }
    }

    glob_mkdir("$temp_dir/docs");
    for(("COPYING",
         "LICENSES",
         "KNOWN_ISSUES"
        )) {
        copy("$ROOT/docs/$_", "$temp_dir/docs/$_.txt");
    }
    if ($fonts) {
        copy("$ROOT/docs/profontdoc.txt", "$temp_dir/docs/profontdoc.txt");
    }
    for(("sample.colours",
         "sample.icons"
        )) {
        copy("$ROOT/docs/$_", "$temp_dir/docs/$_");
    }

    # Now do the WPS dance
    if(-d "$ROOT/wps") {
        my $wps_build_cmd="perl $ROOT/wps/wpsbuild.pl ";
        $wps_build_cmd=$wps_build_cmd."-v " if $verbose;
        $wps_build_cmd=$wps_build_cmd." --tempdir=$temp_dir --rbdir=$rbdir -r $ROOT -m $modelname $ROOT/wps/WPSLIST $target";
        print "wpsbuild: $wps_build_cmd\n" if $verbose;
        system("$wps_build_cmd");
        print "wps_build_cmd: done\n" if $verbose;
    }
    else {
        print STDERR "No wps module present, can't do the WPS magic!\n";
    }
    
    # until buildwps.pl is fixed, manually copy the classic_statusbar theme across
    mkdir "$temp_dir/wps/classic_statusbar", 0777;
    glob_copy("$ROOT/wps/classic_statusbar/*.bmp", "$temp_dir/wps/classic_statusbar");
    if ($swcodec) {
        if ($depth == 16) {
            copy("$ROOT/wps/classic_statusbar.sbs", "$temp_dir/wps");
        } elsif ($depth > 1) {
            copy("$ROOT/wps/classic_statusbar.grey.sbs", "$temp_dir/wps/classic_statusbar.sbs");
        } else {
            copy("$ROOT/wps/classic_statusbar.mono.sbs", "$temp_dir/wps/classic_statusbar.sbs");
        }
    } else {
        copy("$ROOT/wps/classic_statusbar.112x64x1.sbs", "$temp_dir/wps/classic_statusbar.sbs");
    }
    if ($remote_depth != $depth) {
        copy("$ROOT/wps/classic_statusbar.mono.sbs", "$temp_dir/wps/classic_statusbar.rsbs");
    } else {
        copy("$temp_dir/wps/classic_statusbar.sbs", "$temp_dir/wps/classic_statusbar.rsbs");
    }
    copy("$temp_dir/wps/rockbox_none.sbs", "$temp_dir/wps/rockbox_none.rsbs");

    # and the info file
    copy("rockbox-info.txt", "$temp_dir/rockbox-info.txt");

    # copy the already built lng files
    glob_copy('apps/lang/*lng', "$temp_dir/langs/");
    glob_copy('apps/lang/*.zip', "$temp_dir/langs/");

    # copy the .lua files
    glob_mkdir("$temp_dir/rocks/viewers/lua/");
    glob_copy('apps/plugins/lua/*.lua', "$temp_dir/rocks/viewers/lua/");
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

    # Strip the leading / from $rbdir unless we are installing an application
    # build - the layout is different (no .rockbox, but bin/lib/share)
    unless ($app && $install) {
        $rbdir = substr($rbdir, 1);
    }

    # build a full install .rockbox ($rbdir) directory
    buildzip($target, $fonts);

    unlink($output);

    if($fonts == 1) {
        # Don't include image file in fonts-only package
        undef $target;
    }
    if($target && ($target !~ /(mod|ajz|wma)\z/i)) {
        # On some targets, the image goes into .rockbox.
        copy("$target", ".rockbox/$target");
        undef $target;
    }

    if($install) {
        if($mklinks) {
            my $cwd = getcwd();
            symlink("$cwd/.rockbox", "$install/.rockbox");
            print "link .rockbox $install\n" if $verbose;
        } else {
            make_install(".rockbox", $install) or die "MKDIRFAILED\n";
            rmtree(".rockbox");
            print "rm .rockbox\n" if $verbose;
        }
    }
    else {
        unless (".rockbox" eq $rbdir) {
            mkpath($rbdir);
            rmtree($rbdir);
            move(".rockbox", $rbdir);
            print "mv .rockbox $rbdir\n" if $verbose;
        }
        system("$ziptool $output $rbdir $target >/dev/null");
        print "$ziptool $output $rbdir $target >/dev/null\n" if $verbose;
        rmtree("$rbdir");
        print "rm $rbdir\n" if $verbose;
    }
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
elsif(($exe =~ /rockboxui/)) {
    # simulator, exclude the exe file
    $exe = "";
}
elsif($exe eq "librockbox.so") {
    # android, exclude the binary
    $exe="";
}

runone($exe, $incfonts);

