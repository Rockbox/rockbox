#ifndef __NAND_X1000_ERR_H__
#define __NAND_X1000_ERR_H__

/* Error codes which can be returned by the nand-x1000 API. These codes are
 * also used by host-side tools, so we define them here to avoid polluting
 * the namespace with useless X1000 APIs. */
#define NANDERR_CHIP_UNSUPPORTED  (-1)
#define NANDERR_WRITE_PROTECTED   (-2)
#define NANDERR_UNALIGNED_ADDRESS (-3)
#define NANDERR_UNALIGNED_LENGTH  (-4)
#define NANDERR_READ_FAILED       (-5)
#define NANDERR_ECC_FAILED        (-6)
#define NANDERR_ERASE_FAILED      (-7)
#define NANDERR_PROGRAM_FAILED    (-8)
#define NANDERR_COMMAND_FAILED    (-9)
#define NANDERR_OTHER             (-99)

/* TEMPORARY -- compatibility hack for jztool's sake.
 * This will go away once the new bootloader gets merged */
#define NAND_SUCCESS              0
#define NAND_ERR_UNKNOWN_CHIP     NANDERR_CHIP_UNSUPPORTED
#define NAND_ERR_UNALIGNED        NANDERR_UNALIGNED_ADDRESS
#define NAND_ERR_WRITE_PROTECT    NANDERR_WRITE_PROTECTED
#define NAND_ERR_CONTROLLER       NANDERR_OTHER
#define NAND_ERR_COMMAND          NANDERR_COMMAND_FAILED

#endif /* __NAND_X1000_ERR_H__ */
