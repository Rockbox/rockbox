while (<STDIN>) {
    chomp;	# strip record separator
    my @Fld = split(' ', $_, 9999);
    if ($a && length($Fld[1])) {
	print "\"$_\",\n";
    }
    if (/Friend/) {
	$a++;
    }
}
