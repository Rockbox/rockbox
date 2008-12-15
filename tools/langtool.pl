#!/usr/bin/perl
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
# KIND, either express or implied.

sub usage {
    print <<MOO
Usage langtool [--inplace] --options langfile1 [langfile2 ...]

  For all actions, the modified langfile will be output on stdout. When doing
  stuff to english.lang, you should almost always apply the same change to all
  other languages to avoid bothering translators.

  --deprecate --id ID_1,ID_2 --id ID_3 langfile

    Deprecate a number of ids.
    Example: langtool --id LANG_LEFT,LANG_RIGHT --LANG_BOTTOM english.lang

  --changesource --id ID --target target --to string langfile
    
    Change the source text of a specific string specified by id and target. Use
    this on all langfiles if you're changing a source in english.lang that
    doesn't need attention from translators (case changes etc.).
    Example:
      langtool --changesource --id LANG_OK --target e200 --to "OK" english.lang

  --changeid --from LANG_LFET --to LANG_LEFT langfile
    
    Change the name of an ID. THIS WILL BREAK BACKWARDS COMPATIBILITY. Use with
    extreme caution.
    Example: langtool --changeid --from LANG_OK --to LANG_YES english.lang

  --changedesc --to string --id LANG_LEFT langfile

    Change the desc for an ID.
    Example: langtool --changedesc --to "New desc" --id LANG_OK english.lang

  --changetarget --from target --to target --id ID1 langfile

    Change the target for the specified id from one value to another
    Example:
      langtool --changetarget --from e200 --to e200,c200 --id LANG_ON dansk.lang

  --inplace

    Perform file operations in-place, instead of outputting the result to
    stdout. With this option set, you can specify multiple langfiles for
    all commands.
    Example: langtool --deprecate --id LANG_ASK --inplace *.lang
MOO
}

use Getopt::Long;
use strict;

# Actions
my $deprecate = '';
my $changesource = '';
my $changeid = '';
my $changetarget = '';
my $changedesc = '';
my $inplace = '';
my $help = '';
# Parameters
my @ids = ();
my $from = '';
my $to = '';
my $s_target = '';

GetOptions(
    'deprecate'    => \$deprecate,
    'changesource' => \$changesource,
    'changeid'     => \$changeid,
    'changetarget' => \$changetarget,
    'changedesc'   => \$changedesc,
    'help'         => \$help,
    'inplace'      => \$inplace,

    'ids=s'        => \@ids,
    'from=s'       => \$from,
    'to=s'         => \$to,
    'target=s'     => \$s_target,
);
# Allow comma-separated ids as well as several --id switches
@ids = split(/,/,join(',',@ids));
my $numids = @ids;
my $numfiles = @ARGV;

# Show help if necessary
if (
    $help
        or # More than one option set
        ($deprecate + $changesource + $changeid + $changetarget + $changedesc) != 1
        or # Do changeid, but either from or to is empty
        ($changeid and ($from eq "" or $to eq ""))
        or # Do changedesc, but to isn't set
        ($changedesc and $to eq "")
        or # Do changetarget, but 
        ($changetarget and ($from eq "" or $to eq ""))
        or # Do deprecate, but no ids set
        ($deprecate and $numids < 1)
        or # Do changesource, but either target or to not set
        ($changesource and ($s_target eq "" or $to eq ""))
        or # More than one file passed, but inplace isn't set
        ($numfiles > 1 and not $inplace)
   ) {
    usage();
    exit(1);
}

# Check that all supplied files exist before doing anything
foreach my $file (@ARGV) {
    if (not -f $file) {
        printf("File doesn't exist: %s\n", $file);
        exit(2);
    }
}

if ($changesource and not $to =~ /none|deprecated/) {
    $to = sprintf('"%s"', $to);
}

foreach my $file (@ARGV) {
    print(STDERR "$file\n");
    open(LANGFILE, $file) or die(sprintf("Couldn't open file for reading: %s", $file));
    my $id = "";
    my $desc = "";
    my $location = "";
    my $target = "";
    my $string = "";
    my $open = 0;
    my $output = "";

    for (<LANGFILE>) {
        my $line = $_;

        ### Set up values when a tag starts or ends ###
        if ($line =~ /^\s*<(\/?)([^>]+)>\s*$/) {
            my $tag = $2;
            $open = $1 eq "/" ? 0 : 1;
            if ($open) {
                $location = $tag;
                ($target, $string) = ("", "");
            }
            if ($open and $tag eq "phrase") {
                $id = "";
            }
            if (not $open) {
                $location = "";
            }
        }
        ### Set up values when a key: value pair is found ###
        elsif ($line =~ /^\s*([^:]*?)\s*:\s*(.*?)\s*$/) {
            my ($key, $val) = ($1, $2);
            if ($location =~ /source|dest|voice/) {
                ($target, $string) = ($key, $val);
            }
            if ($key eq "id") {
                $id = $val;
            }
            elsif ($key eq "desc") {
                $desc = $val;
            }
        }

        if ($deprecate) {
            if ($id ne "" and grep(/$id/, @ids)) {
                # Set desc
                $line =~ s/\s*desc:.*/  desc: deprecated/;
                # Set user
                $line =~ s/\s*user:.*/  user:/;
                # Print an empty target line after opening tag (target isn't set)
                if ($location =~ /source|dest|voice/ and $target eq "") {
                    $line .= "    *: none\n";
                }
                # Do not print target: string lines
                elsif ($location =~ /source|dest|voice/ and $target ne "") {
                    $line = "";
                }
            }
        }
        elsif ($changetarget) {
            # Change target if set and it's the same as $from
            if ($id ne "" and grep(/$id/, @ids) and $location =~ /source|dest|voice/ and $target eq $from) {
                $line =~ s/$from/$to/;
            }
        }
        elsif ($changesource) {
            # Change string if $target is set and matches $s_target
            if ($id ne "" and grep(/$id/, @ids) and $target eq $s_target and $location eq "source") {
                $line =~ s/$string/$to/;
            }
        }
        elsif ($changedesc) {
            # Simply change the desc line if the id matches
            if ($id ne "" and grep(/$id/, @ids)) {
                $line =~ s/\s*desc:.*/  desc: $to/;
            }
        }
        elsif ($changeid) {
            $line =~ s/^\s*id:\s*$from.*$/  id: $to/;
        }
        else {
            print("This should never happen.\n");
            exit(3);
        }
        if ($inplace) {
            $output .= $line;
        }
        else {
            print($line);
        }
    }
    close(LANGFILE);
    if ($inplace) {
        open(LANGFILE, ">", $file) or die(sprintf("Couldn't open file for writing: %s\n", $file));
        print(LANGFILE $output);
        close(LANGFILE);
    }
}
