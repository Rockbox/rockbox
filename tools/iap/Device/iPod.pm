package Device::iPod;

use Device::SerialPort;
use POSIX qw(isgraph);
use strict;

sub new {
    my $class = shift;
    my $port = shift;
    my $self = {};
    my $s;

    $self->{-serial} = undef;
    $self->{-inbuf} = '';
    $self->{-error} = undef;
    $self->{-baudrate} = 57600;
    $self->{-debug} = 0;

    return bless($self, $class);
}

sub open {
    my $self = shift;
    my $port = shift;

    $self->{-serial} = new Device::SerialPort($port);
    unless(defined($self->{-serial})) {
        $self->{-error} = $!;
        return undef;
    }

    $self->{-serial}->parity('none');
    $self->{-serial}->databits(8);
    $self->{-serial}->stopbits(1);
    $self->{-serial}->handshake('none');
    return $self->baudrate($self->{-baudrate});
}

sub baudrate {
    my $self = shift;
    my $baudrate = shift;

    if ($baudrate < 1) {
        $self->{-error} = "Invalid baudrate";
        return undef;
    }

    $self->{-baudrate} = $baudrate;
    if (defined($self->{-serial})) {
        $self->{-serial}->baudrate($baudrate);
    }

    return 1;
}

sub sendmsg {
    my $self = shift;
    my $lingo = shift;
    my $command = shift;
    my $data = shift || '';

    return $self->_nosetup() unless(defined($self->{-serial}));

    if (($lingo < 0) || ($lingo > 255)) {
        $self->{-error} = 'Invalid lingo';
        return undef;
    }

    if ($command < 0) {
        $self->{-error} = 'Invalid command';
        return undef;
    }

    if ($lingo == 4) {
        if ($command > 0xffff) {
            $self->{-error} = 'Invalid command';
            return undef;
        }
        return $self->_send($self->_frame_cmd(pack("Cn", $lingo, $command) . $data));
    } else {
        if ($command > 0xff) {
            $self->{-error} = 'Invalid command';
            return undef;
        }
        return $self->_send($self->_frame_cmd(pack("CC", $lingo, $command) . $data));
    }
}

sub sendraw {
    my $self = shift;
    my $data = shift;

    return $self->_nosetup() unless(defined($self->{-serial}));

    return $self->_send($data);
}

sub recvmsg {
    my $self = shift;
    my $m;
    my @m;

    return $self->_nosetup() unless(defined($self->{-serial}));

    $m = $self->_fillbuf();
    unless(defined($m)) {
        # Error was set by lower levels
        return wantarray?():undef;
    }

    printf("Fetched %s\n", $self->_hexstring($m)) if $self->{-debug};

    @m = $self->_unframe_cmd($m);

    unless(@m) {
        return undef;
    }

    if (wantarray()) {
        return @m;
    } else {
        return {-lingo => $m[0], -cmd => $m[1], -payload => $m[2]};
    }
}

sub emptyrecv {
    my $self = shift;
    my $m;

    while ($m = $self->_fillbuf()) {
        printf("Discarded %s\n", $self->_hexstring($m)) if (defined($m) && $self->{-debug});
    }
}

sub error {
    my $self = shift;

    return $self->{-error};
}

sub _nosetup {
    my $self = shift;

    $self->{-error} = 'Serial port not setup';
    return undef;
}

sub _frame_cmd {
    my $self = shift;
    my $data = shift;
    my $l = length($data);
    my $csum;

    if ($l > 0xffff) {
        $self->{-error} = 'Command too long';
        return undef;
    }

    if ($l > 255) {
        $data = pack("Cn", 0, length($data)) . $data;
    } else {
        $data = pack("C", length($data)) . $data;
    }

    foreach (unpack("C" x length($data), $data)) {
        $csum += $_;
    }
    $csum &= 0xFF;
    $csum = 0x100 - $csum;

    return "\xFF\x55" . $data . pack("C", $csum);
}

sub _unframe_cmd {
    my $self = shift;
    my $data = shift;
    my $payload = '';
    my ($count, $length, $csum);
    my $state = 0;
    my $c;
    my ($lingo, $cmd);

    return () unless(defined($data));

    foreach $c (unpack("C" x length($data), $data)) {
        if ($state == 0) {
            # Wait for sync
            next unless($c == 255);
            $state = 1;
        } elsif ($state == 1) {
            # Wait for sop
            next unless($c == 85);
            $state = 2;
        } elsif ($state == 2) {
            # Length (short frame)
            $csum = $c;
            if ($c == 0) {
                # Large frame
                $state = 3;
            } else {
                $state = 5;
            }
            $length = $c;
            $count = 0;
            next;
        } elsif ($state == 3) {
            # Large frame, hi
            $csum += $c;
            $length = ($c << 8);
            $state = 4;
            next;
        } elsif ($state == 4) {
            # Large frame, lo
            $csum += $c;
            $length |= $c;
            if ($length == 0) {
                $self->{-error} = 'Length is 0';
                return ();
            }
            $state = 5;
            next;
        } elsif ($state == 5) {
            # Data bytes
            $csum += $c;
            $payload .= chr($c);
            $count += 1;
            if ($count == $length) {
                $state = 6;
            }
        } elsif ($state == 6) {
            # Checksum byte
            $csum += $c;
            if (($csum & 0xFF) != 0) {
                $self->{-error} = 'Invalid checksum';
                return ();
            }
            $state = 7;
            last;
        } else {
            $self->{-error} = 'Invalid state';
            return ();
        }
    }

    # If we get here, we either have data or not. Check.
    if ($state != 7) {
        $self->{-error} = 'Could not unframe data';
        return ();
    }

    $lingo = unpack("C", $payload);
    if ($lingo == 4) {
        return unpack("Cna*", $payload);
    } else {
        return unpack("CCa*", $payload);
    }
}

sub _send {
    my $self = shift;
    my $data = shift;
    my $l = length($data);
    my $c;

    printf("Sending %s\n", $self->_hexstring($data)) if $self->{-debug};

    $c = $self->{-serial}->write($data);
    unless(defined($c)) {
        $self->{-error} = 'write failed';
        return undef;
    }

    if ($c != $l) {
        $self->{-error} = 'incomplete write';
        return undef;
    }

    return 1;
}

sub _fillbuf {
    my $self = shift;
    my $timeout = shift || 2;
    my $to;

    # Read from the port until we have a complete message in the buffer,
    # or until we haven't read any new data for $timeout seconds, whatever
    # comes first.

    $to = $timeout;

    while(!$self->_message_in_buffer() && $to > 0) {
        my ($c, $s) = $self->{-serial}->read(255);
        if ($c == 0) {
            # No data read
            select(undef, undef, undef, 0.1);
            $to -= 0.1;
        } else {
            $self->{-inbuf} .= $s;
            $to = $timeout;
        }
    }
    if ($self->_message_in_buffer()) {
        # There is a complete message in the buffer
        return $self->_message();
    } else {
        # Timeout occured
        $self->{-error} = 'Timeout reading from port';
        return undef;
    }
}

sub _message_in_buffer {
    my $self = shift;
    my $sp = 0;
    my $i;

    $i = index($self->{-inbuf}, "\xFF\x55", $sp);
    while ($i != -1) {
        my $header;
        my $len;
        my $large = 0;


        $header = substr($self->{-inbuf}, $i, 3);
        if (length($header) != 3) {
            # Runt frame
            return ();
        }
        $len = unpack("x2C", $header);
        if ($len == 0) {
            # Possible large frame
            $header = substr($self->{-inbuf}, $i, 5);
            if (length($header) != 5) {
                # Runt frame
                return ();
            }
            $large = 1;
            $len = unpack("x3n", $header);
        }

        # Add framing, checksum and length
        $len = $len+3+($large?3:1);

        if (length($self->{-inbuf}) < ($i+$len)) {
            # Buffer too short to hold rest of frame. Try again.
            $sp = $i+1;
            $i = index($self->{-inbuf}, "\xFF\x55", $sp);
        } else {
            return ($i, $len);
        }
    }

    # No complete message found
    return ();
}


sub _message {
    my $self = shift;
    my $start;
    my $len;
    my $m;

    # Return the first complete message in the buffer, removing the message
    # and everything before it from the buffer.
    ($start, $len) = $self->_message_in_buffer();
    unless(defined($start)) {
        $self->{-error} = 'No complete message in buffer';
        return undef;
    }
    $m = substr($self->{-inbuf}, $start, $len);
    $self->{-inbuf} = substr($self->{-inbuf}, $start+$len);

    return $m;
}

sub _hexstring {
    my $self = shift;
    my $s = shift;

    return join("", map { (($_ == 0x20) || isgraph(chr($_)))?chr($_):sprintf("\\x%02x", $_) }
            unpack("C" x length($s), $s));
}

1;
