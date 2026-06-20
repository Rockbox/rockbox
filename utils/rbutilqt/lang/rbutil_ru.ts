<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="ru">
<context>
    <name>BackupDialog</name>
    <message>
        <location filename="../gui/backupdialogfrm.ui" line="17"/>
        <location filename="../gui/backupdialogfrm.ui" line="43"/>
        <source>Backup</source>
        <translation>Резервная копия</translation>
    </message>
    <message>
        <location filename="../gui/backupdialogfrm.ui" line="33"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;This dialog will create a backup by archiving the contents of the Rockbox installation on the player into a zip file. This will include installed themes and settings stored below the .rockbox folder on the player.&lt;/p&gt;&lt;p&gt;The backup filename will be created based on the installed version. &lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Здесь можно создать резервную копию текущей установки Rockbox и упаковать её в ZIP-файл. Это также касается установленных тем и настроек, находящихся в папке .rockbox на Вашем плеере.&lt;/p&gt;&lt;p&gt;Название файла резервной копии будет соответствовать установленной версии прошивки.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location filename="../gui/backupdialogfrm.ui" line="49"/>
        <source>Size: unknown</source>
        <translation>Размер: неизвестен</translation>
    </message>
    <message>
        <location filename="../gui/backupdialogfrm.ui" line="56"/>
        <source>Backup to: unknown</source>
        <translation>Создать в: неизвстно</translation>
    </message>
    <message>
        <location filename="../gui/backupdialogfrm.ui" line="76"/>
        <source>&amp;Change</source>
        <translation>&amp;Изменить</translation>
    </message>
    <message>
        <location filename="../gui/backupdialogfrm.ui" line="116"/>
        <source>&amp;Backup</source>
        <translation>&amp;Создать копию</translation>
    </message>
    <message>
        <location filename="../gui/backupdialogfrm.ui" line="127"/>
        <source>&amp;Cancel</source>
        <translation>&amp;Отмена</translation>
    </message>
    <message>
        <location filename="../gui/backupdialog.cpp" line="69"/>
        <source>Installation size: calculating ...</source>
        <translation>Размер установки: вычисляется...</translation>
    </message>
    <message>
        <location filename="../gui/backupdialog.cpp" line="96"/>
        <source>Select Backup Filename</source>
        <translation>Выберите название файла резервной копии</translation>
    </message>
    <message>
        <location filename="../gui/backupdialog.cpp" line="116"/>
        <source>Installation size: %L1 %2</source>
        <translation>Размер установки: %L1 %2</translation>
    </message>
    <message>
        <location filename="../gui/backupdialog.cpp" line="132"/>
        <source>Starting backup ...</source>
        <translation>Начало копии ...</translation>
    </message>
    <message>
        <location filename="../gui/backupdialog.cpp" line="151"/>
        <source>Backup successful.</source>
        <translation>Резервная копия удалась.</translation>
    </message>
    <message>
        <location filename="../gui/backupdialog.cpp" line="154"/>
        <source>Backup failed!</source>
        <translation>Не удалось создать резервную копию!</translation>
    </message>
    <message>
        <location filename="../gui/backupdialog.cpp" line="123"/>
        <source>File exists</source>
        <translation>Файл уже существует</translation>
    </message>
    <message>
        <location filename="../gui/backupdialog.cpp" line="124"/>
        <source>The selected backup file already exists. Overwrite?</source>
        <translation>Такой файл уже существует. Заменить?</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallAms</name>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="58"/>
        <source>Downloading bootloader file</source>
        <translation>Скачивается файл загрузчика</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="100"/>
        <location filename="../base/bootloaderinstallams.cpp" line="113"/>
        <source>Could not load %1</source>
        <translation>Не удалось загрузить %1</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="137"/>
        <source>Patching Firmware...</source>
        <translation>Изменяется прошивка...</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="148"/>
        <source>Could not open %1 for writing</source>
        <translation>Не удалось открыть %1 для записи</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="161"/>
        <source>Could not write firmware file</source>
        <translation>Сбой записи файла прошивки</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="177"/>
        <source>Success: modified firmware file created</source>
        <translation>Изменённая прошивка успешно создана</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="127"/>
        <source>No room to insert bootloader, try another firmware version</source>
        <translation>Нет места для записи загрузчика, попробуйте другую версию прошивки</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="185"/>
        <source>To uninstall, perform a normal upgrade with an unmodified original firmware</source>
        <translation>Для удаления выполните нормальное обновление с неизменённой фирменной прошивкой</translation>
    </message>
    <message>
        <source>Bootloader installation requires you to provide a copy of the original Sandisk firmware (bin file). This firmware file will be patched and then installed to your player along with the rockbox bootloader. You need to download this file yourself due to legal reasons. Please browse the &lt;a href=&apos;http://forums.sandisk.com/sansa/&apos;&gt;Sansa Forums&lt;/a&gt; or refer to the &lt;a href=&apos;https://www.rockbox.org/manual.shtml&apos;&gt;manual&lt;/a&gt; and the &lt;a href=&apos;https://www.rockbox.org/wiki/SansaAMS&apos;&gt;SansaAMS&lt;/a&gt; wiki page on how to obtain this file.&lt;br/&gt;Press Ok to continue and browse your computer for the firmware file.</source>
        <translation type="obsolete">Установка загрузчика потребует от вас копию оригинальной прошивки Sandisk\&apos;а (bin файл). Эта прошивка будет пропатчена и затем установлена в ваш плеер вместе с зарузчиком Rockbox\&apos;а. По причинам легальности данного действия вам нужно будет самим скачать загрузчик. Зайдите на &lt;a href=\&apos;http://forums.sandisk.com/sansa/\&apos;&gt;Sansa Forums\&apos;&lt;/a&gt; или обратитесь к &lt;a href=\&apos;https://www.rockbox.org/manual.shtml\&apos;&gt;инструкции&lt;/a&gt; и вики-странице &lt;a href=\&apos;https://www.rockbox.org/wiki/SansaAMS\&apos;&gt;SansaAMS&lt;/a&gt; за помощью с получением файла.&lt;br/&gt;Нажмите ОК чтобы продолжить и выбрать файл прошивки.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="33"/>
        <source>Bootloader installation requires you to provide a copy of the original Sandisk firmware (bin file). This firmware file will be patched and then installed to your player along with the rockbox bootloader. You need to download this file yourself due to legal reasons. Please browse the &lt;a href=&apos;http://forums.sandisk.com/sansa/&apos;&gt;Sansa Forums&lt;/a&gt; or refer to the &lt;a href=&apos;https://www.rockbox.org/manual.shtml&apos;&gt;manual&lt;/a&gt; and the &lt;a href=&apos;https://www.rockbox.org/wiki/SansaAMS&apos;&gt;SansaAMS&lt;/a&gt; wiki page on how to obtain this file.&lt;br/&gt;&lt;b&gt;Note:&lt;/b&gt; This file is not present on your player and will disappear automatically after installing it.&lt;br/&gt;&lt;br/&gt;Press Ok to continue and browse your computer for the firmware file.</source>
        <translation>Установка загрузчика потребует от вас копию оригинальной прошивки от Sandisk (bin-файл). Эта прошивка будет пропатчена, а затем установлена на ваш плеер вместе с зарузчиком Rockbox\&apos;а. По законным причинам, Вам необходимо скачать файл самостоятельно. Зайдите на &lt;a href=&apos;http://forums.sandisk.com/sansa/&apos;&gt;форумы Sansa&lt;/a&gt; или читайте &lt;a href=&apos;https://www.rockbox.org/manual.shtml&apos;&gt;инструкцию&lt;/a&gt; и вики-страничку &lt;a href=&apos;https://www.rockbox.org/wiki/SansaAMS&apos;&gt;SansaAMS&lt;/a&gt; о том, как получить файл.&lt;br/&gt;Нажмите ОК чтобы продолжить и выбрать файл прошивки.</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallBSPatch</name>
    <message>
        <location filename="../base/bootloaderinstallbspatch.cpp" line="65"/>
        <source>Bootloader installation requires you to provide the correct verrsion of the original firmware file. This file will be patched with the Rockbox bootloader and installed to your player. You need to download this file yourself due to legal reasons. Please refer to the &lt;a href=&apos;https://www.rockbox.org/wiki/&apos;&gt;rockbox wiki&lt;/a&gt; pages on how to obtain this file.&lt;br/&gt;Press Ok to continue and browse your computer for the firmware file.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbspatch.cpp" line="84"/>
        <source>Could not read original firmware file</source>
        <translation type="unfinished">Не удалось прочитать фирменную прошивку</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbspatch.cpp" line="90"/>
        <source>Downloading bootloader file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbspatch.cpp" line="99"/>
        <source>Patching file...</source>
        <translation type="unfinished">Изменяется прошивка...</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbspatch.cpp" line="124"/>
        <source>Patching the original firmware failed</source>
        <translation type="unfinished">Сбой патчирования фирменной прошивки</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbspatch.cpp" line="130"/>
        <source>Succesfully patched firmware file</source>
        <translation type="unfinished">Прошивка успешно пропатчена</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbspatch.cpp" line="145"/>
        <source>Bootloader successful installed</source>
        <translation type="unfinished">Загрузчик успешно установлен</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbspatch.cpp" line="151"/>
        <source>Patched bootloader could not be installed</source>
        <translation type="unfinished">Не удалось установить пропатченный загрузчик</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbspatch.cpp" line="161"/>
        <source>To uninstall, perform a normal upgrade with an unmodified original firmware.</source>
        <translation type="unfinished">Для удаления выполните нормальное обновление с неизменённой фирменной прошивкой.</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallBase</name>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="69"/>
        <source>Download error: received HTTP error %1.</source>
        <translation>Ошибка при скачивании: ошибка HTTP %1.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="75"/>
        <source>Download error: %1</source>
        <translation>Ошибка при скачивании: %1</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="81"/>
        <source>Download finished (cache used).</source>
        <translation>Скачивание завершено (из кэша).</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="83"/>
        <source>Download finished.</source>
        <translation>Скачивание завершено.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="112"/>
        <source>Creating backup of original firmware file.</source>
        <translation>Создаётся резервная копия фирменной прошивки.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="114"/>
        <source>Creating backup folder failed</source>
        <translation>Сбой при попытке создания папки резервной копии</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="120"/>
        <source>Creating backup copy failed.</source>
        <translation>Сбой при попытке создания резервной копии.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="123"/>
        <source>Backup created.</source>
        <translation>Резервная копия создана.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="140"/>
        <source>Creating installation log</source>
        <translation>Создаётся журнал установки</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="152"/>
        <source>Installation log created</source>
        <translation>Журнал установки создан</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="162"/>
        <source>Waiting for system to remount player</source>
        <translation>Ожидание, пока система заново смонтирует плеер</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="192"/>
        <source>Player remounted</source>
        <translation>Плеер смонтирован</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="197"/>
        <source>Timeout on remount</source>
        <translation>Время ожидания для монтирования истекло</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="245"/>
        <source>Zip file format detected</source>
        <translation>Обнаружен формат ZIP</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="278"/>
        <source>Extracting firmware %1 from archive</source>
        <translation>Извлекается прошивка %1 из архива</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="285"/>
        <source>Error extracting firmware from archive</source>
        <translation>Ошибка при извлечении прошивки</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="294"/>
        <source>Could not find firmware in archive</source>
        <translation>Прошивка в архиве не найдена</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="257"/>
        <source>CAB file format detected</source>
        <translation>Обнаружен формат CAB</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallChinaChip</name>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="33"/>
        <source>Bootloader installation requires you to provide a firmware file of the original firmware (HXF file). You need to download this file yourself due to legal reasons. Please refer to the &lt;a href=&apos;https://www.rockbox.org/manual.shtml&apos;&gt;manual&lt;/a&gt; and the &lt;a href=&apos;https://www.rockbox.org/wiki/OndaVX747#Download_and_extract_a_recent_ve&apos;&gt;OndaVX747&lt;/a&gt; wiki page on how to obtain this file.&lt;br/&gt;Press Ok to continue and browse your computer for the firmware file.</source>
        <translation>Чтобы установить загрузчик, требуется файл фирменной прошивки (HXF-файл). По законодательным причинам, этот файл вам необходимо скачать самостоятельно. О том, как получить этот файл, смотрите в &lt;a href=&apos;https://www.rockbox.org/manual.shtml&apos;&gt;руководстве по эксплуатации&lt;/a&gt; и на &lt;a href=&apos;https://www.rockbox.org/wiki/OndaVX747#Download_and_extract_a_recent_ve&apos;&gt;вики-страничке OndaVX747&lt;/a&gt;. &lt;br/&gt; Нажмине на ОК, чтобы продолжить и указать путь к прошивке на Вашем компьютере.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="50"/>
        <source>Downloading bootloader file</source>
        <translation>Скачивается файл загрузчика</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="75"/>
        <source>Could not open firmware file</source>
        <translation>Не удалось открыть файл прошивки</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="78"/>
        <source>Could not open bootloader file</source>
        <translation>Не удалось открыть файл загрузчика</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="81"/>
        <source>Could not allocate memory</source>
        <translation>Не удалось выделить память</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="84"/>
        <source>Could not load firmware file</source>
        <translation>Не удалось загрузить файл прошивки</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="87"/>
        <source>File is not a valid ChinaChip firmware</source>
        <translation>Файл не является годной прошивкой ChinaChip</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="90"/>
        <source>Could not find ccpmp.bin in input file</source>
        <translation>ccpmp.bin во входном файле не найден</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="93"/>
        <source>Could not open backup file for ccpmp.bin</source>
        <translation>Не удалось открыть резервную копию для ccpmp.bin</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="96"/>
        <source>Could not write backup file for ccpmp.bin</source>
        <translation>Не удалось записать резервную копию для ccpmp.bin</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="99"/>
        <source>Could not load bootloader file</source>
        <translation>Не удалось считать файл загрузчика</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="102"/>
        <source>Could not get current time</source>
        <translation>Не удалось получить текущее время</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="105"/>
        <source>Could not open output file</source>
        <translation>Не удалось открыть выходной файл</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="108"/>
        <source>Could not write output file</source>
        <translation>Не удалось записать выходной файл</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="111"/>
        <source>Unexpected error from chinachippatcher</source>
        <translation>Неожиданный сбой chinachippatcher</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallFile</name>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="34"/>
        <source>Downloading bootloader</source>
        <translation>Скачивается загрузчик</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="43"/>
        <source>Installing Rockbox bootloader</source>
        <translation>Устанавливается загрузчик Rockbox</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="75"/>
        <source>Error accessing output folder</source>
        <translation>Ошибка доступа к выходной папке</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="111"/>
        <source>Removing Rockbox bootloader</source>
        <translation>Удаляется загрузчик Rockbox</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="115"/>
        <source>No original firmware file found.</source>
        <translation>Не найдено фирменной прошивки.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="121"/>
        <source>Can&apos;t remove Rockbox bootloader file.</source>
        <translation>Не удалось удалить файл загрузчика Rockbox.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="126"/>
        <source>Can&apos;t restore bootloader file.</source>
        <translation>Не удалось восстановить файл загрузчика.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="130"/>
        <source>Original bootloader restored successfully.</source>
        <translation>Фирменный загрузчик успешно восстановлен.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="94"/>
        <source>Bootloader successful installed</source>
        <translation>Загрузчик успешно установлен</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="89"/>
        <source>A firmware file is already present on player</source>
        <translation>Файл прошивки уже присутствует на плеере</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="97"/>
        <source>Copying modified firmware file failed</source>
        <translation>Не удалось скопировать изменённый файл прошивки</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallHex</name>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="69"/>
        <source>checking MD5 hash of input file ...</source>
        <translation>проверка хэша MD5 входного файла ...</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="80"/>
        <source>Could not verify original firmware file</source>
        <translation>Не удалось проверить фирменную прошивку</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="95"/>
        <source>Firmware file not recognized.</source>
        <translation>Файл прошивки не распознан.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="99"/>
        <source>MD5 hash ok</source>
        <translation>Хэш MD5 проверен</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="106"/>
        <source>Firmware file doesn&apos;t match selected player.</source>
        <translation>Прошивка не соответствует указанному плееру.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="111"/>
        <source>Descrambling file</source>
        <translation>Расшифровка файла</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="119"/>
        <source>Error in descramble: %1</source>
        <translation>Ошибка при расшифровке: %1</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="124"/>
        <source>Downloading bootloader file</source>
        <translation>Скачивается файл загрузчика</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="134"/>
        <source>Adding bootloader to firmware file</source>
        <translation>Добавляется загрузчик к прошивке</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="172"/>
        <source>could not open input file</source>
        <translation>Не удалось открыть входной файл</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="173"/>
        <source>reading header failed</source>
        <translation>Сбой чтения заголовка</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="174"/>
        <source>reading firmware failed</source>
        <translation>Сбой чтения прошивки</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="175"/>
        <source>can&apos;t open bootloader file</source>
        <translation>Не удалось открыть файл загрузчика</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="176"/>
        <source>reading bootloader file failed</source>
        <translation>Сбой чтения файла загрузчика</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="177"/>
        <source>can&apos;t open output file</source>
        <translation>Не удалось открыть выходной файл</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="178"/>
        <source>writing output file failed</source>
        <translation>Сбой записи выходного файла</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="180"/>
        <source>Error in patching: %1</source>
        <translation>Ошибка применения патча: %1</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="191"/>
        <source>Error in scramble: %1</source>
        <translation>Ошибка кодирования: %1</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="206"/>
        <source>Checking modified firmware file</source>
        <translation>Проверка изменённой прошивки</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="208"/>
        <source>Error: modified file checksum wrong</source>
        <translation>Ошибка: неверная контрольная сумма изменённого файла</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="220"/>
        <source>Success: modified firmware file created</source>
        <translation>Изменённая прошивка успешно создана</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="259"/>
        <source>Can&apos;t open input file</source>
        <translation>Не удалось открыть входной файл</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="260"/>
        <source>Can&apos;t open output file</source>
        <translation>Не удалось открыть выходной вайл</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="261"/>
        <source>invalid file: header length wrong</source>
        <translation>Неверный файл: неверная длина заголовка</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="262"/>
        <source>invalid file: unrecognized header</source>
        <translation>Неверный файл: неопознанный заголовок</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="263"/>
        <source>invalid file: &quot;length&quot; field wrong</source>
        <translation>Неверный файл: неверное поле &quot;длина&quot;</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="264"/>
        <source>invalid file: &quot;length2&quot; field wrong</source>
        <translation>Неверный файл: неверное поле &quot;длина2&quot;</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="265"/>
        <source>invalid file: internal checksum error</source>
        <translation>Неверный файл: ошибка во внутренней контрольной сумме</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="267"/>
        <source>unknown</source>
        <translation>неизвестная ошибка</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="50"/>
        <source>Bootloader installation requires you to provide a firmware file of the original firmware (hex file). You need to download this file yourself due to legal reasons. Please refer to the &lt;a href=&apos;https://www.rockbox.org/manual.shtml&apos;&gt;manual&lt;/a&gt; and the &lt;a href=&apos;https://www.rockbox.org/wiki/IriverBoot#Download_and_extract_a_recent_ve&apos;&gt;IriverBoot&lt;/a&gt; wiki page on how to obtain this file.&lt;br/&gt;Press Ok to continue and browse your computer for the firmware file.</source>
        <translation>Для установки загрузчика требуется файл прошивки с фирменной прошивкой (hex-файл). По законодательным причинам, вам необходимо скачать этот файл самостоятельно. Как найти этот файл, смотрите в &lt;a href=&apos;https://www.rockbox.org/manual.shtml&apos;&gt;руководстве по экслуатации&lt;/a&gt; и на вики-странице &lt;a href=&apos;https://www.rockbox.org/wiki/IriverBoot#Download_and_extract_a_recent_ve&apos;&gt;IriverBoot&lt;/a&gt;.&lt;br/&gt;Чтобы продолжить, нажмите на OK и укажите файл прошивки на компьютере.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="266"/>
        <source>invalid file: &quot;length3&quot; field wrong</source>
        <translation>Неверный файл: неверное поле &quot;длина3&quot;</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="237"/>
        <source>Uninstallation not possible, only installation info removed</source>
        <translation>Полное удаление невозможно, удалена только информация об установке</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="215"/>
        <source>A firmware file is already present on player</source>
        <translation>Файл прошивки уже присутствует на плеере</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="223"/>
        <source>Copying modified firmware file failed</source>
        <translation>Не удалось скопировать изменённый файл прошивки</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallImx</name>
    <message>
        <source>Bootloader installation requires you to provide a copy of the original Sandisk firmware (firmware.sb file). This file will be patched with the Rockbox bootloader and installed to your player. You need to download this file yourself due to legal reasons. Please browse the &lt;a href=&apos;http://forums.sandisk.com/sansa/&apos;&gt;Sansa Forums&lt;/a&gt; or refer to the &lt;a href= &apos;https://www.rockbox.org/wiki/SansaFuzePlus&apos;&gt;SansaFuzePlus&lt;/a&gt; wiki page on how to obtain this file.&lt;br/&gt;Press Ok to continue and browse your computer for the firmware file.</source>
        <translation type="vanished">Установка загрузчика потребует от вас копию оригинальной прошивки от Sandisk (файл firmware.sb). Эта прошивка будет пропатчена, а затем установлена на ваш плеер вместе с зарузчиком Rockbox. По законным причинам, Вам необходимо скачать файл самостоятельно. Зайдите на &lt;a href=&apos;http://forums.sandisk.com/sansa/&apos;&gt;форумы Sansa&lt;/a&gt; или читайте вики-страничку &lt;a href=&apos;https://www.rockbox.org/wiki/SansaFuzePlus&apos;&gt;SansaFuzePlus&lt;/a&gt; о том, как получить файл.&lt;br/&gt;Нажмите ОК чтобы продолжить и выбрать файл прошивки.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallimx.cpp" line="72"/>
        <source>Bootloader installation requires you to provide a copy of the original Sandisk firmware (firmware.sb file). This file will be patched with the Rockbox bootloader and installed to your player. You need to download this file yourself due to legal reasons. Please browse the &lt;a href=&apos;http://forums.sandisk.com/sansa/&apos;&gt;Sansa Forums&lt;/a&gt; or refer to the &lt;a href=&apos;https://www.rockbox.org/manual.shtml&apos;&gt;manual&lt;/a&gt; and &lt;a href= &apos;https://www.rockbox.org/wiki/SansaFuzePlus&apos;&gt;SansaFuzePlus&lt;/a&gt; wiki page on how to obtain this file.&lt;br/&gt;Press Ok to continue and browse your computer for the firmware file.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallimx.cpp" line="95"/>
        <source>Could not read original firmware file</source>
        <translation>Не удалось прочитать фирменную прошивку</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallimx.cpp" line="101"/>
        <source>Downloading bootloader file</source>
        <translation>Скачивается файл загрузчика</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallimx.cpp" line="111"/>
        <source>Patching file...</source>
        <translation>Изменяется прошивка...</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallimx.cpp" line="137"/>
        <source>Patching the original firmware failed</source>
        <translation>Сбой патчирования фирменной прошивки</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallimx.cpp" line="143"/>
        <source>Succesfully patched firmware file</source>
        <translation>Прошивка успешно пропатчена</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallimx.cpp" line="158"/>
        <source>Bootloader successful installed</source>
        <translation>Загрузчик успешно установлен</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallimx.cpp" line="164"/>
        <source>Patched bootloader could not be installed</source>
        <translation>Не удалось установить пропатченный загрузчик</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallimx.cpp" line="175"/>
        <source>To uninstall, perform a normal upgrade with an unmodified original firmware.</source>
        <translation>Для удаления выполните нормальное обновление с неизменённой фирменной прошивкой.</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallIpod</name>
    <message>
        <source>Error: can&apos;t allocate buffer memory!</source>
        <translation type="vanished">Ошибка: не удалось выделить буферную память!</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="72"/>
        <source>Downloading bootloader file</source>
        <translation>Скачивается файл заргузчика</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="56"/>
        <location filename="../base/bootloaderinstallipod.cpp" line="143"/>
        <source>Failed to read firmware directory</source>
        <translation>Сбой чтения папки микропрограммы</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="61"/>
        <location filename="../base/bootloaderinstallipod.cpp" line="148"/>
        <source>Unknown version number in firmware (%1)</source>
        <translation>Неизвестный номер версии прошивки (%1)</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="86"/>
        <location filename="../base/bootloaderinstallipod.cpp" line="155"/>
        <source>Could not open Ipod in R/W mode</source>
        <translation>Не удалось открыть iPod в режиме записи</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="107"/>
        <source>Failed to add bootloader</source>
        <translation>Сбой установки загрузчика</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="161"/>
        <source>No bootloader detected.</source>
        <translation>Загрузчика не обнаружено.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="167"/>
        <source>Successfully removed bootloader</source>
        <translation>Загрузчик успешно удалён</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="175"/>
        <source>Removing bootloader failed.</source>
        <translation>Сбой удаления загрузчика.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="266"/>
        <source>Could not open Ipod</source>
        <translation>Не удалось открыть iPod</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="277"/>
        <source>No firmware partition on disk</source>
        <translation>Не найдено раздела прошивки на диске</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="82"/>
        <source>Installing Rockbox bootloader</source>
        <translation>Установка загрузчика Rockbox</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="134"/>
        <source>Uninstalling bootloader</source>
        <translation>Удаление загрузчика</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="271"/>
        <source>Error reading partition table - possibly not an Ipod</source>
        <translation>Сбой чтения таблицы разделов - возможно, это не iPod</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="67"/>
        <source>Warning: This is a MacPod, Rockbox only runs on WinPods. 
See https://www.rockbox.org/wiki/IpodConversionToFAT32</source>
        <translation>Предупреждение: это - MacPod, Rockbox работает только на WinPod&apos;ах.
См. https://www.rockbox.org/wiki/IpodConversionToFAT32</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="96"/>
        <source>Successfull added bootloader</source>
        <translation>Загрузчик успешно добавлен</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="119"/>
        <source>Bootloader Installation complete.</source>
        <translation>Установка загрузчика завершена.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="124"/>
        <source>Writing log aborted</source>
        <translation>Запись журнала отменена</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="257"/>
        <source>Error: no mountpoint specified!</source>
        <translation>Ошибка: не указано точки монтирования!</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="262"/>
        <source>Could not open Ipod: permission denied</source>
        <translation>Не удалось открыть iPod: доступ запрещён</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="241"/>
        <source>Error: could not retrieve device name</source>
        <translation>Ошибка: не удалось найти название устройства</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallMi4</name>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="34"/>
        <source>Downloading bootloader</source>
        <translation>Скачивается загрузчик</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="43"/>
        <source>Installing Rockbox bootloader</source>
        <translation>Устанавливается загрузчик Rockbox</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="71"/>
        <location filename="../base/bootloaderinstallmi4.cpp" line="79"/>
        <source>Bootloader successful installed</source>
        <translation>Загрузчик успешно установлен</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="91"/>
        <source>Checking for Rockbox bootloader</source>
        <translation>Проверяется наличие загрузчика Rockbox</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="93"/>
        <source>No Rockbox bootloader found</source>
        <translation>Загрузчик Rockbox не найден</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="99"/>
        <source>Checking for original firmware file</source>
        <translation>Проверяется наличие фирменной прошивки</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="104"/>
        <source>Error finding original firmware file</source>
        <translation>Фирменной прошивки не найдено</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="115"/>
        <source>Rockbox bootloader successful removed</source>
        <translation>Загрузчик Rockbox успешно удалён</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="66"/>
        <source>A firmware file is already present on player</source>
        <translation>Файл прошивки уже присутствует на плеере</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="74"/>
        <source>Copying modified firmware file failed</source>
        <translation>Не удалось скопировать изменённый файл прошивки</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallMpio</name>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="33"/>
        <source>Bootloader installation requires you to provide a firmware file of the original firmware (bin file). You need to download this file yourself due to legal reasons. Please refer to the &lt;a href=&apos;https://www.rockbox.org/manual.shtml&apos;&gt;manual&lt;/a&gt; and the &lt;a href=&apos;https://www.rockbox.org/wiki/MPIOHD200Port&apos;&gt;MPIOHD200Port&lt;/a&gt; wiki page on how to obtain this file.&lt;br/&gt;Press Ok to continue and browse your computer for the firmware file.</source>
        <translation>Для установки загрузчика требуется файл прошивки с фирменной микропрограммой (hex-файл). По законодательным причинам, вам необходимо скачать этот файл самостоятельно. Как найти этот файл, смотрите в &lt;a href=&apos;https://www.rockbox.org/manual.shtml&apos;&gt;руководстве по экслуатации&lt;/a&gt; и на вики-странице &lt;a href=&apos;https://www.rockbox.org/wiki/MPIOHD200Port&apos;&gt;MPIOHD200Port&lt;/a&gt;.&lt;br/&gt;Чтобы продолжить, нажмите на OK и укажите файл прошивки на компьютере.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="52"/>
        <source>Downloading bootloader file</source>
        <translation>Скачивается файл загрузчика</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="79"/>
        <source>Could not open the original firmware.</source>
        <translation>Не удалось открыть фирменную прошивку.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="82"/>
        <source>Could not read the original firmware.</source>
        <translation>Не удалось прочитать фирменную прошивку.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="88"/>
        <source>Could not open downloaded bootloader.</source>
        <translation>Не удалось открыть скачаный загрузчик.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="91"/>
        <source>Place for bootloader in OF file not empty.</source>
        <translation>Место для загрузчика в фирменной прошивке не пустое.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="94"/>
        <source>Could not read the downloaded bootloader.</source>
        <translation>Не удалось прочитать полученный загрузчик.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="97"/>
        <source>Bootloader checksum error.</source>
        <translation>Ошибка в контрольной сумме загрузчика.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="111"/>
        <source>Patching original firmware failed: %1</source>
        <translation>Сбой патчирования фирменной прошивки: %1</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="118"/>
        <source>Success: modified firmware file created</source>
        <translation>Изменённая прошивка успешно создана</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="126"/>
        <source>To uninstall, perform a normal upgrade with an unmodified original firmware</source>
        <translation>Для удаления выполните нормальное обновление с неизменённой фирменной прошивкой</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="85"/>
        <source>Loaded firmware file does not look like MPIO original firmware file.</source>
        <translation>Загруженная прошивка не похожа на фирменную прошивку MPIO.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="100"/>
        <source>Could not open output file.</source>
        <translation>Не удалось открыть выходной вайл.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="103"/>
        <source>Could not write output file.</source>
        <translation>Сбой записи выходного файла.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="106"/>
        <source>Unknown error number: %1</source>
        <translation>Неизвестная ошибка номер %1</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallS5l</name>
    <message>
        <location filename="../base/bootloaderinstalls5l.cpp" line="61"/>
        <source>Could not find mounted iPod.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalls5l.cpp" line="68"/>
        <source>Downloading bootloader file...</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalls5l.cpp" line="113"/>
        <source>Could not make DFU image.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalls5l.cpp" line="119"/>
        <source>Ejecting iPod...</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalls5l.cpp" line="141"/>
        <source>Action required:

Please make sure no programs are accessing files on the device. If ejecting still fails please use your computers eject functionality.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalls5l.cpp" line="151"/>
        <source>Device successfully ejected.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalls5l.cpp" line="171"/>
        <source>Action required:

Quit iTunes application.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalls5l.cpp" line="179"/>
        <source>iTunes closed.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalls5l.cpp" line="192"/>
        <source>Could not suspend iTunesHelper. Stop it using the Task Manager, and try again.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalls5l.cpp" line="201"/>
        <source>Waiting for HDD spin-down...</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalls5l.cpp" line="217"/>
        <source>Waiting for DFU mode...</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalls5l.cpp" line="218"/>
        <source>Action required:

Press and hold SELECT+MENU buttons, after about 12 seconds a new action will require you to release the buttons, DO IT QUICKLY, otherwise the process could fail.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalls5l.cpp" line="241"/>
        <source>DFU mode detected.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalls5l.cpp" line="243"/>
        <source>Action required:

Release SELECT+MENU buttons and wait...</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalls5l.cpp" line="268"/>
        <source>Device is not in DFU mode. It seems that the previous required action failed, please try again.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalls5l.cpp" line="275"/>
        <source>Transfering DFU image...</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalls5l.cpp" line="285"/>
        <source>No valid DFU USB driver found.

Install iTunes (or the Apple Device Driver) and try again.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalls5l.cpp" line="294"/>
        <source>Could not transfer DFU image.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalls5l.cpp" line="299"/>
        <source>DFU transfer completed.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalls5l.cpp" line="302"/>
        <source>Restarting iPod, waiting for remount...</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalls5l.cpp" line="321"/>
        <source>Action required:

Could not remount the device, try to do it manually. If the iPod didn&apos;t restart, force a reset by pressing SELECT+MENU buttons for about 5 seconds. If the problem could not be solved then click &apos;Abort&apos; to cancel.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalls5l.cpp" line="333"/>
        <source>Device remounted.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalls5l.cpp" line="336"/>
        <source>Bootloader successfully installed.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalls5l.cpp" line="338"/>
        <source>Bootloader successfully uninstalled.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalls5l.cpp" line="350"/>
        <source>Could not resume iTunesHelper.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalls5l.cpp" line="368"/>
        <source>Install aborted by user.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalls5l.cpp" line="370"/>
        <source>Uninstall aborted by user.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>BootloaderInstallSansa</name>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="54"/>
        <source>Error: can&apos;t allocate buffer memory!</source>
        <translation>Ошибка: не удалось выделить буферную память!</translation>
    </message>
    <message>
        <source>Searching for Sansa</source>
        <translation type="vanished">Поиск плеера Sansa</translation>
    </message>
    <message>
        <source>Permission for disc access denied!
This is required to install the bootloader</source>
        <translation type="vanished">Доступ к диску отказан!
Это необходимо для установки загрузчика</translation>
    </message>
    <message>
        <source>No Sansa detected!</source>
        <translation type="vanished">Не найдено плеера Sansa!</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="68"/>
        <source>Downloading bootloader file</source>
        <translation>Скачивается файл загрузчика</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="60"/>
        <location filename="../base/bootloaderinstallsansa.cpp" line="164"/>
        <source>OLD ROCKBOX INSTALLATION DETECTED, ABORTING.
You must reinstall the original Sansa firmware before running
sansapatcher for the first time.
See https://www.rockbox.org/wiki/SansaE200Install
</source>
        <translation>ОБНАРУЖЕНА СТАРАЯ УСТАНОВКА ROCKBOX, ОПЕРАЦИЯ ОТМЕНЯЕТСЯ.
Вам необходимо переустановить фирменную микропрограмму 
вашего плеера перед первым запуском sansapatcher&apos;а.
См. https://www.rockbox.org/wiki/SansaE200Install</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="87"/>
        <location filename="../base/bootloaderinstallsansa.cpp" line="174"/>
        <source>Could not open Sansa in R/W mode</source>
        <translation>Не удалось открыть плеер в режиме записи</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="114"/>
        <source>Successfully installed bootloader</source>
        <translation>Загрузчик успешно установлен</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="125"/>
        <source>Failed to install bootloader</source>
        <translation>Сбой при установке загрузчика</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="260"/>
        <source>Can&apos;t find Sansa</source>
        <translation>Не удалось найти плеер Sansa</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="265"/>
        <source>Could not open Sansa</source>
        <translation>Не удалось открыть плеер Sansa</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="270"/>
        <source>Could not read partition table</source>
        <translation>Не удалось прочитать таблицу разделов</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="277"/>
        <source>Disk is not a Sansa (Error %1), aborting.</source>
        <translation>Диск не принадлежит плееру Sansa (Ошибка %1), отмена.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="180"/>
        <source>Successfully removed bootloader</source>
        <translation>Загручик успешно удалён</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="188"/>
        <source>Removing bootloader failed.</source>
        <translation>Сбой удаления загрузчика.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="83"/>
        <source>Installing Rockbox bootloader</source>
        <translation>Установка загрузчика Rockbox</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="96"/>
        <source>Checking downloaded bootloader</source>
        <translation>Проверка скаченного загрузчика</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="104"/>
        <source>Bootloader mismatch! Aborting.</source>
        <translation>Загрузчик не соответствует! Отмена.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="155"/>
        <source>Uninstalling bootloader</source>
        <translation>Удаление загрузчика</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="138"/>
        <source>Bootloader Installation complete.</source>
        <translation>Установка загрузчика завершена.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="143"/>
        <source>Writing log aborted</source>
        <translation>Запись журнала отменена</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="244"/>
        <source>Error: could not retrieve device name</source>
        <translation>Ошибка: не удалось найти имя устройства</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallTcc</name>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="50"/>
        <source>Downloading bootloader file</source>
        <translation>Скачивается файл загрузчика</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="82"/>
        <location filename="../base/bootloaderinstalltcc.cpp" line="99"/>
        <source>Could not load %1</source>
        <translation>Не удалось загрузить %1</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="90"/>
        <source>Unknown OF file used: %1</source>
        <translation>Используется неизвестный файл фирменной прошивки: %1</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="104"/>
        <source>Patching Firmware...</source>
        <translation>Изменяется прошивка...</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="117"/>
        <source>Could not open %1 for writing</source>
        <translation>Не удалось открыть %1 для записи</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="126"/>
        <source>Could not write firmware file</source>
        <translation>Сбой записи файла прошиви</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="131"/>
        <source>Success: modified firmware file created</source>
        <translation>Изменённый файл прошивки успешно создан</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="33"/>
        <source>Bootloader installation requires you to provide a firmware file of the original firmware (bin file). You need to download this file yourself due to legal reasons. Please refer to the &lt;a href=&apos;https://www.rockbox.org/manual.shtml&apos;&gt;manual&lt;/a&gt; and the &lt;a href=&apos;https://www.rockbox.org/wiki/CowonD2Info&apos;&gt;CowonD2Info&lt;/a&gt; wiki page on how to obtain the file.&lt;br/&gt;Press Ok to continue and browse your computer for the firmware file.</source>
        <translation>Для установки загрузчика требуется файл прошивки с фирменной микропрограммой (hex-файл). По законодательным причинам, вам необходимо скачать этот файл самостоятельно. Как найти этот файл, смотрите в &lt;a href=&apos;https://www.rockbox.org/manual.shtml&apos;&gt;руководстве по экслуатации&lt;/a&gt; и на вики-странице &lt;a href=&apos;https://www.rockbox.org/wiki/CowonD2Info&apos;&gt;CowonD2Info&lt;/a&gt;.&lt;br/&gt;Чтобы продолжить, нажмите на OK и укажите файл прошивки на компьютере.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="111"/>
        <source>Could not patch firmware</source>
        <translation>Сбой записи файла прошивки</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="151"/>
        <source>To uninstall, perform a normal upgrade with an unmodified original firmware</source>
        <translation>Для удаления выполните нормальное обновление с неизменённой фирменной прошивкой</translation>
    </message>
</context>
<context>
    <name>Changelog</name>
    <message>
        <location filename="../gui/changelogfrm.ui" line="17"/>
        <source>Changelog</source>
        <translation>История изменений</translation>
    </message>
    <message>
        <location filename="../gui/changelogfrm.ui" line="39"/>
        <source>Show on startup</source>
        <translation>Показать при запуске</translation>
    </message>
    <message>
        <location filename="../gui/changelogfrm.ui" line="46"/>
        <source>&amp;Ok</source>
        <translation>&amp;OK</translation>
    </message>
</context>
<context>
    <name>Config</name>
    <message>
        <location filename="../configure.cpp" line="858"/>
        <source>Autodetection</source>
        <translation>Автоопределение</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="859"/>
        <source>Could not detect a Mountpoint.
Select your Mountpoint manually.</source>
        <translation>Не удалось определить точку монтирования
Укажите её вручную.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="761"/>
        <source>Could not detect a device.
Select your device and Mountpoint manually.</source>
        <translation>Не удалось определить устройство.
Укажите устройство и точку монтирования вручную.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="869"/>
        <source>Really delete cache?</source>
        <translation>Удалить кэш?</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="870"/>
        <source>Do you really want to delete the cache? Make absolutely sure this setting is correct as it will remove &lt;b&gt;all&lt;/b&gt; files in this folder!</source>
        <translation>Вы дейсвительно хотите удалить кэш? Эта операция удалит &lt;b&gt;все&lt;/b&gt; файлы из этой папки!</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="878"/>
        <source>Path wrong!</source>
        <translation>Неверный путь!</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="879"/>
        <source>The cache path is invalid. Aborting.</source>
        <translation>Неверный путь кэша. Отмена.</translation>
    </message>
    <message>
        <source>Fatal error</source>
        <translation type="obsolete">Фатальная ошибка</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="315"/>
        <source>Current cache size is %L1 kiB.</source>
        <translation>Текущий размер кэша %L1 КБ.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="450"/>
        <location filename="../configure.cpp" line="484"/>
        <source>Configuration OK</source>
        <translation>Настройки верны</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="460"/>
        <location filename="../configure.cpp" line="489"/>
        <source>Configuration INVALID</source>
        <translation>Настройки НЕВЕРНЫ</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="125"/>
        <source>The following errors occurred:</source>
        <translation>Обнаружены следующие ошибки:</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="170"/>
        <source>No mountpoint given</source>
        <translation>Не указана точка монтирования</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="174"/>
        <source>Mountpoint does not exist</source>
        <translation>Точка монтирования не существует</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="178"/>
        <source>Mountpoint is not a directory.</source>
        <translation>Указанная точка монтирования не является папкой.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="182"/>
        <source>Mountpoint is not writeable</source>
        <translation>Точка монтирования незаписываема</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="196"/>
        <source>No player selected</source>
        <translation>Плеер не выбран</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="209"/>
        <source>Cache path not writeable. Leave path empty to default to systems temporary path.</source>
        <translation>Указанный путь кэша незаписываем. Оставьте это поле пустым для использования системного пути по умолчанию.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="228"/>
        <source>You need to fix the above errors before you can continue.</source>
        <translation>Вам необходимо исправить вышеуказанные ошибкм перед тем, как продолжить.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="231"/>
        <source>Configuration error</source>
        <translation>Ошибка в настройках</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="771"/>
        <source>Detected an unsupported player:
%1
Sorry, Rockbox doesn&apos;t run on your player.</source>
        <translation>Обнаружен неподдерживаемый плеер:
%1
К сожалению, Rockbox не работает на этом плеере.</translation>
    </message>
    <message>
        <source>Fatal: player incompatible</source>
        <translation type="obsolete">Ошибка: плеер несовместим</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="916"/>
        <source>TTS configuration invalid</source>
        <translation>Настройки TTS неверны</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="917"/>
        <source>TTS configuration invalid. 
 Please configure TTS engine.</source>
        <translation>Настройки TTS неверны.
 Пожалуйста, настройте движок TTS.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="922"/>
        <source>Could not start TTS engine.</source>
        <translation>Не удалось запустить движок TTS.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="923"/>
        <source>Could not start TTS engine.
</source>
        <translation>Не удалось запустить движок TTS.
</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="924"/>
        <location filename="../configure.cpp" line="943"/>
        <source>
Please configure TTS engine.</source>
        <translation>
Пожалуйста, настройте движок TTS.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="938"/>
        <source>Rockbox Utility Voice Test</source>
        <translation>Проверка голоса</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="941"/>
        <source>Could not voice test string.</source>
        <translation>Невозможно озвучить введённый текст.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="333"/>
        <source>Showing disabled targets</source>
        <translation>Отображение отображение неподдерживаемых устройств</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="334"/>
        <source>You just enabled showing targets that are marked disabled. Disabled targets are not recommended to end users. Please use this option only if you know what you are doing.</source>
        <translation>Вы только что указали, что хотите видеть в списке устройства, помеченные как официально неподдерживаемые. Таковые не рекомендуются к использованию конечным пользователям. Используйте эту функцию только если знаете, что делаете.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="639"/>
        <source>Set Cache Path</source>
        <translation>Указать путь к кэшу</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="787"/>
        <source>%1 &quot;MacPod&quot; found!
Rockbox needs a FAT formatted Ipod (so-called &quot;WinPod&quot;) to run. </source>
        <translation>%1 является MacPod&apos;ом!
Для работы, Rockbox нужен iPod форматированный в FAT (так называемый &quot;WinPod&quot;).</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="528"/>
        <source>Proxy Detection</source>
        <translation>Определение прокси</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="529"/>
        <source>The System Proxy settings are invalid!
Rockbox Utility can&apos;t work with this proxy settings. Make sure the system proxy is set correctly. Note that &quot;proxy auto-config (PAC)&quot; scripts are not supported by Rockbox Utility. If your system uses this you need to use manual proxy settings.</source>
        <translation>Системные настройки прокси неверны!
Мастер Rockbox не может работать с этими настройками. Проверьте правильность системных настроек прокси. Учтите, что мастер Rockbox не поддерживает сценарии &quot;proxy auto config&quot; (PAC). Если таковые используются на Вашей системе, вам необходимо использовать ручные настройки.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="778"/>
        <source>%1 in MTP mode found!
You need to change your player to MSC mode for installation. </source>
        <translation>Найден %1 в режиме MTP!
Для установки вам нужно сменить режим подключения плеера на MSC.</translation>
    </message>
    <message>
        <source>Until you change this installation will fail!</source>
        <translation type="obsolete">Пока вы это не измените, установка не пройдёт успешно!</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="942"/>
        <source>Could not voice test string.
</source>
        <translation>Не удалось произнести проверочное предложение.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="443"/>
        <location filename="../configure.cpp" line="909"/>
        <source>TTS error</source>
        <translation>Ошибка TTS</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="444"/>
        <location filename="../configure.cpp" line="910"/>
        <source>The selected TTS failed to initialize. You can&apos;t use this TTS.</source>
        <translation>Не удалось инициализироавть выбранный движок. Вы не можете им пользоваться.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="735"/>
        <source>Multiple devices have been detected. Please disconnect all players but one and try again.</source>
        <translation>Было обнаружено несколько устройств. Отключите все, кроме одного и попробуйте снова.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="738"/>
        <source>Detected devices:</source>
        <translation>Обнаруженные устройства:</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="743"/>
        <source>(unknown)</source>
        <translation>(неизвестный)</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="752"/>
        <source>Note: detecting connected devices might be ambiguous. You might have less devices connected than listed. In this case it might not be possible to detect your player unambiguously.</source>
        <translation>Примечание: обнаружение подключенных устройств может оказаться двусмысленно. Может быть подключено меньше устройств, чем в списке. В таком случае, Ваш плеер может быть невозможно узнать однозначно.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="756"/>
        <location filename="../configure.cpp" line="760"/>
        <location filename="../configure.cpp" line="805"/>
        <source>Device Detection</source>
        <translation>Обнаружение устройств</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="794"/>
        <source>The player contains an incompatible filesystem.
Make sure you selected the correct mountpoint and the player is set up to use a filesystem compatible with Rockbox.</source>
        <translation>Обнаружена несовместимая файловая система на плеере.
Убедитесь в том, что выбрана верная точка монтирования и плеер настроен на использование совместимой с Rockbox файловой системы.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="802"/>
        <source>An unknown error occured during player detection.</source>
        <translation>Неизвестная ошибка при обнаружении плеера.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="661"/>
        <source>%1 (%2 GiB of %3 GiB free)</source>
        <translation>%1 (свободны %2 ГиБ из %3 ГиБ)</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="745"/>
        <source>%1 at %2</source>
        <translation>%1 на %2</translation>
    </message>
</context>
<context>
    <name>ConfigForm</name>
    <message>
        <location filename="../configurefrm.ui" line="14"/>
        <source>Configuration</source>
        <translation>Настройки</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="20"/>
        <source>Configure Rockbox Utility</source>
        <translation>Настроить мастера Rockbox</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="35"/>
        <source>&amp;Device</source>
        <translation>&amp;Устройство</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="41"/>
        <source>Select your device in the &amp;filesystem</source>
        <translation>Укажите Ваше устройство в &amp;файловой системе</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="326"/>
        <source>&amp;Browse</source>
        <translation>&amp;Обзор</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="72"/>
        <source>&amp;Select your audio player</source>
        <translation>&amp;Выберите ваш аудио плеер</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="114"/>
        <source>&amp;Autodetect</source>
        <translation>&amp;Автоопределение</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="136"/>
        <source>&amp;Proxy</source>
        <translation>П&amp;рокси</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="142"/>
        <source>&amp;No Proxy</source>
        <translation>&amp;Без прокси</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="152"/>
        <source>Use S&amp;ystem values</source>
        <translation>Использовать с&amp;истемные настройки</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="159"/>
        <source>&amp;Manual Proxy settings</source>
        <translation>&amp;Ручные настройки прокси</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="166"/>
        <source>Proxy Values</source>
        <translation>Параметры прокси</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="172"/>
        <source>&amp;Host:</source>
        <translation>&amp;Хост:</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="182"/>
        <source>&amp;Port:</source>
        <translation>&amp;Порт:</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="199"/>
        <source>&amp;Username</source>
        <translation>&amp;Имя пользователя</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="209"/>
        <source>Pass&amp;word</source>
        <translation>Пар&amp;оль</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="267"/>
        <source>&amp;Language</source>
        <translation>&amp;Язык</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="281"/>
        <source>Cac&amp;he</source>
        <translation>К&amp;эш</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="284"/>
        <source>Download cache settings</source>
        <translation>Настройки кэша загрузок</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="290"/>
        <source>Rockbox Utility uses a local download cache to save network traffic. You can change the path to the cache and use it as local repository by enabling Offline mode.</source>
        <translation>Мастер Rockbox использует локальный кэш скачивания для экономии сетевой передачи. Вы можете изменить путь к кэшу и использовать его как локальное хранилище с помощью автономного режима.</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="300"/>
        <source>Current cache size is %1</source>
        <translation>Текущий размер кэша: %1</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="309"/>
        <source>P&amp;ath</source>
        <translation>&amp;Путь</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="319"/>
        <source>Entering an invalid folder will reset the path to the systems temporary path.</source>
        <translation>Введение неверного пути сбросит путь в значение системной временной папки.</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="341"/>
        <source>Disable local &amp;download cache</source>
        <translation>Отключить локальный кэш &amp;загрузок</translation>
    </message>
    <message>
        <source>&lt;p&gt;This will try to use all information from the cache, even information about updates. Only use this option if you want to install without network connection. Note: you need to do the same install you want to perform later with network access first to download all required files to the cache.&lt;/p&gt;</source>
        <translation type="obsolete">&lt;p&gt;Эта функция попытается использовать всю информацию из кэша, даже информацию об обновлениях. Используйте эту возможность только если вы хотите устанавливать без подключения к сети. Примечание: Вам необходимо выполнить такую-же установку, которую вы хотите выполнить потом, пока подключение к сети действует, чтобы загрузить все нужные файлы в кэш.&lt;/p&gt;</translation>
    </message>
    <message>
        <source>O&amp;ffline mode</source>
        <translation type="obsolete">Ав&amp;тономный режим</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="376"/>
        <source>Clean cache &amp;now</source>
        <translation>Вычистить кэш &amp;сейчас</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="392"/>
        <source>&amp;TTS &amp;&amp; Encoder</source>
        <translation>&amp;TTS &amp;&amp; Кодировщик</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="398"/>
        <source>TTS Engine</source>
        <translation>Движок TTS</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="404"/>
        <source>&amp;Select TTS Engine</source>
        <translation>&amp;Выберите движок TTS</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="473"/>
        <source>Encoder Engine</source>
        <translation>движок кодирования</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="547"/>
        <source>&amp;Ok</source>
        <translation>&amp;OK</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="558"/>
        <source>&amp;Cancel</source>
        <translation>&amp;Отмена</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="417"/>
        <source>Configure TTS Engine</source>
        <translation>Настроить движок TTS</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="424"/>
        <location filename="../configurefrm.ui" line="479"/>
        <source>Configuration invalid!</source>
        <translation>Настройка неверна!</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="441"/>
        <source>Configure &amp;TTS</source>
        <translation>Настроить &amp;TTS</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="496"/>
        <source>Configure &amp;Enc</source>
        <translation>Настроить &amp;кодировщик</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="507"/>
        <source>encoder name</source>
        <translation>имя кодировщика</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="452"/>
        <source>Test TTS</source>
        <translation>Проверить TTS</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="95"/>
        <source>Show disabled targets</source>
        <translation>Показывать отключенные устройства</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="58"/>
        <source>&amp;Refresh</source>
        <translation>&amp;Обновить</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="463"/>
        <source>&amp;Use string corrections for TTS</source>
        <translation>&amp;Использовать корекции строк для TTS</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="219"/>
        <source>Show</source>
        <translation>Показать</translation>
    </message>
</context>
<context>
    <name>Configure</name>
    <message>
        <location filename="../configure.cpp" line="586"/>
        <source>English</source>
        <comment>This is the localized language name, i.e. your language.</comment>
        <translation>Русский</translation>
    </message>
</context>
<context>
    <name>CreateVoiceFrm</name>
    <message>
        <location filename="../createvoicefrm.ui" line="17"/>
        <source>Create Voice File</source>
        <translation>Создать голосовой файл</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="42"/>
        <source>Select the Language you want to generate a voicefile for:</source>
        <translation>Выберите язык, для которого Вы хотите создать голосовой файл:</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="49"/>
        <source>Generation settings</source>
        <translation>Настройки генерирования</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="72"/>
        <source>Change</source>
        <translation>Изменить</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="143"/>
        <source>&amp;Install</source>
        <translation>&amp;Установить</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="154"/>
        <source>&amp;Cancel</source>
        <translation>&amp;Отмена</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="92"/>
        <source>Wavtrim Threshold</source>
        <translation>Порог Wavtrim</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="167"/>
        <source>Language</source>
        <translation>Язык</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="55"/>
        <source>TTS:</source>
        <translation>TTS:</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="105"/>
        <source>Silence threshold</source>
        <translation>Порог тишины</translation>
    </message>
</context>
<context>
    <name>CreateVoiceWindow</name>
    <message>
        <location filename="../createvoicewindow.cpp" line="120"/>
        <location filename="../createvoicewindow.cpp" line="123"/>
        <source>Engine: &lt;b&gt;%1&lt;/b&gt;</source>
        <translation>Движок: &lt;b&gt;%1&lt;/b&gt;</translation>
    </message>
    <message>
        <location filename="../createvoicewindow.cpp" line="115"/>
        <source>TTS error</source>
        <translation>Ошибка TTS</translation>
    </message>
    <message>
        <location filename="../createvoicewindow.cpp" line="116"/>
        <source>The selected TTS failed to initialize. You can&apos;t use this TTS.</source>
        <translation>Не удалось инициализироавть выбранный движок. Вы не можете им пользоваться.</translation>
    </message>
</context>
<context>
    <name>EncTtsCfgGui</name>
    <message>
        <location filename="../encttscfggui.cpp" line="43"/>
        <source>Waiting for engine...</source>
        <translation>Ожидание движка...</translation>
    </message>
    <message>
        <location filename="../encttscfggui.cpp" line="90"/>
        <source>Ok</source>
        <translation>OK</translation>
    </message>
    <message>
        <location filename="../encttscfggui.cpp" line="93"/>
        <source>Cancel</source>
        <translation>Отмена</translation>
    </message>
    <message>
        <location filename="../encttscfggui.cpp" line="256"/>
        <source>Browse</source>
        <translation>Обзор</translation>
    </message>
    <message>
        <location filename="../encttscfggui.cpp" line="271"/>
        <source>Refresh</source>
        <translation>Обновить</translation>
    </message>
    <message>
        <location filename="../encttscfggui.cpp" line="262"/>
        <source>Select executable</source>
        <translation>Выбрать исполняемый файл</translation>
    </message>
</context>
<context>
    <name>EncoderExe</name>
    <message>
        <location filename="../base/encoderexe.cpp" line="37"/>
        <source>Path to Encoder:</source>
        <translation>Путь к кодировщику:</translation>
    </message>
    <message>
        <location filename="../base/encoderexe.cpp" line="39"/>
        <source>Encoder options:</source>
        <translation>Настройки кодировщика:</translation>
    </message>
</context>
<context>
    <name>EncoderLame</name>
    <message>
        <location filename="../base/encoderlame.cpp" line="75"/>
        <location filename="../base/encoderlame.cpp" line="85"/>
        <source>LAME</source>
        <translation>LAME</translation>
    </message>
    <message>
        <location filename="../base/encoderlame.cpp" line="77"/>
        <source>Volume</source>
        <translation>Громкость</translation>
    </message>
    <message>
        <location filename="../base/encoderlame.cpp" line="81"/>
        <source>Quality</source>
        <translation>Качество</translation>
    </message>
    <message>
        <location filename="../base/encoderlame.cpp" line="85"/>
        <source>Could not find libmp3lame!</source>
        <translation>Не удалось найти libmp3lame!</translation>
    </message>
</context>
<context>
    <name>EncoderRbSpeex</name>
    <message>
        <location filename="../base/encoderrbspeex.cpp" line="34"/>
        <source>Volume:</source>
        <translation>Громкость:</translation>
    </message>
    <message>
        <location filename="../base/encoderrbspeex.cpp" line="36"/>
        <source>Quality:</source>
        <translation>Качество:</translation>
    </message>
    <message>
        <location filename="../base/encoderrbspeex.cpp" line="38"/>
        <source>Complexity:</source>
        <translation>Сложность:</translation>
    </message>
    <message>
        <location filename="../base/encoderrbspeex.cpp" line="40"/>
        <source>Use Narrowband:</source>
        <translation>Узкополосный:</translation>
    </message>
</context>
<context>
    <name>InfoWidget</name>
    <message>
        <location filename="../gui/infowidget.cpp" line="30"/>
        <location filename="../gui/infowidget.cpp" line="108"/>
        <source>File</source>
        <translation>Файл</translation>
    </message>
    <message>
        <location filename="../gui/infowidget.cpp" line="30"/>
        <location filename="../gui/infowidget.cpp" line="108"/>
        <source>Version</source>
        <translation>Версия</translation>
    </message>
    <message>
        <location filename="../gui/infowidget.cpp" line="56"/>
        <source>Loading, please wait ...</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>InfoWidgetFrm</name>
    <message>
        <location filename="../gui/infowidgetfrm.ui" line="20"/>
        <source>Currently installed packages.&lt;br/&gt;&lt;b&gt;Note:&lt;/b&gt; if you manually installed packages this might not be correct!</source>
        <translation>Установленные пакеты.&lt;br/&gt;&lt;b&gt;Примечание:&lt;/b&gt;Если вы установили некоторые пакеты вручную, могут возникнуть несоответствия!</translation>
    </message>
    <message>
        <location filename="../gui/infowidgetfrm.ui" line="14"/>
        <source>Info</source>
        <translation>Сведения</translation>
    </message>
    <message>
        <location filename="../gui/infowidgetfrm.ui" line="34"/>
        <source>Package</source>
        <translation>Пакет</translation>
    </message>
</context>
<context>
    <name>InstallTalkFrm</name>
    <message>
        <location filename="../installtalkfrm.ui" line="17"/>
        <source>Install Talk Files</source>
        <translation>Установить голосовые файлы</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="78"/>
        <source>TTS profile:</source>
        <translation>Профиль TTS :</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="36"/>
        <source>Generation options</source>
        <translation>Свойства сгенерированого</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="42"/>
        <source>Strip Extensions</source>
        <translation>Отрезать расширения</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="158"/>
        <source>&amp;Cancel</source>
        <translation>&amp;Отмена</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="147"/>
        <source>&amp;Install</source>
        <translation>&amp;Установить</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="115"/>
        <source>Change</source>
        <translation>Изменить</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="52"/>
        <source>Generate for files</source>
        <translation>Генерировать для файлов</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="85"/>
        <source>Generate for folders</source>
        <translation>Генерировать для папок</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="95"/>
        <source>Recurse into folders</source>
        <translation>Рекурсировать в папки</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="122"/>
        <source>Ignore files</source>
        <translation>Игнорировать файлы</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="132"/>
        <source>Skip existing</source>
        <translation>Пропускать существующие</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="174"/>
        <source>Select folders for Talkfile generation (Ctrl for multiselect)</source>
        <translation>Выбрать папки для создания файлов произношения (Ctrl для выбора нескольких)</translation>
    </message>
</context>
<context>
    <name>InstallTalkWindow</name>
    <message>
        <location filename="../installtalkwindow.cpp" line="95"/>
        <source>Empty selection</source>
        <translation>Пусто</translation>
    </message>
    <message>
        <location filename="../installtalkwindow.cpp" line="96"/>
        <source>No files or folders selected. Please select files or folders first.</source>
        <translation>Не выбрано ни одного файла или папки. Сначала выберите файлы и (или) папки.</translation>
    </message>
    <message>
        <location filename="../installtalkwindow.cpp" line="140"/>
        <source>TTS error</source>
        <translation>Ошибка TTS</translation>
    </message>
    <message>
        <location filename="../installtalkwindow.cpp" line="141"/>
        <source>The selected TTS failed to initialize. You can&apos;t use this TTS.</source>
        <translation>Не удалось инициализироавть выбранный движок. Вы не можете им пользоваться.</translation>
    </message>
</context>
<context>
    <name>ManualWidget</name>
    <message>
        <source>&lt;a href=&apos;%1&apos;&gt;PDF Manual&lt;/a&gt;</source>
        <translation type="vanished">&lt;a href=&apos;%1&apos;&gt;Руководство по эксплуатации в PDF&lt;/a&gt;</translation>
    </message>
    <message>
        <source>&lt;a href=&apos;%1&apos;&gt;HTML Manual (opens in browser)&lt;/a&gt;</source>
        <translation type="vanished">&lt;a href=&apos;%1&apos;&gt;Руководство по эксплуатации в HTML (открывается в обозревателе)&lt;/a&gt;</translation>
    </message>
    <message>
        <source>Select a device for a link to the correct manual</source>
        <translation type="vanished">Выберите устройство, чтобы получить ссылку на соответствующее руководство по эксплуатации</translation>
    </message>
    <message>
        <source>&lt;a href=&apos;%1&apos;&gt;Manual Overview&lt;/a&gt;</source>
        <translation type="vanished">&lt;a href=&apos;%1&apos;&gt;Обзор руководства по эксплуатации&lt;/a&gt;</translation>
    </message>
    <message>
        <source>Confirm download</source>
        <translation type="vanished">Потвердите скачивание</translation>
    </message>
    <message>
        <source>Do you really want to download the manual? The manual will be saved to the root folder of your player.</source>
        <translation type="vanished">Вы действительно хотите скачать руководство по эксплуатации? Оно будет записано в коренную папку Вашего плеера.</translation>
    </message>
</context>
<context>
    <name>ManualWidgetFrm</name>
    <message>
        <source>Read the manual</source>
        <translation type="vanished">Читать руководство по эксплуатации</translation>
    </message>
    <message>
        <source>PDF manual</source>
        <translation type="vanished">Руководство по эксплуатации в PDF</translation>
    </message>
    <message>
        <source>HTML manual</source>
        <translation type="vanished">Руководство по эксплуатации в HTML</translation>
    </message>
    <message>
        <source>Download the manual</source>
        <translation type="vanished">Скачать руководство по эксплуатации</translation>
    </message>
    <message>
        <source>&amp;PDF version</source>
        <translation type="vanished">Версия &amp;PDF</translation>
    </message>
    <message>
        <source>&amp;HTML version (zip file)</source>
        <translation type="vanished">Версия &amp;HTML (.zip-файл)</translation>
    </message>
    <message>
        <source>Down&amp;load</source>
        <translation type="vanished">С&amp;качать</translation>
    </message>
    <message>
        <source>Manual</source>
        <translation type="vanished">Руководство по эксплуатации</translation>
    </message>
</context>
<context>
    <name>MsPackUtil</name>
    <message>
        <location filename="../base/mspackutil.cpp" line="101"/>
        <source>Creating output path failed</source>
        <translation>Ошибка создания выходной папки</translation>
    </message>
    <message>
        <location filename="../base/mspackutil.cpp" line="109"/>
        <source>Error during CAB operation</source>
        <translation>Ошибка при выполнении операции с CAB-пакетом</translation>
    </message>
</context>
<context>
    <name>PlayerBuildInfo</name>
    <message>
        <location filename="../base/playerbuildinfo.cpp" line="358"/>
        <source>Stable (Retired)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/playerbuildinfo.cpp" line="361"/>
        <source>Unusable</source>
        <translation type="unfinished">Непригодный</translation>
    </message>
    <message>
        <location filename="../base/playerbuildinfo.cpp" line="364"/>
        <source>Unstable</source>
        <translation type="unfinished">Нестабильный</translation>
    </message>
    <message>
        <location filename="../base/playerbuildinfo.cpp" line="367"/>
        <source>Stable</source>
        <translation type="unfinished">Стабильный</translation>
    </message>
    <message>
        <location filename="../base/playerbuildinfo.cpp" line="370"/>
        <source>Unknown</source>
        <translation type="unfinished">Неизвестный</translation>
    </message>
</context>
<context>
    <name>PreviewFrm</name>
    <message>
        <location filename="../previewfrm.ui" line="16"/>
        <source>Preview</source>
        <translation>Предпросмотр</translation>
    </message>
</context>
<context>
    <name>ProgressLoggerFrm</name>
    <message>
        <location filename="../progressloggerfrm.ui" line="18"/>
        <location filename="../progressloggerfrm.ui" line="24"/>
        <source>Progress</source>
        <translation>Продвижение</translation>
    </message>
    <message>
        <location filename="../progressloggerfrm.ui" line="87"/>
        <source>&amp;Abort</source>
        <translation>&amp;Отмена</translation>
    </message>
    <message>
        <location filename="../progressloggerfrm.ui" line="37"/>
        <source>progresswindow</source>
        <translation>Окно продвижения</translation>
    </message>
    <message>
        <location filename="../progressloggerfrm.ui" line="63"/>
        <source>Save Log</source>
        <translation>Сохранить журнал</translation>
    </message>
</context>
<context>
    <name>ProgressLoggerGui</name>
    <message>
        <location filename="../progressloggergui.cpp" line="117"/>
        <source>&amp;Ok</source>
        <translation>&amp;OK</translation>
    </message>
    <message>
        <location filename="../progressloggergui.cpp" line="99"/>
        <source>&amp;Abort</source>
        <translation>&amp;Отмена</translation>
    </message>
    <message>
        <location filename="../progressloggergui.cpp" line="141"/>
        <source>Save system trace log</source>
        <translation>Сохранить журнал трассировки системы</translation>
    </message>
</context>
<context>
    <name>QObject</name>
    <message>
        <location filename="../configure.cpp" line="621"/>
        <location filename="../main.cpp" line="102"/>
        <source>LTR</source>
        <extracomment>This string is used to indicate the writing direction. Translate it to &quot;RTL&quot; (without quotes) for RTL languages. Anything else will get treated as LTR language.</extracomment>
        <translation>LTR</translation>
    </message>
    <message>
        <location filename="../base/system.cpp" line="333"/>
        <source>(unknown vendor name) </source>
        <translation>(неизвестный поставщик)</translation>
    </message>
    <message>
        <location filename="../base/system.cpp" line="351"/>
        <source>(unknown product name)</source>
        <translation>(неизвестный продукт)</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhelper.cpp" line="106"/>
        <source>Before Bootloader installation begins, Please check the following:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhelper.cpp" line="112"/>
        <source>&lt;li&gt;Ensure your SD card is formatted as FAT. exFAT is &lt;i&gt;not&lt;/i&gt; supported. You can reformat using the Original Firmware on your player if need be. It is located under (System Settings --&gt; Reset --&gt; Format TF Card).&lt;/li&gt;&lt;li&gt;Please use a quality SD card from a reputable source. The SD cards that come bundled with players are often of substandard quality and may cause issues.&lt;/li&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhelper.cpp" line="135"/>
        <source>Bootloader installation is almost complete. Installation &lt;b&gt;requires&lt;/b&gt; you to perform the following steps manually:</source>
        <translation>Установка загрузчика почти завершена. Вам &lt;b&gt;протребуется&lt;/b&gt; выполнить следующие операции вручную:</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhelper.cpp" line="141"/>
        <source>&lt;li&gt;Safely remove your player.&lt;/li&gt;</source>
        <translation>&lt;li&gt;Отключить плеер от компьютера с использованием безопасного извлечения.&lt;/li&gt;</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhelper.cpp" line="148"/>
        <source>&lt;li&gt;Reboot your player into the original firmware.&lt;/li&gt;&lt;li&gt;Perform a firmware upgrade using the update functionality of the original firmware. Please refer to your player&apos;s manual on details.&lt;br/&gt;&lt;b&gt;Important:&lt;/b&gt; updating the firmware is a critical process that must not be interrupted. &lt;b&gt;Make sure the player is charged before starting the firmware update process.&lt;/b&gt;&lt;/li&gt;&lt;li&gt;After the firmware has been updated reboot your player.&lt;/li&gt;</source>
        <translation>&lt;li&gt;Перезагрузите плеер на фирменную программу.&lt;/li&gt;&lt;li&gt;Обновите программу с помощью функции обновления в фирменной программе. Для подробностей, см. руководство по эксплуатации Вашего плеера.&lt;br/&gt;&lt;b&gt;Важно:&lt;/b&gt; обновление програмного обеспечения является критичной процедурой и не должно быть прервано. &lt;b&gt;Убедитесь в том, что плеер полностью заряжен перед тем, как приступить к обновлению.&lt;/b&gt;&lt;/li&gt;&lt;li&gt;После обновления, перезагрузите плеер.&lt;/li&gt;</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhelper.cpp" line="159"/>
        <source>&lt;li&gt;Remove any previously inserted microSD card&lt;/li&gt;</source>
        <translation>&lt;li&gt;Если в плеере стоит SD-карта, извлеките её&lt;/li&gt;</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhelper.cpp" line="160"/>
        <source>&lt;li&gt;Disconnect your player. The player will reboot and perform an update of the original firmware. Please refer to your players manual on details.&lt;br/&gt;&lt;b&gt;Important:&lt;/b&gt; updating the firmware is a critical process that must not be interrupted. &lt;b&gt;Make sure the player is charged before disconnecting the player.&lt;/b&gt;&lt;/li&gt;&lt;li&gt;After the firmware has been updated reboot your player.&lt;/li&gt;</source>
        <translation>&lt;li&gt;Отсоедините плеер, после чего произойдёт перезагрузка и обновится фирменная программа. Для подробностей, см. руковолство по эксплуатации Вашего плеера.&lt;br/&gt;&lt;b&gt;Важно:&lt;/b&gt; обновление програмного обеспечения является критичной процедурой и не должно быть прервано. &lt;b&gt;Убедитесь в том, что плеер полностью заряжен перед тем, как приступить к обновлению.&lt;/b&gt;&lt;/li&gt;&lt;li&gt;После обновления, перезагрузите плеер.&lt;/li&gt;</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhelper.cpp" line="171"/>
        <source>&lt;li&gt;Turn the player off&lt;/li&gt;&lt;li&gt;Insert the charger&lt;/li&gt;</source>
        <translation>&lt;li&gt;Выключить плеер&lt;/li&gt;&lt;li&gt;Подключить зарядное устройство&lt;/li&gt;</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhelper.cpp" line="176"/>
        <source>&lt;li&gt;Unplug USB and power adaptors&lt;/li&gt;&lt;li&gt;Hold &lt;i&gt;Power&lt;/i&gt; to turn the player off&lt;/li&gt;&lt;li&gt;Toggle the battery switch on the player&lt;/li&gt;&lt;li&gt;Hold &lt;i&gt;Power&lt;/i&gt; to boot into Rockbox&lt;/li&gt;</source>
        <translation>&lt;li&gt;Отключить плеер от USB и сетевого питания&lt;/li&gt;&lt;li&gt;Выключить плеер&lt;/li&gt;&lt;li&gt;Переключить плеер в режим питания от батареи&lt;/li&gt;&lt;li&gt;Обратно включить плеер для загрузки Rockbox&lt;/li&gt;</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhelper.cpp" line="182"/>
        <source>&lt;p&gt;&lt;b&gt;Note:&lt;/b&gt; You can safely install other parts first, but the above steps are &lt;b&gt;required&lt;/b&gt; to finish the installation!&lt;/p&gt;</source>
        <translation>&lt;p&gt;&lt;b&gt;Примечание:&lt;/b&gt; Вы можете безопасно устанавливать дополнения, но выше указанные операции &lt;b&gt;обязательны&lt;/b&gt; для завершения установки!&lt;/p&gt;</translation>
    </message>
</context>
<context>
    <name>QuaZipFile</name>
    <message>
        <location filename="../quazip/quazipfile.cpp" line="251"/>
        <source>ZIP/UNZIP API error %1</source>
        <translation>ошибка ZIP/UNZIP API %1</translation>
    </message>
</context>
<context>
    <name>RbUtilQt</name>
    <message>
        <source>Confirm Installation</source>
        <translation type="vanished">Подтвердите установку</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="564"/>
        <source>Mount point is wrong!</source>
        <translation>Точка монтирования неверная!</translation>
    </message>
    <message>
        <source>Do you really want to install the voice file?</source>
        <translation type="vanished">Вы действительно хотите установить голосовой файл?</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="495"/>
        <source>Confirm Uninstallation</source>
        <translation>Подтвердите удаление</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="496"/>
        <source>Do you really want to uninstall the Bootloader?</source>
        <translation>Вы действительно хотите удалить загрузчик?</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="550"/>
        <source>Confirm installation</source>
        <translation>Подтвердите установку</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="551"/>
        <source>Do you really want to install Rockbox Utility to your player? After installation you can run it from the players hard drive.</source>
        <translation>Вы действительно хотите установить мастера Rockbox на Ваш аудио плеер? Вы сможете потом запустить это с диска или памяти плеера.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="560"/>
        <source>Installing Rockbox Utility</source>
        <translation>Установка мастера Rockbox</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="578"/>
        <source>Error installing Rockbox Utility</source>
        <translation>Ошибка при установке мастера Rockbox</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="582"/>
        <source>Installing user configuration</source>
        <translation>Установка настроек пользователя</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="586"/>
        <source>Error installing user configuration</source>
        <translation>Ошибка при установке настроек пользователя</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="590"/>
        <source>Successfully installed Rockbox Utility.</source>
        <translation>Мастер Rockbox успешно установлен.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="393"/>
        <location filename="../rbutilqt.cpp" line="624"/>
        <source>Configuration error</source>
        <translation>Ошибка в настройках</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="625"/>
        <source>Your configuration is invalid. Please go to the configuration dialog and make sure the selected values are correct.</source>
        <translation>Ваши настройки недействительны. Проверьте, что ваши настройки правильные в окне настроек.</translation>
    </message>
    <message>
        <source>Warning</source>
        <translation type="vanished">Предупреждение</translation>
    </message>
    <message>
        <source>The Application is still downloading Information about new Builds. Please try again shortly.</source>
        <translation type="vanished">Программа ещё загружает информацию о новых версиях. Попробуйте снова через несколько мгновений.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="385"/>
        <source>New installation</source>
        <translation>Новая установка</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="394"/>
        <source>Your configuration is invalid. This is most likely due to a changed device path. The configuration dialog will now open to allow you to correct the problem.</source>
        <translation>Ваши настройки негодны. Это скорее всего из-за изменённого пути к устройству. Окно настроек сейчас откроется, чтобы позволить Вам решить проблему.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="263"/>
        <source>Network error</source>
        <translation>Ошибка сети</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="212"/>
        <source>Downloading build information, please wait ...</source>
        <translation>Скачивается информация о сборке, пожалуйста подождите...</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="262"/>
        <source>Can&apos;t get version information!</source>
        <translation>Не удалось получить информацию о версии!</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="386"/>
        <source>This is a new installation of Rockbox Utility, or a new version. The configuration dialog will now open to allow you to setup the program,  or review your settings.</source>
        <translation>Это новая установка или новая версия мастера Rockbox. Диалог настройки сейчас откроется и даст возможность настроить программу или пересмотреть ваши настройки.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="276"/>
        <source>Download build information finished.</source>
        <translation>Загрузка информации о сборке завершена.</translation>
    </message>
    <message>
        <source>RockboxUtility Update available</source>
        <translation type="vanished">Доступно обновление мастера Rockbox</translation>
    </message>
    <message>
        <source>&lt;b&gt;New RockboxUtility Version available.&lt;/b&gt; &lt;br&gt;&lt;br&gt;Download it from here: &lt;a href=&apos;%1&apos;&gt;%2&lt;/a&gt;</source>
        <translation type="vanished">Доступна новая версия мастера Rockbox. Скачать можно отсюда: &lt;a href=&apos;%1&apos;&gt;%2&lt;/a&gt;</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="104"/>
        <source>Wine detected!</source>
        <translation>Обнаружен Wine!</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="105"/>
        <source>It seems you are trying to run this program under Wine. Please don&apos;t do this, running under Wine will fail. Use the native Linux binary instead.</source>
        <translation>Похоже, что вы пытаетесь пользоваться этой программой с помощью Wine. Не делайте этого, это приведёт к сбою. Лучше пользуйтесь нативной программой для Linux.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="264"/>
        <source>Can&apos;t get version information.
Network error: %1. Please check your network and proxy settings.</source>
        <translation>Не удалось получить информацию о версии.
Ошибка сети: %1. Проверьте настройки сети и прокси.</translation>
    </message>
    <message>
        <source>No Rockbox installation found</source>
        <translation type="vanished">Не найдено установки Rockbox</translation>
    </message>
    <message>
        <source>Could not determine the installed Rockbox version. Please install a Rockbox build before installing voice files.</source>
        <translation type="vanished">Не удалось определить версию установленного Rockbox. Устанавливайте Rockbox перед установкой голосовых файлов.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="508"/>
        <source>No uninstall method for this target known.</source>
        <translation>Нет известного способа удаления с этого устройства.</translation>
    </message>
    <message>
        <source>Rockbox Utility can not uninstall the bootloader on this target. Try a normal firmware update to remove the booloader.</source>
        <translation type="vanished">Мастер Rockbox не может удалить загрузчик с этого устройства. Попробуйте нормальное обновление прошивки, чтобы удалить загрузчик.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="646"/>
        <source>Checking for update ...</source>
        <translation>Проверяется наличие обновления ...</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="715"/>
        <source>New version of Rockbox Utility available.</source>
        <translation>Доступна новая версия мастера Rockbox.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="718"/>
        <source>Rockbox Utility is up to date.</source>
        <translation>Мастер Rockbox не требует обновления.</translation>
    </message>
    <message>
        <source>No voice file available</source>
        <translation type="vanished">Нет доступного голосового файла</translation>
    </message>
    <message>
        <source>The installed version of Rockbox is a development version. Pre-built voices are only available for release versions of Rockbox. Please generate a voice yourself using the &quot;Create voice file&quot; functionality.</source>
        <translation type="vanished">Установленная версия Rockbox является официально нестабильной. Готовые голоса доступны только для стабильных версий Rockbox. Создайте вручную голосовой файл с помощью кнопки &quot;Создать голосовой файл&quot;.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="531"/>
        <source>No Rockbox bootloader found.</source>
        <translation>Не найдено загрузчика Rockbox.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="741"/>
        <source>Device ejected</source>
        <translation>Устройство извлечено</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="742"/>
        <source>Device successfully ejected. You may now disconnect the player from the PC.</source>
        <translation>Устройство успешно извлечено. Теперь можно отсоединить плеер от компьютера.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="746"/>
        <source>Ejecting failed</source>
        <translation>Не удалось извлечь</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="747"/>
        <source>Ejecting the device failed. Please make sure no programs are accessing files on the device. If ejecting still fails please use your computers eject funtionality.</source>
        <translation>Извлечение не удалось. Убедитесь в том, что на устройстве нет файлов, занятых другими программами. Если извлечь всё равно не получается, пользуйтесь функцией извлечения Вашего компьютера.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="228"/>
        <source>Certificate error</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="230"/>
        <source>%1

Issuer: %2
Subject: %3
Valid since: %4
Valid until: %5

Temporarily trust certificate?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="305"/>
        <source>Libraries used</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="520"/>
        <source>Rockbox Utility can not uninstall the bootloader on your player. Please perform a firmware update using your player vendors firmware update process.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="523"/>
        <source>Important: make sure to boot your player into the original firmware before using the vendors firmware update process.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="710"/>
        <source>Rockbox Utility Update available</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="711"/>
        <source>&lt;b&gt;New Rockbox Utility version available.&lt;/b&gt;&lt;br&gt;&lt;br&gt;You are currently using version %1. Get version %2 at &lt;a href=&apos;%3&apos;&gt;%3&lt;/a&gt;</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>RbUtilQtFrm</name>
    <message>
        <location filename="../rbutilqtfrm.ui" line="14"/>
        <source>Rockbox Utility</source>
        <translation>Мастер Rockbox</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="59"/>
        <source>Device</source>
        <translation>Устройство</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="120"/>
        <source>&amp;Change</source>
        <translation>&amp;Изменить</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="166"/>
        <source>Welcome</source>
        <translation>Добро пожаловать</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="163"/>
        <location filename="../rbutilqtfrm.ui" line="620"/>
        <source>&amp;Installation</source>
        <translation>&amp;Установка</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="171"/>
        <location filename="../rbutilqtfrm.ui" line="413"/>
        <source>&amp;Accessibility</source>
        <translation>&amp;Специальные возможности</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="174"/>
        <source>Install accessibility add-ons</source>
        <translation>Установить дополнения для специальных возможностей</translation>
    </message>
    <message>
        <source>Install Voice files</source>
        <translation type="vanished">Установить голосовые файлы</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="180"/>
        <source>Install Talk files</source>
        <translation>Установить файлы произношения</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="420"/>
        <source>&amp;Uninstallation</source>
        <translation>&amp;Удаление</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="262"/>
        <location filename="../rbutilqtfrm.ui" line="295"/>
        <source>Uninstall Rockbox</source>
        <translation>Удалить Rockbox</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="268"/>
        <source>Uninstall Bootloader</source>
        <translation>Удалить загрузчик</translation>
    </message>
    <message>
        <source>&amp;Manual</source>
        <translation type="vanished">&amp;Руководство по эксплуатации</translation>
    </message>
    <message>
        <source>View and download the manual</source>
        <translation type="vanished">Смотреть и/или загрузить руководство по эксплуатации</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="369"/>
        <source>Inf&amp;o</source>
        <translation>&amp;Информация</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="387"/>
        <source>&amp;File</source>
        <translation>&amp;Файл</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="459"/>
        <source>&amp;About</source>
        <translation>&amp;О программе</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="436"/>
        <source>Empty local download cache</source>
        <translation>Очистить локальный кэш скачивания</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="441"/>
        <source>Install Rockbox Utility on player</source>
        <translation>Установить мастера Rockbox на плеер</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="446"/>
        <source>&amp;Configure</source>
        <translation>&amp;Настройки</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="451"/>
        <source>E&amp;xit</source>
        <translation>&amp;Выйти</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="454"/>
        <source>Ctrl+Q</source>
        <translation>Ctrl+Q</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="464"/>
        <source>About &amp;Qt</source>
        <translation>О &amp;Qt</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="396"/>
        <location filename="../rbutilqtfrm.ui" line="469"/>
        <source>&amp;Help</source>
        <translation>&amp;Помощь</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="409"/>
        <source>Action&amp;s</source>
        <translation>&amp;Действия</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="474"/>
        <source>Info</source>
        <translation>Информация</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="578"/>
        <source>Read PDF manual</source>
        <translation>Читать руководство по эксплуатации в PDF</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="583"/>
        <source>Read HTML manual</source>
        <translation>Читать руководство по эксплуатации в HTML</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="588"/>
        <source>Download PDF manual</source>
        <translation>Скачать руководство по эксплуатации в PDF</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="593"/>
        <source>Download HTML manual (zip)</source>
        <translation>Скачать руководство по эксплуатации в HTML (zip)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="230"/>
        <source>Create Voice files</source>
        <translation>Создать голосовые файлы</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="605"/>
        <source>Create Voice File</source>
        <translation>Создать голосовой файл</translation>
    </message>
    <message>
        <source>&lt;b&gt;Install Voice file&lt;/b&gt;&lt;br/&gt;Voice files are needed to make Rockbox speak the user interface. Speaking is enabled by default, so if you installed the voice file Rockbox will speak.</source>
        <translation type="vanished">&lt;b&gt;Установить голосовой файл&lt;/b&gt;&lt;br/&gt;Он нужен, чтобы Rockbox произносил пользовательский интерфейс. Произношение включено по умолчанию, поэтому если Вы установили голосовой файл, Rockbox станет разговаривать.</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="197"/>
        <source>&lt;b&gt;Create Talk Files&lt;/b&gt;&lt;br/&gt;Talkfiles are needed to let Rockbox speak File and Foldernames</source>
        <translation>&lt;b&gt;Создать файлы произношения.&lt;/b&gt;&lt;br/&gt;Они нужны, чтобы Rockbox мог произносить имена файлов и папок</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="285"/>
        <source>&lt;b&gt;Remove the bootloader&lt;/b&gt;&lt;br/&gt;After removing the bootloader you won&apos;t be able to start Rockbox.</source>
        <translation>&lt;b&gt;Удалить загрузчик&lt;/b&gt;&lt;br/&gt;После удаления загрузчика, вы не сможете запустить Rockbox.</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="312"/>
        <source>&lt;b&gt;Uninstall Rockbox from your audio player.&lt;/b&gt;&lt;br/&gt;This will leave the bootloader in place (you need to remove it manually).</source>
        <translation>&lt;b&gt;Удалить Rockbox с Вашего плеера.&lt;/b&gt;&lt;br/&gt;Это оставит загрузчик установленным (его нужно будет удалить вручную).</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="501"/>
        <source>Install &amp;Bootloader</source>
        <translation>Установить &amp;загрузчик</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="510"/>
        <source>Install &amp;Rockbox</source>
        <translation>Установить &amp;Rockbox</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="519"/>
        <source>Install &amp;Fonts Package</source>
        <translation>Установить пакет &amp;шрифтов</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="528"/>
        <source>Install &amp;Themes</source>
        <translation>Установить &amp;темы</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="537"/>
        <source>Install &amp;Game Files</source>
        <translation>Установить игровые &amp;файлы</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="546"/>
        <source>&amp;Install Voice File</source>
        <translation>&amp;Установить голосовой файл</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="555"/>
        <source>Create &amp;Talk Files</source>
        <translation>Установить файлы &amp;произношения</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="564"/>
        <source>Remove &amp;bootloader</source>
        <translation>У&amp;далить загрузчик</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="573"/>
        <source>Uninstall &amp;Rockbox</source>
        <translation>Удалить &amp;Rockbox</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="602"/>
        <source>Create &amp;Voice File</source>
        <translation>&amp;Создать голосовой файл</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="610"/>
        <source>&amp;System Info</source>
        <translation>Информация о &amp;системе</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="483"/>
        <source>&amp;Complete Installation</source>
        <translation>&amp;Полная установка</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="492"/>
        <source>&amp;Minimal Installation</source>
        <translation>&amp;Минимальная установка</translation>
    </message>
    <message>
        <source>&amp;Troubleshoot</source>
        <translation type="vanished">&amp;Устранение неполадок</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="615"/>
        <source>System &amp;Trace</source>
        <translation>&amp;Трассировка системы</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="247"/>
        <source>&lt;b&gt;Create Voice file&lt;/b&gt;&lt;br/&gt;Voice files are needed to make Rockbox speak the  user interface. Speaking is enabled by default, so
 if you installed the voice file Rockbox will speak.</source>
        <translation>&lt;b&gt;Создать голосовой файл&lt;/b&gt;&lt;br/&gt;Он нужен, чтобы Rockbox произносил пользовательский интерфейс. Произношение включено по умолчанию, поэтому если Вы установили голосовой файл, Rockbox станет разговаривать.</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="149"/>
        <source>mountpoint unknown or invalid</source>
        <translation>точка монтирования неизвестна или неправильна</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="142"/>
        <source>Mountpoint:</source>
        <translation>Точка монтирования:</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="100"/>
        <source>device unknown or invalid</source>
        <translation>устройство неизвестно или неправильно</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="93"/>
        <source>Device:</source>
        <translation>Устройство:</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="259"/>
        <source>Backup &amp;&amp; &amp;Uninstallation</source>
        <translation>Резервная копия и &amp;Удаление</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="325"/>
        <source>Backup</source>
        <translation>Создать резервную копию</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="342"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;&lt;span style=&quot; font-weight:600;&quot;&gt;Backup current installation.&lt;/span&gt;&lt;/p&gt;&lt;p&gt;Create a backup by archiving the contents of the Rockbox installation folder.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;&lt;span style=&quot; font-weight:600;&quot;&gt;Создать резервную копию текущей установки.&lt;/span&gt;&lt;/p&gt;&lt;p&gt;Создать копию, архивируя содержимое системной папки Rockbox.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="131"/>
        <source>&amp;Eject</source>
        <translation>&amp;Извлечь</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="625"/>
        <source>Show &amp;Changelog</source>
        <translation>Показать &amp;историю изменений</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="43"/>
        <source>Welcome to Rockbox Utility, the installation and housekeeping tool for Rockbox.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="46"/>
        <source>Rockbox Logo</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>SelectiveInstallWidget</name>
    <message>
        <location filename="../gui/selectiveinstallwidgetfrm.ui" line="26"/>
        <source>Rockbox version to install</source>
        <translation>Версия Rockbox для установки</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidgetfrm.ui" line="35"/>
        <source>Version information not available yet.</source>
        <translation>Сведения о версии пока отсутствуют.</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidgetfrm.ui" line="54"/>
        <source>Rockbox components to install</source>
        <translation>Части Rockbox для установки</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidgetfrm.ui" line="60"/>
        <source>&amp;Bootloader</source>
        <translation>&amp;Загрузчик</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidgetfrm.ui" line="253"/>
        <source>The main Rockbox firmware.</source>
        <translation>Основная программа Rockbox.</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidgetfrm.ui" line="115"/>
        <source>Fonts</source>
        <translation>Шрифты</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidgetfrm.ui" line="74"/>
        <source>&amp;Rockbox</source>
        <translation>&amp;Rockbox</translation>
    </message>
    <message>
        <source>Some game plugins require additional files.</source>
        <translation type="vanished">Некоторые игры требуют дополнительных файлов.</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidgetfrm.ui" line="178"/>
        <source>Additional fonts for the User Interface.</source>
        <translation>Дополнительные шрифты для пользовательского интерфейса.</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidgetfrm.ui" line="135"/>
        <source>The bootloader is required for starting Rockbox. Only necessary for first time install.</source>
        <translation>Загрузчик требуется для запуска Rockbox. Нужен только для первой установки.</translation>
    </message>
    <message>
        <source>Game Files</source>
        <translation type="vanished">Файлы игр</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidgetfrm.ui" line="161"/>
        <source>Customize</source>
        <translation>Выбрать</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidgetfrm.ui" line="104"/>
        <source>Themes</source>
        <translation>Темы</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidgetfrm.ui" line="94"/>
        <source>Themes allow adjusting the user interface of Rockbox. Use &quot;Customize&quot; to select themes.</source>
        <translation>Темы позволяют изменить пользовательский интерфейс Rockbox. Вы можете их выбрать в списке.</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidgetfrm.ui" line="292"/>
        <source>&amp;Install</source>
        <translation>&amp;Установить</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="78"/>
        <source>This is the latest stable release available.</source>
        <translation>Это последняя стабильная версия.</translation>
    </message>
    <message>
        <source>The development version is updated on every code change. Last update was on %1</source>
        <translation type="vanished">Разрабатываемая версия обновляется с каждым изменением исходного кода. Дата последнего обновления: %1</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="88"/>
        <source>This will eventually become the next Rockbox version. Install it to help testing.</source>
        <translation>Это станет следующей стабильной версией Rockbox. Устанавливайте для тестирования.</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="128"/>
        <source>Stable Release (Version %1)</source>
        <translation>Стабильная версия (%1)</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="132"/>
        <source>Development Version (Revison %1)</source>
        <translation>Разрабатываемая версия (ревизия %1)</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="130"/>
        <source>Release Candidate (Revison %1)</source>
        <translation>Пробная версия (ревизия %1)</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="159"/>
        <source>The selected player doesn&apos;t need a bootloader.</source>
        <translation>Выбранный плеер не требует загрузчика.</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="164"/>
        <source>The bootloader is required for starting Rockbox. Installation of the bootloader is only necessary on first time installation.</source>
        <translation>Загрузчик требуется для запуска Rockbox. Установка загрузчика требуется только при первой установке.</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="237"/>
        <source>Mountpoint is wrong</source>
        <translation>Точка монтирования неверна</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="295"/>
        <source>No install method known.</source>
        <translation>Нет известного способа установки.</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="319"/>
        <source>Bootloader detected</source>
        <translation>Найден загрузчик</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="320"/>
        <source>Bootloader already installed. Do you want to reinstall the bootloader?</source>
        <translation>Загрузчик уже установлен. Переустановить?</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="324"/>
        <source>Bootloader installation skipped</source>
        <translation>Установка загрузчика пропущена</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="338"/>
        <source>Create Bootloader backup</source>
        <translation>Создать резервную копию загрузчика</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="339"/>
        <source>You can create a backup of the original bootloader file. Press &quot;Yes&quot; to select an output folder on your computer to save the file to. The file will get placed in a new folder &quot;%1&quot; created below the selected folder.
Press &quot;No&quot; to skip this step.</source>
        <translation>Вы можете создать резервную копию фирменного файла загрузчика. Нажмите на &quot;Да&quot;, чтобы выбрать выходную папку, в которой будет создана ещё одна папка &quot;%1&quot;, содержащая файл.
Нажмите на &quot;Нет&quot;, чтобы пропустить этот шаг.</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="346"/>
        <source>Browse backup folder</source>
        <translation>Обзор папки резервных копий</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="358"/>
        <source>Prerequisites</source>
        <translation>Предварительные требования</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="363"/>
        <source>Bootloader installation aborted</source>
        <translation>Установка загрузчика отменена</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="373"/>
        <source>Bootloader files (%1)</source>
        <translation>Загрузочные файлы (%1)</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="375"/>
        <source>All files (*)</source>
        <translation>Все файлы (*)</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="377"/>
        <source>Select firmware file</source>
        <translation>Выберите файл прошивки</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="379"/>
        <source>Error opening firmware file</source>
        <translation>Ошибка при открытии файла прошивки</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="385"/>
        <source>Error reading firmware file</source>
        <translation>Ошибка при чтении файла прошивки</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="395"/>
        <source>Backup error</source>
        <translation>Ошибка резервной копии</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="396"/>
        <source>Could not create backup file. Continue?</source>
        <translation>Не удалось создать резеврную копию файла. Продолжить?</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="419"/>
        <location filename="../gui/selectiveinstallwidget.cpp" line="431"/>
        <source>Manual steps required</source>
        <translation>Требуются действия вручную</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="225"/>
        <source>Continue with installation?</source>
        <translation>Продолжить и приступить к установке?</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="226"/>
        <source>Really continue?</source>
        <translatorcomment>:-)</translatorcomment>
        <translation>Точно продожить?</translation>
    </message>
    <message>
        <source>Aborted!</source>
        <translation type="obsolete">Отменено!</translation>
    </message>
    <message>
        <source>Your installation doesn&apos;t require game files, skipping.</source>
        <translation type="vanished">Ваша установка не требует игровых файлов, шаг пропущен.</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidgetfrm.ui" line="20"/>
        <source>Selective Installation</source>
        <translation>Выборочная установка</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidgetfrm.ui" line="151"/>
        <source>Some plugins require additional data files.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidgetfrm.ui" line="188"/>
        <source>Install prerendered voice file.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidgetfrm.ui" line="195"/>
        <source>Plugin Data</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidgetfrm.ui" line="222"/>
        <source>&amp;Manual</source>
        <translation type="unfinished">&amp;Руководство по эксплуатации</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidgetfrm.ui" line="233"/>
        <source>&amp;Voice File</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidgetfrm.ui" line="263"/>
        <source>Save a copy of the manual on the player.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="83"/>
        <source>The development version is updated on every code change.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="93"/>
        <source>Daily updated development version.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="100"/>
        <source>Not available for the selected version</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="131"/>
        <source>Daily Build (%1)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="667"/>
        <source>Your installation doesn&apos;t require any plugin data files, skipping.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>ServerInfo</name>
    <message>
        <source>Unknown</source>
        <translation type="vanished">Неизвестный</translation>
    </message>
    <message>
        <source>Unusable</source>
        <translation type="vanished">Непригодный</translation>
    </message>
    <message>
        <source>Unstable</source>
        <translation type="vanished">Нестабильный</translation>
    </message>
    <message>
        <source>Stable</source>
        <translation type="vanished">Стабильный</translation>
    </message>
</context>
<context>
    <name>SysTrace</name>
    <message>
        <location filename="../systrace.cpp" line="100"/>
        <location filename="../systrace.cpp" line="109"/>
        <source>Save system trace log</source>
        <translation>Сохранить журнал трассировки системы</translation>
    </message>
</context>
<context>
    <name>SysTraceFrm</name>
    <message>
        <location filename="../systracefrm.ui" line="14"/>
        <source>System Trace</source>
        <translation>Трассировка системы</translation>
    </message>
    <message>
        <location filename="../systracefrm.ui" line="20"/>
        <source>System State trace</source>
        <translation>Трассировка состояния системы</translation>
    </message>
    <message>
        <location filename="../systracefrm.ui" line="46"/>
        <source>&amp;Close</source>
        <translation>&amp;Закрыть</translation>
    </message>
    <message>
        <location filename="../systracefrm.ui" line="57"/>
        <source>&amp;Save</source>
        <translation>&amp;Сохранить</translation>
    </message>
    <message>
        <location filename="../systracefrm.ui" line="68"/>
        <source>&amp;Refresh</source>
        <translation>&amp;Обновить</translation>
    </message>
    <message>
        <location filename="../systracefrm.ui" line="79"/>
        <source>Save &amp;previous</source>
        <translation>Сохранить &amp;предыдущий</translation>
    </message>
</context>
<context>
    <name>Sysinfo</name>
    <message>
        <location filename="../sysinfo.cpp" line="46"/>
        <source>&lt;b&gt;OS&lt;/b&gt;&lt;br/&gt;</source>
        <translation>&lt;b&gt;ОС&lt;/b&gt;&lt;br/&gt;</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="47"/>
        <source>&lt;b&gt;Username&lt;/b&gt;&lt;br/&gt;%1&lt;hr/&gt;</source>
        <translation>&lt;b&gt;Имя пользователя&lt;/b&gt;&lt;br/&gt;%1&lt;hr/&gt;</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="49"/>
        <source>&lt;b&gt;Permissions&lt;/b&gt;&lt;br/&gt;%1&lt;hr/&gt;</source>
        <translation>&lt;b&gt;Полномочия&lt;/b&gt;&lt;br/&gt;%1&lt;hr/&gt;</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="51"/>
        <source>&lt;b&gt;Attached USB devices&lt;/b&gt;&lt;br/&gt;</source>
        <translation>&lt;b&gt;Подключенные USB-устройства&lt;/b&gt;&lt;br/&gt;</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="55"/>
        <source>VID: %1 PID: %2, %3</source>
        <translation>VID: %1 PID: %2, %3</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="64"/>
        <source>Filesystem</source>
        <translation>Файловая система</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="67"/>
        <source>Mountpoint</source>
        <translation>Точка монтирования</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="67"/>
        <source>Label</source>
        <translation>Метка</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="68"/>
        <source>Free</source>
        <translation>Свободно</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="68"/>
        <source>Total</source>
        <translation>Всего</translation>
    </message>
    <message>
        <source>Cluster Size</source>
        <translation type="vanished">Размер кластера</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="71"/>
        <source>&lt;tr&gt;&lt;td&gt;%1&lt;/td&gt;&lt;td&gt;%4&lt;/td&gt;&lt;td&gt;%2 GiB&lt;/td&gt;&lt;td&gt;%3 GiB&lt;/td&gt;&lt;td&gt;%5&lt;/td&gt;&lt;/tr&gt;</source>
        <translation>&lt;tr&gt;&lt;td&gt;%1&lt;/td&gt;&lt;td&gt;%4&lt;/td&gt;&lt;td&gt;%2 ГиБ&lt;/td&gt;&lt;td&gt;%3 ГБ&lt;/td&gt;&lt;td&gt;%5&lt;/td&gt;&lt;/tr&gt;</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="69"/>
        <source>Type</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>SysinfoFrm</name>
    <message>
        <location filename="../sysinfofrm.ui" line="13"/>
        <source>System Info</source>
        <translation>Информация о системе</translation>
    </message>
    <message>
        <location filename="../sysinfofrm.ui" line="22"/>
        <source>&amp;Refresh</source>
        <translation>&amp;Обновить</translation>
    </message>
    <message>
        <location filename="../sysinfofrm.ui" line="46"/>
        <source>&amp;OK</source>
        <translation>&amp;OK</translation>
    </message>
</context>
<context>
    <name>System</name>
    <message>
        <location filename="../base/system.cpp" line="117"/>
        <source>Guest</source>
        <translation>Гость</translation>
    </message>
    <message>
        <location filename="../base/system.cpp" line="120"/>
        <source>Admin</source>
        <translation>Администратор</translation>
    </message>
    <message>
        <location filename="../base/system.cpp" line="123"/>
        <source>User</source>
        <translation>Пользователь</translation>
    </message>
    <message>
        <location filename="../base/system.cpp" line="126"/>
        <source>Error</source>
        <translation>Ошибка</translation>
    </message>
    <message>
        <location filename="../base/system.cpp" line="273"/>
        <source>(no description available)</source>
        <translation>(описание недоступно)</translation>
    </message>
</context>
<context>
    <name>TTSBase</name>
    <message>
        <location filename="../base/ttsbase.cpp" line="47"/>
        <source>Espeak TTS Engine</source>
        <translation>Espeak TTS движок</translation>
    </message>
    <message>
        <location filename="../base/ttsbase.cpp" line="51"/>
        <source>Flite TTS Engine</source>
        <translation>Flite TTS движок</translation>
    </message>
    <message>
        <location filename="../base/ttsbase.cpp" line="52"/>
        <source>Swift TTS Engine</source>
        <translation>Swift TTS движок</translation>
    </message>
    <message>
        <source>SAPI TTS Engine</source>
        <translation type="obsolete">SAPI TTS движок</translation>
    </message>
    <message>
        <location filename="../base/ttsbase.cpp" line="61"/>
        <source>Festival TTS Engine</source>
        <translation>Festival TTS движок</translation>
    </message>
    <message>
        <location filename="../base/ttsbase.cpp" line="64"/>
        <source>OS X System Engine</source>
        <translation>Системный движок OS X</translation>
    </message>
    <message>
        <location filename="../base/ttsbase.cpp" line="55"/>
        <source>SAPI4 TTS Engine</source>
        <translation>Движок TTS SAPI4</translation>
    </message>
    <message>
        <location filename="../base/ttsbase.cpp" line="57"/>
        <source>SAPI5 TTS Engine</source>
        <translation>Движок TTS SAPI5</translation>
    </message>
    <message>
        <location filename="../base/ttsbase.cpp" line="58"/>
        <source>MS Speech Platform</source>
        <translation>Платформа MS Speech</translation>
    </message>
    <message>
        <location filename="../base/ttsbase.cpp" line="48"/>
        <source>Espeak-ng TTS Engine</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/ttsbase.cpp" line="49"/>
        <source>Mimic TTS Engine</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>TTSCarbon</name>
    <message>
        <location filename="../base/ttscarbon.cpp" line="139"/>
        <source>Voice:</source>
        <translation>Голос:</translation>
    </message>
    <message>
        <location filename="../base/ttscarbon.cpp" line="145"/>
        <source>Speed (words/min):</source>
        <translation>Скорость (слов/мин):</translation>
    </message>
    <message>
        <location filename="../base/ttscarbon.cpp" line="222"/>
        <source>Could not voice string</source>
        <translation>Не удалось произнести</translation>
    </message>
    <message>
        <location filename="../base/ttscarbon.cpp" line="232"/>
        <source>Could not convert intermediate file</source>
        <translation>Не удалось преобразовать промежуточный файл</translation>
    </message>
    <message>
        <location filename="../base/ttscarbon.cpp" line="152"/>
        <source>Pitch (0 for default):</source>
        <translation>Тон (0 по умолчанию) :</translation>
    </message>
</context>
<context>
    <name>TTSExes</name>
    <message>
        <location filename="../base/ttsexes.cpp" line="78"/>
        <source>TTS executable not found</source>
        <translation>Выполняемый файл TTS не найден</translation>
    </message>
    <message>
        <location filename="../base/ttsexes.cpp" line="44"/>
        <source>Path to TTS engine:</source>
        <translation>Путь к мотору TTS:</translation>
    </message>
    <message>
        <location filename="../base/ttsexes.cpp" line="46"/>
        <source>TTS engine options:</source>
        <translation>Настройки мотора TTS:</translation>
    </message>
</context>
<context>
    <name>TTSFestival</name>
    <message>
        <location filename="../base/ttsfestival.cpp" line="207"/>
        <source>engine could not voice string</source>
        <translation>мотор не смог озвучить выражение</translation>
    </message>
    <message>
        <location filename="../base/ttsfestival.cpp" line="290"/>
        <source>No description available</source>
        <translation>Описание отсутствует</translation>
    </message>
    <message>
        <location filename="../base/ttsfestival.cpp" line="53"/>
        <source>Path to Festival client:</source>
        <translation>Путь к клиенту Festival:</translation>
    </message>
    <message>
        <location filename="../base/ttsfestival.cpp" line="58"/>
        <source>Voice:</source>
        <translation>Голос:</translation>
    </message>
    <message>
        <location filename="../base/ttsfestival.cpp" line="69"/>
        <source>Voice description:</source>
        <translation>Описание голоса:</translation>
    </message>
    <message>
        <location filename="../base/ttsfestival.cpp" line="173"/>
        <source>Festival could not be started</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>TTSSapi</name>
    <message>
        <location filename="../base/ttssapi.cpp" line="46"/>
        <source>Language:</source>
        <translation>Язык:</translation>
    </message>
    <message>
        <location filename="../base/ttssapi.cpp" line="53"/>
        <source>Voice:</source>
        <translation>Голос:</translation>
    </message>
    <message>
        <location filename="../base/ttssapi.cpp" line="65"/>
        <source>Speed:</source>
        <translation>Скорость:</translation>
    </message>
    <message>
        <location filename="../base/ttssapi.cpp" line="68"/>
        <source>Options:</source>
        <translation>Настройки:</translation>
    </message>
    <message>
        <location filename="../base/ttssapi.cpp" line="112"/>
        <source>Could not copy the SAPI script</source>
        <translation>Не удалось скопировать SAPI-сценарий</translation>
    </message>
    <message>
        <location filename="../base/ttssapi.cpp" line="130"/>
        <source>Could not start SAPI process</source>
        <translation>Не удалось запустить SAPI-задачу</translation>
    </message>
</context>
<context>
    <name>TalkFileCreator</name>
    <message>
        <location filename="../base/talkfile.cpp" line="45"/>
        <source>Talk file creation aborted</source>
        <translation>Создание файла произношения отменено</translation>
    </message>
    <message>
        <location filename="../base/talkfile.cpp" line="78"/>
        <source>Finished creating Talk files</source>
        <translation>Создание файлов произношения завершено</translation>
    </message>
    <message>
        <location filename="../base/talkfile.cpp" line="42"/>
        <source>Reading Filelist...</source>
        <translation>Чтение списка файлов...</translation>
    </message>
    <message>
        <location filename="../base/talkfile.cpp" line="276"/>
        <source>Copying of %1 to %2 failed</source>
        <translation>Сбой копирования %1 в %2</translation>
    </message>
    <message>
        <location filename="../base/talkfile.cpp" line="66"/>
        <source>Copying Talkfiles...</source>
        <translation>Копирую файлы произношения...</translation>
    </message>
    <message>
        <location filename="../base/talkfile.cpp" line="257"/>
        <source>File copy aborted</source>
        <translation>Копия файлов отменена</translation>
    </message>
    <message>
        <location filename="../base/talkfile.cpp" line="298"/>
        <source>Cleaning up...</source>
        <translation>Навожу порядок...</translation>
    </message>
    <message>
        <location filename="../base/talkfile.cpp" line="309"/>
        <source>Finished</source>
        <translation>Всё</translation>
    </message>
    <message>
        <location filename="../base/talkfile.cpp" line="36"/>
        <source>Starting Talk file generation for folder %1</source>
        <translation>Начинается создание голосового файла для папки %1</translation>
    </message>
</context>
<context>
    <name>TalkGenerator</name>
    <message>
        <location filename="../base/talkgenerator.cpp" line="38"/>
        <source>Starting TTS Engine</source>
        <translation>Запуск мотора TTS</translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="43"/>
        <location filename="../base/talkgenerator.cpp" line="50"/>
        <source>Init of TTS engine failed</source>
        <translation>Сбой инициализации мотора TTS</translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="57"/>
        <source>Starting Encoder Engine</source>
        <translation>Запуск мотора кодировщика</translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="62"/>
        <source>Init of Encoder engine failed</source>
        <translation>Сбой инициализации мотора кодировщика</translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="72"/>
        <source>Voicing entries...</source>
        <translation>Озвучивание вводов...</translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="87"/>
        <source>Encoding files...</source>
        <translation>Кодировка файлов...</translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="126"/>
        <source>Voicing aborted</source>
        <translation>Озвучивание отменено</translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="163"/>
        <location filename="../base/talkgenerator.cpp" line="168"/>
        <source>Voicing of %1 failed: %2</source>
        <translation>Сбой озвучивания %1 : %2</translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="212"/>
        <source>Encoding aborted</source>
        <translation>Кодировка отменена</translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="240"/>
        <source>Encoding of %1 failed</source>
        <translation>Сбой кодировки %1</translation>
    </message>
</context>
<context>
    <name>ThemeInstallFrm</name>
    <message>
        <location filename="../themesinstallfrm.ui" line="13"/>
        <source>Theme Installation</source>
        <translation>Установка тем</translation>
    </message>
    <message>
        <location filename="../themesinstallfrm.ui" line="48"/>
        <source>Selected Theme</source>
        <translation>Выбранная тема</translation>
    </message>
    <message>
        <location filename="../themesinstallfrm.ui" line="73"/>
        <source>Description</source>
        <translation>Описание</translation>
    </message>
    <message>
        <location filename="../themesinstallfrm.ui" line="83"/>
        <source>Download size:</source>
        <translation>Объём скачивания :</translation>
    </message>
    <message>
        <location filename="../themesinstallfrm.ui" line="126"/>
        <source>&amp;Cancel</source>
        <translation>&amp;Отмена</translation>
    </message>
    <message>
        <location filename="../themesinstallfrm.ui" line="115"/>
        <source>&amp;Install</source>
        <translation>&amp;Установить</translation>
    </message>
    <message>
        <location filename="../themesinstallfrm.ui" line="93"/>
        <source>Hold Ctrl to select multiple item, Shift for a range</source>
        <translation>Нажать и держать Ctrl для выделения нескольких элеметнов, Shift для выделения ряда элементов</translation>
    </message>
</context>
<context>
    <name>ThemesInstallWindow</name>
    <message>
        <location filename="../themesinstallwindow.cpp" line="41"/>
        <source>no theme selected</source>
        <translation>Тема не выделена</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="127"/>
        <source>Network error: %1.
Please check your network and proxy settings.</source>
        <translation>Ошибка сети: %1.
Проверьте настройки сети и прокси.</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="140"/>
        <source>the following error occured:
%1</source>
        <translation>Произошла следующая ошибка :
%1</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="146"/>
        <source>done.</source>
        <translation>выполнено.</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="219"/>
        <source>fetching details for %1</source>
        <translation>получаю подробности о %1</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="222"/>
        <source>fetching preview ...</source>
        <translation>Получаю предпросмотр ...</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="235"/>
        <source>&lt;b&gt;Author:&lt;/b&gt; %1&lt;hr/&gt;</source>
        <translation>&lt;b&gt;Автор :&lt;/b&gt; %1&lt;hr/&gt;</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="236"/>
        <location filename="../themesinstallwindow.cpp" line="238"/>
        <source>unknown</source>
        <translation>неизвестный</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="237"/>
        <source>&lt;b&gt;Version:&lt;/b&gt; %1&lt;hr/&gt;</source>
        <translation>&lt;b&gt;Версия :&lt;/b&gt; %1&lt;hr/&gt;</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="239"/>
        <source>&lt;b&gt;Description:&lt;/b&gt; %1&lt;hr/&gt;</source>
        <translation>&lt;b&gt;Описание:&lt;/b&gt; %1&lt;hr/&gt;</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="240"/>
        <source>no description</source>
        <translation>нет описания</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="267"/>
        <source>no theme preview</source>
        <translation>предпросмотр недоступен</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="302"/>
        <source>getting themes information ...</source>
        <translation>получаю информацию о темах ...</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="360"/>
        <source>Mount point is wrong!</source>
        <translation>Неправильная точка монтирования!</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="42"/>
        <source>no selection</source>
        <translation>нет выделения</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="183"/>
        <source>Information</source>
        <translation>Информация</translation>
    </message>
    <message numerus="yes">
        <location filename="../themesinstallwindow.cpp" line="203"/>
        <source>Download size %L1 kiB (%n item(s))</source>
        <translation>
            <numerusform>Размер загрузки %L1 КиБ (%n штука)</numerusform>
            <numerusform>Размер загрузки %L1 КиБ (%n штуки)</numerusform>
            <numerusform>Размер загрузки %L1 КиБ (%n штук)</numerusform>
        </translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="256"/>
        <source>Retrieving theme preview failed.
HTTP response code: %1</source>
        <translation>Сбой при получении предпросмотра темы.
Код ответа HTTP : %1</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="295"/>
        <source>Select</source>
        <translation>Выбрать</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="330"/>
        <source>No themes selected, skipping</source>
        <translation>Не выбрано ни одной темы, этап пропускается</translation>
    </message>
</context>
<context>
    <name>UninstallFrm</name>
    <message>
        <location filename="../uninstallfrm.ui" line="16"/>
        <source>Uninstall Rockbox</source>
        <translation>Удалить Rockbox</translation>
    </message>
    <message>
        <location filename="../uninstallfrm.ui" line="35"/>
        <source>Please select the Uninstallation Method</source>
        <translation>Выберите способ удаления</translation>
    </message>
    <message>
        <location filename="../uninstallfrm.ui" line="45"/>
        <source>Uninstallation Method</source>
        <translation>Способ удаления</translation>
    </message>
    <message>
        <location filename="../uninstallfrm.ui" line="51"/>
        <source>Complete Uninstallation</source>
        <translation>Полное удаление</translation>
    </message>
    <message>
        <location filename="../uninstallfrm.ui" line="58"/>
        <source>Smart Uninstallation</source>
        <translation>Выборочное удаление</translation>
    </message>
    <message>
        <location filename="../uninstallfrm.ui" line="68"/>
        <source>Please select what you want to uninstall</source>
        <translation>Выберите, что вы желаете удалить</translation>
    </message>
    <message>
        <location filename="../uninstallfrm.ui" line="78"/>
        <source>Installed Parts</source>
        <translation>Установленные части</translation>
    </message>
    <message>
        <location filename="../uninstallfrm.ui" line="139"/>
        <source>&amp;Cancel</source>
        <translation>&amp;Отмена</translation>
    </message>
    <message>
        <location filename="../uninstallfrm.ui" line="128"/>
        <source>&amp;Uninstall</source>
        <translation>&amp;Удалить</translation>
    </message>
</context>
<context>
    <name>Uninstaller</name>
    <message>
        <location filename="../base/uninstall.cpp" line="32"/>
        <location filename="../base/uninstall.cpp" line="43"/>
        <source>Starting Uninstallation</source>
        <translation>Начало удаления</translation>
    </message>
    <message>
        <location filename="../base/uninstall.cpp" line="36"/>
        <source>Finished Uninstallation</source>
        <translation>Удаление завершено</translation>
    </message>
    <message>
        <location filename="../base/uninstall.cpp" line="115"/>
        <source>Uninstallation finished</source>
        <translation>Удаление завершено</translation>
    </message>
    <message>
        <location filename="../base/uninstall.cpp" line="49"/>
        <source>Uninstalling %1...</source>
        <translation>Удаляется %1...</translation>
    </message>
    <message>
        <location filename="../base/uninstall.cpp" line="81"/>
        <source>Could not delete %1</source>
        <translation>Не удалось удалить %1</translation>
    </message>
</context>
<context>
    <name>Utils</name>
    <message>
        <location filename="../base/utils.cpp" line="375"/>
        <source>&lt;li&gt;Permissions insufficient for bootloader installation.
Administrator priviledges are necessary.&lt;/li&gt;</source>
        <translation>&lt;li&gt;Недостаточные полномочия для установки загрузчика.
Нужны полномочия администратора.&lt;/li&gt;</translation>
    </message>
    <message>
        <location filename="../base/utils.cpp" line="396"/>
        <source>Problem detected:</source>
        <translation>Обнаружена проблема:</translation>
    </message>
    <message>
        <location filename="../base/utils.cpp" line="387"/>
        <source>&lt;li&gt;Target mismatch detected.&lt;br/&gt;Installed target: %1&lt;br/&gt;Selected target: %2.&lt;/li&gt;</source>
        <translation>&lt;li&gt;Обнаруженно несовпадение устройств.&lt;br/&gt;Установленное устройство: %1&lt;br/&gt;Выбранное устройство : %2.&lt;/li&gt;</translation>
    </message>
</context>
<context>
    <name>VoiceFileCreator</name>
    <message>
        <location filename="../base/voicefile.cpp" line="43"/>
        <source>Starting Voicefile generation</source>
        <translation>Начинаю вырабатывание голосового файла</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="95"/>
        <source>Extracted voice corrections file from installation</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="100"/>
        <source>Using internal voice corrections file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="113"/>
        <source>Extracted language enumeration file from installation</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="234"/>
        <source>Download error: received HTTP error %1.</source>
        <translation>Ошибка скачивания : получена ошибка HTTP %1.</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="241"/>
        <source>Cached file used.</source>
        <translation>Использован файл из кэша.</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="244"/>
        <source>Download error: %1</source>
        <translation>Ошибка скачивания : %1</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="249"/>
        <source>Download finished.</source>
        <translation>Скачивание завершено.</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="262"/>
        <source>failed to open downloaded file</source>
        <translation>Сбой при открытии скаченного файла</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="324"/>
        <source>The downloaded file was empty!</source>
        <translation>Скачаный файл пуст!</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="355"/>
        <source>Error opening downloaded file</source>
        <translation>Сбой при открытии скаченного файла</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="366"/>
        <source>Error opening output file</source>
        <translation>Сбой при открытии выводного файла</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="390"/>
        <source>successfully created.</source>
        <translation>успешно создано.</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="56"/>
        <source>could not find rockbox-info.txt</source>
        <translation>Не удалось найти rockbox-info.txt</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="221"/>
        <source>Downloading voice info...</source>
        <translation>Получаю информацию о голосе...</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="268"/>
        <source>Reading strings...</source>
        <translation>Читаются значения...</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="350"/>
        <source>Creating voicefiles...</source>
        <translation>Создаются голосовые файлы...</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="399"/>
        <source>Cleaning up...</source>
        <translation>Навожу порядок...</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="410"/>
        <source>Finished</source>
        <translation>Всё</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="133"/>
        <source>Extracted voice strings from installation</source>
        <translation>Извлечены голосовые произношения</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="143"/>
        <source>Extracted voice strings incompatible</source>
        <translation>Извлечённые произношения несовместимы</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="194"/>
        <source>Could not retrieve strings from installation, downloading</source>
        <translation>Не удалось найти произношения в установке, скачиваются</translation>
    </message>
</context>
<context>
    <name>ZipInstaller</name>
    <message>
        <location filename="../base/zipinstaller.cpp" line="60"/>
        <source>done.</source>
        <translation>выполнено.</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="79"/>
        <source>Downloading file %1.%2</source>
        <translation>Скачивается файл %1.%2</translation>
    </message>
    <message>
        <source>Download error: received HTTP error %1.</source>
        <translation type="vanished">Сбой скачивания. Ошибка HTTP %1.</translation>
    </message>
    <message>
        <source>Cached file used.</source>
        <translation type="vanished">Используется файл из кэша.</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="121"/>
        <source>Download error: %1</source>
        <translation>Сбой скачивания: %1</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="129"/>
        <source>Download finished.</source>
        <translation>Скачивание завершено.</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="136"/>
        <source>Extracting file.</source>
        <translation>Извлечение файла.</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="169"/>
        <source>Installing file.</source>
        <translation>Установка файла.</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="181"/>
        <source>Installing file failed.</source>
        <translation>Сбой установки файла.</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="194"/>
        <source>Creating installation log</source>
        <translation>Создаю журнал установки</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="150"/>
        <source>Not enough disk space! Aborting.</source>
        <translation>Не достаточно дискового пространства! Отмена.</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="157"/>
        <source>Extraction failed!</source>
        <translation>Ошибка распаковки!</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="68"/>
        <source>Package installation finished successfully.</source>
        <translation>Установка файла успешно завершена.</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="114"/>
        <source>Download error: received HTTP error %1
%2</source>
        <translation type="unfinished">Сбой скачивания. Ошибка HTTP %1. {1
%2?}</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="126"/>
        <source>Download finished (cache used).</source>
        <translation type="unfinished">Скачивание завершено (из кэша).</translation>
    </message>
</context>
<context>
    <name>ZipUtil</name>
    <message>
        <location filename="../base/ziputil.cpp" line="125"/>
        <source>Creating output path failed</source>
        <translation>Ошибка создания выходной папки</translation>
    </message>
    <message>
        <location filename="../base/ziputil.cpp" line="132"/>
        <source>Creating output file failed</source>
        <translation>Ошибка создания выходного файла</translation>
    </message>
    <message>
        <location filename="../base/ziputil.cpp" line="141"/>
        <source>Error during Zip operation</source>
        <translation>Ошибка при выполнении операции с ZIP-пакетом</translation>
    </message>
</context>
<context>
    <name>aboutBox</name>
    <message>
        <location filename="../aboutbox.ui" line="14"/>
        <source>About Rockbox Utility</source>
        <translation>О мастере Rockbox</translation>
    </message>
    <message>
        <location filename="../aboutbox.ui" line="32"/>
        <source>The Rockbox Utility</source>
        <translation>Мастер Rockbox</translation>
    </message>
    <message>
        <location filename="../aboutbox.ui" line="74"/>
        <source>&amp;Credits</source>
        <translation>&amp;Благодарности</translation>
    </message>
    <message>
        <location filename="../aboutbox.ui" line="106"/>
        <source>&amp;License</source>
        <translation>&amp;Лицензия</translation>
    </message>
    <message>
        <location filename="../aboutbox.ui" line="158"/>
        <source>&amp;Ok</source>
        <translation>&amp;OK</translation>
    </message>
    <message>
        <location filename="../aboutbox.ui" line="54"/>
        <source>Installer and housekeeping utility for the Rockbox open source digital audio player firmware.&lt;br/&gt;© The Rockbox Team.&lt;br/&gt;Released under the GNU General Public License v2.&lt;br/&gt;Uses icons by the &lt;a href=&quot;http://tango.freedesktop.org/&quot;&gt;Tango Project&lt;/a&gt;.&lt;br/&gt;&lt;center&gt;&lt;a href=&quot;https://www.rockbox.org&quot;&gt;https://www.rockbox.org&lt;/a&gt;&lt;/center&gt;</source>
        <translation>Мастер установки и управления Rockbox, микропрограммы с открытым исходным кодом для цифровых аудиоплееров.&lt;br/&gt;© Команда Rockbox.&lt;br/&gt;Раздаётся по лицензии GNU General Public License v2.&lt;br/&gt;Используются иконки из &lt;a href=&quot;http://tango.freedesktop.org/&quot;&gt;проекта Tango&lt;/a&gt;.&lt;br/&gt;&lt;center&gt;&lt;a href=&quot;https://www.rockbox.org&quot;&gt;https://www.rockbox.org&lt;/a&gt;&lt;/center&gt;</translation>
    </message>
    <message>
        <source>&amp;Speex License</source>
        <translation type="vanished">&amp;Лицензия Speex</translation>
    </message>
    <message>
        <location filename="../aboutbox.ui" line="132"/>
        <source>L&amp;ibraries</source>
        <translation type="unfinished"></translation>
    </message>
</context>
</TS>
