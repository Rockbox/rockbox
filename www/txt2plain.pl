#!/usr/bin/perl

# this is really a faq2html and should only be used for this purpose

while(<STDIN>) {

    $_ =~ s/\</&lt;/g;
    $_ =~ s/\>/&gt;/g;

    $_ =~ s/(http:\/\/([a-zA-Z0-9_.\/-]*)[^\).])/\<a href=\"$1\"\>$1\<\/a\>/g;

    $_ =~ s/^$/\&nbsp;/g;   # empty lines are nbsp
    $_ =~ s/(\\|\/)$/$1&nbsp;/g; # clobber backslash on end of line


    # detect and mark Q-sections
    if( $_ =~ /^Q(\d*)/) {
        print "</pre>\n<a name=\"$1\"></a><p class=\"faqq\">$_";
        my $line;
        while(<STDIN>) {
            $line = $_;
            if($_ !~ /^A/) {
                print "$_";
            }
            else {
                last;
            }
        }
        print "</p>\n<pre class=\"faqa\">\n$line";
        next;
    }

    print $_;
}
