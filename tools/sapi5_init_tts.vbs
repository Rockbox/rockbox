'***************************************************************************
'             __________               __   ___.
'   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
'   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
'   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
'   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
'                     \/            \/     \/    \/            \/
' $Id: sapi5_init_tts.vbs$
'
' Copyright (C) 2007 Steve Bavin, Jens Arnold, Mesar Hameed
'
' All files in this archive are subject to the GNU General Public License.
' See the file COPYING in the source tree root for full license agreement.
'
' This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
' KIND, either express or implied.
'
'***************************************************************************
Dim oSpVoice, sVoice

Set oSpVoice = CreateObject("SAPI.SpVoice")
If Err.Number <> 0 Then
    WScript.Echo "Error - could not get SpVoice object. " & _
                 "SAPI 5 not installed?"
    Err.Clear
    WScript.Quit 1
End If

WScript.Quit 0
