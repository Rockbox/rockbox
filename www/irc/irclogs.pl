#!/usr/bin/perl

require "/home/dast/perl/date.pm";

opendir(DIR, ".") or
    die "Can't opendir()";
@logs = sort grep { /^rockbox-/ } readdir(DIR);
closedir DIR;

print "<table><tr valign=top>\n";

$lasty = 0;
$lastm = 0;

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
                print "</ul></td>\n" if $lastm != 0;
#                if ( $m % 6 == 0 ) {
#                    print "</tr><tr valign=top>\n";
#                }
                print "<td align=\"right\"><b>$mname $y</b>\n";
                print "<ul>\n";
                $lastm = $m;
            }
#            $lines = `wc -l $file` + 0;
#            print "<li><a test href=\"$file\">$mname $d</a> <small>($lines lines)</small>\n";
            print "<li><a test href=\"$file\">$mname $d</a>\n";
        }
    }
    #print "<li><a href=\"daily/$_\">$_</a> ($size bytes) $log\n";
}
print "</ul></td></tr></table>\n";
