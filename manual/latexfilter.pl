#!/usr/bin/perl -s
#
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#

my $verbose = $v;
my $reflowed = "";
my $last = "";
my $currentfile;
while(<STDIN>) {
    chomp $_;
    $reflowed .= $_;
    if(/^.{79,}$/) {
        # collapse all "full" lines
    }
    elsif(/^!/) {
        # collapse lines indicating an error with next one and append a space.
        $reflowed .= " ";
    }
    else {
        # skip empty lines
        if(length($reflowed) > 0) {
            # collapse with previous line if it continues some "area".
            if($reflowed =~ /^\s*(\]|\[|\\|\)|<)/) {
                $last .= $reflowed;
            }
            else {
                # find current input file
                my @inputfile = $last =~ /\(([a-zA-Z_\-\/\.]+\.tex)/g;
                foreach(@inputfile) {
                    if($verbose) {
                        print "\n";
                    }
                    print "LaTeX processing $_\n";
                    $currentfile = $_;
                }
                if($verbose) {
                    print $last;
                }
                # check for error
                if($reflowed =~ /^!\s*(.*)/) {
                    my $l = $reflowed;
                    $l =~ s/^!\s*(.*)l.(\d+) /$2: $1/;
                    print "$currentfile:$l\n";
                }
                # restart for next reflowed line
                $last = $reflowed;
            }
        }
        # restart reflowing.
        $reflowed = "";
    }
}
if($verbose) {
    print $last;
}

