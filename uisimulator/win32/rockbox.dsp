# Microsoft Developer Studio Project File - Name="rockbox" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=rockbox - Win32 Player
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "rockbox.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "rockbox.mak" CFG="rockbox - Win32 Player"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "rockbox - Win32 Recorder" (based on "Win32 (x86) Application")
!MESSAGE "rockbox - Win32 Player" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "rockbox - Win32 Recorder"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "rockbox___Win32_Recorder"
# PROP BASE Intermediate_Dir "rockbox___Win32_Recorder"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "rockbox___Win32_Recorder"
# PROP Intermediate_Dir "rockbox___Win32_Recorder"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "../win32" /I "../../apps/recorder" /I "../../firmware/export" /I "../../firmware/drivers" /I "../../firmware/common" /I "../common" /I "../../apps" /D "ARCHOS_RECORDER" /D "HAVE_LCD_BITMAP" /D "HAVE_RECORDER_KEYPAD" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "HAVE_CONFIG_H" /D "GETTIMEOFDAY_TWO_ARGS" /D "SIMULATOR" /D "HAVE_RTC" /D APPSVERSION=\"WIN32SIM\" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib libcmtd.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"libcd.lib" /pdbtype:sept

!ELSEIF  "$(CFG)" == "rockbox - Win32 Player"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "rockbox___Win32_Player"
# PROP BASE Intermediate_Dir "rockbox___Win32_Player"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "rockbox___Win32_Player"
# PROP Intermediate_Dir "rockbox___Win32_Player"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "../win32" /I "../../apps/player" /I "../../firmware/export" /I "../../firmware/drivers" /I "../../firmware/common" /I "../common" /I "../../apps" /D "ARCHOS_PLAYER" /D "HAVE_LCD_CHARCELLS" /D "HAVE_PLAYER_KEYPAD" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "HAVE_CONFIG_H" /D "GETTIMEOFDAY_TWO_ARGS" /D "SIMULATOR" /D APPSVERSION=\"WIN32SIM\" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib libcmtd.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"libcd.lib" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "rockbox - Win32 Recorder"
# Name "rockbox - Win32 Player"
# Begin Group "Firmware"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\firmware\buffer.c
# End Source File
# Begin Source File

SOURCE=..\..\firmware\font.c

!IF  "$(CFG)" == "rockbox - Win32 Recorder"

!ELSEIF  "$(CFG)" == "rockbox - Win32 Player"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\firmware\id3.c
# End Source File
# Begin Source File

SOURCE=..\..\firmware\mp3_playback.c
# End Source File
# Begin Source File

SOURCE=..\..\firmware\mp3data.c
# End Source File
# Begin Source File

SOURCE=..\..\firmware\mpeg.c
# End Source File
# Begin Source File

SOURCE=..\..\firmware\powermgmt.c
# End Source File
# Begin Source File

SOURCE=..\..\firmware\usb.c
# End Source File
# End Group
# Begin Group "Drivers"

# PROP Default_Filter ""
# Begin Source File

SOURCE="..\..\firmware\drivers\lcd-player-charset.c"

!IF  "$(CFG)" == "rockbox - Win32 Recorder"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "rockbox - Win32 Player"

!ENDIF 

# End Source File
# Begin Source File

SOURCE="..\..\firmware\drivers\lcd-player.c"

!IF  "$(CFG)" == "rockbox - Win32 Recorder"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "rockbox - Win32 Player"

!ENDIF 

# End Source File
# Begin Source File

SOURCE="..\common\lcd-playersim.c"

!IF  "$(CFG)" == "rockbox - Win32 Recorder"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "rockbox - Win32 Player"

!ENDIF 

# End Source File
# Begin Source File

SOURCE="..\..\firmware\drivers\lcd-recorder.c"

!IF  "$(CFG)" == "rockbox - Win32 Recorder"

!ELSEIF  "$(CFG)" == "rockbox - Win32 Player"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\firmware\drivers\power.c
# End Source File
# End Group
# Begin Group "Common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\firmware\common\errno.c
# End Source File
# Begin Source File

SOURCE=..\..\firmware\common\random.c
# End Source File
# Begin Source File

SOURCE=..\..\firmware\common\sprintf.c
# End Source File
# Begin Source File

SOURCE=..\..\firmware\common\strtok.c
# End Source File
# Begin Source File

SOURCE=..\..\firmware\common\timefuncs.c
# End Source File
# End Group
# Begin Group "Apps"

# PROP Default_Filter ""
# Begin Group "Player"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\apps\player\icons.c

!IF  "$(CFG)" == "rockbox - Win32 Recorder"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "rockbox - Win32 Player"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\apps\player\icons.h

!IF  "$(CFG)" == "rockbox - Win32 Recorder"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "rockbox - Win32 Player"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\apps\player\keyboard.c

!IF  "$(CFG)" == "rockbox - Win32 Recorder"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "rockbox - Win32 Player"

!ENDIF 

# End Source File
# End Group
# Begin Group "Recorder"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\apps\recorder\bmp.c

!IF  "$(CFG)" == "rockbox - Win32 Recorder"

!ELSEIF  "$(CFG)" == "rockbox - Win32 Player"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\apps\recorder\icons.c

!IF  "$(CFG)" == "rockbox - Win32 Recorder"

!ELSEIF  "$(CFG)" == "rockbox - Win32 Player"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\apps\recorder\keyboard.c

!IF  "$(CFG)" == "rockbox - Win32 Recorder"

!ELSEIF  "$(CFG)" == "rockbox - Win32 Player"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\apps\recorder\peakmeter.c

!IF  "$(CFG)" == "rockbox - Win32 Recorder"

!ELSEIF  "$(CFG)" == "rockbox - Win32 Player"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\apps\recorder\widgets.c

!IF  "$(CFG)" == "rockbox - Win32 Recorder"

!ELSEIF  "$(CFG)" == "rockbox - Win32 Player"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=..\..\apps\alarm_menu.c
# End Source File
# Begin Source File

SOURCE=..\..\apps\bookmark.c
# End Source File
# Begin Source File

SOURCE=..\..\apps\credits.c
# End Source File
# Begin Source File

SOURCE=..\..\apps\debug_menu.c
# End Source File
# Begin Source File

SOURCE=..\..\apps\filetypes.c
# End Source File
# Begin Source File

SOURCE=..\..\apps\language.c
# End Source File
# Begin Source File

SOURCE=..\..\apps\main.c
# End Source File
# Begin Source File

SOURCE=..\..\apps\main_menu.c
# End Source File
# Begin Source File

SOURCE=..\..\apps\menu.c
# End Source File
# Begin Source File

SOURCE=..\..\apps\misc.c
# End Source File
# Begin Source File

SOURCE=..\..\apps\onplay.c
# End Source File
# Begin Source File

SOURCE=..\..\apps\playlist.c
# End Source File
# Begin Source File

SOURCE=..\..\apps\playlist_menu.c
# End Source File
# Begin Source File

SOURCE=..\..\apps\playlist_viewer.c
# End Source File
# Begin Source File

SOURCE=..\..\apps\plugin.c
# End Source File
# Begin Source File

SOURCE=..\..\apps\screens.c
# End Source File
# Begin Source File

SOURCE=..\..\apps\settings.c
# End Source File
# Begin Source File

SOURCE=..\..\apps\settings_menu.c
# End Source File
# Begin Source File

SOURCE=..\..\apps\sleeptimer.c
# End Source File
# Begin Source File

SOURCE=..\..\apps\sound_menu.c
# End Source File
# Begin Source File

SOURCE=..\..\apps\status.c
# End Source File
# Begin Source File

SOURCE=..\..\apps\tree.c
# End Source File
# Begin Source File

SOURCE="..\..\apps\wps-display.c"
# End Source File
# Begin Source File

SOURCE=..\..\apps\wps.c
# End Source File
# End Group
# Begin Group "Win32"

# PROP Default_Filter ""
# Begin Group "AppsCommon"

# PROP Default_Filter ""
# Begin Source File

SOURCE="..\common\lcd-common.c"
# End Source File
# Begin Source File

SOURCE=..\common\mpegplay.c
# End Source File
# Begin Source File

SOURCE=..\common\sim_icons.c
# End Source File
# Begin Source File

SOURCE=..\common\stubs.c
# End Source File
# End Group
# Begin Source File

SOURCE=.\button.c
# End Source File
# Begin Source File

SOURCE=".\debug-win32.c"
# End Source File
# Begin Source File

SOURCE=".\dir-win32.c"
# End Source File
# Begin Source File

SOURCE="..\common\font-player.c"

!IF  "$(CFG)" == "rockbox - Win32 Recorder"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "rockbox - Win32 Player"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\io.c
# End Source File
# Begin Source File

SOURCE=.\kernel.c
# End Source File
# Begin Source File

SOURCE=.\lang.c
# End Source File
# Begin Source File

SOURCE=".\lcd-win32.c"
# End Source File
# Begin Source File

SOURCE=".\mpeg-win32.c"
# End Source File
# Begin Source File

SOURCE=".\panic-win32.c"
# End Source File
# Begin Source File

SOURCE=".\string-win32.c"
# End Source File
# Begin Source File

SOURCE=.\sysfont.c
# End Source File
# Begin Source File

SOURCE=".\thread-win32.c"
# End Source File
# Begin Source File

SOURCE=.\uisw32.c
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\uisw32.rc
# End Source File
# End Group
# Begin Group "Script inputs"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\firmware\fonts\clR6x8.bdf

!IF  "$(CFG)" == "rockbox - Win32 Recorder"

# Begin Custom Build - Creating font from $(InputPath)
InputPath=..\..\firmware\fonts\clR6x8.bdf

"sysfont.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\tools\convbdf -c -o sysfont.c $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "rockbox - Win32 Player"

# Begin Custom Build - Creating font from $(InputPath)
InputPath=..\..\firmware\fonts\clR6x8.bdf

"sysfont.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\tools\convbdf -c -o sysfont.c $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\docs\CREDITS

!IF  "$(CFG)" == "rockbox - Win32 Recorder"

# PROP Ignore_Default_Tool 1
# Begin Custom Build - Creating credits from $(InputPath)
InputPath=..\..\docs\CREDITS

"../../apps/credits.raw" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	perl ../../apps/credits.pl < $(InputPath) > ../../apps/credits.raw

# End Custom Build

!ELSEIF  "$(CFG)" == "rockbox - Win32 Player"

# PROP Ignore_Default_Tool 1
# Begin Custom Build - Creating credits from $(InputPath)
InputPath=..\..\docs\CREDITS

"../../apps/credits.raw" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	perl ../../apps/credits.pl < $(InputPath) > ../../apps/credits.raw

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\apps\lang\english.lang

!IF  "$(CFG)" == "rockbox - Win32 Recorder"

# Begin Custom Build - Creating language files from $(InputPath)
InputPath=..\..\apps\lang\english.lang

BuildCmds= \
	perl -s ..\..\tools\genlang -p=lang $(InputPath)

"lang.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"lang.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "rockbox - Win32 Player"

# Begin Custom Build - Creating language files from $(InputPath)
InputPath=..\..\apps\lang\english.lang

BuildCmds= \
	perl -s ..\..\tools\genlang -p=lang $(InputPath)

"lang.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"lang.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
# End Group
# End Target
# End Project
