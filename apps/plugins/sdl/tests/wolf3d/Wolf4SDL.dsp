# Microsoft Developer Studio Project File - Name="Wolf4SDL" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=Wolf4SDL - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Wolf4SDL.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Wolf4SDL.mak" CFG="Wolf4SDL - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Wolf4SDL - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Wolf4SDL - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Wolf4SDL - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "c:\sdl-1.2.12\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib  SDL_mixer.lib SDL.lib /nologo /subsystem:windows /machine:I386 /libpath:"c:\sdl-1.2.12\lib"

!ELSEIF  "$(CFG)" == "Wolf4SDL - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "c:\sdl-1.2.12\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib SDL_mixer.lib SDL.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept /libpath:"c:\sdl-1.2.12\lib"

!ENDIF 

# Begin Target

# Name "Wolf4SDL - Win32 Release"
# Name "Wolf4SDL - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\fmopl.cpp
# End Source File
# Begin Source File

SOURCE=.\id_ca.cpp
# End Source File
# Begin Source File

SOURCE=.\id_in.cpp
# End Source File
# Begin Source File

SOURCE=.\id_pm.cpp
# End Source File
# Begin Source File

SOURCE=.\id_sd.cpp
# End Source File
# Begin Source File

SOURCE=.\id_us_1.cpp
# End Source File
# Begin Source File

SOURCE=.\id_vh.cpp
# End Source File
# Begin Source File

SOURCE=.\id_vl.cpp
# End Source File
# Begin Source File

SOURCE=.\sdl_winmain.cpp
# End Source File
# Begin Source File

SOURCE=.\signon.cpp
# End Source File
# Begin Source File

SOURCE=.\wl_act1.cpp
# End Source File
# Begin Source File

SOURCE=.\wl_act2.cpp
# End Source File
# Begin Source File

SOURCE=.\wl_agent.cpp
# End Source File
# Begin Source File

SOURCE=.\wl_atmos.cpp
# End Source File
# Begin Source File

SOURCE=.\wl_cloudsky.cpp
# End Source File
# Begin Source File

SOURCE=.\wl_debug.cpp
# End Source File
# Begin Source File

SOURCE=.\wl_dir3dspr.cpp
# End Source File
# Begin Source File

SOURCE=.\wl_draw.cpp
# End Source File
# Begin Source File

SOURCE=.\wl_floorceiling.cpp
# End Source File
# Begin Source File

SOURCE=.\wl_game.cpp
# End Source File
# Begin Source File

SOURCE=.\wl_inter.cpp
# End Source File
# Begin Source File

SOURCE=.\wl_main.cpp
# End Source File
# Begin Source File

SOURCE=.\wl_menu.cpp
# End Source File
# Begin Source File

SOURCE=.\wl_parallax.cpp
# End Source File
# Begin Source File

SOURCE=.\wl_play.cpp
# End Source File
# Begin Source File

SOURCE=.\wl_shade.cpp
# End Source File
# Begin Source File

SOURCE=.\wl_state.cpp
# End Source File
# Begin Source File

SOURCE=.\wl_text.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\audiosod.h
# End Source File
# Begin Source File

SOURCE=.\audiowl6.h
# End Source File
# Begin Source File

SOURCE=.\dc_vmu.h
# End Source File
# Begin Source File

SOURCE=.\f_spear.h
# End Source File
# Begin Source File

SOURCE=.\fmopl.h
# End Source File
# Begin Source File

SOURCE=.\foreign.h
# End Source File
# Begin Source File

SOURCE=.\gfxv_apo.h
# End Source File
# Begin Source File

SOURCE=.\gfxv_sod.h
# End Source File
# Begin Source File

SOURCE=.\gfxv_wl6.h
# End Source File
# Begin Source File

SOURCE=.\id_ca.h
# End Source File
# Begin Source File

SOURCE=.\id_in.h
# End Source File
# Begin Source File

SOURCE=.\id_pm.h
# End Source File
# Begin Source File

SOURCE=.\id_sd.h
# End Source File
# Begin Source File

SOURCE=.\id_us.h
# End Source File
# Begin Source File

SOURCE=.\id_vh.h
# End Source File
# Begin Source File

SOURCE=.\id_vl.h
# End Source File
# Begin Source File

SOURCE=.\version.h
# End Source File
# Begin Source File

SOURCE=.\wl_atmos.h
# End Source File
# Begin Source File

SOURCE=.\wl_cloudsky.h
# End Source File
# Begin Source File

SOURCE=.\wl_def.h
# End Source File
# Begin Source File

SOURCE=.\wl_menu.h
# End Source File
# Begin Source File

SOURCE=.\wl_shade.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
