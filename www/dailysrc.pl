#!/usr/bin/perl

$basedir = "/home/dast/rockbox-build/daily-build/";

opendir(DIR, $basedir) or
    die "Can't opendir($basedir)";
@tarballs = sort grep { /^rockbox-daily-/ } readdir(DIR);
closedir DIR;

for ( sort {$b cmp $a} @tarballs ) {
    $size = (stat("$basedir/$_"))[7];
    $log = "";
    if (/-(\d+)/) {
        $date = $1;
        if ( -f "$basedir/changes-$date.html") {
            $log = "<a href=\"daily/changes-$date.html\">Changes done $date</a>";
        }
    }
    print "$log\n";
    last;
}
