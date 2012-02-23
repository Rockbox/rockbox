<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.0" language="he">
<context>
    <name>BootloaderInstallAms</name>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="32"/>
        <source>Bootloader installation requires you to provide a copy of the original Sandisk firmware (bin file). This firmware file will be patched and then installed to your player along with the rockbox bootloader. You need to download this file yourself due to legal reasons. Please browse the &lt;a href=&apos;http://forums.sandisk.com/sansa/&apos;&gt;Sansa Forums&apos;&lt;/a&gt; or refer to the &lt;a href=&apos;http://www.rockbox.org/manual.shtml&apos;&gt;manual&lt;/a&gt; and the &lt;a href=&apos;http://www.rockbox.org/wiki/SansaAMS&apos;&gt;SansaAMS&lt;/a&gt; wiki page on how to obtain this file.&lt;br/&gt;Press Ok to continue and browse your computer for the firmware file.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="55"/>
        <source>Downloading bootloader file</source>
        <translation>מוריד את קובץ מנהל האיתחול</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="97"/>
        <location filename="../base/bootloaderinstallams.cpp" line="110"/>
        <source>Could not load %1</source>
        <translation>לא מצליח לטעון את %1</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="124"/>
        <source>No room to insert bootloader, try another firmware version</source>
        <translation>אין מספיק מקום להכנסת מנהל האיתחול, נסה גירסת קושחה אחרת</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="134"/>
        <source>Patching Firmware...</source>
        <translation>תופר קושחה...</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="145"/>
        <source>Could not open %1 for writing</source>
        <translation>לא מצליח לפתוח את %1 לכתיבה</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="158"/>
        <source>Could not write firmware file</source>
        <translation>לא מצליח לכתוב קובץ קושחה</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="174"/>
        <source>Success: modified firmware file created</source>
        <translation>הצלחה: קובץ קושחה מוסגל נוצר</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="182"/>
        <source>To uninstall, perform a normal upgrade with an unmodified original firmware</source>
        <translation>כדי להסיר את ההתקנה, בצע שדרוג רגיל עם קובץ קושחה מקורי שלא נעשה בו שינוי</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallBase</name>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="124"/>
        <source>Download error: received HTTP error %1.</source>
        <translation>שגיאת הורדה: התקבלה שגיאת HTTP: %1.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="130"/>
        <source>Download error: %1</source>
        <translation>שגיאת הורדה: %1</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="136"/>
        <source>Download finished (cache used).</source>
        <translation>הורדה הסתיימה (נעשה שימוש במטמון).</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="138"/>
        <source>Download finished.</source>
        <translation>הורדה הסתיימה.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="159"/>
        <source>Creating backup of original firmware file.</source>
        <translation>יוצר גיבוי של קובץ מנהל האיתחול המקורי.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="161"/>
        <source>Creating backup folder failed</source>
        <translation>יצירת ספריית גיבוי נכשלה</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="167"/>
        <source>Creating backup copy failed.</source>
        <translation>יצירת עותק גיבוי נכשלה.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="170"/>
        <source>Backup created.</source>
        <translation>גיבוי נוצר.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="183"/>
        <source>Creating installation log</source>
        <translation>יוצר קובץ רישום של ההתקנה</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="207"/>
        <source>Bootloader installation is almost complete. Installation &lt;b&gt;requires&lt;/b&gt; you to perform the following steps manually:</source>
        <translation>התקנת מנהל האיתחול כמעט הסתיימה. ההתקנה &lt;b&gt;מחייבת&lt;/b&gt; שתבצע את הצעדים הבאים באופן ידני:</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="213"/>
        <source>&lt;li&gt;Safely remove your player.&lt;/li&gt;</source>
        <translation>&lt;li&gt;נתק את הנגן שלך בזהירות.&lt;/li&gt;</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="218"/>
        <source>&lt;li&gt;Reboot your player into the original firmware.&lt;/li&gt;&lt;li&gt;Perform a firmware upgrade using the update functionality of the original firmware. Please refer to your player&apos;s manual on details.&lt;br/&gt;&lt;b&gt;Important:&lt;/b&gt; updating the firmware is a critical process that must not be interrupted. &lt;b&gt;Make sure the player is charged before starting the firmware update process.&lt;/b&gt;&lt;/li&gt;&lt;li&gt;After the firmware has been updated reboot your player.&lt;/li&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="229"/>
        <source>&lt;li&gt;Remove any previously inserted microSD card&lt;/li&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="230"/>
        <source>&lt;li&gt;Disconnect your player. The player will reboot and perform an update of the original firmware. Please refer to your players manual on details.&lt;br/&gt;&lt;b&gt;Important:&lt;/b&gt; updating the firmware is a critical process that must not be interrupted. &lt;b&gt;Make sure the player is charged before disconnecting the player.&lt;/b&gt;&lt;/li&gt;&lt;li&gt;After the firmware has been updated reboot your player.&lt;/li&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="329"/>
        <source>Zip file format detected</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="339"/>
        <source>Extracting firmware %1 from archive</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="346"/>
        <source>Error extracting firmware from archive</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="353"/>
        <source>Could not find firmware in archive</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="241"/>
        <source>&lt;li&gt;Turn the player off&lt;/li&gt;&lt;li&gt;Insert the charger&lt;/li&gt;</source>
        <translation>&lt;/il&gt;כבה את הנגן&lt;li&gt;&lt;/il&gt;הכנס את המטען&lt;li&gt;</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="246"/>
        <source>&lt;li&gt;Unplug USB and power adaptors&lt;/li&gt;&lt;li&gt;Hold &lt;i&gt;Power&lt;/i&gt; to turn the player off&lt;/li&gt;&lt;li&gt;Toggle the battery switch on the player&lt;/li&gt;&lt;li&gt;Hold &lt;i&gt;Power&lt;/i&gt; to boot into Rockbox&lt;/li&gt;</source>
        <translation>&lt;/il&gt;נתן את ה- USB ואת ספקי הכח&lt;li&gt;לחץ על &lt;i&gt;Power&lt;/i&gt; על מנת לכבות את הנגן&lt;/il&gt;&lt;li&gt;הזז את מתג הסוללה על הנגן&lt;/il&gt;&lt;li&gt;לחץ על &lt;i&gt;Power&lt;/i&gt; כדי לאתחל לתוך רוקבוקס&lt;/il&gt;&lt;li&gt;</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="252"/>
        <source>&lt;p&gt;&lt;b&gt;Note:&lt;/b&gt; You can safely install other parts first, but the above steps are &lt;b&gt;required&lt;/b&gt; to finish the installation!&lt;/p&gt;</source>
        <translation>&lt;p&gt;&lt;b&gt;שים לב:&lt;/b&gt; אתה יכול להתקין חלקים אחרים קודם לכן ללא חשש, אבל הצעדים לעיל הינם &lt;b&gt;נדרשים&lt;/b&gt; על מנת לסיים את ההתקנה!&lt;/p&gt;</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="266"/>
        <source>Waiting for system to remount player</source>
        <translation>ממתין למערכת שתעגון את הנגן מחדש</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="296"/>
        <source>Player remounted</source>
        <translation>הנגן עוגן מחדש</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="301"/>
        <source>Timeout on remount</source>
        <translation>העגינה ארכה זמן רב מדי</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="195"/>
        <source>Installation log created</source>
        <translation>קובץ רישום של ההתקנה נוצר</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallChinaChip</name>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="33"/>
        <source>Bootloader installation requires you to provide a firmware file of the original firmware (HXF file). You need to download this file yourself due to legal reasons. Please refer to the &lt;a href=&apos;http://www.rockbox.org/manual.shtml&apos;&gt;manual&lt;/a&gt; and the &lt;a href=&apos;http://www.rockbox.org/wiki/OndaVX747#Download_and_extract_a_recent_ve&apos;&gt;OndaVX747&lt;/a&gt; wiki page on how to obtain this file.&lt;br/&gt;Press Ok to continue and browse your computer for the firmware file.</source>
        <translation>התקנת מנהל האיתחול דורשת שתספק קובץ קושחה של הקושחה המקורית (קובץ HXF). עליך להוריד קובץ זה בעצמך מסיבות משפטיות. אנא פנה אל &lt;a href=&apos;http://www.rockbox.org/manual.shtml&apos;&gt;המדריך למשתמש&lt;/a&gt; ולדף הויקי &lt;a href=&apos;http://www.rockbox.org/wiki/OndaVX747#Download_and_extract_a_recent_ve&apos;&gt;OndaVX747&lt;/a&gt; לגבי מידע על איך להשיג קובץ זה.&lt;br/&gt;לחץ OK כדי להמשיך ולהפנות את התוכנה לקובץ הקושחה שברשותך.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="50"/>
        <source>Downloading bootloader file</source>
        <translation>מוריד את קובץ מנהל האיתחול</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="75"/>
        <source>Could not open firmware file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="78"/>
        <source>Could not open bootloader file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="81"/>
        <source>Could not allocate memory</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="84"/>
        <source>Could not load firmware file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="87"/>
        <source>File is not a valid ChinaChip firmware</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="90"/>
        <source>Could not find ccpmp.bin in input file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="93"/>
        <source>Could not open backup file for ccpmp.bin</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="96"/>
        <source>Could not write backup file for ccpmp.bin</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="99"/>
        <source>Could not load bootloader file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="102"/>
        <source>Could not get current time</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="105"/>
        <source>Could not open output file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="108"/>
        <source>Could not write output file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="111"/>
        <source>Unexpected error from chinachippatcher</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>BootloaderInstallFile</name>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="33"/>
        <source>Downloading bootloader</source>
        <translation>מוריד את קובץ מנהל האיתחול</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="42"/>
        <source>Installing Rockbox bootloader</source>
        <translation>מתקין את מנהל האיתחול של רוקבוקס</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="74"/>
        <source>Error accessing output folder</source>
        <translation>שגיאה בגישה לספריית הקלט</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="87"/>
        <source>Bootloader successful installed</source>
        <translation>מנהל האיתחול הותקן בהצלחה</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="97"/>
        <source>Removing Rockbox bootloader</source>
        <translation>מסיר את מנהל האיתחול של רוקבוקס</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="101"/>
        <source>No original firmware file found.</source>
        <translation>קובץ קושחה מקורי לא נמצא.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="107"/>
        <source>Can&apos;t remove Rockbox bootloader file.</source>
        <translation>לא מצליח להסיר את מנהל האיתחול של רוקבוקס.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="112"/>
        <source>Can&apos;t restore bootloader file.</source>
        <translation>לא מצליח לשחזר את קובץ מנהל האיתחול.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="116"/>
        <source>Original bootloader restored successfully.</source>
        <translation>קובץ איתחול מקורי שוחזר בהצלחה.</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallHex</name>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="67"/>
        <source>checking MD5 hash of input file ...</source>
        <translation>בודק MD5 hash של קובץ הקלט...</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="78"/>
        <source>Could not verify original firmware file</source>
        <translation>לא מצליח לאמת קובץ קושחה מקורית</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="93"/>
        <source>Firmware file not recognized.</source>
        <translation>קובץ קושחה לא מזוהה.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="97"/>
        <source>MD5 hash ok</source>
        <translation></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="104"/>
        <source>Firmware file doesn&apos;t match selected player.</source>
        <translation>קובץ קושחה לא מתאים לנגן שזוהה.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="109"/>
        <source>Descrambling file</source>
        <translation>מפענח קובץ</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="117"/>
        <source>Error in descramble: %1</source>
        <translation>שגיאה בפיענוח: %1</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="122"/>
        <source>Downloading bootloader file</source>
        <translation>מוריד את קובץ מנהל האיתחול</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="132"/>
        <source>Adding bootloader to firmware file</source>
        <translation>מוסיף את מנהל האיתחול לקובץ הקושחה</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="170"/>
        <source>could not open input file</source>
        <translation>לא מצליח לפתוח קובץ קלט</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="171"/>
        <source>reading header failed</source>
        <translation>קריאת כותרת נכשלה</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="172"/>
        <source>reading firmware failed</source>
        <translation>קריאת קושחה נכשלה</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="173"/>
        <source>can&apos;t open bootloader file</source>
        <translation>לא מצליח לפתוח קובץ מנהל איתחול</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="174"/>
        <source>reading bootloader file failed</source>
        <translation>קריאת קובץ מנהל האיתחול נכשלה</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="175"/>
        <source>can&apos;t open output file</source>
        <translation>לא מצליח לפתוח קובץ פלט</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="176"/>
        <source>writing output file failed</source>
        <translation>כתיבת קובץ פלט נכשלה</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="178"/>
        <source>Error in patching: %1</source>
        <translation>שגיאה בתפירה: %1</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="189"/>
        <source>Error in scramble: %1</source>
        <translation>שגיאה בהצפנה (ערבוב): %1</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="204"/>
        <source>Checking modified firmware file</source>
        <translation>בודק את קובץ הקושחה המוסגל</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="206"/>
        <source>Error: modified file checksum wrong</source>
        <translation>שגיאה: חישוב checksum של קובץ הקושחה המוסגל נכשל</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="214"/>
        <source>Success: modified firmware file created</source>
        <translation>הצלחה: קובץ קושחה מוסגל נוצר</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="224"/>
        <source>Uninstallation not possible, only installation info removed</source>
        <translation>הסרת ההתקנה אינה אפשרית, רק מידע ההתקנה הוסר</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="245"/>
        <source>Can&apos;t open input file</source>
        <translation>לא מצליח לפתוח קובץ קלט</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="246"/>
        <source>Can&apos;t open output file</source>
        <translation>לא מצליח לפתוח קובץ פלט</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="247"/>
        <source>invalid file: header length wrong</source>
        <translation>קובץ לא תקין: אורך הכותרת אינו נכון</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="248"/>
        <source>invalid file: unrecognized header</source>
        <translation>קובץ לא תקין: כותרת לא מזוהה</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="249"/>
        <source>invalid file: &quot;length&quot; field wrong</source>
        <translation>קובץ לא תקין: שדה &quot;length&quot; אינו תקין</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="250"/>
        <source>invalid file: &quot;length2&quot; field wrong</source>
        <translation>קובץ לא תקין: שדה &quot;length2&quot; אינו תקין</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="251"/>
        <source>invalid file: internal checksum error</source>
        <translation>קובץ לא תקין: שגיאה פנימית בחישוב checksum</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="252"/>
        <source>invalid file: &quot;length3&quot; field wrong</source>
        <translation>קובץ לא תקין: שדה &quot;length3&quot; אינו תקין</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="253"/>
        <source>unknown</source>
        <translation>לא ידוע</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="48"/>
        <source>Bootloader installation requires you to provide a firmware file of the original firmware (hex file). You need to download this file yourself due to legal reasons. Please refer to the &lt;a href=&apos;http://www.rockbox.org/manual.shtml&apos;&gt;manual&lt;/a&gt; and the &lt;a href=&apos;http://www.rockbox.org/wiki/IriverBoot#Download_and_extract_a_recent_ve&apos;&gt;IriverBoot&lt;/a&gt; wiki page on how to obtain this file.&lt;br/&gt;Press Ok to continue and browse your computer for the firmware file.</source>
        <translation>התקנת מנהל האיתחול דורשת שתספק קובץ קושחה של הקושחה המקורית (קובץ bin). עליך להוריד קובץ זה בעצמך מסיבות משפטיות. אנא פנה אל &lt;a href=&apos;http://www.rockbox.org/manual.shtml&apos;&gt;המדריך למשתמש&lt;/a&gt; ולדף הויקי &lt;a href=&apos;http://www.rockbox.org/wiki/IriverBoot#Download_and_extract_a_recent_ve&apos;&gt;IriverBoot&lt;/a&gt; לגבי מידע על איך להשיג קובץ זה.&lt;br/&gt;לחץ OK כדי להמשיך ולהפנות את התוכנה לקובץ הקושחה שברשותך.</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallImx</name>
    <message>
        <location filename="../base/bootloaderinstallimx.cpp" line="69"/>
        <source>Bootloader installation requires you to provide a copy of the original Sandisk firmware (firmware.sb file). This file will be patched with the Rockbox bootloader and installed to your player. You need to download this file yourself due to legal reasons. Please browse the &lt;a href=&apos;http://forums.sandisk.com/sansa/&apos;&gt;Sansa Forums&lt;/a&gt; or refer to the &lt;a href= &apos;http://www.rockbox.org/wiki/SansaFuzePlus&apos;&gt;SansaFuzePlus&lt;/a&gt; wiki page on how to obtain this file.&lt;br/&gt;Press Ok to continue and browse your computer for the firmware file.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallimx.cpp" line="91"/>
        <source>Could not read original firmware file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallimx.cpp" line="97"/>
        <source>Downloading bootloader file</source>
        <translation type="unfinished">מוריד את קובץ מנהל האיתחול</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallimx.cpp" line="107"/>
        <source>Patching file...</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallimx.cpp" line="134"/>
        <source>Patching the original firmware failed</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallimx.cpp" line="140"/>
        <source>Succesfully patched firmware file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallimx.cpp" line="155"/>
        <source>Bootloader successful installed</source>
        <translation type="unfinished">מנהל האיתחול הותקן בהצלחה</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallimx.cpp" line="161"/>
        <source>Patched bootloader could not be installed</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallimx.cpp" line="172"/>
        <source>To uninstall, perform a normal upgrade with an unmodified original firmware.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>BootloaderInstallIpod</name>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="53"/>
        <source>Error: can&apos;t allocate buffer memory!</source>
        <translation>שגיאה: לא יכול להקצות זיכרון חוצץ!</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="81"/>
        <source>Downloading bootloader file</source>
        <translation>מוריד את קובץ מנהל האיתחול</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="65"/>
        <location filename="../base/bootloaderinstallipod.cpp" line="152"/>
        <source>Failed to read firmware directory</source>
        <translation>לא מצליח לקרוא את ספריית מנהל האיתחול</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="70"/>
        <location filename="../base/bootloaderinstallipod.cpp" line="157"/>
        <source>Unknown version number in firmware (%1)</source>
        <translation>מספר גירסה לא מוכר בקושחה (%1)</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="76"/>
        <source>Warning: This is a MacPod, Rockbox only runs on WinPods. 
See http://www.rockbox.org/wiki/IpodConversionToFAT32</source>
        <translation>אזהרה: זוהי גירסת MacPod, רוקבוקס רצה רק על נגני WinPos.
ראה http://www.rockbox.org/wiki/IpodConversionToFAT32</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="95"/>
        <location filename="../base/bootloaderinstallipod.cpp" line="164"/>
        <source>Could not open Ipod in R/W mode</source>
        <translation>לא מצליח לפתוח את האייפוד במצב קריאה/כתיבה</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="105"/>
        <source>Successfull added bootloader</source>
        <translation>מנהל האיתחול התווסף בהצלחה</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="116"/>
        <source>Failed to add bootloader</source>
        <translation>כישלון בהוספת מנהל האיתחול</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="128"/>
        <source>Bootloader Installation complete.</source>
        <translation>מנהל האיתחול הותקן בהצלחה</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="133"/>
        <source>Writing log aborted</source>
        <translation>כתיבת היומן בוטלה</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="170"/>
        <source>No bootloader detected.</source>
        <translation>לא זוהה מנהל איתחול.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="176"/>
        <source>Successfully removed bootloader</source>
        <translation>מנהל האיתחול הוסר בהצלחה</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="183"/>
        <source>Removing bootloader failed.</source>
        <translation>הסרת מנהל האיתחול נכשלה.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="230"/>
        <source>Error: could not retrieve device name</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="246"/>
        <source>Error: no mountpoint specified!</source>
        <translation>שגיאה: נקודת עגינה לא צויינה!</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="251"/>
        <source>Could not open Ipod: permission denied</source>
        <translation>לא מצליח לפתוח את האייפוד: הגישה נדחתה</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="255"/>
        <source>Could not open Ipod</source>
        <translation>לא מצליח לפתוח אייפוד</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="266"/>
        <source>No firmware partition on disk</source>
        <translation>אין מחיצת מנהל איתחול על הדיסק</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="91"/>
        <source>Installing Rockbox bootloader</source>
        <translation>מתקין את מנהל האיתחול של רוקבוקס</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="143"/>
        <source>Uninstalling bootloader</source>
        <translation>מסיר את מנהל האיתחול</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="260"/>
        <source>Error reading partition table - possibly not an Ipod</source>
        <translation>שגיאה בקריאת טבלת המחיצות - ככל הנראה נגן זה אינו אייפוד</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallMi4</name>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="33"/>
        <source>Downloading bootloader</source>
        <translation>מוריד את קובץ מנהל האיתחול</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="42"/>
        <source>Installing Rockbox bootloader</source>
        <translation>מתקין את מנהל האיתחול של רוקבוקס</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="65"/>
        <source>Bootloader successful installed</source>
        <translation>מנהל האיתחול הותקן בהצלחה</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="77"/>
        <source>Checking for Rockbox bootloader</source>
        <translation>מחפש את מנהל האיתחול של רוקבוקס</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="79"/>
        <source>No Rockbox bootloader found</source>
        <translation>לא נמצא מנהל איתחול של רוקבוקס</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="84"/>
        <source>Checking for original firmware file</source>
        <translation>מחפש קובץ קושחה מקורי</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="89"/>
        <source>Error finding original firmware file</source>
        <translation>שגיאה בחיפוש קובץ קושחה מקורי</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="99"/>
        <source>Rockbox bootloader successful removed</source>
        <translation>מנהל האיתחול של רוקבוקס הוסר בהצלחה</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallMpio</name>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="33"/>
        <source>Bootloader installation requires you to provide a firmware file of the original firmware (bin file). You need to download this file yourself due to legal reasons. Please refer to the &lt;a href=&apos;http://www.rockbox.org/manual.shtml&apos;&gt;manual&lt;/a&gt; and the &lt;a href=&apos;http://www.rockbox.org/wiki/MPIOHD200Port&apos;&gt;MPIOHD200Port&lt;/a&gt; wiki page on how to obtain this file.&lt;br/&gt;Press Ok to continue and browse your computer for the firmware file.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="52"/>
        <source>Downloading bootloader file</source>
        <translation type="unfinished">מוריד את קובץ מנהל האיתחול</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="79"/>
        <source>Could not open the original firmware.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="82"/>
        <source>Could not read the original firmware.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="85"/>
        <source>Loaded firmware file does not look like MPIO original firmware file.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="100"/>
        <source>Could not open output file.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="103"/>
        <source>Could not write output file.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="106"/>
        <source>Unknown error number: %1</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="88"/>
        <source>Could not open downloaded bootloader.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="91"/>
        <source>Place for bootloader in OF file not empty.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="94"/>
        <source>Could not read the downloaded bootloader.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="97"/>
        <source>Bootloader checksum error.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="110"/>
        <location filename="../base/bootloaderinstallmpio.cpp" line="111"/>
        <source>Patching original firmware failed: %1</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="118"/>
        <source>Success: modified firmware file created</source>
        <translation type="unfinished">הצלחה: קובץ קושחה מוסגל נוצר</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="126"/>
        <source>To uninstall, perform a normal upgrade with an unmodified original firmware</source>
        <translation type="unfinished">כדי להסיר את ההתקנה, בצע שדרוג רגיל עם קובץ קושחה מקורי שלא נעשה בו שינוי</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallSansa</name>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="55"/>
        <source>Error: can&apos;t allocate buffer memory!</source>
        <translation>שגיאה: לא יכול להקצות זיכרון חוצץ!</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="60"/>
        <source>Searching for Sansa</source>
        <translation>מחפש נגן מסוג סנסה</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="66"/>
        <source>Permission for disc access denied!
This is required to install the bootloader</source>
        <translation>אין גישה לדיסק!
דבר זה נדרש על מנת להתקין את מנהל האיתחול</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="73"/>
        <source>No Sansa detected!</source>
        <translation>לא נמצאו נגני סנסה!</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="86"/>
        <source>Downloading bootloader file</source>
        <translation>מוריד את קובץ מנהל האיתחול</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="78"/>
        <location filename="../base/bootloaderinstallsansa.cpp" line="188"/>
        <source>OLD ROCKBOX INSTALLATION DETECTED, ABORTING.
You must reinstall the original Sansa firmware before running
sansapatcher for the first time.
See http://www.rockbox.org/wiki/SansaE200Install
</source>
        <translation>זוהתה התקנה ישנה של רוקבוקס. מבטל.
עליך להתקין מחדש את הקושחה המקורית של הנגן בטרם תפעיל
את sansapatcher בפעם הראשונה.
ראה http://www.rockbox.org/wiki/SansaE200Install
</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="110"/>
        <location filename="../base/bootloaderinstallsansa.cpp" line="198"/>
        <source>Could not open Sansa in R/W mode</source>
        <translation>לא מצליח לפתוח את נגן הסנסה במצב קריאה/כתיבה</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="137"/>
        <source>Successfully installed bootloader</source>
        <translation>מנהל האיתחול הותקן בהצלחה</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="148"/>
        <source>Failed to install bootloader</source>
        <translation>התקנת מנהל האיתחול נכשלה</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="161"/>
        <source>Bootloader Installation complete.</source>
        <translation>מנהל האיתחול הותקן בהצלחה</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="166"/>
        <source>Writing log aborted</source>
        <translation>כתיבת היומן בוטלה</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="248"/>
        <source>Error: could not retrieve device name</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="264"/>
        <source>Can&apos;t find Sansa</source>
        <translation>לא מצליח למצוא נגן סנסה</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="269"/>
        <source>Could not open Sansa</source>
        <translation>לא מצליח לפתוח את נגן הסנסה</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="274"/>
        <source>Could not read partition table</source>
        <translation>לא מצליח לקרוא את טבלת המחיצות</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="281"/>
        <source>Disk is not a Sansa (Error %1), aborting.</source>
        <translation>הדיסק אינו נגן סנסה (שגיאה %1), מבטל.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="204"/>
        <source>Successfully removed bootloader</source>
        <translation>מנהל האיתחול הוסר בהצלחה</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="211"/>
        <source>Removing bootloader failed.</source>
        <translation>הסרת מנהל האיתחול נכשלה.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="102"/>
        <source>Installing Rockbox bootloader</source>
        <translation>מתקין את מנהל האיתחול של רוקבוקס</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="119"/>
        <source>Checking downloaded bootloader</source>
        <translation>בודק את מנהל האיתחול שהורד</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="127"/>
        <source>Bootloader mismatch! Aborting.</source>
        <translation>מנהל האיתחול אינו תואם! מבטל.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="179"/>
        <source>Uninstalling bootloader</source>
        <translation>מסיר את מנהל האיתחול</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallTcc</name>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="33"/>
        <source>Bootloader installation requires you to provide a firmware file of the original firmware (bin file). You need to download this file yourself due to legal reasons. Please refer to the &lt;a href=&apos;http://www.rockbox.org/manual.shtml&apos;&gt;manual&lt;/a&gt; and the &lt;a href=&apos;http://www.rockbox.org/wiki/CowonD2Info&apos;&gt;CowonD2Info&lt;/a&gt; wiki page on how to obtain the file.&lt;br/&gt;Press Ok to continue and browse your computer for the firmware file.</source>
        <translation>התקנת מנהל האיתחול דורשת שתספק קובץ קושחה של הקושחה המקורית (קובץ bin). עליך להוריד קובץ זה בעצמך מסיבות משפטיות. אנא פנה אל &lt;a href=&apos;http://www.rockbox.org/manual.shtml&apos;&gt;המדריך למשתמש&lt;/a&gt; ולדף הויקי &lt;a href=&apos;http://www.rockbox.org/wiki/CowonD2Info&apos;&gt;CowonD2Info&lt;/a&gt; לגבי מידע על איך להשיג קובץ זה.&lt;br/&gt;לחץ OK כדי להמשיך ולהפנות את התוכנה לקובץ הקושחה שברשותך.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="50"/>
        <source>Downloading bootloader file</source>
        <translation>מוריד את קובץ מנהל האיתחול</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="82"/>
        <location filename="../base/bootloaderinstalltcc.cpp" line="99"/>
        <source>Could not load %1</source>
        <translation>לא מצליח לטעון את %1</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="90"/>
        <source>Unknown OF file used: %1</source>
        <translation>משתמש בקובץ קושחה מקורית לא ידוע: %1</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="104"/>
        <source>Patching Firmware...</source>
        <translation>תופר קושחה...</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="111"/>
        <source>Could not patch firmware</source>
        <translation>לא מצליח לתפור את הקושחה</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="117"/>
        <source>Could not open %1 for writing</source>
        <translation>לא מצליח לפתוח את %1 לכתיבה</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="126"/>
        <source>Could not write firmware file</source>
        <translation>לא מצליח לכתוב קובץ קושחה</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="131"/>
        <source>Success: modified firmware file created</source>
        <translation>הצלחה: קובץ קושחה מוסגל נוצר</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="151"/>
        <source>To uninstall, perform a normal upgrade with an unmodified original firmware</source>
        <translation>כדי להסיר את ההתקנה, בצע שדרוג רגיל עם קובץ קושחה מקורי שלא נעשה בו שינוי</translation>
    </message>
</context>
<context>
    <name>Config</name>
    <message>
        <location filename="../configure.cpp" line="301"/>
        <source>Current cache size is %L1 kiB.</source>
        <translation>גודל זכרון מטמון נוכחי הוא %L1 kiB.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="310"/>
        <source>Showing disabled targets</source>
        <translation>מציג נגנים שאינם מאופשרים</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="311"/>
        <source>You just enabled showing targets that are marked disabled. Disabled targets are not recommended to end users. Please use this option only if you know what you are doing.</source>
        <translation>אפשרת הצגת נגנים המסומנים כלא מאופשרים. נגנים אלו אינם מומלצים למשתמשי קצה. אנא השתמש באפשרות זו רק אם אתה יודע מה אתה עושה.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="418"/>
        <location filename="../configure.cpp" line="448"/>
        <source>Configuration OK</source>
        <translation>ההגדרות תקינות</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="424"/>
        <location filename="../configure.cpp" line="453"/>
        <source>Configuration INVALID</source>
        <translation>ההגדרות אינן תקינות</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="493"/>
        <source>Proxy Detection</source>
        <translation>זיהוי פרוקסי</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="494"/>
        <source>The System Proxy settings are invalid!
Rockbox Utility can&apos;t work with this proxy settings. Make sure the system proxy is set correctly. Note that &quot;proxy auto-config (PAC)&quot; scripts are not supported by Rockbox Utility. If your system uses this you need to use manual proxy settings.</source>
        <translation>הגדרות הפרוקסי של המערכת אינן תקינות
תוכנת השירות של רוקבוקס אינה יכולה לעבוד עם הגדרות פרוקסי אלו. אנא וודא שהגדרות הפרוקסי של המערכת הינן נכונות. שים לב שהגדרות פרוקסי אוטומאטיות (proxy auto config - PAC) אינן נתמכות על ידי תוכנה זו. אם אלו הן הגדרות המערכת שלך עליך להשתמש בהגדרות פרוקסי ידניות.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="607"/>
        <source>Set Cache Path</source>
        <translation>קבע נתיב מטמון</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="736"/>
        <source>%1 &quot;MacPod&quot; found!
Rockbox needs a FAT formatted Ipod (so-called &quot;WinPod&quot;) to run. </source>
        <translation>נמצא %1 &quot;MacPod&quot;!
רוקבוקס זקוקה לאייפוד המפורמט בשיטת FAT (נגנים אלי מכונים WinPod) על מנת לרוץ.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="750"/>
        <source>Fatal error</source>
        <translation>טעות מכרעת</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="755"/>
        <source>Detected an unsupported player:
%1
Sorry, Rockbox doesn&apos;t run on your player.</source>
        <translation>זוהה נגן לא נתמך:
%1
מצטערים, רוקבוקס איננה יכולה לרוץ על הנגן שלך.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="760"/>
        <source>Fatal: player incompatible</source>
        <translation>תקלה מכרעת: נגן לא תואם</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="840"/>
        <source>TTS configuration invalid</source>
        <translation>הגדרות מנוע דיבור אינן תקינות</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="841"/>
        <source>TTS configuration invalid. 
 Please configure TTS engine.</source>
        <translation>הגדרות מנוע דיבור אינן תקינות.
אנא הגדר את תצורת מנוע הדיבור.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="846"/>
        <source>Could not start TTS engine.</source>
        <translation>לא מצליח להתחיל את מנוע הדיבור.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="847"/>
        <source>Could not start TTS engine.
</source>
        <translation>לא מצליח להתחיל את מנוע הדיבור.
</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="848"/>
        <location filename="../configure.cpp" line="867"/>
        <source>
Please configure TTS engine.</source>
        <translation>
אנא הגדר את תצורת מנוע הדיבור.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="862"/>
        <source>Rockbox Utility Voice Test</source>
        <translation>בדיקת דיבור של תוכנת השרות של רוקבוקס</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="865"/>
        <source>Could not voice test string.</source>
        <translation>לא מצליח להקריא את מחרוזת הבדיקה.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="866"/>
        <source>Could not voice test string.
</source>
        <translation>לא מצליח להקריא את מחרוזת הבדיקה.
</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="771"/>
        <location filename="../configure.cpp" line="780"/>
        <source>Autodetection</source>
        <translation>זיהוי אוטומטי</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="743"/>
        <source>%1 in MTP mode found!
You need to change your player to MSC mode for installation. </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="748"/>
        <source>Until you change this installation will fail!</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="772"/>
        <source>Could not detect a Mountpoint.
Select your Mountpoint manually.</source>
        <translation>לא מזהה נקודת עגינה.
בחר את נקודת העגינה ידנית.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="781"/>
        <source>Could not detect a device.
Select your device and Mountpoint manually.</source>
        <translation>לא מזהה נגן.
בחר את הנגן ונקודת העגינה ידנית.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="792"/>
        <source>Really delete cache?</source>
        <translation>באמת למחוק את המטמון?</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="793"/>
        <source>Do you really want to delete the cache? Make absolutely sure this setting is correct as it will remove &lt;b&gt;all&lt;/b&gt; files in this folder!</source>
        <translation>האם באמת ברצונך למחוק את המטמון? אנא וודא שברצונך לבצע פעולה זו נכונה, כיוון שהיא תמחק את &lt;b&gt;כל&lt;/b&gt; הקבצים בספרייה זו!</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="801"/>
        <source>Path wrong!</source>
        <translation>נתיב שגוי!</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="802"/>
        <source>The cache path is invalid. Aborting.</source>
        <translation>נתיב זכרון המטמון שגוי. מבטל.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="122"/>
        <source>The following errors occurred:</source>
        <translation>השגיאות הבאות התרחשו:</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="156"/>
        <source>No mountpoint given</source>
        <translation>לא ניתנה נקודת עגינה</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="160"/>
        <source>Mountpoint does not exist</source>
        <translation>נקודת העגינה אינה קיימת</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="164"/>
        <source>Mountpoint is not a directory.</source>
        <translation>נקודת העגינה איננה ספרייה.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="168"/>
        <source>Mountpoint is not writeable</source>
        <translation>נקודת העגינה אינה ניתנת לכתיבה</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="183"/>
        <source>No player selected</source>
        <translation>לא נבחר כל נגן</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="190"/>
        <source>Cache path not writeable. Leave path empty to default to systems temporary path.</source>
        <translation>נתיב המטמון אינו ניתן לכתיבה. השאר את הנתיב ריק על מנת להשתמש בנתיב הקבצים הזמניים לפי ברירת המחדל של המערכת.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="210"/>
        <source>You need to fix the above errors before you can continue.</source>
        <translation>עליך לתקן את השגיאות לעיל לפני שאתה יכול להמשיך.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="213"/>
        <source>Configuration error</source>
        <translation>שגיאת הגדרות</translation>
    </message>
</context>
<context>
    <name>ConfigForm</name>
    <message>
        <location filename="../configurefrm.ui" line="14"/>
        <source>Configuration</source>
        <translation>הגדרות</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="20"/>
        <source>Configure Rockbox Utility</source>
        <translation>הגדרות תוכנת השירות של רוקבוקס</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="35"/>
        <source>&amp;Device</source>
        <translation>&amp;נגן</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="41"/>
        <source>Select your device in the &amp;filesystem</source>
        <translation>בחר את הנגן שלך ב&amp;מערכת הקבצים</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="312"/>
        <source>&amp;Browse</source>
        <translation>&amp;עיון</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="72"/>
        <source>&amp;Select your audio player</source>
        <translation>&amp;בחר את נגן השמע שלך</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="58"/>
        <source>&amp;Refresh</source>
        <translation type="unfinished">&amp;רענן</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="95"/>
        <source>Show disabled targets</source>
        <translation>הצג נגנים שאינם מאופשרים</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="114"/>
        <source>&amp;Autodetect</source>
        <translation>&amp;זיהוי אוטומטי</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="136"/>
        <source>&amp;Proxy</source>
        <translation>&amp;פרוקסי</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="142"/>
        <source>&amp;No Proxy</source>
        <translation>&amp;ללא פרוקסי</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="152"/>
        <source>Use S&amp;ystem values</source>
        <translation>השתמש בהגדרות &amp;מערכת</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="159"/>
        <source>&amp;Manual Proxy settings</source>
        <translation>הגדרות פרוקסי &amp;ידניות</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="166"/>
        <source>Proxy Values</source>
        <translation>ערכי פרוקסי</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="172"/>
        <source>&amp;Host:</source>
        <translation>&amp;שרת:</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="189"/>
        <source>&amp;Port:</source>
        <translation>&amp;פורט:</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="212"/>
        <source>&amp;Username</source>
        <translation>&amp;שם משתמש</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="222"/>
        <source>Pass&amp;word</source>
        <translation>&amp;סיסמה</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="253"/>
        <source>&amp;Language</source>
        <translation>ש&amp;פה</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="267"/>
        <source>Cac&amp;he</source>
        <translation>מ&amp;טמון</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="270"/>
        <source>Download cache settings</source>
        <translation>הגדרות זכרון מטמון של הורדות</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="276"/>
        <source>Rockbox Utility uses a local download cache to save network traffic. You can change the path to the cache and use it as local repository by enabling Offline mode.</source>
        <translation>תוכנת רוקבוקס משתמשת במטמון הורדות מקומי על מנת לחסוך בתעבורת רשת. באפשרותך לשנות את הנתיב למטמון ולהשתמש בו כמאגר מקומי, באמצעות איפשור מצב לא מקוון.</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="286"/>
        <source>Current cache size is %1</source>
        <translation>גודל מטמון נוכחי הוא %1</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="295"/>
        <source>P&amp;ath</source>
        <translation>&amp;נתיב</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="305"/>
        <source>Entering an invalid folder will reset the path to the systems temporary path.</source>
        <translation>הזנת נתיב שגוי תאפס את הנתיב לספריית הקבצים הזמניים של המערכת.</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="327"/>
        <source>Disable local &amp;download cache</source>
        <translation>ביטול מטמון &amp;הורדות מקומי</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="334"/>
        <source>&lt;p&gt;This will try to use all information from the cache, even information about updates. Only use this option if you want to install without network connection. Note: you need to do the same install you want to perform later with network access first to download all required files to the cache.&lt;/p&gt;</source>
        <translation>&lt;p&gt;בצורה זו התוכנה תנסה להשתמש בכל במידע מהמטמון, אף כאשר מדובר במידע על עדכונים. השתמש באפשרות זו רק אם ברצונך להתקין ללא חיבור לרשת. הערה: אם ברצונך לבצע התקנות חוזרות ללא חיבור לרשת, עליך לבצע התקנה ראשונה במצב מקוון על מנת להוריד את כל הקבצים הדרושים למטמון.&lt;/p&gt;</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="337"/>
        <source>O&amp;ffline mode</source>
        <translation>מצב &amp;לא מקוון</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="372"/>
        <source>Clean cache &amp;now</source>
        <translation>&amp;נקה מטמון עכשיו</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="388"/>
        <source>&amp;TTS &amp;&amp; Encoder</source>
        <translation>מנוע &amp;דיבור ומקודד</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="394"/>
        <source>TTS Engine</source>
        <translation>מנוע דיבור</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="400"/>
        <source>&amp;Select TTS Engine</source>
        <translation>&amp;בחר מנוע דיבור</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="413"/>
        <source>Configure TTS Engine</source>
        <translation>הגדרות מנוע דיבור</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="420"/>
        <location filename="../configurefrm.ui" line="471"/>
        <source>Configuration invalid!</source>
        <translation>הגדרות שגויות!</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="437"/>
        <source>Configure &amp;TTS</source>
        <translation>הגדרות מנוע &amp;דיבור</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="448"/>
        <source>Test TTS</source>
        <translation>בדוק מנוע דיבור</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="455"/>
        <source>&amp;Use string corrections for TTS</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="465"/>
        <source>Encoder Engine</source>
        <translation>מנוע מקודד</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="488"/>
        <source>Configure &amp;Enc</source>
        <translation>הגדרות &amp;מקודד</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="499"/>
        <source>encoder name</source>
        <translation>שם מקודד</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="539"/>
        <source>&amp;Ok</source>
        <translation>&amp;אישור</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="550"/>
        <source>&amp;Cancel</source>
        <translation>&amp;ביטול</translation>
    </message>
</context>
<context>
    <name>Configure</name>
    <message>
        <location filename="../configure.cpp" line="553"/>
        <source>English</source>
        <comment>This is the localized language name, i.e. your language.</comment>
        <translation>עברית</translation>
    </message>
</context>
<context>
    <name>CreateVoiceFrm</name>
    <message>
        <location filename="../createvoicefrm.ui" line="16"/>
        <source>Create Voice File</source>
        <translation>צור קובץ הקראת תפריטים</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="41"/>
        <source>Select the Language you want to generate a voicefile for:</source>
        <translation>בחר את השפה עבור הינך מעונייך ליצור קובץ הקראת תפריטים:</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="48"/>
        <source>Language</source>
        <translation>שפה</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="55"/>
        <source>Generation settings</source>
        <translation>הגדרות יצירת קובץ הקראה</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="61"/>
        <source>Encoder profile:</source>
        <translation>פרופיל מקודד:</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="68"/>
        <source>TTS profile:</source>
        <translation>פרופיל מנוע דיבור:</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="81"/>
        <source>Change</source>
        <translation>שינוי</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="132"/>
        <source>&amp;Install</source>
        <translation>&amp;התקנה</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="142"/>
        <source>&amp;Cancel</source>
        <translation>&amp;ביטול</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="156"/>
        <location filename="../createvoicefrm.ui" line="163"/>
        <source>Wavtrim Threshold</source>
        <translation>ערך סף לקטעון</translation>
    </message>
</context>
<context>
    <name>CreateVoiceWindow</name>
    <message>
        <location filename="../createvoicewindow.cpp" line="97"/>
        <location filename="../createvoicewindow.cpp" line="100"/>
        <source>Selected TTS engine: &lt;b&gt;%1&lt;/b&gt;</source>
        <translation>בחר מנוע דיבור: &lt;b&gt;%1&lt;/b&gt;</translation>
    </message>
    <message>
        <location filename="../createvoicewindow.cpp" line="108"/>
        <location filename="../createvoicewindow.cpp" line="111"/>
        <location filename="../createvoicewindow.cpp" line="115"/>
        <source>Selected encoder: &lt;b&gt;%1&lt;/b&gt;</source>
        <translation>בחר מקודד: &lt;b&gt;%1&lt;/b&gt;</translation>
    </message>
</context>
<context>
    <name>EncTtsCfgGui</name>
    <message>
        <location filename="../encttscfggui.cpp" line="31"/>
        <source>Waiting for engine...</source>
        <translation>ממתין למנוע...</translation>
    </message>
    <message>
        <location filename="../encttscfggui.cpp" line="81"/>
        <source>Ok</source>
        <translation>אישור</translation>
    </message>
    <message>
        <location filename="../encttscfggui.cpp" line="84"/>
        <source>Cancel</source>
        <translation>ביטול</translation>
    </message>
    <message>
        <location filename="../encttscfggui.cpp" line="183"/>
        <source>Browse</source>
        <translation>עיין</translation>
    </message>
    <message>
        <location filename="../encttscfggui.cpp" line="191"/>
        <source>Refresh</source>
        <translation>רענן</translation>
    </message>
    <message>
        <location filename="../encttscfggui.cpp" line="363"/>
        <source>Select executable</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>EncoderExe</name>
    <message>
        <location filename="../base/encoderexe.cpp" line="40"/>
        <source>Path to Encoder:</source>
        <translation type="unfinished">נתיב למקודד:</translation>
    </message>
    <message>
        <location filename="../base/encoderexe.cpp" line="42"/>
        <source>Encoder options:</source>
        <translation type="unfinished">אפשרויות קידוד:</translation>
    </message>
</context>
<context>
    <name>EncoderLame</name>
    <message>
        <location filename="../base/encoderlame.cpp" line="69"/>
        <location filename="../base/encoderlame.cpp" line="79"/>
        <source>LAME</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/encoderlame.cpp" line="71"/>
        <source>Volume</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/encoderlame.cpp" line="75"/>
        <source>Quality</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/encoderlame.cpp" line="79"/>
        <source>Could not find libmp3lame!</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>EncoderRbSpeex</name>
    <message>
        <location filename="../base/encoderrbspeex.cpp" line="33"/>
        <source>Volume:</source>
        <translation type="unfinished">עוצמת קול:</translation>
    </message>
    <message>
        <location filename="../base/encoderrbspeex.cpp" line="35"/>
        <source>Quality:</source>
        <translation type="unfinished">איכות:</translation>
    </message>
    <message>
        <location filename="../base/encoderrbspeex.cpp" line="37"/>
        <source>Complexity:</source>
        <translation type="unfinished">מורכבות:</translation>
    </message>
    <message>
        <location filename="../base/encoderrbspeex.cpp" line="39"/>
        <source>Use Narrowband:</source>
        <translation type="unfinished">השתמש בפס-צר:</translation>
    </message>
</context>
<context>
    <name>InfoWidget</name>
    <message>
        <location filename="../gui/infowidget.cpp" line="29"/>
        <source>File</source>
        <translation type="unfinished">קובץ</translation>
    </message>
    <message>
        <location filename="../gui/infowidget.cpp" line="29"/>
        <source>Version</source>
        <translation type="unfinished">גירסה</translation>
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
        <translation type="unfinished">חבילות מותקנות. &lt;br/&gt;&lt;b&gt;שים לב:&lt;/b&gt; אם התקנת חבילות ידנית, מידע זה עשוי שלא להיות מדויק!</translation>
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
        <translation>התקן קבצי הקראת תפריטים</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="36"/>
        <source>Select the Folder to generate Talkfiles for.</source>
        <translation>בחר את הספריה בה יווצרו קבצי הקראת התפריטים.</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="43"/>
        <source>Talkfile Folder</source>
        <translation>ספריית קבצי הקראת התפריטים</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="50"/>
        <source>&amp;Browse</source>
        <translation>&amp;עיין</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="61"/>
        <source>Generation settings</source>
        <translation>הגדרת יצירת קבצי הקראת תפריטים</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="67"/>
        <source>Encoder profile:</source>
        <translation>פרופיל מקודד:</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="74"/>
        <source>TTS profile:</source>
        <translation>פרופיל מנוע דיבור:</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="87"/>
        <source>Change</source>
        <translation>שינוי</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="162"/>
        <source>Generation options</source>
        <translation>אפשרויות יצירת קבצי הקראת תפריטים</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="171"/>
        <source>Ignore files (comma seperated Wildcards):</source>
        <translatorcomment>תו כל = wildcard</translatorcomment>
        <translation>התעלם מקבצים (תו-כלים מופרדים בפסיקים):</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="201"/>
        <source>Run recursive</source>
        <translation>פעל בצורה רקורסיבית</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="211"/>
        <source>Strip Extensions</source>
        <translation>הסר סיומות קבצים</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="221"/>
        <source>Create only new Talkfiles</source>
        <translation>צור רק קבצי הקראת תפריטים חדשים</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="191"/>
        <source>Generate .talk files for Folders</source>
        <translation>צור קבצי talk. עבור ספריות</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="178"/>
        <source>Generate .talk files for Files</source>
        <translation>צור קבצי talk. עבור קבצים</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="138"/>
        <source>&amp;Install</source>
        <translation>&amp;התקנה</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="149"/>
        <source>&amp;Cancel</source>
        <translation>&amp;ביטול</translation>
    </message>
</context>
<context>
    <name>InstallTalkWindow</name>
    <message>
        <location filename="../installtalkwindow.cpp" line="54"/>
        <source>Select folder to create talk files</source>
        <translation>בחר את הספריה בה יווצרו קבצי talk</translation>
    </message>
    <message>
        <location filename="../installtalkwindow.cpp" line="89"/>
        <source>The Folder to Talk is wrong!</source>
        <translation>הספרייה להקראה הינה שגוייה!</translation>
    </message>
    <message>
        <location filename="../installtalkwindow.cpp" line="122"/>
        <location filename="../installtalkwindow.cpp" line="125"/>
        <source>Selected TTS engine: &lt;b&gt;%1&lt;/b&gt;</source>
        <translation>מנוע הדיבור שנבחר: &lt;b&gt;%1&lt;/b&gt;</translation>
    </message>
    <message>
        <location filename="../installtalkwindow.cpp" line="132"/>
        <location filename="../installtalkwindow.cpp" line="135"/>
        <location filename="../installtalkwindow.cpp" line="139"/>
        <source>Selected encoder: &lt;b&gt;%1&lt;/b&gt;</source>
        <translation>המקודד שנבחר: &lt;b&gt;%1&lt;/b&gt;</translation>
    </message>
</context>
<context>
    <name>InstallWindow</name>
    <message>
        <location filename="../installwindow.cpp" line="107"/>
        <source>Backup to %1</source>
        <translation>גיבוי ל %1</translation>
    </message>
    <message>
        <location filename="../installwindow.cpp" line="137"/>
        <source>Mount point is wrong!</source>
        <translation>נקודת העגינה הינה שגויה!</translation>
    </message>
    <message>
        <location filename="../installwindow.cpp" line="174"/>
        <source>Really continue?</source>
        <translation>באמת להמשיך?</translation>
    </message>
    <message>
        <location filename="../installwindow.cpp" line="178"/>
        <source>Aborted!</source>
        <translation>בוטל!</translation>
    </message>
    <message>
        <location filename="../installwindow.cpp" line="187"/>
        <source>Beginning Backup...</source>
        <translation>מתחיל גיבוי...</translation>
    </message>
    <message>
        <location filename="../installwindow.cpp" line="209"/>
        <source>Backup finished.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../installwindow.cpp" line="212"/>
        <source>Backup failed!</source>
        <translation>הגיבוי נכשל!</translation>
    </message>
    <message>
        <location filename="../installwindow.cpp" line="243"/>
        <source>Select Backup Filename</source>
        <translation>בחר את שם קובץ הגיבוי</translation>
    </message>
    <message>
        <location filename="../installwindow.cpp" line="276"/>
        <source>This is the absolute up to the minute Rockbox built. A current build will get updated every time a change is made. Latest version is %1 (%2).</source>
        <translation>זוהי הגירסה העדכנית ביותר בהחלט של רוקבוקס. הגירסה הנוכחית מתעדכנת בכל פעם ששינוי נעשה. הגירסה האחרונה היא %1 (%2).</translation>
    </message>
    <message>
        <location filename="../installwindow.cpp" line="282"/>
        <source>&lt;b&gt;This is the recommended version.&lt;/b&gt;</source>
        <translation>&lt;b&gt;זוהי הגירסה המומלצת.&lt;/b&gt;</translation>
    </message>
    <message>
        <location filename="../installwindow.cpp" line="293"/>
        <source>This is the last released version of Rockbox.</source>
        <translation>זוהי הגירסה המשוחררת האחרונה של רוקבוקס.</translation>
    </message>
    <message>
        <location filename="../installwindow.cpp" line="296"/>
        <source>&lt;b&gt;Note:&lt;/b&gt; The lastest released version is %1. &lt;b&gt;This is the recommended version.&lt;/b&gt;</source>
        <translation>&lt;b&gt;שים לב:&lt;/b&gt; הגירסה המשוחררת האחרונה היא %1. &lt;b&gt;זוהי הגירסה המומלצת.&lt;/b&gt;</translation>
    </message>
    <message>
        <location filename="../installwindow.cpp" line="308"/>
        <source>These are automatically built each day from the current development source code. This generally has more features than the last stable release but may be much less stable. Features may change regularly.</source>
        <translation>גירסאות אלו נבנות אוטומטית בכל יום מקוד המקור העדכני. לגירסאות אלו תכונות רבות יותר מהגירסה היציבה, אך מאידך הן עלולות להיות יציבות פחות. תכונות עשויות להשתנות לעיתים קרובות.</translation>
    </message>
    <message>
        <location filename="../installwindow.cpp" line="312"/>
        <source>&lt;b&gt;Note:&lt;/b&gt; archived version is %1 (%2).</source>
        <translation>&lt;b&gt;שים לב:&lt;/b&gt; גירסת הארכיב היא %1 (%2).</translation>
    </message>
</context>
<context>
    <name>InstallWindowFrm</name>
    <message>
        <location filename="../installwindowfrm.ui" line="16"/>
        <source>Install Rockbox</source>
        <translation>התקן את רוקבוקס</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="35"/>
        <source>Please select the Rockbox version you want to install on your player:</source>
        <translation>אנא בחר את גירסת הרוקבוקס שברצונך להתקין על הנגן:</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="45"/>
        <source>Version</source>
        <translation>גירסה</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="51"/>
        <source>Rockbox &amp;stable</source>
        <translation>גירסה &amp;יציבה</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="58"/>
        <source>&amp;Archived Build</source>
        <translation>גירסת &amp;ארכיון</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="65"/>
        <source>&amp;Current Build</source>
        <translation>גירסה &amp;נוכחית</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="75"/>
        <source>Details</source>
        <translation>פרטים</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="81"/>
        <source>Details about the selected version</source>
        <translation>פרטים אודות הגירסה שנבחרה</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="91"/>
        <source>Note</source>
        <translation>הערה</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="119"/>
        <source>&amp;Install</source>
        <translation>&amp;התקנה</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="130"/>
        <source>&amp;Cancel</source>
        <translation>&amp;ביטול</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="156"/>
        <source>Backup</source>
        <translation>גיבוי</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="162"/>
        <source>Backup before installing</source>
        <translation>גיבוי לפני התקנה</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="169"/>
        <source>Backup location</source>
        <translation>מיקום הגיבוי</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="188"/>
        <source>Change</source>
        <translation>שינוי</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="198"/>
        <source>Rockbox Utility stores copies of Rockbox it has downloaded on the local hard disk to save network traffic. If your local copy is no longer working, tick this box to download a fresh copy.</source>
        <translation>תוכנת השירות של רוקבוקס מאחסנת עותקים של רוקבוקס שהיא הורידה על הדיסק הקשיח המקומי על מנת לחסוך בתעבורת רשת. אם העותק המקומי שלך אינו עובד יותר, סמן תיבה זו על מנת להוריד עותק חדש.</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="201"/>
        <source>&amp;Don&apos;t use locally cached copy</source>
        <translation>&amp;אל תשתמש בעותק שמור מקומי</translation>
    </message>
</context>
<context>
    <name>ManualWidget</name>
    <message>
        <location filename="../gui/manualwidget.cpp" line="78"/>
        <source>&lt;a href=&apos;%1&apos;&gt;PDF Manual&lt;/a&gt;</source>
        <translation type="unfinished">&lt;a href=&apos;%1&apos;&gt;מדריך PDF&lt;/a&gt;</translation>
    </message>
    <message>
        <location filename="../gui/manualwidget.cpp" line="80"/>
        <source>&lt;a href=&apos;%1&apos;&gt;HTML Manual (opens in browser)&lt;/a&gt;</source>
        <translation type="unfinished">&lt;a href=&apos;%1&apos;&gt;מדריך HTML (נפתח בדפדפן)&lt;/a&gt;</translation>
    </message>
    <message>
        <location filename="../gui/manualwidget.cpp" line="84"/>
        <source>Select a device for a link to the correct manual</source>
        <translation type="unfinished">בחר בנגן בשביל קישור למדריך המתאים</translation>
    </message>
    <message>
        <location filename="../gui/manualwidget.cpp" line="85"/>
        <source>&lt;a href=&apos;%1&apos;&gt;Manual Overview&lt;/a&gt;</source>
        <translation type="unfinished">&lt;a href=&apos;%1&apos;&gt;רשימת כל מדריכי המשתמש&lt;/a&gt;</translation>
    </message>
    <message>
        <location filename="../gui/manualwidget.cpp" line="96"/>
        <source>Confirm download</source>
        <translation type="unfinished">אשר הורדה</translation>
    </message>
    <message>
        <location filename="../gui/manualwidget.cpp" line="97"/>
        <source>Do you really want to download the manual? The manual will be saved to the root folder of your player.</source>
        <translation type="unfinished">האם באמת ברצונך להוריד את המדריך למשתמש? המדריך יישמר בספרייה הראשית של הנגן שלך.</translation>
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
        <translation type="unfinished">קרא את המדריך למשתמש</translation>
    </message>
    <message>
        <location filename="../gui/manualwidgetfrm.ui" line="26"/>
        <source>PDF manual</source>
        <translation type="unfinished">PDF מדריך למשתמש מסוג</translation>
    </message>
    <message>
        <location filename="../gui/manualwidgetfrm.ui" line="39"/>
        <source>HTML manual</source>
        <translation type="unfinished">HTML מדריך למשתמש מסוג</translation>
    </message>
    <message>
        <location filename="../gui/manualwidgetfrm.ui" line="55"/>
        <source>Download the manual</source>
        <translation type="unfinished">הורד את המדריך למשתמש</translation>
    </message>
    <message>
        <location filename="../gui/manualwidgetfrm.ui" line="63"/>
        <source>&amp;PDF version</source>
        <translation type="unfinished">&amp;גירסת PDF</translation>
    </message>
    <message>
        <location filename="../gui/manualwidgetfrm.ui" line="70"/>
        <source>&amp;HTML version (zip file)</source>
        <translation type="unfinished">&amp;גירסת HTML (קובץ מכווץ)</translation>
    </message>
    <message>
        <location filename="../gui/manualwidgetfrm.ui" line="92"/>
        <source>Down&amp;load</source>
        <translation type="unfinished">&amp;הורדה</translation>
    </message>
</context>
<context>
    <name>PreviewFrm</name>
    <message>
        <location filename="../previewfrm.ui" line="16"/>
        <source>Preview</source>
        <translation>תצוגה מקדימה</translation>
    </message>
</context>
<context>
    <name>ProgressLoggerFrm</name>
    <message>
        <location filename="../progressloggerfrm.ui" line="13"/>
        <location filename="../progressloggerfrm.ui" line="19"/>
        <source>Progress</source>
        <translation>התקדמות</translation>
    </message>
    <message>
        <location filename="../progressloggerfrm.ui" line="58"/>
        <source>Save Log</source>
        <translation>שמור לוג</translation>
    </message>
    <message>
        <location filename="../progressloggerfrm.ui" line="82"/>
        <source>&amp;Abort</source>
        <translation>&amp;ביטול</translation>
    </message>
    <message>
        <location filename="../progressloggerfrm.ui" line="32"/>
        <source>progresswindow</source>
        <translation>חלון התקדמות</translation>
    </message>
</context>
<context>
    <name>ProgressLoggerGui</name>
    <message>
        <location filename="../progressloggergui.cpp" line="121"/>
        <source>&amp;Ok</source>
        <translation>&amp;אישור</translation>
    </message>
    <message>
        <location filename="../progressloggergui.cpp" line="144"/>
        <source>Save system trace log</source>
        <translation>שמור קובץ יומן מערכת</translation>
    </message>
    <message>
        <location filename="../progressloggergui.cpp" line="103"/>
        <source>&amp;Abort</source>
        <translation>&amp;ביטול</translation>
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
        <translation>RTL</translation>
    </message>
    <message>
        <location filename="../base/system.cpp" line="384"/>
        <source>(unknown vendor name) </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/system.cpp" line="402"/>
        <source>(unknown product name)</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>QuaZipFile</name>
    <message>
        <location filename="../quazip/quazipfile.cpp" line="141"/>
        <source>ZIP/UNZIP API error %1</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>RbUtilQt</name>
    <message>
        <location filename="../rbutilqt.cpp" line="227"/>
        <location filename="../rbutilqt.cpp" line="260"/>
        <source>Downloading build information, please wait ...</source>
        <translation>מוריד מידע גירסאות, אנא המתן...</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="238"/>
        <location filename="../rbutilqt.cpp" line="269"/>
        <source>Can&apos;t get version information!</source>
        <translation>לא מצליח לקבל את מידע הגירסאות!</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="240"/>
        <location filename="../rbutilqt.cpp" line="271"/>
        <source>Can&apos;t get version information.
Network error: %1. Please check your network and proxy settings.</source>
        <translation>לא מצליח להשיג מידע גירסה.
שגיאת רשת: %1. אנא בדוק את הרשת והגדרות הפרוקסי.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="357"/>
        <source>New installation</source>
        <translation>התקנה חדשה</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="358"/>
        <source>This is a new installation of Rockbox Utility, or a new version. The configuration dialog will now open to allow you to setup the program,  or review your settings.</source>
        <translation>זוהי התקנה חדשה של תוכנת השירות של רוקבוקס, או גירסה חדשה. תיבת השיח של ההגדרות תיפתח כעת על מנת לאפשר לך להגדיר את התוכנית, או לעבור על ההגדרות הקיימות.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="365"/>
        <location filename="../rbutilqt.cpp" line="1159"/>
        <source>Configuration error</source>
        <translation>שגיאת הגדרות</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="366"/>
        <source>Your configuration is invalid. This is most likely due to a changed device path. The configuration dialog will now open to allow you to correct the problem.</source>
        <translation>ההגדרות שלך שגויות. לרוב הדבר נובע מכך שהנתיב לנגן השתנה. תיבת השיח של ההגדרות תיפתח כעת, על מנת לאפשר לך לתקן את הבעיה.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="281"/>
        <source>Download build information finished.</source>
        <translation>הורדת מידע הגירסאות הסתיימה.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="411"/>
        <source>&lt;b&gt;%1 %2&lt;/b&gt; at &lt;b&gt;%3&lt;/b&gt;</source>
        <translation>&lt;b&gt;%1 %2&lt;/b&gt; ב- &lt;b&gt;%3&lt;/b&gt;</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="426"/>
        <location filename="../rbutilqt.cpp" line="482"/>
        <location filename="../rbutilqt.cpp" line="659"/>
        <location filename="../rbutilqt.cpp" line="830"/>
        <location filename="../rbutilqt.cpp" line="917"/>
        <location filename="../rbutilqt.cpp" line="961"/>
        <source>Confirm Installation</source>
        <translation>אשר התקנה</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="505"/>
        <location filename="../rbutilqt.cpp" line="1105"/>
        <source>Mount point is wrong!</source>
        <translation>נקודת העגינה הינה שגויה!</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="573"/>
        <source>Aborted!</source>
        <translation>בוטל!</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="583"/>
        <source>Installed Rockbox detected</source>
        <translation>זוהתה התקנה של רוקבוקס</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="584"/>
        <source>Rockbox installation detected. Do you want to backup first?</source>
        <translation>זוהתה התקנה של רוקבוקס. האם ברצונך לגבות לפני כן?</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="588"/>
        <source>Starting backup...</source>
        <translation>מתחיל גיבוי...</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="600"/>
        <source>Beginning Backup...</source>
        <translation type="unfinished">מתחיל גיבוי...</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="614"/>
        <source>Backup successful</source>
        <translation>הגיבוי הסתיים בהצלחה</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="618"/>
        <source>Backup failed!</source>
        <translation>הגיבוי נכשל!</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="660"/>
        <source>Do you really want to install the Bootloader?</source>
        <translation>האם באמת ברצונך להתקין את מנהל האיתחול?</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="771"/>
        <source>Error reading firmware file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="824"/>
        <location filename="../rbutilqt.cpp" line="902"/>
        <source>No Rockbox installation found</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="825"/>
        <source>Could not determine the installed Rockbox version. Please install a Rockbox build before installing fonts.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="831"/>
        <source>Do you really want to install the fonts package?</source>
        <translation>האם באמת ברצונך להתקין את חבילת הגופנים?</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="888"/>
        <source>Warning</source>
        <translation>אזהרה</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="889"/>
        <source>The Application is still downloading Information about new Builds. Please try again shortly.</source>
        <translation>תוכנית השרות עדיין מורידה מידע לגבי גירסאות חדשות. אנא נסה שוב בקרוב.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="903"/>
        <source>Could not determine the installed Rockbox version. Please install a Rockbox build before installing voice files.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="918"/>
        <source>Do you really want to install the voice file?</source>
        <translation>האם באמת ברצונך להתקין את קובץ הקראת התפריטים?</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="956"/>
        <source>Error</source>
        <translation>שגיאה</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="957"/>
        <source>Your device doesn&apos;t have a doom plugin. Aborting.</source>
        <translation>לנגן שברשותך אין הרחבת doom. מבטל.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="962"/>
        <source>Do you really want to install the game addon files?</source>
        <translation>האם באמת ברצונך להתקין את קבצי הרחבות המשחקים?</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1040"/>
        <source>Confirm Uninstallation</source>
        <translation>אשר הסרת התקנה</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1041"/>
        <source>Do you really want to uninstall the Bootloader?</source>
        <translation>האם באמת ברצונך להסיר את מנהל האיתחול?</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1055"/>
        <source>No uninstall method for this target known.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1072"/>
        <source>Rockbox Utility can not uninstall the bootloader on this target. Try a normal firmware update to remove the booloader.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1091"/>
        <source>Confirm installation</source>
        <translation>אשר התקנה</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1092"/>
        <source>Do you really want to install Rockbox Utility to your player? After installation you can run it from the players hard drive.</source>
        <translation>האם באמת ברצונך להתקין את תוכנית השרות של רוקבוקס לנגן שלך? לאחר ההתקנה תוכל להריץ אותה מהתקן האיחסון של הנגן.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1101"/>
        <source>Installing Rockbox Utility</source>
        <translation>מתקין את תוכנת השרות של רוקבוקס</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1119"/>
        <source>Error installing Rockbox Utility</source>
        <translation>שגיאה בהתקנת תוכנת השרות של רוקבוקס</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1123"/>
        <source>Installing user configuration</source>
        <translation>מתקין הגדרות משתמש</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1127"/>
        <source>Error installing user configuration</source>
        <translation>שגיאה בהתקנת הגדרות משתמש</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1131"/>
        <source>Successfully installed Rockbox Utility.</source>
        <translation>התקנת תוכנית השרות של רוקבוקס הסתיימה בהצלחה.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1160"/>
        <source>Your configuration is invalid. Please go to the configuration dialog and make sure the selected values are correct.</source>
        <translation>ההגדרות שלך שגויות. אנא עבור לתיבת השיח של ההגדרות וודא שהערכים הנכונים נבחרו.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1183"/>
        <source>Checking for update ...</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1248"/>
        <source>RockboxUtility Update available</source>
        <translation>קיים עידכון של תוכנת השירות של רוקבוקס</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1249"/>
        <source>&lt;b&gt;New RockboxUtility Version available.&lt;/b&gt; &lt;br&gt;&lt;br&gt;Download it from here: &lt;a href=&apos;%1&apos;&gt;%2&lt;/a&gt;</source>
        <translation>&lt;b&gt;קיימת גירסה חדשה של תוכנית השרות&lt;/b&gt; &lt;br&gt;&lt;br&gt;ניתן להורידה מכאן: &lt;a href=&apos;%1&apos;&gt;%2&lt;/a&gt;</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1252"/>
        <source>New version of Rockbox Utility available.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1255"/>
        <source>Rockbox Utility is up to date.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="239"/>
        <location filename="../rbutilqt.cpp" line="270"/>
        <source>Network error</source>
        <translation>שגיאת רשת</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="569"/>
        <source>Really continue?</source>
        <translation>באמת להמשיך?</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="679"/>
        <source>No install method known.</source>
        <translation>אין שיטת התקנה ידועה.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="706"/>
        <source>Bootloader detected</source>
        <translation>מנהל האיתחול זוהה</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="707"/>
        <source>Bootloader already installed. Do you want to reinstall the bootloader?</source>
        <translation>מנהל האיתחול כבר מותקן. האם אתה באמת רוצה להתקין מחדש את מנהל האיתחול?</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="730"/>
        <source>Create Bootloader backup</source>
        <translation>יוצר גיבוי מנהל האיתחול</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="731"/>
        <source>You can create a backup of the original bootloader file. Press &quot;Yes&quot; to select an output folder on your computer to save the file to. The file will get placed in a new folder &quot;%1&quot; created below the selected folder.
Press &quot;No&quot; to skip this step.</source>
        <translation>ביכולתך ליצור גיבוי של קובץ מנהל האיתחול המקורי. לחץ &quot;כן&quot; על מנת לבחור ספריית פלט על המחשב שלך אליה יישמר הקובץ, אשר יימצא תחת ספרייה חדשה בשם &quot;%1&quot; מתחת לספרייה שנבחרה.
לחץ &quot;לא&quot; כדי לדלג על שלב זה.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="738"/>
        <source>Browse backup folder</source>
        <translation>עיין בספריית הגיבוי</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="750"/>
        <source>Prerequisites</source>
        <translation>דרישות מוקדמות</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="763"/>
        <source>Select firmware file</source>
        <translation>בחר קובץ קושחה</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="765"/>
        <source>Error opening firmware file</source>
        <translation>שגיאה בפתיחת קובץ קושחה</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="781"/>
        <source>Backup error</source>
        <translation>שגיאת גיבוי</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="782"/>
        <source>Could not create backup file. Continue?</source>
        <translation>לא מצליח ליצור קובץ גיבוי. להמשיך?</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="812"/>
        <source>Manual steps required</source>
        <translation>צעדים ידניים נדרשים</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="427"/>
        <source>Do you really want to perform a complete installation?

This will install Rockbox %1. To install the most recent development build available press &quot;Cancel&quot; and use the &quot;Installation&quot; tab.</source>
        <translation>האם באמת ברצונך לבצע התקנה אוטומאטית?

דבר זה יתקין את רוקבוקס %1. על מנת להתקין את גירסת הפיתוח המעודכנת ביותר לחץ &quot;ביטול&quot; והשתמש בלשונית ה&quot;התקנה&quot;.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="103"/>
        <source>Wine detected!</source>
        <translation>Wine זוהתה!</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="104"/>
        <source>It seems you are trying to run this program under Wine. Please don&apos;t do this, running under Wine will fail. Use the native Linux binary instead.</source>
        <translation>נראה שאתה מנסה להריץ את תוכנה זו תחת Wine. אנא הימנע מכך, כיוון שהריצה תיכשל. השתמש בקבצים הבינאריים של לינוקס במקום.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="483"/>
        <source>Do you really want to perform a minimal installation? A minimal installation will contain only the absolutely necessary parts to run Rockbox.

This will install Rockbox %1. To install the most recent development build available press &quot;Cancel&quot; and use the &quot;Installation&quot; tab.</source>
        <translation>האם באמת ברצונך לבצע התקנה מינימאלית? התקנה זו מכילה אך ורק את החלקים ההכרחיים ביותר להפעלת רוקבוקס.

דבר זה יתקין את רוקבוקס %1. על מנת להתקין את גירסת הפיתוח המעודכנת ביותר לחץ &quot;ביטול&quot; והשתמש בלשונית ה&quot;התקנה&quot;.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="712"/>
        <source>Bootloader installation skipped</source>
        <translation>התקנת מנהל האיתחול לא בוצעה</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="756"/>
        <source>Bootloader installation aborted</source>
        <translation>התקנת מנהל האיתחול בוטלה</translation>
    </message>
</context>
<context>
    <name>RbUtilQtFrm</name>
    <message>
        <location filename="../rbutilqtfrm.ui" line="14"/>
        <source>Rockbox Utility</source>
        <translation>תוכנת השרות של רוקבוקס</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="71"/>
        <source>Device</source>
        <translation>נגן</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="83"/>
        <source>Selected device:</source>
        <translation>הנגן שנבחר:</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="90"/>
        <source>device / mountpoint unknown or invalid</source>
        <translation>נגן / נקודת עגינה לא ידועים או שגויים</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="110"/>
        <source>&amp;Change</source>
        <translation>&amp;שינוי</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="128"/>
        <location filename="../rbutilqtfrm.ui" line="711"/>
        <source>&amp;Quick Start</source>
        <translation>התחלה &amp;מהירה</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="131"/>
        <source>Welcome</source>
        <translation>ברוכים הבאים</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="137"/>
        <source>Complete Installation</source>
        <translation>התקנה מלאה</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="154"/>
        <source>&lt;b&gt;Complete Installation&lt;/b&gt;&lt;br/&gt;This installs the bootloader, a current build and the extras package. This is the recommended method for new installations.</source>
        <translation>&lt;b&gt;התקנה מלאה&lt;/b&gt;&lt;br/&gt;התקנת מנהל האיתחול, הגירסה הנוכחית של רוקבוקס וחבילת התוספות. זוהי הבחירה המומלצת עבור התקנות חדשות.</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="224"/>
        <location filename="../rbutilqtfrm.ui" line="704"/>
        <source>&amp;Installation</source>
        <translation>&amp;התקנה</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="227"/>
        <source>Basic Rockbox installation</source>
        <translation>התקנה בסיסית של רוקבוקס</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="233"/>
        <source>Install Bootloader</source>
        <translation>התקנת מנהל האיתחול</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="250"/>
        <source>&lt;b&gt;Install the bootloader&lt;/b&gt;&lt;br/&gt;Before Rockbox can be run on your audio player, you may have to install a bootloader. This is only necessary the first time Rockbox is installed.</source>
        <translation>&lt;b&gt;התקנת מנהל האיתחול&lt;/b&gt;&lt;br/&gt;בטרם ניתן להריץ את רוקבוקס על הנגן שלך, ייתכן ותצטרך להתקין מנהל איתחול. הדבר נדרש רק בפעם הראשונה שרוקבוקס מותקנת.</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="260"/>
        <source>Install Rockbox</source>
        <translation>התקן את רוקבוקס</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="277"/>
        <source>&lt;b&gt;Install Rockbox&lt;/b&gt; on your audio player</source>
        <translation>&lt;b&gt;התקן את רוקבוקס&lt;/b&gt; על הנגן שלך</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="320"/>
        <location filename="../rbutilqtfrm.ui" line="718"/>
        <source>&amp;Extras</source>
        <translation>&amp;תוספות</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="323"/>
        <source>Install extras for Rockbox</source>
        <translation>התקן תוספות עבור רוקבוקס</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="329"/>
        <source>Install Fonts package</source>
        <translation>התקן חבילת גופנים</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="346"/>
        <source>&lt;b&gt;Fonts Package&lt;/b&gt;&lt;br/&gt;The Fonts Package contains a couple of commonly used fonts. Installation is highly recommended.</source>
        <translation>&lt;b&gt;חבילת גופנים&lt;/b&gt;&lt;br/&gt;חבילת הגופנים מכילה מספר גופנים בהם נעשה שימוש לעתים קרובות. התקנה זו מומלצת ביותר.</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="356"/>
        <source>Install themes</source>
        <translation>התקנת ערכות נושא</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="383"/>
        <source>Install game files</source>
        <translation>התקנת קבצי משחק</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="400"/>
        <source>&lt;b&gt;Install Game Files&lt;/b&gt;&lt;br/&gt;Doom needs a base wad file to run.</source>
        <translation>&lt;b&gt;התקנת קבצי משחק&lt;/b&gt;&lt;br/&gt; דום זקוקה לקובץ wad בסיסי על מנת לרוץ.</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="437"/>
        <location filename="../rbutilqtfrm.ui" line="726"/>
        <source>&amp;Accessibility</source>
        <translation>&amp;נגישות</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="440"/>
        <source>Install accessibility add-ons</source>
        <translation>התקנת תוסף נגישות</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="446"/>
        <source>Install Voice files</source>
        <translation>התקנת קבצי הקראת תפריטים</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="463"/>
        <source>&lt;b&gt;Install Voice file&lt;/b&gt;&lt;br/&gt;Voice files are needed to make Rockbox speak the user interface. Speaking is enabled by default, so if you installed the voice file Rockbox will speak.</source>
        <translation>&lt;b&gt;התקנת קבצי הקראת תפריטים&lt;/b&gt;&lt;br/&gt;קבצי הקראת תפריטים נחוצים על מנת להקריא את ממשק המשתמש. הקראה מופעלת כברירת מחדל, כך שאם התקנת את קבצי הקראת התפריטים, רוקבוקס תקריא אותם.</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="473"/>
        <source>Install Talk files</source>
        <translation>התקנת קבצי דיבור</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="490"/>
        <source>&lt;b&gt;Create Talk Files&lt;/b&gt;&lt;br/&gt;Talkfiles are needed to let Rockbox speak File and Foldernames</source>
        <translation>&lt;b&gt;יצירת קבצי דיבור&lt;/b&gt;&lt;br/&gt;קבצי דיבור נחוצים על מנת לתת לרוקבוקס להקריא שמות קבצים וספריות</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="523"/>
        <source>Create Voice files</source>
        <translation>יצירת קבצי קול</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="540"/>
        <source>&lt;b&gt;Create Voice file&lt;/b&gt;&lt;br/&gt;Voice files are needed to make Rockbox speak the  user interface. Speaking is enabled by default, so
 if you installed the voice file Rockbox will speak.</source>
        <translation>&lt;b&gt;יצירת קבצי הקראת התפריטים&lt;/b&gt;&lt;br/&gt;קבצי קול נחוצים על מנת לגרום לרוקבוקס להקריא את ממשק המשתמש. הקראה מאופשרת כברירת מחדל, כך שאם התקנת את קבצי הקראת התפריטים, רוקבוקס תקריא אותם.</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="552"/>
        <location filename="../rbutilqtfrm.ui" line="734"/>
        <source>&amp;Uninstallation</source>
        <translation>ה&amp;סרת התקנה</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="555"/>
        <location filename="../rbutilqtfrm.ui" line="588"/>
        <source>Uninstall Rockbox</source>
        <translation>הסרת התקנת רוקבוקס</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="561"/>
        <source>Uninstall Bootloader</source>
        <translation>הסרת מנהל האיתחול</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="578"/>
        <source>&lt;b&gt;Remove the bootloader&lt;/b&gt;&lt;br/&gt;After removing the bootloader you won&apos;t be able to start Rockbox.</source>
        <translation>&lt;b&gt;הסרת מנהל האיתחול&lt;/b&gt;&lt;br/&gt;לאחר הסרת מנהל האיתחול לא תוכל להפעיל את רוקבוקס.</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="605"/>
        <source>&lt;b&gt;Uninstall Rockbox from your audio player.&lt;/b&gt;&lt;br/&gt;This will leave the bootloader in place (you need to remove it manually).</source>
        <translation>&lt;b&gt;הסרת רוקבוקס מהנגן שלך&lt;/b&gt;&lt;br/&gt;מנהל האתחול ישאר במקומו (יהיה עליך להסירו ידנית).</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="648"/>
        <source>&amp;Manual</source>
        <translation>מ&amp;דריך למשתמש</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="651"/>
        <source>View and download the manual</source>
        <translation>צפה והורד את המדריך למשתמש</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="656"/>
        <source>Inf&amp;o</source>
        <translation>&amp;מידע</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="674"/>
        <source>&amp;File</source>
        <translation>&amp;קובץ</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="687"/>
        <source>&amp;Troubleshoot</source>
        <translation>תפעול ת&amp;קלות</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="700"/>
        <source>Action&amp;s</source>
        <translation>&amp;פעולות</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="752"/>
        <source>Empty local download cache</source>
        <translation>ריקון מטמון הורדות מקומי</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="757"/>
        <source>Install Rockbox Utility on player</source>
        <translation>התקנת תוכנית השירות של רוקבוקס על הנגן</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="762"/>
        <source>&amp;Configure</source>
        <translation>&amp;הגדרות</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="767"/>
        <source>E&amp;xit</source>
        <translation>&amp;יציאה</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="770"/>
        <source>Ctrl+Q</source>
        <translation>Ctrl+Q</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="775"/>
        <source>&amp;About</source>
        <translation>&amp;אודות</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="780"/>
        <source>About &amp;Qt</source>
        <translation>Qt או&amp;דות</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="683"/>
        <location filename="../rbutilqtfrm.ui" line="785"/>
        <source>&amp;Help</source>
        <translation>&amp;עזרה</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="373"/>
        <source>&lt;b&gt;Install Themes&lt;/b&gt;&lt;br/&gt;Rockbox&apos;s look can be customized by themes. You can choose and install several officially distributed themes.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="790"/>
        <source>Info</source>
        <translation>מידע</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="799"/>
        <source>&amp;Complete Installation</source>
        <translation>התקנה &amp;מלאה</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="817"/>
        <source>Install &amp;Bootloader</source>
        <translation>התקנת מנהל ה&amp;איתחול</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="826"/>
        <source>Install &amp;Rockbox</source>
        <translation>התקנת &amp;רוקבוקס</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="835"/>
        <source>Install &amp;Fonts Package</source>
        <translation>התקנת חבילת &amp;גופנים</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="844"/>
        <source>Install &amp;Themes</source>
        <translation>התקנת &amp;ערכות נושא</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="853"/>
        <source>Install &amp;Game Files</source>
        <translation>התקנת קבצי &amp;משחק</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="862"/>
        <source>&amp;Install Voice File</source>
        <translation>התקנת קבצי הקראת &amp;תפריטים</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="871"/>
        <source>Create &amp;Talk Files</source>
        <translation>יצירת קבצי &amp;דיבור</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="880"/>
        <source>Remove &amp;bootloader</source>
        <translation>הסרת &amp;מנהל האיתחול</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="889"/>
        <source>Uninstall &amp;Rockbox</source>
        <translation>הסרת &amp;רוקבוקס</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="894"/>
        <source>Read PDF manual</source>
        <translation>קריאת מדריך למשתמש מסוג PDF</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="899"/>
        <source>Read HTML manual</source>
        <translation>קריאת מדריך למשתמש מסוג HTML</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="904"/>
        <source>Download PDF manual</source>
        <translation>הורדת מדריך למשתמש מסוג PDF</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="909"/>
        <source>Download HTML manual (zip)</source>
        <translation>הורדת מדריך למשתמש מסוג HTML (קובץ zip)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="918"/>
        <source>Create &amp;Voice File</source>
        <translation>יצירת קובץ &amp;הקראת תפריטים</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="921"/>
        <source>Create Voice File</source>
        <translation>יצירת קובץ הקראת תפריטים</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="926"/>
        <source>&amp;System Info</source>
        <translation>&amp;מידע מערכת</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="931"/>
        <source>System &amp;Trace</source>
        <translation>יומן רי&amp;צת מערכת</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="167"/>
        <source>Minimal Installation</source>
        <translation>התקנה מינימאלית</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="184"/>
        <source>&lt;b&gt;Minimal installation&lt;/b&gt;&lt;br/&gt;This installs bootloader and the current build of Rockbox. If you don&apos;t want the extras package, choose this option.</source>
        <translation>&lt;b&gt;התקנה מינימאלית&lt;/b&gt;&lt;br/&gt;התקנת מנהל האיתחול והגירסה הנוכחית של רוקבוקס. אם אינך מעוניין בחבילת התוספות, בחר באפשרות זו.</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="808"/>
        <source>&amp;Minimal Installation</source>
        <translation>התקנה מינימא&amp;לית</translation>
    </message>
</context>
<context>
    <name>ServerInfo</name>
    <message>
        <location filename="../base/serverinfo.cpp" line="71"/>
        <source>Unknown</source>
        <translation>לא ידוע</translation>
    </message>
    <message>
        <location filename="../base/serverinfo.cpp" line="75"/>
        <source>Unusable</source>
        <translation>לא שמיש</translation>
    </message>
    <message>
        <location filename="../base/serverinfo.cpp" line="78"/>
        <source>Unstable</source>
        <translation>לא יציב</translation>
    </message>
    <message>
        <location filename="../base/serverinfo.cpp" line="81"/>
        <source>Stable</source>
        <translation>יציב</translation>
    </message>
</context>
<context>
    <name>SysTrace</name>
    <message>
        <location filename="../systrace.cpp" line="75"/>
        <location filename="../systrace.cpp" line="84"/>
        <source>Save system trace log</source>
        <translation>שמור יומן ריצת מערכת</translation>
    </message>
</context>
<context>
    <name>SysTraceFrm</name>
    <message>
        <location filename="../systracefrm.ui" line="14"/>
        <source>System Trace</source>
        <translation>יומן ריצת מערכת</translation>
    </message>
    <message>
        <location filename="../systracefrm.ui" line="20"/>
        <source>System State trace</source>
        <translation>יומן ריצת מערכת</translation>
    </message>
    <message>
        <location filename="../systracefrm.ui" line="46"/>
        <source>&amp;Close</source>
        <translation>&amp;סגור</translation>
    </message>
    <message>
        <location filename="../systracefrm.ui" line="57"/>
        <source>&amp;Save</source>
        <translation>&amp;שמור</translation>
    </message>
    <message>
        <location filename="../systracefrm.ui" line="68"/>
        <source>&amp;Refresh</source>
        <translation>&amp;רענן</translation>
    </message>
    <message>
        <location filename="../systracefrm.ui" line="79"/>
        <source>Save &amp;previous</source>
        <translation>שמור &amp;קודם</translation>
    </message>
</context>
<context>
    <name>Sysinfo</name>
    <message>
        <location filename="../sysinfo.cpp" line="44"/>
        <source>&lt;b&gt;OS&lt;/b&gt;&lt;br/&gt;</source>
        <translation>&lt;b&gt;מערכת הפעלה&lt;/b&gt;&lt;br/&gt;</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="45"/>
        <source>&lt;b&gt;Username&lt;/b&gt;&lt;br/&gt;%1&lt;hr/&gt;</source>
        <translation>&lt;b&gt;שם משתמש&lt;/b&gt;&lt;br/&gt;%1&lt;hr/&gt;</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="47"/>
        <source>&lt;b&gt;Permissions&lt;/b&gt;&lt;br/&gt;%1&lt;hr/&gt;</source>
        <translation>&lt;b&gt;הרשאות&lt;/b&gt;&lt;br/&gt;%1&lt;hr/&gt;</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="49"/>
        <source>&lt;b&gt;Attached USB devices&lt;/b&gt;&lt;br/&gt;</source>
        <translation>&lt;b&gt;התקני USB מחוברים&lt;/b&gt;&lt;br/&gt;</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="53"/>
        <source>VID: %1 PID: %2, %3</source>
        <translation></translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="62"/>
        <source>Filesystem</source>
        <translation>מערכת קבצים</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="65"/>
        <source>Mountpoint</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="65"/>
        <source>Label</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="66"/>
        <source>Free</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="66"/>
        <source>Total</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="67"/>
        <source>Cluster Size</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="69"/>
        <source>&lt;tr&gt;&lt;td&gt;%1&lt;/td&gt;&lt;td&gt;%4&lt;/td&gt;&lt;td&gt;%2 GiB&lt;/td&gt;&lt;td&gt;%3 GiB&lt;/td&gt;&lt;td&gt;%5&lt;/td&gt;&lt;/tr&gt;</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>SysinfoFrm</name>
    <message>
        <location filename="../sysinfofrm.ui" line="13"/>
        <source>System Info</source>
        <translation>מידע מערכת</translation>
    </message>
    <message>
        <location filename="../sysinfofrm.ui" line="22"/>
        <source>&amp;Refresh</source>
        <translation>&amp;רענן</translation>
    </message>
    <message>
        <location filename="../sysinfofrm.ui" line="45"/>
        <source>&amp;OK</source>
        <translation>&amp;אישור</translation>
    </message>
</context>
<context>
    <name>System</name>
    <message>
        <location filename="../base/system.cpp" line="120"/>
        <source>Guest</source>
        <translation type="unfinished">אורח</translation>
    </message>
    <message>
        <location filename="../base/system.cpp" line="123"/>
        <source>Admin</source>
        <translation type="unfinished">מנהל</translation>
    </message>
    <message>
        <location filename="../base/system.cpp" line="126"/>
        <source>User</source>
        <translation type="unfinished">משתמש</translation>
    </message>
    <message>
        <location filename="../base/system.cpp" line="129"/>
        <source>Error</source>
        <translation type="unfinished">שגיאה</translation>
    </message>
    <message>
        <location filename="../base/system.cpp" line="277"/>
        <location filename="../base/system.cpp" line="322"/>
        <source>(no description available)</source>
        <translation type="unfinished">(אין תיאור זמין)</translation>
    </message>
</context>
<context>
    <name>TTSBase</name>
    <message>
        <location filename="../base/ttsbase.cpp" line="39"/>
        <source>Espeak TTS Engine</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/ttsbase.cpp" line="40"/>
        <source>Flite TTS Engine</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/ttsbase.cpp" line="41"/>
        <source>Swift TTS Engine</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/ttsbase.cpp" line="43"/>
        <source>SAPI TTS Engine</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/ttsbase.cpp" line="46"/>
        <source>Festival TTS Engine</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/ttsbase.cpp" line="49"/>
        <source>OS X System Engine</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>TTSCarbon</name>
    <message>
        <location filename="../base/ttscarbon.cpp" line="138"/>
        <source>Voice:</source>
        <translation>קול:</translation>
    </message>
    <message>
        <location filename="../base/ttscarbon.cpp" line="144"/>
        <source>Speed (words/min):</source>
        <translation>מהירות (מילים/דקה):</translation>
    </message>
    <message>
        <location filename="../base/ttscarbon.cpp" line="151"/>
        <source>Pitch (0 for default):</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/ttscarbon.cpp" line="221"/>
        <source>Could not voice string</source>
        <translation>לא מצליח להקריא את המחרוזת</translation>
    </message>
    <message>
        <location filename="../base/ttscarbon.cpp" line="231"/>
        <source>Could not convert intermediate file</source>
        <translation>לא מצליח להמיר קובץ ביניים</translation>
    </message>
</context>
<context>
    <name>TTSExes</name>
    <message>
        <location filename="../base/ttsexes.cpp" line="77"/>
        <source>TTS executable not found</source>
        <translation>קובץ הפעלה של הדיבור לא נמצא</translation>
    </message>
    <message>
        <location filename="../base/ttsexes.cpp" line="42"/>
        <source>Path to TTS engine:</source>
        <translation>נתיב למנוע הדיבור:</translation>
    </message>
    <message>
        <location filename="../base/ttsexes.cpp" line="44"/>
        <source>TTS engine options:</source>
        <translation>אפשרויות מנוע דיבור:</translation>
    </message>
</context>
<context>
    <name>TTSFestival</name>
    <message>
        <location filename="../base/ttsfestival.cpp" line="201"/>
        <source>engine could not voice string</source>
        <translation>המנוע אינו יכול להקריא את המחרוזת</translation>
    </message>
    <message>
        <location filename="../base/ttsfestival.cpp" line="284"/>
        <source>No description available</source>
        <translation>אין תיאור זמין</translation>
    </message>
    <message>
        <location filename="../base/ttsfestival.cpp" line="49"/>
        <source>Path to Festival client:</source>
        <translation>נתיב ללקוח Festival:</translation>
    </message>
    <message>
        <location filename="../base/ttsfestival.cpp" line="54"/>
        <source>Voice:</source>
        <translation>קול:</translation>
    </message>
    <message>
        <location filename="../base/ttsfestival.cpp" line="63"/>
        <source>Voice description:</source>
        <translation>תיאור קול:</translation>
    </message>
</context>
<context>
    <name>TTSSapi</name>
    <message>
        <location filename="../base/ttssapi.cpp" line="43"/>
        <source>Language:</source>
        <translation>שפה:</translation>
    </message>
    <message>
        <location filename="../base/ttssapi.cpp" line="49"/>
        <source>Voice:</source>
        <translation>קול:</translation>
    </message>
    <message>
        <location filename="../base/ttssapi.cpp" line="59"/>
        <source>Speed:</source>
        <translation>מהירות:</translation>
    </message>
    <message>
        <location filename="../base/ttssapi.cpp" line="62"/>
        <source>Options:</source>
        <translation>אפשרויות:</translation>
    </message>
    <message>
        <location filename="../base/ttssapi.cpp" line="106"/>
        <source>Could not copy the SAPI script</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/ttssapi.cpp" line="127"/>
        <source>Could not start SAPI process</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>TalkFileCreator</name>
    <message>
        <location filename="../base/talkfile.cpp" line="35"/>
        <source>Starting Talk file generation</source>
        <translation>מתחיל ביצירת קובץ הקראת התפריטים</translation>
    </message>
    <message>
        <location filename="../base/talkfile.cpp" line="40"/>
        <source>Reading Filelist...</source>
        <translation>קורא רשימת קבצים...</translation>
    </message>
    <message>
        <location filename="../base/talkfile.cpp" line="43"/>
        <source>Talk file creation aborted</source>
        <translation>יצירת קובץ הקראת התפריטים בוטלה</translation>
    </message>
    <message>
        <location filename="../base/talkfile.cpp" line="66"/>
        <source>Copying Talkfiles...</source>
        <translation>מעתיק קבצי הקראת התפריטים...</translation>
    </message>
    <message>
        <location filename="../base/talkfile.cpp" line="229"/>
        <source>File copy aborted</source>
        <translation>העתקת קובץ בוטלה</translation>
    </message>
    <message>
        <location filename="../base/talkfile.cpp" line="268"/>
        <source>Cleaning up...</source>
        <translation>מנקה...</translation>
    </message>
    <message>
        <location filename="../base/talkfile.cpp" line="279"/>
        <source>Finished</source>
        <translation>הסתיים</translation>
    </message>
    <message>
        <location filename="../base/talkfile.cpp" line="78"/>
        <source>Finished creating Talk files</source>
        <translation>יצירת קבצי הקראה הסתיימה</translation>
    </message>
    <message>
        <location filename="../base/talkfile.cpp" line="247"/>
        <source>Copying of %1 to %2 failed</source>
        <translation>העתקת %1 ל- %2 נכשלה</translation>
    </message>
</context>
<context>
    <name>TalkGenerator</name>
    <message>
        <location filename="../base/talkgenerator.cpp" line="38"/>
        <source>Starting TTS Engine</source>
        <translation>מתחיל מנוע דיבור</translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="43"/>
        <source>Init of TTS engine failed</source>
        <translation>איתחול מנוע הדיבור נכשל</translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="50"/>
        <source>Starting Encoder Engine</source>
        <translation>מתחיל מנוע קידוד</translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="54"/>
        <source>Init of Encoder engine failed</source>
        <translation>איתחול מנוע הקידוד נכשל</translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="64"/>
        <source>Voicing entries...</source>
        <translation>מקריא רשומות...</translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="79"/>
        <source>Encoding files...</source>
        <translation>מקודד קבצים...</translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="118"/>
        <source>Voicing aborted</source>
        <translation>הקראה בוטלה</translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="154"/>
        <location filename="../base/talkgenerator.cpp" line="159"/>
        <source>Voicing of %1 failed: %2</source>
        <translation>הקראת %1 בוטלה: %2</translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="203"/>
        <source>Encoding aborted</source>
        <translation>הקידוד בוטל</translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="230"/>
        <source>Encoding of %1 failed</source>
        <translation>הקידוד של %1 נכשל</translation>
    </message>
</context>
<context>
    <name>ThemeInstallFrm</name>
    <message>
        <location filename="../themesinstallfrm.ui" line="13"/>
        <source>Theme Installation</source>
        <translation>התקנת ערכת נושא</translation>
    </message>
    <message>
        <location filename="../themesinstallfrm.ui" line="48"/>
        <source>Selected Theme</source>
        <translation>ערכת הנושא שנבחרה</translation>
    </message>
    <message>
        <location filename="../themesinstallfrm.ui" line="73"/>
        <source>Description</source>
        <translation>תיאור</translation>
    </message>
    <message>
        <location filename="../themesinstallfrm.ui" line="83"/>
        <source>Download size:</source>
        <translation>גודל הורדה:</translation>
    </message>
    <message>
        <location filename="../themesinstallfrm.ui" line="115"/>
        <source>&amp;Install</source>
        <translation>&amp;התקנה</translation>
    </message>
    <message>
        <location filename="../themesinstallfrm.ui" line="125"/>
        <source>&amp;Cancel</source>
        <translation>&amp;ביטול</translation>
    </message>
    <message>
        <location filename="../themesinstallfrm.ui" line="93"/>
        <source>Hold Ctrl to select multiple item, Shift for a range</source>
        <translatorcomment>בשביל לבחור טווח Shift ,בשביל לבחור מספר פריטים Ctrl לחץ על</translatorcomment>
        <translation>לחץ על Ctrl בשביל לבחור מספר פריטים, או על Shift בשביל לבחור טווח</translation>
    </message>
</context>
<context>
    <name>ThemesInstallWindow</name>
    <message>
        <location filename="../themesinstallwindow.cpp" line="38"/>
        <source>no theme selected</source>
        <translation>לא נבחרה ערכת נושא</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="110"/>
        <source>Network error: %1.
Please check your network and proxy settings.</source>
        <translation>תקלת רשת: %1.
אנא בדוק את הגדרות הרשת והפרוקסי שלך.</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="123"/>
        <source>the following error occured:
%1</source>
        <translation>השגיאה הבאה התרחשה:
%1</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="129"/>
        <source>done.</source>
        <translation>הסתיים.</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="196"/>
        <source>fetching details for %1</source>
        <translation>טוען פרטים עבור %1</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="199"/>
        <source>fetching preview ...</source>
        <translation>טוען תצוגה מקדימה...</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="212"/>
        <source>&lt;b&gt;Author:&lt;/b&gt; %1&lt;hr/&gt;</source>
        <translatorcomment>Keep in English</translatorcomment>
        <translation>&lt;b&gt;Author:&lt;/b&gt; %1&lt;hr/&gt;</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="213"/>
        <location filename="../themesinstallwindow.cpp" line="215"/>
        <source>unknown</source>
        <translation>לא ידוע</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="214"/>
        <source>&lt;b&gt;Version:&lt;/b&gt; %1&lt;hr/&gt;</source>
        <translatorcomment>Keep in English</translatorcomment>
        <translation>&lt;b&gt;Version:&lt;/b&gt; %1&lt;hr/&gt;</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="216"/>
        <source>&lt;b&gt;Description:&lt;/b&gt; %1&lt;hr/&gt;</source>
        <translatorcomment>Keep in English</translatorcomment>
        <translation>&lt;b&gt;Description:&lt;/b&gt; %1&lt;hr/&gt;</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="217"/>
        <source>no description</source>
        <translation>אין תיאור</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="260"/>
        <source>no theme preview</source>
        <translation>אין תצוגה מקדימה של ערכת נושא</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="291"/>
        <source>getting themes information ...</source>
        <translation>מוריד מידע על ערכות הנושא...</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="339"/>
        <source>Mount point is wrong!</source>
        <translation>נקודת העגינה הינה שגויה!</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="39"/>
        <source>no selection</source>
        <translation>אין בחירה</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="166"/>
        <source>Information</source>
        <translation>מידע</translation>
    </message>
    <message numerus="yes">
        <location filename="../themesinstallwindow.cpp" line="183"/>
        <source>Download size %L1 kiB (%n item(s))</source>
        <translation>
            <numerusform>גודל הורדה kiB %L1 (פריט אחד)</numerusform>
            <numerusform>גודל הורדה kiB %L1 (%n פריטים)</numerusform>
        </translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="248"/>
        <source>Retrieving theme preview failed.
HTTP response code: %1</source>
        <translation>נכשלה תצורגה מקדימה של ערכת נושא.
HTTP response code: %1</translation>
    </message>
</context>
<context>
    <name>UninstallFrm</name>
    <message>
        <location filename="../uninstallfrm.ui" line="16"/>
        <source>Uninstall Rockbox</source>
        <translation>הסרת התקנת רוקבוקס</translation>
    </message>
    <message>
        <location filename="../uninstallfrm.ui" line="35"/>
        <source>Please select the Uninstallation Method</source>
        <translation>אנא בחר שיטת הסרת התקנה</translation>
    </message>
    <message>
        <location filename="../uninstallfrm.ui" line="45"/>
        <source>Uninstallation Method</source>
        <translation>שיטת הסרת התקנה</translation>
    </message>
    <message>
        <location filename="../uninstallfrm.ui" line="51"/>
        <source>Complete Uninstallation</source>
        <translation>הסרת התקנה מלאה</translation>
    </message>
    <message>
        <location filename="../uninstallfrm.ui" line="58"/>
        <source>Smart Uninstallation</source>
        <translation>הסרת התקנה חכמה</translation>
    </message>
    <message>
        <location filename="../uninstallfrm.ui" line="68"/>
        <source>Please select what you want to uninstall</source>
        <translation>אנא בחר מה ברצונך להסיר</translation>
    </message>
    <message>
        <location filename="../uninstallfrm.ui" line="78"/>
        <source>Installed Parts</source>
        <translation>רכיבים מותקנים</translation>
    </message>
    <message>
        <location filename="../uninstallfrm.ui" line="128"/>
        <source>&amp;Uninstall</source>
        <translation>&amp;הסרת התקנה</translation>
    </message>
    <message>
        <location filename="../uninstallfrm.ui" line="138"/>
        <source>&amp;Cancel</source>
        <translation>&amp;ביטול</translation>
    </message>
</context>
<context>
    <name>Uninstaller</name>
    <message>
        <location filename="../base/uninstall.cpp" line="31"/>
        <location filename="../base/uninstall.cpp" line="42"/>
        <source>Starting Uninstallation</source>
        <translation>מתחיל להסיר התקנה</translation>
    </message>
    <message>
        <location filename="../base/uninstall.cpp" line="35"/>
        <source>Finished Uninstallation</source>
        <translation>הסרת התקנה הושלמה</translation>
    </message>
    <message>
        <location filename="../base/uninstall.cpp" line="48"/>
        <source>Uninstalling %1...</source>
        <translation>מסיר %1...</translation>
    </message>
    <message>
        <location filename="../base/uninstall.cpp" line="79"/>
        <source>Could not delete %1</source>
        <translation>לא הצלחתי למחוק את %1</translation>
    </message>
    <message>
        <location filename="../base/uninstall.cpp" line="108"/>
        <source>Uninstallation finished</source>
        <translation>הסרת התקנה הסתיימה</translation>
    </message>
</context>
<context>
    <name>Utils</name>
    <message>
        <location filename="../base/utils.cpp" line="309"/>
        <source>&lt;li&gt;Permissions insufficient for bootloader installation.
Administrator priviledges are necessary.&lt;/li&gt;</source>
        <translation type="unfinished">&lt;li&gt;הרשאות אינן מספיקות להתקנת מנהל איתחול.
הרשאות מנהל הינן הכרחיות&lt;/li&gt;</translation>
    </message>
    <message>
        <location filename="../base/utils.cpp" line="321"/>
        <source>&lt;li&gt;Target mismatch detected.&lt;br/&gt;Installed target: %1&lt;br/&gt;Selected target: %2.&lt;/li&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/utils.cpp" line="328"/>
        <source>Problem detected:</source>
        <translation type="unfinished">זוהתה בעיה:</translation>
    </message>
</context>
<context>
    <name>VoiceFileCreator</name>
    <message>
        <location filename="../base/voicefile.cpp" line="40"/>
        <source>Starting Voicefile generation</source>
        <translation>מתחיל ביצירת קבצי הקראה</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="85"/>
        <source>Downloading voice info...</source>
        <translation>מוריד מידע קול...</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="98"/>
        <source>Download error: received HTTP error %1.</source>
        <translation>כשלון בהורדה: התקבלה שגיאת HTTP %1.</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="104"/>
        <source>Cached file used.</source>
        <translation>נעשה שימוש בקובץ הנמצא במטמון.</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="107"/>
        <source>Download error: %1</source>
        <translation>שגיאת הורדה: %1</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="112"/>
        <source>Download finished.</source>
        <translation>הורדה הסתיימה.</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="120"/>
        <source>failed to open downloaded file</source>
        <translation>פתיחת הקובץ שירד נכשלה</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="128"/>
        <source>Reading strings...</source>
        <translation>קורא מחרוזות...</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="200"/>
        <source>Creating voicefiles...</source>
        <translation>יוצר קבצי הקראה...</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="245"/>
        <source>Cleaning up...</source>
        <translation>מנקה...</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="256"/>
        <source>Finished</source>
        <translation>הסתיים</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="174"/>
        <source>The downloaded file was empty!</source>
        <translation>הקובץ שירד היה ריק!</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="205"/>
        <source>Error opening downloaded file</source>
        <translation>שגיאה בפתיחת הקובץ שירד</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="216"/>
        <source>Error opening output file</source>
        <translation>שגיאה בפתיחת קובץ הפלט</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="236"/>
        <source>successfully created.</source>
        <translation>נוצר בהצלחה.</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="53"/>
        <source>could not find rockbox-info.txt</source>
        <translation>לא מוצא rockbox-info.txt</translation>
    </message>
</context>
<context>
    <name>ZipInstaller</name>
    <message>
        <location filename="../base/zipinstaller.cpp" line="58"/>
        <source>done.</source>
        <translation>הסתיים.</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="66"/>
        <source>Installation finished successfully.</source>
        <translation>התקנה הסתיימה בהצלחה.</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="79"/>
        <source>Downloading file %1.%2</source>
        <translation>מוריד קובץ %1.%2</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="113"/>
        <source>Download error: received HTTP error %1.</source>
        <translation>שגיאת הורדה: התקבלה שגיאת %1 HTTP.</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="119"/>
        <source>Cached file used.</source>
        <translation>נעשה שימוש בקובץ מהמטמון.</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="121"/>
        <source>Download error: %1</source>
        <translation>שגיאת הורדה: %1</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="125"/>
        <source>Download finished.</source>
        <translation>הורדה הסתיימה.</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="131"/>
        <source>Extracting file.</source>
        <translation>פורס קובץ.</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="151"/>
        <source>Extraction failed!</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="160"/>
        <source>Installing file.</source>
        <translation>מתקין קובץ.</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="171"/>
        <source>Installing file failed.</source>
        <translation>התקנת קובץ נכשלה.</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="180"/>
        <source>Creating installation log</source>
        <translation>יוצר קובץ רישום של ההתקנה</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="144"/>
        <source>Not enough disk space! Aborting.</source>
        <translation>אין מספיק מקום בדיסק! מבטל.</translation>
    </message>
</context>
<context>
    <name>ZipUtil</name>
    <message>
        <location filename="../base/ziputil.cpp" line="118"/>
        <source>Creating output path failed</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/ziputil.cpp" line="125"/>
        <source>Creating output file failed</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/ziputil.cpp" line="134"/>
        <source>Error during Zip operation</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>aboutBox</name>
    <message>
        <location filename="../aboutbox.ui" line="14"/>
        <source>About Rockbox Utility</source>
        <translation>אודות תוכנת השרות של רוקבוקס</translation>
    </message>
    <message>
        <location filename="../aboutbox.ui" line="32"/>
        <source>The Rockbox Utility</source>
        <translation>תוכנת השרות של רוקבוקס</translation>
    </message>
    <message utf8="true">
        <location filename="../aboutbox.ui" line="54"/>
        <source>Installer and housekeeping utility for the Rockbox open source digital audio player firmware.&lt;br/&gt;© 2005 - 2012 The Rockbox Team.&lt;br/&gt;Released under the GNU General Public License v2.&lt;br/&gt;Uses icons by the &lt;a href=&quot;http://tango.freedesktop.org/&quot;&gt;Tango Project&lt;/a&gt;.&lt;br/&gt;&lt;center&gt;&lt;a href=&quot;http://www.rockbox.org&quot;&gt;http://www.rockbox.org&lt;/a&gt;&lt;/center&gt;</source>
        <translation></translation>
    </message>
    <message>
        <location filename="../aboutbox.ui" line="74"/>
        <source>&amp;Credits</source>
        <translation>&amp;תודות</translation>
    </message>
    <message>
        <location filename="../aboutbox.ui" line="106"/>
        <source>&amp;License</source>
        <translation>&amp;רישיון</translation>
    </message>
    <message>
        <location filename="../aboutbox.ui" line="132"/>
        <source>&amp;Speex License</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../aboutbox.ui" line="158"/>
        <source>&amp;Ok</source>
        <translation>&amp;אישור</translation>
    </message>
</context>
</TS>
