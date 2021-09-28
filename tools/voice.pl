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
#
# All files in this archive are subject to the GNU General Public License.
# See the file COPYING in the source tree root for full license agreement.
#
# This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
# KIND, either express or implied.

use strict;
use warnings;
use File::Basename;
use File::Copy;
use vars qw($V $C $t $l $e $E $s $S $i $v $f);
use IPC::Open2;
use IPC::Open3;
use Digest::MD5 qw(md5_hex);
use DirHandle;
use open ':encoding(utf8)';
use open ':std';
use utf8;

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

 -f=<file> Use existing voiceids file

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
    'english' => 'en-gb',  # Always first, it's the golden master
	'czech' => 'cs',   # not supported
	'dansk' => 'da',
	'deutsch' => 'de',
        'english-us' => 'en-us',
	'espanol' => 'es-es',
	'francais' => 'fr-fr',
        'greek' => 'el',
        'magyar' => 'hu',
        'italiano' => 'it',
        'nederlands' => 'nl',
        'norsk' => 'no',
	'polski' => 'pl',
	'russian' => 'ru',
	'slovak' => 'sk',
        'srpski' => 'sr',
        'svenska' => 'sv',
        'turkce' => 'tr',
);

my %espeak_lang_map = (
    'english' => 'en-gb',  # Always first, it's the golden master
	'czech' => 'cs',
	'dansk' => 'da',
	'deutsch' => 'de',
        'english-us' => 'en-us',
	'espanol' => 'es',
	'francais' => 'fr-fr',
	'greek' => 'el',
        'nederlands' => 'nl',
        'magyar' => 'hu',
        'italiano' => 'it',
        'japanese' => 'ja',
        'nederlands' => 'nl',
        'norsk' => 'no',
        'polski' => 'pl',
        'russian' => 'ru',
        'slovak' => 'sk',
        'srpski' => 'sr',
        'svenska' => 'sv',
        'turkce' => 'tr',
);

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
        my $pid = open(FESTIVAL_SERVER, "| festival $tts_engine_opts --server > /dev/null 2>&1");
        my $dummy = *FESTIVAL_SERVER; #suppress warning
        $SIG{INT} = sub { kill TERM => $pid; print("foo"); panic_cleanup(); };
        $SIG{KILL} = sub { kill TERM => $pid; print("boo"); panic_cleanup(); };
        $ret{"pid"} = $pid;
        if (defined($festival_lang_map{$language}) && $tts_engine_opts !~ /--language/) {
            $ret{"ttsoptions"} = "--language $festival_lang_map{$language} ";
        }
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
            $ret{"ttsoptions"} = "-l $gtts_lang_map{$language} ";
        }
    } elsif ($tts_engine eq 'espeak' || $tts_engine eq 'espeak-ng') {
        if (defined($espeak_lang_map{$language}) && $tts_engine_opts !~ /-v/) {
            $ret{"ttsoptions"} = "-v$espeak_lang_map{$language} ";
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
    my ($language, $target, $encoder, $encoder_opts, $tts_engine, $tts_engine_opts, $existingids) = @_;
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

    my $tts_object = init_tts($tts_engine, $tts_engine_opts, $language);
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
                my $enc = $id . '.mp3';

                # Print some progress information
                if (++$i % 10 == 0 and !$verbose) {
                    print(".");
                }

                # Apply corrections to the string
                $voice = correct_string($voice, $language, $tts_object);

                # If we have a pool of snippets, see if the string exists there first
                if (defined($ENV{'POOL'})) {
                    $pool_file = sprintf("%s/%s-%s.mp3", $ENV{'POOL'},
                                         md5_hex(Encode::encode_utf8("$voice $tts_engine $tts_engine_opts $encoder_opts")),
                                         $language);
                    if (-f $pool_file) {
                        printf("Re-using %s (%s) from pool\n", $id, $voice) if $verbose;
                        copy($pool_file, $enc);
                    }
                }

                # Don't generate encoded file if it already exists (probably from the POOL)
                if (! -f $enc) {
                    if ($id eq "VOICE_PAUSE") {
                        print("Use distributed $wav\n") if $verbose;
                        copy(dirname($0)."/VOICE_PAUSE.wav", $wav);
                    } else {
			voicestring($voice, $wav, $tts_engine_opts, $tts_object);
			if ($tts_object->{'format'} eq "wav") {
			    wavtrim($wav, 500, $tts_object);
			    # 500 seems to be a reasonable default for now
			}
                    }
		    if ($tts_object->{'format'} eq "wav" || $id eq "VOICE_PAUSE") {
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
                $voice = "";
                $id = "";
            }
        }
    }
    close(VOICEFONTIDS);

    print("\n");

    unlink($updfile) if (-f $updfile);
    shutdown_tts($tts_object);
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
    for (glob('*.mp3')) {
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
    my ($dir, $tts_object, $encoder, $encoder_opts, $tts_engine_opts, $i) = @_;
    my $d = new DirHandle $dir;
    while (my $file = $d->read) {
        my ($voice, $wav, $enc);
        # Print some progress information
        if (++$i % 10 == 0 and !$verbose) {
            print(".");
        }

        # Convert to a complete path
        my $path = sprintf("%s/%s", $dir, $file);

        $voice = $file;
        $wav = sprintf("%s.talk.wav", $path);

        # Ignore dot-dirs and talk files
        if ($file eq '.' || $file eq '..' || $file =~ /\.talk$/) {
            next;
        }
        # Element is a dir
        if ( -d $path) {
            gentalkclips($path, $tts_object, $encoder, $encoder_opts, $tts_engine_opts, $i);
            $enc = sprintf("%s/_dirname.talk", $path);
        }
        # Element is a file
        else {
            $enc = sprintf("%s.talk", $path);
            $voice =~ s/\.[^\.]*$//; # Trim extension
        }

        printf("Talkclip %s: %s", $enc, $voice) if $verbose;

        voicestring($voice, $wav, $tts_engine_opts, $tts_object);
        wavtrim($wav, 500, $tts_object);
        # 500 seems to be a reasonable default for now
        encodewav($wav, $enc, $encoder, $encoder_opts, $tts_object);
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

# Do what we're told
if ($V == 1) {
    # Only do the panic cleanup for voicefiles
    $SIG{INT} = \&panic_cleanup;
    $SIG{KILL} = \&panic_cleanup;

    printf("Generating voice\n  Target: %s\n  Language: %s\n  Encoder (options): %s (%s)\n  TTS Engine (options): %s (%s)\n",
           defined($t) ? $t : "unknown",
           $l, $e, $E, $s, $S);
    generateclips($l, $t, $e, $E, $s, $S, $f);
    createvoice($l, $i, $f);
    deleteencs();
}
elsif ($C) {
    printf("Generating .talk clips\n  Path: %s\n  Language: %s\n  Encoder (options): %s (%s)\n  TTS Engine (options): %s (%s)\n", $ARGV[0], $l, $e, $E, $s, $S);
    my $tts_object = init_tts($s, $S, $l);
    gentalkclips($ARGV[0], $tts_object, $e, $E, $S, 0);
    shutdown_tts($tts_object);
}
else {
    printusage();
    exit 1;
}
