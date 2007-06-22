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

bool ipodpatcherDetect(UsbDeviceInfo* tempdevice)
{
   /* use ipodpatcher for ipod detecting */
    struct ipod_t ipod;
    int n = ipod_scan(&ipod);
    if(n == 1)  /* we found an ipod */
    {
        wxString temp(ipod.targetname,wxConvUTF8);
        int index = gv->plat_bootloadername.Index(temp);   // use the bootloader names..
        tempdevice->device_index = index;
        tempdevice->status=DEVICEFOUND;

        /* find mount point if possible */
#if !(defined( __WXMSW__ ) || defined( __DARWIN__))          //linux code
        wxString tmp = resolve_mount_point(wxString(ipod.diskname,wxConvUTF8)+wxT("2"));
        if( tmp != wxT("") )
            tempdevice->path = tmp;
#endif
        return true;

    }
    else if (n > 1)  /* to many ipods */
    {
        tempdevice->status = TOMANYDEVICES;
        return true;
    }
    else  /* no ipod */
    {
        return false;
    }

}

bool sansapatcherDetect(UsbDeviceInfo* tempdevice)
{
   /* scann for sansas */
    struct sansa_t sansa;
    int n = sansa_scan(&sansa);
    if(n==1)
    {
        tempdevice->device_index = gv->plat_id.Index(wxT("sansae200"));
        tempdevice->status = DEVICEFOUND;
       /* find mount point if possible */
#if !(defined( __WXMSW__ ) || defined( __DARWIN__))    //linux code
        wxString tmp = resolve_mount_point(wxString(sansa.diskname,wxConvUTF8)+wxT("1"));
        if( tmp != wxT("") )
           tempdevice->path = tmp;
#endif
        return true;
    }
    else if (n > 1)
    {
        tempdevice->status = TOMANYDEVICES;
        return true;
    }
    else
    {
        return false;
    }
}


bool rockboxinfoDetect(wxString filename,UsbDeviceInfo* tempdevice)
{
    wxTextFile rockboxinfo(filename);
    rockboxinfo.Open();
    wxString line = rockboxinfo.GetFirstLine();
    wxString targetstring;
    if(line.StartsWith(wxT("Target: "), &targetstring))
    {
        int index = gv->plat_id.Index(targetstring);
        if(index < 0) return false;

        tempdevice->device_index = index;
        wxString myPath;
        if(filename.EndsWith(wxT(".rockbox" PATH_SEP "rockbox-info.txt"),&myPath));
            tempdevice->path = myPath;

        tempdevice->status = DEVICEFOUND;

        return true;
    }
    else
    {
        return false;
    }


}


bool detectDevices(UsbDeviceInfo* tempdevice)
{
    tempdevice->device_index= 0;
    tempdevice->path=wxT("");
    tempdevice->status =NODEVICE;

    /* try ipodpatcher */
    if(ipodpatcherDetect(tempdevice))
    {
        return true;
    }

    /* try sansapatcher */
    if(sansapatcherDetect(tempdevice))
    {
        return true;
    }

    /*try via files on the devices */
    wxArrayString mountpoints = getPossibleMountPoints();

    for(unsigned int i=0;i<mountpoints.GetCount();i++)
    {
        if(wxDir::Exists(mountpoints[i]))
        {
            /*check for rockbox-info.txt */
            wxString filename;
            filename.Printf("%s" PATH_SEP ".rockbox" PATH_SEP "rockbox-info.txt",mountpoints[i].c_str());
            if(wxFile::Exists(filename))
            {
                if(rockboxinfoDetect(filename,tempdevice))
                     return true;
            }

        }
    }

    return false;
}





/***************************************************
* Windows code for autodetection
****************************************************/
#if defined( __WXMSW__ )

wxArrayString getPossibleMountPoints()
{
    wxArrayString tempList;
    tempList.Add(wxT("D:\\"));
    tempList.Add(wxT("E:\\"));
    tempList.Add(wxT("F:\\"));
    tempList.Add(wxT("G:\\"));
    tempList.Add(wxT("H:\\"));
    tempList.Add(wxT("I:\\"));
    tempList.Add(wxT("J:\\"));
    tempList.Add(wxT("K:\\"));
    tempList.Add(wxT("L:\\"));
    tempList.Add(wxT("M:\\"));
    tempList.Add(wxT("N:\\"));
    tempList.Add(wxT("O:\\"));
    tempList.Add(wxT("P:\\"));
    tempList.Add(wxT("Q:\\"));
    tempList.Add(wxT("R:\\"));
    tempList.Add(wxT("S:\\"));
    tempList.Add(wxT("T:\\"));
    tempList.Add(wxT("U:\\"));
    tempList.Add(wxT("V:\\"));
    tempList.Add(wxT("W:\\"));
    tempList.Add(wxT("X:\\"));
    tempList.Add(wxT("Y:\\"));
    tempList.Add(wxT("Z:\\"));

    return tempList;
}


#endif /* windows code */

/**********************************************************
* Linux code for autodetection
*******************************************************/
#if !(defined( __WXMSW__ ) || defined( __DARWIN__))

wxArrayString getPossibleMountPoints()
{
    wxArrayString tempList;

    FILE *fp = fopen( "/proc/mounts", "r" );
    if( !fp ) return tempList;
    char *dev, *dir;
    while( fscanf( fp, "%as %as %*s %*s %*s %*s", &dev, &dir ) != EOF )
    {
        wxString directory = wxString( dir, wxConvUTF8 );
        tempList.Add(directory);
        free( dev );
        free( dir );
    }
    fclose( fp );

    return tempList;
}

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



#endif /* linux code */

/**********************************************************
* MAC  code for autodetection
*******************************************************/
#if  defined( __DARWIN__)

wxArrayString getPossibleMountPoints()
{
    wxArrayString tempList;

    wxDir volumes;

    if(volumes.Open(wxT("/Volumes")))
    {
        wxString filename;
        bool cont = volumes.GetFirst(&filename, wxEmptyString, wxDIR_DIRS);
        while ( cont )
        {
            tempList.Add(filename);
            cont = dir.GetNext(&filename);
        }
    }
    return tempList;

}

#endif /* Mac Code */
