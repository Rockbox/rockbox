#!/usr/bin/perl

# A very simple load balancing script:
# If more than $nlim hits in under $tlim seconds, redirect to $mirror.
#
# 2002-01-24 Björn Stenberg <bjorn@haxx.se>

# redirect is triggered by more than:
$nlim = 10; # accesses in...
$tlim = 10; # seconds
$mirror = "http://rockbox.sourceforge.net/rockbox/";

open FILE, "+<.load" or die "Can't open .load: $!";
flock FILE, LOCK_EX;
@a = <FILE>;
if ( scalar @a > $nlim ) {
    $first = shift @a;
}
else {
    $first = $a[0];
}
$now = time();
@a = ( @a, "$now\n" );
truncate FILE, 0;
seek FILE, 0, 0;
for ( @a ) {
    print FILE $_;
}
flock FILE, LOCK_UN;
close FILE;

$diff = $now - $first;
if ( $diff < $tlim ) {
    print "Location: $mirror\n\n";
}
else {
    print "Content-Type: text/html\n\n";
    open FILE, "<main.html" or die "Can't open main.html: $!\n";
    print <FILE>;
    close FILE;
}
