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

# RequestRemoteUIMode
# We expect an ACK Bad Parameter as response, as we have not
# negotiated lingo 0x04
$m = $ipod->sendmsg(0x00, 0x03);
ok($m == 1, "RequestRemoteUIMode sent");
($l, $c, $p) = $ipod->recvmsg();
subtest "ACK Bad Parameter" => sub {
    ok(defined($l), "Response received");
    is($l, 0x00, "Response lingo");
    is($c, 0x02, "Response command");
    is($p, "\x04\x03", "Response payload");
};

# Empty the buffer
$ipod->emptyrecv();

# EnterRemoteUIMode
# We expect an ACK Bad Parameter as response, as we have not
# negotiated lingo 0x04
$m = $ipod->sendmsg(0x00, 0x05);
ok($m == 1, "EnterRemoteUIMode sent");
($l, $c, $p) = $ipod->recvmsg();
subtest "ACK Bad Parameter" => sub {
    ok(defined($l), "Response received");
    is($l, 0x00, "Response lingo");
    is($c, 0x02, "Response command");
    is($p, "\x04\x05", "Response payload");
};

# Empty the buffer
$ipod->emptyrecv();

# ExitRemoteUIMode
# We expect an ACK Bad Parameter as response, as we have not
# negotiated lingo 0x04
$m = $ipod->sendmsg(0x00, 0x06);
ok($m == 1, "ExitRemoteUIMode sent");
($l, $c, $p) = $ipod->recvmsg();
subtest "ACK Bad Parameter" => sub {
    ok(defined($l), "Response received");
    is($l, 0x00, "Response lingo");
    is($c, 0x02, "Response command");
    is($p, "\x04\x06", "Response payload");
};

# Empty the buffer
$ipod->emptyrecv();

# RequestiPodName
# We expect a ReturniPodName packet
$m = $ipod->sendmsg(0x00, 0x07);
ok($m == 1, "RequestiPodName sent");
($l, $c, $p) = $ipod->recvmsg();
subtest "ReturniPodName" => sub {
    ok(defined($l), "Response received");
    is($l, 0x00, "Response lingo");
    is($c, 0x08, "Response command");
    like($p, "/^[^\\x00]*\\x00\$/", "Response payload");
};

# Empty the buffer
$ipod->emptyrecv();

# RequestiPodSoftwareVersion
# We expect a ReturniPodSoftwareVersion packet
$m = $ipod->sendmsg(0x00, 0x09);
ok($m == 1, "RequestiPodSoftwareVersion sent");
($l, $c, $p) = $ipod->recvmsg();
subtest "ReturniPodSoftwareVersion" => sub {
    ok(defined($l), "Response received");
    is($l, 0x00, "Response lingo");
    is($c, 0x0A, "Response command");
    like($p, "/^...\$/", "Response payload");
};

# Empty the buffer
$ipod->emptyrecv();

# RequestiPodSerialNumber
# We expect a ReturniPodSerialNumber packet
$m = $ipod->sendmsg(0x00, 0x0B);
ok($m == 1, "RequestiPodSerialNumber sent");
($l, $c, $p) = $ipod->recvmsg();
subtest "ReturniPodSerialNumber" => sub {
    ok(defined($l), "Response received");
    is($l, 0x00, "Response lingo");
    is($c, 0x0C, "Response command");
    like($p, "/^[^\\x00]*\\x00\$/", "Response payload");
};

# Empty the buffer
$ipod->emptyrecv();

# RequestiPodModelNum
# We expect a ReturniPodModelNum packet
$m = $ipod->sendmsg(0x00, 0x0D);
ok($m == 1, "RequestiPodModelNum sent");
($l, $c, $p) = $ipod->recvmsg();
subtest "ReturniPodModelNum" => sub {
    ok(defined($l), "Response received");
    is($l, 0x00, "Response lingo");
    is($c, 0x0E, "Response command");
    like($p, "/^....[^\\x00]*\\x00\$/", "Response payload");
};

# Empty the buffer
$ipod->emptyrecv();

# RequestLingoProtocolVersion(lingo=0x00)
# We expect a ReturnLingoProtocolVersion packet
$m = $ipod->sendmsg(0x00, 0x0F, "\x00");
ok($m == 1, "RequestLingoProtocolVersion(lingo=0x00) sent");
($l, $c, $p) = $ipod->recvmsg();
subtest "ReturnLingoProtocolVersion" => sub {
    ok(defined($l), "Response received");
    is($l, 0x00, "Response lingo");
    is($c, 0x10, "Response command");
    like($p, "/^\\x00..\$/", "Response payload");
};

# Empty the buffer
$ipod->emptyrecv();

# IdentifyDeviceLingoes(lingos=0x11, options=0x00, deviceid=0x00)
# We expect an ACK OK message as response
$m = $ipod->sendmsg(0x00, 0x13, "\x00\x00\x00\x11\x00\x00\x00\x00\x00\x00\x00\x00");
ok($m == 1, "IdentifyDeviceLingoes(lingos=0x11, options=0x00, deviceid=0x00) sent");
($l, $c, $p) = $ipod->recvmsg();
subtest "ACK OK" => sub {
    ok(defined($l), "Response received");
    is($l, 0x00, "Response lingo");
    is($c, 0x02, "Response command");
    is($p, "\x00\x13", "Response payload");
};

# Empty the buffer
$ipod->emptyrecv();

# RequestRemoteUIMode
# We expect an ReturnRemoteUIMode packet specifying standard mode
$m = $ipod->sendmsg(0x00, 0x03);
ok($m == 1, "RequestRemoteUIMode sent");
($l, $c, $p) = $ipod->recvmsg();
subtest "ReturnRemoteUIMode" => sub {
    ok(defined($l), "Response received");
    is($l, 0x00, "Response lingo");
    is($c, 0x04, "Response command");
    is($p, "\x00", "Response payload");
};

# Empty the buffer
$ipod->emptyrecv();

# EnterRemoteUIMode
# We expect an ACK Pending packet, followed by an ACK OK packet
$m = $ipod->sendmsg(0x00, 0x05);
ok($m == 1, "EnterRemoteUIMode sent");
($l, $c, $p) = $ipod->recvmsg();
subtest "ACK Pending" => sub {
    ok(defined($l), "Response received");
    is($l, 0x00, "Response lingo");
    is($c, 0x02, "Response command");
    like($p, "/^\\x06\\x05/", "Response payload");
};
($l, $c, $p) = $ipod->recvmsg();
subtest "ACK OK" => sub {
    ok(defined($l), "Response received");
    is($l, 0x00, "Response lingo");
    is($c, 0x02, "Response command");
    is($p, "\x00\x05", "Response payload");
};

# Empty the buffer
$ipod->emptyrecv();

# RequestRemoteUIMode
# We expect an ReturnRemoteUIMode packet specifying extended mode
$m = $ipod->sendmsg(0x00, 0x03);
ok($m == 1, "RequestRemoteUIMode sent");
($l, $c, $p) = $ipod->recvmsg();
subtest "ReturnRemoteUIMode" => sub {
    ok(defined($l), "Response received");
    is($l, 0x00, "Response lingo");
    is($c, 0x04, "Response command");
    isnt($p, "\x00", "Response payload");
};

# Empty the buffer
$ipod->emptyrecv();

# ExitRemoteUIMode
# We expect an ACK Pending packet, followed by an ACK OK packet
$m = $ipod->sendmsg(0x00, 0x06);
ok($m == 1, "ExitRemoteUIMode sent");
($l, $c, $p) = $ipod->recvmsg();
subtest "ACK Pending" => sub {
    ok(defined($l), "Response received");
    is($l, 0x00, "Response lingo");
    is($c, 0x02, "Response command");
    like($p, "/^\\x06\\x06/", "Response payload");
};
($l, $c, $p) = $ipod->recvmsg();
subtest "ACK OK" => sub {
    ok(defined($l), "Response received");
    is($l, 0x00, "Response lingo");
    is($c, 0x02, "Response command");
    is($p, "\x00\x06", "Response payload");
};

# Empty the buffer
$ipod->emptyrecv();

# RequestRemoteUIMode
# We expect an ReturnRemoteUIMode packet specifying standard mode
$m = $ipod->sendmsg(0x00, 0x03);
ok($m == 1, "RequestRemoteUIMode sent");
($l, $c, $p) = $ipod->recvmsg();
subtest "ReturnRemoteUIMode" => sub {
    ok(defined($l), "Response received");
    is($l, 0x00, "Response lingo");
    is($c, 0x04, "Response command");
    is($p, "\x00", "Response payload");
};

# Empty the buffer
$ipod->emptyrecv();

# Send an undefined command
# We expect an ACK Bad Parameter as response
$m = $ipod->sendmsg(0x00, 0xFF);
ok($m == 1, "Undefined command sent");
($l, $c, $p) = $ipod->recvmsg();
subtest "ACK Bad Parameter" => sub {
    ok(defined($l), "Response received");
    is($l, 0x00, "Response lingo");
    is($c, 0x02, "Response command");
    is($p, "\x04\xFF", "Response payload");
};

# Empty the buffer
$ipod->emptyrecv();
