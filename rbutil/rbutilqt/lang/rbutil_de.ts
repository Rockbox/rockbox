<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.0" language="de">
<context>
    <name>BootloaderInstallAms</name>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="32"/>
        <source>Bootloader installation requires you to provide a copy of the original Sandisk firmware (bin file). This firmware file will be patched and then installed to your player along with the rockbox bootloader. You need to download this file yourself due to legal reasons. Please browse the &lt;a href=&apos;http://forums.sandisk.com/sansa/&apos;&gt;Sansa Forums&apos;&lt;/a&gt; or refer to the &lt;a href=&apos;http://www.rockbox.org/manual.shtml&apos;&gt;manual&lt;/a&gt; and the &lt;a href=&apos;http://www.rockbox.org/wiki/SansaAMS&apos;&gt;SansaAMS&lt;/a&gt; wiki page on how to obtain this file.&lt;br/&gt;Press Ok to continue and browse your computer for the firmware file.</source>
        <translation type="unfinished">Die Bootloader-Installation erfordert eine Datei der Originalfirmware (bin-Datei). Die Firmware-Datei wird angepasst und auf dem Gerät mit dem Rockbox-Bootloader installiert. Aus rechtlichen Gründen muss diese Datei separat heruntergeladen werden. Diese Datei ist im &lt;a href=&apos;http://forums.sandisk.com/sansa/&apos;&gt;Sansa-Forum&lt;/a&gt; zu finden sowie im &lt;a href=&apos;http://www.rockbox.org/manual.shtml&apos;&gt;Handbuch&lt;/a&gt; und auf der &lt;a href=&apos;http://www.rockbox.org/wiki/SansaAMS&apos;&gt;SansaAMS&lt;/a&gt; Wiki-Seite beschrieben.&lt;br/&gt;OK um fortzufahren und die Datei auf dem Computer auszuwählen.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="55"/>
        <source>Downloading bootloader file</source>
        <translation type="unfinished">Lade Bootloader-Datei herunter</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="97"/>
        <location filename="../base/bootloaderinstallams.cpp" line="110"/>
        <source>Could not load %1</source>
        <translation>Konnte %1 nicht laden</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="124"/>
        <source>No room to insert bootloader, try another firmware version</source>
        <translation type="unfinished">Kein Platz um den Bootloader einzufügen. Bitte andere Firmware-Version probieren</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="134"/>
        <source>Patching Firmware...</source>
        <translation type="unfinished">Patche Firmware ...</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="145"/>
        <source>Could not open %1 for writing</source>
        <translation>Konnte %1 nicht zum schreiben öffnen</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="158"/>
        <source>Could not write firmware file</source>
        <translation>Konnte Firmware-Datei nicht schreiben</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="174"/>
        <source>Success: modified firmware file created</source>
        <translation type="unfinished">Erfolg: modifizierte Firmware-Datei erzeugt</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="182"/>
        <source>To uninstall, perform a normal upgrade with an unmodified original firmware</source>
        <translation>Zum deinstallieren ein Upgrade mit einer unveränderten Originalfirmware-Datei durchführen</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallBase</name>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="124"/>
        <source>Download error: received HTTP error %1.</source>
        <translation>Fehler beim Herunterladen: HTTP Fehler %1.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="130"/>
        <source>Download error: %1</source>
        <translation>Fehler beim Herunterladen: %1</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="136"/>
        <source>Download finished (cache used).</source>
        <translation>Download abgeschlossen (Cache verwendet).</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="138"/>
        <source>Download finished.</source>
        <translation>Download abgeschlossen.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="159"/>
        <source>Creating backup of original firmware file.</source>
        <translation>Erzeuge Sicherungskopie der Original-Firmware.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="161"/>
        <source>Creating backup folder failed</source>
        <translation>Erzeugen des Sicherungskopie-Ordners fehlgeschlagen</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="167"/>
        <source>Creating backup copy failed.</source>
        <translation>Erzeugen der Sicherungskopie fehlgeschlagen.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="170"/>
        <source>Backup created.</source>
        <translation>Sicherungskopie erzeugt.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="183"/>
        <source>Creating installation log</source>
        <translation>Erzeuge Installationslog</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="207"/>
        <source>Bootloader installation is almost complete. Installation &lt;b&gt;requires&lt;/b&gt; you to perform the following steps manually:</source>
        <translation>Installation des Bootloader ist fast abgeschlossen. Die Installation &lt;b&gt;benötigt&lt;/b&gt; die folgenden, manuell auszuführenden Schritte:</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="213"/>
        <source>&lt;li&gt;Safely remove your player.&lt;/li&gt;</source>
        <translation>&lt;li&gt;Gerät sicher entfernen.&lt;/li&gt;</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="218"/>
        <source>&lt;li&gt;Reboot your player into the original firmware.&lt;/li&gt;&lt;li&gt;Perform a firmware upgrade using the update functionality of the original firmware. Please refer to your player&apos;s manual on details.&lt;br/&gt;&lt;b&gt;Important:&lt;/b&gt; updating the firmware is a critical process that must not be interrupted. &lt;b&gt;Make sure the player is charged before starting the firmware update process.&lt;/b&gt;&lt;/li&gt;&lt;li&gt;After the firmware has been updated reboot your player.&lt;/li&gt;</source>
        <translation type="unfinished">&lt;li&gt;Gerät mit der Original-Firmware starten.&lt;/li&gt;&lt;li&gt;EinFirmware-Update mit der Update-Funktion der Original-Firmware entsprechend der Anleitung des Geräts durchführen.&lt;/li&gt;&lt;b&gt;Wichtig:&lt;/b&gt;Das Firmware-Update ist ein kritischer Prozess der nicht unterbrochen werden darf. &lt;b&gt;Bitte darauf achten dass der Akku vor dem Starten des Updates geladen ist.&lt;/b&gt;&lt;/li&gt;&lt;li&gt;Nach Abschluß des Updates das Gerät neu starten.&lt;/li&gt;</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="229"/>
        <source>&lt;li&gt;Remove any previously inserted microSD card&lt;/li&gt;</source>
        <translation type="unfinished">&lt;li&gt;Eine eventuell eingelegte microSD-Karte entfernen&lt;/li&gt;</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="230"/>
        <source>&lt;li&gt;Disconnect your player. The player will reboot and perform an update of the original firmware. Please refer to your players manual on details.&lt;br/&gt;&lt;b&gt;Important:&lt;/b&gt; updating the firmware is a critical process that must not be interrupted. &lt;b&gt;Make sure the player is charged before disconnecting the player.&lt;/b&gt;&lt;/li&gt;&lt;li&gt;After the firmware has been updated reboot your player.&lt;/li&gt;</source>
        <translation type="unfinished">&lt;li&gt;Das Gerät entfernen. Es wird einen Neustart und ein Update der Original-Firmware durchführen. Für Details bitte das Handbuch des Gerätes beachten.&lt;br/&gt;&lt;b&gt;Wichtig:&lt;/b&gt;Das Firmware-Update ist ein kritischer Prozess der nicht unterbrochen werden darf. &lt;b&gt;Unbedingt vor dem Trennen darauf achten dass das Gerät aufgeladen ist.&lt;/b&gt;&lt;/li&gt;&lt;li&gt;Nach Abschluß des Updates das Gerät neu starten.&lt;/li&gt;</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="329"/>
        <source>Zip file format detected</source>
        <translation type="unfinished">Zip-Format erkannt</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="339"/>
        <source>Extracting firmware %1 from archive</source>
        <translation type="unfinished">Entpacke Firmware %1 aus Archiv</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="346"/>
        <source>Error extracting firmware from archive</source>
        <translation type="unfinished">Fehler beim Extrahieren der Firmwaredatei</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="353"/>
        <source>Could not find firmware in archive</source>
        <translation type="unfinished">Konnte Firmware nicht im Archiv finden</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="241"/>
        <source>&lt;li&gt;Turn the player off&lt;/li&gt;&lt;li&gt;Insert the charger&lt;/li&gt;</source>
        <translation>&lt;li&gt;Gerät ausschalten&lt;/li&gt;&lt;li&gt;Ladegerät anstecken&lt;/li&gt;</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="246"/>
        <source>&lt;li&gt;Unplug USB and power adaptors&lt;/li&gt;&lt;li&gt;Hold &lt;i&gt;Power&lt;/i&gt; to turn the player off&lt;/li&gt;&lt;li&gt;Toggle the battery switch on the player&lt;/li&gt;&lt;li&gt;Hold &lt;i&gt;Power&lt;/i&gt; to boot into Rockbox&lt;/li&gt;</source>
        <translation>&lt;li&gt;USB und Stromkabel abziehen&lt;/li&gt;&lt;li&gt;&lt;i&gt;Power&lt;/i&gt; gedrückt halten um das Gerät auszuschalten&lt;/li&gt;&lt;li&gt;Batterieschalter am Gerät umlegen&lt;/li&gt;&lt;li&gt;&lt;i&gt;Power&lt;/i&gt; halten um Rockbox zu booten&lt;/li&gt;</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="252"/>
        <source>&lt;p&gt;&lt;b&gt;Note:&lt;/b&gt; You can safely install other parts first, but the above steps are &lt;b&gt;required&lt;/b&gt; to finish the installation!&lt;/p&gt;</source>
        <translation>&lt;p&gt;&lt;b&gt;Hinweis:&lt;/b&gt; andere Teile von Rockbox können problemlos vorher installiert werden, aber die genannten Schritte sind &lt;b&gt;notwendig&lt;/b&gt; um die Installation abzuschließen!&lt;/p&gt;</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="266"/>
        <source>Waiting for system to remount player</source>
        <translation type="unfinished">Warte bis das Gerät wieder eingehängt ist</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="296"/>
        <source>Player remounted</source>
        <translation type="unfinished">Gerät wieder eingehängt</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="301"/>
        <source>Timeout on remount</source>
        <translation>Zeitüberschreitung beim Warten</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="195"/>
        <source>Installation log created</source>
        <translation>Installationslog erzeugt</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallChinaChip</name>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="33"/>
        <source>Bootloader installation requires you to provide a firmware file of the original firmware (HXF file). You need to download this file yourself due to legal reasons. Please refer to the &lt;a href=&apos;http://www.rockbox.org/manual.shtml&apos;&gt;manual&lt;/a&gt; and the &lt;a href=&apos;http://www.rockbox.org/wiki/OndaVX747#Download_and_extract_a_recent_ve&apos;&gt;OndaVX747&lt;/a&gt; wiki page on how to obtain this file.&lt;br/&gt;Press Ok to continue and browse your computer for the firmware file.</source>
        <translation type="unfinished">Die Bootloader-Installation erfordert eine Firmware-Datei der Originalfirmware (HXF-Datei). Diese Datei muss aus rechtlichen Gründen separat heruntergeladen werden. Wie diese Datei zu beziehen ist ist im &lt;a href=&apos;http://www.rockbox.org/manual.shtml&apos;&gt;Handbuch&lt;/a&gt; und auf der &lt;a href=&apos;http://www.rockbox.org/wiki/OndaVX747#Download_and_extract_a_recent_ve&apos;&gt;OndaVX747&lt;/a&gt; Wiki-Seite beschrieben.&lt;br/&gt;OK um fortzufahren und die Datei auf dem Computer auszuwählen.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="50"/>
        <source>Downloading bootloader file</source>
        <translation type="unfinished">Lade Bootloader-Datei herunter</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="75"/>
        <source>Could not open firmware file</source>
        <translation type="unfinished">Konnte Firmware-Datei nicht öffnen</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="78"/>
        <source>Could not open bootloader file</source>
        <translation type="unfinished">Konnte Bootloader-Datei nicht öffnen</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="81"/>
        <source>Could not allocate memory</source>
        <translation type="unfinished">Konnte Speicher nicht allokieren</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="84"/>
        <source>Could not load firmware file</source>
        <translation type="unfinished">Konnte Firmware-Datei nicht laden</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="87"/>
        <source>File is not a valid ChinaChip firmware</source>
        <translation type="unfinished">Datei ist keine gültige ChinaChip-Firmware</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="90"/>
        <source>Could not find ccpmp.bin in input file</source>
        <translation type="unfinished">Konnte ccpmp.bin in Eingabedatei nicht finden</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="93"/>
        <source>Could not open backup file for ccpmp.bin</source>
        <translation type="unfinished">Konnte Backup-Date für ccpmp.bin nicht öffnen</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="96"/>
        <source>Could not write backup file for ccpmp.bin</source>
        <translation type="unfinished">Konnte Backup-Datei für ccpmp.bin nicht schreiben</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="99"/>
        <source>Could not load bootloader file</source>
        <translation type="unfinished">Konnte Bootloader-Datei nicht laden</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="102"/>
        <source>Could not get current time</source>
        <translation type="unfinished">Konnte aktuelle Zeit nicht lesen</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="105"/>
        <source>Could not open output file</source>
        <translation type="unfinished">Konnte Ausgabedatei nicht öffnen</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="108"/>
        <source>Could not write output file</source>
        <translation type="unfinished">Konnte Ausgabedatei nicht schreiben</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="111"/>
        <source>Unexpected error from chinachippatcher</source>
        <translation type="unfinished">Unerwarteter Fehler von chinachippatcher</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallFile</name>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="33"/>
        <source>Downloading bootloader</source>
        <translation>Lade Bootloader herunter</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="42"/>
        <source>Installing Rockbox bootloader</source>
        <translation>Installiere Rockbox Bootloader</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="74"/>
        <source>Error accessing output folder</source>
        <translation>Fehler beim Zugriff auf den Ausgabeordner</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="87"/>
        <source>Bootloader successful installed</source>
        <translation>Bootloader erfolgreich installiert</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="97"/>
        <source>Removing Rockbox bootloader</source>
        <translation>Entferne Rockbox Bootloader</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="101"/>
        <source>No original firmware file found.</source>
        <translation>Keine Original-Firmware gefunden.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="107"/>
        <source>Can&apos;t remove Rockbox bootloader file.</source>
        <translation>Kann Rockbox Bootloader-Datei nicht entfernen.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="112"/>
        <source>Can&apos;t restore bootloader file.</source>
        <translation>Kann Bootloader-Datei nicht wiederherstellen.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="116"/>
        <source>Original bootloader restored successfully.</source>
        <translation>Original-Bootloader erfolgreich wiederhergestellt.</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallHex</name>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="67"/>
        <source>checking MD5 hash of input file ...</source>
        <translation>prüfe MD5-Hash der Eingabedatei ...</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="78"/>
        <source>Could not verify original firmware file</source>
        <translation>Konnte Originalfirmware-Datei nicht prüfen</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="93"/>
        <source>Firmware file not recognized.</source>
        <translation>Firmware-Datei nicht erkannt.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="97"/>
        <source>MD5 hash ok</source>
        <translation>MD5-Hash ok</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="104"/>
        <source>Firmware file doesn&apos;t match selected player.</source>
        <translation>Firmware passt nicht zum gewählten Gerät.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="109"/>
        <source>Descrambling file</source>
        <translation>Descramble Datei</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="117"/>
        <source>Error in descramble: %1</source>
        <translation>Fehler bei Descramble: %1</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="122"/>
        <source>Downloading bootloader file</source>
        <translation>Lade Bootloader-Datei herunter</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="132"/>
        <source>Adding bootloader to firmware file</source>
        <translation>Füge Bootloader zu Firmware-Datei hinzu</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="170"/>
        <source>could not open input file</source>
        <translation>Konnte die Eingabedatei nicht öffnen</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="171"/>
        <source>reading header failed</source>
        <translation>Konnte Header nicht lesen</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="172"/>
        <source>reading firmware failed</source>
        <translation>Konnte Firmware nicht lesen</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="173"/>
        <source>can&apos;t open bootloader file</source>
        <translation>Konnte Bootloader nicht öffnen</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="174"/>
        <source>reading bootloader file failed</source>
        <translation>Konnte Bootloader nicht lesen</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="175"/>
        <source>can&apos;t open output file</source>
        <translation>Konnte Ausgabedatei nicht öffnen</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="176"/>
        <source>writing output file failed</source>
        <translation>Konnte Ausgabedatei nicht schreiben</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="178"/>
        <source>Error in patching: %1</source>
        <translation>Fehler beim Patchen %1</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="189"/>
        <source>Error in scramble: %1</source>
        <translation>Fehler bei Scramble: %1</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="204"/>
        <source>Checking modified firmware file</source>
        <translation>Prüfe modifizierte Firmware-Datei</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="206"/>
        <source>Error: modified file checksum wrong</source>
        <translation>Fehler: Prüfsumme der modifizierten Datei falsch</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="214"/>
        <source>Success: modified firmware file created</source>
        <translation>Erfolg: modifizierte Firmware-Datei erzeugt</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="224"/>
        <source>Uninstallation not possible, only installation info removed</source>
        <translation type="unfinished">Deinstallation nicht möglich, Installationsinformationen entfernt</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="245"/>
        <source>Can&apos;t open input file</source>
        <translation>Konnte Eingabedatei nicht öffnen</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="246"/>
        <source>Can&apos;t open output file</source>
        <translation>Konnte Ausgabedatei nicht öffnen</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="247"/>
        <source>invalid file: header length wrong</source>
        <translation>ungültige Datei: Länge des Headers ist falsch</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="248"/>
        <source>invalid file: unrecognized header</source>
        <translation>ungültige Datei: unbekannter Header</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="249"/>
        <source>invalid file: &quot;length&quot; field wrong</source>
        <translation>ungültige Datei: &quot;length&quot; Eintrag ist falsch</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="250"/>
        <source>invalid file: &quot;length2&quot; field wrong</source>
        <translation>ungültige Datei: &quot;length2&quot; Eintrag ist falsch</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="251"/>
        <source>invalid file: internal checksum error</source>
        <translation>ungültige Datei: interne Prüfsumme ist falsch</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="252"/>
        <source>invalid file: &quot;length3&quot; field wrong</source>
        <translation>ungültige Datei: &quot;length3&quot; Eintrag ist falsch</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="253"/>
        <source>unknown</source>
        <translation>unbekannt</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="48"/>
        <source>Bootloader installation requires you to provide a firmware file of the original firmware (hex file). You need to download this file yourself due to legal reasons. Please refer to the &lt;a href=&apos;http://www.rockbox.org/manual.shtml&apos;&gt;manual&lt;/a&gt; and the &lt;a href=&apos;http://www.rockbox.org/wiki/IriverBoot#Download_and_extract_a_recent_ve&apos;&gt;IriverBoot&lt;/a&gt; wiki page on how to obtain this file.&lt;br/&gt;Press Ok to continue and browse your computer for the firmware file.</source>
        <translation>Die Bootloader-Installation benötigt eine Firmware-Datei der originalen Firmware (Hex-Datei). Diese Datei muss aus rechtlichen Gründen separat heruntergeladen werden. Informationen wie diese Datei heruntergeladen werden kann sind im &lt;a href=&apos;http://www.rockbox.org/manual.shtml&apos;&gt;Handbuch&lt;/a&gt; und der Wiki-Seite &lt;a href=&apos;http://www.rockbox.org/wiki/IriverBoot#Download_and_extract_a_recent_ve&apos;&gt;IriverBoot&lt;/a&gt; aufgeführt.&lt;br/&gt;OK um fortzufahren und die Datei auf dem Computer auszuwählen.</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallImx</name>
    <message>
        <location filename="../base/bootloaderinstallimx.cpp" line="69"/>
        <source>Bootloader installation requires you to provide a copy of the original Sandisk firmware (firmware.sb file). This file will be patched with the Rockbox bootloader and installed to your player. You need to download this file yourself due to legal reasons. Please browse the &lt;a href=&apos;http://forums.sandisk.com/sansa/&apos;&gt;Sansa Forums&lt;/a&gt; or refer to the &lt;a href= &apos;http://www.rockbox.org/wiki/SansaFuzePlus&apos;&gt;SansaFuzePlus&lt;/a&gt; wiki page on how to obtain this file.&lt;br/&gt;Press Ok to continue and browse your computer for the firmware file.</source>
        <translation type="unfinished">Die Bootloader-Installation erfordert eine Datei der Originalfirmware (firmware.sb-Datei). Die Firmware-Datei wird angepasst und auf dem Gerät mit dem Rockbox-Bootloader installiert. Aus rechtlichen Gründen muss diese Datei separat heruntergeladen werden. Diese Datei ist im &lt;a href=&apos;http://forums.sandisk.com/sansa/&apos;&gt;Sansa-Forum&lt;/a&gt; zu finden und auf der &lt;a href=&apos;http://www.rockbox.org/wiki/SansaFuzePlus&apos;&gt;SansaFuzePlus&lt;/a&gt; Wiki-Seite beschrieben.&lt;br/&gt;OK um fortzufahren und die Datei auf dem Computer auszuwählen.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallimx.cpp" line="91"/>
        <source>Could not read original firmware file</source>
        <translation type="unfinished">Konnte Original-Firmware-Datei nicht lesen</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallimx.cpp" line="97"/>
        <source>Downloading bootloader file</source>
        <translation type="unfinished">Lade Bootloader-Datei herunter</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallimx.cpp" line="107"/>
        <source>Patching file...</source>
        <translation type="unfinished">Patche Firmware ...</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallimx.cpp" line="134"/>
        <source>Patching the original firmware failed</source>
        <translation type="unfinished">Modifizieren der Firmware-Datei fehlgeschlagen</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallimx.cpp" line="140"/>
        <source>Succesfully patched firmware file</source>
        <translation type="unfinished">Firmware-Datei erfolgreich modifiziert</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallimx.cpp" line="155"/>
        <source>Bootloader successful installed</source>
        <translation type="unfinished">Bootloader erfolgreich installiert</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallimx.cpp" line="161"/>
        <source>Patched bootloader could not be installed</source>
        <translation type="unfinished">Modifizierter Bootloader konnte nicht installiert werden</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallimx.cpp" line="172"/>
        <source>To uninstall, perform a normal upgrade with an unmodified original firmware.</source>
        <translation type="unfinished">Zum deinstallieren ein Upgrade mit einer unveränderten Originalfirmware-Datei durchführen.</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallIpod</name>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="53"/>
        <source>Error: can&apos;t allocate buffer memory!</source>
        <translation>Fehler: kann Speicher nicht allokieren!</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="81"/>
        <source>Downloading bootloader file</source>
        <translation>Lade Bootloader-Datei herunter</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="65"/>
        <location filename="../base/bootloaderinstallipod.cpp" line="152"/>
        <source>Failed to read firmware directory</source>
        <translation>Konnte Firmwareverzeichnis nicht lesen</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="70"/>
        <location filename="../base/bootloaderinstallipod.cpp" line="157"/>
        <source>Unknown version number in firmware (%1)</source>
        <translation>Unbekannte Versionsnummer in Firmware (%1)</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="76"/>
        <source>Warning: This is a MacPod, Rockbox only runs on WinPods. 
See http://www.rockbox.org/wiki/IpodConversionToFAT32</source>
        <translation>Warnung: Dies ist ein MacPod, Rockbox läuft nur auf WinPods.
Siehe http://www.rockbox.org/wiki/IpodConversionToFAT32</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="95"/>
        <location filename="../base/bootloaderinstallipod.cpp" line="164"/>
        <source>Could not open Ipod in R/W mode</source>
        <translation>Kann Ipod nicht im R/W-Modus öffnen</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="105"/>
        <source>Successfull added bootloader</source>
        <translation>Bootloader erfolgreich hinzugefügt</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="116"/>
        <source>Failed to add bootloader</source>
        <translation>Konnte Bootloader nicht hinzufügen</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="128"/>
        <source>Bootloader Installation complete.</source>
        <translation>Bootloader-Installation vollständig.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="133"/>
        <source>Writing log aborted</source>
        <translation>Schreiben der Log-Datei abgebrochen</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="170"/>
        <source>No bootloader detected.</source>
        <translation>Kein Bootloader erkannt.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="230"/>
        <source>Error: could not retrieve device name</source>
        <translation type="unfinished">Fehler: konnte Gerätenamen nicht ermitteln</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="246"/>
        <source>Error: no mountpoint specified!</source>
        <translation>Fehler: kein Einhängepunkt angegeben!</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="251"/>
        <source>Could not open Ipod: permission denied</source>
        <translation>Konnte Ipod nicht öffnen: Zugriff verweigert</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="255"/>
        <source>Could not open Ipod</source>
        <translation>Konnte Ipod nicht öffnen</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="266"/>
        <source>No firmware partition on disk</source>
        <translation>Keine Firmware-Partition auf dem Laufwerk</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="176"/>
        <source>Successfully removed bootloader</source>
        <translation>Bootloader erfolgreich entfernt</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="183"/>
        <source>Removing bootloader failed.</source>
        <translation>Entfernen des Bootloaders fehlgeschlagen.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="91"/>
        <source>Installing Rockbox bootloader</source>
        <translation>Installiere Rockbox Bootloader</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="143"/>
        <source>Uninstalling bootloader</source>
        <translation>Entferne Bootloader</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="260"/>
        <source>Error reading partition table - possibly not an Ipod</source>
        <translation>Fehler beim Lesen der Partitionstabelle - möglicherweise kein Ipod</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallMi4</name>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="33"/>
        <source>Downloading bootloader</source>
        <translation>Lade Bootloader herunter</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="42"/>
        <source>Installing Rockbox bootloader</source>
        <translation>Installiere Rockbox Bootloader</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="65"/>
        <source>Bootloader successful installed</source>
        <translation>Bootloader erfolgreich installiert</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="77"/>
        <source>Checking for Rockbox bootloader</source>
        <translation>Prüfe auf Rockbox-Bootloader</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="79"/>
        <source>No Rockbox bootloader found</source>
        <translation>Kein Rockbox-Bootloader gefunden</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="84"/>
        <source>Checking for original firmware file</source>
        <translation>Prüfe auf Firmwaredatei der Originalfirmware</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="89"/>
        <source>Error finding original firmware file</source>
        <translation>Fehler beim finden der Originalfirmware-Datei</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="99"/>
        <source>Rockbox bootloader successful removed</source>
        <translation>Rockbox Bootloader erfolgreich entfernt</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallMpio</name>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="33"/>
        <source>Bootloader installation requires you to provide a firmware file of the original firmware (bin file). You need to download this file yourself due to legal reasons. Please refer to the &lt;a href=&apos;http://www.rockbox.org/manual.shtml&apos;&gt;manual&lt;/a&gt; and the &lt;a href=&apos;http://www.rockbox.org/wiki/MPIOHD200Port&apos;&gt;MPIOHD200Port&lt;/a&gt; wiki page on how to obtain this file.&lt;br/&gt;Press Ok to continue and browse your computer for the firmware file.</source>
        <translation type="unfinished">Die Bootloader-Installation benötigt eine Firmware-Datei der originalen Firmware (bin-Datei). Diese Datei muss aus rechtlichen Gründen separat heruntergeladen werden. Informationen wie diese Datei heruntergeladen werden kann ist im &lt;a href=&apos;http://www.rockbox.org/manual.shtml&apos;&gt;Handbuch&lt;/a&gt; und der Wiki-Seite &lt;a href=&apos;http://www.rockbox.org/wiki/MPIO200Port&apos;&gt;MPIO200Port&lt;/a&gt; aufgeführt.&lt;br/&gt;OK um fortzufahren und die Datei auf dem Computer auszuwählen.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="52"/>
        <source>Downloading bootloader file</source>
        <translation type="unfinished">Lade Bootloader-Datei herunter</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="79"/>
        <source>Could not open the original firmware.</source>
        <translation type="unfinished">Konnte Firmware-Datei nicht öffnen.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="82"/>
        <source>Could not read the original firmware.</source>
        <translation type="unfinished">Konnte Firmware-Datei nicht lesen.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="85"/>
        <source>Loaded firmware file does not look like MPIO original firmware file.</source>
        <translation type="unfinished">Geladene Firmware-Datei scheint keine MPIO Firmware-Datei zu sein.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="100"/>
        <source>Could not open output file.</source>
        <translation type="unfinished">Konnte Ausgabedatei nicht öffnen.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="103"/>
        <source>Could not write output file.</source>
        <translation type="unfinished">Kann Ausgabedatei nicht öffnen.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="106"/>
        <source>Unknown error number: %1</source>
        <translation type="unfinished">Unbekannter Fehler Nummer: %1</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="88"/>
        <source>Could not open downloaded bootloader.</source>
        <translation type="unfinished">Kann heruntergeladenen Bootloader nicht öffnen.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="91"/>
        <source>Place for bootloader in OF file not empty.</source>
        <translation type="unfinished">Zielbereich für Bootloader in Original-Firmware nicht leer.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="94"/>
        <source>Could not read the downloaded bootloader.</source>
        <translation type="unfinished">Konnte heruntergeladenen Bootloader nicht lesen.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="97"/>
        <source>Bootloader checksum error.</source>
        <translation type="unfinished">Prüfsummenfehler im Bootloader.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="110"/>
        <location filename="../base/bootloaderinstallmpio.cpp" line="111"/>
        <source>Patching original firmware failed: %1</source>
        <translation type="unfinished">Patchen der Original-Firmware fehlgeschlagen: %1</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="118"/>
        <source>Success: modified firmware file created</source>
        <translation type="unfinished">Erfolg: modifizierte Firmware-Datei erzeugt</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="126"/>
        <source>To uninstall, perform a normal upgrade with an unmodified original firmware</source>
        <translation type="unfinished">Zum deinstallieren ein Upgrade mit einer unveränderten Originalfirmware-Datei durchführen</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallSansa</name>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="55"/>
        <source>Error: can&apos;t allocate buffer memory!</source>
        <translation>Fehler: kann Speicher nicht allokieren!</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="60"/>
        <source>Searching for Sansa</source>
        <translation>Suche nach Sansa</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="66"/>
        <source>Permission for disc access denied!
This is required to install the bootloader</source>
        <translation>Direkter Laufwerkszugriff verweigert!
Der Zugriff ist notwendig um den Bootloader zu installieren</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="73"/>
        <source>No Sansa detected!</source>
        <translation>Kein Sansa gefunden!</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="86"/>
        <source>Downloading bootloader file</source>
        <translation>Lade Bootloader-Datei herunter</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="78"/>
        <location filename="../base/bootloaderinstallsansa.cpp" line="188"/>
        <source>OLD ROCKBOX INSTALLATION DETECTED, ABORTING.
You must reinstall the original Sansa firmware before running
sansapatcher for the first time.
See http://www.rockbox.org/wiki/SansaE200Install
</source>
        <translation>ALTE ROCKBOX-INSTALLATION ERKANNT, ABBRUCH.
Die Original-Sansa-Firmware muss neu installiert werden
bevor sansapatcher zum ersten Mal verwendet werden kann.
Siehe http://www.rockbox.org/wiki/SansaE200Install
</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="110"/>
        <location filename="../base/bootloaderinstallsansa.cpp" line="198"/>
        <source>Could not open Sansa in R/W mode</source>
        <translation>Konnte Sansa nicht im R/W-Modus öffnen</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="137"/>
        <source>Successfully installed bootloader</source>
        <translation>Bootloader erfolgreich installiert</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="148"/>
        <source>Failed to install bootloader</source>
        <translation>Bootloader-Installation fehlgeschlagen</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="161"/>
        <source>Bootloader Installation complete.</source>
        <translation>Bootloader-Installation vollständig.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="166"/>
        <source>Writing log aborted</source>
        <translation>Schreiben der Log-Datei abgebrochen</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="248"/>
        <source>Error: could not retrieve device name</source>
        <translation type="unfinished">Fehler: konnte Gerätenamen nicht ermitteln</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="264"/>
        <source>Can&apos;t find Sansa</source>
        <translation>Konnte Sansa nicht finden</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="269"/>
        <source>Could not open Sansa</source>
        <translation>Konnte Sansa nicht öffnen </translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="274"/>
        <source>Could not read partition table</source>
        <translation>Konnte Partitionstabelle nicht lesen</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="281"/>
        <source>Disk is not a Sansa (Error %1), aborting.</source>
        <translation>Laufwerk ist kein Sansa (Fehler: %1), breche ab.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="204"/>
        <source>Successfully removed bootloader</source>
        <translation>Bootloader erfolgreich entfernt</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="211"/>
        <source>Removing bootloader failed.</source>
        <translation>Entfernen des Bootloaders fehlgeschlagen.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="102"/>
        <source>Installing Rockbox bootloader</source>
        <translation>Installiere Rockbox Bootloader</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="179"/>
        <source>Uninstalling bootloader</source>
        <translation>Entferne Bootloader</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="119"/>
        <source>Checking downloaded bootloader</source>
        <translation>Prüfe heruntergeladenen Bootloader</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="127"/>
        <source>Bootloader mismatch! Aborting.</source>
        <translation type="unfinished">Fehler im Bootloader! Abbruch.</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallTcc</name>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="33"/>
        <source>Bootloader installation requires you to provide a firmware file of the original firmware (bin file). You need to download this file yourself due to legal reasons. Please refer to the &lt;a href=&apos;http://www.rockbox.org/manual.shtml&apos;&gt;manual&lt;/a&gt; and the &lt;a href=&apos;http://www.rockbox.org/wiki/CowonD2Info&apos;&gt;CowonD2Info&lt;/a&gt; wiki page on how to obtain the file.&lt;br/&gt;Press Ok to continue and browse your computer for the firmware file.</source>
        <translation type="unfinished">Die Bootloader-Installation benötigt eine Firmware-Datei der originalen Firmware (bin-Datei). Diese Datei muss aus rechtlichen Gründen separat heruntergeladen werden. Informationen wie diese Datei heruntergeladen werden kann sind im &lt;a href=&apos;http://www.rockbox.org/manual.shtml&apos;&gt;Handbuch&lt;/a&gt; und der Wiki-Seite &lt;a href=&apos;http://www.rockbox.org/wiki/CowonD2Info&apos;&gt;CowonD2Info&lt;/a&gt; aufgeführt.&lt;br/&gt;OK um fortzufahren und die Datei auf dem Computer auszuwählen.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="50"/>
        <source>Downloading bootloader file</source>
        <translation type="unfinished">Lade Bootloader-Datei herunter</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="82"/>
        <location filename="../base/bootloaderinstalltcc.cpp" line="99"/>
        <source>Could not load %1</source>
        <translation type="unfinished">Konnte %1 nicht laden</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="90"/>
        <source>Unknown OF file used: %1</source>
        <translation type="unfinished">Unbekannte Original-Firmware-Datei: %1</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="104"/>
        <source>Patching Firmware...</source>
        <translation type="unfinished">Patche Firmware ...</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="111"/>
        <source>Could not patch firmware</source>
        <translation type="unfinished">Konnte Firmware nicht patchen</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="117"/>
        <source>Could not open %1 for writing</source>
        <translation type="unfinished">Konnte %1 nicht zum schreiben öffnen</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="126"/>
        <source>Could not write firmware file</source>
        <translation type="unfinished">Konnte Firmware-Datei nicht schreiben</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="131"/>
        <source>Success: modified firmware file created</source>
        <translation type="unfinished">Erfolg: modifizierte Firmware-Datei erzeugt</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="151"/>
        <source>To uninstall, perform a normal upgrade with an unmodified original firmware</source>
        <translation type="unfinished">Zum deinstallieren ein Upgrade mit einer unveränderten Originalfirmware-Datei durchführen</translation>
    </message>
</context>
<context>
    <name>Config</name>
    <message>
        <location filename="../configure.cpp" line="771"/>
        <location filename="../configure.cpp" line="780"/>
        <source>Autodetection</source>
        <translation>Automatische Erkennung</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="772"/>
        <source>Could not detect a Mountpoint.
Select your Mountpoint manually.</source>
        <translation>Konnte Einhängepunkt nicht erkennen.
Bitte manuell auswählen.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="781"/>
        <source>Could not detect a device.
Select your device and Mountpoint manually.</source>
        <translation>Konnte kein Gerät erkennen.
Bitte Gerät und Einhängepunt manuell auswählen.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="792"/>
        <source>Really delete cache?</source>
        <translation>Cache wirklich löschen?</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="793"/>
        <source>Do you really want to delete the cache? Make absolutely sure this setting is correct as it will remove &lt;b&gt;all&lt;/b&gt; files in this folder!</source>
        <translation>Cache wirklich löschen? Unbedingt sicherstellen dass die Enstellungen korrekt sind, dies löscht &lt;b&gt;alle&lt;/b&gt; Dateien im Cache-Ordner!</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="801"/>
        <source>Path wrong!</source>
        <translation>Pfad fehlerhaft!</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="802"/>
        <source>The cache path is invalid. Aborting.</source>
        <translation>Cache-Pfad ist ungültig. Abbruch.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="301"/>
        <source>Current cache size is %L1 kiB.</source>
        <translation>Aktuelle Cachegröße ist %L1 kiB. </translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="750"/>
        <source>Fatal error</source>
        <translation>Fataler Fehler</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="418"/>
        <location filename="../configure.cpp" line="448"/>
        <source>Configuration OK</source>
        <translation>Konfiguration OK</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="424"/>
        <location filename="../configure.cpp" line="453"/>
        <source>Configuration INVALID</source>
        <translation>Konfiguration UNGÜLTIG</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="122"/>
        <source>The following errors occurred:</source>
        <translation>Die folgenden Fehler sind aufgetreten:</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="156"/>
        <source>No mountpoint given</source>
        <translation>Kein Einhängepunkt ausgewählt</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="160"/>
        <source>Mountpoint does not exist</source>
        <translation>Einhängepunkt existiert nicht</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="164"/>
        <source>Mountpoint is not a directory.</source>
        <translation type="unfinished">Einhängepunkt ist kein Verzeichnis.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="168"/>
        <source>Mountpoint is not writeable</source>
        <translation>Einhängepunkt ist nicht schreibbar</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="183"/>
        <source>No player selected</source>
        <translation>Kein Gerät ausgewählt</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="190"/>
        <source>Cache path not writeable. Leave path empty to default to systems temporary path.</source>
        <translation>Cache-Pfad ist nicht schreibbar. Um auf den temporären Pfad des Systems zurückzusetzen den Pfad leer lassen.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="210"/>
        <source>You need to fix the above errors before you can continue.</source>
        <translation>Die Fehler müssen beseitigt werden um fortzufahren.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="213"/>
        <source>Configuration error</source>
        <translation>Konfigurationsfehler</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="310"/>
        <source>Showing disabled targets</source>
        <translation type="unfinished">Zeige deaktivierte Geräte</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="311"/>
        <source>You just enabled showing targets that are marked disabled. Disabled targets are not recommended to end users. Please use this option only if you know what you are doing.</source>
        <translation type="unfinished">Deaktivierte Geräte werden jetzt angezeigt. Deaktivierte Geräte sind nicht für Anwender gedacht. Bitte diese Option nur benutzen wenn die Folgen klar sind.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="493"/>
        <source>Proxy Detection</source>
        <translation type="unfinished">Proxy-Erkennung</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="494"/>
        <source>The System Proxy settings are invalid!
Rockbox Utility can&apos;t work with this proxy settings. Make sure the system proxy is set correctly. Note that &quot;proxy auto-config (PAC)&quot; scripts are not supported by Rockbox Utility. If your system uses this you need to use manual proxy settings.</source>
        <translation type="unfinished">Die System-Proxy-Werte sind ungültig!
Rockbox Utility kann mit diesen Proxy-Einstellungen nicht arbeiten. Bitte sicherstellen dass die Proxy-Einstellungen im System korrekt sind. Hinweis: &quot;Proxy Auto-Konfiguration (PAC)&quot;-Skripte werden von Rockbox Utility nicht unterstützt. Sofern das System dies benutzt muss der Proxy manuell angegeben werden.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="607"/>
        <source>Set Cache Path</source>
        <translation type="unfinished">Cache-Pfad einstellen</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="736"/>
        <source>%1 &quot;MacPod&quot; found!
Rockbox needs a FAT formatted Ipod (so-called &quot;WinPod&quot;) to run. </source>
        <translation type="unfinished">%1 &quot;MacPod&quot; gefunden!
Rockbox benötigt einen mit dem Dateisystem FAT formatierten Ipod (sogenannter &quot;WinPod&quot;).</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="743"/>
        <source>%1 in MTP mode found!
You need to change your player to MSC mode for installation. </source>
        <translation type="unfinished">%1 im MTP-Modus gefunden!
Das Gerät muss für die Installation im MSC-Modus sein.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="748"/>
        <source>Until you change this installation will fail!</source>
        <translation type="unfinished">Solange dies nicht geändert ist wird die Installation fehlschlagen!</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="755"/>
        <source>Detected an unsupported player:
%1
Sorry, Rockbox doesn&apos;t run on your player.</source>
        <translation>Nicht unterstütztes Gerät gefunden:
%1
Rockbox funktioniert auf diesem Gerät leider nicht.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="760"/>
        <source>Fatal: player incompatible</source>
        <translation>Fatal: Gerät nicht kompatibel</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="840"/>
        <source>TTS configuration invalid</source>
        <translation>TTS-Konfiguration ungültig</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="841"/>
        <source>TTS configuration invalid. 
 Please configure TTS engine.</source>
        <translation>TTS-Konfiguration ungültig. Bitte TTS-System konfigurieren.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="846"/>
        <source>Could not start TTS engine.</source>
        <translation>Konnte TTS-System nicht starten.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="847"/>
        <source>Could not start TTS engine.
</source>
        <translation>Konnte TTS-System nicht starten.
</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="848"/>
        <location filename="../configure.cpp" line="867"/>
        <source>
Please configure TTS engine.</source>
        <translation>Bitte TTS-System konfigurieren.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="862"/>
        <source>Rockbox Utility Voice Test</source>
        <translation>Rockbox Utility Sprachtest</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="865"/>
        <source>Could not voice test string.</source>
        <translation>Konnte Teststring nicht sprechen.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="866"/>
        <source>Could not voice test string.
</source>
        <translation>Konnte Teststring nicht sprechen.
</translation>
    </message>
</context>
<context>
    <name>ConfigForm</name>
    <message>
        <location filename="../configurefrm.ui" line="14"/>
        <source>Configuration</source>
        <translation>Konfiguration</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="20"/>
        <source>Configure Rockbox Utility</source>
        <translation>Rockbox Utility konfigurieren</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="539"/>
        <source>&amp;Ok</source>
        <translation>&amp;Ok</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="550"/>
        <source>&amp;Cancel</source>
        <translation>&amp;Abbrechen</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="136"/>
        <source>&amp;Proxy</source>
        <translation>&amp;Proxy</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="95"/>
        <source>Show disabled targets</source>
        <translation type="unfinished">Deaktivierte Geräte anzeigen</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="142"/>
        <source>&amp;No Proxy</source>
        <translation>&amp;kein Proxy</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="159"/>
        <source>&amp;Manual Proxy settings</source>
        <translation>&amp;Manuelle Proxyeinstellungen</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="166"/>
        <source>Proxy Values</source>
        <translation>Proxy-Einstellungen</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="172"/>
        <source>&amp;Host:</source>
        <translation>&amp;Host:</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="189"/>
        <source>&amp;Port:</source>
        <translation>&amp;Port:</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="212"/>
        <source>&amp;Username</source>
        <translation>&amp;Benutzername</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="253"/>
        <source>&amp;Language</source>
        <translation>&amp;Sprache</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="35"/>
        <source>&amp;Device</source>
        <translation>&amp;Gerät</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="41"/>
        <source>Select your device in the &amp;filesystem</source>
        <translation>Gerät im &amp;Dateisystem auswählen</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="312"/>
        <source>&amp;Browse</source>
        <translation>D&amp;urchsuchen</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="72"/>
        <source>&amp;Select your audio player</source>
        <translation>Gerät au&amp;swählen</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="58"/>
        <source>&amp;Refresh</source>
        <translation type="unfinished">&amp;Aktualisieren</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="114"/>
        <source>&amp;Autodetect</source>
        <translation>&amp;automatische Erkennung</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="152"/>
        <source>Use S&amp;ystem values</source>
        <translation>S&amp;ystemwerte verwenden</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="222"/>
        <source>Pass&amp;word</source>
        <translation>Pass&amp;wort</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="267"/>
        <source>Cac&amp;he</source>
        <translation>Cac&amp;he</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="270"/>
        <source>Download cache settings</source>
        <translation>Downloadcache-Einstellungen</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="276"/>
        <source>Rockbox Utility uses a local download cache to save network traffic. You can change the path to the cache and use it as local repository by enabling Offline mode.</source>
        <translation>Rockbox Utility verwendet einen lokalen Download-Cache um die übertragene Datenmenge zu begrenzen. Der Pfad zum Cache kann geändert sowie im Offline-Modus als lokales Repository verwenden werden.</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="286"/>
        <source>Current cache size is %1</source>
        <translation>Aktuelle Cache-Größe ist %1</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="295"/>
        <source>P&amp;ath</source>
        <translation>P&amp;fad</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="327"/>
        <source>Disable local &amp;download cache</source>
        <translation>lokalen &amp;Download-Cache ausschalten</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="337"/>
        <source>O&amp;ffline mode</source>
        <translation>O&amp;ffline-Modus</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="372"/>
        <source>Clean cache &amp;now</source>
        <translation>C&amp;ache löschen</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="305"/>
        <source>Entering an invalid folder will reset the path to the systems temporary path.</source>
        <translation>Ein ungültiger Ordner setzt den Pfad auf den temporären Pfad des Systems zurück.</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="334"/>
        <source>&lt;p&gt;This will try to use all information from the cache, even information about updates. Only use this option if you want to install without network connection. Note: you need to do the same install you want to perform later with network access first to download all required files to the cache.&lt;/p&gt;</source>
        <translation>&lt;p&gt;Dies versucht alle Informationen aus dem Cache zu beziehen, selbst die Informationen über Updates. Diese Option nur benutzen um ohne Internetverbindung zu installieren. Hinweis: die gleiche Installation, die später durchgeführt werden soll, muss einmal mit Netzwerkverbindung durchführt werden, damit die notwendigen Dateien im Cache gespeichert sind.&lt;/p&gt;</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="388"/>
        <source>&amp;TTS &amp;&amp; Encoder</source>
        <translation>&amp;TTS &amp;&amp; Encoder</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="394"/>
        <source>TTS Engine</source>
        <translation>TTS-System</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="465"/>
        <source>Encoder Engine</source>
        <translation>Encoder-System</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="400"/>
        <source>&amp;Select TTS Engine</source>
        <translation>TT&amp;S-System auswählen</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="413"/>
        <source>Configure TTS Engine</source>
        <translation>TTS-System konfigurieren</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="420"/>
        <location filename="../configurefrm.ui" line="471"/>
        <source>Configuration invalid!</source>
        <translation>Konfiguration ungültig!</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="437"/>
        <source>Configure &amp;TTS</source>
        <translation>&amp;TTS konfigurieren</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="455"/>
        <source>&amp;Use string corrections for TTS</source>
        <translation type="unfinished">Verwende &amp;Lautkorrektur für TTS</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="488"/>
        <source>Configure &amp;Enc</source>
        <translation> &amp;Encoder konfigurieren</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="499"/>
        <source>encoder name</source>
        <translation>Encoder-Name</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="448"/>
        <source>Test TTS</source>
        <translation>TTS testen</translation>
    </message>
</context>
<context>
    <name>Configure</name>
    <message>
        <location filename="../configure.cpp" line="553"/>
        <source>English</source>
        <comment>This is the localized language name, i.e. your language.</comment>
        <translation>Deutsch</translation>
    </message>
</context>
<context>
    <name>CreateVoiceFrm</name>
    <message>
        <location filename="../createvoicefrm.ui" line="16"/>
        <source>Create Voice File</source>
        <translation>Sprachdatei erstellen</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="41"/>
        <source>Select the Language you want to generate a voicefile for:</source>
        <translation>Sprache auswählen, für die die Sprachdatei generiert werden soll:</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="55"/>
        <source>Generation settings</source>
        <translation>Allgemeine Einstellungen</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="61"/>
        <source>Encoder profile:</source>
        <translation>Encoder-Profil:</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="68"/>
        <source>TTS profile:</source>
        <translation>TTS-Profil:</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="81"/>
        <source>Change</source>
        <translation>Ändern</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="132"/>
        <source>&amp;Install</source>
        <translation>&amp;Installieren</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="142"/>
        <source>&amp;Cancel</source>
        <translation>&amp;Abbrechen</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="156"/>
        <location filename="../createvoicefrm.ui" line="163"/>
        <source>Wavtrim Threshold</source>
        <translation>Wavtrim Schwellenwert</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="48"/>
        <source>Language</source>
        <translation>Sprache</translation>
    </message>
</context>
<context>
    <name>CreateVoiceWindow</name>
    <message>
        <location filename="../createvoicewindow.cpp" line="97"/>
        <location filename="../createvoicewindow.cpp" line="100"/>
        <source>Selected TTS engine: &lt;b&gt;%1&lt;/b&gt;</source>
        <translation>Gewähltes TTS-System: &lt;b&gt;%1&lt;/b&gt;</translation>
    </message>
    <message>
        <location filename="../createvoicewindow.cpp" line="108"/>
        <location filename="../createvoicewindow.cpp" line="111"/>
        <location filename="../createvoicewindow.cpp" line="115"/>
        <source>Selected encoder: &lt;b&gt;%1&lt;/b&gt;</source>
        <translation>Gewählter Encoder: &lt;b&gt;%1&lt;/b&gt;</translation>
    </message>
</context>
<context>
    <name>EncTtsCfgGui</name>
    <message>
        <location filename="../encttscfggui.cpp" line="31"/>
        <source>Waiting for engine...</source>
        <translation type="unfinished">Warte auf Engine ...</translation>
    </message>
    <message>
        <location filename="../encttscfggui.cpp" line="81"/>
        <source>Ok</source>
        <translation>Ok</translation>
    </message>
    <message>
        <location filename="../encttscfggui.cpp" line="84"/>
        <source>Cancel</source>
        <translation>Abbrechen</translation>
    </message>
    <message>
        <location filename="../encttscfggui.cpp" line="183"/>
        <source>Browse</source>
        <translation>Durchsuchen</translation>
    </message>
    <message>
        <location filename="../encttscfggui.cpp" line="191"/>
        <source>Refresh</source>
        <translation>Aktualisieren</translation>
    </message>
    <message>
        <location filename="../encttscfggui.cpp" line="363"/>
        <source>Select executable</source>
        <translation type="unfinished">Ausführbare Datei auswählen</translation>
    </message>
</context>
<context>
    <name>EncoderExe</name>
    <message>
        <location filename="../base/encoderexe.cpp" line="40"/>
        <source>Path to Encoder:</source>
        <translation type="unfinished">Pfad zum Encoder:</translation>
    </message>
    <message>
        <location filename="../base/encoderexe.cpp" line="42"/>
        <source>Encoder options:</source>
        <translation type="unfinished">Encoder Optionen:</translation>
    </message>
</context>
<context>
    <name>EncoderLame</name>
    <message>
        <location filename="../base/encoderlame.cpp" line="69"/>
        <location filename="../base/encoderlame.cpp" line="79"/>
        <source>LAME</source>
        <translation type="unfinished">LAME</translation>
    </message>
    <message>
        <location filename="../base/encoderlame.cpp" line="71"/>
        <source>Volume</source>
        <translation type="unfinished">Lautstärke</translation>
    </message>
    <message>
        <location filename="../base/encoderlame.cpp" line="75"/>
        <source>Quality</source>
        <translation type="unfinished">Qualität</translation>
    </message>
    <message>
        <location filename="../base/encoderlame.cpp" line="79"/>
        <source>Could not find libmp3lame!</source>
        <translation type="unfinished">Konnte libmp3lame nicht finden!</translation>
    </message>
</context>
<context>
    <name>EncoderRbSpeex</name>
    <message>
        <location filename="../base/encoderrbspeex.cpp" line="33"/>
        <source>Volume:</source>
        <translation type="unfinished">Lautstärke:</translation>
    </message>
    <message>
        <location filename="../base/encoderrbspeex.cpp" line="35"/>
        <source>Quality:</source>
        <translation type="unfinished">Qualität:</translation>
    </message>
    <message>
        <location filename="../base/encoderrbspeex.cpp" line="37"/>
        <source>Complexity:</source>
        <translation type="unfinished">Komplexität:</translation>
    </message>
    <message>
        <location filename="../base/encoderrbspeex.cpp" line="39"/>
        <source>Use Narrowband:</source>
        <translation type="unfinished">Benutze Schmalband:</translation>
    </message>
</context>
<context>
    <name>InfoWidget</name>
    <message>
        <location filename="../gui/infowidget.cpp" line="29"/>
        <source>File</source>
        <translation type="unfinished">Datei</translation>
    </message>
    <message>
        <location filename="../gui/infowidget.cpp" line="29"/>
        <source>Version</source>
        <translation type="unfinished">Version</translation>
    </message>
</context>
<context>
    <name>InfoWidgetFrm</name>
    <message>
        <location filename="../gui/infowidgetfrm.ui" line="14"/>
        <source>Form</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../gui/infowidgetfrm.ui" line="20"/>
        <source>Currently installed packages.&lt;br/&gt;&lt;b&gt;Note:&lt;/b&gt; if you manually installed packages this might not be correct!</source>
        <translation type="unfinished">Aktuell installierte Pakete.&lt;br/&gt;&lt;b&gt;Hinweis:&lt;/b&gt; wenn Pakete manuell installiert wurden wird diese Anzeige nicht korrekt sein!</translation>
    </message>
    <message>
        <location filename="../gui/infowidgetfrm.ui" line="34"/>
        <source>1</source>
        <translation type="unfinished">1</translation>
    </message>
</context>
<context>
    <name>InstallTalkFrm</name>
    <message>
        <location filename="../installtalkfrm.ui" line="17"/>
        <source>Install Talk Files</source>
        <translation>Talk-Dateien installieren</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="36"/>
        <source>Select the Folder to generate Talkfiles for.</source>
        <translation>Ordner, für den Talk-Dateien erstellt werden sollen, auswählen.</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="50"/>
        <source>&amp;Browse</source>
        <translation>&amp;Durchsuchen</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="201"/>
        <source>Run recursive</source>
        <translation>Rekursiv durchlaufen</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="211"/>
        <source>Strip Extensions</source>
        <translation>Dateiendungen entfernen</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="149"/>
        <source>&amp;Cancel</source>
        <translation>&amp;Abbrechen</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="61"/>
        <source>Generation settings</source>
        <translation>Allgemeine Einstellungen</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="67"/>
        <source>Encoder profile:</source>
        <translation>Encoder-Profil:</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="74"/>
        <source>TTS profile:</source>
        <translation>TTS-Profil:</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="162"/>
        <source>Generation options</source>
        <translation>Generierungsoptionen</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="221"/>
        <source>Create only new Talkfiles</source>
        <translation>Nur neue Sprachdateien erzeugen</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="87"/>
        <source>Change</source>
        <translation>Ändern</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="191"/>
        <source>Generate .talk files for Folders</source>
        <translation>.talk Dateien für Ordner erzeugen</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="178"/>
        <source>Generate .talk files for Files</source>
        <translation>.talk Dateien für Dateien erzeugen</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="138"/>
        <source>&amp;Install</source>
        <translation>&amp;Installieren</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="43"/>
        <source>Talkfile Folder</source>
        <translation>Ordner für Sprachdateien</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="171"/>
        <source>Ignore files (comma seperated Wildcards):</source>
        <translation type="unfinished">Ignoriere Dateien (Kommagetrennte Maskenzeichen):</translation>
    </message>
</context>
<context>
    <name>InstallTalkWindow</name>
    <message>
        <location filename="../installtalkwindow.cpp" line="54"/>
        <source>Select folder to create talk files</source>
        <translation type="unfinished">Ordner für Talk-Dateien auswählen</translation>
    </message>
    <message>
        <location filename="../installtalkwindow.cpp" line="89"/>
        <source>The Folder to Talk is wrong!</source>
        <translation>Der Ordner für den Talk-Dateien erzeugt werden sollen ist falsch!</translation>
    </message>
    <message>
        <location filename="../installtalkwindow.cpp" line="122"/>
        <location filename="../installtalkwindow.cpp" line="125"/>
        <source>Selected TTS engine: &lt;b&gt;%1&lt;/b&gt;</source>
        <translation>Gewähltes TTS-System: &lt;b&gt;%1&lt;/b&gt;</translation>
    </message>
    <message>
        <location filename="../installtalkwindow.cpp" line="132"/>
        <location filename="../installtalkwindow.cpp" line="135"/>
        <location filename="../installtalkwindow.cpp" line="139"/>
        <source>Selected encoder: &lt;b&gt;%1&lt;/b&gt;</source>
        <translation>Gewählter Encoder: &lt;b&gt;%1&lt;/b&gt;</translation>
    </message>
</context>
<context>
    <name>InstallWindow</name>
    <message>
        <location filename="../installwindow.cpp" line="107"/>
        <source>Backup to %1</source>
        <translation>Sicherungskopie nach %1</translation>
    </message>
    <message>
        <location filename="../installwindow.cpp" line="137"/>
        <source>Mount point is wrong!</source>
        <translation>Falscher Einhängepunkt!</translation>
    </message>
    <message>
        <location filename="../installwindow.cpp" line="174"/>
        <source>Really continue?</source>
        <translation>Wirklich fortfahren?</translation>
    </message>
    <message>
        <location filename="../installwindow.cpp" line="178"/>
        <source>Aborted!</source>
        <translation>Abgebrochen!</translation>
    </message>
    <message>
        <location filename="../installwindow.cpp" line="187"/>
        <source>Beginning Backup...</source>
        <translation>Erstelle Sicherungskopie ...</translation>
    </message>
    <message>
        <location filename="../installwindow.cpp" line="209"/>
        <source>Backup finished.</source>
        <translation type="unfinished">Backup abgeschlossen.</translation>
    </message>
    <message>
        <location filename="../installwindow.cpp" line="212"/>
        <source>Backup failed!</source>
        <translation>Sicherung fehlgeschlagen!</translation>
    </message>
    <message>
        <location filename="../installwindow.cpp" line="243"/>
        <source>Select Backup Filename</source>
        <translation>Dateiname für Sicherungskopie auswählen</translation>
    </message>
    <message>
        <location filename="../installwindow.cpp" line="276"/>
        <source>This is the absolute up to the minute Rockbox built. A current build will get updated every time a change is made. Latest version is %1 (%2).</source>
        <translation>Dies ist das aktuellste Rockbox build. Es wird bei jeder Änderung aktualisiert. Letzte Version ist %1 (%2).</translation>
    </message>
    <message>
        <location filename="../installwindow.cpp" line="282"/>
        <source>&lt;b&gt;This is the recommended version.&lt;/b&gt;</source>
        <translation>&lt;b&gt;Dies ist die empfohlene Version.&lt;/b&gt;</translation>
    </message>
    <message>
        <location filename="../installwindow.cpp" line="293"/>
        <source>This is the last released version of Rockbox.</source>
        <translation>Dies ist die letzte veröffentlichte Version von Rockbox.</translation>
    </message>
    <message>
        <location filename="../installwindow.cpp" line="296"/>
        <source>&lt;b&gt;Note:&lt;/b&gt; The lastest released version is %1. &lt;b&gt;This is the recommended version.&lt;/b&gt;</source>
        <translation>&lt;b&gt;Hinweis:&lt;/b&gt; Die letzte Release-Version ist %1. &lt;b&gt;Dies ist die empfohlene Version.&lt;/b&gt;</translation>
    </message>
    <message>
        <location filename="../installwindow.cpp" line="308"/>
        <source>These are automatically built each day from the current development source code. This generally has more features than the last stable release but may be much less stable. Features may change regularly.</source>
        <translation>Diese Builds werden jeden Tag automatisch aus dem aktuellen Source Code gebaut. Sie haben meist mehr Features als das letzte stabile Release, können aber weniger stabil sein. Features können sich regelmäßig ändern.</translation>
    </message>
    <message>
        <location filename="../installwindow.cpp" line="312"/>
        <source>&lt;b&gt;Note:&lt;/b&gt; archived version is %1 (%2).</source>
        <translation>&lt;b&gt;Hinweis:&lt;/b&gt; Archivierte Version ist %1 (%2).</translation>
    </message>
</context>
<context>
    <name>InstallWindowFrm</name>
    <message>
        <location filename="../installwindowfrm.ui" line="16"/>
        <source>Install Rockbox</source>
        <translation>Rockbox installieren</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="35"/>
        <source>Please select the Rockbox version you want to install on your player:</source>
        <translation>Bitte die zu installierende Version von Rockbox auswählen:</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="45"/>
        <source>Version</source>
        <translation>Version</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="51"/>
        <source>Rockbox &amp;stable</source>
        <translation>&amp;Stabile Rockbox-Version</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="58"/>
        <source>&amp;Archived Build</source>
        <translation>&amp;Archivierte Version</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="65"/>
        <source>&amp;Current Build</source>
        <translation>Aktuelle &amp;Version</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="75"/>
        <source>Details</source>
        <translation>Details</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="81"/>
        <source>Details about the selected version</source>
        <translation>Details über die ausgewählte Version</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="91"/>
        <source>Note</source>
        <translation>Hinweis</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="119"/>
        <source>&amp;Install</source>
        <translation>&amp;Installieren</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="130"/>
        <source>&amp;Cancel</source>
        <translation>&amp;Abbrechen</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="156"/>
        <source>Backup</source>
        <translation>Sicherungskopie</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="162"/>
        <source>Backup before installing</source>
        <translation>Erstelle Sicherungskopie vor der Installation</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="169"/>
        <source>Backup location</source>
        <translation>Speicherort für Sicherungskopie</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="188"/>
        <source>Change</source>
        <translation>Ändern</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="198"/>
        <source>Rockbox Utility stores copies of Rockbox it has downloaded on the local hard disk to save network traffic. If your local copy is no longer working, tick this box to download a fresh copy.</source>
        <translation>Rockbox Utility speichert bereits heruntergeladenen Kopien von Rockbox auf der lokalen Festplatte um den Netzwerkverkehr zu begrenzen. Wenn die lokale Kopie nicht weiter funktioniert, diese Option verwenden um eine neue Kopie herunterzuladen.</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="201"/>
        <source>&amp;Don&apos;t use locally cached copy</source>
        <translation>&amp;keine lokale Zwischenkopie verwenden</translation>
    </message>
</context>
<context>
    <name>ManualWidget</name>
    <message>
        <location filename="../gui/manualwidget.cpp" line="78"/>
        <source>&lt;a href=&apos;%1&apos;&gt;PDF Manual&lt;/a&gt;</source>
        <translation type="unfinished">&lt;a href=&apos;%1&apos;&gt;PDF-Handbuch&lt;/a&gt;</translation>
    </message>
    <message>
        <location filename="../gui/manualwidget.cpp" line="80"/>
        <source>&lt;a href=&apos;%1&apos;&gt;HTML Manual (opens in browser)&lt;/a&gt;</source>
        <translation type="unfinished">&lt;a href=&apos;%1&apos;&gt;HTML-Handbuch (öffnet im Browser)&lt;/a&gt;</translation>
    </message>
    <message>
        <location filename="../gui/manualwidget.cpp" line="84"/>
        <source>Select a device for a link to the correct manual</source>
        <translation type="unfinished">Ein Gerät muss ausgewählt sein, damit ein Link zum entsprechenden Handbuch angezeigt wird</translation>
    </message>
    <message>
        <location filename="../gui/manualwidget.cpp" line="85"/>
        <source>&lt;a href=&apos;%1&apos;&gt;Manual Overview&lt;/a&gt;</source>
        <translation type="unfinished">&lt;a href=&apos;%1&apos;&gt;Anleitungen-Übersicht&lt;/a&gt;</translation>
    </message>
    <message>
        <location filename="../gui/manualwidget.cpp" line="96"/>
        <source>Confirm download</source>
        <translation type="unfinished">Download bestätigen</translation>
    </message>
    <message>
        <location filename="../gui/manualwidget.cpp" line="97"/>
        <source>Do you really want to download the manual? The manual will be saved to the root folder of your player.</source>
        <translation type="unfinished">Handbuch wirklich herunterladen? Das Handbuch wird im Wurzelordner des Geräts gespeichert.</translation>
    </message>
</context>
<context>
    <name>ManualWidgetFrm</name>
    <message>
        <location filename="../gui/manualwidgetfrm.ui" line="14"/>
        <source>Form</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../gui/manualwidgetfrm.ui" line="20"/>
        <source>Read the manual</source>
        <translation type="unfinished">Anleitung lesen</translation>
    </message>
    <message>
        <location filename="../gui/manualwidgetfrm.ui" line="26"/>
        <source>PDF manual</source>
        <translation type="unfinished">PDF-Anleitung</translation>
    </message>
    <message>
        <location filename="../gui/manualwidgetfrm.ui" line="39"/>
        <source>HTML manual</source>
        <translation type="unfinished">HTML-Anleitung</translation>
    </message>
    <message>
        <location filename="../gui/manualwidgetfrm.ui" line="55"/>
        <source>Download the manual</source>
        <translation type="unfinished">Anleitung herunterladen</translation>
    </message>
    <message>
        <location filename="../gui/manualwidgetfrm.ui" line="63"/>
        <source>&amp;PDF version</source>
        <translation type="unfinished">&amp;PDF-Version</translation>
    </message>
    <message>
        <location filename="../gui/manualwidgetfrm.ui" line="70"/>
        <source>&amp;HTML version (zip file)</source>
        <translation type="unfinished">&amp;HTML-Version (Zip-Datei)</translation>
    </message>
    <message>
        <location filename="../gui/manualwidgetfrm.ui" line="92"/>
        <source>Down&amp;load</source>
        <translation type="unfinished">Herunter&amp;laden</translation>
    </message>
</context>
<context>
    <name>PreviewFrm</name>
    <message>
        <location filename="../previewfrm.ui" line="16"/>
        <source>Preview</source>
        <translation>Vorschau</translation>
    </message>
</context>
<context>
    <name>ProgressLoggerFrm</name>
    <message>
        <location filename="../progressloggerfrm.ui" line="13"/>
        <location filename="../progressloggerfrm.ui" line="19"/>
        <source>Progress</source>
        <translation>Fortschritt</translation>
    </message>
    <message>
        <location filename="../progressloggerfrm.ui" line="82"/>
        <source>&amp;Abort</source>
        <translation>&amp;Abbrechen</translation>
    </message>
    <message>
        <location filename="../progressloggerfrm.ui" line="32"/>
        <source>progresswindow</source>
        <translation>Fortschrittsfenster</translation>
    </message>
    <message>
        <location filename="../progressloggerfrm.ui" line="58"/>
        <source>Save Log</source>
        <translation>Log speichern</translation>
    </message>
</context>
<context>
    <name>ProgressLoggerGui</name>
    <message>
        <location filename="../progressloggergui.cpp" line="121"/>
        <source>&amp;Ok</source>
        <translation>&amp;Ok</translation>
    </message>
    <message>
        <location filename="../progressloggergui.cpp" line="103"/>
        <source>&amp;Abort</source>
        <translation>&amp;Abbrechen</translation>
    </message>
    <message>
        <location filename="../progressloggergui.cpp" line="144"/>
        <source>Save system trace log</source>
        <translation>System-Trace Log speichern</translation>
    </message>
</context>
<context>
    <name>QObject</name>
    <message>
        <location filename="../configure.cpp" line="589"/>
        <location filename="../main.cpp" line="70"/>
        <source>LTR</source>
        <extracomment>This string is used to indicate the writing direction. Translate it to &quot;RTL&quot; (without quotes) for RTL languages. Anything else will get treated as LTR language.
----------
This string is used to indicate the writing direction. Translate it to &quot;RTL&quot; (without quotes) for RTL languages. Anything else will get treated as LTR language.</extracomment>
        <translation>LTR</translation>
    </message>
    <message>
        <location filename="../base/system.cpp" line="384"/>
        <source>(unknown vendor name) </source>
        <translation type="unfinished">(unbekannter Hersteller)</translation>
    </message>
    <message>
        <location filename="../base/system.cpp" line="402"/>
        <source>(unknown product name)</source>
        <translation type="unfinished">(Unbekannter Produktname)</translation>
    </message>
</context>
<context>
    <name>QuaZipFile</name>
    <message>
        <location filename="../quazip/quazipfile.cpp" line="141"/>
        <source>ZIP/UNZIP API error %1</source>
        <translation type="unfinished">ZIP / Unzip API Fehler %1</translation>
    </message>
</context>
<context>
    <name>RbUtilQt</name>
    <message>
        <location filename="../rbutilqt.cpp" line="411"/>
        <source>&lt;b&gt;%1 %2&lt;/b&gt; at &lt;b&gt;%3&lt;/b&gt;</source>
        <translation>&lt;b&gt;%1 %2&lt;/b&gt; an &lt;b&gt;%3&lt;/b&gt;</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="426"/>
        <location filename="../rbutilqt.cpp" line="482"/>
        <location filename="../rbutilqt.cpp" line="659"/>
        <location filename="../rbutilqt.cpp" line="830"/>
        <location filename="../rbutilqt.cpp" line="917"/>
        <location filename="../rbutilqt.cpp" line="961"/>
        <source>Confirm Installation</source>
        <translation>Installation bestätigen</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="600"/>
        <source>Beginning Backup...</source>
        <translation type="unfinished">Erstelle Sicherungskopie ...</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="660"/>
        <source>Do you really want to install the Bootloader?</source>
        <translation>Bootloader wirklich installieren?</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="824"/>
        <location filename="../rbutilqt.cpp" line="902"/>
        <source>No Rockbox installation found</source>
        <translation>Keine Rockbox-Installation gefunden</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="825"/>
        <source>Could not determine the installed Rockbox version. Please install a Rockbox build before installing fonts.</source>
        <translation type="unfinished">Konnte die installierte Rockbox-Version nicht herausfinden. Bitte vor der Installation der Schriften Rockbox installieren.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="831"/>
        <source>Do you really want to install the fonts package?</source>
        <translation>Schriftarten-Paket wirklich installieren?</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="903"/>
        <source>Could not determine the installed Rockbox version. Please install a Rockbox build before installing voice files.</source>
        <translation type="unfinished">Konnte die installierte Rockbox-Version nicht herausfinden. Bitte vor der Installation der Sprachdatei Rockbox installieren.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="918"/>
        <source>Do you really want to install the voice file?</source>
        <translation>Sprachdateien wirklich installieren?</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="962"/>
        <source>Do you really want to install the game addon files?</source>
        <translation>Zusatzdateien für Spiele wirklich installieren?</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1040"/>
        <source>Confirm Uninstallation</source>
        <translation>Entfernen bestätigen</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1041"/>
        <source>Do you really want to uninstall the Bootloader?</source>
        <translation>Bootloader wirklich entfernen?</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1055"/>
        <source>No uninstall method for this target known.</source>
        <translation type="unfinished">Keine Deinstallationsmethode für dieses Gerät verfügbar.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1072"/>
        <source>Rockbox Utility can not uninstall the bootloader on this target. Try a normal firmware update to remove the booloader.</source>
        <translation type="unfinished">Rockbox Utility kann den Bootloader auf diesem Gerät nicht entfernen. Bitte ein reguläres Firmware-Update versuchen um den Bootloader zu installieren.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1091"/>
        <source>Confirm installation</source>
        <translation>Installation bestätigen</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1092"/>
        <source>Do you really want to install Rockbox Utility to your player? After installation you can run it from the players hard drive.</source>
        <translation>Rockbox Utility wirklich auf dem Gerät installieren? Nach der Installation kann es von dem Laufwerk des Geräts ausgeführt werden.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1101"/>
        <source>Installing Rockbox Utility</source>
        <translation>Installiere Rockbox Utility</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1252"/>
        <source>New version of Rockbox Utility available.</source>
        <translation>Neue Version von Rockbox Utility verfügbar.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1255"/>
        <source>Rockbox Utility is up to date.</source>
        <translation>Rockbox Utility ist aktuell.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="505"/>
        <location filename="../rbutilqt.cpp" line="1105"/>
        <source>Mount point is wrong!</source>
        <translation>Falscher Einhängepunkt!</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1119"/>
        <source>Error installing Rockbox Utility</source>
        <translation>Fehler beim installieren von Rockbox Utility</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1123"/>
        <source>Installing user configuration</source>
        <translation>Installiere Benutzerkonfiguration</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1127"/>
        <source>Error installing user configuration</source>
        <translation>Fehler beim installieren der Benutzerkonfiguration</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1131"/>
        <source>Successfully installed Rockbox Utility.</source>
        <translation>Rockbox Utility erfolgreich installiert.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="365"/>
        <location filename="../rbutilqt.cpp" line="1159"/>
        <source>Configuration error</source>
        <translation>Konfigurationsfehler</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="956"/>
        <source>Error</source>
        <translation>Fehler</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="957"/>
        <source>Your device doesn&apos;t have a doom plugin. Aborting.</source>
        <translation>Für das gewählte Gerät existiert kein Doom-Plugin. Abbruch.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1160"/>
        <source>Your configuration is invalid. Please go to the configuration dialog and make sure the selected values are correct.</source>
        <translation>Die Konfiguration ist ungültig. Bitte im Konfigurationsdialog sicherstellen dass die Einstellungen korrekt sind.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="358"/>
        <source>This is a new installation of Rockbox Utility, or a new version. The configuration dialog will now open to allow you to setup the program,  or review your settings.</source>
        <translation>Dies ist eine neue Installation oder eine neue Version von Rockbox Utility. Der Konfigurationsdialog wird nun automatisch geöffnet, um das Programm zu konfigurieren oder die Einstellungen zu prüfen.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="103"/>
        <source>Wine detected!</source>
        <translation>Wine entdeckt!</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="104"/>
        <source>It seems you are trying to run this program under Wine. Please don&apos;t do this, running under Wine will fail. Use the native Linux binary instead.</source>
        <translation type="unfinished">Es scheint so als ob dieses Programm mit Wine ausgeführt wird. Bitte dies nicht tun, es wird fehlschlagen. Stattdessen die native Linux-Version verwenden.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="240"/>
        <location filename="../rbutilqt.cpp" line="271"/>
        <source>Can&apos;t get version information.
Network error: %1. Please check your network and proxy settings.</source>
        <translation type="unfinished">Kann Versions-Informationen nicht laden.
Netzwerkfehler: %1. Bitte Netzwerk und Proxy-Einstellungen überprüfen.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="573"/>
        <source>Aborted!</source>
        <translation>Abgebrochen!</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="583"/>
        <source>Installed Rockbox detected</source>
        <translation>Installiertes Rockbox erkannt</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="584"/>
        <source>Rockbox installation detected. Do you want to backup first?</source>
        <translation>Installiertes Rockbox erkannt. Soll zunächst eine Sicherungskopie gemacht werden?</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="618"/>
        <source>Backup failed!</source>
        <translation>Sicherung fehlgeschlagen!</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="888"/>
        <source>Warning</source>
        <translation>Warnung</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="889"/>
        <source>The Application is still downloading Information about new Builds. Please try again shortly.</source>
        <translation type="unfinished">Das Progamm lädt noch Informationen über neue Builds. Bitte in Kürze nochmals versuchen.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="588"/>
        <source>Starting backup...</source>
        <translation>Erstelle Sicherungskopie ...</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="357"/>
        <source>New installation</source>
        <translation>Neue Installation</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="366"/>
        <source>Your configuration is invalid. This is most likely due to a changed device path. The configuration dialog will now open to allow you to correct the problem.</source>
        <translation>Die Konfiguration ist ungültig. Dies kommt wahrscheinlich von einem geänderten Gerätepfad. Der Konfigurationsdialog wird geöffnet, damit das Problem korrigiert werden kann.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="614"/>
        <source>Backup successful</source>
        <translation>Sicherung erfolgreich</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="239"/>
        <location filename="../rbutilqt.cpp" line="270"/>
        <source>Network error</source>
        <translation>Netzwerkfehler</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="227"/>
        <location filename="../rbutilqt.cpp" line="260"/>
        <source>Downloading build information, please wait ...</source>
        <translation type="unfinished">Lade Informationen über Builds, bitte warten ...</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="238"/>
        <location filename="../rbutilqt.cpp" line="269"/>
        <source>Can&apos;t get version information!</source>
        <translation type="unfinished">Konnte Versionsinformationen nicht ermitteln!</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="281"/>
        <source>Download build information finished.</source>
        <translation type="unfinished">Informationen über Builds heruntergeladen.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="569"/>
        <source>Really continue?</source>
        <translation>Wirklich fortfahren?</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="679"/>
        <source>No install method known.</source>
        <translation>Keine Installationsmethode bekannt.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="706"/>
        <source>Bootloader detected</source>
        <translation>Bootloader erkannt</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="707"/>
        <source>Bootloader already installed. Do you want to reinstall the bootloader?</source>
        <translation>Bootloader ist bereits installiert. Soll der Bootloader neu installiert werden?</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="730"/>
        <source>Create Bootloader backup</source>
        <translation>Erzeuge Sicherungskopie vom Bootloader</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="731"/>
        <source>You can create a backup of the original bootloader file. Press &quot;Yes&quot; to select an output folder on your computer to save the file to. The file will get placed in a new folder &quot;%1&quot; created below the selected folder.
Press &quot;No&quot; to skip this step.</source>
        <translation>Es kann eine Sicherungskopie der originalen Bootloader-Datei erstellt werden. &quot;Ja&quot; um einen Zielordner auf dem Computer auszuwählen. Die Datei wird in einem neuen Unterordner &quot;%1&quot; im gewählten Ordner abgelegt.
&quot;Nein&quot; um diesen Schritt zu überspringen.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="738"/>
        <source>Browse backup folder</source>
        <translation>Ordner für Sicherungskopie suchen</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="750"/>
        <source>Prerequisites</source>
        <translation>Voraussetzungen</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="763"/>
        <source>Select firmware file</source>
        <translation>Firmware-Datei auswählen</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="765"/>
        <source>Error opening firmware file</source>
        <translation>Fehler beim Öffnen der Firmware-Datei</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="771"/>
        <source>Error reading firmware file</source>
        <translation type="unfinished">Fehler beim Lesen der Firmware-Datei</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="781"/>
        <source>Backup error</source>
        <translation>Sicherungskopie-Fehler</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="782"/>
        <source>Could not create backup file. Continue?</source>
        <translation>Konnte Sicherungskopie-Datei nicht erzeugen. Fortfahren?</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="812"/>
        <source>Manual steps required</source>
        <translation>Manuelle Schritte erforderlich</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="427"/>
        <source>Do you really want to perform a complete installation?

This will install Rockbox %1. To install the most recent development build available press &quot;Cancel&quot; and use the &quot;Installation&quot; tab.</source>
        <translation>Wirklich eine vollständige Installation durchführen?

Dies installiert Rockbox %1. Um die letzte Entwicklerversion zu installieren &quot;Abbrechen&quot; wählen und den Reiter &quot;Installation&quot; verwenden.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="483"/>
        <source>Do you really want to perform a minimal installation? A minimal installation will contain only the absolutely necessary parts to run Rockbox.

This will install Rockbox %1. To install the most recent development build available press &quot;Cancel&quot; and use the &quot;Installation&quot; tab.</source>
        <translation>Wirklich eine Minimalinstallation durchführen? Eine Minimalinstallation enthält nur die Teile die zum Verwenden von Rockbox absolut notwendig sind.Dies installiert Rockbox %1. Um die letzte Entwicklerversion zu installieren &quot;Abbrechen&quot; wählen und den Reiter &quot;Installation&quot; verwenden.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="712"/>
        <source>Bootloader installation skipped</source>
        <translation>Bootloader-Installation übersprungen</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="756"/>
        <source>Bootloader installation aborted</source>
        <translation>Bootloader-Installation abgebrochen</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1183"/>
        <source>Checking for update ...</source>
        <translation>Prüfe auf Update ...</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1248"/>
        <source>RockboxUtility Update available</source>
        <translation>Rockbox Utility Update verfügbar</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1249"/>
        <source>&lt;b&gt;New RockboxUtility Version available.&lt;/b&gt; &lt;br&gt;&lt;br&gt;Download it from here: &lt;a href=&apos;%1&apos;&gt;%2&lt;/a&gt;</source>
        <translation>&lt;b&gt;Neue Version von Rockbox Utility verfügbar.&lt;/b&gt;&lt;br/&gt;&lt;br/&gt;Hier herunterladen: &lt;a href=&apos;%1&apos;&gt;%2&lt;/a&gt;</translation>
    </message>
</context>
<context>
    <name>RbUtilQtFrm</name>
    <message>
        <location filename="../rbutilqtfrm.ui" line="14"/>
        <source>Rockbox Utility</source>
        <translation>Rockbox Utility</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="128"/>
        <location filename="../rbutilqtfrm.ui" line="711"/>
        <source>&amp;Quick Start</source>
        <translation>&amp;Schnellstart</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="224"/>
        <location filename="../rbutilqtfrm.ui" line="704"/>
        <source>&amp;Installation</source>
        <translation>&amp;Installation</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="320"/>
        <location filename="../rbutilqtfrm.ui" line="718"/>
        <source>&amp;Extras</source>
        <translation>&amp;Extras</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="552"/>
        <location filename="../rbutilqtfrm.ui" line="734"/>
        <source>&amp;Uninstallation</source>
        <translation>Ent&amp;fernen</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="648"/>
        <source>&amp;Manual</source>
        <translation>&amp;Anleitung</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="674"/>
        <source>&amp;File</source>
        <translation>&amp;Datei</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="775"/>
        <source>&amp;About</source>
        <translation>Ü&amp;ber</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="752"/>
        <source>Empty local download cache</source>
        <translation>Download-Cache löschen</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="757"/>
        <source>Install Rockbox Utility on player</source>
        <translation>Rockbox Utility auf dem Gerät installieren</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="762"/>
        <source>&amp;Configure</source>
        <translation>&amp;Konfigurieren</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="767"/>
        <source>E&amp;xit</source>
        <translation>&amp;Beenden</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="770"/>
        <source>Ctrl+Q</source>
        <translation type="unfinished">Ctrl+Q</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="780"/>
        <source>About &amp;Qt</source>
        <translation>Über &amp;Qt</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="260"/>
        <source>Install Rockbox</source>
        <translation>Rockbox installieren</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="233"/>
        <source>Install Bootloader</source>
        <translation>Bootloader installieren</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="329"/>
        <source>Install Fonts package</source>
        <translation>Schriftarten-Paket installieren</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="356"/>
        <source>Install themes</source>
        <translation>Themes installieren</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="383"/>
        <source>Install game files</source>
        <translation>Spieldateien installieren</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="561"/>
        <source>Uninstall Bootloader</source>
        <translation>Bootloader entfernen</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="555"/>
        <location filename="../rbutilqtfrm.ui" line="588"/>
        <source>Uninstall Rockbox</source>
        <translation>Rockbox entfernen</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="71"/>
        <source>Device</source>
        <translation>Gerät</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="83"/>
        <source>Selected device:</source>
        <translation>Ausgewähltes Gerät:</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="110"/>
        <source>&amp;Change</source>
        <translation>Ä&amp;ndern</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="131"/>
        <source>Welcome</source>
        <translation>Willkommen</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="227"/>
        <source>Basic Rockbox installation</source>
        <translation>Einfache Rockbox-Installation</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="323"/>
        <source>Install extras for Rockbox</source>
        <translation>Installiere Extras für Rockbox</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="373"/>
        <source>&lt;b&gt;Install Themes&lt;/b&gt;&lt;br/&gt;Rockbox&apos;s look can be customized by themes. You can choose and install several officially distributed themes.</source>
        <translation type="unfinished">&lt;b&gt;Themes installieren&lt;/b&gt;&lt;br/&gt;Das Aussehen von Rockbox kann mit Themes angepasst werden. Es lassen sich verschiedene offiziell verfügbare Themes auswählen und installieren.</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="437"/>
        <location filename="../rbutilqtfrm.ui" line="726"/>
        <source>&amp;Accessibility</source>
        <translation type="unfinished">&amp;Zugänglichkeit</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="440"/>
        <source>Install accessibility add-ons</source>
        <translation>Installiere Zugänglichkeits-Erweiterungen</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="446"/>
        <source>Install Voice files</source>
        <translation>Sprachdateien installieren</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="473"/>
        <source>Install Talk files</source>
        <translation>Talk-Dateien installieren</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="651"/>
        <source>View and download the manual</source>
        <translation>Anleitung herunterladen und lesen</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="656"/>
        <source>Inf&amp;o</source>
        <translation>Inf&amp;o</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="683"/>
        <location filename="../rbutilqtfrm.ui" line="785"/>
        <source>&amp;Help</source>
        <translation>&amp;Hilfe</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="137"/>
        <source>Complete Installation</source>
        <translation>Komplette Installation</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="700"/>
        <source>Action&amp;s</source>
        <translation>A&amp;ktionen</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="790"/>
        <source>Info</source>
        <translation>Info</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="894"/>
        <source>Read PDF manual</source>
        <translation>Lese Anleitung im PDF-Format</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="899"/>
        <source>Read HTML manual</source>
        <translation>Lese Anleitung im HTML-Format</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="904"/>
        <source>Download PDF manual</source>
        <translation>Lade Anleitung im PDF-Format herunter</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="909"/>
        <source>Download HTML manual (zip)</source>
        <translation>Lade Anleitung im HTML-Format herunter</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="523"/>
        <source>Create Voice files</source>
        <translation>Erstelle Sprachdateien</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="921"/>
        <source>Create Voice File</source>
        <translation>Erstelle Sprachdatei</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="154"/>
        <source>&lt;b&gt;Complete Installation&lt;/b&gt;&lt;br/&gt;This installs the bootloader, a current build and the extras package. This is the recommended method for new installations.</source>
        <translation type="unfinished">&lt;b&gt;Komplette Installation&lt;/b&gt;&lt;br/&gt;Dies installiert den Bootloader, ein aktuellen Build und die Extra-Pakete. Dies ist die empfohlene Methode für eine neue Installation.</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="250"/>
        <source>&lt;b&gt;Install the bootloader&lt;/b&gt;&lt;br/&gt;Before Rockbox can be run on your audio player, you may have to install a bootloader. This is only necessary the first time Rockbox is installed.</source>
        <translation>&lt;b&gt;Bootloader installieren&lt;/b&gt;&lt;br/&gt;Bevor Rockbox auf dem Gerät läuft muss der Bootloader installiert werden. Dies ist nur bei der ersten Installation notwendig.</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="277"/>
        <source>&lt;b&gt;Install Rockbox&lt;/b&gt; on your audio player</source>
        <translation>&lt;b&gt;Installiere Rockbox&lt;/b&gt; auf dem Gerät</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="346"/>
        <source>&lt;b&gt;Fonts Package&lt;/b&gt;&lt;br/&gt;The Fonts Package contains a couple of commonly used fonts. Installation is highly recommended.</source>
        <translation>&lt;b&gt;Installiere Schriften&lt;/b&gt;&lt;br/&gt;Das Schriftenpaket enthält eine Reihe von häufig benutzen Schriften. Die Installation wird empfohlen.</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="400"/>
        <source>&lt;b&gt;Install Game Files&lt;/b&gt;&lt;br/&gt;Doom needs a base wad file to run.</source>
        <translation>&lt;b&gt;Installiere Spiele-Dateien&lt;/b&gt;&lt;br/&gt;Doom benötigt eine &quot;base wad&quot;-Datei um zu funktionieren.</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="463"/>
        <source>&lt;b&gt;Install Voice file&lt;/b&gt;&lt;br/&gt;Voice files are needed to make Rockbox speak the user interface. Speaking is enabled by default, so if you installed the voice file Rockbox will speak.</source>
        <translation>&lt;b&gt;Installiere Sprachdatei&lt;/b&gt;&lt;br&gt;Sprachdateien werden benötigt, damit Rockbox die Menüs vorlesen kann. Sprachausgabe ist standardmäßig angeschaltet. Sobald eine Sprachdatei installiert ist, werden die Menüs gesprochen.</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="490"/>
        <source>&lt;b&gt;Create Talk Files&lt;/b&gt;&lt;br/&gt;Talkfiles are needed to let Rockbox speak File and Foldernames</source>
        <translation>&lt;b&gt;Erstelle Talk Dateien&lt;/b&gt;&lt;br/&gt;Talkdateien werden benötigt, damit Rockbox Dateien und Ordner vorlesen kann</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="540"/>
        <source>&lt;b&gt;Create Voice file&lt;/b&gt;&lt;br/&gt;Voice files are needed to make Rockbox speak the  user interface. Speaking is enabled by default, so
 if you installed the voice file Rockbox will speak.</source>
        <translation>&lt;b&gt;Erzeuge Sprachdatei&lt;/b&gt;&lt;br&gt; Sprachdateien werden benötigt, damit Rockbox seine Benutzeroberfläche vorlesen kann. Sprachausgabe ist Standardmäßig angeschaltet, sobald sie eine Sprachdatei installieren wird Rockbox sprechen.</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="578"/>
        <source>&lt;b&gt;Remove the bootloader&lt;/b&gt;&lt;br/&gt;After removing the bootloader you won&apos;t be able to start Rockbox.</source>
        <translation>&lt;b&gt;Entferne Bootloader&lt;/b&gt;&lt;br/&gt;Nach dem Entfernen des Bootloaders kann Rockbox nicht mehr gestartet werden.</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="605"/>
        <source>&lt;b&gt;Uninstall Rockbox from your audio player.&lt;/b&gt;&lt;br/&gt;This will leave the bootloader in place (you need to remove it manually).</source>
        <translation>&lt;b&gt;Entferne Rockbox vom Gerät&lt;/b&gt;&lt;br/&gt;Dies wird den Bootloader intakt lassen (er muss manuell entfernt werden).</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="817"/>
        <source>Install &amp;Bootloader</source>
        <translation>Installiere &amp;Bootloader</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="826"/>
        <source>Install &amp;Rockbox</source>
        <translation>Installiere &amp;Rockbox</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="835"/>
        <source>Install &amp;Fonts Package</source>
        <translation>Installiere &amp;Schriften-Paket</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="844"/>
        <source>Install &amp;Themes</source>
        <translation>Installiere &amp;Themen</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="853"/>
        <source>Install &amp;Game Files</source>
        <translation>Installiere &amp;Spiele-Dateien</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="862"/>
        <source>&amp;Install Voice File</source>
        <translation>&amp;Installiere Sprachdateien</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="871"/>
        <source>Create &amp;Talk Files</source>
        <translation>Erstelle &amp;Talk-Dateien</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="880"/>
        <source>Remove &amp;bootloader</source>
        <translation>&amp;Bootloader entfernen</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="889"/>
        <source>Uninstall &amp;Rockbox</source>
        <translation>&amp;Rockbox entfernen</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="918"/>
        <source>Create &amp;Voice File</source>
        <translation>&amp;Sprachdateien erzeugen</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="926"/>
        <source>&amp;System Info</source>
        <translation>&amp;Systeminfo</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="799"/>
        <source>&amp;Complete Installation</source>
        <translation type="unfinished">&amp;Vollständige Installation</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="90"/>
        <source>device / mountpoint unknown or invalid</source>
        <translation type="unfinished">Gerät / Einhängepunkt unbekannt oder ungültig</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="167"/>
        <source>Minimal Installation</source>
        <translation>Minimale Installation</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="184"/>
        <source>&lt;b&gt;Minimal installation&lt;/b&gt;&lt;br/&gt;This installs bootloader and the current build of Rockbox. If you don&apos;t want the extras package, choose this option.</source>
        <translation>&lt;b&gt;Minimale Installation&lt;/b&gt;&lt;br/&gt;Dies installiert Bootloader und die aktuelle Version von Rockbox. Diese Option verwenden wenn keine Zusatzpakete gewünscht werden.</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="808"/>
        <source>&amp;Minimal Installation</source>
        <translation>&amp;Minimale Installation</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="687"/>
        <source>&amp;Troubleshoot</source>
        <translation>&amp;Fehlerbehebung</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="931"/>
        <source>System &amp;Trace</source>
        <translation>System &amp;Trace</translation>
    </message>
</context>
<context>
    <name>ServerInfo</name>
    <message>
        <location filename="../base/serverinfo.cpp" line="71"/>
        <source>Unknown</source>
        <translation>Unbekannt</translation>
    </message>
    <message>
        <location filename="../base/serverinfo.cpp" line="75"/>
        <source>Unusable</source>
        <translation>Unbenutzbar</translation>
    </message>
    <message>
        <location filename="../base/serverinfo.cpp" line="78"/>
        <source>Unstable</source>
        <translation>Instabil</translation>
    </message>
    <message>
        <location filename="../base/serverinfo.cpp" line="81"/>
        <source>Stable</source>
        <translation>Stabil</translation>
    </message>
</context>
<context>
    <name>SysTrace</name>
    <message>
        <location filename="../systrace.cpp" line="75"/>
        <location filename="../systrace.cpp" line="84"/>
        <source>Save system trace log</source>
        <translation>System-Trace Log speichern</translation>
    </message>
</context>
<context>
    <name>SysTraceFrm</name>
    <message>
        <location filename="../systracefrm.ui" line="14"/>
        <source>System Trace</source>
        <translation>System-Trace</translation>
    </message>
    <message>
        <location filename="../systracefrm.ui" line="20"/>
        <source>System State trace</source>
        <translation>System-Status Trace</translation>
    </message>
    <message>
        <location filename="../systracefrm.ui" line="46"/>
        <source>&amp;Close</source>
        <translation>S&amp;chließen</translation>
    </message>
    <message>
        <location filename="../systracefrm.ui" line="57"/>
        <source>&amp;Save</source>
        <translation>&amp;Speichern</translation>
    </message>
    <message>
        <location filename="../systracefrm.ui" line="68"/>
        <source>&amp;Refresh</source>
        <translation>&amp;Aktualisieren</translation>
    </message>
    <message>
        <location filename="../systracefrm.ui" line="79"/>
        <source>Save &amp;previous</source>
        <translation type="unfinished">&amp;vorhergehenden Speichern</translation>
    </message>
</context>
<context>
    <name>Sysinfo</name>
    <message>
        <location filename="../sysinfo.cpp" line="44"/>
        <source>&lt;b&gt;OS&lt;/b&gt;&lt;br/&gt;</source>
        <translation>&lt;b&gt;Betriebssystem&lt;/b&gt;&lt;br/&gt;</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="45"/>
        <source>&lt;b&gt;Username&lt;/b&gt;&lt;br/&gt;%1&lt;hr/&gt;</source>
        <translation>&lt;b&gt;Benutzername&lt;/b&gt;&lt;br/&gt;%1&lt;hr/&gt;</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="47"/>
        <source>&lt;b&gt;Permissions&lt;/b&gt;&lt;br/&gt;%1&lt;hr/&gt;</source>
        <translation>&lt;b&gt;Berechtigungen&lt;/b&gt;&lt;br/&gt;%1&lt;hr/&gt;</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="49"/>
        <source>&lt;b&gt;Attached USB devices&lt;/b&gt;&lt;br/&gt;</source>
        <translation>&lt;b&gt;Angeschlossene USB-Geräte&lt;/b&gt;&lt;br/&gt;</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="53"/>
        <source>VID: %1 PID: %2, %3</source>
        <translation>VID: %1 PID: %2, %3</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="62"/>
        <source>Filesystem</source>
        <translation>Dateisystem</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="65"/>
        <source>Mountpoint</source>
        <translation>Einhängepunkt</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="65"/>
        <source>Label</source>
        <translation>Bezeichnung</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="66"/>
        <source>Free</source>
        <translation>Frei</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="66"/>
        <source>Total</source>
        <translation>Gesamt</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="67"/>
        <source>Cluster Size</source>
        <translation>Cluster-Größe</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="69"/>
        <source>&lt;tr&gt;&lt;td&gt;%1&lt;/td&gt;&lt;td&gt;%4&lt;/td&gt;&lt;td&gt;%2 GiB&lt;/td&gt;&lt;td&gt;%3 GiB&lt;/td&gt;&lt;td&gt;%5&lt;/td&gt;&lt;/tr&gt;</source>
        <translation>&lt;tr&gt;&lt;td&gt;%1&lt;/td&gt;&lt;td&gt;%4&lt;/td&gt;&lt;td&gt;%2 GiB&lt;/td&gt;&lt;td&gt;%3 GiB&lt;/td&gt;&lt;td&gt;%5&lt;/td&gt;&lt;/tr&gt;</translation>
    </message>
</context>
<context>
    <name>SysinfoFrm</name>
    <message>
        <location filename="../sysinfofrm.ui" line="13"/>
        <source>System Info</source>
        <translation>Systeminfo</translation>
    </message>
    <message>
        <location filename="../sysinfofrm.ui" line="22"/>
        <source>&amp;Refresh</source>
        <translation>&amp;Aktualisieren</translation>
    </message>
    <message>
        <location filename="../sysinfofrm.ui" line="45"/>
        <source>&amp;OK</source>
        <translation>&amp;Ok</translation>
    </message>
</context>
<context>
    <name>System</name>
    <message>
        <location filename="../base/system.cpp" line="120"/>
        <source>Guest</source>
        <translation type="unfinished">Gast</translation>
    </message>
    <message>
        <location filename="../base/system.cpp" line="123"/>
        <source>Admin</source>
        <translation type="unfinished">Admin</translation>
    </message>
    <message>
        <location filename="../base/system.cpp" line="126"/>
        <source>User</source>
        <translation type="unfinished">Benutzer</translation>
    </message>
    <message>
        <location filename="../base/system.cpp" line="129"/>
        <source>Error</source>
        <translation type="unfinished">Fehler</translation>
    </message>
    <message>
        <location filename="../base/system.cpp" line="277"/>
        <location filename="../base/system.cpp" line="322"/>
        <source>(no description available)</source>
        <translation>(keine Beschreibung verfügbar)</translation>
    </message>
</context>
<context>
    <name>TTSBase</name>
    <message>
        <location filename="../base/ttsbase.cpp" line="39"/>
        <source>Espeak TTS Engine</source>
        <translation type="unfinished">Espeak TTS-System</translation>
    </message>
    <message>
        <location filename="../base/ttsbase.cpp" line="40"/>
        <source>Flite TTS Engine</source>
        <translation type="unfinished">Flite TTS-System</translation>
    </message>
    <message>
        <location filename="../base/ttsbase.cpp" line="41"/>
        <source>Swift TTS Engine</source>
        <translation type="unfinished">Swift TTS-System</translation>
    </message>
    <message>
        <location filename="../base/ttsbase.cpp" line="43"/>
        <source>SAPI TTS Engine</source>
        <translation type="unfinished">SAPI TTS-System</translation>
    </message>
    <message>
        <location filename="../base/ttsbase.cpp" line="46"/>
        <source>Festival TTS Engine</source>
        <translation type="unfinished">Festival TTS-System</translation>
    </message>
    <message>
        <location filename="../base/ttsbase.cpp" line="49"/>
        <source>OS X System Engine</source>
        <translation type="unfinished">Mac OS X TTS-System</translation>
    </message>
</context>
<context>
    <name>TTSCarbon</name>
    <message>
        <location filename="../base/ttscarbon.cpp" line="138"/>
        <source>Voice:</source>
        <translation type="unfinished">Stimme:</translation>
    </message>
    <message>
        <location filename="../base/ttscarbon.cpp" line="144"/>
        <source>Speed (words/min):</source>
        <translation>Geschwindigkeit (Wörter / Minute):</translation>
    </message>
    <message>
        <location filename="../base/ttscarbon.cpp" line="151"/>
        <source>Pitch (0 for default):</source>
        <translation type="unfinished">Tonhöhe (0 für Standard):</translation>
    </message>
    <message>
        <location filename="../base/ttscarbon.cpp" line="221"/>
        <source>Could not voice string</source>
        <translation>Konnte Text nicht sprechen</translation>
    </message>
    <message>
        <location filename="../base/ttscarbon.cpp" line="231"/>
        <source>Could not convert intermediate file</source>
        <translation>Konnte Zwischendatei nicht umwandeln</translation>
    </message>
</context>
<context>
    <name>TTSExes</name>
    <message>
        <location filename="../base/ttsexes.cpp" line="77"/>
        <source>TTS executable not found</source>
        <translation>TTS-System nicht gefunden</translation>
    </message>
    <message>
        <location filename="../base/ttsexes.cpp" line="42"/>
        <source>Path to TTS engine:</source>
        <translation>Pfad zum TTS-System:</translation>
    </message>
    <message>
        <location filename="../base/ttsexes.cpp" line="44"/>
        <source>TTS engine options:</source>
        <translation>TTS-System Optionen:</translation>
    </message>
</context>
<context>
    <name>TTSFestival</name>
    <message>
        <location filename="../base/ttsfestival.cpp" line="201"/>
        <source>engine could not voice string</source>
        <translation type="unfinished">Konnte String nicht sprechen</translation>
    </message>
    <message>
        <location filename="../base/ttsfestival.cpp" line="284"/>
        <source>No description available</source>
        <translation>keine Beschreibung verfügbar</translation>
    </message>
    <message>
        <location filename="../base/ttsfestival.cpp" line="49"/>
        <source>Path to Festival client:</source>
        <translation>Pfad zu Festival-Client:</translation>
    </message>
    <message>
        <location filename="../base/ttsfestival.cpp" line="54"/>
        <source>Voice:</source>
        <translation>Stimme:</translation>
    </message>
    <message>
        <location filename="../base/ttsfestival.cpp" line="63"/>
        <source>Voice description:</source>
        <translation>Stimmenbeschreibung:</translation>
    </message>
</context>
<context>
    <name>TTSSapi</name>
    <message>
        <location filename="../base/ttssapi.cpp" line="43"/>
        <source>Language:</source>
        <translation>Sprache:</translation>
    </message>
    <message>
        <location filename="../base/ttssapi.cpp" line="49"/>
        <source>Voice:</source>
        <translation>Stimme:</translation>
    </message>
    <message>
        <location filename="../base/ttssapi.cpp" line="59"/>
        <source>Speed:</source>
        <translation>Geschwindigkeit:</translation>
    </message>
    <message>
        <location filename="../base/ttssapi.cpp" line="62"/>
        <source>Options:</source>
        <translation>Optionen:</translation>
    </message>
    <message>
        <location filename="../base/ttssapi.cpp" line="106"/>
        <source>Could not copy the SAPI script</source>
        <translation type="unfinished">Konnte SAPI-Skript nicht kopieren</translation>
    </message>
    <message>
        <location filename="../base/ttssapi.cpp" line="127"/>
        <source>Could not start SAPI process</source>
        <translation type="unfinished">Konnte SAPI-Prozess nicht starten</translation>
    </message>
</context>
<context>
    <name>TalkFileCreator</name>
    <message>
        <location filename="../base/talkfile.cpp" line="35"/>
        <source>Starting Talk file generation</source>
        <translation>Beginne Talkdatei-Erzeugung</translation>
    </message>
    <message>
        <location filename="../base/talkfile.cpp" line="43"/>
        <source>Talk file creation aborted</source>
        <translation>Erzeugen der Sprachdatei abgebrochen</translation>
    </message>
    <message>
        <location filename="../base/talkfile.cpp" line="78"/>
        <source>Finished creating Talk files</source>
        <translation>Erstellen der Sprachdateien beendet</translation>
    </message>
    <message>
        <location filename="../base/talkfile.cpp" line="40"/>
        <source>Reading Filelist...</source>
        <translation>Lese Dateiliste ...</translation>
    </message>
    <message>
        <location filename="../base/talkfile.cpp" line="247"/>
        <source>Copying of %1 to %2 failed</source>
        <translation>Kopieren von %1 nach %2 fehlgeschlagen</translation>
    </message>
    <message>
        <location filename="../base/talkfile.cpp" line="66"/>
        <source>Copying Talkfiles...</source>
        <translation type="unfinished">Kopiere Sprachdateien ...</translation>
    </message>
    <message>
        <location filename="../base/talkfile.cpp" line="229"/>
        <source>File copy aborted</source>
        <translation type="unfinished">Kopieren abgebrochen</translation>
    </message>
    <message>
        <location filename="../base/talkfile.cpp" line="268"/>
        <source>Cleaning up...</source>
        <translation type="unfinished">Räume auf ...</translation>
    </message>
    <message>
        <location filename="../base/talkfile.cpp" line="279"/>
        <source>Finished</source>
        <translation>Fertig</translation>
    </message>
</context>
<context>
    <name>TalkGenerator</name>
    <message>
        <location filename="../base/talkgenerator.cpp" line="38"/>
        <source>Starting TTS Engine</source>
        <translation type="unfinished">Starte TTS-System</translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="43"/>
        <source>Init of TTS engine failed</source>
        <translation type="unfinished">Initalisierung des TTS-Systems fehlgeschlagen</translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="50"/>
        <source>Starting Encoder Engine</source>
        <translation type="unfinished">Starte Encoder</translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="54"/>
        <source>Init of Encoder engine failed</source>
        <translation type="unfinished">Starten des Encoders fehlgeschlagen</translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="64"/>
        <source>Voicing entries...</source>
        <translation type="unfinished">Spreche Einträge ...</translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="79"/>
        <source>Encoding files...</source>
        <translation type="unfinished">Kodiere Dateien ...</translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="118"/>
        <source>Voicing aborted</source>
        <translation type="unfinished">Sprechen abgebrochen</translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="154"/>
        <location filename="../base/talkgenerator.cpp" line="159"/>
        <source>Voicing of %1 failed: %2</source>
        <translation type="unfinished">Sprechen von %1 fehlgeschlagen: %2</translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="203"/>
        <source>Encoding aborted</source>
        <translation type="unfinished">Kodieren abgebrochen</translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="230"/>
        <source>Encoding of %1 failed</source>
        <translation type="unfinished">Kodieren of %1 ist fehlgeschlagen</translation>
    </message>
</context>
<context>
    <name>ThemeInstallFrm</name>
    <message>
        <location filename="../themesinstallfrm.ui" line="13"/>
        <source>Theme Installation</source>
        <translation>Theme-Installation</translation>
    </message>
    <message>
        <location filename="../themesinstallfrm.ui" line="48"/>
        <source>Selected Theme</source>
        <translation>Ausgewähltes Theme</translation>
    </message>
    <message>
        <location filename="../themesinstallfrm.ui" line="73"/>
        <source>Description</source>
        <translation>Beschreibung</translation>
    </message>
    <message>
        <location filename="../themesinstallfrm.ui" line="83"/>
        <source>Download size:</source>
        <translation>Downloadgröße:</translation>
    </message>
    <message>
        <location filename="../themesinstallfrm.ui" line="125"/>
        <source>&amp;Cancel</source>
        <translation>&amp;Abbrechen</translation>
    </message>
    <message>
        <location filename="../themesinstallfrm.ui" line="115"/>
        <source>&amp;Install</source>
        <translation>&amp;Installieren</translation>
    </message>
    <message>
        <location filename="../themesinstallfrm.ui" line="93"/>
        <source>Hold Ctrl to select multiple item, Shift for a range</source>
        <translation>Strg halten um mehrer Einträge, Umschalt einen Bereich auszuwählen</translation>
    </message>
</context>
<context>
    <name>ThemesInstallWindow</name>
    <message>
        <location filename="../themesinstallwindow.cpp" line="38"/>
        <source>no theme selected</source>
        <translation>Kein Theme ausgewählt</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="110"/>
        <source>Network error: %1.
Please check your network and proxy settings.</source>
        <translation>Netzwerkfehler: %1
Bitte Netzwerk- und Proxyeinstellungen überprüfen.</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="123"/>
        <source>the following error occured:
%1</source>
        <translation>Der folgende Fehler ist aufgetreten:
%1</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="129"/>
        <source>done.</source>
        <translation>Abgeschlossen.</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="196"/>
        <source>fetching details for %1</source>
        <translation>lade Details für %1</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="199"/>
        <source>fetching preview ...</source>
        <translation>lade Vorschau ...</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="212"/>
        <source>&lt;b&gt;Author:&lt;/b&gt; %1&lt;hr/&gt;</source>
        <translation>&lt;b&gt;Autor:&lt;/b&gt; %1&lt;hr/&gt;</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="213"/>
        <location filename="../themesinstallwindow.cpp" line="215"/>
        <source>unknown</source>
        <translation>unbekannt</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="214"/>
        <source>&lt;b&gt;Version:&lt;/b&gt; %1&lt;hr/&gt;</source>
        <translation>&lt;b&gt;Version:&lt;/b&gt; %1&lt;hr/&gt;</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="217"/>
        <source>no description</source>
        <translation>Keine Beschreibung vorhanden</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="260"/>
        <source>no theme preview</source>
        <translation>Keine Themevorschau vorhanden</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="291"/>
        <source>getting themes information ...</source>
        <translation>lade Theme-Informationen ...</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="339"/>
        <source>Mount point is wrong!</source>
        <translation>Einhängepunkt ungültig!</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="216"/>
        <source>&lt;b&gt;Description:&lt;/b&gt; %1&lt;hr/&gt;</source>
        <translation>&lt;b&gt;Beschreibung:&lt;/b&gt; %1&lt;hr/&gt;</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="39"/>
        <source>no selection</source>
        <translation>keine Auswahl</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="166"/>
        <source>Information</source>
        <translation>Information</translation>
    </message>
    <message numerus="yes">
        <location filename="../themesinstallwindow.cpp" line="183"/>
        <source>Download size %L1 kiB (%n item(s))</source>
        <translation>
            <numerusform>Download-Größe %L1 kiB (%n Element)</numerusform>
            <numerusform>Download-Größe %L1 kiB (%n Elemente)</numerusform>
        </translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="248"/>
        <source>Retrieving theme preview failed.
HTTP response code: %1</source>
        <translation>Laden der Vorschau fehlgeschlagen.
HTTP Antwortcode: %1</translation>
    </message>
</context>
<context>
    <name>UninstallFrm</name>
    <message>
        <location filename="../uninstallfrm.ui" line="16"/>
        <source>Uninstall Rockbox</source>
        <translation>Rockbox entfernen</translation>
    </message>
    <message>
        <location filename="../uninstallfrm.ui" line="35"/>
        <source>Please select the Uninstallation Method</source>
        <translation>Bitte Methode zum Entfernen auswählen</translation>
    </message>
    <message>
        <location filename="../uninstallfrm.ui" line="45"/>
        <source>Uninstallation Method</source>
        <translation>Methode zum Entfernen</translation>
    </message>
    <message>
        <location filename="../uninstallfrm.ui" line="51"/>
        <source>Complete Uninstallation</source>
        <translation>Vollständiges Entfernen</translation>
    </message>
    <message>
        <location filename="../uninstallfrm.ui" line="58"/>
        <source>Smart Uninstallation</source>
        <translation>Intelligente Entfernen</translation>
    </message>
    <message>
        <location filename="../uninstallfrm.ui" line="68"/>
        <source>Please select what you want to uninstall</source>
        <translation>Bitte die zu entfernenden Teile auswählen</translation>
    </message>
    <message>
        <location filename="../uninstallfrm.ui" line="78"/>
        <source>Installed Parts</source>
        <translation>Installierte Teile</translation>
    </message>
    <message>
        <location filename="../uninstallfrm.ui" line="138"/>
        <source>&amp;Cancel</source>
        <translation>&amp;Abbrechen</translation>
    </message>
    <message>
        <location filename="../uninstallfrm.ui" line="128"/>
        <source>&amp;Uninstall</source>
        <translation>&amp;Entfernen</translation>
    </message>
</context>
<context>
    <name>Uninstaller</name>
    <message>
        <location filename="../base/uninstall.cpp" line="31"/>
        <location filename="../base/uninstall.cpp" line="42"/>
        <source>Starting Uninstallation</source>
        <translation>Beginne Entfernen</translation>
    </message>
    <message>
        <location filename="../base/uninstall.cpp" line="35"/>
        <source>Finished Uninstallation</source>
        <translation>Entfernen erfolgreich</translation>
    </message>
    <message>
        <location filename="../base/uninstall.cpp" line="48"/>
        <source>Uninstalling %1...</source>
        <translation type="unfinished">Entferne %1 ...</translation>
    </message>
    <message>
        <location filename="../base/uninstall.cpp" line="79"/>
        <source>Could not delete %1</source>
        <translation type="unfinished">Konnte %1 nicht löschen</translation>
    </message>
    <message>
        <location filename="../base/uninstall.cpp" line="108"/>
        <source>Uninstallation finished</source>
        <translation>Entfernen erfolgreich</translation>
    </message>
</context>
<context>
    <name>Utils</name>
    <message>
        <location filename="../base/utils.cpp" line="309"/>
        <source>&lt;li&gt;Permissions insufficient for bootloader installation.
Administrator priviledges are necessary.&lt;/li&gt;</source>
        <translation type="unfinished">&lt;li&gt;Bereichtigung für Bootloader-Installation nicht ausreichend.
Administratorrechte sind notwendig.&lt;/li&gt;</translation>
    </message>
    <message>
        <location filename="../base/utils.cpp" line="321"/>
        <source>&lt;li&gt;Target mismatch detected.&lt;br/&gt;Installed target: %1&lt;br/&gt;Selected target: %2.&lt;/li&gt;</source>
        <translation type="unfinished">&lt;li&gt;Abweichendes Gerät entdeckt.&lt;br/&gt;Installiertes Gerät: %1&lt;br/&gt;Ausgewähltes Gerät: %2&lt;/li&gt;</translation>
    </message>
    <message>
        <location filename="../base/utils.cpp" line="328"/>
        <source>Problem detected:</source>
        <translation type="unfinished">Problem gefunden:</translation>
    </message>
</context>
<context>
    <name>VoiceFileCreator</name>
    <message>
        <location filename="../base/voicefile.cpp" line="40"/>
        <source>Starting Voicefile generation</source>
        <translation>Erzeugen der Sprachdatei beginnt</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="98"/>
        <source>Download error: received HTTP error %1.</source>
        <translation>Fehler beim Herunterladen: HTTP Fehler %1.</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="104"/>
        <source>Cached file used.</source>
        <translation>Datei aus Cache verwendet.</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="107"/>
        <source>Download error: %1</source>
        <translation>Downloadfehler: %1</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="112"/>
        <source>Download finished.</source>
        <translation>Download abgeschlossen.</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="120"/>
        <source>failed to open downloaded file</source>
        <translation>Konnte heruntergeladene Datei nicht öffnen</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="174"/>
        <source>The downloaded file was empty!</source>
        <translation>Die heruntergeladene Datei war leer!</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="205"/>
        <source>Error opening downloaded file</source>
        <translation>Konnte heruntergeladene Datei nicht öffnen</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="216"/>
        <source>Error opening output file</source>
        <translation>Konnte Ausgabedatei nicht öffnen</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="236"/>
        <source>successfully created.</source>
        <translation>erfolgreich erzeugt.</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="53"/>
        <source>could not find rockbox-info.txt</source>
        <translation>Konnte rockbox-info.txt nicht finden</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="85"/>
        <source>Downloading voice info...</source>
        <translation type="unfinished">Lade Sprachinformationen herunter ...</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="128"/>
        <source>Reading strings...</source>
        <translation type="unfinished">Lese Strings ...</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="200"/>
        <source>Creating voicefiles...</source>
        <translation type="unfinished">Erzeuge Sprachdateien ...</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="245"/>
        <source>Cleaning up...</source>
        <translation type="unfinished">Räume auf ...</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="256"/>
        <source>Finished</source>
        <translation>Fertig</translation>
    </message>
</context>
<context>
    <name>ZipInstaller</name>
    <message>
        <location filename="../base/zipinstaller.cpp" line="58"/>
        <source>done.</source>
        <translation>Abgeschlossen.</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="66"/>
        <source>Installation finished successfully.</source>
        <translation>Installation erfolgreich abgeschlossen.</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="79"/>
        <source>Downloading file %1.%2</source>
        <translation>Herunterladen von Datei %1.%2</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="113"/>
        <source>Download error: received HTTP error %1.</source>
        <translation>Fehler beim Herunterladen: HTTP Fehler %1.</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="121"/>
        <source>Download error: %1</source>
        <translation>Downloadfehler: %1</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="125"/>
        <source>Download finished.</source>
        <translation>Download abgeschlossen.</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="131"/>
        <source>Extracting file.</source>
        <translation>Extrahiere Datei.</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="151"/>
        <source>Extraction failed!</source>
        <translation type="unfinished">Extrahieren fehlgeschlagen!</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="160"/>
        <source>Installing file.</source>
        <translation>Installiere Datei.</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="171"/>
        <source>Installing file failed.</source>
        <translation>Dateiinstallation fehlgeschlagen.</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="180"/>
        <source>Creating installation log</source>
        <translation>Erstelle Installationslog</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="119"/>
        <source>Cached file used.</source>
        <translation>Datei aus Cache verwendet.</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="144"/>
        <source>Not enough disk space! Aborting.</source>
        <translation>Nicht genügend Speicherplatz verfügbar! Abbruch.</translation>
    </message>
</context>
<context>
    <name>ZipUtil</name>
    <message>
        <location filename="../base/ziputil.cpp" line="118"/>
        <source>Creating output path failed</source>
        <translation type="unfinished">Ausgabepfad konnte nicht erzeugt werden</translation>
    </message>
    <message>
        <location filename="../base/ziputil.cpp" line="125"/>
        <source>Creating output file failed</source>
        <translation type="unfinished">Ausgabedatei konnte nicht geschrieben werden</translation>
    </message>
    <message>
        <location filename="../base/ziputil.cpp" line="134"/>
        <source>Error during Zip operation</source>
        <translation type="unfinished">Fehler bei Zip-Vorgang</translation>
    </message>
</context>
<context>
    <name>aboutBox</name>
    <message>
        <location filename="../aboutbox.ui" line="14"/>
        <source>About Rockbox Utility</source>
        <translation>Über Rockbox Utility</translation>
    </message>
    <message>
        <location filename="../aboutbox.ui" line="74"/>
        <source>&amp;Credits</source>
        <translation>&amp;Credits</translation>
    </message>
    <message>
        <location filename="../aboutbox.ui" line="106"/>
        <source>&amp;License</source>
        <translation>&amp;Lizenz</translation>
    </message>
    <message>
        <location filename="../aboutbox.ui" line="132"/>
        <source>&amp;Speex License</source>
        <translation>&amp;Speex-Lizenz</translation>
    </message>
    <message>
        <location filename="../aboutbox.ui" line="158"/>
        <source>&amp;Ok</source>
        <translation>&amp;Ok</translation>
    </message>
    <message>
        <location filename="../aboutbox.ui" line="32"/>
        <source>The Rockbox Utility</source>
        <translation>Rockbox Utility</translation>
    </message>
    <message utf8="true">
        <location filename="../aboutbox.ui" line="54"/>
        <source>Installer and housekeeping utility for the Rockbox open source digital audio player firmware.&lt;br/&gt;© 2005 - 2012 The Rockbox Team.&lt;br/&gt;Released under the GNU General Public License v2.&lt;br/&gt;Uses icons by the &lt;a href=&quot;http://tango.freedesktop.org/&quot;&gt;Tango Project&lt;/a&gt;.&lt;br/&gt;&lt;center&gt;&lt;a href=&quot;http://www.rockbox.org&quot;&gt;http://www.rockbox.org&lt;/a&gt;&lt;/center&gt;</source>
        <translation>Installations- und Wartungstool für die Open-Source-Firmware für digitale Audioabspieler Rockbox.&lt;br/&gt;© 2005 - 2012 Das Rockbox Team.&lt;br/&gt;Veröffentlicht unter der GNU General Public License v2.&lt;br/&gt;Verwendet Icons des &lt;a href=&quot;http://tango.freedesktop.org/&quot;&gt;Tango-Projekts&lt;/a&gt;.&lt;br/&gt;&lt;center&gt;&lt;a href=&quot;http://www.rockbox.org&quot;&gt;http://www.rockbox.org&lt;/a&gt;&lt;/center&gt;</translation>
    </message>
</context>
</TS>
