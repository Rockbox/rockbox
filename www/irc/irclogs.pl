#!/usr/bin/perl

require "/home/dast/perl/date.pm";

opendir(DIR, ".") or
    die "Can't opendir()";
@logs = sort grep { /^rockbox-/ } readdir(DIR);
closedir DIR;

print "<table class=archive>\n";

$lasty = 0;
$lastm = 0;
$count = 0;

for ( @logs ) {
    $size = (stat("$_"))[7];
    $file = $_;
    $log = "";
    if (/-(\d+)/) {
        if ( $1 =~ /(\d\d\d\d)(\d\d)(\d\d)/ ) {
            $y = $1;
            $m = $2;
            $d = $3;
            $mname = ucfirst MonthNameEng($m);
            if ( $m != $lastm ) {
                $count=0;
                print "</tr><tr>\n" if $lastm != 0;
#                if ( $m % 6 == 0 ) {
#                    print "</tr><tr valign=top>\n";
#                }
                print "<th>$mname</th>\n";
                $lastm = $m;
            }
#            $lines = `wc -l $file` + 0;
#            print "<li><a test href=\"$file\">$mname $d</a> <small>($lines lines)</small>\n";
            print "<td><a test href=\"$file\">$d</a></td>\n";
            if ( ++$count > 15 ) {
                print "</tr><tr><th></th>\n";
                $count=0;
            }
        }
    }
    #print "<li><a href=\"daily/$_\">$_</a> ($size bytes) $log\n";
}
print "</ul></td></tr></table>\n";
