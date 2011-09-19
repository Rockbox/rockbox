use Test::More qw( no_plan );
use strict;

BEGIN { use_ok('Device::iPod'); }
require_ok('Device::iPod');

my $ipod = Device::iPod->new();
my $m;
my ($l, $c, $p);

isa_ok($ipod, 'Device::iPod');

# Frame a short command
$m = $ipod->_frame_cmd("\x00\x02\x00\x06");
ok(defined($m) && ($m eq "\xFF\x55\x04\x00\x02\x00\x06\xF4"), "Framed command valid");

# Frame a long command
$m = $ipod->_frame_cmd("\x00" x 1024);
ok(defined($m) && ($m eq "\xFF\x55\x00\x04\x00" . ("\x00" x 1024) . "\xFC"), "Long framed command valid");

# Frame an overly long command
$m = $ipod->_frame_cmd("\x00" x 65537);
ok(!defined($m) && ($ipod->error() =~ 'Command too long'), "Overly long command failed");

# Unframe a short command
($l, $c, $p) = $ipod->_unframe_cmd("\xFF\x55\x04\x00\x02\x00\x06\xF4");
ok(defined($l) && ($l == 0x00) && ($c == 0x02) && ($p eq "\x00\x06"), "Unframed short command");

# Unframe a long command
($l, $c, $p) = $ipod->_unframe_cmd("\xFF\x55\x00\x04\x00" . ("\x00" x 1024) . "\xFC");
ok(defined($l) && ($l == 0x00) && ($c == 0x00) && ($p eq "\x00" x 1022), "Unframed long command");

# Frame without sync byte
($l, $c, $p) = $ipod->_unframe_cmd("\x00");
ok(!defined($l) && ($ipod->error() =~ /Could not unframe data/), "Frame without sync byte failed");

# Frame without SOP byte
($l, $c, $p) = $ipod->_unframe_cmd("\xFF");
ok(!defined($l) && ($ipod->error() =~ /Could not unframe data/), "Frame without SOP byte failed");

# Frame with length 0
($l, $c, $p) = $ipod->_unframe_cmd("\xFF\x55\x00\x00\x00");
ok(!defined($l) && ($ipod->error() =~ /Length is 0/), "Frame with length 0 failed");

# Too short frame
($l, $c, $p) = $ipod->_unframe_cmd("\xFF\x55\x03\x00\x00");
ok(!defined($l) && ($ipod->error() =~ /Could not unframe data/), "Too short frame failed");

# Invalid checksum
($l, $c, $p) = $ipod->_unframe_cmd("\xFF\x55\x03\x00\x00\x00\x00");
ok(!defined($l) && ($ipod->error() =~ /Invalid checksum/), "Invalid checksum failed");

# Find a message in a string
$ipod->{-inbuf} = "\x00\xFF\x55\x00\xFF\xFF\xFF\x55\x04\x00\x02\x00\x06\xF4";
($c, $l) = $ipod->_message_in_buffer();
ok(defined($l) && ($c == 6) && ($l == 8), "Found message in buffer");

# Return message from a string
$ipod->{-inbuf} = "\x00\xFF\x55\x00\xFF\xFF\xFF\x55\x04\x00\x02\x00\x06\xF4\x00";
$m = $ipod->_message();
ok(defined($m) && ($ipod->{-inbuf} eq "\x00"), "Retrieved message from buffer");

# Return two messages from buffer
$ipod->{-inbuf} = "\xffU\x04\x00\x02\x00\x13\xe7\xffU\x02\x00\x14\xea";
$m = $ipod->_message();
subtest "First message" => sub {
    ok(defined($m), "Message received");
    is($m, "\xffU\x04\x00\x02\x00\x13\xe7");
};
$m = $ipod->_message();
subtest "Second message" => sub {
    ok(defined($m), "Message received");
    is($m, "\xffU\x02\x00\x14\xea");
};
