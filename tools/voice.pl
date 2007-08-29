#!/usr/bin/perl -s
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id: 
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
use IPC::Open3;
use Digest::MD5 qw(md5_hex);

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
    my $ret = undef;
    switch($tts_engine) {
        case "festival" {
            print("> festival $tts_engine_opts --server\n") if $verbose;
            my $pid = open(FESTIVAL_SERVER, "| festival $tts_engine_opts --server > /dev/null 2>&1");
            $ret = *FESTIVAL_SERVER;
            $ret = $pid;
            $SIG{INT} = sub { kill TERM => $pid; print("foo"); panic_cleanup(); };
            $SIG{KILL} = sub { kill TERM => $pid; print("boo"); panic_cleanup(); };
        }
        case "sapi5" {
            my $toolsdir = dirname($0);
            my $path = `cygpath $toolsdir -a -w`;
            chomp($path);
            $path = $path . "\\sapi5_voice_new.vbs $language $tts_engine_opts";
            $path =~ s/\\/\\\\/g;
            print("> cscript /B $path\n") if $verbose;
            my $pid = open(F, "| cscript /B $path");
            $ret = *F;
            $SIG{INT} = sub { print($ret "\r\n\r\n"); panic_cleanup(); };
            $SIG{KILL} = sub { print($ret "\r\n\r\n"); panic_cleanup(); };
        }
    }
    return $ret;
}

# Shutdown TTS engine if necessary.
sub shutdown_tts {
    my ($tts_engine, $tts_object) = @_;
    switch($tts_engine) {
        case "festival" {
            # Send SIGTERM to festival server
            kill TERM => $tts_object;
        }
        case "sapi5" {
            print($tts_object "\r\n\r\n");
            close($tts_object);
        }
    }
}

# Apply corrections to a voice-string to make it sound better
sub correct_string {
    our $verbose;
    my ($string, $language, $tts_engine) = @_;
    my $orig = $string;
    switch($language) {
        # General for all engines and languages (perhaps - just an example)
        $string =~ s/USB/U S B/;

        case ("deutsch") {
            switch($tts_engine) {
                $string =~ s/alphabet/alfabet/;
                $string =~ s/alkaline/alkalein/;
                $string =~ s/ampere/amper/;
                $string =~ s/byte(s?)\b/beit$1/;
                $string =~ s/\bdezibel\b/de-zibell/;
                $string =~ s/energie\b/ener-gie/;
                $string =~ s/\bflash\b/fläsh/g;
                $string =~ s/\bfirmware(s?)\b/firmwer$1/;
                $string =~ s/\bid3 tag\b/id3 täg/g; # can't just use "tag" here
                $string =~ s/\bloudness\b/laudness/;
                $string =~ s/\bnumerisch\b/numehrisch/;
                $string =~ s/\brücklauf\b/rück-lauf/;
                $string =~ s/\bsuchlauf\b/such-lauf/;
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
    my ($string, $output, $tts_engine, $tts_engine_opts, $tts_object) = @_;
    my $cmd;
    printf("Generate \"%s\" with %s in file %s\n", $string, $tts_engine, $output) if $verbose;
    switch($tts_engine) {
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
            # xxx: $tts_engine_opts isn't used
            $cmd = "espeak $tts_engine_opts -w $output";
            print("> $cmd\n") if $verbose;
            open(ESPEAK, "| $cmd");
            print ESPEAK $string . "\n";
            close(ESPEAK);
        }
        case "sapi5" {
            print($tts_object sprintf("%s\r\n%s\r\n", $string, $output));
        }
    }
}

# Encode a wav file into the given destination file
sub encodewav {
    our $verbose;
    my ($input, $output, $encoder, $encoder_opts) = @_;
    my $cmd = '';
    printf("Encode \"%s\" with %s in file %s\n", $input, $encoder, $output) if $verbose;
    switch ($encoder) {
        case 'lame' {
            $cmd = "lame $encoder_opts \"$input\" \"$output\"";
        }
        case 'vorbis' {
            $cmd = "oggenc $encoder_opts \"$input\" -o \"$output\"";
        }
        case 'speexenc' {
            $cmd = "speexenc $encoder_opts \"$input\" \"$output\"";
        }
    }
    print("> $cmd\n") if $verbose;
    `$cmd`;
}

sub wavtrim {
    our $verbose;
    my ($file) = @_;
    my $cmd = dirname($0) . "/wavtrim \"$file\"";
    print("> $cmd\n") if $verbose;
    `$cmd`;
}

# Run genlang and create voice clips for each string
sub generateclips {
    our $verbose;
    my ($language, $target, $encoder, $encoder_opts, $tts_engine, $tts_engine_opts) = @_;
    my $genlang = dirname($0) . '/genlang';
    my $english = dirname($0) . '/../apps/lang/english.lang';
    my $langfile = dirname($0) . '/../apps/lang/' . $language . '.lang';
    my $id = '';
    my $voice = '';
    my $cmd = "$genlang -o -t=$target -e=$english $langfile 2>/dev/null";
    my $pool_file;
    open(VOICEFONTIDS, "> voicefontids");
    my $i = 0;

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
                $voice = correct_string($voice);

                # If we have a pool of snippes, see if the string exists there first
                if (defined($ENV{'POOL'})) {
                    $pool_file = sprintf("%s/%s-%s-%s.mp3", $ENV{'POOL'}, md5_hex($voice), $language, $tts_engine);
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
                        voicestring($voice, $wav, $tts_engine, $tts_engine_opts, $tts_object);
                        wavtrim($wav, 500); # 500 seems to be a reasonable default for now
                    }

                    encodewav($wav, $mp3, $encoder, $encoder_opts);
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
    shutdown_tts($tts_engine, $tts_object);
}

# Assemble the voicefile
sub createvoice {
    our $verbose;
    my ($language, $target_id) = @_;
    my $voicefont = dirname($0) . '/voicefont';
    my $outfile = "";
    my $i = 0;
    do {
        $outfile = sprintf("%s%s.voice", $language, ($i++ == 0 ? '' : '-'.$i));
    } while (-f $outfile);
    printf("Saving voice file to %s\n", $outfile) if $verbose;
    my $cmd = "$voicefont 'voicefontids' $target_id ./ $outfile";
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

$SIG{INT} = \&panic_cleanup;
$SIG{KILL} = \&panic_cleanup;

if (defined($v) or defined($ENV{'V'})) {
    our $verbose = 1;
}


# Do what we're told
if ($V == 1) {
    printf("Generating voice\n  Target: %s\n  Language: %s\n  Encoder (options): %s (%s)\n  TTS Engine (options): %s (%s)\n",
        $t, $l, $e, $E, $s, $S);
    generateclips($l, $t, $e, $E, $s, $S);
    createvoice($l, $i);
    deletemp3s();
}
elsif ($C) {
    # xxx: Implement .talk clip generation
}
else {
    printusage();
    exit 1;
}
