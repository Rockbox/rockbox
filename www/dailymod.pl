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

print "<table border=1><tr valign=\"top\"><td>\n";
print "<h3>player-old</h3>\n";
print "<p>This version is for old Archos Jukebox 6000 models with ROM firmware older than 4.50:\n";
&list("playerold");

print "</td><td>\n";
print "<h3>player</h3>\n";
print "<p>This version is for Archos Jukebox 5000/6000 with ROM firmware 4.50 or later, and all Studio models:\n";
&list("player");

print "</td><td>\n";
print "<h3>recorder</h3>\n";
print "<p>This version is for all Archos Jukebox Recorder models:\n";
&list("recorder");

print "</td></tr></table>\n";
