#!/usr/bin/perl

sub buildlangs {
    my ($outputlang)=@_;
    my $dir = "../apps/lang";
    opendir(DIR, $dir);
    my @files = grep { /\.lang$/ } readdir(DIR);
    closedir(DIR);

    for(@files) {
        my $output = $_;
        $output =~ s/(.*)\.lang/$1.lng/;
        print "lang $_\n" if($verbose);
        system ("../tools/binlang $dir/english.lang $dir/$_ $outputlang/$output >/dev/null 2>&1");
    }
}

sub buildzip {
    my ($zip, $image, $notplayer)=@_;

    # remove old traces
    `rm -rf .rockbox`;

    mkdir ".rockbox", 0777;
    mkdir ".rockbox/langs", 0777;
    mkdir ".rockbox/rocks", 0777;
    `find . -name "*.rock" ! -empty | xargs --replace=foo cp foo .rockbox/rocks/`;

    if($notplayer) {
        `cp ../apps/plugins/sokoban.levels .rockbox/`; # sokoban levels

        mkdir ".rockbox/fonts", 0777;

        opendir(DIR, "../fonts") || die "can't open dir fonts";
        my @fonts = grep { /\.bdf$/ && -f "../fonts/$_" } readdir(DIR);
        closedir DIR;

        for(@fonts) {
            my $f = $_;

            print "FONT: $f\n" if($verbose);
            my $o = $f;
            $o =~ s/\.bdf/\.fnt/;
            my $cmd ="../tools/convbdf -s 32 -l 255 -f -o \".rockbox/fonts/$o\" \"../fonts/$f\" >/dev/null 2>&1";
            print "CMD: $cmd\n" if($verbose);
            `$cmd`;
        }

        `cp rockbox.ucl .rockbox/`;  # UCL for flashing
    }

    mkdir ".rockbox/docs", 0777;
    for(("BATTERY-FAQ",
         "CUSTOM_CFG_FORMAT",
         "CUSTOM_WPS_FORMAT",
         "FAQ",
         "NODO",
         "TECH")) {
        `cp ../docs/$_ .rockbox/docs/$_.txt`;
    }

    # now copy the file made for reading on the unit:
    #if($notplayer) {
    #    `cp $webroot/docs/Help-JBR.txt .rockbox/docs/`;
    #}
    #else {
    #    `cp $webroot/docs/Help-Stu.txt .rockbox/docs/`;
    #}

    buildlangs(".rockbox/langs");

    `find .rockbox | zip $zip -@ >/dev/null`;

    `zip $zip $image`;

    # remove the .rockbox afterwards
    `rm -rf .rockbox`;

    print "Created $zip\n";
}

my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) =
 localtime(time);

$mon+=1;
$year+=1900;

$date=sprintf("%04d%02d%02d", $year,$mon, $mday);
$shortdate=sprintf("%02d%02d%02d", $year%100,$mon, $mday);

my $verbose;
if($ARGV[0] eq "-v") {
    $verbose =1;
    shift @ARGV;
}

# made once for all targets
sub runone {
    my ($type, $target)=@_;

    # build a full install zip file 
    buildzip("rockbox.zip", $target,
             ($type eq "player")?0:1);
};

if($ARGV[0] !~ /player/i) {
    runone("recorder", "ajbrec.ajz");
}
else {
    runone("player", "archos.mod");
}

