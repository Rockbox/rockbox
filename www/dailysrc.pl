#!/usr/bin/perl

$basedir = "/home/dast/rockbox-build/daily-build/";

opendir(DIR, $basedir) or
    die "Can't opendir($basedir)";
@tarballs = sort grep { /^rockbox-daily-/ } readdir(DIR);
closedir DIR;

print "<ul>\n";

for ( @tarballs ) {
    $size = (stat("$basedir/$_"))[7];
    $log = "";
    if (/-(\d+)/) {
        $date = $1;
        if ( -f "$basedir/changes-$date.log") {
            $log = "<a href=\"daily/changes-$date.log\">Changelog</a>";
        }
    }
    print "<li><a href=\"daily/$_\">$_</a> ($size bytes) $log\n";
}

print "</ul>\n";
