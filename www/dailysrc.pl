#!/usr/bin/perl

$basedir = "/home/dast/rockbox-build/daily-build/";

opendir(DIR, $basedir) or
    die "Can't opendir($basedir)";
@tarballs = sort grep { /^rockbox-daily-/ } readdir(DIR);
closedir DIR;

print "<ul>\n";

for ( sort {$b <=> $a} @tarballs ) {
    $size = (stat("$basedir/$_"))[7];
    $log = "";
    if (/-(\d+)/) {
        $date = $1;
        if ( -f "$basedir/changes-$date.txt") {
            $lines = `grep "Number of changes:" $basedir/changes-$date.txt | cut "-d " -f4` + 0;
            $log = "<a href=\"daily/changes-$date.html\">Changelog</a> <small>($lines changes)</small>";
        }
    }
    print "<li><a href=\"daily/$_\">$_</a> <small>($size bytes)</small> $log\n";
    print "<li><a href=\"dl.cgi?bin=source\">old versions</a>\n";
    last;
}

print "</ul>\n";
