#!/usr/bin/perl

$version="3.0";

my $verbose;
if($ARGV[0] eq "-v") {
    $verbose =1;
    shift @ARGV;
}

my $doonly;
if($ARGV[0]) {
    $doonly = $ARGV[0];
    print "only build $doonly\n" if($verbose);
}

# made once for all targets
sub runone {
    my ($dir, $conf, $nl)=@_;
    my $a;

    if($doonly && ($doonly ne $dir)) {
        return;
    }

    mkdir "buildm-$dir";
    chdir "buildm-$dir";
    print "Build in buildm-$dir\n" if($verbose);

    # build the manual(s)
    $a = buildit($dir, $conf, $nl);

    chdir "..";

    my $o="buildm-$dir/manual/rockbox-build.pdf";
    if (-f $o) {
        my $newo="output/rockbox-$dir-$version.pdf";
        system("mv $o $newo");
        print "moved $o to $newo\n" if($verbose);
    }

    $o="buildm-$dir/rockbox-manual.zip";
    if (-f $o) {
        my $newo="output/rockbox-$dir-$version-html.zip";
        system("mv $o $newo");
        print "moved $o to $newo\n" if($verbose);
    }

    print "remove all contents in buildm-$dir\n" if($verbose);
    system("rm -rf buildm-$dir");

    return $a;
};

sub buildit {
    my ($target, $confnum, $newl)=@_;

    `rm -rf * >/dev/null 2>&1`;

    my $c = sprintf('echo -e "%s\n%sm\n" | ../tools/configure',
                    $confnum, $newl?'\n':'');

    print "C: $c\n" if($verbose);
    `$c`;

    print "Run 'make'\n" if($verbose);
    `make manual 2>/dev/null`;

    print "Run 'make manual-zip'\n" if($verbose);
    `make manual-zip 2>/dev/null`;
}

# run make in tools first to make sure they're up-to-date
`(cd tools && make ) >/dev/null 2>&1`;

runone("player", "player", 1);
runone("recorder", "recorder", 1);
runone("fmrecorder", "fmrecorder", 1);
runone("recorderv2", "recorderv2", 1);
runone("ondiosp", "ondiosp", 1);
runone("ondiofm", "ondiofm", 1);
runone("h100", "h100");
#runone("h120", 9);
runone("h300", "h300");
runone("ipodcolor", "ipodcolor");
runone("ipodnano", "ipodnano");
runone("ipod4gray", "ipod4g");
runone("ipodvideo", "ipodvideo", 1);
runone("ipod3g", "ipod3g");
runone("ipod1g2g", "ipod1g2g");
runone("iaudiox5", "x5");
runone("iaudiom5", "m5");
runone("ipodmini2g", "ipodmini2g");
runone("h10", "h10");
runone("h10_5gb", "h10_5gb");
runone("gigabeatf", "gigabeatf");
runone("sansae200", "e200");
runone("sansac200", "c200");
runone("mrobe100", "mrobe100");
