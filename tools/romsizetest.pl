#!/usr/bin/perl

#
# Check that the given file is smaller than the given size and if not, return
# an error code. Used to verify that the rombox.ucl file fits on the particular
# model you build for.

sub filesize {
    my ($filename)=@_;
    my ($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size,
        $atime,$mtime,$ctime,$blksize,$blocks)
        = stat($filename);
    return $size;
}

my $romsize = 256*1024; # 256 KB

my $romstart = $ARGV[0];

if($romstart =~ /^0x(.*)/i) {
    $romstart = hex($romstart);
}


my $max = $romsize - $romstart;

my $file = filesize($ARGV[1]);

if($file > $max ) {
    printf "Output is %d bytes larger than max ($max)\n", $file-$max;
    exit 1;
}
