#!/usr/local/bin/perl

# this is really a faq2html and should only be used for this purpose

sub fixline {
    $_ =~ s/\</&lt;/g;
    $_ =~ s/\>/&gt;/g;

    $_ =~ s/(http:\/\/([a-zA-Z0-9_.\#\/-]*)[^\) .\n])/\<a href=\"$1\"\>$1\<\/a\>/g;

    $_ =~ s/(\\|\/)$/$1&nbsp;/g; # clobber backslash on end of line
}

sub show {
    if(@q) {
        print @q;
        undef @q;
    }
    if(@a) {
        print @a;
        undef @a;
    }
    if(@p) {
        print "<pre>\n";
        print @p;
        print "</pre>\n";
        undef @p;
    }
}

while(<STDIN>) {

    fixline($_);

    # detect and mark Q-sections
    if( $_ =~ /^(Q(\d*)[.:] )(.*)/) {

        show();

        # collect the full Q
        push @q, "<a name=\"$2\"></a><p class=\"faqq\">";
        push @q, "$2. $3";
        my $line;

        $indent = length($1);
        $first = " " x $indent;

        #print "$indent|$first|$1|\n";

        while(<STDIN>) {

            fixline($_);

            $line = $_;

            if($_ !~ /^A/) {
                push @q, "$_";
            }
            else {
                last;
            }
        }
        # first line of A
        $line =~ s/^A(\d*)[.:] *//g; # cut off the "A[num]."
        push @a, "<p class=\"faqa\">";
        push @a, $line;

        $prev='a';
        next;
    }
  #  print "$_ matches '$first'?\n";

    if($_ =~ /^$first(\S)/) {


        if($prev ne 'a') {
            show();
            push @a, "<p class=\"faqa\">";
        }

        push @a, $_;
        $prev='a';
    }
    else {
        if($prev ne 'p') {
            show();
        }
        if(@p) {
            # if we have data, we fix blank lines
            $_ =~ s/^\s*$/\&nbsp;\n/g;   # empty lines are nbsp
            push @p, $_; # add it
        }
        elsif($_ !~ /^\s*$/) {
            # this is not a blank line, add it
            push @p, $_;
        }
        $prev = 'p';
    }
}
show();

