#!/usr/bin/perl

$basedir = "/home/dast/rockbox-build/daily-build";

sub list {
    $dir = shift @_;

    opendir(DIR, "$basedir/$dir") or
        die "Can't opendir($basedir/$dir)";
    @tarballs = grep { /^archos/ } readdir(DIR);
    closedir DIR;
    
    print "<ul>\n";
    for ( @tarballs ) {
        print "<li><a href=\"daily/$dir/$_\">$_</a>\n";
    }
    print "</ul>\n";
}

print "<h3>player-old</h3>\n";
print "<p>This version is for old Archos Jukebox 6000 models with ROM firmware older than 4.50.\n";
&list("playerold");

print "<h3>player</h3>\n";
print "<p>This version is for Archos Jukebox 6000 with ROM firmware 4.50 or later, and all Studio models.\n";
&list("player");

print "<h3>recorder</h3>\n";
print "<p>This version is for all Archos Jukebox Recorder models.\n";
&list("recorder");
