#!/usr/bin/perl

require "rockbox.pm";

my $basedir = "/home/dast/rockbox-build/daily-build";

my @list=("player",
          "recorder", "recorder8mb",
          "fmrecorder", "fmrecorder8mb",
          "recorderv2",
          "ondiofm", "ondiosp",
          "h100", "h120", "h300", "ipodcolor", "ipodnano",

          # install and source are special cases
          "install", "source");

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

for(reverse sort keys %date) {
    my $d = $_;
    my $nice = $d;
    if($d =~ /(\d\d\d\d)(\d\d)(\d\d)/) {
        $nice = "$1-$2-$3";
    }
    print "<table class=rockbox cellpadding=\"0\"><tr valign=top>\n";

    $color1 -= 0x18;
    $color2 -= 0x18;
    $color3 -= 0x18;
    
    my $count = 0;
    my $split = int((scalar @list) / 2);
    my $x = 0;
    my @head;

    foreach $t (@list) {
        my $show = $t;
        $show =~ s/recorder/rec/;
        # Remove the comment below to get long names
        # $show = $longname{$t};
        $head[$x] .= "<th>$show</th>\n";
	$count++;
	if ($count == $split) {
	    $x++;
	}
    }
    print "$head[0]</tr><tr>\n";

    $count = 0;
    for(@list) {
        my $m = $_;
        printf "<td><img alt=\"$m\" src=\"$model{$m}\"><br>";
        # new-style full zip:
        my $file = "rockbox-${m}-${d}.zip";
        if($m eq "source") {
            $file = "rockbox-daily-${d}.tar.gz";
        }
        elsif($m eq "install") {
            $file = "Rockbox-${d}-install.exe";
        }
        if( -f "$basedir/$m/$file") {
            printf "<a href=\"/daily/$_/$file\">latest</a>",
        }
        print "<p><a href=\"/dl.cgi?bin=$_\">older</a></td>\n";

	$count++;
	if ($count == $split) {
	    print "</tr><tr>$head[1]</tr><tr>\n";
	}
    }
    print "</tr>\n";
    last;
}
print "</table>\n";

