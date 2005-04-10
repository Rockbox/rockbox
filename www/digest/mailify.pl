#!/usr/bin/perl

my $end++;

my @out;
my $url;

my $urlnum=1;

sub showlinks {
    my ($date)=@_;
    if(scalar(keys %store)) {
        print "\n";
        
        for(sort {$store{$a} <=> $store{$b} } keys %store) {
            my $url=$_;
            
            $url =~ s/^\"(.*)\"/$1/g;
            printf("[%d] = %s\n", $store{$_}, $url);
        }
        undef %store;
        $urlnum=1; # reset to 1
    }
}

sub showitem {
    my @text=@_;

    if(@text) {
        my $c=3;
        my $width=72;
        print " * ";

        my $thelot = join(" ", @text);
        
        for (split(/[ \t\n]/, $thelot)) {
            my $word = $_;

            $word =~ s/[ \t]//g;

            my $len = length($word);
            if(!$len) {
                next; # skip blanks
            }

            if($len + $c + 1> $width) {
                print "\n   ";
                $c = 3;
            }
            elsif($c != 3) {
                print " ";
                $c++;
            }
            print $word;
            $c += $len;
        }
    }
    print "\n"; # end of item

 #   my @words=split(

}

sub date2secs {
    my ($date)=@_;
    my $secs = `date -d "$date" +"%s"`;
    return 0+$secs;
}

while(<STDIN>) {
    my $line = $_;

    if($_ =~ /^Date: (.*)/) {
        my $date=$1;
        my $secs = date2secs($date);
        my $tendaysago = time()-(14*24*60*60);
        showitem(@out);
        undef @out;
        chomp;

        showlinks();

        if($secs < $tendaysago) {
            last;
        }

        print "\n----------------------------------------------------------------------\n$_";
        next;
    }
    elsif($line =~ /^ *(---)(.*)/) {

        showitem(@out);

        @out="";
        $line = $2;
    }

    if($line =~ s/\[URL\](.*)\[URL\]//) {
        $url=$1;

        if(!$store{$url}) {
            $footnote = "[$urlnum]";
            $store{$url} = $urlnum;
            $urlnum++;
        }
        else {
            $footnote = "[".$store{$url}."]";
        }
 #       print STDERR "Set $footnote for $url\n";
    }
    if($line =~ s/\[TEXT\](.*)\[TEXT\]/$1$footnote/) {
 #       print STDERR "Output $footnote (full TEXT)\n";
        undef $text;
    }
    elsif(!$text && ($line =~ s/\[TEXT\](.*)/$1/)) {
 #       print STDERR "Detected start of TEXT\n";
        $text = $1;
    }
    elsif($text && ($line =~ s/(.*)\[TEXT\]/$1$footnote/)) {
 #       print STDERR "Output $footnote (end-TEXT)\n";
        undef $text;
    }

    push @out, $line;
}

if(@out) {
    showitem(@out);
}

