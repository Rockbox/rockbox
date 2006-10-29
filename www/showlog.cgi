#!/usr/bin/perl

require "CGI.pm";

$req = new CGI;

$date = $req->param('date');
$type = $req->param('type');

print "Content-Type: text/html\n\n";

print <<MOO
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">

<html>
<head>
<link rel="STYLESHEET" type="text/css" href="/style.css">
<title>Rockbox: $type $date</title>
<meta name="author" content="Daniel Stenberg, in perl">
</head>
<body bgcolor="#b6c6e5" text="black" link="blue" vlink="purple" alink="red"
      topmargin=3 leftmargin=4 marginwidth=4 marginheight=4>
MOO
    ;


print "<h1>$date, $type</h1>\n";

my @o;
my $prob;
my $lserver;
my $buildtime;

if($date =~ /(....)-(..)-(..)/) {
    my $file = "allbuilds-$1$2$3";

    open(LOG, "</home/dast/rockbox-distbuild/output/$file");
    while(<LOG>) {
        if($_ =~ /^Build Server: (.*)/) {
            $lserver = $1;
        }
        if($_ =~ /^Build Time: (.*)/) {
            $buildtime = $1;
        }
        if( $_  =~ /^Build Date: (.*)/) {
            if($date eq $1) {
                $match++;
            }
            else {
                $match=0;
            }
        }
        elsif( $_  =~ /^Build Type: (.*)/) {
            if($type eq $1) {
                $match++;
            }
            else {
                $match=0;
            }
        }
        elsif(($match == 2) &&
              ($_  =~ /^Build Log Start/)) {
            $match++;
            push @o, "<div class=\"gccoutput\">";
        }
        elsif($match == 3) {
            if($_  =~ /^Build Log End/) {
                $match=0;
            }
            else {
                my $class="";
                $_ =~ s:/home/dast/rockbox-auto/::g;
                $line = $_;
                chomp $line;

                if($lserver) {
                    push @o, "Built on <b>$lserver</b> in $buildtime seconds<br>";
                    $lserver="";
                }

                if($line =~ /^([^:]*):(\d*):.*warning:/) {
                    $prob++;
                    push @o, "<a name=\"prob$prob\"></a>\n";
                    push @o, "<div class=\"gccwarn\">$line</div>\n";
                }
                elsif($line =~ /^([^:]*):(\d+):/) {
                    $prob++;
                    push @o, "<a name=\"prob$prob\"></a>\n";
                    push @o, "<div class=\"gccerror\">$line</div>\n";
                }
                elsif($line =~ /(: undefined reference to|ld returned (\d+) exit status|gcc: .*: No such file or)/) {
                    $prob++;
                    push @o, "<a name=\"prob$prob\"></a>\n";
                    push @o, "<div class=\"gccerror\">$line</div>\n";
                }
                else {
                    push @o, "$line\n<br>\n";
                }
            }
        }
    }
    close(LOG);

    if($prob) {
        print "Goto problem: ";
        my $p;
        foreach $p (1 .. $prob) {
            print "<a href=\"#prob$p\">$p</a>\n";
            if($p == 5) {
                last;
            }
        }
        if($prob > 5 ) {
            print "... <a href=\"#prob$prob\">last</a>\n";
        }

        print "<p>\n";
    }

    print @o;

    print "</div></body></html>\n";

}
