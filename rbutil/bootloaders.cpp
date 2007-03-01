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


// for ipodpatcher
int verbose = 0;
// reserves memory for ipodpatcher
bool initIpodpatcher()
{
      if (ipod_alloc_buffer(&sectorbuf,BUFFER_SIZE) < 0) return true;
      else return false;
}
// uses ipodpatcher for add and rem of bootloader
bool ipodpatcher(int mode)
{
    wxString src,dest,buf;

    // downloading files
    if(mode == BOOTLOADER_ADD)
    {
        src.Printf("%s/ipod/%s.ipod", gv->bootloader_url.c_str(),gv->curbootloader.c_str());
        dest.Printf("%s" PATH_SEP "download" PATH_SEP "%s",
                    gv->stdpaths->GetUserDataDir().c_str(),gv->curbootloader.c_str());
        if ( DownloadURL(src, dest) )
        {
            wxRemoveFile(dest);
            buf.Printf(_("Unable to download %s"), src.c_str() );
            ERR_DIALOG(buf, _("Install"));
            return false;
        }
    }

    struct ipod_t ipod;

    int n = ipod_scan(&ipod);
    if (n == 0)
    {
         ERR_DIALOG("[ERR]  No ipods found.", _("Scanning for Ipods"));
         return false;
    }
    if (n > 1)
    {
         ERR_DIALOG("[ERR]  to many ipods found.", _("Scanning for Ipods"));
         return false;
    }

    if (ipod_open(&ipod, 0) < 0)
    {
       ERR_DIALOG("[ERR] could not open ipod", _("open Ipod"));
       return false;
    }

    if (read_partinfo(&ipod,0) < 0)
    {
       ERR_DIALOG("[ERR] could not read partitiontable", _("reading partitiontable"));
       return false;
    }

    if (ipod.pinfo[0].start==0)
    {
       ERR_DIALOG("[ERR]  No partition 0 on disk", _("reading partitiontable"));
       int i;
       double sectors_per_MB = (1024.0*1024.0)/ipod.sector_size;

       buf.Printf("[INFO] Part    Start Sector    End Sector   Size (MB)   Type\n");
       ERR_DIALOG(buf, _("reading partitiontable"));
       for ( i = 0; i < 4; i++ ) {
         if (ipod.pinfo[i].start != 0) {
             buf.Printf("[INFO]    %d      %10ld    %10ld  %10.1f   %s (0x%02x)\n",
                   i,
                   ipod.pinfo[i].start,
                   ipod.pinfo[i].start+ipod.pinfo[i].size-1,
                   ipod.pinfo[i].size/sectors_per_MB,
                   get_parttype(ipod.pinfo[i].type),
                   ipod.pinfo[i].type);
             ERR_DIALOG(buf, _("reading partitiontable"));
        }
    }
       return false;
    }

    read_directory(&ipod);

    if (ipod.nimages <= 0)
    {
        ERR_DIALOG("[ERR]  Failed to read firmware directory", _("reading directory"));
        return false;
    }
    if (getmodel(&ipod,(ipod.ipod_directory[0].vers>>8)) < 0)
    {
       buf.Printf(_("[ERR] Unknown version number in firmware (%08x)\n"),
                                        ipod.ipod_directory[0].vers );
       ERR_DIALOG(buf, _("reading directory"));
       return false;
    }

    if (ipod.macpod)
    {
      WARN_DIALOG("Warning this is a MacPod, Rockbox doesnt work on this. Convert it to WinPod",_("MacPod"));
    }


    if(mode == BOOTLOADER_ADD)
    {
        if (ipod_reopen_rw(&ipod) < 0) {
          ERR_DIALOG("[ERR] Could not open Ipod in RW mode", _("Bootloader add"));
          return false;
        }

        if (add_bootloader(&ipod, (char*)dest.c_str(), FILETYPE_DOT_IPOD)==0) {

        } else {
           ERR_DIALOG("[ERR] failed to add Bootloader", _("Bootloader add"));
           return false;
        }
    }
    else if(mode == BOOTLOADER_REM)
    {
        if (ipod_reopen_rw(&ipod) < 0) {
          ERR_DIALOG("[ERR] Could not open Ipod in RW mode", _("Bootloader add"));
          return false;
        }

        if (ipod.ipod_directory[0].entryOffset==0) {
            ERR_DIALOG("[ERR]  No bootloader detected.\n", _("Bootloader del"));
            return false;
        } else {
            if (delete_bootloader(&ipod)==0) {

            } else {
               ERR_DIALOG("[ERR]  --delete-bootloader failed.\n", _("Bootloader del"));
               return false;
            }
        }
    }

    ipod_close(&ipod);
    return true;
}

// gigabeatinstallation
bool gigabeatf(int mode)
{
   wxString path1,path2;
   wxString err;
   wxString src,dest;

   path1.Printf("%s" PATH_SEP "GBSYSTEM" PATH_SEP "FWIMG" PATH_SEP "FWIMG01.DAT",gv->curdestdir.c_str());

    if(mode == BOOTLOADER_ADD)
    {
        //Files downloaden
        src.Printf("%s/gigabeat/%s", gv->bootloader_url.c_str(),gv->curbootloader.c_str());
        dest.Printf("%s" PATH_SEP "download" PATH_SEP "%s",
                    gv->stdpaths->GetUserDataDir().c_str(),gv->curbootloader.c_str());
         if( DownloadURL(src, dest) )
         {
              wxRemoveFile(dest);
              err.Printf(_("Unable to download %s"), src.c_str() );
              ERR_DIALOG(err, _("Install"));
              return false;
         }


      if(!wxFileExists(path1))
      {
          err.Printf("[ERR] Coud not find %s",path1.c_str());
          ERR_DIALOG(err, _("Bootloader add"));
          return false;
      }
      path2 = path1;
      path2.Append(".ORIG");
      if(wxFileExists(path2))
      {
          err = "Its seems there is already a Bootloader install, if not, delete the *.IMG.ORIG file";
          ERR_DIALOG(err, _("Bootloader add"));
          return false;
      }

      if(!wxRenameFile(path1,path2,false))
      {
         err.Printf("[ERR] Coud not rename %s to %s",path1.c_str(),path2.c_str());
         ERR_DIALOG(err, _("Bootloader add"));
         return false;
      }
      if(!wxCopyFile(dest,path1))
      {
         err.Printf("[ERR] Coud not copy %s to %s",dest.c_str(),path2.c_str());
         ERR_DIALOG(err, _("Bootloader add"));
         return false;
      }
    }
    else if(mode == BOOTLOADER_REM)
    {
      path2 = path1;
      path2.Append(".ORIG");
      if(!wxFileExists(path2))
      {
          err.Printf("[ERR] Coud not find %s",path1.c_str());
          ERR_DIALOG(err, _("Bootloader del"));
          return false;
      }
      if(!wxRenameFile(path2,path1,true))
      {
         err.Printf("[ERR] Coud not rename %s to %s",path1.c_str(),path2.c_str());
         ERR_DIALOG(err, _("Bootloader del"));
         return false;
      }
    }
    return true;
}

// iaudio bootloader install
bool iaudiox5(int mode)
{
    wxString path1,path2;
    wxString err;
    wxString src,dest;

    path1.Printf("%s" PATH_SEP "FIRMWARE" PATH_SEP "%s",gv->curdestdir.c_str(),gv->curbootloader.c_str());

    if(mode == BOOTLOADER_ADD)
    {
        //Files downloaden
        src.Printf("%s/iaudio/%s", gv->bootloader_url.c_str(),gv->curbootloader.c_str());
        dest.Printf("%s" PATH_SEP "download" PATH_SEP "%s",
                    gv->stdpaths->GetUserDataDir().c_str(),gv->curbootloader.c_str());
         if( DownloadURL(src, dest) )
         {
              wxRemoveFile(dest);
              err.Printf(_("Unable to download %s"), src.c_str() );
              ERR_DIALOG(err, _("Install"));
              return false;
         }

         // copy file
         if(!wxCopyFile(dest,path1))
         {
            err.Printf("[ERR] Coud not copy %s to %s",dest.c_str(),path2.c_str());
            ERR_DIALOG(err, _("Bootloader add"));
            return false;
         }

         return true;   // install ready
    }
    else
      return false;   //no uninstallation possible
}

// H10 install
bool h10(int mode)
{
  wxString err,src,dest,path1,path2;

   int pos = gv->curbootloader.Find('/');
   if(pos == wxNOT_FOUND) pos = 0;
   wxString firmwarename = gv->curbootloader.SubString(pos,gv->curbootloader.Length());
   //wxString firmDir = gv->curbootloader.SubString(0,pos);

  if(mode == BOOTLOADER_ADD)
  {
     //Files downloaden
     src.Printf("%s/iriver/%s", gv->bootloader_url.c_str(),gv->curbootloader.c_str());
     dest.Printf("%s" PATH_SEP "download" PATH_SEP "%s",
                  gv->stdpaths->GetUserDataDir().c_str(),firmwarename.c_str());
     if( DownloadURL(src, dest) )
     {
           wxRemoveFile(dest);
           err.Printf(_("Unable to download %s"), src.c_str() );
           ERR_DIALOG(err, _("Install"));
           return false;
     }

     path1.Printf("%sSYSTEM" PATH_SEP "%s",gv->curdestdir.c_str(),firmwarename.c_str());
     path2.Printf("%sSYSTEM" PATH_SEP "Original.mi4",gv->curdestdir.c_str());

     if(!wxFileExists(path1))  //Firmware dosent exists on player
     {
        path1.Printf("%sSYSTEM" PATH_SEP "H10EMP.mi4");   //attempt other firmwarename
        if(!wxFileExists(path1))  //Firmware dosent exists on player
        {
            err.Printf("[ERR] File %s does not Exist",path1.c_str());
            ERR_DIALOG(err, _("Bootloader add"));
            return false;
        }
     }
     if(wxFileExists(path2))  //there is already a original firmware
     {
        err.Printf("[ERR2] File %s does Exist",path2.c_str());
        ERR_DIALOG(err, _("Bootloader add"));
        return false;
     }
     if(!wxRenameFile(path1,path2,false))  //rename Firmware to Original
     {
         err.Printf("[ERR] Coud not rename %s to %s",path1.c_str(),path2.c_str());
         ERR_DIALOG(err, _("Bootloader add"));
         return false;
     }
     if(!wxCopyFile(dest,path1))  // copy file
     {
        err.Printf("[ERR] Coud not copy %s to %s",dest.c_str(),path1.c_str());
        ERR_DIALOG(err, _("Bootloader add"));
        return false;
     }

     return true;  //install ready

  }
  else if(mode == BOOTLOADER_REM)
  {
     path1.Printf("%sSYSTEM" PATH_SEP "%s",gv->curdestdir.c_str(),firmwarename.c_str());
     path2.Printf("%sSYSTEM" PATH_SEP "Original.mi4",gv->curdestdir.c_str());
     if(!wxFileExists(path1))  //Firmware dosent exists on player
     {
         path1.Printf("%s" PATH_SEP "SYSTEM" PATH_SEP "H10EMP.mi4");   //attempt other firmwarename
         if(!wxFileExists(path1))  //Firmware dosent exists on player
         {
            err.Printf("[ERR] File %s does not Exist",path1.c_str());
            ERR_DIALOG(err, _("Bootloader rem"));
            return false;
         }
     }

     if(!wxFileExists(path2))  //Original Firmware dosent exists on player
     {
         err.Printf("[ERR] File %s does not Exist",path2.c_str());
         ERR_DIALOG(err, _("Bootloader rem"));
         return false;
     }

     if(!wxRenameFile(path2,path1,true))  //rename Firmware to Original
     {
         err.Printf("[ERR] Coud not rename %s to %s",path2.c_str(),path1.c_str());
         ERR_DIALOG(err, _("Bootloader add"));
         return false;
     }

  }

}

// FWPatcher
bool fwpatcher(int mode)
{
    if(mode == BOOTLOADER_ADD)
    {
        wxString md5sum_str,src,dest,err;
        int series,table_entry;

        if (!FileMD5(gv->curfirmware, &md5sum_str)) {
        ERR_DIALOG("Could not open firmware", _("Open Firmware"));
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
                    ERR_DIALOG("Could not detect firmware type", _("Detect Player out of Firmware"));
                    return false;
                }
                else
                {
                    //Download bootloader
                    src.Printf("%s/iriver/%s", gv->bootloader_url.c_str(),gv->curbootloader.c_str());
                    dest.Printf("%s" PATH_SEP "download" PATH_SEP "%s",
                            gv->stdpaths->GetUserDataDir().c_str(),gv->curbootloader.c_str());
                    if( DownloadURL(src, dest) )
                    {
                        wxRemoveFile(dest);
                        err.Printf(_("Unable to download %s"), src.c_str() );
                        ERR_DIALOG(err, _("Install"));
                        return false;
                    }

                    if(!PatchFirmware(gv->curfirmware,dest,series, table_entry))  // Patch firmware
                    {
                        ERR_DIALOG("Patching Firmware failed", _("Patching Firmware"));
                        return false;
                    }
                }

                // Load patched Firmware to player
                src.Printf("%s" PATH_SEP "download" PATH_SEP "new.hex",
                        gv->stdpaths->GetUserDataDir().c_str());

                if(gv->curplat == "h100")
                    dest.Printf("%s" PATH_SEP "ihp_100.hex",gv->curdestdir.c_str());
                else if(gv->curplat == "h120")
                    dest.Printf("%s" PATH_SEP "ihp_120.hex",gv->curdestdir.c_str());
                else if(gv->curplat == "h300")
                dest.Printf("%s" PATH_SEP "H300.hex",gv->curdestdir.c_str());

                if(!wxRenameFile(src,dest))
                {
                     ERR_DIALOG("Copying Firmware to Device failed", _("Copying Firmware"));
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
