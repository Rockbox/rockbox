#!/usr/bin/perl -w

package iap::decode;

use Device::iPod;
use Data::Dumper;
use strict;

sub new {
    my $class = shift;
    my $self = {-state => {}};

    return bless($self, $class);
}

sub display {
    my $self = shift;
    my $lingo = shift;
    my $command = shift;
    my $data = shift;
    my $name;
    my $handler;
    my $r;


    $name = sprintf("_h_%02x_%04x", $lingo, $command);
    $handler = $self->can($name);
    if ($handler) {
        unless(exists($self->{-state}->{$name})) {
            $self->{-state}->{$name} = {};
        }
        $r = $handler->($self, $data, $self->{-state}->{$name});
    } else {
        $r = $self->generic($lingo, $command, $data);
    }

    printf("\n");
    return $r;
}

sub generic {
    my $self = shift;
    my $lingo = shift;
    my $command = shift;
    my $data = shift;

    printf("Unknown command\n");
    printf(" Lingo:     0x%02x\n", $lingo);
    printf(" Command    0x%04x\n", $command);
    printf(" Data:      %s\n", Device::iPod->_hexstring($data));

    exit(1);

    return 1;
}

sub _h_00_0002 {
    my $self = shift;
    my $data = shift;
    my $state = shift;
    my $res;
    my $cmd;
    my $delay;

    ($res, $cmd, $delay) = unpack("CCN", $data);

    printf("ACK (0x00, 0x02) I->D\n");
    printf(" Acknowledged command: 0x%02x\n", $cmd);
    printf(" Status:               %s (%d)\n",
        ("Success",
         "ERROR: Unknown Database Category",
         "ERROR: Command Failed",
         "ERROR: Out Of Resource",
         "ERROR: Bad Parameter",
         "ERROR: Unknown ID",
         "Command Pending",
         "ERROR: Not Authenticated",
         "ERROR: Bad Authentication Version",
         "ERROR: Accessory Power Mode Request Failed",
         "ERROR: Certificate Invalid",
         "ERROR: Certificate permissions invalid",
         "ERROR: File is in use",
         "ERROR: Invalid file handle",
         "ERROR: Directory not empty",
         "ERROR: Operation timed out",
         "ERROR: Command unavailable in this iPod mode",
         "ERROR: Invalid accessory resistor ID",
         "Reserved",
         "Reserved",
         "Reserved",
         "ERROR: Maximum number of accessory connections already reached")[$res], $res);
    if ($res == 6) {
        # ($res, $cmd, $delay) = unpack("CCN", $data);
        printf(" Delay:                %d ms\n", $delay);
    }

    return 1;
}

sub _h_00_0005 {
    my $self = shift;
    my $data = shift;
    my $state = shift;

    printf("EnterRemoteUIMode (0x00, 0x05) D->I\n");

    return 1;
}

sub _h_00_000f {
    my $self = shift;
    my $data = shift;
    my $state = shift;
    my $lingo;

    $lingo = unpack("C", $data);

    printf("RequestLingoProtocolVersion (0x00, 0x0F) D->I\n");
    printf(" Lingo:               0x%02x\n", $lingo);

    return 1;
}

sub _h_00_0010 {
    my $self = shift;
    my $data = shift;
    my $state = shift;
    my ($lingo, $maj, $min);

    ($lingo, $maj, $min) = unpack("CCC", $data);

    printf("ReturnLingoProtocolVersion (0x00, 0x10) I->D\n");
    printf(" Lingo:               0x%02x\n", $lingo);
    printf(" Version:             %d.%02d\n", $maj, $min);

    return 1;
}


sub _h_00_0013 {
    my $self = shift;
    my $data = shift;
    my $state = shift;
    my @lingolist;
    my ($lingoes, $options, $devid);

    ($lingoes, $options, $devid) = unpack("N3", $data);

    foreach (0..31) {
        push(@lingolist, $_) if ($lingoes & (1 << $_));
    }

    printf("IdentifyDeviceLingoes (0x00, 0x13) D->I\n");
    printf(" Supported lingoes:    %s\n", join(", ", @lingolist));
    printf(" Options:\n");
    printf("  Authentication:      %s\n", ("None", "Defer (1.0)", "Immediate (2.0)", "Reserved")[$options & 0x03]);
    printf("  Power:               %s\n", ("Low", "Intermittent high", "Reserved", "Constant high")[($options & 0x0C) >> 2]);
    printf(" Device ID:            0x%08x\n", $devid);

    return 1;
}

sub _h_00_0014 {
    my $self = shift;
    my $data = shift;
    my $state = shift;

    printf("GetDevAuthenticationInfo (0x00, 0x14) I->D\n");

    return 1;
}

sub _h_00_0015 {
    my $self = shift;
    my $data = shift;
    my $state = shift;
    my ($amaj, $amin, $curidx, $maxidx, $certdata);

    $state->{-curidx} = -1 unless(exists($state->{-curidx}));
    $state->{-maxidx} = -1 unless(exists($state->{-maxidx}));
    $state->{-cert} = '' unless(exists($state->{-cert}));

    ($amaj, $amin, $curidx, $maxidx, $certdata) = unpack("CCCCa*", $data);

    printf("RetDevAuthenticationInfo (0x00, 0x15) D->I\n");
    printf(" Authentication version: %d.%d\n", $amaj, $amin);
    printf(" Segment:                %d of %d\n", $curidx, $maxidx);

    if ($curidx-1 != $state->{-curidx}) {
        printf("  WARNING! Out of order segment\n");
        return 0;
    }

    if (($maxidx != $state->{-maxidx}) && ($state->{-maxidx} != -1)) {
        printf("  WARNING! maxidx changed midstream\n");
        return 0;
    }

    if ($curidx > $maxidx) {
        printf("  WARNING! Too many segments\n");
        return 0;
    }

    $state->{-curidx} = $curidx;
    $state->{-maxidx} = $maxidx;
    $state->{-cert} .= $certdata;

    if ($curidx == $maxidx) {
        printf(" Certificate:            %s\n", Device::iPod->_hexstring($state->{-cert}));
    }

    return 1;
}

sub _h_00_0016 {
    my $self = shift;
    my $data = shift;
    my $state = shift;
    my $res;

    $res = unpack("C", $data);

    printf("AckDevAuthenticationInfo (0x00, 0x16) I->D\n");
    printf(" Result:         ");
    if ($res == 0x00) {
        printf("Authentication information supported\n");
    } elsif ($res == 0x08) {
        printf("Authentication information unpported\n");
    } elsif ($res == 0x0A) {
        printf("Certificate invalid\n");
    } elsif ($res == 0x0B) {
        printf("Certificate permissions are invalid\n");
    } else {
        printf("Unknown result 0x%02x\n", $res);
    }

    return 1;
}

sub _h_00_0017 {
    my $self = shift;
    my $data = shift;
    my $state = shift;
    my ($challenge, $retry);

    printf("GetDevAuthenticationSignature (0x00, 0x17) I->D\n");

    if (length($data) == 17) {
        ($challenge, $retry) = unpack("a16C", $data);
    } elsif (length($data) == 21) {
        ($challenge, $retry) = unpack("a20C", $data);
    } else {
        printf(" WARNING! Unsupported data length: %d\n", length($data));
        return 0;
    }

    printf(" Challenge:              %s (%d bytes)\n", Device::iPod->_hexstring($challenge), length($challenge));
    printf(" Retry counter:          %d\n", $retry);

    return 1;
}

sub _h_00_0018 {
    my $self = shift;
    my $data = shift;
    my $state = shift;
    my $reply;

    printf("RetDevAuthenticationSignature (0x00, 0x18) D->I\n");
    printf(" Data:                  %s\n", Device::iPod->_hexstring($data));

    return 1;
}

sub _h_00_0019 {
    my $self = shift;
    my $data = shift;
    my $state = shift;
    my $res;

    $res = unpack("C", $data);

    printf("AckiPodAuthenticationInfo (0x00, 0x19) I->D\n");
    printf(" Status:           %s (%d)\n", (
        "OK")[$res], $res);

    return 1;
}

sub _h_00_0024 {
    my $self = shift;
    my $data = shift;
    my $state = shift;

    printf("GetiPodOptions (0x00, 0x24) D->I\n");

    return 1;
}

sub _h_00_0025 {
    my $self = shift;
    my $data = shift;
    my $state = shift;
    my ($ophi, $oplo);

    ($ophi, $oplo) = unpack("NN", $data);

    printf("RetiPodOptions (0x00, 0x24) I->D\n");
    printf(" Options:\n");
    printf("  iPod supports SetiPodPreferences\n") if ($oplo & 0x02);
    printf("  iPod supports video\n") if ($oplo & 0x01);

    return 1;
}


sub _h_00_0027 {
    my $self = shift;
    my $data = shift;
    my $state = shift;
    my $acctype;
    my $accparam;

    $acctype = unpack("C", $data);

    printf("GetAccessoryInfo (0x00, 0x27) I->D\n");
    printf(" Accessory Info Type:    %s (%d)\n", (
        "Info capabilities",
        "Name",
        "Minimum supported iPod firmware version",
        "Minimum supported lingo version",
        "Firmware version",
        "Hardware version",
        "Manufacturer",
        "Model Number",
        "Serial Number",
        "Maximum payload size")[$acctype], $acctype);
    if ($acctype == 0x02) {
        my ($modelid, $maj, $min, $rev);
        
        ($modelid, $maj, $min, $rev) = unpack("xNCCC", $data);
        printf(" Model ID:             0x%04x\n", $modelid);
        printf(" iPod Firmware:        %d.%d.%d\n", $maj, $min, $rev);
    } elsif ($acctype == 0x03) {
        my $lingo;

        $lingo = unpack("xC", $data);
        printf(" Lingo:                0x%02x\n", $lingo);
    }

    return 1;
}

sub _h_04_0001 {
    my $self = shift;
    my $data = shift;
    my $state = shift;
    my $res;
    my $cmd;

    ($res, $cmd) = unpack("Cn", $data);

    printf("ACK (0x04, 0x0001) I->D\n");
    printf(" Acknowledged command: 0x%02x\n", $cmd);
    printf(" Status:               %s (%d)\n",
        ("Success",
         "ERROR: Unknown Database Category",
         "ERROR: Command Failed",
         "ERROR: Out Of Resource",
         "ERROR: Bad Parameter",
         "ERROR: Unknown ID",
         "Reserved",
         "ERROR: Not Authenticated")[$res], $res);

    return 1;
}

sub _h_04_000c {
    my $self = shift;
    my $data = shift;
    my $state = shift;
    my ($info, $track, $chapter);

    ($info, $track, $chapter) = unpack("CNn", $data);

    printf("GetIndexedPlayingTrackInfo (0x04, 0x000C) D->I\n");
    printf(" Track:                 %d\n", $track);
    printf(" Chapter:               %d\n", $chapter);
    printf(" Info requested:        %s (%d)\n", (
        "Capabilities and information",
        "Podcast name",
        "Track release date",
        "Track description",
        "Track song lyrics",
        "Track genre",
        "Track Composer",
        "Tracn Artwork count")[$info], $info);

    return 1;
}

sub _h_04_000d {
    my $self = shift;
    my $data = shift;
    my $state = shift;
    my $info;

    $info = unpack("C", $data);

    printf("ReturnIndexedPlayingTrackInfo (0x04, 0x000D) I->D\n");
    if ($info == 0x00) {
        my ($capability, $length, $chapter);

        ($capability, $length, $chapter) = unpack("xNNn", $data);
        printf(" Capabilities:\n");
        printf("  Is audiobook\n") if ($capability & 0x00000001);
        printf("  Has chapters\n") if ($capability & 0x00000002);
        printf("  Has album artwork\n") if ($capability & 0x00000004);
        printf("  Has song lyrics\n") if ($capability & 0x00000008);
        printf("  Is a podcast episode\n") if ($capability & 0x00000010);
        printf("  Has release date\n") if ($capability & 0x00000020);
        printf("  Has description\n") if ($capability & 0x00000040);
        printf("  Contains video\n") if ($capability & 0x00000080);
        printf("  Queued to play as video\n") if ($capability & 0x00000100);

        printf(" Length:              %d ms\n", $length);
        printf(" Chapters:            %d\n", $chapter);
    } else {
        printf(" WARNING: Unknown info\n");
        return 1;
    }

    return 1;
}

sub _h_04_0012 {
    my $self = shift;
    my $data = shift;
    my $state = shift;

    printf("RequestProtocolVersion (0x04, 0x0012) D->I\n");

    return 1;
}

sub _h_04_0013 {
    my $self = shift;
    my $data = shift;
    my $state = shift;
    my ($maj, $min);

    ($maj, $min) = unpack("CC", $data);

    printf("ReturnProtocolVersion (0x04, 0x0013) I->D\n");
    printf(" Lingo 0x04 version:         %d.%02d\n", $maj, $min);

    return 1;
}

sub _h_04_0016 {
    my $self = shift;
    my $data = shift;
    my $state = shift;

    printf("ResetDBSelection (0x04, 0x0016) D->I\n");

    return 1;
}

sub _h_04_0018 {
    my $self = shift;
    my $data = shift;
    my $state = shift;
    my $category;

    $category = unpack("C", $data);

    printf("GetNumberCategorizedDBRecords (0x04, 0x0018) D->I\n");
    printf(" Category:              %s (%d)\n", (
        "Reserved",
        "Playlist",
        "Artist",
        "Album",
        "Genre",
        "Track",
        "Composer",
        "Audiobook",
        "Podcast",
        "Nested Playlist")[$category], $category);

    return 1;
}

sub _h_04_0019 {
    my $self = shift;
    my $data = shift;
    my $state = shift;
    my $count;

    $count = unpack("N", $data);

    printf("ReturnNumberCategorizedDBRecords (0x04, 0x0019) I->D\n");
    printf(" Count:                 %d\n", $count);

    return 1;
}

sub _h_04_001c {
    my $self = shift;
    my $data = shift;
    my $state = shift;

    printf("GetPlayStatus (0x04, 0x001C) D->I\n");

    return 1;
}

sub _h_04_001d {
    my $self = shift;
    my $data = shift;
    my $state = shift;
    my ($len, $pos, $s);

    ($len, $pos, $s) = unpack("NNC", $data);

    printf("ReturnPlayStatus (0x04, 0x001D) I->D\n");
    printf(" Song length:             %d ms\n", $len);
    printf(" Song position:           %d ms\n", $pos);
    printf(" Player state:            ");
    if ($s == 0x00) {
        printf("Stopped\n");
    } elsif ($s == 0x01) {
        printf("Playing\n");
    } elsif ($s == 0x02) {
        printf("Paused\n");
    } elsif ($s == 0xFF) {
        printf("Error\n");
    } else {
        printf("Reserved\n");
    }

    return 1;
}

sub _h_04_001e {
    my $self = shift;
    my $data = shift;
    my $state = shift;

    printf("GetCurrentPlayingTrackIndex (0x04, 0x001E) D->I\n");

    return 1;
}

sub _h_04_001f {
    my $self = shift;
    my $data = shift;
    my $state = shift;
    my $num;

    $num = unpack("N", $data);

    printf("ReturnCurrentPlayingTrackIndex (0x04, 0x001F) I->D\n");
    printf(" Index:                  %d\n", $num);

    return 1;
}

sub _h_04_0020 {
    my $self = shift;
    my $data = shift;
    my $state = shift;
    my $track;

    $track = unpack("N", $data);

    printf("GetIndexedPlayingTrackTitle (0x04, 0x0020) D->I\n");
    printf(" Track:                 %d\n", $track);

    return 1;
}

sub _h_04_0021 {
    my $self = shift;
    my $data = shift;
    my $state = shift;
    my $title;

    $title = unpack("Z*", $data);

    printf("ReturnIndexedPlayingTrackTitle (0x04, 0x0021) I->D\n");
    printf(" Title:                 %s\n", $title);

    return 1;
}

sub _h_04_0022 {
    my $self = shift;
    my $data = shift;
    my $state = shift;
    my $track;

    $track = unpack("N", $data);

    printf("GetIndexedPlayingTrackArtistName (0x04, 0x0022) D->I\n");
    printf(" Track:                 %d\n", $track);

    return 1;
}

sub _h_04_0023 {
    my $self = shift;
    my $data = shift;
    my $state = shift;
    my $artist;

    $artist = unpack("Z*", $data);

    printf("ReturnIndexedPlayingTrackArtistName (0x04, 0x0023) I->D\n");
    printf(" Artist:                %s\n", $artist);

    return 1;
}

sub _h_04_0024 {
    my $self = shift;
    my $data = shift;
    my $state = shift;
    my $track;

    $track = unpack("N", $data);

    printf("GetIndexedPlayingTrackAlbumName (0x04, 0x0024) D->I\n");
    printf(" Track:                 %d\n", $track);

    return 1;
}

sub _h_04_0025 {
    my $self = shift;
    my $data = shift;
    my $state = shift;
    my $title;

    $title = unpack("Z*", $data);

    printf("ReturnIndexedPlayingTrackAlbumName (0x04, 0x0025) I->D\n");
    printf(" Album:                 %s\n", $title);

    return 1;
}

sub _h_04_0026 {
    my $self = shift;
    my $data = shift;
    my $state = shift;
    my $notification;

    if (length($data) == 1) {
        $notification = unpack("C", $data);
    } elsif (length($data) == 4) {
        $notification = unpack("N", $data);
    }

    printf("SetPlayStatusChangeNotification (0x04, 0x0026) D->I\n");

    if (length($data) == 1) {
        printf(" Events for:            %s (%d)\n", (
            "Disable all",
            "Basic play state, track index, track time position, FFW/REW seek stop, and chapter index changes")[$notification], $notification);
    } elsif (length($data) == 4) {
        printf(" Events for:\n");
        printf("  Basic play state changes\n") if ($notification & 0x00000001);
        printf("  Extended play state changes\n") if ($notification & 0x00000002);
        printf("  Track index\n") if ($notification & 0x00000004);
        printf("  Track time offset (ms)\n") if ($notification & 0x00000008);
        printf("  Track time offset (s)\n") if ($notification & 0x00000010);
        printf("  Chapter index\n") if ($notification & 0x00000020);
        printf("  Chapter time offset (ms)\n") if ($notification & 0x00000040);
        printf("  Chapter time offset (s)\n") if ($notification & 0x00000080);
        printf("  Track unique identifier\n") if ($notification & 0x00000100);
        printf("  Track media tyoe\n") if ($notification & 0x00000200);
        printf("  Track lyrics\n") if ($notification & 0x00000400);
    } else {
        printf(" WARNING: Unknown length for state\n");
        return 0;
    }

    return 1;
}

sub _h_04_0027 {
    my $self = shift;
    my $data = shift;
    my $state = shift;
    my $info;

    $info = unpack("C", $data);

    printf("PlayStatusChangeNotification (0x04, 0x0029) I->D\n");
    printf(" Status:\n");
    if ($info == 0x00) {
        printf("  Playback stopped\n");
    } elsif ($info == 0x01) {
        my $index = unpack("xN", $data);

        printf("  Track index:             %d\n", $index);
    } elsif ($info == 0x02) {
        printf("  Playback FF seek stop\n");
    } elsif ($info == 0x03) {
        printf("  Playback REW seek stop\n");
    } elsif ($info == 0x04) {
        my $offset = unpack("xN", $data);

        printf("  Track time offset:       %d ms\n", $offset);
    } elsif ($info == 0x05) {
        my $index = unpack("xN", $data);

        printf("  Chapter index:           %d\n", $index);
    } elsif ($info == 0x06) {
        my $status = unpack("xC", $data);

        printf("  Playback status extended: %s (%d)\n", (
            "Reserved",
            "Reserved",
            "Stopped",
            "Reserved",
            "Reserved",
            "FF seek started",
            "REW seek started",
            "FF/REW seek stopped",
            "Reserved",
            "Reserved",
            "Playing",
            "Paused")[$status], $status);
    } elsif ($info == 0x07) {
        my $offset = unpack("xN", $data);

        printf("  Track time offset:       %d s\n", $offset);
    } elsif ($info == 0x08) {
        my $offset = unpack("xN", $data);

        printf("  Chapter time offset      %d ms\n", $offset);
    } elsif ($info == 0x09) {
        my $offset = unpack("xN", $data);

        printf("  Chapter time offset      %d s\n", $offset);
    } elsif ($info == 0x0A) {
        my ($uidhi, $uidlo) = unpack("xNN", $data);

        printf("  Track UID:               %08x%08x\n", $uidhi, $uidlo);
    } elsif ($info == 0x0B) {
        my $mode = unpack("xC", $data);

        printf("  Track mode: %s (%d)\n", (
            "Audio track",
            "Video track")[$mode], $mode);
    } elsif ($info == 0x0C) {
        printf("  Track lyrics ready\n");
    } else {
        printf("  Reserved\n");
    }

    return 1;
}

sub _h_04_0029 {
    my $self = shift;
    my $data = shift;
    my $state = shift;
    my $control;

    $control = unpack("C", $data);

    printf("PlayControl (0x04, 0x0029) D->I\n");
    printf(" Command:             %s (%d)\n", (
        "Reserved",
        "Toggle Play/Pause",
        "Stop",
        "Next track",
        "Previous track",
        "Start FF",
        "Start Rev",
        "Stop FF/Rev",
        "Next",
        "Previous",
        "Play",
        "Pause",
        "Next chapter",
        "Previous chapter")[$control], $control);

    return 1;
}

sub _h_04_002c {
    my $self = shift;
    my $data = shift;
    my $state = shift;

    printf("GetShuffle (0x04, 0x002C) D->I\n");

    return 1;
}

sub _h_04_002d {
    my $self = shift;
    my $data = shift;
    my $state = shift;
    my $mode;

    $mode = unpack("C", $data);

    printf("ReturnShuffle (0x04, 0x002D) I->D\n");
    printf(" Mode:                %s (%d)\n", (
        "Off",
        "Tracks",
        "Albums")[$mode], $mode);

    return 1;
}

sub _h_04_002f {
    my $self = shift;
    my $data = shift;
    my $state = shift;

    printf("GetRepeat (0x04, 0x002F) D->I\n");

    return 1;
}

sub _h_04_0030 {
    my $self = shift;
    my $data = shift;
    my $state = shift;
    my $mode;

    $mode = unpack("C", $data);

    printf("ReturnRepeat (0x04, 0x0030) I->D\n");
    printf(" Mode:                %s (%d)\n", (
        "Off",
        "One Track",
        "All Tracks")[$mode], $mode);

    return 1;
}

sub _h_04_0032 {
    my $self = shift;
    my $data = shift;
    my $state = shift;
    my ($index, $format, $width, $height, $stride, $imagedata);

    $state->{-index} = -1 unless(exists($state->{-index}));
    $state->{-image} = '' unless(exists($state->{-image}));

    ($index, $format, $width, $height, $stride, $imagedata) = unpack("nCnnNa*", $data);

    printf("SetDisplayImage (0x04, 0x0032) D->I\n");
    if ($index == 0) {
        printf(" Width:               %d\n", $width);
        printf(" Height:              %d\n", $height);
        printf(" Stride:              %d\n", $stride);
        printf(" Format:              %s (%d)\n", (
            "Reserved",
            "Monochrome, 2 bps",
            "RGB 565, little endian",
            "RGB 565, big endian")[$format], $format);

        $state->{-imagelength} = $height * $stride;
    } else {
        ($index, $imagedata) = unpack("na*", $data);
    }

    if ($index-1 != $state->{-index}) {
        printf(" WARNING! Out of order segment\n");
        return 0;
    }

    $state->{-index} = $index;
    $state->{-image} .= $imagedata;

    if (length($state->{-image}) >= $state->{-imagelength}) {
        printf(" Image data:           %s\n", Device::iPod->_hexstring($state->{-image}));
    }

    return 1;
}

sub _h_04_0033 {
    my $self = shift;
    my $data = shift;
    my $state = shift;

    printf("GetMonoDisplayImageLimits (0x04, 0x0033) D->I\n");

    return 1;
}

sub _h_04_0034 {
    my $self = shift;
    my $data = shift;
    my $state = shift;
    my ($width, $height, $format);

    ($width, $height, $format) = unpack("nnC", $data);

    printf("ReturnMonoDisplayImageLimits (0x04, 0x0034) I->D\n");
    printf(" Width:                  %d\n", $width);
    printf(" Height:                 %d\n", $height);
    printf(" Format:                 %s (%d)\n", (
        "Reserved",
        "Monochrome, 2 bps",
        "RGB 565, little endian",
        "RGB 565, big endian")[$format], $format);

    return 1;
}

sub _h_04_0035 {
    my $self = shift;
    my $data = shift;
    my $state = shift;

    printf("GetNumPlayingTracks (0x04, 0x0035) D->I\n");

    return 1;
}

sub _h_04_0036 {
    my $self = shift;
    my $data = shift;
    my $state = shift;
    my $num;

    $num = unpack("N", $data);

    printf("ReturnNumPlayingTracks (0x04, 0x0036) I->D\n");
    printf(" Number:                 %d\n", $num);

    return 1;
}

sub _h_04_0037 {
    my $self = shift;
    my $data = shift;
    my $state = shift;
    my $num;

    $num = unpack("N", $data);

    printf("SetCurrentPlayingTrack (0x04, 0x0037) D->I\n");
    printf(" Track:                 %d\n", $num);

    return 1;
}
    


package main;

use Device::iPod;
use strict;

my $decoder;
my $device;
my $line;

$decoder = iap::decode->new();
$device = Device::iPod->new();

while ($line = <>) {
    my $m;
    my @m;

    chomp($line);
    $line =~ s/(..)/chr(hex($1))/ge;
    $device->{-inbuf} = $line;

    $m = $device->_message();
    next unless defined($m);
    @m = $device->_unframe_cmd($m);
    unless(@m) {
        printf("Line %d: Error decoding frame: %s\n", $., $device->error());
        next;
    }

    printf("Line %d: ", $.);
    $decoder->display(@m);
}
