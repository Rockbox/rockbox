#!/usr/bin/perl

# this is really a faq2html and should only be used for this purpose

sub fixline {
    $_ =~ s/\</&lt;/g;
    $_ =~ s/\>/&gt;/g;

    $_ =~ s/(http:\/\/([a-zA-Z0-9_.\/-]*)[^\) .\n])/\<a href=\"$1\"\>$1\<\/a\>/g;

    $_ =~ s/^\s*$/\&nbsp;\n/g;   # empty lines are nbsp
    $_ =~ s/(\\|\/)$/$1&nbsp;/g; # clobber backslash on end of line
}

while(<STDIN>) {

    fixline($_);

    # detect and mark Q-sections
    if( $_ =~ /^Q(\d*)/) {
        print "</pre>\n<a name=\"$1\"></a><div class=\"faqq\">$_";
        my $line;
        while(<STDIN>) {

            fixline($_);

            $line = $_;
            if($_ !~ /^A/) {
                print "$_";
            }
            else {
                last;
            }
        }
        print "</div>\n<pre class=\"faqa\">$line";
        next;
    }

    print $_;
}
