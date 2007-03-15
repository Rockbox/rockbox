#!/usr/bin/perl

# feed this script the output from svn update and it will tell if a rebuild
# is needed/wanted
my $change;
while(<STDIN>) {
    if(/^([A-Z]+)  *(.*)/) {
        my ($w, $path) = ($1, $2);
        if($path !~ /^(rbutil|manual)/) {
            $change++;
        }
    }
}

print "rebuild!\n" if($change);
