#!/usr/bin/perl

while(<STDIN>) {

    $_ =~ s/\</&lt;/g;
    $_ =~ s/\>/&gt;/g;

    $_ =~ s/^$/\&nbsp;/g;

    print $_;
}
