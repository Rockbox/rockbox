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
' Purpose: Make a voice clip file for the given text
' Parameters: $0 - text to convert
'             $1 - file to write spoken text into (WAV format)



'To be done:
' - Somehow, persist oSpVoice across multiple clips to increase speed
' - Allow user to override voice, speed and/or format (currently uses Control Panel defaults for voice/speed)
' - Voice specific replacements/corrections for pronounciation (this should be at a higher level really)

Const SSFMCreateForWrite = 3

Const SPSF_8kHz16BitMono = 6
Const SPSF_11kHz16BitMono = 10
Const SPSF_12kHz16BitMono = 14
Const SPSF_16kHz16BitMono = 18
Const SPSF_22kHz16BitMono = 22
Const SPSF_24kHz16BitMono = 26
Const SPSF_32kHz16BitMono = 30
Const SPSF_44kHz16BitMono = 34
Const SPSF_48kHz16BitMono = 38

Dim oSpVoice, oSpFS, nAudioFormat, sText, sOutputFile

sText = Replace(WScript.Arguments(0), "\", "")
sOutputFile = WScript.Arguments(1)
nAudioFormat = SPSF_22kHz16BitMono 'Audio format to use, recommended settings:
                                   '- for AT&T natural voices, use SPSF_32kHz16BitMono
                                   '- for MS voices, use SPSF_22kHz16BitMono

Set oSpVoice = CreateObject("SAPI.SpVoice")
If Err.Number <> 0 Then
    WScript.Echo "Error - could not get SpVoice object. " & _
                 "SAPI 5 not installed?"
    Err.Clear
    WScript.Quit 1
End If

Set oSpFS = CreateObject("SAPI.SpFileStream")
oSpFS.Format.Type = nAudioFormat
oSpFS.Open sOutputFile, SSFMCreateForWrite, False
Set oSpVoice.AudioOutputStream = oSpFS
oSpVoice.Speak sText
oSpFS.Close

Set oSpFS = Nothing
Set oSpVoice = Nothing
Set oArgs = Nothing
WScript.Quit 0
