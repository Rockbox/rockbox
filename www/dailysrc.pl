#!/usr/bin/perl

$basedir = "/home/dast/rockbox-build/daily-build/";

opendir(DIR, $basedir) or
    die "Can't opendir($basedir)";
@tarballs = grep { /^rockbox-daily-/ } readdir(DIR);
closedir DIR;

print "<ul>\n";

for ( @tarballs ) {
    print "<li><a href=\"daily/$_\">$_</a>\n";
}

print "</ul>\n";
