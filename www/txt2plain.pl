#!/usr/bin/perl

while(<STDIN>) {

    $_ =~ s/\</&lt;/g;
    $_ =~ s/\>/&gt;/g;

    $_ =~ s/(http:\/\/([a-zA-Z0-9_.\/-]*)[^).])/\<a href=\"$1\"\>$1\<\/a\>/g;

    $_ =~ s/^$/\&nbsp;/g;   # empty lines are nbsp
    $_ =~ s/(\\|\/)$/$1&nbsp;/g; # clobber backslash on end of line


    print $_;
}
