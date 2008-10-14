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
    my ($dir, $select, $newl)=@_;
    my $a;

    if($doonly && ($doonly ne $dir)) {
        return;
    }

    mkdir "build-$dir";
    chdir "build-$dir";
    print "Build in build-$dir\n" if($verbose);

    # build the manual(s)
    $a = buildit($dir, $select, $newl);

    chdir "..";

    my $o="build-$dir/english.voice";
    if (-f $o) {
        my $newo="output/$dir-$version-english.zip";
        system("cp $o output/$dir-$version-english.voice");
        system("mkdir -p .rockbox/langs");
        system("cp $o .rockbox/langs");
        system("zip -r $newo .rockbox");
        system("rm -rf .rockbox");
        print "moved $o to $newo\n" if($verbose);
    }

    print "remove all contents in build-$dir\n" if($verbose);
    system("rm -rf build-$dir");

    return $a;
};

sub buildit {
    my ($dir, $select, $newl)=@_;

    `rm -rf * >/dev/null 2>&1`;

    # V (voice), F (festival), L (lame), [blank] (English)
    my $c = sprintf('echo -e "%s\n%sa\nv\n\n\nf\n\n" | ../tools/configure',
                    $select, $newl?'\n':"");

    print "C: $c\n" if($verbose);
    `$c`;

    print "Run 'make voice'\n" if($verbose);
    print `make voice 2>/dev/null`;
}

# run make in tools first to make sure they're up-to-date
`(cd tools && make ) >/dev/null 2>&1`;

`rm -f /home/dast/tmp/rockbox-voices-$version/voice-pool/*`;
$ENV{'POOL'}="/home/dast/tmp/rockbox-voices-$version/voice-pool";

runone("player", "player", 1);
runone("recorder", "recorder", 1);
runone("fmrecorder", "fmrecorder", 1);
runone("recorderv2", "recorderv2", 1);
runone("ondiosp", "ondiosp", 1);
runone("ondiofm", "ondiofm", 1);
runone("h100", "h100");
runone("h120", "h120");
runone("h300", "h300");
runone("ipodcolor", "ipodcolor");
runone("ipodnano", "ipodnano");
runone("ipod4gray", "ipod4g");
runone("ipodvideo", "ipodvideo", 1);
runone("ipod3g", "ipod3g");
runone("ipod1g2g", "ipod1g2g");
runone("iaudiox5", "x5");
runone("iaudiom5", "m5");
runone("iaudiom3", "m3");
runone("ipodmini2g", "ipodmini2g");
runone("ipodmini1g", "ipodmini");
runone("h10", "h10");
runone("h10_5gb", "h10_5gb");
runone("gigabeatf", "gigabeatf");
runone("sansae200", "e200");
runone("sansac200", "c200");
runone("mrobe100", "mrobe100");
#runone("cowond2", "cowond2");

