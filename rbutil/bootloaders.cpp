/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * Module: rbutil
 * File: bootloaders.cpp
 *
 * Copyright (C) 2007 Dominik Wenger
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "bootloaders.h"
#include "irivertools.h"
#include "md5sum.h"

#include "rbutil.h"
#include "installlog.h"


int verbose =0;
// reserves memory for ipodpatcher
bool initIpodpatcher()
{
      if (ipod_alloc_buffer(&sectorbuf,BUFFER_SIZE) < 0) return true;
      else return false;
}
// uses ipodpatcher for add and rem of bootloader
bool ipodpatcher(int mode,wxString bootloadername)
{
    wxString src,dest,buf;

    struct ipod_t ipod;

    int n = ipod_scan(&ipod);
    if (n == 0)
    {
         ERR_DIALOG(wxT("[ERR]  No ipods found."), wxT("Scanning for Ipods"));
         return false;
    }
    if (n > 1)
    {
         ERR_DIALOG(wxT("[ERR]  to many ipods found."), wxT("Scanning for Ipods"));
         return false;
    }

     // downloading files
    if(mode == BOOTLOADER_ADD)
    {
        src = gv->bootloader_url + wxT("/ipod/")
              + bootloadername + wxT(".ipod");
        dest = gv->stdpaths->GetUserDataDir()
               + wxT("" PATH_SEP "download" PATH_SEP) + bootloadername;
        if ( DownloadURL(src, dest) )
        {
            wxRemoveFile(dest);
            ERR_DIALOG(wxT("Unable to download ") + src, wxT("Install"));
            return false;
        }
    }

    if (ipod_open(&ipod, 0) < 0)
    {
       ERR_DIALOG(wxT("[ERR] could not open ipod"), wxT("open Ipod"));
       return false;
    }

    if (read_partinfo(&ipod,0) < 0)
    {
       ERR_DIALOG(wxT("[ERR] could not read partitiontable"), wxT("reading partitiontable"));
       return false;
    }

    if (ipod.pinfo[0].start==0)
    {
       ERR_DIALOG(wxT("[ERR]  No partition 0 on disk"), wxT("reading partitiontable"));
       int i;
       double sectors_per_MB = (1024.0*1024.0)/ipod.sector_size;

       buf.Printf(wxT("[INFO] Part    Start Sector    End Sector   Size (MB)   Type\n"));
       ERR_DIALOG(buf, wxT("reading partitiontable"));
       for ( i = 0; i < 4; i++ ) {
         if (ipod.pinfo[i].start != 0) {
             buf.Printf(wxT("[INFO]    %d      %10ld    %10ld  %10.1f   %s (0x%02x)\n"),
                   i,
                   ipod.pinfo[i].start,
                   ipod.pinfo[i].start+ipod.pinfo[i].size-1,
                   ipod.pinfo[i].size/sectors_per_MB,
                   get_parttype(ipod.pinfo[i].type),
                   ipod.pinfo[i].type);
             ERR_DIALOG(buf, wxT("reading partitiontable"));
        }
    }
       return false;
    }

    read_directory(&ipod);

    if (ipod.nimages <= 0)
    {
        ERR_DIALOG(wxT("[ERR]  Failed to read firmware directory"), wxT("reading directory"));
        return false;
    }
    if (getmodel(&ipod,(ipod.ipod_directory[0].vers>>8)) < 0)
    {
       buf.Printf(wxT("[ERR] Unknown version number in firmware (%08x)\n"),
                                        ipod.ipod_directory[0].vers );
       ERR_DIALOG(buf, wxT("reading directory"));
       return false;
    }

    if (ipod.macpod)
    {
      WARN_DIALOG(wxT("Warning this is a MacPod, Rockbox doesnt work on this. Convert it to WinPod"),wxT("MacPod"));
    }


    if(mode == BOOTLOADER_ADD)
    {
        if (ipod_reopen_rw(&ipod) < 0) {
          ERR_DIALOG(wxT("[ERR] Could not open Ipod in RW mode"), wxT("Bootloader add"));
          return false;
        }

        if (add_bootloader(&ipod, (char*)dest.c_str(), FILETYPE_DOT_IPOD)==0) {

        } else {
           ERR_DIALOG(wxT("[ERR] failed to add Bootloader"), wxT("Bootloader add"));
           return false;
        }
    }
    else if(mode == BOOTLOADER_REM)
    {
        if (ipod_reopen_rw(&ipod) < 0) {
          ERR_DIALOG(wxT("[ERR] Could not open Ipod in RW mode"), wxT("Bootloader add"));
          return false;
        }

        if (ipod.ipod_directory[0].entryOffset==0) {
            ERR_DIALOG(wxT("[ERR]  No bootloader detected.\n"), wxT("Bootloader del"));
            return false;
        } else {
            if (delete_bootloader(&ipod)==0) {

            } else {
               ERR_DIALOG(wxT("[ERR]  --delete-bootloader failed.\n"), wxT("Bootloader del"));
               return false;
            }
        }
    }

    ipod_close(&ipod);
    return true;
}

// reserves memory for sansapatcher
bool initSansaPatcher()
{
      if (sansa_alloc_buffer(&sectorbuf,BUFFER_SIZE) < 0) return true;
      else return false;
}


// sansainstallation
bool sansapatcher(int mode,wxString bootloadername)
{
    wxString src,dest,buf;

    struct sansa_t sansa;

    int n = sansa_scan(&sansa);
    if (n == 0)
    {
         ERR_DIALOG(wxT("[ERR]  No Sansa found."), wxT("Scanning for Sansa"));
         return false;
    }
    if (n > 1)
    {
         ERR_DIALOG(wxT("[ERR]  to many Sansa found."), wxT("Scanning for Sansa"));
         return false;
    }

      // downloading files
    if(mode == BOOTLOADER_ADD)
    {
        src = gv->bootloader_url + wxT("/sandisk-sansa/e200/")
              + bootloadername;
        dest = gv->stdpaths->GetUserDataDir()
               + wxT("" PATH_SEP "download" PATH_SEP) + bootloadername;
        if ( DownloadURL(src, dest) )
        {
            wxRemoveFile(dest);
            ERR_DIALOG(wxT("Unable to download ") + src, wxT("Download"));
            return false;
        }
    }

    if (sansa_open(&sansa, 0) < 0)
    {
       ERR_DIALOG(wxT("[ERR] could not open sansa"), wxT("open Sansa"));
       return false;
    }

    if (sansa_read_partinfo(&sansa,0) < 0)
    {
       ERR_DIALOG(wxT("[ERR] could not read partitiontable"), wxT("reading partitiontable"));
       return false;
    }

    int i = is_e200(&sansa);
    if (i < 0) {
        ERR_DIALOG(wxT("[ERR]  Disk is not an E200 (%d), aborting.\n"), wxT("Checking Disk"));
        return false;
    }

    if (sansa.hasoldbootloader)
    {
        ERR_DIALOG(wxT("[ERR]  ************************************************************************\n"
                      "[ERR]  *** OLD ROCKBOX INSTALLATION DETECTED, ABORTING.\n"
                      "[ERR]  *** You must reinstall the original Sansa firmware before running\n"
                      "[ERR]  *** sansapatcher for the first time.\n"
                      "[ERR]  *** See http://www.rockbox.org/twiki/bin/view/Main/SansaE200Install\n"
                      "[ERR]  ************************************************************************\n"),wxT("Checking Disk"));
        return false;
    }

    if(mode == BOOTLOADER_ADD)
    {
        if (sansa_reopen_rw(&sansa) < 0) {
              ERR_DIALOG(wxT("[ERR] Could not open Sansa in RW mode"), wxT("Bootloader add"));
          return false;
        }

        if (sansa_add_bootloader(&sansa, (char*)dest.c_str(), FILETYPE_MI4)==0) {

        } else {
           ERR_DIALOG(wxT("[ERR] failed to add Bootloader"), wxT("Bootloader add"));
        }

    }
    else if(mode == BOOTLOADER_REM)
    {
         if (sansa_reopen_rw(&sansa) < 0) {
             ERR_DIALOG(wxT("[ERR] Could not open Sansa in RW mode"), wxT("Bootloader Remove"));
         }

         if (sansa_delete_bootloader(&sansa)==0) {

         } else {
             ERR_DIALOG(wxT("[ERR] failed to remove Bootloader"), wxT("Bootloader remove"));
         }
    }

    sansa_close(&sansa);
    return true;
}

// gigabeatinstallation
bool gigabeatf(int mode,wxString bootloadername,wxString deviceDir)
{
   wxString path1,path2;
   wxString err;
   wxString src,dest;

   path1 = deviceDir
         + wxT("" PATH_SEP "GBSYSTEM" PATH_SEP "FWIMG" PATH_SEP "FWIMG01.DAT");

    if(mode == BOOTLOADER_ADD)
    {
        //Files downloaden
        src = gv->bootloader_url + wxT("/gigabeat/") + bootloadername;
        dest = gv->stdpaths->GetUserDataDir()
               + wxT("" PATH_SEP "download" PATH_SEP) + bootloadername;
         if( DownloadURL(src, dest) )
         {
              wxRemoveFile(dest);
              ERR_DIALOG(wxT("Unable to download ") + src, wxT("Install"));
              return false;
         }


      if(!wxFileExists(path1))
      {
          ERR_DIALOG(wxT("[ERR] Coud not find ")+path1, wxT("Bootloader add"));
          return false;
      }
      path2 = path1;
      path2.Append(wxT(".ORIG"));
      if(!wxFileExists(path2))
      {
         if(!wxRenameFile(path1,path2,false))
         {
            ERR_DIALOG(wxT("[ERR] Coud not rename ") + path1 + wxT(" to ")
                       + path2, wxT("Bootloader add"));
           return false;
         }
      }


      if(!wxCopyFile(dest,path1))
      {
         ERR_DIALOG(wxT("[ERR] Coud not copy ") + dest + wxT(" to ")
                    + path2, wxT("Bootloader add"));
         return false;
      }
    }
    else if(mode == BOOTLOADER_REM)
    {
      path2 = path1;
      path2.Append(wxT(".ORIG"));
      if(!wxFileExists(path2))
      {
          ERR_DIALOG(wxT("[ERR] Coud not find ") + path1,
                     wxT("Bootloader del"));
          return false;
      }
      if(!wxRenameFile(path2,path1,true))
      {
         ERR_DIALOG(wxT("[ERR] Coud not rename ") + path1 + wxT(" to ")
                    + path2, wxT("Bootloader del"));
         return false;
      }
    }
    return true;
}

// iaudio bootloader install
bool iaudiox5(int mode,wxString bootloadername,wxString deviceDir)
{
    wxString path1,path2;
    wxString err;
    wxString src,dest;

    path1 = deviceDir + wxT("" PATH_SEP "FIRMWARE" PATH_SEP)
            + bootloadername;

    if(mode == BOOTLOADER_ADD)
    {
        //Files downloaden
        src = gv->bootloader_url + wxT("/iaudio/") + bootloadername;
        dest = gv->stdpaths->GetUserDataDir()
               + wxT("" PATH_SEP "download" PATH_SEP) + bootloadername;
         if( DownloadURL(src, dest) )
         {
              wxRemoveFile(dest);
              ERR_DIALOG(wxT("Unable to download ") + src, wxT("Install"));
              return false;
         }

         // copy file
         if(!wxCopyFile(dest,path1))
         {
            ERR_DIALOG(wxT("[ERR] Coud not copy ")+dest+wxT(" to ")+path2,
                       wxT("Bootloader add"));
            return false;
         }

         return true;   // install ready
    }
    else
      return false;   //no uninstallation possible
}

// H10 install
bool h10(int mode,wxString bootloadername,wxString deviceDir)
{
  wxString err,src,dest,path1,path2;

   int pos = bootloadername.Find('/');
   if(pos == wxNOT_FOUND) pos = 0;
   wxString firmwarename = bootloadername.SubString(pos,bootloadername.Length());
   //wxString firmDir = gv->curbootloader.SubString(0,pos);

  if(mode == BOOTLOADER_ADD)
  {
     //Files downloaden
     src = gv->bootloader_url + wxT("/iriver/") + bootloadername;
     dest = gv->stdpaths->GetUserDataDir()
            + wxT("" PATH_SEP "download" PATH_SEP) + firmwarename;
     if( DownloadURL(src, dest) )
     {
           wxRemoveFile(dest);
           ERR_DIALOG(wxT("Unable to download ") + src, wxT("Install"));
           return false;
     }

     path1 = deviceDir + wxT("SYSTEM" PATH_SEP) + firmwarename;
     path2 = deviceDir + wxT("SYSTEM" PATH_SEP "Original.mi4");

     if(!wxFileExists(path1))  //Firmware dosent exists on player
     {
        path1 = deviceDir + wxT("SYSTEM" PATH_SEP "H10EMP.mi4");   //attempt other firmwarename
        if(!wxFileExists(path1))  //Firmware dosent exists on player
        {
            ERR_DIALOG(wxT("[ERR] File ") + path1 + wxT(" does not Exist"),
                       wxT("Bootloader add"));
            return false;
        }
     }
     if(!wxFileExists(path2))  //there is already a original firmware
     {
        if(!wxRenameFile(path1,path2,false))  //rename Firmware to Original
        {
           ERR_DIALOG(wxT("[ERR] Coud not rename ") + path1 + wxT(" to ")
                      + path2, wxT("Bootloader add"));
           return false;
        }
     }

     if(!wxCopyFile(dest,path1))  // copy file
     {
        ERR_DIALOG(wxT("[ERR] Coud not copy ") + dest + wxT(" to ") + path1,
                   wxT("Bootloader add"));
        return false;
     }

     return true;  //install ready

  }
  else if(mode == BOOTLOADER_REM)
  {
     path1 = deviceDir + wxT("SYSTEM" PATH_SEP) + firmwarename;
     path2 = gv->curdestdir + wxT("SYSTEM" PATH_SEP "Original.mi4");
     if(!wxFileExists(path1))  //Firmware dosent exists on player
     {
         path1 = deviceDir + wxT("" PATH_SEP "SYSTEM" PATH_SEP "H10EMP.mi4");   //attempt other firmwarename
         if(!wxFileExists(path1))  //Firmware dosent exists on player
         {
            ERR_DIALOG(wxT("[ERR] File ") + path1 + wxT(" does not Exist"),
                       wxT("Bootloader rem"));
            return false;
         }
     }

     if(!wxFileExists(path2))  //Original Firmware dosent exists on player
     {
         ERR_DIALOG(wxT("[ERR] File ") + path2 + wxT(" does not Exist"),
                    wxT("Bootloader rem"));
         return false;
     }

     if(!wxRenameFile(path2,path1,true))  //rename Firmware to Original
     {
         ERR_DIALOG(wxT("[ERR] Coud not rename ") + path2 + wxT(" to ")
                    + path1, wxT("Bootloader add"));
         return false;
     }

  }
  // shouldn't get here
  return false;
}

// FWPatcher
bool fwpatcher(int mode,wxString bootloadername,wxString deviceDir,wxString firmware)
{
    if(mode == BOOTLOADER_ADD)
    {
		char md5sum_str[32];
        wxString src,dest,err;
        int series,table_entry;

        if (!FileMD5(firmware, md5sum_str)) {
        ERR_DIALOG(wxT("Could not open firmware"), wxT("Open Firmware"));
        return false;
       }
       else {
               /* Check firmware against md5sums in h120sums and h100sums */
                series = 0;
                table_entry = intable(md5sum_str, &h120pairs[0],
                                      sizeof(h120pairs)/sizeof(struct sumpairs));
                if (table_entry >= 0) {
                    series = 120;
                }
                else {
                    table_entry = intable(md5sum_str, &h100pairs[0],
                                          sizeof(h100pairs)/sizeof(struct sumpairs));
                    if (table_entry >= 0) {
                        series = 100;
                    }
                    else {
                        table_entry =
                            intable(md5sum_str, &h300pairs[0],
                                    sizeof(h300pairs)/sizeof(struct sumpairs));
                        if (table_entry >= 0)
                            series = 300;
                    }
                }
                if (series == 0) {
                    ERR_DIALOG(wxT("Could not detect firmware type"), wxT("Detect Player out of Firmware"));
                    return false;
                }
                else
                {
                    //Download bootloader
                    src = gv->bootloader_url + wxT("/iriver/")
                          + bootloadername;
                    dest = gv->stdpaths->GetUserDataDir()
                           + wxT("" PATH_SEP "download" PATH_SEP)
                           + bootloadername;
                    if( DownloadURL(src, dest) )
                    {
                        wxRemoveFile(dest);
                        ERR_DIALOG(wxT("Unable to download ") + src,
                                   wxT("Install"));
                        return false;
                    }

                    if(!PatchFirmware(firmware,dest,series, table_entry))  // Patch firmware
                    {
                        ERR_DIALOG(wxT("Patching Firmware failed"),
                                   wxT("Patching Firmware"));
                        return false;
                    }
                }

                // Load patched Firmware to player
                src = gv->stdpaths->GetUserDataDir()
                      + wxT("" PATH_SEP "download" PATH_SEP "new.hex");

                if(gv->curplat == wxT("h100"))
                    dest = deviceDir + wxT("" PATH_SEP "ihp_100.hex");
                else if(gv->curplat == wxT("h120"))
                    dest = deviceDir + wxT("" PATH_SEP "ihp_120.hex");
                else if(gv->curplat == wxT("h300"))
                    dest = deviceDir + wxT("" PATH_SEP "H300.hex");

                if(!wxRenameFile(src,dest))
                {
                     ERR_DIALOG(wxT("Copying Firmware to Device failed"),
                                wxT("Copying Firmware"));
                     return false;
                }
                else
                {
                    return true;
                }
         }

    }
    else
    {
        return false;  //no uninstall possible
    }
}
