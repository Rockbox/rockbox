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

Dim oShell, oArgs, oEnv
Dim bVerbose, bSAPI4
Dim sLanguage, sVoice, sSpeed

Dim oSpVoice, oSpFS ' SAPI5 voice and filestream
Dim oTTS, nMode ' SAPI4 TTS object, mode selector
Dim aLangIDs, sLangID, sSelectString

Dim aLine, aData    ' used in command reading


On Error Resume Next

Set oShell = CreateObject("WScript.Shell")
Set oEnv = oShell.Environment("Process")
bVerbose = (oEnv("V") <> "")

Set oArgs = WScript.Arguments.Named
bSAPI4 = oArgs.Exists("sapi4")
sLanguage = oArgs.Item("language")
sVoice = oArgs.Item("voice")
sRate = oArgs.Item("speed")

If bSAPI4 Then
    ' Create SAPI4 ActiveVoice object
    Set oTTS = WScript.CreateObject("ActiveVoice.ActiveVoice", "TTS_")
    If Err.Number <> 0 Then
        Err.Clear
        Set oTTS = WScript.CreateObject("ActiveVoice.ActiveVoice.1", "TTS_")
        If Err.Number <> 0 Then
            WScript.StdErr.WriteLine "Error - could not get ActiveVoice" _
                                     & " object. SAPI 4 not installed?"
            WScript.Quit 1
        End If
    End If
    oTTS.Initialized = 1

    ' Select matching voice
    aLangIDs = LangIDs(sLanguage)
    For Each sLangID in aLangIDs
        sLangID = HexToDec(sLangID) ' SAPI4 wants it decimal
        sSelectString = "LanguageID=" & sLangID
        If sVoice <> "" Then
            sSelectString = sSelectString & ";Speaker=" & sVoice _
                            & ";ModeName=" & sVoice
        End If
        nMode = oTTS.Find(sSelectString)
        If oTTS.LanguageID(nMode) = sLangID And (sVoice = "" Or _
           oTTS.Speaker(nMode) = sVoice Or oTTS.ModeName(nMode) = sVoice) Then
            If bVerbose Then WScript.StdErr.WriteLine "Using " & sSelectString
            Exit For
        Else
            sSelectString = ""
        End If
    Next
    If sSelectString = "" Then
        WScript.StdErr.WriteLine "Error - found no matching voice for " _
                                 & sLanguage & ", " & sVoice
        WScript.Quit 1
    End If
    oTTS.Select nMode

    ' Speed selection
    If sRate <> "" Then oSpVoice.Speed = sSpeed
Else ' SAPI5
    ' Create SAPI5 object
    Set oSpVoice = CreateObject("SAPI.SpVoice")
    If Err.Number <> 0 Then
        WScript.StdErr.WriteLine "Error - could not get SpVoice object." _
                                 & " SAPI 5 not installed?"
        WScript.Quit 1
    End If

    ' Select matching voice
    aLangIDs = LangIDs(sLanguage)
    For Each sLangID in aLangIDs
        sSelectString = "Language=" & sLangID
        If sVoice <> "" Then
            sSelectString = sSelectString & ";Name=" & sVoice
        End If
        Set oSpVoice.Voice = oSpVoice.GetVoices(sSelectString).Item(0)
        If Err.Number = 0 Then
            If bVerbose Then WScript.StdErr.WriteLine "Using " & sSelectString
            Exit For
        Else
            sSelectString = ""
            Err.Clear
        End If
    Next
    If sSelectString = "" Then
        WScript.StdErr.WriteLine "Error - found no matching voice for " _
                                 & sLanguage & ", " & sVoice
        WScript.Quit 1
    End If

    ' Speed selection
    If sRate <> "" Then oSpVoice.Rate = sSpeed

    ' Filestream object for output
    Set oSpFS = CreateObject("SAPI.SpFileStream")
    oSpFS.Format.Type = AudioFormat(oSpVoice.Voice.GetAttribute("Vendor"))
End If

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
            If bSAPI4 Then
                oTTS.FileName = aData(0)
                oTTS.Speak aData(1)
                While oTTS.Speaking
                    WScript.Sleep 100
                Wend
                oTTS.FileName = ""
            Else
                oSpFS.Open aData(0), SSFMCreateForWrite, false
                Set oSpVoice.AudioOutputStream = oSpFS
                oSpVoice.Speak aData(1)
                oSpFS.Close
            End If
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

' Subroutines
' -----------

' SAPI5 output format selection based on engine
Function AudioFormat(sVendor)
    Select Case sVendor
        Case "Microsoft"
            AudioFormat = SPSF_22kHz16BitMono
        Case "AT&T Labs"
            AudioFormat = SPSF_32kHz16BitMono
        Case Else
            AudioFormat = SPSF_22kHz16BitMono
            WScript.StdErr.WriteLine "Warning - unknown vendor """ & sVendor _
                                     & """ - using default wave format"
    End Select
End Function

Function HexToDec(sHex)
    Dim i, nDig

    HexToDec = 0
    For i = 1 To Len(sHex)
        nDig = InStr("0123456789abcdef", LCase(Mid(sHex, i, 1))) - 1
        HexToDec = 16 * HexToDec + nDig
    Next
End Function

' Language mapping rockbox->windows (hex strings as needed by SAPI)
Function LangIDs(sLanguage)
    Dim aIDs

    Select Case sLanguage
        Case "afrikaans"
            LangIDs = Array("436")
        Case "bulgarian"
            LangIDs = Array("402")
        Case "catala"
            LangIDs = Array("403")
        Case "chinese-simp"
            LangIDs = Array("804") ' PRC
        Case "chinese-trad"
            LangIDs = Array("404") ' Taiwan. Perhaps also Hong Kong, Singapore, Macau?
        Case "czech"
            LangIDs = Array("405")
        Case "dansk"
            LangIDs = Array("406")
        Case "deutsch"
            LangIDs = Array("407", "c07", "1007", "1407") 
            ' Standard, Austrian, Luxembourg, Liechtenstein (Swiss -> wallisertitsch)
        Case "eesti"
            LangIDs = Array("425")
        Case "english"
            LangIDs = Array("809", "409", "c09", "1009", "1409", "1809", _
                            "1c09", "2009", "2409", "2809", "2c09", "3009", _
                            "3409")
            ' Britsh, American, Australian, Canadian, New Zealand, Ireland,
            ' South Africa, Jamaika, Caribbean, Belize, Trinidad, Zimbabwe,
            ' Philippines
        Case "espanol"
            LangIDs = Array("40a", "c0a", "80a", "100a", "140a", "180a", _
                            "1c0a", "200a", "240a", "280a", "2c0a", "300a", _
                            "340a", "380a", "3c0a", "400a", "440a", "480a", _
                            "4c0a", "500a")
            ' trad. sort., mordern sort., Mexican, Guatemala, Costa Rica,
            ' Panama, Dominican Republic, Venezuela, Colombia, Peru, Argentina,
            ' Ecuador, Chile, Uruguay, Paraguay, Bolivia, El Salvador,
            ' Honduras, Nicaragua, Puerto Rico
        Case "esperanto"
            WScript.StdErr.WriteLine "Error: no esperanto support in Windows"
            WScript.Quit 1
        Case "finnish"
            LangIDs = Array("40b")
        Case "francais"
            LangIDs = Array("40c", "80c", "c0c", "100c", "140c", "180c")
            ' Standard, Belgian, Canadian, Swiss, Luxembourg, Monaco
        Case "galego"
            LangIDs = Array("456")
        Case "greek"
            LangIDs = Array("408")
        Case "hebrew"
            LangIDs = Array("40d")
        Case "islenska"
            LangIDs = Array("40f")
        Case "italiano"
            LangIDs = Array("410", "810") ' Standard, Swiss
        Case "japanese"
            LangIDs = Array("411")
        Case "korean"
            LangIDs = Array("412")
        Case "magyar"
            LangIDs = Array("40e")
        Case "nederlands"
            LangIDs = Array("413", "813") ' Standard, Belgian
        Case "norsk"
            LangIDs = Array("414") ' Bokmal
        Case "norsk-nynorsk"
            LangIDs = Array("814")
        Case "polski"
            LangIDs = Array("415")
        Case "portugues"
            LangIDs = Array("816", "416") ' Standard, Brazilian
        Case "romaneste"
            LangIDs = Array("418")
        Case "russian"
            LangIDs = Array("419")
        Case "slovenscina"
            LangIDs = Array("424")
        Case "svenska"
            LangIDs = Array("41d", "81d") ' Standard, Finland
        Case "turkce"
            LangIDs = Array("41f")
        Case "wallisertitsch"
            LangIDs = Array("807") ' Swiss German
    End Select
End Function
