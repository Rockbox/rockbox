#!/usr/bin/perl

$basedir = "/home/dast/rockbox-build/daily-build";

sub list {
    $dir = shift @_;

    opendir(DIR, "$basedir/$dir") or
        die "Can't opendir($basedir/$dir)";
    @tarballs = sort grep { /^a/ } readdir(DIR);
    closedir DIR;
    
    print "<ul>\n";
    for ( @tarballs ) {
        print "<li><a href=\"daily/$dir/$_\">$_</a>\n";
    }
    print "</ul>\n";
}

print "<table class=rockbox><tr><th>player</th><th>recorder</th></tr>\n";
print "<tr><td>\n";
&list("player");

print "</td><td>\n";
&list("recorder");

print "</td></tr></table>\n";
