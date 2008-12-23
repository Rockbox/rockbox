#!/usr/bin/perl

$version="3.1";

my $verbose;
if($ARGV[0] eq "-v") {
    $verbose =1;
    shift @ARGV;
}

my $update;
if($ARGV[0] eq "-u") {
    $update =1;
    shift @ARGV;
}

my $doonly;
if($ARGV[0]) {
    $doonly = $ARGV[0];
    print "only build $doonly\n" if($verbose);
}

if($update) {
    # svn update!
    system("svn -q up");
}

$rev = `svnversion`;
chomp $rev;
print "rev $rev\n" if($verbose);

# made once for all targets
sub runone {
    my ($dir, $confnum, $extra)=@_;
    my $a;

    if($doonly && ($doonly ne $dir)) {
        return;
    }

    mkdir "build-$dir";
    chdir "build-$dir";
    print "Build in build-$dir\n" if($verbose);

    # build the manual(s)
    $a = buildit($dir, $confnum, $extra);

    chdir "..";

    my $o="build-$dir/rockbox.zip";
    if (-f $o) {
        my $newo="output/rockbox-$dir-$version.zip";
        system("mkdir -p output");
        system("mv $o $newo");
        print "moved $o to $newo\n" if($verbose);
    }

    print "remove all contents in build-$dir\n" if($verbose);
    system("rm -rf build-$dir");

    return $a;
};

sub fonts {
    my ($dir, $confnum, $newl)=@_;
    my $a;

    if($doonly && ($doonly ne $dir)) {
        return;
    }

    mkdir "build-$dir";
    chdir "build-$dir";
    print "Build fonts in build-$dir\n" if($verbose);

    # build the manual(s)
    $a = buildfonts($dir, $confnum, $newl);

    chdir "..";

    my $o="build-$dir/rockbox-fonts.zip";
    if (-f $o) {
        my $newo="output/rockbox-fonts-$version.zip";
        system("mv $o $newo");
        print "moved $o to $newo\n" if($verbose);
    }

    print "remove all contents in build-$dir\n" if($verbose);
    system("rm -rf build-$dir");

    return $a;
};



sub buildit {
    my ($target, $confnum, $extra)=@_;

    `rm -rf * >/dev/null 2>&1`;

    my $c = sprintf('echo -e "%s\n%sn\n" | ../tools/configure',
                    $confnum, $extra);

    print "C: $c\n" if($verbose);
    `$c`;

    print "Run 'make'\n" if($verbose);
    `make -j 2>/dev/null`;

    print "Run 'make zip'\n" if($verbose);
    `make zip 2>/dev/null`;

    print "Run 'make mapzip'\n" if($verbose);
    `make mapzip 2>/dev/null`;
}

sub buildfonts {
    my ($target, $confnum, $newl)=@_;

    `rm -rf * >/dev/null 2>&1`;

    my $c = sprintf('echo -e "%s\n%sn\n" | ../tools/configure',
                    $confnum, $newl?'\n':'');

    print "C: $c\n" if($verbose);
    `$c`;

    print "Run 'make fontzip'\n" if($verbose);
    `make fontzip 2>/dev/null`;
}

# run make in tools first to make sure they're up-to-date
print "cd tools && make\n" if($verbose);
`(cd tools && make ) >/dev/null 2>&1`;

runone("player", "player", '\n');
runone("recorder", "recorder", '\n');
runone("recorder8mb", "recorder", '8\n');
runone("fmrecorder", "fmrecorder", '\n');
runone("fmrecorder8mb", "fmrecorder", '8\n');
runone("recorderv2", "recorderv2", '\n');
runone("ondiosp", "ondiosp", '\n');
runone("ondiofm", "ondiofm", '\n');
runone("h100", "h100");
runone("h120", "h120");
runone("h300", "h300");
runone("ipodcolor", "ipodcolor");
runone("ipodnano", "ipodnano");
runone("ipod4gray", "ipod4g");
runone("ipodvideo", "ipodvideo", '32\n');
runone("ipodvideo64mb", "ipodvideo", '64\n');
runone("ipod3g", "ipod3g");
runone("ipod1g2g", "ipod1g2g");
runone("iaudiox5", "x5");
runone("iaudiom5", "m5");
runone("iaudiom3", "m3");
runone("ipodmini1g", "ipodmini");
runone("ipodmini2g", "ipodmini2g");
runone("h10", "h10");
runone("h10_5gb", "h10_5gb");
runone("gigabeatf", "gigabeatf");
runone("sansae200", "e200");
runone("sansac200", "c200");
#runone("mrobe500", "mrobe500");
runone("mrobe100", "mrobe100");
runone("cowond2", "cowond2");
fonts("fonts", "x5");


