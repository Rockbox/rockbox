#!/usr/bin/perl
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

$ROOT="..";

my $ziptool="zip";
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

my $cppdef = $target;


sub filesize {
    my ($filename)=@_;
    my ($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size,
        $atime,$mtime,$ctime,$blksize,$blocks)
        = stat($filename);
    return $size;
}

sub buildlangs {
    my ($outputlang)=@_;
    my $dir = "$ROOT/apps/lang";
    opendir(DIR, $dir);
    my @files = grep { /\.lang$/ } readdir(DIR);
    closedir(DIR);

    for(@files) {
        my $output = $_;
        $output =~ s/(.*)\.lang/$1.lng/;
        print "$ROOT/tools/genlang -e=$dir/english.lang -t=$archos -b=$outputlang/$output $dir/$_\n" if($verbose);
        system ("$ROOT/tools/genlang -e=$dir/english.lang -t=$archos -b=$outputlang/$output $dir/$_ >/dev/null 2>&1");
    }
}

sub buildzip {
    my ($zip, $image, $notplayer, $fonts)=@_;

    # remove old traces
    `rm -rf .rockbox`;

    mkdir ".rockbox", 0777;

    if($fonts) {
        mkdir ".rockbox/fonts", 0777;

        opendir(DIR, "$ROOT/fonts") || die "can't open dir fonts";
        my @fonts = grep { /\.bdf$/ && -f "$ROOT/fonts/$_" } readdir(DIR);
        closedir DIR;

        for(@fonts) {
            my $f = $_;

            print "FONT: $f\n" if($verbose);
            my $o = $f;
            $o =~ s/\.bdf/\.fnt/;
            my $cmd ="$ROOT/tools/convbdf -f -o \".rockbox/fonts/$o\" \"$ROOT/fonts/$f\" >/dev/null 2>&1";
            print "CMD: $cmd\n" if($verbose);
            `$cmd`;
            
        }
        if($fonts < 2) {
          # fonts-only package, return
          return;
        }
    }

    mkdir ".rockbox/langs", 0777;
    mkdir ".rockbox/rocks", 0777;
    if($notplayer) {
        mkdir ".rockbox/codepages", 0777;
        mkdir ".rockbox/codecs", 0777;
        mkdir ".rockbox/wps", 0777;
        mkdir ".rockbox/themes", 0777;
        mkdir ".rockbox/backdrops", 0777;
        mkdir ".rockbox/eqs", 0777;

        my $c = 'find apps -name "*.codec" ! -empty -exec cp {} .rockbox/codecs/ \; 2>/dev/null';
        `$c`;

        system("$ROOT/tools/codepages");
        my $c = 'find . -name "*.cp" ! -empty -exec mv {} .rockbox/codepages/ \; >/dev/null 2>&1';
        `$c`;

        my @call = `find .rockbox/codecs -type f 2>/dev/null`;
        if(!$call[0]) {
            # no codec was copied, remove directory again
            rmdir(".rockbox/codecs");

        }
    }

    $c= 'find apps "(" -name "*.rock" -o -name "*.ovl" ")" ! -empty -exec cp {} .rockbox/rocks/ \;';
    print `$c`;

    open VIEWERS, "$ROOT/apps/plugins/viewers.config" or
        die "can't open viewers.config";
    @viewers = <VIEWERS>;
    close VIEWERS;

    open VIEWERS, ">.rockbox/viewers.config" or
        die "can't create .rockbox/viewers.config";
    mkdir ".rockbox/viewers", 0777;
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
                    `mv .rockbox/rocks/$name .rockbox/$r`;
                }
                print VIEWERS $line;
            }
            elsif(-e ".rockbox/$r") {
                # in case the same plugin works for multiple extensions, it
                # was already moved to the viewers dir
                print VIEWERS $line;
            }

            if(-e ".rockbox/rocks/$oname") {
                # if there's an "overlay" file for the .rock, move that as
                # well
                `mv .rockbox/rocks/$oname .rockbox/$dir`;
            }
        }
    }
    close VIEWERS;
    
    `cp $ROOT/apps/tagnavi.config .rockbox/`;
      
    if($notplayer) {
        `cp $ROOT/apps/plugins/sokoban.levels .rockbox/rocks/`; # sokoban levels
        `cp $ROOT/apps/plugins/snake2.levels .rockbox/rocks/`; # snake2 levels
    }

    if($image) {
        # image is blank when this is a simulator
        if( filesize("rockbox.ucl") > 1000 ) {
            `cp rockbox.ucl .rockbox/`;  # UCL for flashing
        }
        if( filesize("rombox.ucl") > 1000) {
            `cp rombox.ucl .rockbox/`;  # UCL for flashing
        }
    }

    mkdir ".rockbox/docs", 0777;
    for(("COPYING",
         "LICENSES",
        )) {
        `cp $ROOT/docs/$_ .rockbox/docs/$_.txt`;
    }

    # Now do the WPS dance
    if(-d "$ROOT/wps") {
        system("perl $ROOT/wps/wpsbuild.pl -r $ROOT $ROOT/wps/WPSLIST $target");
    }
    else {
        print STDERR "No wps module present, can't do the WPS magic!\n";
    }

    # now copy the file made for reading on the unit:
    #if($notplayer) {
    #    `cp $webroot/docs/Help-JBR.txt .rockbox/docs/`;
    #}
    #else {
    #    `cp $webroot/docs/Help-Stu.txt .rockbox/docs/`;
    #}

    buildlangs(".rockbox/langs");

}

my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) =
 localtime(time);

$mon+=1;
$year+=1900;

$date=sprintf("%04d%02d%02d", $year,$mon, $mday);
$shortdate=sprintf("%02d%02d%02d", $year%100,$mon, $mday);

# made once for all targets
sub runone {
    my ($type, $target, $fonts)=@_;

    # build a full install .rockbox directory
    buildzip($output, $target,
             ($type eq "player")?0:1, $fonts);

    # create a zip file from the .rockbox dfir

    `rm -f $output`;
    if($verbose) {
      print "find .rockbox | xargs $ziptool $output >/dev/null\n";
    }
    `find .rockbox | xargs $ziptool $output >/dev/null`;

    if($target && ($fonts != 1)) {
        if($verbose) {
            print "$ziptool $output $target\n";
        }
        `$ziptool $output $target`;
    }

    # remove the .rockbox afterwards
    `rm -rf .rockbox`;
};

if(!$exe) {
    # not specified, guess!
    if($target =~ /(recorder|ondio)/i) {
        $exe = "ajbrec.ajz";
    }
    elsif($target =~ /iriver/i) {
        $exe = "rockbox.iriver";
    }
    elsif($target =~ /gmini/i) {
        $exe = "rockbox.gmini";
    }
    else {
        $exe = "archos.mod";
    }
}
elsif($exe =~ /rockboxui/) {
    # simulator, exclude the exe file
    $exe = "";
}

if($target =~ /player/i) {
    runone("player", $exe, 0);
}
else {
    runone("recorder", $exe, $incfonts);
}

