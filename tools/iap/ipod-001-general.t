use Test::More qw( no_plan );
use strict;

BEGIN { use_ok('Device::iPod'); }
require_ok('Device::iPod');

my $ipod = Device::iPod->new();
my $m;
my ($l, $c, $p);

isa_ok($ipod, 'Device::iPod');

$ipod->{-debug} = 1;
$ipod->open("/dev/ttyUSB0");

$m = $ipod->sendraw("\xFF" x 16); # Wake up and sync
ok($m == 1, "Wakeup sent");

# Empty the buffer
$ipod->emptyrecv();

# Send a command with wrong checksum
# We expect no response (Timeout)
$m = $ipod->sendraw("\xff\x55\x02\x00\x03\x00");
ok($m == 1, "Broken checksum sent");
($l, $c, $p) = $ipod->recvmsg();
subtest "Timeout" => sub {
    ok(!defined($l), "No response received");
    like($ipod->error(), '/Timeout/', "Timeout reading response");
};

# Empty the buffer
$ipod->emptyrecv();

# Send a too short command
# We expect an ACK Bad Parameter as response
$m = $ipod->sendmsg(0x00, 0x13, "");
ok($m == 1, "Short command sent");
($l, $c, $p) = $ipod->recvmsg();
subtest "ACK Bad Parameter" => sub {
    ok(defined($l), "Response received");
    is($l, 0x00, "Response lingo");
    is($c, 0x02, "Response command");
    is($p, "\x04\x13", "Response payload");
};

# Empty the buffer
$ipod->emptyrecv();

# Send an undefined lingo
# We expect a timeout
$m = $ipod->sendmsg(0x1F, 0x00);
ok($m == 1, "Undefined lingo sent");
($l, $c, $p) = $ipod->recvmsg();
subtest "Timeout" => sub {
    ok(!defined($l), "No response received");
    like($ipod->error(), '/Timeout/', "Timeout reading response");
};

# Empty the buffer
$ipod->emptyrecv();

# IdentifyDeviceLingoes(lingos=0x80000011, options=0x00, deviceid=0x00)
# We expect an ACK Command Failed message as response
$m = $ipod->sendmsg(0x00, 0x13, "\x80\x00\x00\x11\x00\x00\x00\x00\x00\x00\x00\x00");
ok($m == 1, "IdentifyDeviceLingoes(lingos=0x80000011, options=0x00, deviceid=0x00) sent");
($l, $c, $p) = $ipod->recvmsg();
subtest "ACK Command Failed" => sub {
    ok(defined($l), "Response received");
    is($l, 0x00, "Response lingo");
    is($c, 0x02, "Response command");
    is($p, "\x02\x13", "Response payload");
};

# Empty the buffer
$ipod->emptyrecv();

# Identify(lingo=0xFF)
# We expect no response (timeout)
$m = $ipod->sendmsg(0x00, 0x01, "\xFF");
ok($m == 1, "Identify(lingo=0xFF) sent");
($l, $c, $p) = $ipod->recvmsg();
subtest "Timeout" => sub {
    ok(!defined($l), "No response received");
    like($ipod->error(), '/Timeout/', "Timeout reading response");
};

# Empty the buffer
$ipod->emptyrecv();

# IdentifyDeviceLingoes(lingos=0x10, options=0x00, deviceid=0x00)
# We expect an ACK Command Failed message as response
$m = $ipod->sendmsg(0x00, 0x13, "\x00\x00\x00\x10\x00\x00\x00\x00\x00\x00\x00\x00");
ok($m == 1, "IdentifyDeviceLingoes(lingos=0x10, options=0x00, deviceid=0x00) sent");
($l, $c, $p) = $ipod->recvmsg();
subtest "ACK Command Failed" => sub {
    ok(defined($l), "Response received");
    is($l, 0x00, "Response lingo");
    is($c, 0x02, "Response command");
    is($p, "\x02\x13", "Response payload");
};

# Empty the buffer
$ipod->emptyrecv();
# IdentifyDeviceLingoes(lingos=0x00, options=0x00, deviceid=0x00)
# We expect an ACK Command Failed message as response
$m = $ipod->sendmsg(0x00, 0x13, "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00");
ok($m == 1, "IdentifyDeviceLingoes(lingos=0x00, options=0x00, deviceid=0x00) sent");
($l, $c, $p) = $ipod->recvmsg();
subtest "ACK Command Failed" => sub {
    ok(defined($l), "Response received");
    is($l, 0x00, "Response lingo");
    is($c, 0x02, "Response command");
    is($p, "\x02\x13", "Response payload");
};

# Empty the buffer
$ipod->emptyrecv();

# RequestLingoProtocolVersion(lingo=0xFF)
# We expect an ACK Bad Parameter message as response
$m = $ipod->sendmsg(0x00, 0x0F, "\xFF");
ok($m == 1, "RequestLingoProtocolVersion(lingo=0xFF) sent");
($l, $c, $p) = $ipod->recvmsg();
subtest "ACK Bad Parameter" => sub {
    ok(defined($l), "Response received");
    is($l, 0x00, "Response lingo");
    is($c, 0x02, "Response command");
    is($p, "\x04\x0F", "Response payload");
};

# Empty the buffer
$ipod->emptyrecv();
