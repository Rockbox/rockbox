/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2009 by Karl Kurbjun  
 *   $Id$
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdio.h>
#include <errno.h>
#include <inttypes.h>
#include <string.h>
#include "mr500.h"

/* This is the patch necessary for the SVG exploit (decrypted) */
struct patch_single hack[] = {  {0x29B28, 0xE12FFF30},
                                {0x2F960, 0xE12FFF30} };

static void usage(void)
{
    printf( "Usage: mk500boot <options> <input file> [output file]\n"
            "options:\n"
            "\t-decrypt   Decrypt the input file and save the output file\n"
            "\t-encrypt   Encrypt the input file and save the output file\n"
            "\t-patch     Patch the input file with the SVF hack\n\n");

    exit(1);
}

/* This is a fake flag that is used to let the tool know what's up */
#define HEADER_DECRYPTED 0x8000

void display_header(struct olympus_header *header) {
    printf("Magic Name: \t%s\n",        header->magic_name);
    printf("Unknown: \t0x%08hX\n",      header->unknown);
    printf("Header Length: \t0x%04X\n", header->header_length);
    printf("Flags: \t\t0x%04X\n",       header->flags);
    printf("Unknonwn Zeros: 0x%08X\n",  header->unknown_zeros);
    printf("Image Length: \t0x%08X\n",  header->image_length);
}

/* This is a demonstration of the encryption and decryption process.
 *  It patches a FW image to include the SVG exploit.
 */
int main (int argc, char *argv[]) {
    uint32_t checksum;
    uint32_t stored_crc;
    
    enum operations {
        decrypt,
        encrypt,
        patch
    } operation;
    
    char *encrypt_file;
    char *decrypt_file;

    struct olympus_header header;

    if (argc < 2) {
        usage();
        return -1;
    }
    
    if(!strcmp(argv[1], "-decrypt")) {
        if(argc < 3) {
            usage();
            return -1;
        }
        encrypt_file=argv[2];
        decrypt_file=argv[3];
        operation = decrypt;
    } else if(!strcmp(argv[1], "-encrypt")) {
        if(argc < 3) {
            usage();
            return -1;
        }
        decrypt_file = argv[2];
        encrypt_file = argv[3];
        operation = encrypt;
    } else if(!strcmp(argv[1], "-patch")) {
        decrypt_file = argv[2];
        encrypt_file = argv[3];
        operation = patch;
    } else {
        return -1;
    }
    
    /* Initialize encryption/decryption routine */
    mr500_init();
    
    if(operation == decrypt) {
        /* Read in the header of the encrypted file */
        if(mr500_read_header(encrypt_file, &header) < 0 ) {
            printf("ERROR: Unable to read header: %s\n", strerror(errno));
            return -1;
        }
    
        /* Read CRC of encrypted file */
        if(mr500_read_crc(encrypt_file, 
                header.header_length+header.image_length, &stored_crc) < 0 ) {
            printf("ERROR: Unable to read CRC: %s\n", strerror(errno));
            return -1;
        }

        /* Display the header information */
        printf("File format:\n");
        
        printf("*****Header*****\n");
        display_header(&header);
        printf("****************\n\n");
    
        printf("*****Image******\n\n");
    
        printf("*****Footer*****\n");
        printf("Checksum: \t0x%08X\n",      stored_crc);
        printf("****************\n\n");
    
        printf("Writing Decrypted file...\n");
    
        /*********************************************************************
        *  Save a decrypted file
        **********************************************************************/
     
        /* Check to make sure this is a encrypted file (bogus flag not set) */
        if(header.flags & HEADER_DECRYPTED) {
            printf("ERROR: This appears to be a decrypted file! Quitting\n");
            return -1;
        }
        
        /* Check to make sure MAGIC string matches expected*/
        if(strncmp((char *)header.magic_name, "OIMCFWUP", 8)) {
            printf("ERROR: Magic string does not match expected! Quitting\n");
            return -1;
        }
     
        /* Set a bogus flag to let the tool know that this is a decrypted file*/
        header.flags |= HEADER_DECRYPTED;
     
        /* Start by writing out the header */
        if(mr500_save_header(decrypt_file, &header) < 0 ) {
            printf("ERROR: Unable to save header: %s\n", strerror(errno));
            return -1;
        }
    
        /* Read encrypted data and save decrypted data */
        if(mr500_save_data( encrypt_file, decrypt_file, header.header_length,
                    header.image_length, decrypt_array) < 0 ) {
            printf("ERROR: Unable to save decrypted data: %s\n", strerror(errno));
            return -1;
        }
    
        printf("Calculating Checksum...\n");
        /* Calculate CRC of decrypted data */
        if(mr500_calculate_crc( decrypt_file, header.header_length, 
                    header.image_length, &checksum) < 0 ) {
            printf("ERROR: Unable to calculate CRC: %s\n", strerror(errno));
            return -1;
        }
    
        printf("Calculated Checksum: \n\t\t0x%08X\n", checksum);
    
        /* Double check to make sure that the two CRCs match */
        if(checksum!=stored_crc) {
            printf("\tERROR: \tCalculated checksum: \t0x%08X and\n", checksum);
            printf("\t\tStored checksum: \t0x%08X do not match\n", stored_crc);
            return -1;
        } else {
            printf("\tOK: Calculated checksum and stored checksum match.\n");
        }
    
        printf("Saving Checksum...\n");
        /* Save the calculated CRC to the file */
        if(mr500_save_crc(decrypt_file, header.header_length+header.image_length,
                    &checksum) < 0 ) {
            printf("ERROR: Unable to save CRC: %s\n", strerror(errno));
            return -1;
        } 
    
    } else if(operation == patch) {
    
        /**********************************************************************
         *  Patch decryped file with SVG exploit
         **********************************************************************/
        printf("Patching decrypted file.\n");
        
        /* Read in the header of the encrypted file */
        if(mr500_read_header(decrypt_file, &header) < 0 ) {
            printf("ERROR: Unable to read header: %s\n", strerror(errno));
            return -1;
        }
        
        /* Check to make sure this is a decrypted file (bogus flag not set) */
        if(!(header.flags & HEADER_DECRYPTED)) {
            printf("ERROR: This appears to be a encrypted file! Quitting\n");
            return -1;
        }
        
        /* Check to make sure MAGIC string matches expected*/
        if(strncmp((char *)header.magic_name, "OIMCFWUP", 8)) {
            printf("ERROR: Magic string does not match expected! Quitting\n");
            return -1;
        }
        
        printf("File Header:\n");
        display_header(&header);
        
        if(mr500_patch_file (decrypt_file, hack, 2) < 0 ) {
            printf("ERROR: Unable to patch file: %s\n", strerror(errno));
            return -1;
        }
        
        printf("\nCalculating new CRC\n");
        
        /* Calculate the 'CRC' of the patched file */
        if(mr500_calculate_crc( decrypt_file, header.header_length, 
                            header.image_length, &checksum) < 0 ) {
            printf("ERROR: Unable to calculate CRC: %s\n", strerror(errno));
            return -1;
        }
                            
        printf("Calculated CRC: \n\t\t0x%08X\n", checksum);
        /* Store the calculated 'CRC' (not encrypted) */
        if(mr500_save_crc(decrypt_file, header.header_length+header.image_length,
                    &checksum) < 0 ) {
            printf("ERROR: Unable to save CRC: %s\n", strerror(errno));
            return -1;
        } 
        
    } else if(operation == encrypt) {
    
        /**********************************************************************
         *  Save an encrypted file
         **********************************************************************/
        printf("Saving Encrypted file\n");
        
        /* Read in the header of the encrypted file */
        if(mr500_read_header(decrypt_file, &header) < 0 ) {
            printf("ERROR: Unable to read header: %s\n", strerror(errno));
            return -1;
        }
        
        /* Check to make sure this is a decrypted file (bogus flag not set) */
        if(!(header.flags & HEADER_DECRYPTED)) {
            printf("ERROR: This appears to be a encrypted file! Quitting\n");
            return -1;
        }
        
        /* Check to make sure MAGIC string matches expected*/
        if(strncmp((char *)header.magic_name, "OIMCFWUP", 7)) {
            printf("ERROR: Magic string does not match expected! Quitting\n");
            return -1;
        }
        
        /* Remove the bogus flag */
        header.flags &= ~HEADER_DECRYPTED;
        
        printf("File Header:\n");
        display_header(&header);
    
        /* Header is not encrypted, save it */
        if(mr500_save_header(encrypt_file, &header) < 0 ) {
            printf("ERROR: Unable to save header: %s\n", strerror(errno));
            return -1;
        }
        
        /* Read CRC of decrypted file */
        if(mr500_read_crc(decrypt_file, 
                header.header_length+header.image_length, &stored_crc) < 0 ) {
            printf("ERROR: Unable to read CRC: %s\n", strerror(errno));
            return -1;
        }
    
        /* Calculate the 'CRC' of the decrypted data */
        if(mr500_calculate_crc( decrypt_file, header.header_length, 
                            header.image_length, &checksum) < 0 ) {
            printf("ERROR: Unable to calculate CRC: %s\n", strerror(errno));
            return -1;
        }
        
        if(stored_crc != checksum) {
            printf("\nERROR: Stored and calculated checksums do not match!\n"
                    "\tFile has been improperly modified. Quitting\n");
            return -1;
        }
        
        printf("Encrypting data...\n");
        
        /* Write the encrypted data to a file */
        if(mr500_save_data( decrypt_file, encrypt_file, header.header_length,
                    header.image_length, encrypt_array) < 0 ) {
            printf("ERROR: Unable to save encrypted data: %s\n", strerror(errno));
            return -1;
        }

        printf("Saving CRC\n");
    
        /* Store the calculated 'CRC' (not encrypted) */
        if(mr500_save_crc(encrypt_file, header.header_length+header.image_length,
                    &checksum) < 0 ) {
            printf("ERROR: Unable to save CRC: %s\n", strerror(errno));
            return -1;
        } 
                    
        printf("File sucesfully encrypted!\n");
    }
    
    printf("Done\n");
    return 0;
}

