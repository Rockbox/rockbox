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
use Getopt::Long qw(:config pass_through);	# pass_through so not confused by -DTYPE_STUFF

my $ROOT="..";
my $wpsdir;
my $verbose;
my $rbdir=".rockbox";
my $tempdir=".rockbox";
my $wpslist;
my $target;
my $modelname;

# Get options
GetOptions ( 'r|root=s'		=> \$ROOT,
	     'm|modelname=s'	=> \$modelname,
	     'v|verbose'	=> \$verbose,
	     'rbdir=s'          => \$rbdir, # If we want to put in a different directory
         'tempdir=s'    => \$tempdir, # override .rockbox temporary dir
    );

($wpslist, $target) = @ARGV;

my $firmdir="$ROOT/firmware";
my $cppdef = $target;

# These parameters are filled in as we parse wpslist
my $req_t;
my $theme;
my $has_wps;
my $wps;
my $has_rwps;
my $rwps;
my $has_sbs;
my $sbs;
my $has_rsbs;
my $rsbs;
my $has_fms;
my $fms;
my $has_rfms;
my $rfms;
my $width;
my $height;
my $font;
my $remotefont;
my $fgcolor;
my $bgcolor;
my $statusbar;
my $remotestatusbar;
my $author;
my $backdrop;
my $lineselectstart;
my $lineselectend;
my $selecttype;
my $iconset;
my $remoteiconset;
my $viewericon;
my $remoteviewericon;
my $lineselecttextcolor;
my $filetylecolor;
my $listviewport;
my $remotelistviewport;

# LCD sizes
my ($main_height, $main_width, $main_depth);
my ($remote_height, $remote_width, $remote_depth);
my $has_remote;


if(!$wpslist) {
    print "Usage: wpsbuilds.pl <WPSLIST> <target>\n",
    "Run this script in the root of the target build, and it will put all the\n",
    "stuff in $tempdir/wps/\n and build the cfg according to $rbdir";
    exit;
}

sub getlcdsizes
{
    my ($remote) = @_;

    open(GCC, ">gcctemp");
    if($remote) {
        # Get the remote LCD screen size
    print GCC <<STOP
\#include "config.h"
#ifdef HAVE_REMOTE_LCD
Height: LCD_REMOTE_HEIGHT
Width: LCD_REMOTE_WIDTH
Depth: LCD_REMOTE_DEPTH
#endif
STOP
;
    }
    else {
    print GCC <<STOP
\#include "config.h"
Height: LCD_HEIGHT
Width: LCD_WIDTH
Depth: LCD_DEPTH
STOP
;
}
    close(GCC);

    my $c="cat gcctemp | gcc $cppdef -I. -I$firmdir/export -E -P -";

    #print "CMD $c\n";

    open(GETSIZE, "$c|");

    my ($height, $width, $depth);
    while(<GETSIZE>) {
        if($_ =~ /^Height: (\d*)/) {
            $height = $1;
        }
        elsif($_ =~ /^Width: (\d*)/) {
            $width = $1;
        }
        elsif($_ =~ /^Depth: (\d*)/) {
            $depth = $1;
        }
        if($height && $width && $depth) {
            last;
        }
    }
    close(GETSIZE);
    unlink("gcctemp");

    return ($height, $width, $depth);
}

sub mkdirs
{
    mkdir "$tempdir/wps", 0777;
    mkdir "$tempdir/themes", 0777;
    mkdir "$tempdir/icons", 0777;

    if( -d "$tempdir/wps/$theme") {
        #print STDERR "wpsbuild warning: directory wps/$theme already exists!\n";
    }
    else
    {
       mkdir "$tempdir/wps/$theme", 0777;
    }
}

sub normalize
{
    my $in = $_[0];
    # strip resolution
    $in =~ s/(\.[0-9]*x[0-9]*x[0-9]*)//;
    return $in;
}

sub copybackdrop
{
    #copy the backdrop file into the build dir
    if ($backdrop ne '') {
        my $dst = normalize($backdrop);
        system("cp $ROOT/$backdrop $tempdir/$dst");
    }
}

sub copythemefont
{
    #copy the font specified by the theme
    my $o = $_[0];

    $o =~ s/\.fnt/\.bdf/;
    mkdir "$tempdir/fonts";
    system("$ROOT/tools/convbdf -f -o \"$tempdir/fonts/$_[0]\" \"$ROOT/fonts/$o\" ");
}

sub copythemeicon
{
    my $i = $_[0];
    #copy the icon specified by the theme
    if ($i ne "-") {
        my $tempicon = $tempdir . "/" . $i;
        $tempicon =~ /\/.*icons\/(.*)/i;
        system("cp $ROOT/icons/$1 $tempicon");
    }
}

sub uniq {
    my %seen = ();
    my @r = ();
    foreach my $a (@_) {
        unless ($seen{$a}) {
            push @r, $a;
            $seen{$a} = 1;
        }
    }
    return @r;
}

sub copywps
{
    # we assume that we copy the WPS files from the same dir the WPSLIST
    # file is located in
    my $dir;
    my %skinfiles = ("wps", $wps,
                     "sbs", $sbs,
                     "fms", $fms,
                     "rwps", $rwps,
                     "rsbs", $rsbs,
                     "rfms", $rfms);
    my @filelist;
    my $file;

    if($wpslist =~ /(.*)\/WPSLIST/) {
        $dir = $1;

        # copy fully-fledged wps, sbs, etc. including graphics
        foreach my $ext (keys %skinfiles) {
            next unless ($skinfiles{$ext});
            $file = $skinfiles{$ext};
            system("cp $dir/$file $tempdir/wps/$theme.$ext");
            open(SKIN, "$dir/$file");
            while (<SKIN>) {
                $filelist[$#filelist + 1] = $1 if (/[\(,]([^,]*?.bmp)[\),]/);
            }
            close(SKIN);
        }

        if ($#filelist >= 0) {
            if (-e "$dir/$theme") {
                foreach $file (uniq(@filelist)) {
                    system("cp $dir/$theme/$file $tempdir/wps/$theme/");
                }
            }
            else {
                print STDERR "beep, no dir to copy WPS from!\n";
            }
        }
    } else {
        print STDERR "No source directory!\n";
    }
}

sub buildcfg {
    my $cfg = $theme . ".cfg";
    my @out;

    push @out, <<MOO
\#
\# $cfg generated by wpsbuild.pl
\# $wps is made by $author
\#
MOO
;

    my %skinfiles = ("wps" => $wps,
                     "sbs" => $sbs,
                     "fms" => $fms,
                     "rwps" => $rwps,
                     "rsbs" => $rsbs,
                     "rfms" => $rfms);
    for my $skin (keys %skinfiles) {
        my $val = $skinfiles{$skin};
        if (!defined($val)) {
            # dont put value if not defined (e.g. rwps when there's no remote)
            next;
        } elsif ($val eq '') {
            # empty resets to built-in
            push @out, "$skin: -\n";
        } else {
            # file name is always <theme>.{wps,rwps,...} (see copywps())
            push @out, "$skin: $rbdir/wps/$theme.$skin\n";
        }
    }

    push @out, "selector type: $selecttype\n"   if (defined($selecttype));
    push @out, "backdrop: $backdrop\n"          if (defined($backdrop));
    push @out, "filetype colours: $filetylecolor\n" if (defined($filetylecolor));

    if ($main_depth > 2) {
        push @out, "foreground color: $fgcolor\n"                     if($fgcolor);
        push @out, "background color: $bgcolor\n"                     if($bgcolor);
        push @out, "line selector start color: $lineselectstart\n"    if($lineselectstart);
        push @out, "line selector end color: $lineselectend\n"        if($lineselectend);;
        push @out, "line selector text color: $lineselecttextcolor\n" if($lineselecttextcolor);
    }

    push @out, "font: $font\n"                  if (defined($font));
    push @out, "statusbar: $statusbar\n"        if (defined($statusbar));
    push @out, "iconset: $iconset\n"            if (defined($iconset));
    push @out, "viewers iconset: $viewericon\n" if (defined($viewericon));
    push @out, "ui viewport: $listviewport\n"   if (defined($listviewport));

    if ($has_remote) {
        push @out, "remote font: $remotefont\n"                  if (defined($remotefont));
        push @out, "remote statusbar: $remotestatusbar\n"        if (defined($remotestatusbar));
        push @out, "remote iconset: $remoteiconset\n"            if (defined($remoteiconset));
        push @out, "remote viewers iconset: $remoteviewericon\n" if (defined($remoteviewericon));
        push @out, "remote ui viewport: $remotelistviewport\n"   if (defined($remotelistviewport));
    }

    if(-f "$tempdir/wps/$cfg") {
        print STDERR "wpsbuild warning: wps/$cfg already exists!\n";
    }
    else {
        open(CFG, ">$tempdir/themes/$cfg");
        print CFG @out;
        close(CFG);
    }
}

# Get the LCD sizes first
($main_height, $main_width, $main_depth) = getlcdsizes();
($remote_height, $remote_width, $remote_depth) = getlcdsizes(1);

#print "LCD: ${main_width}x${main_height}x${main_depth}\n";
$has_remote = 1 if ($remote_height && $remote_width && $remote_depth);


# check if line matches the setting string or if it contains a regex
# that contains the targets resolution
sub check_res {
    my ($line, $string, $remote) = @_;
    if ($line =~ /^${string}: *(.*)/i) {
        return $1;
    }
    elsif($line =~ /^${string}.(.*): *(.*)/i) {
        # $1 is a resolution regex, $2 the filename incl. resolution
        my $fn = $2;
        my $size_str = "${main_width}x${main_height}x${main_depth}";
        if ($remote) {
            $size_str = "${remote_width}x${remote_height}x${remote_depth}";
        }
        if ($size_str =~ /$1$/) {
            return $fn;
        }
    }
    return "";
}

# check if <theme>.<model>.<ext> exists. If not, check if <theme>.<ext> exists
sub check_skinfile {
    my $ext = $_[0];
    my $req_skin = $theme . "." . $modelname . ".$ext";
    if (-e "$wpsdir/$req_skin") {
        return $req_skin;
    } else {
        $req_skin = $theme . ".$ext";
        if (-e "$wpsdir/$req_skin") {
            return $req_skin;
        }
    }
    return '';
}


# Infer WPS (etc.) filename from the the if it wasnt given
$wpslist =~ /(.*)WPSLIST/;
$wpsdir = $1;

open(WPS, "<$wpslist");
while(<WPS>) {
    my $l = $_;
    
    # remove CR
    $l =~ s/\r//g;
    if($l =~ /^ *\#/) {
        # skip comment
        next;
    }

    # prefix $rbdir with / if needed (needed for the theme.cfg)
    unless ($rbdir =~ m/^\/.*/) {
        $rbdir = "/" . $rbdir;
    }

    if($l =~ /^ *<theme>/i) {
        # undef is a unary operator (!)
        undef $theme;
        undef $has_wps;
        undef $has_rwps;
        undef $has_sbs;
        undef $has_rsbs;
        undef $has_fms;
        undef $has_rfms;
        undef $wps;
        undef $rwps;
        undef $sbs;
        undef $rsbs;
        undef $fms;
        undef $rfms;
        undef $width;
        undef $height;
        undef $font;
        undef $remotefont;
        undef $fgcolor;
        undef $bgcolor;
        undef $statusbar;
        undef $remotestatusbar;
        undef $author;
        undef $backdrop;
        undef $lineselectstart;
        undef $lineselectend;
        undef $selecttype;
        undef $iconset;
        undef $remoteiconset;
        undef $viewericon;
        undef $remoteviewericon;
        undef $lineselecttextcolor;
        undef $filetylecolor;
        undef $listviewport;
        undef $remotelistviewport;
    }
    elsif($l =~ /^Name: *(.*)/i) {
        $theme = $1;
    }
    elsif($l =~ /^Authors: *(.*)/i) {
        $author = $1;
    }
    elsif ($l =~ /^WPS: *(yes|no)/i) {
        $has_wps = $1;
    }
    elsif ($l =~ /^RWPS: *(yes|no)/i) {
        $has_rwps = $1;
    }
    elsif ($l =~ /^SBS: *(yes|no)/i) {
        $has_sbs = $1;
    }
    elsif ($l =~ /^RSBS: *(yes|no)/i) {
        $has_rsbs = $1;
    }
    elsif ($l =~ /^FMS: *(yes|no)/i) {
        $has_fms = $1;
    }
    elsif ($l =~ /^RFMS: *(yes|no)/i) {
        $has_rfms = $1;
    }
    elsif($l =~ /^ *<main>/i) {
        # parse main unit settings
        while(<WPS>) {
            my $l = $_;
            if ($l =~ /^ *<\/main>/i) {
                last;
            }
            elsif($_ = check_res($l, "wps")) {
                $wps = $_;
            }
            elsif($_ = check_res($l, "sbs")) {
                $sbs = $_;
            }
            elsif($_ = check_res($l, "fms")) {
                $fms = $_;
            }
            elsif($_ = check_res($l, "Font")) {
                $font = $_;
            }
            elsif($_ = check_res($l, "Statusbar")) {
                $statusbar = $_;
            }
            elsif($_ = check_res($l, "Backdrop")) {
                $backdrop = $_;
            }
            elsif($l =~ /^Foreground Color: *(.*)/i) {
                $fgcolor = $1;
            }
            elsif($l =~ /^Background Color: *(.*)/i) {
                $bgcolor = $1;
            }
            elsif($l =~ /^line selector start color: *(.*)/i) {
                $lineselectstart = $1;
            }
            elsif($l =~ /^line selector end color: *(.*)/i) {
                $lineselectend = $1;
            }
            elsif($_ = check_res($l, "selector type")) {
                $selecttype = $_;
            }
            elsif($_ = check_res($l, "iconset")) {
                $iconset = $_;
            }
            elsif($_ = check_res($l, "viewers iconset")) {
                $viewericon = $_;
            }
            elsif($l =~ /^line selector text color: *(.*)/i) {
                $lineselecttextcolor = $1;
            }
            elsif($l =~ /^filetype colours: *(.*)/i) {
                $filetylecolor = $1;
            }
            elsif($_ = check_res($l, "ui viewport")) {
                $listviewport = $_;
            }
        }
    }
    elsif($l =~ /^ *<remote>/i) {
        while(<WPS>) {
            # parse remote settings
            my $l = $_;
            if ($l =~ /^ *<\/remote>/i) {
                last;
            }
            elsif(!$has_remote) {
                next; # dont parse <remote> section
            }
            elsif($_ = check_res($l, "rwps", 1)) {
                $rwps = $_;
            }
            elsif($_ = check_res($l, "rsbs", 1)) {
                $rsbs = $_;
            }
            elsif($_ = check_res($l, "rfms", 1)) {
                $rfms = $_;
            }
            elsif($_ = check_res($l, "Font", 1)) {
                $remotefont = $_;
            }
            elsif($_ = check_res($l, "iconset", 1)) {
                $remoteiconset = $_;
            }
            elsif($_ = check_res($l, "viewers iconset", 1)) {
                $remoteviewericon = $_;
            }
            elsif($_ = check_res($l, "statusbar", 1)) {
                $remotestatusbar = $_;
            }
            elsif($_ = check_res($l, "ui viewport", 1)) {
                $remotelistviewport = $_;
            }
        }
    }
    elsif($l =~ /^ *<\/theme>/i) {
        # for each wps,sbs,fms (+ remote variants) check if <theme>[.<model>].wps
        # exists if no filename was specified in WPSLIST
        my $req_skin;

        if ($has_wps eq "yes" && !$wps) {
            $wps = check_skinfile("wps");
        } elsif ($has_wps eq "no") {
            $wps = '';
        }

        if ($has_sbs eq "yes" && !$sbs) {
            $sbs = check_skinfile("sbs");
        } elsif ($has_sbs eq "no") {
            $sbs = '';
        }

        if ($has_fms eq "yes" && !$fms) {
            $fms = check_skinfile("fms");
        } elsif ($has_fms eq "no") {
            $fms = '';
        }

        # now check for remote skin files (use main screen's extension)
        if ($has_remote) {
            if ($has_rwps eq "yes" && !$rwps) {
                $rwps = check_skinfile("wps");
            } elsif ($has_rwps eq "no") {
                $rwps = '';
            }

            if ($has_rsbs eq "yes" && !$rsbs) {
                $rsbs = check_skinfile("sbs");
            } elsif ($has_rsbs eq "no") {
                $rsbs = '';
            }

            if ($has_rfms eq "yes" && !$rfms) {
                $rfms = check_skinfile("fms");
            } elsif ($has_rfms eq "no") {
                $rfms = '';
            }
        }
        #print "LCD: $wps wants $width x $height\n";

        #
        # The target model has an LCD that is suitable for this
        # WPS
        #
        #print "Size requirement is fine!\n";
        mkdirs() if (-e "$wpsdir/$theme");
        # Do the copying before building the .cfg - buildcfg()
        # mangles some filenames
        if (defined($backdrop) && $backdrop ne "-") {
            copybackdrop();
            $backdrop = normalize($backdrop);
        }
        foreach my $i ($iconset, $viewericon, $remoteiconset, $remoteviewericon) {
            if (defined($i) && $i ne "-") {
                copythemeicon($i);
            }
        }
        if (defined($font) && $font ne "-") {
            copythemefont($font);
            $font = "$rbdir/fonts/$font";
        }
        if (defined($remotefont) && $remotefont ne "-") {
            copythemefont($remotefont);
            $remotefont = "$rbdir/fonts/$remotefont";
        }
        buildcfg();
        copywps();
    }
    else{
        #print "Unknown line:  $l!\n";
    }
}

close(WPS);
