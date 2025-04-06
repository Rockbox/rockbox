#!/usr/bin/perl -w
require "./tools/builds.pm";

my $verbose;
if($ARGV[0] eq "-v") {
    $verbose =1;
    shift @ARGV;
}

my $tag = $ARGV[0];
my $version = $ARGV[1];

my $outdir = "output/voices";

# made once for all targets
sub runone {
    my ($target, $name, $lang, $engine, $voice, $engine_opts)=@_;
    my $a;

    print "*** LANGUAGE: $lang\n";

    print "Build in buildv-$target-$lang\n" if($verbose);

    mkdir "buildv-$target-$lang";
    chdir "buildv-$target-$lang";

    # build the voice(s)
    $a = buildit($target, $lang, $engine, $voice, $engine_opts);

    my $o="$lang.voice";
    if (-f $o) {
        my $newo="../$outdir/$target/voice-$target-$version-$name.zip";
	system("mkdir -p ../$outdir/$target");
        system("mkdir -p .rockbox/langs");
        system("mkdir -p output/$target");
        system("mkdir -p .rockbox/langs");
        system("cp $o .rockbox/langs");
        system("cp $lang.lng.talk .rockbox/langs");
        system("cp InvalidVoice_$lang.talk .rockbox/langs");
        system("zip -q -r $newo .rockbox");
        system("rm -rf .rockbox");
        `chmod a+r $newo`;
        print "moved $o to $newo\n" if($verbose);
    }

    chdir "..";

    print "remove all contents in buildv-$target-$lang\n" if($verbose);
    system("rm -rf buildv-$target-$lang");

    return $a;
};

sub buildit {
    my ($target, $lang, $engine, $voice, $engine_opts)=@_;

    `rm -rf * >/dev/null 2>&1`;

    my $c = "../tools/configure --no-ccache --type=av --target=$target --ram=-1 --language=$lang --tts=$engine --voice=$voice --ttsopts='$engine_opts'";

    print "C: $c\n" if($verbose);
    system($c);

    print "Run 'make voice'\n" if($verbose);
    `make voice`;
}

`git checkout $tag`;

# run make in tools first to make sure they're up-to-date
`(cd tools && make ) >/dev/null 2>&1`;

if (!defined($ENV{'POOL'})) {
    my $home=$ENV{'HOME'};
    my $pool="$home/tmp/rockbox-voices-$version/voice-pool";
    `mkdir -p $pool`;
    $ENV{'POOL'}="$pool";
}
# `rm -f $pool/*`;

`mkdir -p $outdir`;

for my $b (&stablebuilds) {
    next if ($builds{$b}{voice}); # no variants

    for my $v (&allvoices) {
	my %voice = %{$voices{$v}};

	my $engine = $voice{"defengine"};
	my ($opts, $vf);
	if ($engine eq 'piper') {
	    $vf = $voice{"engines"}->{$engine};
	    $opts = "";
	} else {
	    $vf = -1;
	    $opts = $voice{"engines"}->{$engine};
	}
	#            print " runone $b $v ($voice{lang} via $engine)\n";
	runone($b, $v, $voice{"lang"}, $engine, $vf, $opts);
    }
}
