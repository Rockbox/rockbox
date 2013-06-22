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

# IdentifyDeviceLingoes(lingos=0x01, options=0x00, deviceid=0x00)
# We expect an ACK OK message as response
$m = $ipod->sendmsg(0x00, 0x13, "\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00");
ok($m == 1, "IdentifyDeviceLingoes(lingos=0x01, options=0x00, deviceid=0x00) sent");
($l, $c, $p) = $ipod->recvmsg();
subtest "ACK OK" => sub {
    ok(defined($l), "Response received");
    is($l, 0x00, "Response lingo");
    is($c, 0x02, "Response command");
    is($p, "\x00\x13", "Response payload");
};

# Empty the buffer
$ipod->emptyrecv();

# ContextButtonStatus(0x00)
# We expect an ACK Bad Parameter message as response
$m = $ipod->sendmsg(0x02, 0x00, "\x00");
ok($m == 1, "ContextButtonStatus(0x00)");
($l, $c, $p) = $ipod->recvmsg();
subtest "ACK Bad Parameter" => sub {
    ok(defined($l), "Response received");
    is($l, 0x02, "Response lingo");
    is($c, 0x01, "Response command");
    is($p, "\x04\x00", "Response payload");
};

# Empty the buffer
$ipod->emptyrecv();

# Identify(lingo=0x00)
# We expect no response (timeout)
$m = $ipod->sendmsg(0x00, 0x01, "\x00");
ok($m == 1, "Identify(lingo=0x00) sent");
($l, $c, $p) = $ipod->recvmsg();
subtest "Timeout" => sub {
    ok(!defined($l), "No response received");
    like($ipod->error(), '/Timeout/', "Timeout reading response");
};

# Empty the buffer
$ipod->emptyrecv();

# ContextButtonStatus(0x00)
# We expect a timeout as response
$m = $ipod->sendmsg(0x02, 0x00, "\x00");
ok($m == 1, "ContextButtonStatus(0x00)");
($l, $c, $p) = $ipod->recvmsg();
subtest "Timeout" => sub {
    ok(!defined($l), "Response received");
    like($ipod->error(), '/Timeout/', "Timeout reading response");
};

# Empty the buffer
$ipod->emptyrecv();

# Identify(lingo=0x02)
# We expect no response (timeout)
$m = $ipod->sendmsg(0x00, 0x01, "\x02");
ok($m == 1, "Identify(lingo=0x02) sent");
($l, $c, $p) = $ipod->recvmsg();
subtest "Timeout" => sub {
    ok(!defined($l), "No response received");
    like($ipod->error(), '/Timeout/', "Timeout reading response");
};

# Empty the buffer
$ipod->emptyrecv();

# ContextButtonStatus(0x00)
# We expect a timeout as response
$m = $ipod->sendmsg(0x02, 0x00, "\x00");
ok($m == 1, "ContextButtonStatus(0x00)");
($l, $c, $p) = $ipod->recvmsg();
subtest "Timeout" => sub {
    ok(!defined($l), "Response received");
    like($ipod->error(), '/Timeout/', "Timeout reading response");
};

# Empty the buffer
$ipod->emptyrecv();

# IdentifyDeviceLingoes(lingos=0x05, options=0x00, deviceid=0x00)
# We expect an ACK OK message as response
$m = $ipod->sendmsg(0x00, 0x13, "\x00\x00\x00\x05\x00\x00\x00\x00\x00\x00\x00\x00");
ok($m == 1, "IdentifyDeviceLingoes(lingos=0x05, options=0x00, deviceid=0x00) sent");
($l, $c, $p) = $ipod->recvmsg();
subtest "ACK OK" => sub {
    ok(defined($l), "Response received");
    is($l, 0x00, "Response lingo");
    is($c, 0x02, "Response command");
    is($p, "\x00\x13", "Response payload");
};

# Empty the buffer
$ipod->emptyrecv();

# RequestLingoProtocolVersion(lingo=0x02)
# We expect a ReturnLingoProtocolVersion packet
$m = $ipod->sendmsg(0x00, 0x0F, "\x02");
ok($m == 1, "RequestLingoProtocolVersion(lingo=0x02) sent");
($l, $c, $p) = $ipod->recvmsg();
subtest "ReturnLingoProtocolVersion" => sub {
    ok(defined($l), "Response received");
    is($l, 0x00, "Response lingo");
    is($c, 0x10, "Response command");
    like($p, "/^\\x02..\$/", "Response payload");
};

# Empty the buffer
$ipod->emptyrecv();

# Send an undefined command
# We expect an ACK Bad Parameter as response
$m = $ipod->sendmsg(0x02, 0xFF);
ok($m == 1, "Undefined command sent");
($l, $c, $p) = $ipod->recvmsg();
subtest "ACK Bad Parameter" => sub {
    ok(defined($l), "Response received");
    is($l, 0x02, "Response lingo");
    is($c, 0x01, "Response command");
    is($p, "\x04\xFF", "Response payload");
};

# Empty the buffer
$ipod->emptyrecv();

# ContextButtonStatus(0x00)
# We expect a timeout as response
$m = $ipod->sendmsg(0x02, 0x00, "\x00");
ok($m == 1, "ContextButtonStatus(0x00)");
($l, $c, $p) = $ipod->recvmsg();
subtest "Timeout" => sub {
    ok(!defined($l), "Response received");
    like($ipod->error(), '/Timeout/', "Timeout reading response");
};

# Empty the buffer
$ipod->emptyrecv();

# Send a too short command
# We expect an ACK Bad Parameter as response
$m = $ipod->sendmsg(0x02, 0x00, "");
ok($m == 1, "Short command sent");
($l, $c, $p) = $ipod->recvmsg();
subtest "ACK Bad Parameter" => sub {
    ok(defined($l), "Response received");
    is($l, 0x02, "Response lingo");
    is($c, 0x01, "Response command");
    is($p, "\x04\x00", "Response payload");
};

# Empty the buffer
$ipod->emptyrecv();

# ImageButtonStatus(0x00)
# We expect an ACK Not Authenticated as response
$m = $ipod->sendmsg(0x02, 0x02, "\x00");
ok($m == 1, "ImageButtonStatus(0x00)");
($l, $c, $p) = $ipod->recvmsg();
subtest "ACK Not Authenticated" => sub {
    ok(defined($l), "Response received");
    is($l, 0x02, "Response lingo");
    is($c, 0x01, "Response command");
    is($p, "\x07\x02", "Response payload");
};

# Empty the buffer
$ipod->emptyrecv();

# VideoButtonStatus(0x00)
# We expect an ACK Not Authenticated as response
$m = $ipod->sendmsg(0x02, 0x03, "\x00");
ok($m == 1, "VideoButtonStatus(0x00)");
($l, $c, $p) = $ipod->recvmsg();
subtest "ACK Not Authenticated" => sub {
    ok(defined($l), "Response received");
    is($l, 0x02, "Response lingo");
    is($c, 0x01, "Response command");
    is($p, "\x07\x03", "Response payload");
};

# Empty the buffer
$ipod->emptyrecv();

# AudioButtonStatus(0x00)
# We expect an ACK Not Authenticated as response
$m = $ipod->sendmsg(0x02, 0x04, "\x00");
ok($m == 1, "AudioButtonStatus(0x00)");
($l, $c, $p) = $ipod->recvmsg();
subtest "ACK Not Authenticated" => sub {
    ok(defined($l), "Response received");
    is($l, 0x02, "Response lingo");
    is($c, 0x01, "Response command");
    is($p, "\x07\x04", "Response payload");
};

# Empty the buffer
$ipod->emptyrecv();
