#!/usr/bin/perl -s
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#
# Copyright (C) 2007 Jonas Häggqvist
# Copyright (C) 2020 Solomon Peachy
#
# All files in this archive are subject to the GNU General Public License.
# See the file COPYING in the source tree root for full license agreement.
#
# This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
# KIND, either express or implied.

use strict;
use warnings;
use utf8;
use File::Basename;
use File::Copy;
use vars qw($V $C $t $l $e $E $s $S $i $v $f $F);
use IPC::Open2;
use IPC::Open3;
use Digest::MD5 qw(md5_hex);
use DirHandle;
use open ':encoding(utf8)';
use Encode::Locale;
use Encode;
use Unicode::Normalize;

sub printusage {
    print <<USAGE

Usage: voice.pl [options] [path to dir]
 -V
    Create voice file. You must also specify -l, -i, and -t or -f

 -C
    Create .talk clips.

 -t=<target>
    Specify which target you want to build voicefile for. Must include
    any features that target supports.

 -f=<file>
    Use existing voiceids file

 -i=<target_id>
    Numeric target id. Needed for voice building.

 -l=<language>
    Specify which language you want to build. Without .lang extension.

 -e=<encoder>
    Which encoder to use for voice strings

 -E=<encoder options>
    Which encoder options to use when compressing voice strings. Enclose
    in double quotes if the options include spaces.

 -s=<TTS engine>
    Which TTS engine to use.

 -S=<TTS engine options>
    Options to pass to the TTS engine. Enclose in double quotes if the
    options include spaces.

 -F
    Force the file to be regenerated even if present

 -v
    Be verbose
USAGE
;
}

my %festival_lang_map = (
    'english' => 'english',
    'english-us' => 'english',
    'espanol' => 'spanish',
    #'finnish' => 'finnish'
    #'italiano' => 'italian',
    #'czech' => 'czech',
    #'welsh' => 'welsh'
);

my %gtts_lang_map = (
    'english' => '-l en -t co.uk',  # Always first, it's the golden master
    'bulgarian' => '-l bg',
    'chinese-simp' => '-l zh',
    'czech' => '-l cs',
    'dansk' => '-l da',
    'deutsch' => '-l de',
    'eesti' => '-l et',
    'english-us' => '-l en -t us',
    'espanol' => '-l es',
    'francais' => '-l fr',
    'greek' => '-l el',
    'italiano' => '-l it',
    'korean' => '-l ko',
    'magyar' => '-l hu',
    'nederlands' => '-l nl',
    'norsk' => '-l no',
    'polski' => '-l pl',
    'russian' => '-l ru',
    'slovak' => '-l sk',
    'srpski' => '-l sr',
    'svenska' => '-l sv',
    'turkce' => '-l tr',
);

my %espeak_lang_map = (
    'english' => '-ven-gb -k 5',  # Always first, it's the golden master
    'bulgarian' => '-vbg',
    'chinese-simp' => '-vzh',
    'czech' => '-vcs',
    'dansk' => '-vda',
    'deutsch' => '-vde',
    'eesti' => '-vet',
    'english-us' => '-ven-us -k 5',
    'espanol' => '-ves',
    'francais' => '-vfr-fr',
    'greek' => '-vel',
    'italiano' => '-vit',
    'japanese' => '-vja',
    'korean' => '-vko',
    'magyar' => '-vhu',
    'nederlands' => '-vnl',
    'norsk' => '-vno',
    'polski' => '-vpl',
    'russian' => '-vru',
    'slovak' => '-vsk',
    'srpski' => '-vsr',
    'svenska' => '-vsv',
    'turkce' => '-vtr',
    );

my %piper_lang_map = (
    'english' => 'en_GB-semaine-medium.onnx',  # Always first, it's the golden master
#    'bulgarian' => '-vbg',
    'chinese-simp' => 'zh_CN-huayan-medium.onnx',
    'czech' => 'cs_CZ-jirka-medium.onnx',
    'dansk' => 'da_DK-talesyntese-medium.onnx',
    'deutsch' => 'de_DE-thorsten-high.onnx',
#    'eesti' => '-vet',
    'english-us' => 'en_US-lessac-high.onnx',
    'espanol' => 'es_ES-sharvard-medium.onnx',
    'francais' => 'fr_FR-siwis-medium.onnx',
    'greek' => 'el_GR-rapunzelina-low.onnx',
    'italiano' => 'it_IT-paola-medium.onnx',
#    'japanese' => '-vja',
#    'korean' => '-vko',
    'magyar' => 'hu_HU-anna-medium.onnx',
    'nederlands' => 'nl_NL-mls-medium.onnx',
    'norsk' => 'no_NO-talesyntese-medium.onnx',
    'polski' => 'pl_PL-gosia-medium.onnx',
    'russian' => 'ru_RU-irina-medium.onnx',
    'slovak' => 'sk_SK-lili-medium.onnx',
    'srpski' => 'sr_RS-serbski_institut-medium.onnx',
    'svenska' => 'sv_SE-nst-medium.onnx',
    'turkce' => 'tr_TR-fettah-medium.onnx',
);

my $trim_thresh = 250;   # Trim silence if over this, in ms
my $force = 0;           # Don't regenerate files already present

# Initialize TTS engine. May return an object or value which will be passed
# to voicestring and shutdown_tts
sub init_tts {
    our $verbose;
    my ($tts_engine, $tts_engine_opts, $language) = @_;
    my %ret = ("name" => $tts_engine);
    $ret{"format"} = 'wav';
    $ret{"ttsoptions"} = "";

    # Don't use given/when here - it's not compatible with old perl versions
    if ($tts_engine eq 'festival') {
        print("> festival $tts_engine_opts --server\n") if $verbose;
        # Open command, and filehandles for STDIN, STDOUT, STDERR
        my $pid = open(FESTIVAL_SERVER, "| festival $tts_engine_opts --server > /dev/null 2>&1");
        my $dummy = *FESTIVAL_SERVER; #suppress warning
        $SIG{INT} = sub { kill TERM => $pid; print("foo"); panic_cleanup(); };
        $SIG{KILL} = sub { kill TERM => $pid; print("boo"); panic_cleanup(); };
        $ret{"pid"} = $pid;
        if (defined($festival_lang_map{$language}) && $tts_engine_opts !~ /--language/) {
            $ret{"ttsoptions"} = "--language $festival_lang_map{$language} ";
        }
    } elsif ($tts_engine eq 'piper') {
	if (defined($piper_lang_map{$language}) && $tts_engine_opts !~ /--model/) {
	    die("Need PIPER_MODEL_DIR\n") if (!defined($ENV{'PIPER_MODEL_DIR'}));
       	    $tts_engine_opts = "--model $ENV{PIPER_MODEL_DIR}/$piper_lang_map{$language} ";
	}
	my $cmd = "piper $tts_engine_opts --json-input";
        print("> $cmd\n") if $verbose;

        my $pid = open3(*CMD_IN, *CMD_OUT, *CMD_ERR, $cmd);
        $SIG{INT} = sub { kill TERM => $pid; print("foo"); panic_cleanup(); };
        $SIG{KILL} = sub { kill TERM => $pid; print("boo"); panic_cleanup(); };
        $ret{"pid"} = $pid;
        binmode(*CMD_IN, ':encoding(utf8)');
        binmode(*CMD_OUT, ':encoding(utf8)');
        binmode(*CMD_ERR, ':encoding(utf8)');

    } elsif ($tts_engine eq 'sapi') {
        my $toolsdir = dirname($0);
        my $path = `cygpath $toolsdir -a -w`;
        chomp($path);
        $path = $path . '\\';
        my $cmd = $path . "sapi_voice.vbs /language:$language $tts_engine_opts";
        $cmd =~ s/\\/\\\\/g;
        print("> cscript //nologo $cmd\n") if $verbose;
        my $pid = open2(*CMD_OUT, *CMD_IN, "cscript //nologo $cmd");
        binmode(*CMD_IN, ':encoding(utf16le)');
        binmode(*CMD_OUT, ':encoding(utf16le)');
        $SIG{INT} = sub { print(CMD_IN "QUIT\r\n"); panic_cleanup(); };
        $SIG{KILL} = sub { print(CMD_IN "QUIT\r\n"); panic_cleanup(); };
        print(CMD_IN "QUERY\tVENDOR\r\n");
        my $vendor = readline(*CMD_OUT);
        $vendor =~ s/\r\n//;
        %ret = (%ret,
                "stdin" => *CMD_IN,
                "stdout" => *CMD_OUT,
                "vendor" => $vendor);
    } elsif ($tts_engine eq 'gtts') {
        $ret{"format"} = 'mp3';
        if (defined($gtts_lang_map{$language}) && $tts_engine_opts !~ /-l/) {
            $ret{"ttsoptions"} = " $gtts_lang_map{$language} ";
        }
    } elsif ($tts_engine eq 'espeak' || $tts_engine eq 'espeak-ng') {
        if (defined($espeak_lang_map{$language}) && $tts_engine_opts !~ /-v/) {
            $ret{"ttsoptions"} = " $espeak_lang_map{$language} ";
        }
    }

    return \%ret;
}

# Shutdown TTS engine if necessary.
sub shutdown_tts {
    my ($tts_object) = @_;
    if ($$tts_object{'name'} eq 'festival') {
        # Send SIGTERM to festival server
        kill TERM => $$tts_object{"pid"};
    }
    elsif ($$tts_object{'name'} eq 'piper') {
        # Send SIGTERM to piper
        kill TERM => $$tts_object{"pid"};
    }
    elsif ($$tts_object{'name'} eq 'sapi') {
        print({$$tts_object{"stdin"}} "QUIT\r\n");
        close($$tts_object{"stdin"});
    }
}

# Apply corrections to a voice-string to make it sound better
sub correct_string {
    our $verbose;
    my ($string, $language, $tts_object) = @_;
    my $orig = $string;
    my $corrections = $tts_object->{"corrections"};

    foreach (@$corrections) {
        my $r = "s" . $_->{separator} . $_->{search} . $_->{separator}
                . $_->{replace} . $_->{separator} . $_->{modifier};
        eval ('$string =~' . "$r;");
    }
    if ($orig ne $string) {
        printf("%s -> %s\n", $orig, $string) if $verbose;
    }
    return $string;
}

# Produce a wav file of the text given
sub voicestring {
    our $verbose;
    my ($string, $output, $tts_engine_opts, $tts_object) = @_;
    my $cmd;
    my $name = $$tts_object{'name'};

    $tts_engine_opts .= $$tts_object{"ttsoptions"};

    # Normalize Unicode
    $string = NFC($string);

    printf("Generate \"%s\" with %s in file %s\n", $string, $name, $output) if $verbose;
    if ($name eq 'festival') {
        # festival_client lies to us, so we have to do awful soul-eating
        # work with IPC::open3()
        $cmd = "festival_client --server localhost --otype riff --ttw --output \"$output\"";
        # Use festival-prolog.scm if it's there (created by user of tools/configure)
        if (-f "festival-prolog.scm") {
            $cmd .= " --prolog festival-prolog.scm";
        }
        print("> $cmd\n") if $verbose;
        # Open command, and filehandles for STDIN, STDOUT, STDERR
        my $pid = open3(*CMD_IN, *CMD_OUT, *CMD_ERR, $cmd);
        # Put the string to speak into STDIN and close it
        print(CMD_IN $string);
        close(CMD_IN);
        # Read all output from festival_client (because it LIES TO US)
        while (<CMD_ERR>) {
        }
        close(CMD_OUT);
        close(CMD_ERR);
    }
    elsif ($name eq 'piper') {
	$cmd = "{ \"text\": \"$string\", \"output_file\": \"$output\" }";
        print(">> $cmd\n") if $verbose;
	print(CMD_IN "$cmd\n");
	my $res = <CMD_OUT>;
	$res = <CMD_ERR>;
    }
    elsif ($name eq 'flite') {
        $cmd = "flite $tts_engine_opts -t \"$string\" \"$output\"";
        print("> $cmd\n") if $verbose;
        system($cmd);
    }
    elsif ($name eq 'espeak') {
        $cmd = "espeak $tts_engine_opts -w \"$output\" --stdin";
        print("> $cmd\n") if $verbose;
        open(RBSPEAK, "| $cmd");
        print RBSPEAK $string . "\n";
        close(RBSPEAK);
    }
    elsif ($name eq 'espeak-ng') {
        $cmd = "espeak-ng $tts_engine_opts -w \"$output\" --stdin";
        print("> $cmd\n") if $verbose;
        open(RBSPEAK, "| $cmd");
        print RBSPEAK $string . "\n";
        close(RBSPEAK);
    }
    elsif ($name eq 'sapi') {
        print({$$tts_object{"stdin"}} "SPEAK\t$output\t$string\r\n");
    }
    elsif ($name eq 'swift') {
        $cmd = "swift $tts_engine_opts -o \"$output\" \"$string\"";
        print("> $cmd\n") if $verbose;
        system($cmd);
    }
    elsif ($name eq 'rbspeak') {
        # xxx: $tts_engine_opts isn't used
        $cmd = "rbspeak $output";
        print("> $cmd\n") if $verbose;
        open(RBSPEAK, "| $cmd");
        print RBSPEAK $string . "\n";
        close(RBSPEAK);
    }
    elsif ($name eq 'mimic') {
        $cmd = "mimic $tts_engine_opts -o $output";
        print("> $cmd\n") if $verbose;
        open(RBSPEAK, "| $cmd");
        print RBSPEAK $string . "\n";
        close(RBSPEAK);
    }
    elsif ($name eq 'gtts') {
        $cmd = "gtts-cli $tts_engine_opts -o $output -";
        print("> $cmd\n") if $verbose;
        open(RBSPEAK, "| $cmd");
        print RBSPEAK $string . "\n";
        close(RBSPEAK);
    }
}

# trim leading / trailing silence from the clip
sub wavtrim {
    our $verbose;
    my ($file, $threshold, $tts_object) = @_;
    printf("Trim \"%s\"\n", $file) if $verbose;
    my $cmd = "wavtrim \"$file\" $threshold";
    if ($$tts_object{"name"} eq "sapi") {
        print({$$tts_object{"stdin"}} "EXEC\t$cmd\r\n");
    }
    else {
        print("> $cmd\n") if $verbose;
        `$cmd`;
    }
}

# Encode a wav file into the given destination file
sub encodewav {
    our $verbose;
    my ($input, $output, $encoder, $encoder_opts, $tts_object) = @_;
    printf("Encode \"%s\" with %s in file %s\n", $input, $encoder, $output) if $verbose;
    my $cmd = "$encoder $encoder_opts \"$input\" \"$output\"";
    if ($$tts_object{"name"} eq "sapi") {
        print({$$tts_object{"stdin"}} "EXEC\t$cmd\r\n");
    }
    else {
        print("> $cmd\n") if $verbose;
        `$cmd`;
    }
}

# synchronize the clip generation / processing if it's running in another process
sub synchronize {
    my ($tts_object) = @_;
    if ($$tts_object{"name"} eq "sapi") {
        print({$$tts_object{"stdin"}} "SYNC\t42\r\n");
        my $wait = readline($$tts_object{"stdout"});
        #ignore what's actually returned
    }
}

# Run genlang and create voice clips for each string
sub generateclips {
    our $verbose;
    my ($language, $target, $encoder, $encoder_opts, $tts_object, $tts_engine_opts, $existingids) = @_;
    my $english = dirname($0) . '/../apps/lang/english.lang';
    my $langfile = dirname($0) . '/../apps/lang/' . $language . '.lang';
    my $correctionsfile = dirname($0) . '/voice-corrections.txt';
    my $idfile = "$language.vid";
    my $updfile = "$language-update.lang";
    my $id = '';
    my $voice = '';
    my $cmd;
    my $pool_file;
    my $i = 0;
    local $| = 1; # make progress indicator work reliably

    # First run the language through an update pass so any missing strings
    # are backfilled from English.  Without this, BADNESS.
    if ($existingids) {
        $idfile = $existingids;
    } else {
	$cmd = "updatelang $english $langfile $updfile";
	print("> $cmd\n") if $verbose;
        system($cmd);
	$cmd = "genlang -o -t=$target -e=$english $updfile 2>/dev/null > $idfile";
	print("> $cmd\n") if $verbose;
	system($cmd);
    }
    open(VOICEFONTIDS, " < $idfile");

    # add string corrections to tts_object.
    my @corrects = ();
    open(VOICEREGEXP, "<$correctionsfile") or die "Can't open corrections file!\n";
    while(<VOICEREGEXP>) {
        # get first character of line
        my $line = $_;
        my $separator = substr($_, 0, 1);
        if($separator =~ m/\s+/) {
            next;
        }
        chomp($line);
        $line =~ s/^.//g; # remove separator at beginning
        my ($lang, $engine, $vendor, $search, $replace, $modifier) = split(/$separator/, $line);

        # does language match?
        if($language !~ m/$lang/) {
            next;
        }
        if($$tts_object{"name"} !~ m/$engine/) {
            next;
        }
        my $v = $$tts_object{"vendor"} || ""; # vendor might be empty in $tts_object
        if($v !~ m/$vendor/) {
            next;
        }
        push @corrects, {separator => $separator, search => $search, replace => $replace, modifier => $modifier};

    }
    close(VOICEREGEXP);
    $tts_object->{corrections} = [@corrects];

    print("Generating voice clips");
    print("\n") if $verbose;
    for (<VOICEFONTIDS>) {
        my $line = $_;
        if ($line =~ /^id: (.*)$/) {
            $id = $1;
        }
        elsif ($line =~ /^voice: "(.*)"$/) {
            $voice = $1;
            if ($id !~ /^NOT_USED_.*$/ && $voice ne "") {
                my $wav = $id . '.wav';
                my $enc = $id . '.enc';
		my $format = $tts_object->{'format'};

                # Print some progress information
                if (++$i % 10 == 0 and !$verbose) {
                    print(".");
                }

                # Apply corrections to the string
                $voice = correct_string($voice, $language, $tts_object);

                # If we have a pool of snippets, see if the string exists there first
                if (defined($ENV{'POOL'})) {
                    $pool_file = sprintf("%s/%s-%s.enc", $ENV{'POOL'},
                                         md5_hex(Encode::encode_utf8("$voice ". $tts_object->{"name"}." $tts_engine_opts ".$tts_object->{"ttsoptions"}." $encoder_opts")),
                                         $language);
                    if (-f $pool_file) {
                        printf("Re-using %s (%s) from pool\n", $id, $voice) if $verbose;
#                        system("touch $pool_file"); # So we know it's still being used.
                        copy($pool_file, $enc);
                    }
                }

                # Don't generate encoded file if it already exists (probably from the POOL)
                if (! -f $enc && !$force) {
                    if ($id eq "VOICE_PAUSE") {
                        print("Use distributed $wav\n") if $verbose;
                        copy(dirname($0)."/VOICE_PAUSE.wav", $wav);
                    } else {
			voicestring($voice, $wav, $tts_engine_opts, $tts_object);
			if ($format eq "wav") {
			    wavtrim($wav, $trim_thresh, $tts_object);
			}
                    }
		    # Convert from mp3 to wav so we can use rbspeex
		    if ($format eq "mp3") {
			system("ffmpeg -loglevel 0 -i $wav $id$wav");
			rename("$id$wav","$wav");
			$format = "wav";
		    }
		    if ($format eq "wav" || $id eq "VOICE_PAUSE") {
			encodewav($wav, $enc, $encoder, $encoder_opts, $tts_object);
		    } else {
			copy($wav, $enc);
		    }

                    synchronize($tts_object);
                    if (defined($ENV{'POOL'})) {
			copy($enc, $pool_file);
                    }
                    unlink($wav);
                }

		# Special cases
		if ($id eq "VOICE_INVALID_VOICE_FILE") {
		    copy ($enc, "InvalidVoice_$language.talk");
		}
		if ($id eq "VOICE_LANG_NAME") {
		    copy ($enc, "$language.lng.talk");
	        }

                $voice = "";
                $id = "";
            }
        }
    }
    close(VOICEFONTIDS);

    print("\n");

    unlink($updfile) if (-f $updfile);
}

# Assemble the voicefile
sub createvoice {
    our $verbose;
    my ($language, $target_id, $existingids) = @_;
    my $outfile = "";
    my $vfile = "$language.vid";

    if ($existingids) {
        $vfile = $existingids;
    }
    $outfile = sprintf("%s.voice", $language);
    printf("Saving voice file to %s\n", $outfile) if $verbose;
    my $cmd = "voicefont '$vfile' $target_id ./ $outfile";
    print("> $cmd\n") if $verbose;
    my $output = `$cmd`;
    print($output) if $verbose;
    if (!$existingids) {
        unlink("$vfile");
    }
}

sub deleteencs() {
    for (glob('*.enc')) {
        unlink($_);
    }
    for (glob('*.wav')) {
        unlink($_);
    }
}

sub panic_cleanup {
    deletencs();
    die "moo";
}

# Generate .talk clips
sub gentalkclips {
    our $verbose;
    my ($dir, $tts_object, $language, $encoder, $encoder_opts, $tts_engine_opts, $i) = @_;
    my $d = new DirHandle $dir;
    while (my $file = $d->read) {
	$file = Encode::decode( locale_fs => $file);
        my ($voice, $wav, $enc);
	my $format = $tts_object->{'format'};

        # Print some progress information
        if (++$i % 10 == 0 and !$verbose) {
            print(".");
        }

        # Ignore dot-dirs and talk files
        if ($file eq '.' || $file eq '..' || $file =~ /\.talk$/) {
            next;
        }

        $voice = $file;

        # Convert some symbols to spaces
        $voice =~ tr/_-/  /;

        # Convert to a complete path
        my $path = sprintf("%s/%s", $dir, $file);

        $wav = sprintf("%s.talk.wav", $path);

        if ( -d $path) { # Element is a dir
	    $enc = sprintf("%s/_dirname.talk", $path);
            if (! -e "$path/talkclips.ignore") { # Skip directories containing "talkclips.ignore"
                gentalkclips($path, $tts_object, $encoder, $encoder_opts, $tts_engine_opts, $i);
            }
        } else { # Element is a file
            $enc = sprintf("%s.talk", $path);
            $voice =~ s/\.[^\.]*$//; # Trim extension
        }

	# Apply corrections
	$voice = correct_string($voice, $language, $tts_object);

        printf("Talkclip %s: %s", $enc, $voice) if $verbose;
	# Don't generate encoded file if it already exists
	next if (-f $enc && !$force);

	voicestring($voice, $wav, $tts_engine_opts, $tts_object);
	wavtrim($wav, $trim_thresh, $tts_object);

	if ($format eq "mp3") {
	    system("ffmpeg -loglevel 0 -i $wav $voice$wav");
	    rename("$voice$wav","$wav");
	    $format = "wav";
	}
	if ($format eq "wav") {
	    encodewav($wav, $enc, $encoder, $encoder_opts, $tts_object);
	} else {
	    copy($wav, $enc);
	}
	synchronize($tts_object);
	unlink($wav);
    }
}


# Check parameters
my $printusage = 0;

unless (defined($V) or defined($C)) { print("Missing either -V or -C\n"); $printusage = 1; }
if (defined($V)) {
    unless (defined($l)) { print("Missing -l argument\n"); $printusage = 1; }
    unless (defined($i)) { print("Missing -i argument\n"); $printusage = 1; }
    if (defined($t) && defined($f) ||
        !defined($t) && !defined($f)) {
	     print("Missing either -t or -f argument\n"); $printusage = 1;
        }
}
elsif (defined($C)) {
    unless (defined($ARGV[0])) { print "Missing path argument\n"; $printusage = 1; }
}

$force = 1 if (defined($F));

unless (defined($e)) { print("Missing -e argument\n"); $printusage = 1; }
unless (defined($E)) { print("Missing -E argument\n"); $printusage = 1; }
unless (defined($s)) { print("Missing -s argument\n"); $printusage = 1; }
unless (defined($S)) { print("Missing -S argument\n"); $printusage = 1; }
if ($printusage == 1) { printusage(); exit 1; }

if (defined($v) or defined($ENV{'V'})) {
    our $verbose = 1;
}

# add the tools dir to the path temporarily, for calling various tools
$ENV{'PATH'} = dirname($0) . ':' . $ENV{'PATH'};

my $tts_object = init_tts($s, $S, $l);

# Do what we're told
if ($V == 1) {
    # Only do the panic cleanup for voicefiles
    $SIG{INT} = \&panic_cleanup;
    $SIG{KILL} = \&panic_cleanup;

    printf("Generating voice\n  Target: %s\n  Language: %s\n  Encoder (options): %s (%s)\n  TTS Engine (options): %s (%s)\n  Pool directory: %s\n",
           defined($t) ? $t : "unknown",
           $l, $e, $E, $s, "$S $tts_object->{ttsoptions}", defined($ENV{'POOL'}) ? $ENV{'POOL'} : "<none>");
    generateclips($l, $t, $e, $E, $tts_object, $S, $f);
    shutdown_tts($tts_object);
    createvoice($l, $i, $f);
    deleteencs();
} elsif ($C) {
    printf("Generating .talk clips\n  Path: %s\n  Language: %s\n  Encoder (options): %s (%s)\n  TTS Engine (options): %s (%s)\n", $ARGV[0], $l, $e, $E, $s, $S);
    gentalkclips($ARGV[0], $tts_object, $l, $e, $E, $S, 0);
    shutdown_tts($tts_object);
} else {
    printusage();
    exit 1;
}
