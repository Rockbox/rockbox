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

print "<table class=rockbox><tr><th>player-old</th><th>player</th><th>recorder</th></tr>\n";
print "<tr><td>This version is for old Archos Jukebox 5000, 6000 models with ROM firmware older than 4.50:\n";
&list("playerold");

print "</td><td>This version is for Archos Jukebox 5000/6000 with ROM firmware 4.50 or later, and all Studio models:\n";
&list("player");

print "</td><td>This version is for all Archos Jukebox Recorder models:\n";
&list("recorder");

print "</td></tr></table>\n";
