/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2009 by Karl Kurbjun 
 *      based on work by Shirour: 
 *          http://www.mrobe.org/forum/viewtopic.php?f=6&t=2176
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
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/stat.h>
#include "mr500.h"

/* Notes about firmware:
 *  These notes are based on the work and observations of Shirour on the M:Robe
 *  forums.
 *
 *  The firmware for the M:Robe has basic encryption on it.  The data is XORed
 *  and scrambled. The mr500_save_data function provides an implemenation of the
 *  encryption/decryption.
 *
 *  When a firmware update is done the "{#4F494D4346575550#}" folder is stored
 *  in the system folder on the player.  The "{#4F494D4346575550#}" should only
 *  contain the encrypted N5002-BD.BIN file.  At the end of a firmware update
 *  the "{#4F494D4346575550#}" folder and it's contents are removed from the
 *  player.
 *
 *  An interesting note is that the name "{#4F494D4346575550#}" is actually the
 *  Hex representation of the magic text found at the beginning of the firmware
 *  image "OIMCFWUP".
 */

/* These two arrays are used for descrambling or scrambling the data */
int decrypt_array[16]={2, 0, 3, 1, 5, 7, 4, 6, 11, 10, 9, 8, 14, 12, 13, 15};
int encrypt_array[16];

/* mr500_patch_file: This function modifies the specified file with the patches
 *  struct.
 * 
 *  Parameters:
 *      filename:       text filename
 *      patches:        pointer to structure array of patches
 *      num_patches:    number of patches to apply (applied in reverse order)
 *
 *  Returns:
 *      Returns 0 if there was no error, -1 if there was an error
 */
int mr500_patch_file(char *filename, struct patch_single *patches,
        int num_patches) {
    int         fdo;
    int         ret=0;
    uint32_t    endian_int;
    
    /* Open the file write only. */
    fdo = open(filename, O_WRONLY);
    
    if(fdo<0) {
        ret=-1;
    }
    
    while(num_patches--) {
        /* seek to patch offset */
        if(lseek(fdo, patches[num_patches].offset, SEEK_SET) 
                != patches[num_patches].offset) {
            ret=-1;
            break;
        }
        
        /* Make sure patch is written in little endian format */
        endian_int = htole32(patches[num_patches].value);
        
        /* Write the patch value to the file */
        if(write(fdo, (void *) &endian_int, sizeof(endian_int)) < 0) {
            ret = -1;
            break;
        }
    }
    
    /* Close the file and check for errors */
    if(close (fdo) < 0) {
        ret = -1;
    }
    
    return ret;
}

/* mr500_save_header: This function saves the Olympus header to the firmware 
 *  image. The values stored in the header are explained above.  Note that this 
 *  will truncate a file.  The header is stored in little endian format.
 * 
 *  Parameters:
 *      filename: text filename
 *      header: pointer to header structure to be saved
 *
 *  Returns:
 *      Returns 0 if there was no error, -1 if there was an error
 */
int mr500_save_header(char *filename, struct olympus_header *header) {
    int fdo;
    int ret=0;
    
    /* Temporary header used for storing the header in little endian. */
    struct olympus_header save;
    
    /* Open the file write only and truncate it.  If it doesn't exist create. */
    fdo = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    
    if(fdo<0) {
        ret=-1;
    }
    
    /* Header is stored at offset 0 (Not really needed) */
    if(lseek(fdo, 0, SEEK_SET) != 0) {
        ret=-1;
    }
    
    /* Convert header to Little Endian */
    memcpy(&save.magic_name, &header->magic_name, 8*sizeof(int8_t));
    save.unknown          = htole32(header->unknown);
    save.header_length    = htole16(header->header_length);
    save.flags            = htole16(header->flags);
    save.unknown_zeros    = htole32(header->unknown_zeros);
    save.image_length     = htole32(header->image_length);

    /* Write the header to the file */
    if(write(fdo, (void *) &save, sizeof(save)) < 0) {
        ret = -1;
    }
    
    /* Close the file and check for errors */
    if(close (fdo) < 0) {
        ret = -1;
    }
    
    return ret;
}

/* mr500_read_header: This function reads the Olympus header and converts it to 
 *  the host endian format. The values stored in the header are explained above. 
 *  The header is stored in little endian format.
 * 
 *  Parameters:
 *      filename: text filename
 *      header: pointer to header structure to store header read from file
 *
 *  Returns:
 *      Returns 0 if there was no error, -1 if there was an error
 */
int mr500_read_header(char *filename, struct olympus_header *header) {
    int fdi;
    int ret = 0;
    
    /* Open the file read only */
    fdi = open(filename, O_RDONLY);

    if(fdi<0) {
        ret=-1;
    }
    
    /* Header is stored at offset 0 (Not really needed) */
    if(lseek(fdi, 0, SEEK_SET) != 0) {
        ret=-1;
    }
    
    /* Read in the header */
    if(read(fdi, (void *) header, sizeof(*header)) < 0) {
        ret = -1;
    }
    
    /* Convert header to system endian */
    header->unknown          = le32toh(header->unknown);
    header->header_length    = le16toh(header->header_length);
    header->flags            = le16toh(header->flags);
    header->unknown_zeros    = le32toh(header->unknown_zeros);
    header->image_length     = le32toh(header->image_length);
    
    /* Close the file and check for errors */
    if(close (fdi) < 0) {
        ret = -1;
    }
    
    return ret;
}

/* mr500_save_crc: This function saves the 'CRC' of the Olympus firmware image.  
 *  Note that the 'CRC' must be calculated on the decrytped image.  It is stored
 *  in little endian.
 * 
 *  Parameters:
 *      filename: text filename
 *      offset: Offset to store the 'CRC' (header size + data size)
 *      crc_value: pointer to crc value to save
 *
 *  Returns:
 *      Returns 0 if there was no error, -1 if there was an error
 */
int mr500_save_crc(char *filename, off_t offset, uint32_t *crc_value) {
    int fdo;
    int ret = 0;
    uint32_t save_crc;
    
    /* Open the file write only */
    fdo = open(filename, O_WRONLY);
    
    if(fdo<0) {
        ret=-1;
    }
    
    /* Seek to offset and check for errors */
    if(lseek(fdo, offset, SEEK_SET) != offset) {
        ret=-1;
    }
    
    /* Convert 'CRC' to little endian from system native endian */
    save_crc = htole32(*crc_value);
    
    /* Write the 'CRC' and check for errors */
    if(write(fdo, (void *) &save_crc, sizeof(unsigned int)) < 0) {
        ret = -1;
    }
    
    /* Close the file and check for errors */
    if(close (fdo) < 0) {
        ret = -1;
    }
    
    return ret;
}

/* mr500_read_crc: This function reads the 'CRC' of the Olympus firmware image.  
 *  Note that the 'CRC' is calculated on the decrytped values.  It is stored
 *  in little endian.
 * 
 *  Parameters:
 *      filename: text filename
 *      offset: Offset to read the 'CRC' (header size + data size)
 *      crc_value: pointer to crc value to save
 *
 *  Returns:
 *      Returns 0 if there was no error, -1 if there was an error
 */
int mr500_read_crc(char *filename, off_t offset, uint32_t *crc_value) {
    int fdi;
    int ret = 0;
    
    /* Open the file read only */
    fdi = open(filename, O_RDONLY);
    
    if(fdi<0) {
        ret = -1;
    }
    
    /* Seek to offset and check for errors */
    if(lseek(fdi, offset, SEEK_SET) != offset) {
        ret=-1;
    }
    
    /* Read in the 'CRC' */
    if(read(fdi, (void *) crc_value, sizeof(uint32_t)) < 0) {
        ret = -1;
    }
    
    /* Convert the 'CRC' from little endian to system native format */
    *crc_value = le32toh(*crc_value);
    
    /* Close the file and check for errors */
    if(close (fdi) < 0) {
        ret = -1;
    }
    
    return ret;
}

/* mr500_calculate_crc: This function calculates the 'CRC' of the Olympus 
 *  firmware image.  Note that the 'CRC' must be calculated on decrytped values.  
 *  It is stored in little endian.
 * 
 *  Parameters:
 *      filename: text filename
 *      offset: Offset to the start of the data (header size)
 *      length: Length of data to calculate
 *      crc_value: pointer to crc value to save
 *
 *  Returns:
 *      Returns 0 if there was no error, -1 if there was an error
 */
int mr500_calculate_crc(  char *filename, off_t offset, unsigned int length, 
                        uint32_t *crc_value){
    uint32_t temp;
    int fdi;
    int ret = 0;
    
    /* Open the file read only */
    fdi = open(filename, O_RDONLY);
    
    if(fdi<0) {
        ret = -1;
    }
    
    /* Seek to offset and check for errors */
    if(lseek(fdi, offset, SEEK_SET) != offset) {
        ret=-1;
    }
    
    /* Initialize the crc_value to make sure this starts at 0 */
    *crc_value = 0;
    /* Run this loop till the entire sum is created */
    do {
        /* Read an integer at a time */
        if(read(fdi, &temp, sizeof(uint32_t)) < 0) {
            ret = -1;
            break;
        }
        
        /* Keep summing the values */
        *crc_value+=temp;
    } while (length-=4);
    
    /* Close the file and check for errors */
    if(close (fdi) < 0) {
        ret = -1;
    }
    
    return ret;
}

/* mr500_save_data: This function encypts or decrypts the Olympus firmware 
 *      image based on the dictionary passed to it.
 * 
 *  Parameters:
 *      source_filename: text filename where data is read from
 *      dest_filename: text filename where data is written to
 *      offset: Offset to the start of the data (header size)
 *      length: Length of data to modify
 *      dictionary: pointer to dictionary used for scrambling
 *
 *  Returns:
 *      Returns 0 if there was no error, -1 if there was an error
 */
int mr500_save_data(
        char *source_filename, char *dest_filename, off_t offset,
        unsigned int length, int* dictionary) {
    int fdi, fdo;
    int ret = 0;
    int i;
    
    /* read_count stores the number of bytes actually read */
    int read_count;
    
    /* read_request stores the number of bytes to be requested */
    int read_request;
    
    /* These two buffers are used for reading data and scrambling or
     *  descrambling
     */
    int8_t buffer_original[16];
    int8_t buffer_modified[16];
    
    /* Open input read only, output write only */
    fdi = open(source_filename, O_RDONLY);
    fdo = open(dest_filename, O_WRONLY);
    
    /* If there was an error loading the files set ret appropriately */
    if(fdi<0 || fdo < 0) {
        ret = -1;
    }
    
    /* Input file: Seek to offset and check for errors */
    if(lseek(fdi, offset, SEEK_SET) != offset) {
        ret=-1;
    }
    
    /* Output file: Seek to offset and check for errors */
    if(lseek(fdo, offset, SEEK_SET) != offset) {
        ret=-1;
    }

    /* Run this loop till size is 0 */
    do {
        /* Choose the amount of data to read - normally in 16 byte chunks, but
         *  when the end of the file is near may be less.
         */
        if( length > sizeof(buffer_original)){
            read_request = sizeof(buffer_original);
        } else {
            read_request = length;
        }
        
        /* Read in the data */
        read_count = read(fdi, (void *) &buffer_original, read_request);
        
        /* If there was an error set the flag and break */
        if(read_count < 0) {
            ret = -1;
            break;
        }

        for(i=0; i<read_count; i++) {
            /* XOR all of the bits to de/encrypt them */
            buffer_original[i] ^= 0xFF;
            /* Handle byte scrambling */
            buffer_modified[dictionary[i]] = buffer_original[i];
        }
        
        /* write the data: If there was an error set the flag and break */
        if(write(fdo, (void *) &buffer_modified, read_count) < 0) {
            ret = -1;
            break;
        }
    } while (length -= read_count);
    
    /* Close the files and check for errors */
    if(close (fdi) < 0) {
        ret = -1;
    }
    if(close (fdo) < 0) {
        ret = -1;
    }
    
    return ret;
}

/* mr500_init: This function initializes the encryption array
 * 
 *  Parameters:
 *      None
 *
 *  Returns:
 *      Returns 0
 */
int mr500_init(void) {
    int i;
    /* Initialize the encryption array */
    for(i=0; i<16; i++) {
        encrypt_array[decrypt_array[i]]=i;
    }
    return 0;
}

