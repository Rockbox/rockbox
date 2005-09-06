#!/usr/bin/perl

$ROOT="..";

if($ARGV[0] eq "-r") {
    $ROOT=$ARGV[1];
    shift @ARGV;
    shift @ARGV;    
}

my $verbose;
if($ARGV[0] eq "-v") {
    $verbose =1;
    shift @ARGV;
}

my $firmdir="$ROOT/firmware";

my $target = $ARGV[0];
my $cppdef = $target;

my $exe = $ARGV[1];

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
        print "lang $_\n" if($verbose);
        system ("$ROOT/tools/binlang $dir/english.lang $dir/$_ $outputlang/$output >/dev/null 2>&1");
    }
}

sub buildzip {
    my ($zip, $image, $notplayer)=@_;

    # remove old traces
    `rm -rf .rockbox`;

    mkdir ".rockbox", 0777;
    mkdir ".rockbox/langs", 0777;
    mkdir ".rockbox/rocks", 0777;
    mkdir ".rockbox/codecs", 0777;

    my $c = 'find apps -name "*.codec" ! -empty -exec cp {} .rockbox/codecs/ \;';
    print `$c`;

    my @call = `find .rockbox/codecs -type f`;
    if(!$call[0]) {
        # no codec was copied, remove directory again
        rmdir(".rockbox/codecs");
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
    for (@viewers) {
        if (/,(.+).rock,/) {
            my $r = "$1.rock";
            my $o = "$1.ovl";
            if(-e ".rockbox/rocks/$r") {
                `mv .rockbox/rocks/$r .rockbox/viewers`;
                print VIEWERS $_;
            }
            elsif(-e ".rockbox/viewers/$r") {
                # in case the same plugin works for multiple extensions, it
                # was already moved to the viewers dir
                print VIEWERS $_;
            }
            if(-e ".rockbox/rocks/$o") {
                # if there's an "overlay" file for the .rock, move that as
                # well
                `mv .rockbox/rocks/$o .rockbox/viewers`;              
            }
        }
    }
    close VIEWERS;
    
    if($notplayer) {
        `cp $ROOT/apps/plugins/sokoban.levels .rockbox/rocks/`; # sokoban levels
        `cp $ROOT/apps/plugins/snake2.levels .rockbox/rocks/`; # snake2 levels

        mkdir ".rockbox/fonts", 0777;

        opendir(DIR, "$ROOT/fonts") || die "can't open dir fonts";
        my @fonts = grep { /\.bdf$/ && -f "$ROOT/fonts/$_" } readdir(DIR);
        closedir DIR;

        my $maxfont;

        open(SIZE, ">ziptemp");
        print SIZE <<STOP
\#include "font.h"
Font Size We Want: MAX_FONT_SIZE
STOP
;
        close(SIZE);
        my $c="cat ziptemp | gcc $cppdef -I. -I$firmdir/export -E -P -";
        # print STDERR "C: $c\n";
        open(GETSIZE, "$c|");

        while(<GETSIZE>) {
            if($_ =~ /^Font Size We Want: (\d*)/) {
                $maxfont = $1;
                last;
            }
        }
        close(GETSIZE);
        unlink("ziptemp");
        die "no decent max font size" if ($maxfont < 2000);
        
        for(@fonts) {
            my $f = $_;

            print "FONT: $f\n" if($verbose);
            my $o = $f;
            $o =~ s/\.bdf/\.fnt/;
            my $cmd ="$ROOT/tools/convbdf -s 32 -l 255 -f -o \".rockbox/fonts/$o\" \"$ROOT/fonts/$f\" >/dev/null 2>&1";
            print "CMD: $cmd\n" if($verbose);
            `$cmd`;
            
            # no need to add fonts we cannot load anyway
            my $fontsize = filesize(".rockbox/fonts/$o");
            if($fontsize > $maxfont) {
                unlink(".rockbox/fonts/$o");
            }
        }

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
    for(("BATTERY-FAQ",
         "CUSTOM_CFG_FORMAT",
         "CUSTOM_WPS_FORMAT",
         "FAQ",
         "NODO",
         "TECH")) {
        `cp $ROOT/docs/$_ .rockbox/docs/$_.txt`;
    }

    # now copy the file made for reading on the unit:
    #if($notplayer) {
    #    `cp $webroot/docs/Help-JBR.txt .rockbox/docs/`;
    #}
    #else {
    #    `cp $webroot/docs/Help-Stu.txt .rockbox/docs/`;
    #}

    buildlangs(".rockbox/langs");

    `rm -f $zip`;
    `find .rockbox | zip $zip -@ >/dev/null`;

    if($image) {
        `zip $zip $image`;
    }

    # remove the .rockbox afterwards
    `rm -rf .rockbox`;
}

my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) =
 localtime(time);

$mon+=1;
$year+=1900;

$date=sprintf("%04d%02d%02d", $year,$mon, $mday);
$shortdate=sprintf("%02d%02d%02d", $year%100,$mon, $mday);

# made once for all targets
sub runone {
    my ($type, $target)=@_;

    # build a full install zip file 
    buildzip("rockbox.zip", $target,
             ($type eq "player")?0:1);
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
    runone("player", $exe);
}
else {
    runone("recorder", $exe);
}

