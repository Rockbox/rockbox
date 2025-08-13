#!/usr/bin/env python

import sys
import struct
import usb


class rbpms:
  def __init__(self):
    self.handle = usb.core.find(idVendor=0xffff, idProduct=0xa112)
    if self.handle is None:
        raise Exception("Could not find specified device")
    self.handle.set_configuration()
    print("Connected to Rockbox Post-Mortem Stub on iPod Nano 2G")


  @staticmethod
  def __myprint(data):
    sys.stdout.write(data)
    sys.stdout.flush()


  def read(self, offset, size):
    if offset & 15 != 0:
      raise Exception("Unaligned data read!")

    data = ""

    while True:
      blocklen = size
      if blocklen == 0: break
      if blocklen > 256 * 1024: blocklen = 256 * 1024
      self.handle.write(4, struct.pack("<II", offset, blocklen), 0, 100)
      block = struct.unpack("%ds" % blocklen, self.handle.read(0x83, blocklen, 0, 1000))[0]
      offset += blocklen
      data += block
      size -= blocklen

    return data


  def download(self, offset, size, file):
    self.__myprint("Downloading 0x%x bytes from 0x%8x to %s..." % (size, offset, file))
    f = open(file, "wb")

    while True:
      blocklen = size
      if blocklen == 0: break
      if blocklen > 2048 * 1024: blocklen = 2048 * 1024
      f.write(self.read(offset, blocklen))
      offset += blocklen
      size -= blocklen
      self.__myprint(".")

    self.__myprint(" done\n")
