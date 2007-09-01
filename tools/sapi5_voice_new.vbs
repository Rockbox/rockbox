'***************************************************************************
'             __________               __   ___.
'   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
'   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
'   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
'   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
'                     \/            \/     \/    \/            \/
' $Id: sapi5_voice.vbs$
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
' Purpose: Make a voice clip file for the given text on stdin

'To be done:
' - Allow user to override voice, speed and/or format (currently uses Control Panel defaults for voice/speed)

Option Explicit

Const SSFMCreateForWrite = 3

' Audio formats for SAPI5 filestream object
Const SPSF_8kHz16BitMono  = 6
Const SPSF_11kHz16BitMono = 10
Const SPSF_12kHz16BitMono = 14
Const SPSF_16kHz16BitMono = 18
Const SPSF_22kHz16BitMono = 22
Const SPSF_24kHz16BitMono = 26
Const SPSF_32kHz16BitMono = 30
Const SPSF_44kHz16BitMono = 34
Const SPSF_48kHz16BitMono = 38

Dim oShell, oEnv
Dim oSpVoice, oSpFS ' SAPI5 voice and filestream
Dim aLine, aData    ' used in command reading
Dim nAudioFormat   
Dim bVerbose


On Error Resume Next

nAudioFormat = SPSF_22kHz16BitMono 'Audio format to use, recommended settings:
'- for AT&T natural voices, use SPSF_32kHz16BitMono
'- for MS voices, use SPSF_22kHz16BitMono

Set oShell = CreateObject("WScript.Shell")
Set oEnv = oShell.Environment("Process")
bVerbose = (oEnv("V") <> "")

Set oSpVoice = CreateObject("SAPI.SpVoice")
If Err.Number <> 0 Then
    WScript.StdErr.WriteLine "Error - could not get SpVoice object. " & _
                             "SAPI 5 not installed?"
    Err.Clear
    WScript.Quit 1
End If

Set oSpFS = CreateObject("SAPI.SpFileStream")
oSpFS.Format.Type = nAudioFormat

On Error Goto 0

Do
    aLine = Split(WScript.StdIn.ReadLine, vbTab, 2)
    If Err.Number <> 0 Then
        WScript.StdErr.WriteLine "Error " & Err.Number & ": " & Err.Description
        WScript.Quit 1
    End If
    Select Case aLine(0) ' command
        Case "SPEAK"
            aData = Split(aLine(1), vbTab, 2)
            If bVerbose Then WScript.StdErr.WriteLine "Saying " & aData(1) _
                                                      & " in " & aData(0)
            oSpFS.Open aData(0), SSFMCreateForWrite, false
            Set oSpVoice.AudioOutputStream = oSpFS
            oSpVoice.Speak aData(1)
            oSpFS.Close
        Case "EXEC"
            If bVerbose Then WScript.StdErr.WriteLine "> " & aLine(1)
            oShell.Run aLine(1), 0, true
        Case "SYNC"
            If bVerbose Then WScript.StdErr.WriteLine "Syncing"
            WScript.StdOut.WriteLine aLine(1) ' Just echo what was passed
        Case "QUIT"
            If bVerbose Then WScript.StdErr.WriteLine "Quitting"
            WScript.Quit 0
    End Select
Loop
