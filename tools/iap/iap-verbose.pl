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

sub _h_00_0001 {
    my $self = shift;
    my $data = shift;
    my $state = shift;
    my $lingo = shift;

    $lingo = unpack("C", $data);

    printf("Identify (0x00, 0x01) D->I\n");
    printf(" Lingo:               0x%02x\n", $lingo);

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
        $delay = unpack("xxN", $data);
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

sub _h_00_000d {
    my $self = shift;
    my $data = shift;
    my $state = shift;

    printf("RequestiPodModelNum (0x00, 0x0D) D->I\n");

    return 1;
}

sub _h_00_000e {
    my $self = shift;
    my $data = shift;
    my $state = shift;
    my ($modelnum, $name);

    ($modelnum, $name) = unpack("NZ*", $data);

    printf("ReturniPodModelNum (0x00, 0x0E) I->D\n");
    printf(" Model number:          %08x\n", $modelnum);
    printf(" Model name:            %s\n", $name);

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

    delete($self->{-state}->{'_h_00_0015'});

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

    printf("RetiPodOptions (0x00, 0x25) I->D\n");
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

sub _h_00_0028 {
    my $self = shift;
    my $data = shift;
    my $state = shift;
    my $acctype;
    my $accparam;

    $acctype = unpack("C", $data);

    printf("RetAccessoryInfo (0x00, 0x28) D->I\n");
    printf(" Accessory Info Type:      %s (%d)\n", (
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

    if ($acctype == 0x00) {
        $accparam = unpack("xN", $data);
        printf("  Accessory Info capabilities\n") if ($accparam & 0x01);
        printf("  Accessory name\n") if ($accparam & 0x02);
        printf("  Accessory minimum supported iPod firmware\n") if ($accparam & 0x04);
        printf("  Accessory minimum supported lingo version\n") if ($accparam & 0x08);
        printf("  Accessory firmware version\n") if ($accparam & 0x10);
        printf("  Accessory hardware version\n") if ($accparam & 0x20);
        printf("  Accessory manufacturer\n") if ($accparam & 0x40);
        printf("  Accessory model number\n") if ($accparam & 0x80);
        printf("  Accessory serial number\n") if ($accparam & 0x100);
        printf("  Accessory incoming max packet size\n") if ($accparam & 0x200);
    }

    if ($acctype ~~ [0x01, 0x06, 0x07, 0x08]) {
        $accparam = unpack("xZ*", $data);
        printf(" Data:                     %s\n", $accparam);
    }

    if ($acctype == 0x02) {
        $accparam = [ unpack("xNCCC", $data) ];
        printf(" Model ID:                 %08x\n", $accparam->[0]);
        printf(" Firmware version:         %d.%02d.%02d\n", $accparam->[1], $accparam->[2], $accparam->[3]);
    }

    if ($acctype == 0x03) {
        $accparam = [ unpack("xCCC", $data) ];
        printf(" Lingo:                    %02x\n", $accparam->[0]);
        printf(" Version:                  %d.%02d\n", $accparam->[1], $accparam->[2]);
    }

    if ($acctype ~~ [0x04, 0x05]) {
        $accparam = [ unpack("xCCC", $data) ];
        printf(" Version:                  %d.%02d.%02d\n", @{$accparam});
    }

    return 1;
}

sub _h_00_0029 {
    my $self = shift;
    my $data = shift;
    my $state = shift;
    my $class;

    $class = unpack("C", $data);

    printf("GetiPodPreferences (0x00, 0x29) D->I\n");
    printf(" Class:                 %s (%d)\n", (
        "Video out setting",
        "Screen configuration",
        "Video signal format",
        "Line Out usage",
        "(Reserved)",
        "(Reserved)",
        "(Reserved)",
        "(Reserved)",
        "Video out connection",
        "Closed captioning",
        "Video aspect ratio",
        "(Reserved)",
        "Subtitles",
        "Video alternate audio channel")[$class], $class);

    return 1;
}

sub _h_00_002b {
    my $self = shift;
    my $data = shift;
    my $state = shift;
    my ($class, $setting);

    ($class, $setting) = unpack("CC", $data);

    printf("SetiPodPreferences (0x00, 0x2B) D->I\n");
    printf(" Class:             %s (%d)\n", (
        "Video out setting",
        "Screen configuration",
        "Video signal format",
        "Line out usage",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Video-out connection",
        "Closed captioning",
        "Video monitor aspect ratio",
        "Reserved",
        "Subtitles",
        "Video alternate audio channel")[$class], $class);
    printf(" Setting:           %s (%d)\n", (
        ["Off",
         "On",
         "Ask user"],
        ["Fill screen",
         "Fit to screen edge"],
        ["NTSC",
         "PAL"],
        ["Not used",
         "Used"],
        [],
        [],
        [],
        [],
        ["None",
         "Composite",
         "S-video",
         "Component"],
        ["Off",
         "On"],
        ["4:3",
         "16:9"],
        [],
        ["Off",
         "On"],
        ["Off",
         "On"])[$class][$setting], $setting);

    return 1;
}

sub _h_00_0038 {
    my $self = shift;
    my $data = shift;
    my $state = shift;
    my $transid;

    $transid = unpack("n", $data);

    printf("StartIDPS (0x00, 0x38) D->I\n");
    printf(" TransID:          %d\n", $transid);

    return 1;
}

sub _h_00_003b {
    my $self = shift;
    my $data = shift;
    my $state = shift;
    my ($transid, $status);

    ($transid, $status) = unpack("nC", $data);

    printf("EndIDPS (0x00, 0x3B) D->I\n");
    printf(" TransID:          %d\n", $transid);
    printf(" Action:           %s (%d)\n", (
        "Finished",
        "Reset")[$status], $status);

    return 1;
}

sub _h_00_004b {
    my $self = shift;
    my $data = shift;
    my $state = shift;
    my $lingo;

    $lingo = unpack("C", $data);

    printf("GetiPodOptionsForLingo (0x00, 0x4B) D->I\n");
    printf(" Lingo:               0x%02x\n", $lingo);

    return 1;
}

sub _h_02_0000 {
    my $self = shift;
    my $data = shift;
    my $state = shift;
    my @keys;

    @keys = unpack("CCCC", $data);

    printf("ContextButtonStatus (0x02, 0x00) D->I\n");
    printf(" Buttons:\n");
    printf("  Play/Pause\n") if ($keys[0] & 0x01);
    printf("  Volume Up\n") if ($keys[0] & 0x02);
    printf("  Volume Down\n") if ($keys[0] & 0x04);
    printf("  Next Track\n") if ($keys[0] & 0x08);
    printf("  Previous Track\n") if ($keys[0] & 0x10);
    printf("  Next Album\n") if ($keys[0] & 0x20);
    printf("  Previous Album\n") if ($keys[0] & 0x40);
    printf("  Stop\n") if ($keys[0] & 0x80);

    if (exists($keys[1])) {
        printf("  Play/Resume\n") if ($keys[1] & 0x01);
        printf("  Pause\n") if ($keys[1] & 0x02);
        printf("  Mute toggle\n") if ($keys[1] & 0x04);
        printf("  Next Chapter\n") if ($keys[1] & 0x08);
        printf("  Previous Chapter\n") if ($keys[1] & 0x10);
        printf("  Next Playlist\n") if ($keys[1] & 0x20);
        printf("  Previous Playlist\n") if ($keys[1] & 0x40);
        printf("  Shuffle Setting Advance\n") if ($keys[1] & 0x80);
    }

    if (exists($keys[2])) {
        printf("  Repeat Setting Advance\n") if ($keys[2] & 0x01);
        printf("  Power On\n") if ($keys[2] & 0x02);
        printf("  Power Off\n") if ($keys[2] & 0x04);
        printf("  Backlight for 30 seconds\n") if ($keys[2] & 0x08);
        printf("  Begin FF\n") if ($keys[2] & 0x10);
        printf("  Begin REW\n") if ($keys[2] & 0x20);
        printf("  Menu\n") if ($keys[2] & 0x40);
        printf("  Select\n") if ($keys[2] & 0x80);
    }

    if (exists($keys[3])) {
        printf("  Up Arrow\n") if ($keys[3] & 0x01);
        printf("  Down Arrow\n") if ($keys[3] & 0x02);
        printf("  Backlight off\n") if ($keys[3] & 0x04);
    }

    return 1;
}

sub _h_02_0001 {
    my $self = shift;
    my $data = shift;
    my $state = shift;
    my $res;
    my $cmd;

    ($res, $cmd) = unpack("CC", $data);

    printf("ACK (0x02, 0x01) I->D\n");
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

    return 1;
}

sub _h_03_0000 {
    my $self = shift;
    my $data = shift;
    my $state = shift;
    my $res;
    my $cmd;

    ($res, $cmd) = unpack("CC", $data);

    printf("ACK (0x03, 0x00) I->D\n");
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

    return 1;
}

sub _h_03_0008 {
    my $self = shift;
    my $data = shift;
    my $state = shift;
    my $events;;

    $events = unpack("N", $data);

    printf("SetRemoteEventsNotification (0x03, 0x08) D->I\n");
    printf(" Events:\n");
    printf("  Track position in ms\n") if ($events & 0x00000001);
    printf("  Track playback index\n") if ($events & 0x00000002);
    printf("  Chapter index\n") if ($events & 0x00000004);
    printf("  Play status\n") if ($events & 0x00000008);
    printf("  Mute/UI volume\n") if ($events & 0x00000010);
    printf("  Power/Battery\n") if ($events & 0x00000020);
    printf("  Equalizer setting\n") if ($events & 0x00000040);
    printf("  Shuffle setting\n") if ($events & 0x00000080);
    printf("  Repeat setting\n") if ($events & 0x00000100);
    printf("  Date and time setting\n") if ($events & 0x00000200);
    printf("  Alarm setting\n") if ($events & 0x00000400);
    printf("  Backlight state\n") if ($events & 0x00000800);
    printf("  Hold switch state\n") if ($events & 0x00001000);
    printf("  Sound check state\n") if ($events & 0x00002000);
    printf("  Audiobook speed\n") if ($events & 0x00004000);
    printf("  Track position in s\n") if ($events & 0x00008000);
    printf("  Mute/UI/Absolute volume\n") if ($events & 0x00010000);

    return 1;
}

sub _h_03_0009 {
    my $self = shift;
    my $data = shift;
    my $state = shift;
    my $event;
    my $eventdata;

    $event = unpack("C", $data);

    printf("RemoteEventNotification (0x03, 0x09) I->D\n");
    printf(" Event:                %s (%d)\n", (
        "Track position in ms",
        "Track playback index",
        "Chapter index",
        "Play status",
        "Mute/UI volume",
        "Power/Battery",
        "Equalizer setting",
        "Shuffle setting",
        "Repeat setting",
        "Date and time setting",
        "Alarm setting",
        "Backlight state",
        "Hold switch state",
        "Sound check state",
        "Audiobook speed",
        "Track position in s",
        "Mute/UI/Absolute volume")[$event], $event);

    if ($event == 0x00) {
        $eventdata = unpack("xN", $data);
        printf("  Position:               %d ms\n", $eventdata);
    } elsif ($event == 0x01) {
        $eventdata = unpack("xN", $data);
        printf("  Track:                  %d\n", $eventdata);
    } elsif ($event == 0x02) {
        $eventdata = [ unpack("xNnn", $data) ];
        printf("  Track:                  %d\n", $eventdata->[0]);
        printf("  Chapter count:          %d\n", $eventdata->[1]);
        printf("  Chapter index:          %d\n", $eventdata->[2]);
    } elsif ($event == 0x03) {
        $eventdata = unpack("xC", $data);
        printf("  Status:                 %s (%d)\n", (
            "Stopped",
            "Playing",
            "Paused",
            "FF",
            "REW",
            "End FF/REW")[$eventdata], $eventdata);
    } elsif ($event == 0x04) {
        $eventdata = [ unpack("xCC") ];
        printf("  Mute:                   %s\n", $eventdata->[0]?"Off":"On");
        printf("  Volume:                 %d\n", $eventdata->[1]);
    } elsif ($event == 0x0F) {
        $eventdata = unpack("xn", $data);
        printf("  Position:               %d s\n", $eventdata);
    }

    return 1;
}

sub _h_03_000c {
    my $self = shift;
    my $data = shift;
    my $state = shift;
    my $info;

    $info = unpack("C", $data);

    printf("GetiPodStateInfo (0x03, 0x0C) D->I\n");
    printf(" Info to get:           %s (%d)\n", (
        "Track time in ms",
        "Track playback index",
        "Chapter information",
        "Play status",
        "Mute and volume information",
        "Power and battery status",
        "Equalizer setting",
        "Shuffle setting",
        "Repeat setting",
        "Date and time",
        "Alarm state and time",
        "Backlight state",
        "Hold switch state",
        "Audiobook speed",
        "Track time in seconds",
        "Mute/UI/Absolute volume")[$info], $info);

    return 1;
}

sub _h_03_000d {
    my $self = shift;
    my $data = shift;
    my $state = shift;
    my $type;
    my $info;

    $type = unpack("C", $data);

    printf("RetiPodStateInfo (0x03, 0x0E) D->I\n");

    if ($type == 0x00) {
        $info = unpack("xN", $data);
        printf(" Type:             Track position\n");
        printf(" Position:         %d ms\n", $info);
    } elsif ($type == 0x01) {
        $info = unpack("xN", $data);
        printf(" Type:             Track index\n");
        printf(" Index:            %d\n", $info);
    } elsif ($type == 0x02) {
        $info = [ unpack("xNnn", $data) ];
        printf(" Type:             Chapter Information\n");
        printf(" Playing Track:    %d\n", $info->[0]);
        printf(" Chapter count:    %d\n", $info->[1]);
        printf(" Chapter index:    %d\n", $info->[2]);
    } elsif ($type == 0x03) {
        $info = unpack("xC", $data);
        printf(" Type:             Play status\n");
        printf(" Status:           %s (%d)\n", (
            "Stopped",
            "Playing",
            "Paused",
            "FF",
            "REW",
            "End FF/REW")[$info], $info);
    } elsif ($type == 0x04) {
        $info = [unpack("xCC", $data)];
        printf(" Type:             Mute/Volume\n");
        printf(" Mute State:       %s\n", $info->[0]?"On":"Off");
        printf(" Volume level:     %d\n", $info->[1]);
    } elsif ($type == 0x05) {
        $info = [unpack("xCC", $data)];
        printf(" Type:             Battery Information\n");
        printf(" Power state:      %s (%d)\n", (
            "Internal, low power (<30%)",
            "Internal",
            "External battery pack, no charging",
            "External power, no charging",
            "External power, charging",
            "External power, charged")[$info->[0]], $info->[0]);
        printf(" Battery level:    %d%%\n", $info->[1]*100/255);
    } elsif ($type == 0x06) {
        $info = [unpack("xN", $data)];
        printf(" Type:             Equalizer\n");
        printf(" Index:            %d\n", $info->[0]);
    } elsif ($type == 0x07) {
        $info = [unpack("xC", $data)];
        printf(" Type:             Shuffle\n");
        printf(" Shuffle State:    %s\n", $info->[0]?"On":"Off");
    } elsif ($type == 0x08) {
        $info = [unpack("xC", $data)];
        printf(" Type:             Repeat\n");
        printf(" Repeat State:     %s\n", $info->[0]?"On":"Off");
    } elsif ($type == 0x09) {
        $info = [unpack("xnCCCC", $data)];
        printf(" Type:             Date\n");
        printf(" Date:             %02d.%02d.%04d %02d:%02d\n", $info->[2], $info->[1], $info->[0], $info->[3], $info->[4]);
    } elsif ($type == 0x0A) {
        $info = [unpack("xCCC", $data)];
        printf(" Type:             Alarm\n");
        printf(" Alarm State:      %s\n", $info->[0]?"On":"Off");
        printf(" Time:             %02d:%02d\n", $info->[1], $info->[2]);
    } elsif ($type == 0x0B) {
        $info = [unpack("xC", $data)];
        printf(" Type:             Backlight\n");
        printf(" Backlight State:  %s\n", $info->[0]?"On":"Off");
    } elsif ($type == 0x0C) {
        $info = [unpack("xC", $data)];
        printf(" Type:             Hold switch\n");
        printf(" Switch State:     %s\n", $info->[0]?"On":"Off");
    } elsif ($type == 0x0D) {
        $info = [unpack("xC", $data)];
        printf(" Type:             Sound check\n");
        printf(" Sound check:      %s\n", $info->[0]?"On":"Off");
    } elsif ($type == 0x0E) {
        $info = [unpack("xC", $data)];
        printf(" Type:             Audiobook speed\n");
        printf(" Speed:            %s\n", $info->[0]==0x00?"Normal":$info->[0]==0x01?"Faster":$info->[0]==0xFF?"Slower":"Reserved");
    } elsif ($type == 0x0F) {
        $info = unpack("xN", $data);
        printf(" Type:             Track position\n");
        printf(" Position:         %d s\n", $info);
    } elsif ($type == 0x10) {
        $info = [unpack("xCCC", $data)];
        printf(" Type:             Mute/UI/Absolute volume\n");
        printf(" Mute State:       %s\n", $info->[0]?"On":"Off");
        printf(" UI Volume level:  %d\n", $info->[1]);
        printf(" Absolute Volume:  %d\n", $info->[2]);
    } else {
        printf(" Reserved\n");
    }

    return 1;
}


sub _h_03_000e {
    my $self = shift;
    my $data = shift;
    my $state = shift;
    my $type;
    my $info;

    $type = unpack("C", $data);

    printf("SetiPodStateInfo (0x03, 0x0E) D->I\n");

    if ($type == 0x00) {
        $info = unpack("xN", $data);
        printf(" Type:             Track position\n");
        printf(" Position:         %d ms\n", $info);
    } elsif ($type == 0x01) {
        $info = unpack("xN", $data);
        printf(" Type:             Track index\n");
        printf(" Index:            %d\n", $info);
    } elsif ($type == 0x02) {
        $info = unpack("xn", $data);
        printf(" Type:             Chapter index\n");
        printf(" Index:            %d\n", $info);
    } elsif ($type == 0x03) {
        $info = unpack("xC", $data);
        printf(" Type:             Play status\n");
        printf(" Status:           %s (%d)\n", (
            "Stopped",
            "Playing",
            "Paused",
            "FF",
            "REW",
            "End FF/REW")[$info], $info);
    } elsif ($type == 0x04) {
        $info = [unpack("xCCC", $data)];
        printf(" Type:             Mute/Volume\n");
        printf(" Mute State:       %s\n", $info->[0]?"On":"Off");
        printf(" Volume level:     %d\n", $info->[1]);
        printf(" Restore on exit:  %s\n", $info->[2]?"Yes":"No");
    } elsif ($type == 0x06) {
        $info = [unpack("xNC", $data)];
        printf(" Type:             Equalizer\n");
        printf(" Index:            %d\n", $info->[0]);
        printf(" Restore on exit:  %s\n", $info->[1]?"Yes":"No");
    } elsif ($type == 0x07) {
        $info = [unpack("xCC", $data)];
        printf(" Type:             Shuffle\n");
        printf(" Shuffle State:    %s\n", $info->[0]?"On":"Off");
        printf(" Restore on exit:  %s\n", $info->[1]?"Yes":"No");
    } elsif ($type == 0x08) {
        $info = [unpack("xCC", $data)];
        printf(" Type:             Repeat\n");
        printf(" Repeat State:     %s\n", $info->[0]?"On":"Off");
        printf(" Restore on exit:  %s\n", $info->[1]?"Yes":"No");
    } elsif ($type == 0x09) {
        $info = [unpack("xnCCCC", $data)];
        printf(" Type:             Date\n");
        printf(" Date:             %02d.%02d.%04d %02d:%02d\n", $info->[2], $info->[1], $info->[0], $info->[3], $info->[4]);
    } elsif ($type == 0x0A) {
        $info = [unpack("xCCCC", $data)];
        printf(" Type:             Alarm\n");
        printf(" Alarm State:      %s\n", $info->[0]?"On":"Off");
        printf(" Time:             %02d:%02d\n", $info->[1], $info->[2]);
        printf(" Restore on exit:  %s\n", $info->[3]?"Yes":"No");
    } elsif ($type == 0x0B) {
        $info = [unpack("xCC", $data)];
        printf(" Type:             Backlight\n");
        printf(" Backlight State:  %s\n", $info->[0]?"On":"Off");
        printf(" Restore on exit:  %s\n", $info->[1]?"Yes":"No");
    } elsif ($type == 0x0D) {
        $info = [unpack("xCC", $data)];
        printf(" Type:             Sound check\n");
        printf(" Sound check:      %s\n", $info->[0]?"On":"Off");
        printf(" Restore on exit:  %s\n", $info->[1]?"Yes":"No");
    } elsif ($type == 0x0E) {
        $info = [unpack("xCC", $data)];
        printf(" Type:             Audiobook speed\n");
        printf(" Speed:            %s\n", $info->[0]==0x00?"Normal":$info->[0]==0x01?"Faster":$info->[0]==0xFF?"Slower":"Reserved");
        printf(" Restore on exit:  %s\n", $info->[1]?"Yes":"No");
    } elsif ($type == 0x0F) {
        $info = unpack("xN", $data);
        printf(" Type:             Track position\n");
        printf(" Position:         %d s\n", $info);
    } elsif ($type == 0x10) {
        $info = [unpack("xCCCC", $data)];
        printf(" Type:             Mute/UI/Absolute volume\n");
        printf(" Mute State:       %s\n", $info->[0]?"On":"Off");
        printf(" UI Volume level:  %d\n", $info->[1]);
        printf(" Absolute Volume:  %d\n", $info->[2]);
        printf(" Restore on exit:  %s\n", $info->[3]?"Yes":"No");
    } else {
        printf(" Reserved\n");
    }

    return 1;
}

sub _h_03_000f {
    my $self = shift;
    my $data = shift;
    my $state = shift;

    print("GetPlayStatus (0x03, 0x0F) D->I\n");

    return 1;
}

sub _h_03_0010 {
    my $self = shift;
    my $data = shift;
    my $state = shift;
    my ($status, $idx, $len, $pos);

    ($status, $idx, $len, $pos) = unpack("CNNN", $data);

    printf("RetPlayStatus (0x03, 0x10) I->D\n");
    printf(" Status:          %s (%d)\n", (
        "Stopped",
        "Playing",
        "Paused")[$status], $status);
    if ($status != 0x00) {
        printf(" Track index:       %d\n", $idx);
        printf(" Track length:      %d\n", $len);
        printf(" Track position:    %d\n", $pos);
    }

    return 1;
}

sub _h_03_0012 {
    my $self = shift;
    my $data = shift;
    my $state = shift;
    my ($info, $tidx, $cidx);

    ($info, $tidx, $cidx) = unpack("CNn", $data);

    printf("GetIndexedPlayingTrackInfo (0x03, 0x12) D->I\n");
    printf(" Requested info:         %s (%d)\n", (
            "Track caps/info",
            "Chapter time/name",
            "Artist name",
            "Album name",
            "Genre name",
            "Track title",
            "Composer name",
            "Lyrics",
            "Artwork count")[$info], $info);
    printf(" Track index:            %d\n", $tidx);
    printf(" Chapter index:          %d\n", $cidx);

    return 1;
}

sub _h_03_0013 {
    my $self = shift;
    my $data = shift;
    my $state = shift;
    my $info;

    $info = unpack("C", $data);
    printf("RetIndexedPlayingTrackInfo (0x03, 0x13) I->D\n");
    printf(" Returned info:         %s (%d)\n", (
            "Track caps/info",
            "Chapter time/name",
            "Artist name",
            "Album name",
            "Genre name",
            "Track title",
            "Composer name",
            "Lyrics",
            "Artwork count")[$info], $info);
    if ($info == 0x00) {
        my ($caps, $len, $chap) = unpack("xNNn", $data);
        printf("  Track is audiobook\n") if ($caps & 0x01);
        printf("  Track has chapters\n") if ($caps & 0x02);
        printf("  Track has artwork\n") if ($caps & 0x04);
        printf("  Track contains video\n") if ($caps & 0x80);
        printf("  Track queued as video\n") if ($caps & 0x100);

        printf("  Track length:        %d ms\n", $len);
        printf("  Track chapters:      %d\n", $chap);
    } elsif ($info == 0x01) {
        my ($len, $name) = unpack("xNZ*");
        printf("  Chapter time:        %d ms\n", $len);
        printf("  Chapter name:        %s\n", $name);
    } elsif ($info >= 0x02 && $info <= 0x06) {
        my $name = unpack("xZ*", $data);
        printf("  Name/Title:          %s\n", $name)
    } elsif ($info == 0x07) {
        my ($info, $index, $data) = unpack("xCnZ*");
        printf("  Part of multiple packets\n") if ($info & 0x01);
        printf("  Is last packet\n") if ($info & 0x02);
        printf("  Packet index:        %d\n", $index);
        printf("  Data:                %s\n", $data);
    } elsif ($info == 0x08) {

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

sub _h_07_0005 {
    my $self = shift;
    my $data = shift;
    my $state = shift;
    my $control;

    $control = unpack("C", $data);

    printf("SetTunerCtrl (0x07, 0x05) I->D\n");
    printf(" Options:\n");
    printf("  Power %s\n", ($control & 0x01)?"on":"off");
    printf("  Status change notifications %s\n", ($control & 0x02)?"on":"off");
    printf("  Raw mode %s\n", ($control & 0x04)?"on":"off");
}

sub _h_07_0020 {
    my $self = shift;
    my $data = shift;
    my $state = shift;
    my $options;

    $options = unpack("N", $data);

    printf("SetRDSNotifyMask (0x07, 0x20) I->D\n");
    printf(" Options:\n");
    printf("  Radiotext\n") if ($options & 0x00000010);
    printf("  Program Service Name\n") if ($options & 0x40000000);
    printf("  Reserved\n") if ($options & 0xBFFFFFEF);

    return 1;
}

sub _h_07_0024 {
    my $self = shift;
    my $data = shift;
    my $state = shift;

    printf("Reserved command (0x07, 0x24) I->D\n");

    return 1;
}


package main;

use Device::iPod;
use Getopt::Long;
use strict;

my $decoder;
my $device;
my $unpacker;
my $line;

sub unpack_hexstring {
    my $line = shift;
    my $m;
    my @m;

    $line =~ s/(..)/chr(hex($1))/ge;
    $device->{-inbuf} = $line;

    $m = $device->_message();
    next unless defined($m);
    @m = $device->_unframe_cmd($m);
    unless(@m) {
        printf("Line %d: Error decoding frame: %s\n", $., $device->error());
        return ();
    }

    return @m;
}

sub unpack_iaplog {
    my $line = shift;
    my @m;

    unless ($line =~ /^(?:\[\d+\] )?[RT]::? /) {
        printf("Skipped: %s\n", $line);
        return ();
    }

    $line =~ s/^(?:\[\d+\] )?[RT]::? //;
    $line =~ s/\\x(..)/chr(hex($1))/ge;
    $line =~ s/\\\\/\\/g;

    @m = unpack("CCa*", $line);
    if ($m[0] == 0x04) {
        @m = unpack("Cna*", $line);
    }

    return @m;
}


$decoder = iap::decode->new();
$device = Device::iPod->new();
$unpacker = \&unpack_iaplog;

GetOptions("hexstring" => sub {$unpacker = \&unpack_hexstring});

while ($line = <>) {
    my @m;

    chomp($line);

    @m = $unpacker->($line);
    next unless (@m);

    printf("Line %d: ", $.);
    $decoder->display(@m);
}
