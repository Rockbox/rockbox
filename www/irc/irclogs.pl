#!/usr/bin/perl

require "/home/dast/perl/date.pm";

opendir(DIR, ".") or
    die "Can't opendir()";
@logs = grep { /^rockbox-/ } readdir(DIR);
closedir DIR;

print "<table class=archive>\n";

$lasty = 0;
$lastm = 0;
$count = 0;

for ( sort @logs ) {
    $size = (stat("$_"))[7];
    $file = $_;
    $log = "";

    if (/-(\d+)/) {
       if ( $1 =~ /(\d\d\d\d)(\d\d)(\d\d)/ ) {
            $y = $1;
            $m = $2;
            $d = $3;

            $mname = ucfirst MonthNameEng($m);
            if ($y != $lasty) {
                if ($lasty != 0) {
                    print "</tr><tr><th colspan=39><hr></th></tr><tr>\n";
                }
                print "<th>$y</th>\n";
                $lasty = $y;
            } else {
                print "</tr><tr>\n<th></th>" if ( $m != $lastm );
            }

            if ( $m != $lastm ) {
                $count=0;
                print "<th>$mname</th>\n";
                $lastm = $m;
            }

            print "<td><a test href=\"$file\">$d</a></td>\n";

            if ( ++$count > 15 ) {
                print "</tr><tr><th></th><th></th>\n";
                $count=0;
            }
        }
    }
}
print "</ul></td></tr></table>\n";
