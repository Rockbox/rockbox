#!/usr/bin/env python
import sys
import librbpms


def usage():
  print ""
  print "Please provide a command and (if needed) parameters as command line arguments"
  print ""
  print "Available commands:"
  print ""
  print "  download <address> <size> <file>"
  print "    Downloads <size> bytes of data from the specified address on the device,"
  print "    and stores it in the specified file."
  print ""
  print "All numbers are hexadecimal!"
  exit(2)


def parsecommand(dev, argv):
  if len(argv) < 2: usage()

  elif argv[1] == "download":
    if len(argv) != 5: usage()
    dev.download(int(argv[2], 16), int(argv[3], 16), argv[4])

  else: usage()


dev = librbpms.rbpms()
parsecommand(dev, sys.argv)
