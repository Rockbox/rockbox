;             __________               __   ___.
;   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
;   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
;   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
;   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
;                     \/            \/     \/    \/            \/
; $Id$
;
; Copyright (c) 2010 Dominik Riebeling
;
; All files in this archive are subject to the GNU General Public License.
; See the file COPYING in the source tree root for full license agreement.
;
; This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
; KIND, either express or implied.
;

; NSIS installer using Modern UI
; Expects a static build of the Theme Editor (Qt DLLs are not packaged) and to
; find the input files in the source tree (in-tree build).
; This installer uses the ZipDll plugin for font pack extraction
; (http://nsis.sourceforge.net/ZipDLL_plug-in)

!include "MUI2.nsh"

;Name and file
Name "Rockbox Theme Editor"
OutFile "rbthemeeditor-setup.exe"

;Default installation folder
InstallDir "$PROGRAMFILES\Rockbox Theme Editor"

; global registry shortcuts
!define UNINSTALL_HIVE "Software\Microsoft\Windows\CurrentVersion\Uninstall\Rockbox Theme Editor"
!define SETTINGS_HIVE "Software\rockbox.org\Rockbox Theme Editor"

;Get installation folder from registry if available
InstallDirRegKey HKCU "${SETTINGS_HIVE}" ""

SetCompressor /solid lzma
VIAddVersionKey "ProductName" "Rockbox Theme Editor"
VIAddVersionKey "FileVersion" "0.0.0"
VIAddVersionKey "FileDescription" "Editor for the Rockbox Firmware Theme files"
VIAddVersionKey "LegalCopyright" "Contributing Developers"
VIProductVersion "0.0.0.0"
!define MUI_ICON resources\windowicon.ico
; embed XP manifest
XPStyle on

;Interface Configuration. Use Rockbox blue for header.
!define MUI_BGCOLOR b6c6e5
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "themeeditor-setup.bmp"
!define MUI_HEADERIMAGE_RIGHT
!define MUI_ABORTWARNING

;Pages
!insertmacro MUI_PAGE_LICENSE "..\..\docs\COPYING"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

;Languages
!insertmacro MUI_LANGUAGE "English"

;Installer Sections
Section "Theme Editor" SecThemeEditor
    SectionSetFlags ${SecThemeEditor} ${SF_RO}
    SectionIn RO
    SetOutPath "$INSTDIR"
    ; Store installation folder
    WriteRegStr HKCU "${SETTINGS_HIVE}" "" $INSTDIR
    ; files
    CreateDirectory "$INSTDIR"
    File /oname=$INSTDIR\rbthemeeditor.exe release\rbthemeeditor.exe

    ; Create uninstaller and uninstall information
    WriteUninstaller "$INSTDIR\Uninstall.exe"
    WriteRegStr HKLM "${UNINSTALL_HIVE}" "DisplayName" "Rockbox Theme Editor"
    WriteRegStr HKLM "${UNINSTALL_HIVE}" "UninstallString" "$\"$INSTDIR\Uninstall.exe$\""
    WriteRegStr HKLM "${UNINSTALL_HIVE}" "QuietUninstallString" "$\"$INSTDIR\Uninstall.exe$\" /S"
    WriteRegStr HKLM "${UNINSTALL_HIVE}" "URLInfoAbout" "http://www.rockbox.org/wiki/ThemeEditor"
SectionEnd

Section "Download Fonts Package" SecFontsPackage
    SetOutPath "$INSTDIR"
    NSISdl::download http://download.rockbox.org/daily/fonts/rockbox-fonts.zip "$INSTDIR\rockbox-fonts.zip"
    ZipDLL::extractall "$INSTDIR\rockbox-fonts.zip" "$INSTDIR" "<ALL>"
    ; the fonts package uses the Rockbox folder structure. Move all fonts into a
    ; flat directory instead.
    Rename "$INSTDIR\.rockbox\fonts" "$INSTDIR\fonts"
    RMDir /r "$INSTDIR\.rockbox"
    WriteRegStr HKCU "${SETTINGS_HIVE}\RBFont" "fontDir" "$INSTDIR\fonts\"
SectionEnd

Section "Start Menu Shortcut" SecShortCuts
    CreateDirectory "$SMPROGRAMS\Rockbox"
    CreateShortCut "$SMPROGRAMS\Rockbox\Theme Editor.lnk" "$INSTDIR\rbthemeeditor.exe"
    CreateShortCut "$SMPROGRAMS\Rockbox\Uninstall Theme Editor.lnk" "$INSTDIR\Uninstall.exe"
SectionEnd

; Assign language strings to sections
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
!insertmacro MUI_DESCRIPTION_TEXT ${SecThemeEditor} \
    "Rockbox Theme Editor. Required."
!insertmacro MUI_DESCRIPTION_TEXT ${SecFontsPackage} \
    "Download and install the Rockbox Fonts package into program folder. \
     This will set the correct path in the program configuration for the current user only."
!insertmacro MUI_DESCRIPTION_TEXT ${SecShortCuts} \
    "Create Shortcut in Start Menu."
!insertmacro MUI_FUNCTION_DESCRIPTION_END

; Uninstaller Section
Section "Uninstall"
    ; files
    Delete "$INSTDIR\Uninstall.exe"
    Delete "$INSTDIR\rbthemeeditor.exe"
    Delete "$INSTDIR\rockbox-fonts.zip"
    ; folders
    RMDir /r "$INSTDIR\fonts"
    RMDir "$INSTDIR"
    ; start menu folder
    RMDir /r "$SMPROGRAMS\Rockbox"

    ; remove registry information
    DeleteRegKey HKLM "${UNINSTALL_HIVE}"
    DeleteRegKey HKCU "Software\rockbox.org\Rockbox Theme Editor"
SectionEnd

