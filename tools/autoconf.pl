#!/usr/bin/perl
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

# This program attempts to run configure with the correct build target
# and type based on the pwd.
# example: ~/rockbox/sansae200 is the build dir, it would run configure
# for the sansae200 normal build target.
# ~/rockbox/sansae200-sim for the e200 sim build (-boot for bootloader)
# ~/rockbox/sim/sansae200 for e200 sim build. (replace sim with boot is also possible)
# The full shortname is not required, each target name is checked and the first
# possible match is used.

# This script must be placed in the same directory as configure and builds.pm
# 

use File::Basename;
my $srcdir = dirname $0;
require "$srcdir/builds.pm";

my $builddir = `pwd`;
my @dirs = split(/\//, $builddir);

my $test = pop(@dirs);

sub doconfigure {
	my ($target, $type) = @_;
	if (!exists($builds{$target})) {
		for $key (keys(%builds)) {
			if ($key =~ $target) {
				$target = $key;
				last;
			}
		}
	}
	$command = "${srcdir}/configure --type=${type} --target=${target}";
	%typenames = ("n" => "Normal", "s" => "Simulator", "b" => "Bootloader" );
    unless (@ARGV[0] eq "-y") {
        print "Rockbox autoconf: \n\tTarget: $target \n\tType: $typenames{$type} \nCorrect? [Y/n] ";
        chomp($response = <>);
        if ($response eq "") {
            $response = "y";
        }
        if ($response ne "y" && $response ne "Y") {
            print "autoconf: Aborting\n";
            exit(0);
        }
    }
	system($command);
}

sub buildtype {
	my ($text) = @_;
	if ($text eq "sim") {
		$build = "s";
	} elsif ($text eq "boot") {
		$build = "b";
    } elsif ($text eq "build") {
		$build = "n";
	} else {
        $build = "";
    }
	return $build;
}

if ($test =~ /(.*)-(.*)/)
{
    if (buildtype($2)) {
        doconfigure($1, buildtype($2));
    } elsif (buildtype($1)) {
        doconfigure($2, buildtype($1));
    }
}
elsif ($test =~ /(.*)/)
{
	$target = $1;
	$build = buildtype(pop(@dirs));
	doconfigure($target, $build);
}
