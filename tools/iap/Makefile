default: test

test:
	perl -MTest::Harness -e '$$Test::Harness::verbose=1; runtests @ARGV' ipod*.t

moduletest:
	perl -MTest::Harness -e '$$Test::Harness::verbose=1; runtests @ARGV' device-ipod.t
