#! /usr/bin/perl -w

# Wordnet dictionary database converter
#
# Converts the Wordnet prolog data to rockbox dictionary format.
#
# Written by Miika Pekkarinen <slasher@ihme.org>
#
# $Id$

use strict;

# Lookup tables
my %words;
my %descriptions;

sub getcatname {
	my ($id) = @_;
	
	return 'N' if $id == 1;
	return 'V' if $id == 2;
	return 'A' if $id == 3;
	return 'A' if $id == 4;
	return '?';
}

open IN_WORD, "wn_s.pl" or die "Open fail(#1): $!";
open IN_DESC, "wn_g.pl" or die "Open fail(#2): $!";
open OUTPUT, "> dict.preparsed" or die "Open fail(#3): $!";

print "Reading word file...\n";

# Read everything into memory
while (<IN_WORD>) {
	chomp ;
	
	# s(100001740,1,'entity',n,1,11). => 100001740,1,'entity',n,1,11
	s/(^s\()(.*)(\)\.$)/$2/;
	
	my ($seqid, $n1, $word, $n2, $n3, $n4) = split /,/, $_, 6;
	
	# 'entity' => entity
	$word =~ s/(^\')(.*)(\'$)/$2/;
	$word =~ s/\'\'/\'/s;
	
	my $category = substr $seqid, 0, 1;
	
	$words{lc $word}{$seqid} = $category;
}

close IN_WORD;

print "Reading description file...\n";
while (<IN_DESC>) {
	chomp ;
	
	# g(100002056,'(a separate and self-contained entity)').
	# => 100002056,'(a separate and self-contained entity)'
	s/(^g\()(.*)(\)\.$)/$2/;
	
	my ($seqid, $desc) = split /,/, $_, 2;
	
	$desc =~ s/(^\'\()(.*)(\)\'$)/$2/;
	$desc =~ s/\'\'/\'/s;
	
	$descriptions{$seqid} = $desc;
}

close IN_DESC;

print "Sorting and writing output...\n";

# Now sort and find correct descriptions
foreach my $word (sort keys %words) {
	my %categories;
	
	# Find all definitions of the word
	foreach my $id (keys %{$words{$word}}) {
		my $catid = $words{$word}{$id};
		my $description = $descriptions{$id};
		
		if (!defined($description) or $description eq '') {
			print "Error: Failed to link word: $word / ",
			  $words{$word}, "\n";
			exit 1;
		}
		
		push @{$categories{$catid}}, $description;
	}
	
	my $finaldesc;
	
	# 1 = noun
	# 2 = verb
	# 3 = adjective
	# 4 = adverb
	for my $catid (1 .. 4) {
		my $n = 1;
		my $catdesc;
		
		next unless $categories{$catid};
		foreach my $desc ( @{$categories{$catid}} ) {
			$catdesc .= " " if $catdesc;
			$catdesc .= "$n. $desc";
			$n++;
		}
		
		next unless $catdesc;
		$finaldesc .= "\t" if $finaldesc;
		$finaldesc .= getcatname($catid) . ": $catdesc"
	}
	
	die "Internal error" unless $finaldesc;
	
	print OUTPUT "$word\t$finaldesc\n";
}

close OUTPUT;

print "Done, output was successfully written!\n";


