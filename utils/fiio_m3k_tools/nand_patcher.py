#!/usr/bin/python3

import sys

IMAGE_SIZE = 128 * 1024 # image is an 128 KiB erase block
SPL_SIZE   =  12 * 1024 # SPL is at most 12 KiB
BOOT_SIZE  = 102 * 1024 # bootloader at most 102 KiB
BOOT_OFF   =  26 * 1024 # offset of bootloader in image
BOOT_END   = BOOT_OFF+BOOT_SIZE

def patch(in_path, boot_path, spl_path, out_path):
    # Open the input files
    in_file = open(in_path, 'rb')
    boot_file = open(boot_path, 'rb')
    spl_file = open(spl_path, 'rb')

    # Read the data
    in_data = in_file.read()
    boot_data = boot_file.read()
    spl_data = spl_file.read()

    # Close input files
    in_file.close()
    boot_file.close()
    spl_file.close()

    if len(in_data) != IMAGE_SIZE:
        print("error: input image is %d bytes, expected %d" % (len(in_data), IMAGE_SIZE))
        sys.exit(1)

    if len(spl_data) > SPL_SIZE:
        print("error: SPL is %d bytes, maximum is %d" % (len(spl_data), SPL_SIZE))
        sys.exit(1)

    if len(boot_data) > BOOT_SIZE:
        print("error: bootloader is %d bytes, maximum is %d" % (len(boot_data), SPL_SIZE))
        sys.exit(1)

    print('Patching input image %s' % in_path)
    print('- SPL size %d' % len(spl_data))
    print('- Boot size %d' % len(boot_data))

    # Construct output image
    out_data = b''
    out_data += spl_data
    out_data += b'\xff' * (SPL_SIZE - len(spl_data))
    out_data += in_data[SPL_SIZE:BOOT_OFF]
    out_data += boot_data
    out_data += b'\xff' * (BOOT_SIZE - len(boot_data))

    # Sanity check
    assert( len(out_data) == IMAGE_SIZE )

    # Write output
    print('Writing output image %s' % out_path)
    out_file = open(out_path, 'wb')
    out_file.write(out_data)
    out_file.close()


def main():
    if len(sys.argv) != 5:
        print("usage: nand_patcher.py IN_FILE BOOT_FILE SPL_FILE OUT_FILE")
        sys.exit(1)

    patch(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4])

if __name__ == '__main__':
    main()
