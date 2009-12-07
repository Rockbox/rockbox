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
    my ($dir)=@_;
    my $a;

    if($doonly && ($doonly ne $dir)) {
        return;
    }

    mkdir "buildv-$dir";
    chdir "buildv-$dir";
    print "Build in buildv-$dir\n" if($verbose);

    # build the manual(s)
    $a = buildit($dir);

    chdir "..";

    my $o="buildv-$dir/english.voice";
    if (-f $o) {
        my $newo="output/$dir-$version-english.zip";
        system("cp $o output/$dir-$version-english.voice");
        system("mkdir -p .rockbox/langs");
        system("cp $o .rockbox/langs");
        system("zip -r $newo .rockbox");
        system("rm -rf .rockbox");
        print "moved $o to $newo\n" if($verbose);
    }

    print "remove all contents in buildv-$dir\n" if($verbose);
    system("rm -rf buildv-$dir");

    return $a;
};

sub buildit {
    my ($model)=@_;

    `rm -rf * >/dev/null 2>&1`;

    my $c = "../tools/configure --type=av --target=$model --language=0 --tts=f";

    print "C: $c\n" if($verbose);
    `$c`;

    print "Run 'make voice'\n" if($verbose);
    print `make voice 2>/dev/null`;
}

# run make in tools first to make sure they're up-to-date
`(cd tools && make ) >/dev/null 2>&1`;

my $home=$ENV{'HOME'};

my $pool="$home/tmp/rockbox-voices-$version/voice-pool";
`mkdir -p $pool`;
`rm -f $pool/*`;
$ENV{'POOL'}="$pool";

for my $b (&stablebuilds) {
    next if ($builds{$b}{configname} < 3); # no variants

    runone($b);
}
