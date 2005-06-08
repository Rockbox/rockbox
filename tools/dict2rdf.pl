#!/usr/bin/perl

#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#
# Copyright (C) 2005 Tony Motakis
#
# All files in this archive are subject to the GNU General Public License.
# See the file COPYING in the source tree root for full license agreement.
#
# This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
# KIND, either express or implied.

# set the word size limit
$word_limit = 32;

use Compress::Zlib;

# generate base 64 convertion hash
@b64_values = (	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
		'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
		'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
		'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
		'w', 'x', 'y', 'z', '0', 1, 2, 3, 4, 5, 6, 7, 8, 9, '+', '/' );

foreach (0..63) {
	$b64_get_value{$b64_values[$_]} = $_;
}

# base 64 convertion subroutine. note that if input is plain (base 64) 0, perl
# doesn't like it, and the function misinterprents it as a (decimal) 0
# while it actually is a (decimal) 52. Input has a tab in front anyway, so
# this bug actually doesn't matter
sub base64 {
	my $i = 1, $num = 0, $left = $_[0];
	while($left) {
		$left =~ m{([^\s])$};		# use last char of string
		chop $left;			# yes, chop, NOT chomp
		$num += $i * $b64_get_value{$1};
		$i *= 64;
	}
	$num;
}

# Open input files. <INDEX> is the database index, and $DICT is the actuall
# dictionary file we want to access (note the use of zlib, hence the $DICT
# variable instead of a <DICT> filehandle). <RDFOUT> is the output file, in
# plain rockbox dictionary format
open INDEX, $ARGV[0] or die "Could not open index: $!";
$DICT = gzopen($ARGV[1], "rb") or die "Could not open definitions file: $!";
open RDFOUT, ">$ARGV[2]" or die "Could not open output file: $!";

# Read the index
while(<INDEX>)
{
	next if /^00-?database/;

	my @current = split /\t|\n/;			# split in pieces
	$current[0] =~ s/^\s(.{1,$word_limit}).*$/\L\1/;	# lowercase
	push @def_list, $current[0];
	$def_begin{$current[0]} = base64($current[1]);
	$def_length{$current[0]} = base64($current[2]);
}

# sort the definition list. input from the <INDEX> is usualy sorted, but this
# is not mandatory in the dict file format, so we can't rely on this
@def_list = sort @def_list;

# read the whole DICT file into memory. overkill? propably. but the file is
# compressed, and we need quick access to random parts of it
$def_all .= $_ while($DICT->gzread($_));

foreach (@def_list) {
	$def = substr $def_all, $def_begin{$_}, $def_length{$_};
	$def =~ s/\n\s*/ /g;	# remove newlines and whitespace after them
	print RDFOUT $_ . "\t" . $def . "\n";
}

