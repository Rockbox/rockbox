#!/usr/bin/perl

require "rockbox.pm";

my $basedir = "/home/dast/rockbox-build/daily-build";

my @list=("player", "recorder", "fmrecorder", "recorderv2", "recorder8mb", "fmrecorder8mb");

for(@list) {
    my $dir = $_;
    opendir(DIR, "$basedir/$dir") or next;
    my @files = sort grep { /^rockbox/ } readdir(DIR);
    closedir DIR;

    for(@files) {
        /(20\d+)/;
        $date{$1}=$1;
    }
}

$color1 = 0xc6;
$color2 = 0xd6;
$color3 = 0xf5;
$font1 = "<b>";
$font2 = "</b>";

for(reverse sort keys %date) {
    my $d = $_;
    my $nice = $d;
    if($d =~ /(\d\d\d\d)(\d\d)(\d\d)/) {
        $nice = "$1-$2-$3";
    }
    $col = sprintf("style=\"background-color: #%02x%02x%02x\"",
                   $color1, $color2, $color3);
    print "<h2>Download daily build</h2>\n";
    print "<table class=rockbox><tr valign=top>\n";

    $color1 -= 0x18;
    $color2 -= 0x18;
    $color3 -= 0x18;
    
    for(@list) {
        my $n=0;
        my $m = $_;
        printf "<td $col>$font1$m$font2<br><img src=\"$model{$m}\"><br>";
        # new-style full zip:
        if( -f "daily/$m/rockbox-${m}-${d}.zip") {
            printf "%s<a href=\"daily/$_/rockbox-${m}-${d}.zip\">latest</a>",
            $n?", ":"";
            $n++;
        }
        print "$font2 <p><a href=\"dl.cgi?bin=$_\">old versions</a></td>\n";
    }
    printf "<td $col>${font1}windows installer$font2<br><img src=\"$model{install}\"><br>";
    print "<a href=\"daily/Rockbox-${d}-install.exe\">latest</a>",
    "<p><a href=\"dl.cgi?bin=install\">old versions</a></td>";
    print "</tr>\n";
    $font1 = $font2 = "";
    last;
}
print "</table>\n";

