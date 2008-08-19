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
use Switch;
use vars qw($V $C $t $l $e $E $s $S $i $v);
use IPC::Open2;
use IPC::Open3;
use Digest::MD5 qw(md5_hex);
use DirHandle;
use open ':encoding(utf8)';
use open ':std';

sub printusage {
    print <<USAGE

Usage: voice.pl [options] [path to dir]
 -V
    Create voice file. You must also specify -t and -l.
 
 -C
    Create .talk clips.

 -t=<target>
    Specify which target you want to build voicefile for. Must include
    any features that target supports.
 
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

# Initialize TTS engine. May return an object or value which will be passed
# to voicestring and shutdown_tts
sub init_tts {
    our $verbose;
    my ($tts_engine, $tts_engine_opts, $language) = @_;
    my %ret = ("name" => $tts_engine);
    switch($tts_engine) {
        case "festival" {
            print("> festival $tts_engine_opts --server\n") if $verbose;
            my $pid = open(FESTIVAL_SERVER, "| festival $tts_engine_opts --server > /dev/null 2>&1");
            my $dummy = *FESTIVAL_SERVER; #suppress warning
            $SIG{INT} = sub { kill TERM => $pid; print("foo"); panic_cleanup(); };
            $SIG{KILL} = sub { kill TERM => $pid; print("boo"); panic_cleanup(); };
            $ret{"pid"} = $pid;
        }
        case "sapi" {
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
        }
    }
    return \%ret;
}

# Shutdown TTS engine if necessary.
sub shutdown_tts {
    my ($tts_object) = @_;
    switch($$tts_object{"name"}) {
        case "festival" {
            # Send SIGTERM to festival server
            kill TERM => $$tts_object{"pid"};
        }
        case "sapi" {
            print({$$tts_object{"stdin"}} "QUIT\r\n");
            close($$tts_object{"stdin"});
        }
    }
}

# Apply corrections to a voice-string to make it sound better
sub correct_string {
    our $verbose;
    my ($string, $language, $tts_object) = @_;
    my $orig = $string;
    switch($language) {
        # General for all engines and languages
        $string =~ s/USB/U S B/g;
        $string =~ s/ID3/I D 3/g;

        case "english" {
            switch($$tts_object{"name"}) {
                case ["sapi","festival"] {
                    $string =~ s/plugin(s?)/plug-in$1/ig; next
                }
                case "festival" {
                    $string =~ s/\ba\b/ay/ig;
                }
            }
        }
        case "deutsch" {
            # for all german engines (e.g. for english words)
            $string =~ s/alkaline/alkalein/ig;
            $string =~ s/byte(s?)/beit$1/ig;
            $string =~ s/clip(s?)/klipp$1/ig;
            $string =~ s/cuesheet/kjuschiet/ig;
            $string =~ s/dither/didder/ig;
            $string =~ s/equalizer/iquileiser/ig;
            $string =~ s/\bflash\b/fläsh/ig;
            $string =~ s/\bfirmware(s?)\b/firmwer$1/ig;
            $string =~ s/\bI D 3 tag\b/I D 3 täg/ig; # can't just use "tag" here
            $string =~ s/\bloudness\b/laudness/ig;
            $string =~ s/\bunicode\b/unikod/ig;
            switch($$tts_object{"name"}) {
                 case "sapi" {   # just for SAPI
                    switch($$tts_object{"vendor"}) {
                        case "AT&T Labs" {
                            $string =~ s/alphabet/alfabet/ig;
                            $string =~ s/ampere/amper/ig;
                            $string =~ s/\bdezibel\b/de-zibell/ig;
                            $string =~ s/diddering/didde-ring/ig;
                            $string =~ s/energie\b/ener-gie/ig;
                            $string =~ s/\Blauf\b/-lauf/ig;
                            $string =~ s/\bnumerisch\b/numehrisch/ig;
                        }
                    }
                }
            }
        }
        case "svenska" {
            # for all swedish engines (e.g. for english words)
            $string =~ s/kilobyte/kilobajt/ig;
            $string =~ s/megabyte/megabajt/ig;
            $string =~ s/gigabyte/gigabajt/ig;
            $string =~ s/\bloudness\b/laudness/ig;

            switch($$tts_object{"name"}) {
                 case "espeak" {   # just for eSpeak
                     $string =~ s/ampere/ampär/ig;
                     $string =~ s/bokmärken/bok-märken/ig;
                     $string =~ s/generella/schenerella/ig;
                     $string =~ s/dithering/diddering/ig;
                     $string =~ s/\bunicode\b/jynikod/ig;
                     $string =~ s/uttoning/utoning/ig;
                     $string =~ s/procent/pro-cent/ig;
                     $string =~ s/spellistor/spelistor/ig;
                     $string =~ s/cuesheet/qjyschiit/ig;
                 }
            }
        }
        case "italiano" {
            # for all italian engines (e.g. for english words)
            $string =~ s/Replaygain/Ripleyghein/ig;
            $string =~ s/Crossfade/Crossfeid/ig;
            $string =~ s/beep/Bip/ig;
            $string =~ s/cuesheet/chiushit/ig;
            $string =~ s/fade/feid/ig;
            $string =~ s/Crossfeed/crossfid/ig;
            $string =~ s/Cache/chash/ig;
            $string =~ s/\bfirmware(s?)\b/firmuer$1/ig;
            $string =~ s/\bFile(s?)\b/fail$1/ig;
            $string =~ s/\bloudness\b/laudness/ig;
            $string =~ s/\bunicode\b/unikod/ig;
            $string =~ s/Playlist/pleylist/ig;
            $string =~ s/WavPack/wave pak/ig;
            $string =~ s/BITRATE/bit reit/ig;
            $string =~ s/Codepage/cod page/ig;
            $string =~ s/PCM Wave/pcm Ue'iv/ig;
            switch($$tts_object{"name"}) {
                 case "sapi" {   # just for SAPI
                    switch($$tts_object{"vendor"}) {
                        case "Loquendo" {
                            $string =~ s/Inizializza/inizializa/ig;
                        }
                        case "ScanSoft, Inc" {
                            $string =~ s/V/v/ig;
                            $string =~ s/X/x/ig;
                            $string =~ s/stop/stohp/ig;
                        }
                    }
                }
            }
        }
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
    printf("Generate \"%s\" with %s in file %s\n", $string, $$tts_object{"name"}, $output) if $verbose;
    switch($$tts_object{"name"}) {
        case "festival" {
            # festival_client lies to us, so we have to do awful soul-eating
            # work with IPC::open3()
            $cmd = "festival_client --server localhost --otype riff --ttw --output \"$output\"";
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
        case "flite" {
            $cmd = "flite $tts_engine_opts -t \"$string\" \"$output\"";
            print("> $cmd\n") if $verbose;
            `$cmd`;
        }
        case "espeak" {
            $cmd = "espeak $tts_engine_opts -w \"$output\"";
            print("> $cmd\n") if $verbose;
            open(ESPEAK, "| $cmd");
            print ESPEAK $string . "\n";
            close(ESPEAK);
        }
        case "sapi" {
            print({$$tts_object{"stdin"}} "SPEAK\t$output\t$string\r\n");
        }
        case "swift" {
            $cmd = "swift $tts_engine_opts -o \"$output\" \"$string\"";
            print("> $cmd\n") if $verbose;
            system($cmd);
        }
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
    my ($language, $target, $encoder, $encoder_opts, $tts_engine, $tts_engine_opts) = @_;
    my $english = dirname($0) . '/../apps/lang/english.lang';
    my $langfile = dirname($0) . '/../apps/lang/' . $language . '.lang';
    my $id = '';
    my $voice = '';
    my $cmd = "genlang -o -t=$target -e=$english $langfile 2>/dev/null";
    my $pool_file;
    open(VOICEFONTIDS, "> voicefontids");
    my $i = 0;
    local $| = 1; # make progress indicator work reliably

    my $tts_object = init_tts($tts_engine, $tts_engine_opts, $language);
    print("Generating voice clips");
    print("\n") if $verbose;
    for (`$cmd`) {
        my $line = $_;
        print(VOICEFONTIDS $line);
        if ($line =~ /^id: (.*)$/) {
            $id = $1;
        }
        elsif ($line =~ /^voice: "(.*)"$/) {
            $voice = $1;
            if ($id !~ /^NOT_USED_.*$/ && $voice ne "") {
                my $wav = $id . '.wav';
                my $mp3 = $id . '.mp3';

                # Print some progress information
                if (++$i % 10 == 0 and !$verbose) {
                    print(".");
                }

                # Apply corrections to the string
                $voice = correct_string($voice, $language, $tts_object);

                # If we have a pool of snippets, see if the string exists there first
                if (defined($ENV{'POOL'})) {
                    $pool_file = sprintf("%s/%s-%s.mp3", $ENV{'POOL'}, 
                                         md5_hex("$voice $tts_engine $tts_engine_opts $encoder_opts"),
                                         $language);
                    if (-f $pool_file) {
                        printf("Re-using %s (%s) from pool\n", $id, $voice) if $verbose;
                        copy($pool_file, $mp3);
                    }
                }

                # Don't generate MP3 if it already exists (probably from the POOL)
                if (! -f $mp3) {
                    if ($id eq "VOICE_PAUSE") {
                        print("Use distributed $wav\n") if $verbose;
                        copy(dirname($0)."/VOICE_PAUSE.wav", $wav);
                    }
                    else {
                        voicestring($voice, $wav, $tts_engine_opts, $tts_object);
                        wavtrim($wav, 500, $tts_object);
                        # 500 seems to be a reasonable default for now
                    }

                    encodewav($wav, $mp3, $encoder, $encoder_opts, $tts_object);
                    synchronize($tts_object);
                    if (defined($ENV{'POOL'})) {
                        copy($mp3, $pool_file);
                    }
                    unlink($wav);
                }
                $voice = "";
                $id = "";
            }
        }
    }
    print("\n");
    close(VOICEFONTIDS);
    shutdown_tts($tts_object);
}

# Assemble the voicefile
sub createvoice {
    our $verbose;
    my ($language, $target_id) = @_;
    my $outfile = "";
    $outfile = sprintf("%s.voice", $language);
    printf("Saving voice file to %s\n", $outfile) if $verbose;
    my $cmd = "voicefont 'voicefontids' $target_id ./ $outfile";
    print("> $cmd\n") if $verbose;
    my $output = `$cmd`;
    print($output) if $verbose;
}

sub deletemp3s() {
    for (glob('*.mp3')) {
        unlink($_);
    }
    for (glob('*.wav')) {
        unlink($_);
    }
}

sub panic_cleanup {
    deletemp3s();
    die "moo";
}

# Generate .talk clips
sub gentalkclips {
    our $verbose;
    my ($dir, $tts_object, $encoder, $encoder_opts, $tts_engine_opts, $i) = @_;
    my $d = new DirHandle $dir;
    while (my $file = $d->read) {
        my ($voice, $wav, $mp3);
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
            $mp3 = sprintf("%s/_dirname.talk", $path);
        }
        # Element is a file
        else {
            $mp3 = sprintf("%s.talk", $path);
            $voice =~ s/\.[^\.]*$//; # Trim extension
        }

        printf("Talkclip %s: %s", $mp3, $voice) if $verbose;

        voicestring($voice, $wav, $tts_engine_opts, $tts_object);
        wavtrim($wav, 500, $tts_object);
        # 500 seems to be a reasonable default for now
        encodewav($wav, $mp3, $encoder, $encoder_opts, $tts_object);
        synchronize($tts_object);
        unlink($wav);
    }
}


# Check parameters
my $printusage = 0;
unless (defined($V) or defined($C)) { print("Missing either -V or -C\n"); $printusage = 1; }
if (defined($V)) {
    unless (defined($t)) { print("Missing -t argument\n"); $printusage = 1; }
    unless (defined($l)) { print("Missing -l argument\n"); $printusage = 1; }
    unless (defined($i)) { print("Missing -i argument\n"); $printusage = 1; }
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
        $t, $l, $e, $E, $s, $S);
    generateclips($l, $t, $e, $E, $s, $S);
    createvoice($l, $i);
    deletemp3s();
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
