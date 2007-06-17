/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * Module: rbutil
 * File: autodetection.cpp
 *
 * Copyright (C) 2008 Dominik Wenger
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "autodetection.h"
#include "bootloaders.h"
/***************************************************
* General autodetection code
****************************************************/


UsbDeviceInfo detectDevicesViaPatchers()
{
    UsbDeviceInfo tempdevice;
    tempdevice.device_index= 0;
    tempdevice.path=wxT("");
    tempdevice.status =0;

    /* scann for ipods */

    struct ipod_t ipod;
    int n = ipod_scan(&ipod);
    if(n == 1)
    {
        wxString temp(ipod.targetname,wxConvUTF8);
        int index = gv->plat_bootloadername.Index(temp);   // use the bootloader names..
        tempdevice.device_index = index;
        /* find mount point if possible */
#if !(defined( __WXMSW__ ) || defined( __DARWIN__))          //linux code
        wxString tmp = resolve_mount_point(wxString(ipod.diskname,wxConvUTF8)+wxT("2"));
        if( tmp != wxT("") )
            tempdevice.path = tmp;
#endif
#if defined( __WXMSW__ )                                 //Windows code
        wxString tmp = guess_mount_point();
        if( tmp != wxT("") )
           tempdevice.path = tmp;
#endif
        return tempdevice;
    }
    else if (n > 1)
    {
        tempdevice.status = TOMANYDEVICES;
        return tempdevice;
    }

    /* scann for sansas */
    struct sansa_t sansa;
    int n2 = sansa_scan(&sansa);
    if(n2==1)
    {
        tempdevice.device_index = gv->plat_id.Index(wxT("sansae200"));
       /* find mount point if possible */
#if !(defined( __WXMSW__ ) || defined( __DARWIN__))    //linux code
        wxString tmp = resolve_mount_point(wxString(ipod.diskname,wxConvUTF8)+wxT("1"));
        if( tmp != wxT("") )
           tempdevice.path = tmp;
#endif
#if defined( __WXMSW__ )                              // windows code
        wxString tmp = guess_mount_point();
        if( tmp != wxT("") )
           tempdevice.path = tmp;
#endif
        return tempdevice;
    }
    else if (n > 1)
    {
        tempdevice.status = TOMANYDEVICES;
        return tempdevice;
    }


    tempdevice.status = NODEVICE;
    return tempdevice;

}




/***************************************************
* Windows code for autodetection
****************************************************/
#if defined( __WXMSW__ )

wxString guess_mount_point()
{
   wxString mountpoint = wxT("");
   TCHAR szDrvName[33];
   DWORD maxDriveSet, curDriveSet;
   DWORD drive;
   TCHAR szBuf[300];
   HANDLE hDevice;
   PSTORAGE_DEVICE_DESCRIPTOR pDevDesc;

   maxDriveSet = GetLogicalDrives();
   curDriveSet = maxDriveSet;
   for ( drive = 0; drive < 32; ++drive )
   {
      if ( maxDriveSet & (1 << drive) )
      {
          DWORD temp = 1<<drive;
          _stprintf( szDrvName, _T("%c:\\"), 'A'+drive );
		  switch ( GetDriveType( szDrvName ) )
          {
             case 0:					// The drive type cannot be determined.
			 case 1:					// The root directory does not exist.
             case DRIVE_CDROM:		    // The drive is a CD-ROM drive.
             case DRIVE_REMOTE:		    // The drive is a remote (network) drive.
			 case DRIVE_RAMDISK:		// The drive is a RAM disk.
             case DRIVE_REMOVABLE:	    // The drive can be removed from the drive.
            	break;
			 case DRIVE_FIXED:		    // The disk cannot be removed from the drive.
				sprintf(szBuf, "\\\\?\\%c:", 'A'+drive);
				hDevice = CreateFile(szBuf, GENERIC_READ,
							FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL);

				if (hDevice != INVALID_HANDLE_VALUE)
				{
					pDevDesc = (PSTORAGE_DEVICE_DESCRIPTOR)new BYTE[sizeof(STORAGE_DEVICE_DESCRIPTOR) + 512 - 1];
					pDevDesc->Size = sizeof(STORAGE_DEVICE_DESCRIPTOR) + 512 - 1;

					if(GetDisksProperty(hDevice, pDevDesc))
					{
						if(pDevDesc->BusType == BusTypeUsb)
						{
							mountpoint.Printf(wxT("%c:\\"), chFirstDriveFromMask(temp));
						}
					}
					delete pDevDesc;
					CloseHandle(hDevice);
				}
				break;
 		  }
      }
   }
   return mountpoint;

}



/****************************************************************************
*    FUNCTION: GetDisksProperty(HANDLE hDevice, PSTORAGE_DEVICE_DESCRIPTOR pDevDesc)
*    PURPOSE:  get the info of specified device
*****************************************************************************/
BOOL GetDisksProperty(HANDLE hDevice, PSTORAGE_DEVICE_DESCRIPTOR pDevDesc)
{
	STORAGE_PROPERTY_QUERY	Query;	// input param for query
	DWORD dwOutBytes;				// IOCTL output length
	BOOL bResult;					// IOCTL return val

	// specify the query type
	Query.PropertyId = StorageDeviceProperty;
	Query.QueryType = PropertyStandardQuery;

	// Query using IOCTL_STORAGE_QUERY_PROPERTY
	bResult = ::DeviceIoControl(hDevice,			// device handle
			IOCTL_STORAGE_QUERY_PROPERTY,			// info of device property
			&Query, sizeof(STORAGE_PROPERTY_QUERY),	// input data buffer
			pDevDesc, pDevDesc->Size,				// output data buffer
			&dwOutBytes,							// out's length
			(LPOVERLAPPED)NULL);

	return bResult;
}

/*********************************************
* Converts the driveMask to a drive letter
*******************************************/
char chFirstDriveFromMask (ULONG unitmask)
{

      char i;
      for (i = 0; i < 26; ++i)
      {
           if (unitmask & 0x1)
				break;
            unitmask = unitmask >> 1;
      }
    return (i + 'A');
}
#endif /* windows code */

/**********************************************************
* Linux code for autodetection
*******************************************************/
#if !(defined( __WXMSW__ ) || defined( __DARWIN__))



wxString resolve_mount_point( const wxString device )
{
    FILE *fp = fopen( "/proc/mounts", "r" );
    if( !fp ) return wxT("");
    char *dev, *dir;
    while( fscanf( fp, "%as %as %*s %*s %*s %*s", &dev, &dir ) != EOF )
    {
        if( wxString( dev, wxConvUTF8 ) == device )
        {
            wxString directory = wxString( dir, wxConvUTF8 );
            free( dev );
            free( dir );
            return directory;
        }
        free( dev );
        free( dir );
    }
    fclose( fp );
    return wxT("");
}



#endif
