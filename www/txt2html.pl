#!/usr/bin/perl

# this is really a faq2html and should only be used for this purpose

sub fixline {
    $_ =~ s/\</&lt;/g;
    $_ =~ s/\>/&gt;/g;

    $_ =~ s/(http:\/\/([a-zA-Z0-9_.\/-]*)[^\) .\n])/\<a href=\"$1\"\>$1\<\/a\>/g;

    $_ =~ s/(\\|\/)$/$1&nbsp;/g; # clobber backslash on end of line
}

while(<STDIN>) {
    fixline($_);
    push @p, "$_";
}

print "<pre>\n";
print @p;
print "</pre>\n";




