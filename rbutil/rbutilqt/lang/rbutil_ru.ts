<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.0" language="ru">
<context>
    <name>BootloaderInstallAms</name>
    <message>
        <source>Downloading bootloader file</source>
        <translation>Скачиваю файл загрузчика</translation>
    </message>
    <message>
        <source>Could not load %1</source>
        <translation>Не могу загрузить %1</translation>
    </message>
    <message>
        <source>Patching Firmware...</source>
        <translation>Исправляю прошивку...</translation>
    </message>
    <message>
        <source>Could not open %1 for writing</source>
        <translation>Не могу открыть %1 для записи</translation>
    </message>
    <message>
        <source>Could not write firmware file</source>
        <translation>Сбой записи файла прошивки</translation>
    </message>
    <message>
        <source>Success: modified firmware file created</source>
        <translation>Изменённая прошивка успешно создана</translation>
    </message>
    <message>
        <source>No room to insert bootloader, try another firmware version</source>
        <translation>Нет места для записи загрузчика, попробуйте другую версию прошивки</translation>
    </message>
    <message>
        <source>To uninstall, perform a normal upgrade with an unmodified original firmware</source>
        <translation>Для удаления выполните нормальное обновление с неизменённой фирменной прошивкой</translation>
    </message>
    <message>
        <source>Bootloader installation requires you to provide a copy of the original Sandisk firmware (bin file). This firmware file will be patched and then installed to your player along with the rockbox bootloader. You need to download this file yourself due to legal reasons. Please browse the &lt;a href=&apos;http://forums.sandisk.com/sansa/&apos;&gt;Sansa Forums&apos;&lt;/a&gt; or refer to the &lt;a href=&apos;http://www.rockbox.org/manual.shtml&apos;&gt;manual&lt;/a&gt; and the &lt;a href=&apos;http://www.rockbox.org/wiki/SansaAMS&apos;&gt;SansaAMS&lt;/a&gt; wiki page on how to obtain this file.&lt;br/&gt;Press Ok to continue and browse your computer for the firmware file.</source>
        <translation>Установка загрузчика потребует от вас копию оригинальной прошивки Sandisk\&apos;а (bin файл). Эта прошивка будет пропатчена и затем установлена в ваш плеер вместе с зарузчиком Rockbox\&apos;а. По причинам легальности данного действия вам нужно будет самим скачать загрузчик. Зайдите на &lt;a href=\&apos;http://forums.sandisk.com/sansa/\&apos;&gt;Sansa Forums\&apos;&lt;/a&gt; или обратитесь к &lt;a href=\&apos;http://www.rockbox.org/manual.shtml\&apos;&gt;инструкции&lt;/a&gt; и вики-странице &lt;a href=\&apos;http://www.rockbox.org/wiki/SansaAMS\&apos;&gt;SansaAMS&lt;/a&gt; за помощью с получением файла.&lt;br/&gt;Нажмите ОК чтобы продолжить и выбрать файл прошивки.</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallBase</name>
    <message>
        <source>Download error: received HTTP error %1.</source>
        <translation>Ошибка при скачивании: ошибка HTTP %1.</translation>
    </message>
    <message>
        <source>Download error: %1</source>
        <translation>Ошибка при скачивании: %1</translation>
    </message>
    <message>
        <source>Download finished (cache used).</source>
        <translation>Скачивание завершено (из кэша).</translation>
    </message>
    <message>
        <source>Download finished.</source>
        <translation>Скачивание завершено.</translation>
    </message>
    <message>
        <source>Creating backup of original firmware file.</source>
        <translation>Создаётся резервная копия фирменной прошивки.</translation>
    </message>
    <message>
        <source>Creating backup folder failed</source>
        <translation>Сбой при попытке создания папки резервной копии</translation>
    </message>
    <message>
        <source>Creating backup copy failed.</source>
        <translation>Сбой при попытке создания резервной копии.</translation>
    </message>
    <message>
        <source>Backup created.</source>
        <translation>Резервная копия создана.</translation>
    </message>
    <message>
        <source>Creating installation log</source>
        <translation>Создаётся журнал установки</translation>
    </message>
    <message>
        <source>Bootloader installation is almost complete. Installation &lt;b&gt;requires&lt;/b&gt; you to perform the following steps manually:</source>
        <translation>Установка загрузчика почти завершена. Вам &lt;b&gt;протребуется&lt;/b&gt; выполнить следующие операции вручную:</translation>
    </message>
    <message>
        <source>&lt;li&gt;Safely remove your player.&lt;/li&gt;</source>
        <translation>&lt;li&gt;Отключить плеер от компьютера с использованием безопасного извлечения.&lt;/li&gt;</translation>
    </message>
    <message>
        <source>&lt;li&gt;Turn the player off&lt;/li&gt;&lt;li&gt;Insert the charger&lt;/li&gt;</source>
        <translation>&lt;li&gt;Выключить плеер&lt;/li&gt;&lt;li&gt;Подключить зарядное устройство&lt;/li&gt;</translation>
    </message>
    <message>
        <source>&lt;li&gt;Unplug USB and power adaptors&lt;/li&gt;&lt;li&gt;Hold &lt;i&gt;Power&lt;/i&gt; to turn the player off&lt;/li&gt;&lt;li&gt;Toggle the battery switch on the player&lt;/li&gt;&lt;li&gt;Hold &lt;i&gt;Power&lt;/i&gt; to boot into Rockbox&lt;/li&gt;</source>
        <translation>&lt;li&gt;Отключить плеер от USB и сетевого питания&lt;/li&gt;&lt;li&gt;Выключить плеер&lt;/li&gt;&lt;li&gt;Переключить плеер в режим питания от батареи&lt;/li&gt;&lt;li&gt;Обратно включить плеер для загрузки Rockbox&lt;/li&gt;</translation>
    </message>
    <message>
        <source>&lt;p&gt;&lt;b&gt;Note:&lt;/b&gt; You can safely install other parts first, but the above steps are &lt;b&gt;required&lt;/b&gt; to finish the installation!&lt;/p&gt;</source>
        <translation>&lt;p&gt;&lt;b&gt;Примечание:&lt;/b&gt; Вы можете безопасно устанавливать дополнения, но выше указанные операции &lt;b&gt;обязательны&lt;/b&gt; для завершения установки!&lt;/p&gt;</translation>
    </message>
    <message>
        <source>Installation log created</source>
        <translation>Журнал установки создан</translation>
    </message>
    <message>
        <source>Waiting for system to remount player</source>
        <translation>Ожидание, пока система заново смонтирует плеер</translation>
    </message>
    <message>
        <source>Player remounted</source>
        <translation>Плеер смонтирован</translation>
    </message>
    <message>
        <source>Timeout on remount</source>
        <translation>Таймаут ожидания для монтирования</translation>
    </message>
    <message>
        <source>&lt;li&gt;Reboot your player into the original firmware.&lt;/li&gt;&lt;li&gt;Perform a firmware upgrade using the update functionality of the original firmware. Please refer to your player&apos;s manual on details.&lt;br/&gt;&lt;b&gt;Important:&lt;/b&gt; updating the firmware is a critical process that must not be interrupted. &lt;b&gt;Make sure the player is charged before starting the firmware update process.&lt;/b&gt;&lt;/li&gt;&lt;li&gt;After the firmware has been updated reboot your player.&lt;/li&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>&lt;li&gt;Remove any previously inserted microSD card&lt;/li&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>&lt;li&gt;Disconnect your player. The player will reboot and perform an update of the original firmware. Please refer to your players manual on details.&lt;br/&gt;&lt;b&gt;Important:&lt;/b&gt; updating the firmware is a critical process that must not be interrupted. &lt;b&gt;Make sure the player is charged before disconnecting the player.&lt;/b&gt;&lt;/li&gt;&lt;li&gt;After the firmware has been updated reboot your player.&lt;/li&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Zip file format detected</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Extracting firmware %1 from archive</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Error extracting firmware from archive</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Could not find firmware in archive</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>BootloaderInstallChinaChip</name>
    <message>
        <source>Bootloader installation requires you to provide a firmware file of the original firmware (HXF file). You need to download this file yourself due to legal reasons. Please refer to the &lt;a href=&apos;http://www.rockbox.org/manual.shtml&apos;&gt;manual&lt;/a&gt; and the &lt;a href=&apos;http://www.rockbox.org/wiki/OndaVX747#Download_and_extract_a_recent_ve&apos;&gt;OndaVX747&lt;/a&gt; wiki page on how to obtain this file.&lt;br/&gt;Press Ok to continue and browse your computer for the firmware file.</source>
        <translation>Чтобы установить загрузчик, вам необходимо предоставить файл фирменной прошивки (HXF-файл). По законодательным причинам, этот файл вам необходимо скачать вручную. О том, как получить этот файл, смотрите в &lt;a href=&apos;http://www.rockbox.org/manual.shtml&apos;&gt;руководстве по эксплуатации&lt;/a&gt; и на &lt;a href=&apos;http://www.rockbox.org/wiki/OndaVX747#Download_and_extract_a_recent_ve&apos;&gt;wiki-странице OndaVX747&lt;/a&gt;. &lt;br/&gt; Нажмине на ОК, чтобы продолжить и указать путь к прошивке на Вашем компьютере.</translation>
    </message>
    <message>
        <source>Downloading bootloader file</source>
        <translation>Скачивается файл загрузчика</translation>
    </message>
    <message>
        <source>Could not open firmware file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Could not open bootloader file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Could not allocate memory</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Could not load firmware file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>File is not a valid ChinaChip firmware</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Could not find ccpmp.bin in input file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Could not open backup file for ccpmp.bin</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Could not write backup file for ccpmp.bin</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Could not load bootloader file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Could not get current time</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Could not open output file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Could not write output file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Unexpected error from chinachippatcher</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>BootloaderInstallFile</name>
    <message>
        <source>Downloading bootloader</source>
        <translation>Скачивается загрузчик</translation>
    </message>
    <message>
        <source>Installing Rockbox bootloader</source>
        <translation>Устанавливается загрузчик Rockbox</translation>
    </message>
    <message>
        <source>Error accessing output folder</source>
        <translation>Ошибка доступа к выходной папке</translation>
    </message>
    <message>
        <source>Removing Rockbox bootloader</source>
        <translation>Удаляется загрузчик Rockbox</translation>
    </message>
    <message>
        <source>No original firmware file found.</source>
        <translation>Не найдено фирменной прошивки.</translation>
    </message>
    <message>
        <source>Can&apos;t remove Rockbox bootloader file.</source>
        <translation>Не могу удалить файл загрузчика Rockbox.</translation>
    </message>
    <message>
        <source>Can&apos;t restore bootloader file.</source>
        <translation>Не могу восстановить файл загрузчика.</translation>
    </message>
    <message>
        <source>Original bootloader restored successfully.</source>
        <translation>Фирменный загрузчик успешно восстановлен.</translation>
    </message>
    <message>
        <source>Bootloader successful installed</source>
        <translation>Загрузчик успешно установлен</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallHex</name>
    <message>
        <source>checking MD5 hash of input file ...</source>
        <translation>проверка хэша MD5 входного файла ...</translation>
    </message>
    <message>
        <source>Could not verify original firmware file</source>
        <translation>Не могу проверить фирменную прошивку</translation>
    </message>
    <message>
        <source>Firmware file not recognized.</source>
        <translation>Файл прошивки не распознан.</translation>
    </message>
    <message>
        <source>MD5 hash ok</source>
        <translation>Хэш MD5 проверен</translation>
    </message>
    <message>
        <source>Firmware file doesn&apos;t match selected player.</source>
        <translation>Прошивка не соответствует указанному плееру.</translation>
    </message>
    <message>
        <source>Descrambling file</source>
        <translation>Расшифровка файла</translation>
    </message>
    <message>
        <source>Error in descramble: %1</source>
        <translation>Ошибка при расшифровке: %1</translation>
    </message>
    <message>
        <source>Downloading bootloader file</source>
        <translation>Скачиваю файл загрузчика</translation>
    </message>
    <message>
        <source>Adding bootloader to firmware file</source>
        <translation>Добавляется загрузчик к прошивке</translation>
    </message>
    <message>
        <source>could not open input file</source>
        <translation>Не могу открыть входной файл</translation>
    </message>
    <message>
        <source>reading header failed</source>
        <translation>Сбой чтения заголовка</translation>
    </message>
    <message>
        <source>reading firmware failed</source>
        <translation>Сбой чтения прошивки</translation>
    </message>
    <message>
        <source>can&apos;t open bootloader file</source>
        <translation>Не могу открыть файл загрузчика</translation>
    </message>
    <message>
        <source>reading bootloader file failed</source>
        <translation>Сбой чтения файла загрузчика</translation>
    </message>
    <message>
        <source>can&apos;t open output file</source>
        <translation>Не могу открыть выходной файл</translation>
    </message>
    <message>
        <source>writing output file failed</source>
        <translation>Сбой записи выходного файла</translation>
    </message>
    <message>
        <source>Error in patching: %1</source>
        <translation>Ошибка применения патча: %1</translation>
    </message>
    <message>
        <source>Error in scramble: %1</source>
        <translation>Ошибка кодирования: %1</translation>
    </message>
    <message>
        <source>Checking modified firmware file</source>
        <translation>Проверка изменённой прошивки</translation>
    </message>
    <message>
        <source>Error: modified file checksum wrong</source>
        <translation>Ошибка: неверная контрольная сумма изменённого файла</translation>
    </message>
    <message>
        <source>Success: modified firmware file created</source>
        <translation>Изменённая прошивка успешно создана</translation>
    </message>
    <message>
        <source>Can&apos;t open input file</source>
        <translation>Не могу открыть входной файл</translation>
    </message>
    <message>
        <source>Can&apos;t open output file</source>
        <translation>Не могу открыть выходной вайл</translation>
    </message>
    <message>
        <source>invalid file: header length wrong</source>
        <translation>Неверный файл: неверная длина заголовка</translation>
    </message>
    <message>
        <source>invalid file: unrecognized header</source>
        <translation>Неверный файл: неопознанный заголовок</translation>
    </message>
    <message>
        <source>invalid file: &quot;length&quot; field wrong</source>
        <translation>Неверный файл: неверное поле &quot;длина&quot;</translation>
    </message>
    <message>
        <source>invalid file: &quot;length2&quot; field wrong</source>
        <translation>Неверный файл: неверное поле &quot;длина2&quot;</translation>
    </message>
    <message>
        <source>invalid file: internal checksum error</source>
        <translation>Неверный файл: ошибка во внутренней контрольной сумме</translation>
    </message>
    <message>
        <source>unknown</source>
        <translation>неизвестная ошибка</translation>
    </message>
    <message>
        <source>Bootloader installation requires you to provide a firmware file of the original firmware (hex file). You need to download this file yourself due to legal reasons. Please refer to the &lt;a href=&apos;http://www.rockbox.org/manual.shtml&apos;&gt;manual&lt;/a&gt; and the &lt;a href=&apos;http://www.rockbox.org/wiki/IriverBoot#Download_and_extract_a_recent_ve&apos;&gt;IriverBoot&lt;/a&gt; wiki page on how to obtain this file.&lt;br/&gt;Press Ok to continue and browse your computer for the firmware file.</source>
        <translation>Для установки загрузчика вам необходимо предоставить файл прошивки с фирменной прошивкой (hex-файл). По законодательным причинам, вам необходимо скачать этот файл вручную. Как найти этот файл, смотрите в &lt;a href=&apos;http://www.rockbox.org/manual.shtml&apos;&gt;руководстве по экслуатации&lt;/a&gt; и на вики-странице &lt;a href=&apos;http://www.rockbox.org/wiki/IriverBoot#Download_and_extract_a_recent_ve&apos;&gt;IriverBoot&lt;/a&gt;.&lt;br/&gt;Чтобы продолжить, нажмите на OK и укажите файл прошивки на компьютере.</translation>
    </message>
    <message>
        <source>invalid file: &quot;length3&quot; field wrong</source>
        <translation>Неверный файл: неверное поле &quot;длина3&quot;</translation>
    </message>
    <message>
        <source>Uninstallation not possible, only installation info removed</source>
        <translation>Полное удаление невозможно, удалена только информация об установке</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallImx</name>
    <message>
        <source>Bootloader installation requires you to provide a copy of the original Sandisk firmware (firmware.sb file). This file will be patched with the Rockbox bootloader and installed to your player. You need to download this file yourself due to legal reasons. Please browse the &lt;a href=&apos;http://forums.sandisk.com/sansa/&apos;&gt;Sansa Forums&lt;/a&gt; or refer to the &lt;a href= &apos;http://www.rockbox.org/wiki/SansaFuzePlus&apos;&gt;SansaFuzePlus&lt;/a&gt; wiki page on how to obtain this file.&lt;br/&gt;Press Ok to continue and browse your computer for the firmware file.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Could not read original firmware file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Downloading bootloader file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Patching file...</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Patching the original firmware failed</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Succesfully patched firmware file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Bootloader successful installed</source>
        <translation type="unfinished">Загрузчик успешно установлен</translation>
    </message>
    <message>
        <source>Patched bootloader could not be installed</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>To uninstall, perform a normal upgrade with an unmodified original firmware.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>BootloaderInstallIpod</name>
    <message>
        <source>Error: can&apos;t allocate buffer memory!</source>
        <translation>Ошибка: не могу выделить буферную память!</translation>
    </message>
    <message>
        <source>Downloading bootloader file</source>
        <translation>Скачиваю файл заргузчика</translation>
    </message>
    <message>
        <source>Failed to read firmware directory</source>
        <translation>Сбой чтения папки микропрограммы</translation>
    </message>
    <message>
        <source>Unknown version number in firmware (%1)</source>
        <translation>Неизвестный номер версии прошивки (%1)</translation>
    </message>
    <message>
        <source>Could not open Ipod in R/W mode</source>
        <translation>Не могу открыть iPod в режиме записи</translation>
    </message>
    <message>
        <source>Failed to add bootloader</source>
        <translation>Сбой установки загрузчика</translation>
    </message>
    <message>
        <source>No bootloader detected.</source>
        <translation>Загрузчика не обнаружено.</translation>
    </message>
    <message>
        <source>Successfully removed bootloader</source>
        <translation>Загрузчик успешно удалён</translation>
    </message>
    <message>
        <source>Removing bootloader failed.</source>
        <translation>Сбой удаления загрузчика.</translation>
    </message>
    <message>
        <source>Could not open Ipod</source>
        <translation>Не могу открыть iPod</translation>
    </message>
    <message>
        <source>No firmware partition on disk</source>
        <translation>Не найдено раздела прошивки на диске</translation>
    </message>
    <message>
        <source>Installing Rockbox bootloader</source>
        <translation>Установка загрузчика Rockbox</translation>
    </message>
    <message>
        <source>Uninstalling bootloader</source>
        <translation>Удаление загрузчика</translation>
    </message>
    <message>
        <source>Error reading partition table - possibly not an Ipod</source>
        <translation>Сбой чтения таблицы разделов - возможно, это не iPod</translation>
    </message>
    <message>
        <source>Warning: This is a MacPod, Rockbox only runs on WinPods. 
See http://www.rockbox.org/wiki/IpodConversionToFAT32</source>
        <translation>Предупреждение: это - MacPod, Rockbox работает только на WinPod&apos;ах.
См. http://www.rockbox.org/wiki/IpodConversionToFAT32</translation>
    </message>
    <message>
        <source>Successfull added bootloader</source>
        <translation>Загрузчик успешно добавлен</translation>
    </message>
    <message>
        <source>Bootloader Installation complete.</source>
        <translation>Установка загрузчика завершена.</translation>
    </message>
    <message>
        <source>Writing log aborted</source>
        <translation>Запись журнала отменена</translation>
    </message>
    <message>
        <source>Error: no mountpoint specified!</source>
        <translation>Ошибка: не указано точки монтирования!</translation>
    </message>
    <message>
        <source>Could not open Ipod: permission denied</source>
        <translation>Не могу открыть iPod: доступ запрещён</translation>
    </message>
    <message>
        <source>Error: could not retrieve device name</source>
        <translation>Ошибка: не могу найти имя устройства</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallMi4</name>
    <message>
        <source>Downloading bootloader</source>
        <translation>Скачиваю загрузчик</translation>
    </message>
    <message>
        <source>Installing Rockbox bootloader</source>
        <translation>Устанавливается загрузчик Rockbox</translation>
    </message>
    <message>
        <source>Bootloader successful installed</source>
        <translation>Загрузчик успешно установлен</translation>
    </message>
    <message>
        <source>Checking for Rockbox bootloader</source>
        <translation>Проверяется наличие загрузчика Rockbox</translation>
    </message>
    <message>
        <source>No Rockbox bootloader found</source>
        <translation>Загрузчик Rockbox не найден</translation>
    </message>
    <message>
        <source>Checking for original firmware file</source>
        <translation>Проверяется наличие фирменной прошивки</translation>
    </message>
    <message>
        <source>Error finding original firmware file</source>
        <translation>Фирменной прошивки не найдено</translation>
    </message>
    <message>
        <source>Rockbox bootloader successful removed</source>
        <translation>Загрузчик Rockbox успешно удалён</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallMpio</name>
    <message>
        <source>Bootloader installation requires you to provide a firmware file of the original firmware (bin file). You need to download this file yourself due to legal reasons. Please refer to the &lt;a href=&apos;http://www.rockbox.org/manual.shtml&apos;&gt;manual&lt;/a&gt; and the &lt;a href=&apos;http://www.rockbox.org/wiki/MPIOHD200Port&apos;&gt;MPIOHD200Port&lt;/a&gt; wiki page on how to obtain this file.&lt;br/&gt;Press Ok to continue and browse your computer for the firmware file.</source>
        <translation>Для установки загрузчика вам необходимо предоставить файл прошивки с фирменной микропрограммой (hex-файл). По законодательным причинам, вам необходимо скачать этот файл вручную. Как найти этот файл, смотрите в &lt;a href=&apos;http://www.rockbox.org/manual.shtml&apos;&gt;руководстве по экслуатации&lt;/a&gt; и на вики-странице &lt;a href=&apos;http://www.rockbox.org/wiki/MPIOHD200Port&apos;&gt;MPIOHD200Port&lt;/a&gt;.&lt;br/&gt;Чтобы продолжить, нажмите на OK и укажите файл прошивки на компьютере.</translation>
    </message>
    <message>
        <source>Downloading bootloader file</source>
        <translation>Скачивается файл загрузчика</translation>
    </message>
    <message>
        <source>Could not open the original firmware.</source>
        <translation>Не могу открыть фирменную прошивку.</translation>
    </message>
    <message>
        <source>Could not read the original firmware.</source>
        <translation>Не могу прочитать фирменную прошивку.</translation>
    </message>
    <message>
        <source>Could not open downloaded bootloader.</source>
        <translation>Не могу открыть скачаный загрузчик.</translation>
    </message>
    <message>
        <source>Place for bootloader in OF file not empty.</source>
        <translation>Место для загрузчика в фирменной прошивке не пустое.</translation>
    </message>
    <message>
        <source>Could not read the downloaded bootloader.</source>
        <translation>Не могу прочитать скачаный загрузчик.</translation>
    </message>
    <message>
        <source>Bootloader checksum error.</source>
        <translation>Ошибка в контрольной сумме загрузчика.</translation>
    </message>
    <message>
        <source>Patching original firmware failed: %1</source>
        <translation>Сбой патчирования фирменной прошивки: %1</translation>
    </message>
    <message>
        <source>Success: modified firmware file created</source>
        <translation>Изменённая прошивка успешно создана</translation>
    </message>
    <message>
        <source>To uninstall, perform a normal upgrade with an unmodified original firmware</source>
        <translation>Для удаления выполните нормальное обновление с неизменённой фирменной прошивкой</translation>
    </message>
    <message>
        <source>Loaded firmware file does not look like MPIO original firmware file.</source>
        <translation>Загруженная прошивка не похожа на фирменную прошивку MPIO.</translation>
    </message>
    <message>
        <source>Could not open output file.</source>
        <translation>Не могу открыть выходной вайл.</translation>
    </message>
    <message>
        <source>Could not write output file.</source>
        <translation>Сбой записи выходного файла.</translation>
    </message>
    <message>
        <source>Unknown error number: %1</source>
        <translation>Неизвестная ошибка номер %1</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallSansa</name>
    <message>
        <source>Error: can&apos;t allocate buffer memory!</source>
        <translation>Ошибка: не могу выделить буферную память!</translation>
    </message>
    <message>
        <source>Searching for Sansa</source>
        <translation>Поиск плеера Sansa</translation>
    </message>
    <message>
        <source>Permission for disc access denied!
This is required to install the bootloader</source>
        <translation>Доступ к диску отказан!
Это необходимо для установки загрузчика</translation>
    </message>
    <message>
        <source>No Sansa detected!</source>
        <translation>Не найдено плеера Sansa!</translation>
    </message>
    <message>
        <source>Downloading bootloader file</source>
        <translation>Скачиваю файл загрузчика</translation>
    </message>
    <message>
        <source>OLD ROCKBOX INSTALLATION DETECTED, ABORTING.
You must reinstall the original Sansa firmware before running
sansapatcher for the first time.
See http://www.rockbox.org/wiki/SansaE200Install
</source>
        <translation>ОБНАРУЖЕНА СТАРАЯ УСТАНОВКА ROCKBOX, ОПЕРАЦИЯ ОТМЕНЯЕТСЯ.
Вам необходимо переустановить фирменную микропрограмму 
вашего плеера перед первым запуском sansapatcher&apos;а.
См. http://www.rockbox.org/wiki/SansaE200Install</translation>
    </message>
    <message>
        <source>Could not open Sansa in R/W mode</source>
        <translation>Не могу открыть плеер в режиме записи</translation>
    </message>
    <message>
        <source>Successfully installed bootloader</source>
        <translation>Загрузчик успешно установлен</translation>
    </message>
    <message>
        <source>Failed to install bootloader</source>
        <translation>Сбой при установке загрузчика</translation>
    </message>
    <message>
        <source>Can&apos;t find Sansa</source>
        <translation>Не могу найти плеер Sansa</translation>
    </message>
    <message>
        <source>Could not open Sansa</source>
        <translation>Не могу открыть плеер Sansa</translation>
    </message>
    <message>
        <source>Could not read partition table</source>
        <translation>Не могу прочитать таблицу разделов</translation>
    </message>
    <message>
        <source>Disk is not a Sansa (Error %1), aborting.</source>
        <translation>Диск не принадлежит плееру Sansa (Ошибка %1), отмена.</translation>
    </message>
    <message>
        <source>Successfully removed bootloader</source>
        <translation>Загручик успешно удалён</translation>
    </message>
    <message>
        <source>Removing bootloader failed.</source>
        <translation>Сбой удаления загрузчика.</translation>
    </message>
    <message>
        <source>Installing Rockbox bootloader</source>
        <translation>Установка загрузчика Rockbox</translation>
    </message>
    <message>
        <source>Checking downloaded bootloader</source>
        <translation>Проверка скаченного загрузчика</translation>
    </message>
    <message>
        <source>Bootloader mismatch! Aborting.</source>
        <translation>Загрузчик не соответствует! Отмена.</translation>
    </message>
    <message>
        <source>Uninstalling bootloader</source>
        <translation>Удаление загрузчика</translation>
    </message>
    <message>
        <source>Bootloader Installation complete.</source>
        <translation>Установка загрузчика завершена.</translation>
    </message>
    <message>
        <source>Writing log aborted</source>
        <translation>Запись журнала отменена</translation>
    </message>
    <message>
        <source>Error: could not retrieve device name</source>
        <translation>Ошибка: не могу найти имя устройства</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallTcc</name>
    <message>
        <source>Downloading bootloader file</source>
        <translation>Скачиваю файл загрузчика</translation>
    </message>
    <message>
        <source>Could not load %1</source>
        <translation>Не могу загрузить %1</translation>
    </message>
    <message>
        <source>Unknown OF file used: %1</source>
        <translation>Используется неизвестный файл фирменной прошивки: %1</translation>
    </message>
    <message>
        <source>Patching Firmware...</source>
        <translation>Исправляю прошивку...</translation>
    </message>
    <message>
        <source>Could not open %1 for writing</source>
        <translation>Не могу открыть %1 для записи</translation>
    </message>
    <message>
        <source>Could not write firmware file</source>
        <translation>Сбой записи файла прошиви</translation>
    </message>
    <message>
        <source>Success: modified firmware file created</source>
        <translation>Изменённый файл прошивки успешно создан</translation>
    </message>
    <message>
        <source>Bootloader installation requires you to provide a firmware file of the original firmware (bin file). You need to download this file yourself due to legal reasons. Please refer to the &lt;a href=&apos;http://www.rockbox.org/manual.shtml&apos;&gt;manual&lt;/a&gt; and the &lt;a href=&apos;http://www.rockbox.org/wiki/CowonD2Info&apos;&gt;CowonD2Info&lt;/a&gt; wiki page on how to obtain the file.&lt;br/&gt;Press Ok to continue and browse your computer for the firmware file.</source>
        <translation>Для установки загрузчика вам необходимо предоставить файл прошивки с фирменной микропрограммой (hex-файл). По законодательным причинам, вам необходимо скачать этот файл вручную. Как найти этот файл, смотрите в &lt;a href=&apos;http://www.rockbox.org/manual.shtml&apos;&gt;руководстве по экслуатации&lt;/a&gt; и на вики-странице &lt;a href=&apos;http://www.rockbox.org/wiki/CowonD2Info&apos;&gt;CowonD2Info&lt;/a&gt;.&lt;br/&gt;Чтобы продолжить, нажмите на OK и укажите файл прошивки на компьютере.</translation>
    </message>
    <message>
        <source>Could not patch firmware</source>
        <translation>Сбой записи файла прошивки</translation>
    </message>
    <message>
        <source>To uninstall, perform a normal upgrade with an unmodified original firmware</source>
        <translation>Для удаления выполните нормальное обновление с неизменённой фирменной прошивкой</translation>
    </message>
</context>
<context>
    <name>Config</name>
    <message>
        <source>Autodetection</source>
        <translation>Автоопределение</translation>
    </message>
    <message>
        <source>Could not detect a Mountpoint.
Select your Mountpoint manually.</source>
        <translation>Не могу определить точку монтирования
Укажите её вручную.</translation>
    </message>
    <message>
        <source>Could not detect a device.
Select your device and Mountpoint manually.</source>
        <translation>Не могу определить устройство.
Укажите устройство и точку монтирования вручную.</translation>
    </message>
    <message>
        <source>Really delete cache?</source>
        <translation>Удалить кэш?</translation>
    </message>
    <message>
        <source>Do you really want to delete the cache? Make absolutely sure this setting is correct as it will remove &lt;b&gt;all&lt;/b&gt; files in this folder!</source>
        <translation>Вы дейсвительно хотите удалить кэш? Эта операция удалит &lt;b&gt;все&lt;/b&gt; файлы из этой папки!</translation>
    </message>
    <message>
        <source>Path wrong!</source>
        <translation>Неверный путь!</translation>
    </message>
    <message>
        <source>The cache path is invalid. Aborting.</source>
        <translation>Неверный путь кэша. Отмена.</translation>
    </message>
    <message>
        <source>Fatal error</source>
        <translation>Фатальная ошибка</translation>
    </message>
    <message>
        <source>Current cache size is %L1 kiB.</source>
        <translation>Текущий размер кэша %L1 КБ.</translation>
    </message>
    <message>
        <source>Configuration OK</source>
        <translation>Настройки верны</translation>
    </message>
    <message>
        <source>Configuration INVALID</source>
        <translation>Настройки НЕВЕРНЫ</translation>
    </message>
    <message>
        <source>The following errors occurred:</source>
        <translation>Обнаружены следующие ошибки:</translation>
    </message>
    <message>
        <source>No mountpoint given</source>
        <translation>Не указана точка монтирования</translation>
    </message>
    <message>
        <source>Mountpoint does not exist</source>
        <translation>Точка монтирования не существует</translation>
    </message>
    <message>
        <source>Mountpoint is not a directory.</source>
        <translation>Указанная точка монтирования не является папкой.</translation>
    </message>
    <message>
        <source>Mountpoint is not writeable</source>
        <translation>Точка монтирования незаписываема</translation>
    </message>
    <message>
        <source>No player selected</source>
        <translation>Плеер не выбран</translation>
    </message>
    <message>
        <source>Cache path not writeable. Leave path empty to default to systems temporary path.</source>
        <translation>Указанный путь кэша незаписываем. Оставьте это поле пустым для использования системного пути по умолчанию.</translation>
    </message>
    <message>
        <source>You need to fix the above errors before you can continue.</source>
        <translation>Вам необходимо исправить вышеуказанные ошибкм перед тем, как продолжить.</translation>
    </message>
    <message>
        <source>Configuration error</source>
        <translation>Ошибка в настройках</translation>
    </message>
    <message>
        <source>Detected an unsupported player:
%1
Sorry, Rockbox doesn&apos;t run on your player.</source>
        <translation>Обнаружен неподдерживаемый плеер:
%1
К сожалению, Rockbox не работает на этом плеере.</translation>
    </message>
    <message>
        <source>Fatal: player incompatible</source>
        <translation>Ошибка: плеер несовместим</translation>
    </message>
    <message>
        <source>TTS configuration invalid</source>
        <translation>Настройки TTS неверны</translation>
    </message>
    <message>
        <source>TTS configuration invalid. 
 Please configure TTS engine.</source>
        <translation>Настройки TTS неверны.
 Пожалуйста, настройте движок TTS.</translation>
    </message>
    <message>
        <source>Could not start TTS engine.</source>
        <translation>Не могу запустить движок TTS.</translation>
    </message>
    <message>
        <source>Could not start TTS engine.
</source>
        <translation>Не могу запустить движок TTS.
</translation>
    </message>
    <message>
        <source>
Please configure TTS engine.</source>
        <translation>
Пожалуйста, настройте движок TTS.</translation>
    </message>
    <message>
        <source>Rockbox Utility Voice Test</source>
        <translation>Проверка голоса</translation>
    </message>
    <message>
        <source>Could not voice test string.</source>
        <translation>Невозможно озвучить введённый текст.</translation>
    </message>
    <message>
        <source>Showing disabled targets</source>
        <translation>Отображение отображение неподдерживаемых устройств</translation>
    </message>
    <message>
        <source>You just enabled showing targets that are marked disabled. Disabled targets are not recommended to end users. Please use this option only if you know what you are doing.</source>
        <translation>Вы только что указали, что хотите видеть в списке устройства, помеченные как официально неподдерживаемые. Таковые не рекомендуются к использованию конечным пользователям. Используйте эту функцию только если знаете, что делаете.</translation>
    </message>
    <message>
        <source>Set Cache Path</source>
        <translation>Указать путь к кэшу</translation>
    </message>
    <message>
        <source>%1 &quot;MacPod&quot; found!
Rockbox needs a FAT formatted Ipod (so-called &quot;WinPod&quot;) to run. </source>
        <translation>%1 является MacPod&apos;ом!
Для работы, Rockbox нужен iPod форматированный в FAT (так называемый &quot;WinPod&quot;).</translation>
    </message>
    <message>
        <source>Proxy Detection</source>
        <translation>Определение прокси</translation>
    </message>
    <message>
        <source>The System Proxy settings are invalid!
Rockbox Utility can&apos;t work with this proxy settings. Make sure the system proxy is set correctly. Note that &quot;proxy auto-config (PAC)&quot; scripts are not supported by Rockbox Utility. If your system uses this you need to use manual proxy settings.</source>
        <translation>Системные настройки прокси неверны!
Мастер Rockbox не может работать с этими настройками. Проверьте правильность системных настроек прокси. Учтите, что мастер Rockbox не поддерживает сценарии &quot;proxy auto config&quot; (PAC). Если таковые используются на Вашей системе, вам необходимо использовать ручные настройки.</translation>
    </message>
    <message>
        <source>%1 in MTP mode found!
You need to change your player to MSC mode for installation. </source>
        <translation>Найден %1 в режиме MTP!
Для установки вам нужно сменить режим подключения плеера на MSC.</translation>
    </message>
    <message>
        <source>Until you change this installation will fail!</source>
        <translation>Пока вы это не измените, установка не пройдёт успешно!</translation>
    </message>
    <message>
        <source>Could not voice test string.
</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>ConfigForm</name>
    <message>
        <source>Configuration</source>
        <translation>Настройки</translation>
    </message>
    <message>
        <source>Configure Rockbox Utility</source>
        <translation>Настроить мастера Rockbox</translation>
    </message>
    <message>
        <source>&amp;Device</source>
        <translation>&amp;Устройство</translation>
    </message>
    <message>
        <source>Select your device in the &amp;filesystem</source>
        <translation>Укажите Ваше устройство в &amp;файловой системе</translation>
    </message>
    <message>
        <source>&amp;Browse</source>
        <translation>&amp;Обзор</translation>
    </message>
    <message>
        <source>&amp;Select your audio player</source>
        <translation>&amp;Выберите ваш аудио плеер</translation>
    </message>
    <message>
        <source>&amp;Autodetect</source>
        <translation>&amp;Автоопределение</translation>
    </message>
    <message>
        <source>&amp;Proxy</source>
        <translation>П&amp;рокси</translation>
    </message>
    <message>
        <source>&amp;No Proxy</source>
        <translation>&amp;Без прокси</translation>
    </message>
    <message>
        <source>Use S&amp;ystem values</source>
        <translation>Использовать с&amp;истемные настройки</translation>
    </message>
    <message>
        <source>&amp;Manual Proxy settings</source>
        <translation>&amp;Ручные настройки прокси</translation>
    </message>
    <message>
        <source>Proxy Values</source>
        <translation>Параметры прокси</translation>
    </message>
    <message>
        <source>&amp;Host:</source>
        <translation>&amp;Хост:</translation>
    </message>
    <message>
        <source>&amp;Port:</source>
        <translation>&amp;Порт:</translation>
    </message>
    <message>
        <source>&amp;Username</source>
        <translation>&amp;Имя пользователя</translation>
    </message>
    <message>
        <source>Pass&amp;word</source>
        <translation>Пар&amp;оль</translation>
    </message>
    <message>
        <source>&amp;Language</source>
        <translation>&amp;Язык</translation>
    </message>
    <message>
        <source>Cac&amp;he</source>
        <translation>К&amp;эш</translation>
    </message>
    <message>
        <source>Download cache settings</source>
        <translation>Настройки кэша загрузок</translation>
    </message>
    <message>
        <source>Rockbox Utility uses a local download cache to save network traffic. You can change the path to the cache and use it as local repository by enabling Offline mode.</source>
        <translation>Мастер Rockbox использует локальный кэш скачивания для экономии сетевой передачи. Вы можете изменить путь к кэшу и использовать его как локальное хранилище с помощью автономного режима.</translation>
    </message>
    <message>
        <source>Current cache size is %1</source>
        <translation>Текущий размер кэша: %1</translation>
    </message>
    <message>
        <source>P&amp;ath</source>
        <translation>&amp;Путь</translation>
    </message>
    <message>
        <source>Entering an invalid folder will reset the path to the systems temporary path.</source>
        <translation>Введение неверного пути сбросит путь в значение системной временной папки.</translation>
    </message>
    <message>
        <source>Disable local &amp;download cache</source>
        <translation>Отключить локальный кэш &amp;загрузок</translation>
    </message>
    <message>
        <source>&lt;p&gt;This will try to use all information from the cache, even information about updates. Only use this option if you want to install without network connection. Note: you need to do the same install you want to perform later with network access first to download all required files to the cache.&lt;/p&gt;</source>
        <translation>&lt;p&gt;Эта функция попытается использовать всю информацию из кэша, даже информацию об обновлениях. Используйте эту возможность только если вы хотите устанавливать без подключения к сети. Примечание: Вам необходимо выполнить такую-же установку, которую вы хотите выполнить потом, пока подключение к сети действует, чтобы загрузить все нужные файлы в кэш.&lt;/p&gt;</translation>
    </message>
    <message>
        <source>O&amp;ffline mode</source>
        <translation>Ав&amp;тономный режим</translation>
    </message>
    <message>
        <source>Clean cache &amp;now</source>
        <translation>Вычистить кэш &amp;сейчас</translation>
    </message>
    <message>
        <source>&amp;TTS &amp;&amp; Encoder</source>
        <translation>&amp;TTS &amp;&amp; Кодировщик</translation>
    </message>
    <message>
        <source>TTS Engine</source>
        <translation>Движок TTS</translation>
    </message>
    <message>
        <source>&amp;Select TTS Engine</source>
        <translation>&amp;Выберите движок TTS</translation>
    </message>
    <message>
        <source>Encoder Engine</source>
        <translation>движок кодирования</translation>
    </message>
    <message>
        <source>&amp;Ok</source>
        <translation>&amp;OK</translation>
    </message>
    <message>
        <source>&amp;Cancel</source>
        <translation>&amp;Отмена</translation>
    </message>
    <message>
        <source>Configure TTS Engine</source>
        <translation>Настроить движок TTS</translation>
    </message>
    <message>
        <source>Configuration invalid!</source>
        <translation>Настройка неверна!</translation>
    </message>
    <message>
        <source>Configure &amp;TTS</source>
        <translation>Настроить &amp;TTS</translation>
    </message>
    <message>
        <source>Configure &amp;Enc</source>
        <translation>Настроить &amp;кодировщик</translation>
    </message>
    <message>
        <source>encoder name</source>
        <translation>имя кодировщика</translation>
    </message>
    <message>
        <source>Test TTS</source>
        <translation>Проверить TTS</translation>
    </message>
    <message>
        <source>Show disabled targets</source>
        <translation>Показывать отключенные устройства</translation>
    </message>
    <message>
        <source>&amp;Refresh</source>
        <translation>&amp;Обновить</translation>
    </message>
    <message>
        <source>&amp;Use string corrections for TTS</source>
        <translation>&amp;Использовать корекции строк для TTS</translation>
    </message>
</context>
<context>
    <name>Configure</name>
    <message>
        <source>English</source>
        <comment>This is the localized language name, i.e. your language.</comment>
        <translation>Русский</translation>
    </message>
</context>
<context>
    <name>CreateVoiceFrm</name>
    <message>
        <source>Create Voice File</source>
        <translation>Создать голосовой файл</translation>
    </message>
    <message>
        <source>Select the Language you want to generate a voicefile for:</source>
        <translation>Выберите язык, для которого Вы хотите создать голосовой файл:</translation>
    </message>
    <message>
        <source>Generation settings</source>
        <translation>Настройки генерирования</translation>
    </message>
    <message>
        <source>Encoder profile:</source>
        <translation>Профиль кодировщика:</translation>
    </message>
    <message>
        <source>TTS profile:</source>
        <translation>Профиль TTS:</translation>
    </message>
    <message>
        <source>Change</source>
        <translation>Изменить</translation>
    </message>
    <message>
        <source>&amp;Install</source>
        <translation>&amp;Установить</translation>
    </message>
    <message>
        <source>&amp;Cancel</source>
        <translation>&amp;Отмена</translation>
    </message>
    <message>
        <source>Wavtrim Threshold</source>
        <translation>Порог Wavtrim</translation>
    </message>
    <message>
        <source>Language</source>
        <translation>Язык</translation>
    </message>
</context>
<context>
    <name>CreateVoiceWindow</name>
    <message>
        <source>Selected TTS engine: &lt;b&gt;%1&lt;/b&gt;</source>
        <translation>Выбранный движок TTS : &lt;b&gt;%1&lt;/b&gt;</translation>
    </message>
    <message>
        <source>Selected encoder: &lt;b&gt;%1&lt;/b&gt;</source>
        <translation>Выбранный кодировщик: &lt;b&gt;%1&lt;/b&gt;</translation>
    </message>
</context>
<context>
    <name>EncTtsCfgGui</name>
    <message>
        <source>Waiting for engine...</source>
        <translation>Ожидание движка...</translation>
    </message>
    <message>
        <source>Ok</source>
        <translation>OK</translation>
    </message>
    <message>
        <source>Cancel</source>
        <translation>Отмена</translation>
    </message>
    <message>
        <source>Browse</source>
        <translation>Обзор</translation>
    </message>
    <message>
        <source>Refresh</source>
        <translation>Обновить</translation>
    </message>
    <message>
        <source>Select executable</source>
        <translation>Выбрать исполняемый файл</translation>
    </message>
</context>
<context>
    <name>EncoderExe</name>
    <message>
        <source>Path to Encoder:</source>
        <translation type="unfinished">Путь к кодировщику:</translation>
    </message>
    <message>
        <source>Encoder options:</source>
        <translation type="unfinished">Настройки кодировщика:</translation>
    </message>
</context>
<context>
    <name>EncoderLame</name>
    <message>
        <source>LAME</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Volume</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Quality</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Could not find libmp3lame!</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>EncoderRbSpeex</name>
    <message>
        <source>Volume:</source>
        <translation type="unfinished">Громкость:</translation>
    </message>
    <message>
        <source>Quality:</source>
        <translation type="unfinished">Качество:</translation>
    </message>
    <message>
        <source>Complexity:</source>
        <translation type="unfinished">Сложность:</translation>
    </message>
    <message>
        <source>Use Narrowband:</source>
        <translation type="unfinished">Узкополосный:</translation>
    </message>
</context>
<context>
    <name>InfoWidget</name>
    <message>
        <source>File</source>
        <translation type="unfinished">Файл</translation>
    </message>
    <message>
        <source>Version</source>
        <translation type="unfinished">Версия</translation>
    </message>
</context>
<context>
    <name>InfoWidgetFrm</name>
    <message>
        <source>Form</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Currently installed packages.&lt;br/&gt;&lt;b&gt;Note:&lt;/b&gt; if you manually installed packages this might not be correct!</source>
        <translation type="unfinished">Установленные пакеты.&lt;br/&gt;&lt;b&gt;Примечание:&lt;/b&gt;Если вы установили некоторые пакеты вручную, могут возниктуть несоответствия!</translation>
    </message>
    <message>
        <source>1</source>
        <translation type="unfinished">1</translation>
    </message>
</context>
<context>
    <name>InstallTalkFrm</name>
    <message>
        <source>Install Talk Files</source>
        <translation>Установить голосовые файлы</translation>
    </message>
    <message>
        <source>Select the Folder to generate Talkfiles for.</source>
        <translation>Выберите папку, в которую генерировать голосовые файлы.</translation>
    </message>
    <message>
        <source>&amp;Browse</source>
        <translation>&amp;Обзор</translation>
    </message>
    <message>
        <source>Generation settings</source>
        <translation>Настройки создания</translation>
    </message>
    <message>
        <source>Encoder profile:</source>
        <translation>Профиль кодировщика:</translation>
    </message>
    <message>
        <source>TTS profile:</source>
        <translation>Профиль TTS :</translation>
    </message>
    <message>
        <source>Generation options</source>
        <translation>Свойства сгенерированого</translation>
    </message>
    <message>
        <source>Run recursive</source>
        <translation>Рекурсивный пробег</translation>
    </message>
    <message>
        <source>Strip Extensions</source>
        <translation>Отрезать расширения</translation>
    </message>
    <message>
        <source>&amp;Cancel</source>
        <translation>&amp;Отмена</translation>
    </message>
    <message>
        <source>&amp;Install</source>
        <translation>&amp;Установить</translation>
    </message>
    <message>
        <source>Change</source>
        <translation>Изменить</translation>
    </message>
    <message>
        <source>Generate .talk files for Folders</source>
        <translation>Выработать .talk-файлы для папок</translation>
    </message>
    <message>
        <source>Generate .talk files for Files</source>
        <translation>Сгенерировать .talk-файлы для файлов</translation>
    </message>
    <message>
        <source>Talkfile Folder</source>
        <translation>Папка с файлами произношения</translation>
    </message>
    <message>
        <source>Ignore files (comma seperated Wildcards):</source>
        <translation>Не учитывать файлы (через запятую):</translation>
    </message>
    <message>
        <source>Create only new Talkfiles</source>
        <translation>Создавать только новые файлы произношения</translation>
    </message>
</context>
<context>
    <name>InstallTalkWindow</name>
    <message>
        <source>The Folder to Talk is wrong!</source>
        <translation>Папка на произношение неверная!</translation>
    </message>
    <message>
        <source>Selected TTS engine: &lt;b&gt;%1&lt;/b&gt;</source>
        <translation>Выбранный мотор TTS: &lt;b&gt;%1&lt;/b&gt;</translation>
    </message>
    <message>
        <source>Selected encoder: &lt;b&gt;%1&lt;/b&gt;</source>
        <translation>Выбранный кодировщик: &lt;b&gt;%1&lt;/b&gt;</translation>
    </message>
    <message>
        <source>Select folder to create talk files</source>
        <translation>Выберите папку, в которой создавать голосовые файлы</translation>
    </message>
</context>
<context>
    <name>InstallWindow</name>
    <message>
        <source>Backup to %1</source>
        <translation>Резервная копия в %1</translation>
    </message>
    <message>
        <source>Mount point is wrong!</source>
        <translation>Точка монтирования неверная!</translation>
    </message>
    <message>
        <source>Really continue?</source>
        <translation>Продожить?</translation>
    </message>
    <message>
        <source>Aborted!</source>
        <translation>Отменено!</translation>
    </message>
    <message>
        <source>Beginning Backup...</source>
        <translation>Начинаю резервную копию...</translation>
    </message>
    <message>
        <source>Backup failed!</source>
        <translation>Не удалось создать резервную копию!</translation>
    </message>
    <message>
        <source>Select Backup Filename</source>
        <translation>Выберите имя файла резервной копии</translation>
    </message>
    <message>
        <source>This is the absolute up to the minute Rockbox built. A current build will get updated every time a change is made. Latest version is %1 (%2).</source>
        <translation>Это самая-самая последняя сборка Rockbox, она обновляется после каждого изменения в исходном коде. Текущая версия: %1 (%2).</translation>
    </message>
    <message>
        <source>&lt;b&gt;This is the recommended version.&lt;/b&gt;</source>
        <translation>&lt;b&gt;Это рекомендованная версия.&lt;/b&gt;</translation>
    </message>
    <message>
        <source>This is the last released version of Rockbox.</source>
        <translation>Это последняя официальная версия Rockbox.</translation>
    </message>
    <message>
        <source>&lt;b&gt;Note:&lt;/b&gt; The lastest released version is %1. &lt;b&gt;This is the recommended version.&lt;/b&gt;</source>
        <translation>&lt;b&gt;Примечание:&lt;/b&gt; Текущая последняя версия: %1. &lt;b&gt;Это рекомендованная версия.&lt;/b&gt;</translation>
    </message>
    <message>
        <source>These are automatically built each day from the current development source code. This generally has more features than the last stable release but may be much less stable. Features may change regularly.</source>
        <translation>Они автоматически собраны каждый день из текущего исходного кода. В них обычно больше возможностей, чем в последней официальной версии, но они могут работать нестабильно. Возможности могут регулярно изменяться.</translation>
    </message>
    <message>
        <source>&lt;b&gt;Note:&lt;/b&gt; archived version is %1 (%2).</source>
        <translation>&lt;b&gt;Примечание :&lt;/b&gt; текущая версия в архиве: %1 (%2).</translation>
    </message>
    <message>
        <source>Backup finished.</source>
        <translation>Создание резервной копии завершено.</translation>
    </message>
</context>
<context>
    <name>InstallWindowFrm</name>
    <message>
        <source>Install Rockbox</source>
        <translation>Установить Rockbox</translation>
    </message>
    <message>
        <source>Please select the Rockbox version you want to install on your player:</source>
        <translation>Выберите версию Rockbox, котороую хотите установить на Ваш плеер:</translation>
    </message>
    <message>
        <source>Version</source>
        <translation>Версия</translation>
    </message>
    <message>
        <source>Rockbox &amp;stable</source>
        <translation>&amp;Стабильная</translation>
    </message>
    <message>
        <source>&amp;Archived Build</source>
        <translation>&amp;Архивированная сборка</translation>
    </message>
    <message>
        <source>&amp;Current Build</source>
        <translation>&amp;Текущая сборка</translation>
    </message>
    <message>
        <source>Details</source>
        <translation>Подробно</translation>
    </message>
    <message>
        <source>Details about the selected version</source>
        <translation>Подробнее о выбранной версии</translation>
    </message>
    <message>
        <source>Note</source>
        <translation>Примечание</translation>
    </message>
    <message>
        <source>&amp;Install</source>
        <translation>&amp;Установить</translation>
    </message>
    <message>
        <source>&amp;Cancel</source>
        <translation>&amp;Отмена</translation>
    </message>
    <message>
        <source>Backup</source>
        <translation>Создать резервную копию</translation>
    </message>
    <message>
        <source>Backup before installing</source>
        <translation>Создать резервную копию перед установкой</translation>
    </message>
    <message>
        <source>Backup location</source>
        <translation>Путь к резервной копии</translation>
    </message>
    <message>
        <source>Change</source>
        <translation>Изменить</translation>
    </message>
    <message>
        <source>Rockbox Utility stores copies of Rockbox it has downloaded on the local hard disk to save network traffic. If your local copy is no longer working, tick this box to download a fresh copy.</source>
        <translation>Мастер Rockbox сохраняет загруженные копии Rockbox на жёстком диске для экономии сетевой передачи. Если вы не хотите использовать локальную копию или она не работает, поставьте галочку чтобы загрузить свежую копию.</translation>
    </message>
    <message>
        <source>&amp;Don&apos;t use locally cached copy</source>
        <translation>&amp;Не использовать копию из локального кэша</translation>
    </message>
</context>
<context>
    <name>ManualWidget</name>
    <message>
        <source>&lt;a href=&apos;%1&apos;&gt;PDF Manual&lt;/a&gt;</source>
        <translation type="unfinished">&lt;a href=&apos;%1&apos;&gt;Руководство по эксплуатации в PDF&lt;/a&gt;</translation>
    </message>
    <message>
        <source>&lt;a href=&apos;%1&apos;&gt;HTML Manual (opens in browser)&lt;/a&gt;</source>
        <translation type="unfinished">&lt;a href=&apos;%1&apos;&gt;Руководство по эксплуатации в HTML (открывается в обозревателе)&lt;/a&gt;</translation>
    </message>
    <message>
        <source>Select a device for a link to the correct manual</source>
        <translation type="unfinished">Выберите устройство, чтобы получить ссылку на соответствующее руководство по эксплуатации</translation>
    </message>
    <message>
        <source>&lt;a href=&apos;%1&apos;&gt;Manual Overview&lt;/a&gt;</source>
        <translation type="unfinished">&lt;a href=&apos;%1&apos;&gt;Обзор руководства по эксплуатации&lt;/a&gt;</translation>
    </message>
    <message>
        <source>Confirm download</source>
        <translation type="unfinished">Потвердите скачивание</translation>
    </message>
    <message>
        <source>Do you really want to download the manual? The manual will be saved to the root folder of your player.</source>
        <translation type="unfinished">Вы действительно хотите скачать руководство по эксплуатации? Оно будет записано в коренную папку Вашего плеера.</translation>
    </message>
</context>
<context>
    <name>ManualWidgetFrm</name>
    <message>
        <source>Form</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Read the manual</source>
        <translation type="unfinished">Читать руководство по эксплуатации</translation>
    </message>
    <message>
        <source>PDF manual</source>
        <translation type="unfinished">Руководство по эксплуатации в PDF</translation>
    </message>
    <message>
        <source>HTML manual</source>
        <translation type="unfinished">Руководство по эксплуатации в HTML</translation>
    </message>
    <message>
        <source>Download the manual</source>
        <translation type="unfinished">Скачать руководство по эксплуатации</translation>
    </message>
    <message>
        <source>&amp;PDF version</source>
        <translation type="unfinished">Версия &amp;PDF</translation>
    </message>
    <message>
        <source>&amp;HTML version (zip file)</source>
        <translation type="unfinished">Версия &amp;HTML (.zip файл)</translation>
    </message>
    <message>
        <source>Down&amp;load</source>
        <translation type="unfinished">С&amp;качать</translation>
    </message>
</context>
<context>
    <name>PreviewFrm</name>
    <message>
        <source>Preview</source>
        <translation>Предпросмотр</translation>
    </message>
</context>
<context>
    <name>ProgressLoggerFrm</name>
    <message>
        <source>Progress</source>
        <translation>Продвижение</translation>
    </message>
    <message>
        <source>&amp;Abort</source>
        <translation>&amp;Отмена</translation>
    </message>
    <message>
        <source>progresswindow</source>
        <translation>Окно продвижения</translation>
    </message>
    <message>
        <source>Save Log</source>
        <translation>Сохранить журнал</translation>
    </message>
</context>
<context>
    <name>ProgressLoggerGui</name>
    <message>
        <source>&amp;Ok</source>
        <translation>&amp;OK</translation>
    </message>
    <message>
        <source>&amp;Abort</source>
        <translation>&amp;Отмена</translation>
    </message>
    <message>
        <source>Save system trace log</source>
        <translation>Сохранить журнал трассировки системы</translation>
    </message>
</context>
<context>
    <name>QObject</name>
    <message>
        <source>LTR</source>
        <extracomment>This string is used to indicate the writing direction. Translate it to &quot;RTL&quot; (without quotes) for RTL languages. Anything else will get treated as LTR language.
----------
This string is used to indicate the writing direction. Translate it to &quot;RTL&quot; (without quotes) for RTL languages. Anything else will get treated as LTR language.</extracomment>
        <translation>LTR</translation>
    </message>
    <message>
        <source>(unknown vendor name) </source>
        <translation>(неизвестный поставщик)</translation>
    </message>
    <message>
        <source>(unknown product name)</source>
        <translation>(неизвестный продукт)</translation>
    </message>
</context>
<context>
    <name>QuaZipFile</name>
    <message>
        <source>ZIP/UNZIP API error %1</source>
        <translation>ошибка ZIP/UNZIP API %1</translation>
    </message>
</context>
<context>
    <name>RbUtilQt</name>
    <message>
        <source>&lt;b&gt;%1 %2&lt;/b&gt; at &lt;b&gt;%3&lt;/b&gt;</source>
        <translation>&lt;b&gt;%1 %2&lt;/b&gt; на &lt;b&gt;%3&lt;/b&gt;</translation>
    </message>
    <message>
        <source>Confirm Installation</source>
        <translation>Подтвердите установку</translation>
    </message>
    <message>
        <source>Mount point is wrong!</source>
        <translation>Точка монтирования неверная!</translation>
    </message>
    <message>
        <source>Do you really want to install the Bootloader?</source>
        <translation>Вы действительно хотите установить загрузчик?</translation>
    </message>
    <message>
        <source>Do you really want to install the fonts package?</source>
        <translation>Вы действительно хотите установить дополнительные шрифты?</translation>
    </message>
    <message>
        <source>Do you really want to install the voice file?</source>
        <translation>Вы действительно хотите установить голосовой файл?</translation>
    </message>
    <message>
        <source>Do you really want to install the game addon files?</source>
        <translation>Вы действительно хотите установить дополнительные игровые файлы?</translation>
    </message>
    <message>
        <source>Confirm Uninstallation</source>
        <translation>Подтвердите удаление</translation>
    </message>
    <message>
        <source>Do you really want to uninstall the Bootloader?</source>
        <translation>Вы действительно хотите удалить загрузчик?</translation>
    </message>
    <message>
        <source>Confirm installation</source>
        <translation>Подтвердите установку</translation>
    </message>
    <message>
        <source>Do you really want to install Rockbox Utility to your player? After installation you can run it from the players hard drive.</source>
        <translation>Вы действительно хотите установить мастера Rockbox на Ваш аудио плеер? Вы сможете потом запустить это с диска или памяти плеера.</translation>
    </message>
    <message>
        <source>Installing Rockbox Utility</source>
        <translation>Установка мастера Rockbox</translation>
    </message>
    <message>
        <source>Error installing Rockbox Utility</source>
        <translation>Ошибка при установке мастера Rockbox</translation>
    </message>
    <message>
        <source>Installing user configuration</source>
        <translation>Установка настроек пользователя</translation>
    </message>
    <message>
        <source>Error installing user configuration</source>
        <translation>Ошибка при установке настроек пользователя</translation>
    </message>
    <message>
        <source>Successfully installed Rockbox Utility.</source>
        <translation>Мастер Rockbox успешно установлен.</translation>
    </message>
    <message>
        <source>Configuration error</source>
        <translation>Ошибка в настройках</translation>
    </message>
    <message>
        <source>Error</source>
        <translation>Ошибка</translation>
    </message>
    <message>
        <source>Your device doesn&apos;t have a doom plugin. Aborting.</source>
        <translation>На вашем устройстве нет плагина Doom. Отмена.</translation>
    </message>
    <message>
        <source>Your configuration is invalid. Please go to the configuration dialog and make sure the selected values are correct.</source>
        <translation>Ваши настройки недействительны. Проверьте, что ваши настройки правильные в окне настроек.</translation>
    </message>
    <message>
        <source>Aborted!</source>
        <translation>Отменено!</translation>
    </message>
    <message>
        <source>Installed Rockbox detected</source>
        <translation>Обнаружен уже установленный Rockbox</translation>
    </message>
    <message>
        <source>Rockbox installation detected. Do you want to backup first?</source>
        <translation>Обнаружен уже установленный Rockbox. Желаете прежде всего создать резервную копию?</translation>
    </message>
    <message>
        <source>Backup failed!</source>
        <translation>Не удалось создать резервную копию!</translation>
    </message>
    <message>
        <source>Warning</source>
        <translation>Предупреждение</translation>
    </message>
    <message>
        <source>The Application is still downloading Information about new Builds. Please try again shortly.</source>
        <translation>Программа ещё загружает информацию о новых версиях. Попробуйте снова через несколько мгновений.</translation>
    </message>
    <message>
        <source>Starting backup...</source>
        <translation>Начинаю резервную копию...</translation>
    </message>
    <message>
        <source>New installation</source>
        <translation>Новая установка</translation>
    </message>
    <message>
        <source>Your configuration is invalid. This is most likely due to a changed device path. The configuration dialog will now open to allow you to correct the problem.</source>
        <translation>Ваши настройки негодны. Это скорее всего из-за изменённого пути к устройству. Окно настроек сейчас откроется, чтобы позволить Вам решить проблему.</translation>
    </message>
    <message>
        <source>Backup successful</source>
        <translation>Резеврная копия успешно создана</translation>
    </message>
    <message>
        <source>Network error</source>
        <translation>Ошибка сети</translation>
    </message>
    <message>
        <source>Really continue?</source>
        <translation>Продожить?</translation>
    </message>
    <message>
        <source>No install method known.</source>
        <translation>Нет известного способа установки.</translation>
    </message>
    <message>
        <source>Bootloader detected</source>
        <translation>Найден загрузчик</translation>
    </message>
    <message>
        <source>Bootloader already installed. Do you want to reinstall the bootloader?</source>
        <translation>Загрузчик уже установлен. Хотите переустановить?</translation>
    </message>
    <message>
        <source>Create Bootloader backup</source>
        <translation>Создать резервную копию загрузчика</translation>
    </message>
    <message>
        <source>You can create a backup of the original bootloader file. Press &quot;Yes&quot; to select an output folder on your computer to save the file to. The file will get placed in a new folder &quot;%1&quot; created below the selected folder.
Press &quot;No&quot; to skip this step.</source>
        <translation>Вы можете создать резервную копию фирменного файла загрузчика. Нажмите на &quot;Да&quot;, чтобы выбрать выходную папку, в которой будет создана ещё одна папка &quot;%1&quot; содержащая файл.
Нажмите на &quot;Нет&quot;, чтобы пропустить этот шаг.</translation>
    </message>
    <message>
        <source>Browse backup folder</source>
        <translation>Обзор папки резервных копий</translation>
    </message>
    <message>
        <source>Prerequisites</source>
        <translation>Предварительные требования</translation>
    </message>
    <message>
        <source>Select firmware file</source>
        <translation>Выберите файл прошивки</translation>
    </message>
    <message>
        <source>Error opening firmware file</source>
        <translation>Ошибка при открытии файла прошивки</translation>
    </message>
    <message>
        <source>Backup error</source>
        <translation>Ошибка резервной копии</translation>
    </message>
    <message>
        <source>Could not create backup file. Continue?</source>
        <translation>Не могу создать резеврную копию файла. Продолжить?</translation>
    </message>
    <message>
        <source>Manual steps required</source>
        <translation>Требуются действия вручную</translation>
    </message>
    <message>
        <source>Do you really want to perform a complete installation?

This will install Rockbox %1. To install the most recent development build available press &quot;Cancel&quot; and use the &quot;Installation&quot; tab.</source>
        <translation>Вы действительно хотите выполнить полную установку?

Это установит Rockbox %1. Чтобы установить самую новую доступную версию, нажмите на &quot;Отмена&quot; и используйте функции вкладки &quot;Установка&quot;.</translation>
    </message>
    <message>
        <source>Do you really want to perform a minimal installation? A minimal installation will contain only the absolutely necessary parts to run Rockbox.

This will install Rockbox %1. To install the most recent development build available press &quot;Cancel&quot; and use the &quot;Installation&quot; tab.</source>
        <translation>Вы действительно хотите выполнить минимальную установку? Это установит лишь необходимые для работы Rockbox файлы.

Это установит Rockbox %1. Чтобы установить самую новую доступную версию, нажмите на &quot;Отмена&quot; и используйте функции вкладки &quot;Установка&quot;.</translation>
    </message>
    <message>
        <source>Bootloader installation skipped</source>
        <translation>Установка загрузчика пропущена</translation>
    </message>
    <message>
        <source>Bootloader installation aborted</source>
        <translation>Установка загрузчика отменена</translation>
    </message>
    <message>
        <source>Downloading build information, please wait ...</source>
        <translation>Скачивается информация о сборке, пожалуйста подождите...</translation>
    </message>
    <message>
        <source>Can&apos;t get version information!</source>
        <translation>Не могу получить информацию о версии!</translation>
    </message>
    <message>
        <source>This is a new installation of Rockbox Utility, or a new version. The configuration dialog will now open to allow you to setup the program,  or review your settings.</source>
        <translation>Это новая установка или новая версия мастера Rockbox. Диалог настройки сейчас откроется и даст возможность настроить программу или пересмотреть ваши настройки.</translation>
    </message>
    <message>
        <source>Download build information finished.</source>
        <translation>Загрузка информации о сборке завершена.</translation>
    </message>
    <message>
        <source>RockboxUtility Update available</source>
        <translation>Доступно обновление мастера Rockbox</translation>
    </message>
    <message>
        <source>&lt;b&gt;New RockboxUtility Version available.&lt;/b&gt; &lt;br&gt;&lt;br&gt;Download it from here: &lt;a href=&apos;%1&apos;&gt;%2&lt;/a&gt;</source>
        <translation>Доступна новая версия мастера Rockbox. Скачать можно отсюда: &lt;a href=&apos;%1&apos;&gt;%2&lt;/a&gt;</translation>
    </message>
    <message>
        <source>Wine detected!</source>
        <translation>Обнаружен Wine!</translation>
    </message>
    <message>
        <source>It seems you are trying to run this program under Wine. Please don&apos;t do this, running under Wine will fail. Use the native Linux binary instead.</source>
        <translation>Похоже, что вы пытаетесь пользоваться этой программой с помощью Wine. Не делайте этого, это приведёт к сбою. Лучше пользуйтесь нативной программой для Linux.</translation>
    </message>
    <message>
        <source>Can&apos;t get version information.
Network error: %1. Please check your network and proxy settings.</source>
        <translation>Не могу получить информацию о версии.
Ошибка сети: %1. Проверьте настройки сети и прокси.</translation>
    </message>
    <message>
        <source>No Rockbox installation found</source>
        <translation>Не найдено установки Rockbox</translation>
    </message>
    <message>
        <source>Could not determine the installed Rockbox version. Please install a Rockbox build before installing fonts.</source>
        <translation>Не могу определить версию установленного Rockbox. Устанавливайте Rockbox перед установкой шрифтов.</translation>
    </message>
    <message>
        <source>Could not determine the installed Rockbox version. Please install a Rockbox build before installing voice files.</source>
        <translation>Не могу определить версию установленного Rockbox. Устанавливайте Rockbox перед установкой голосовых файлов.</translation>
    </message>
    <message>
        <source>No uninstall method for this target known.</source>
        <translation>Нет известного способа удаления с этого устройства.</translation>
    </message>
    <message>
        <source>Rockbox Utility can not uninstall the bootloader on this target. Try a normal firmware update to remove the booloader.</source>
        <translation>Мастер Rockbox не может удалить загрузчик с этого устройства. Попробуйте нормальное обновление прошивки, чтобы удалить загрузчик.</translation>
    </message>
    <message>
        <source>Checking for update ...</source>
        <translation>Проверяется наличие обновления ...</translation>
    </message>
    <message>
        <source>New version of Rockbox Utility available.</source>
        <translation>Доступна новая версия мастера Rockbox.</translation>
    </message>
    <message>
        <source>Rockbox Utility is up to date.</source>
        <translation>Мастер Rockbox не требует обновления.</translation>
    </message>
    <message>
        <source>Beginning Backup...</source>
        <translation>Начинаю создание резервной копии...</translation>
    </message>
    <message>
        <source>Error reading firmware file</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>RbUtilQtFrm</name>
    <message>
        <source>Rockbox Utility</source>
        <translation>Мастер Rockbox</translation>
    </message>
    <message>
        <source>Device</source>
        <translation>Устройство</translation>
    </message>
    <message>
        <source>Selected device:</source>
        <translation>Выбранное устройство:</translation>
    </message>
    <message>
        <source>&amp;Change</source>
        <translation>&amp;Изменить</translation>
    </message>
    <message>
        <source>&amp;Quick Start</source>
        <translation>&amp;Начало</translation>
    </message>
    <message>
        <source>Welcome</source>
        <translation>Добро пожаловать</translation>
    </message>
    <message>
        <source>&amp;Installation</source>
        <translation>&amp;Установка</translation>
    </message>
    <message>
        <source>Basic Rockbox installation</source>
        <translation>Базовая установка Rockbox</translation>
    </message>
    <message>
        <source>Install Bootloader</source>
        <translation>Установка загрузчика</translation>
    </message>
    <message>
        <source>Install Rockbox</source>
        <translation>Установить Rockbox</translation>
    </message>
    <message>
        <source>&amp;Extras</source>
        <translation>&amp;Дополнения</translation>
    </message>
    <message>
        <source>Install extras for Rockbox</source>
        <translation>Установить дополнения для Rockbox</translation>
    </message>
    <message>
        <source>Install Fonts package</source>
        <translation>Установить пакет со шрифтами</translation>
    </message>
    <message>
        <source>Install themes</source>
        <translation>Установить темы</translation>
    </message>
    <message>
        <source>Install game files</source>
        <translation>Установить игровые файлы</translation>
    </message>
    <message>
        <source>&amp;Accessibility</source>
        <translation>&amp;Специальные возможности</translation>
    </message>
    <message>
        <source>Install accessibility add-ons</source>
        <translation>Установить дополнения для специальных возможностей</translation>
    </message>
    <message>
        <source>Install Voice files</source>
        <translation>Установить голосовые файлы</translation>
    </message>
    <message>
        <source>Install Talk files</source>
        <translation>Установить файлы произношения</translation>
    </message>
    <message>
        <source>&amp;Uninstallation</source>
        <translation>&amp;Удаление</translation>
    </message>
    <message>
        <source>Uninstall Rockbox</source>
        <translation>Удалить Rockbox</translation>
    </message>
    <message>
        <source>Uninstall Bootloader</source>
        <translation>Удалить загрузчик</translation>
    </message>
    <message>
        <source>&amp;Manual</source>
        <translation>&amp;Руководство по эксплуатации</translation>
    </message>
    <message>
        <source>View and download the manual</source>
        <translation>Смотреть и/или загрузить руководство по эксплуатации</translation>
    </message>
    <message>
        <source>Inf&amp;o</source>
        <translation>&amp;Информация</translation>
    </message>
    <message>
        <source>&amp;File</source>
        <translation>&amp;Файл</translation>
    </message>
    <message>
        <source>&amp;About</source>
        <translation>&amp;О программе</translation>
    </message>
    <message>
        <source>Empty local download cache</source>
        <translation>Очистить локальный кэш скачивания</translation>
    </message>
    <message>
        <source>Install Rockbox Utility on player</source>
        <translation>Установить мастера Rockbox на плеер</translation>
    </message>
    <message>
        <source>&amp;Configure</source>
        <translation>&amp;Настройки</translation>
    </message>
    <message>
        <source>E&amp;xit</source>
        <translation>&amp;Выйти</translation>
    </message>
    <message>
        <source>Ctrl+Q</source>
        <translation>Ctrl+Q</translation>
    </message>
    <message>
        <source>About &amp;Qt</source>
        <translation>О &amp;Qt</translation>
    </message>
    <message>
        <source>&amp;Help</source>
        <translation>&amp;Помощь</translation>
    </message>
    <message>
        <source>Complete Installation</source>
        <translation>Полная установка</translation>
    </message>
    <message>
        <source>Action&amp;s</source>
        <translation>&amp;Действия</translation>
    </message>
    <message>
        <source>Info</source>
        <translation>Информация</translation>
    </message>
    <message>
        <source>Read PDF manual</source>
        <translation>Читать руководство по эксплуатации в PDF</translation>
    </message>
    <message>
        <source>Read HTML manual</source>
        <translation>Читать руководство по эксплуатации в HTML</translation>
    </message>
    <message>
        <source>Download PDF manual</source>
        <translation>Скачать руководство по эксплуатации в PDF</translation>
    </message>
    <message>
        <source>Download HTML manual (zip)</source>
        <translation>Скачать руководство по эксплуатации в HTML (zip)</translation>
    </message>
    <message>
        <source>Create Voice files</source>
        <translation>Создать голосовые файлы</translation>
    </message>
    <message>
        <source>Create Voice File</source>
        <translation>Создать голосовой файл</translation>
    </message>
    <message>
        <source>&lt;b&gt;Complete Installation&lt;/b&gt;&lt;br/&gt;This installs the bootloader, a current build and the extras package. This is the recommended method for new installations.</source>
        <translation>&lt;b&gt;Полная установка&lt;/b&gt;&lt;br/&gt;Устанавливает загрузчик, текущую сборку и дополнительные пакеты. Рекомендуется для новых установок.</translation>
    </message>
    <message>
        <source>&lt;b&gt;Install the bootloader&lt;/b&gt;&lt;br/&gt;Before Rockbox can be run on your audio player, you may have to install a bootloader. This is only necessary the first time Rockbox is installed.</source>
        <translation>&lt;b&gt;Установить загрузчик&lt;/b&gt;&lt;br/&gt;Перед использованием Rockbox на Вашем плеере, вам необходимо установить загрузчик. Необходимо только для первой установки Rockbox.</translation>
    </message>
    <message>
        <source>&lt;b&gt;Install Rockbox&lt;/b&gt; on your audio player</source>
        <translation>&lt;b&gt;Установить Rockbox&lt;/b&gt; на Ваш плеер</translation>
    </message>
    <message>
        <source>&lt;b&gt;Fonts Package&lt;/b&gt;&lt;br/&gt;The Fonts Package contains a couple of commonly used fonts. Installation is highly recommended.</source>
        <translation>&lt;b&gt;Пакет шрифтов&lt;/b&gt;&lt;br/&gt;Содержит несколько часто используемых шрифтов. Установка рекомендуется.</translation>
    </message>
    <message>
        <source>&lt;b&gt;Install Game Files&lt;/b&gt;&lt;br/&gt;Doom needs a base wad file to run.</source>
        <translation>&lt;b&gt;Установить игровые файлы&lt;/b&gt;&lt;br/&gt;Чтобы играть в Doom, необходимо установить базовый wad-файл.</translation>
    </message>
    <message>
        <source>&lt;b&gt;Install Voice file&lt;/b&gt;&lt;br/&gt;Voice files are needed to make Rockbox speak the user interface. Speaking is enabled by default, so if you installed the voice file Rockbox will speak.</source>
        <translation>&lt;b&gt;Установить голосовой файл&lt;/b&gt;&lt;br/&gt;Он нужен, чтобы Rockbox произносил пользовательский интерфейс. Произношение включено по умолчанию, поэтому если Вы установили голосовой файл, Rockbox станет разговаривать.</translation>
    </message>
    <message>
        <source>&lt;b&gt;Create Talk Files&lt;/b&gt;&lt;br/&gt;Talkfiles are needed to let Rockbox speak File and Foldernames</source>
        <translation>&lt;b&gt;Создать файлы произношения.&lt;/b&gt;&lt;br/&gt;Они нужны, чтобы Rockbox мог произносить имена файлов и папок</translation>
    </message>
    <message>
        <source>&lt;b&gt;Remove the bootloader&lt;/b&gt;&lt;br/&gt;After removing the bootloader you won&apos;t be able to start Rockbox.</source>
        <translation>&lt;b&gt;Удалить загрузчик&lt;/b&gt;&lt;br/&gt;После удаления загрузчика, вы не сможете запустить Rockbox.</translation>
    </message>
    <message>
        <source>&lt;b&gt;Uninstall Rockbox from your audio player.&lt;/b&gt;&lt;br/&gt;This will leave the bootloader in place (you need to remove it manually).</source>
        <translation>&lt;b&gt;Удалить Rockbox с Вашего плеера.&lt;/b&gt;&lt;br/&gt;Это оставит загрузчик установленным (его нужно будет удалить вручную).</translation>
    </message>
    <message>
        <source>Install &amp;Bootloader</source>
        <translation>Установить &amp;загрузчик</translation>
    </message>
    <message>
        <source>Install &amp;Rockbox</source>
        <translation>Установить &amp;Rockbox</translation>
    </message>
    <message>
        <source>Install &amp;Fonts Package</source>
        <translation>Установить пакет &amp;шрифтов</translation>
    </message>
    <message>
        <source>Install &amp;Themes</source>
        <translation>Установить &amp;темы</translation>
    </message>
    <message>
        <source>Install &amp;Game Files</source>
        <translation>Установить игровые &amp;файлы</translation>
    </message>
    <message>
        <source>&amp;Install Voice File</source>
        <translation>&amp;Установить голосовой файл</translation>
    </message>
    <message>
        <source>Create &amp;Talk Files</source>
        <translation>Установить файлы &amp;произношения</translation>
    </message>
    <message>
        <source>Remove &amp;bootloader</source>
        <translation>У&amp;далить загрузчик</translation>
    </message>
    <message>
        <source>Uninstall &amp;Rockbox</source>
        <translation>Удалить &amp;Rockbox</translation>
    </message>
    <message>
        <source>Create &amp;Voice File</source>
        <translation>&amp;Создать голосовой файл</translation>
    </message>
    <message>
        <source>&amp;System Info</source>
        <translation>Информация о &amp;системе</translation>
    </message>
    <message>
        <source>&amp;Complete Installation</source>
        <translation>&amp;Полная установка</translation>
    </message>
    <message>
        <source>device / mountpoint unknown or invalid</source>
        <translation>устройство / точка монтирования неизвестные или негодные</translation>
    </message>
    <message>
        <source>Minimal Installation</source>
        <translation>Минимальная установка</translation>
    </message>
    <message>
        <source>&lt;b&gt;Minimal installation&lt;/b&gt;&lt;br/&gt;This installs bootloader and the current build of Rockbox. If you don&apos;t want the extras package, choose this option.</source>
        <translation>&lt;b&gt;Минимальная установка&lt;/b&gt;&lt;br/&gt;Устанавливает загрузчик и текущую сборку Rockbox. Если Вам не нужны дополнительные пакеты, это самый подходящий вариант.</translation>
    </message>
    <message>
        <source>&amp;Minimal Installation</source>
        <translation>&amp;Минимальная установка</translation>
    </message>
    <message>
        <source>&amp;Troubleshoot</source>
        <translation>&amp;Устранение неполадок</translation>
    </message>
    <message>
        <source>System &amp;Trace</source>
        <translation>&amp;Трассировка системы</translation>
    </message>
    <message>
        <source>&lt;b&gt;Create Voice file&lt;/b&gt;&lt;br/&gt;Voice files are needed to make Rockbox speak the  user interface. Speaking is enabled by default, so
 if you installed the voice file Rockbox will speak.</source>
        <translation>&lt;b&gt;Создать голосовой файл&lt;/b&gt;&lt;br/&gt;Он нужен, чтобы Rockbox произносил пользовательский интерфейс. Произношение включено по умолчанию, поэтому если Вы установили голосовой файл, Rockbox станет разговаривать.</translation>
    </message>
    <message>
        <source>&lt;b&gt;Install Themes&lt;/b&gt;&lt;br/&gt;Rockbox&apos;s look can be customized by themes. You can choose and install several officially distributed themes.</source>
        <translation>&lt;b&gt;Установить темы&lt;/b&gt;&lt;br/&gt;Внешний вид Rockbox\&apos;а может быть изменен при помощи тем. Вы можете выбрать и установить несколько официально распространяемых тем.</translation>
    </message>
</context>
<context>
    <name>ServerInfo</name>
    <message>
        <source>Unknown</source>
        <translation>Неизвестный</translation>
    </message>
    <message>
        <source>Unusable</source>
        <translation>Непригодный</translation>
    </message>
    <message>
        <source>Unstable</source>
        <translation>Нестабильный</translation>
    </message>
    <message>
        <source>Stable</source>
        <translation>Стабильный</translation>
    </message>
</context>
<context>
    <name>SysTrace</name>
    <message>
        <source>Save system trace log</source>
        <translation>Сохранить журнал трассировки системы</translation>
    </message>
</context>
<context>
    <name>SysTraceFrm</name>
    <message>
        <source>System Trace</source>
        <translation>Трассировка системы</translation>
    </message>
    <message>
        <source>System State trace</source>
        <translation>Трассировка состояния системы</translation>
    </message>
    <message>
        <source>&amp;Close</source>
        <translation>&amp;Закрыть</translation>
    </message>
    <message>
        <source>&amp;Save</source>
        <translation>&amp;Сохранить</translation>
    </message>
    <message>
        <source>&amp;Refresh</source>
        <translation>&amp;Обновить</translation>
    </message>
    <message>
        <source>Save &amp;previous</source>
        <translation>Сохранить &amp;предыдущий</translation>
    </message>
</context>
<context>
    <name>Sysinfo</name>
    <message>
        <source>&lt;b&gt;OS&lt;/b&gt;&lt;br/&gt;</source>
        <translation>&lt;b&gt;ОС&lt;/b&gt;&lt;br/&gt;</translation>
    </message>
    <message>
        <source>&lt;b&gt;Username&lt;/b&gt;&lt;br/&gt;%1&lt;hr/&gt;</source>
        <translation>&lt;b&gt;Имя пользователя&lt;/b&gt;&lt;br/&gt;%1&lt;hr/&gt;</translation>
    </message>
    <message>
        <source>&lt;b&gt;Permissions&lt;/b&gt;&lt;br/&gt;%1&lt;hr/&gt;</source>
        <translation>&lt;b&gt;Полномочия&lt;/b&gt;&lt;br/&gt;%1&lt;hr/&gt;</translation>
    </message>
    <message>
        <source>&lt;b&gt;Attached USB devices&lt;/b&gt;&lt;br/&gt;</source>
        <translation>&lt;b&gt;Подключенные USB-устройства&lt;/b&gt;&lt;br/&gt;</translation>
    </message>
    <message>
        <source>VID: %1 PID: %2, %3</source>
        <translation>VID: %1 PID: %2, %3</translation>
    </message>
    <message>
        <source>Filesystem</source>
        <translation>Файловая система</translation>
    </message>
    <message>
        <source>Mountpoint</source>
        <translation>Точка монтирования</translation>
    </message>
    <message>
        <source>Label</source>
        <translation>Метка</translation>
    </message>
    <message>
        <source>Free</source>
        <translation>Свободно</translation>
    </message>
    <message>
        <source>Total</source>
        <translation>Всего</translation>
    </message>
    <message>
        <source>Cluster Size</source>
        <translation>Размер кластера</translation>
    </message>
    <message>
        <source>&lt;tr&gt;&lt;td&gt;%1&lt;/td&gt;&lt;td&gt;%4&lt;/td&gt;&lt;td&gt;%2 GiB&lt;/td&gt;&lt;td&gt;%3 GiB&lt;/td&gt;&lt;td&gt;%5&lt;/td&gt;&lt;/tr&gt;</source>
        <translation>&lt;tr&gt;&lt;td&gt;%1&lt;/td&gt;&lt;td&gt;%4&lt;/td&gt;&lt;td&gt;%2 ГиБ&lt;/td&gt;&lt;td&gt;%3 ГБ&lt;/td&gt;&lt;td&gt;%5&lt;/td&gt;&lt;/tr&gt;</translation>
    </message>
</context>
<context>
    <name>SysinfoFrm</name>
    <message>
        <source>System Info</source>
        <translation>Информация о системе</translation>
    </message>
    <message>
        <source>&amp;Refresh</source>
        <translation>&amp;Обновить</translation>
    </message>
    <message>
        <source>&amp;OK</source>
        <translation>&amp;OK</translation>
    </message>
</context>
<context>
    <name>System</name>
    <message>
        <source>Guest</source>
        <translation>Гость</translation>
    </message>
    <message>
        <source>Admin</source>
        <translation>Администратор</translation>
    </message>
    <message>
        <source>User</source>
        <translation>Пользователь</translation>
    </message>
    <message>
        <source>Error</source>
        <translation>Ошибка</translation>
    </message>
    <message>
        <source>(no description available)</source>
        <translation>(описание недоступно)</translation>
    </message>
</context>
<context>
    <name>TTSBase</name>
    <message>
        <source>Espeak TTS Engine</source>
        <translation>Espeak TTS движок</translation>
    </message>
    <message>
        <source>Flite TTS Engine</source>
        <translation>Flite TTS движок</translation>
    </message>
    <message>
        <source>Swift TTS Engine</source>
        <translation>Swift TTS движок</translation>
    </message>
    <message>
        <source>SAPI TTS Engine</source>
        <translation>SAPI TTS движок</translation>
    </message>
    <message>
        <source>Festival TTS Engine</source>
        <translation>Festival TTS движок</translation>
    </message>
    <message>
        <source>OS X System Engine</source>
        <translation>Системный движок OS X</translation>
    </message>
</context>
<context>
    <name>TTSCarbon</name>
    <message>
        <source>Voice:</source>
        <translation>Голос:</translation>
    </message>
    <message>
        <source>Speed (words/min):</source>
        <translation>Скорость (слов/мин):</translation>
    </message>
    <message>
        <source>Could not voice string</source>
        <translation>Сбой голосового произношения</translation>
    </message>
    <message>
        <source>Could not convert intermediate file</source>
        <translation>Не могу преобразовать промежуточный файл</translation>
    </message>
    <message>
        <source>Pitch (0 for default):</source>
        <translation>Тон (0 по умолчанию) :</translation>
    </message>
</context>
<context>
    <name>TTSExes</name>
    <message>
        <source>TTS executable not found</source>
        <translation>Выполняемый файл TTS не найден</translation>
    </message>
    <message>
        <source>Path to TTS engine:</source>
        <translation>Путь к мотору TTS:</translation>
    </message>
    <message>
        <source>TTS engine options:</source>
        <translation>Настройки мотора TTS:</translation>
    </message>
</context>
<context>
    <name>TTSFestival</name>
    <message>
        <source>engine could not voice string</source>
        <translation>мотор не смог озвучить выражение</translation>
    </message>
    <message>
        <source>No description available</source>
        <translation>Описание отсутствует</translation>
    </message>
    <message>
        <source>Path to Festival client:</source>
        <translation>Путь к клиенту Festival:</translation>
    </message>
    <message>
        <source>Voice:</source>
        <translation>Голос:</translation>
    </message>
    <message>
        <source>Voice description:</source>
        <translation>Описание голоса:</translation>
    </message>
</context>
<context>
    <name>TTSSapi</name>
    <message>
        <source>Language:</source>
        <translation>Язык:</translation>
    </message>
    <message>
        <source>Voice:</source>
        <translation>Голос:</translation>
    </message>
    <message>
        <source>Speed:</source>
        <translation>Скорость:</translation>
    </message>
    <message>
        <source>Options:</source>
        <translation>Настройки:</translation>
    </message>
    <message>
        <source>Could not copy the SAPI script</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Could not start SAPI process</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>TalkFileCreator</name>
    <message>
        <source>Starting Talk file generation</source>
        <translation>Запуск вырабатывания файла произношения</translation>
    </message>
    <message>
        <source>Talk file creation aborted</source>
        <translation>Создание файла произношения отменено</translation>
    </message>
    <message>
        <source>Finished creating Talk files</source>
        <translation>Создание файлов произношения завершено</translation>
    </message>
    <message>
        <source>Reading Filelist...</source>
        <translation>Чтение списка файлов...</translation>
    </message>
    <message>
        <source>Copying of %1 to %2 failed</source>
        <translation>Сбой копирования %1 в %2</translation>
    </message>
    <message>
        <source>Copying Talkfiles...</source>
        <translation>Копирую файлы произношения...</translation>
    </message>
    <message>
        <source>File copy aborted</source>
        <translation>Копия файлов отменена</translation>
    </message>
    <message>
        <source>Cleaning up...</source>
        <translation>Навожу порядок...</translation>
    </message>
    <message>
        <source>Finished</source>
        <translation>Всё</translation>
    </message>
</context>
<context>
    <name>TalkGenerator</name>
    <message>
        <source>Starting TTS Engine</source>
        <translation>Запуск мотора TTS</translation>
    </message>
    <message>
        <source>Init of TTS engine failed</source>
        <translation>Сбой инициализации мотора TTS</translation>
    </message>
    <message>
        <source>Starting Encoder Engine</source>
        <translation>Запуск мотора кодировщика</translation>
    </message>
    <message>
        <source>Init of Encoder engine failed</source>
        <translation>Сбой инициализации мотора кодировщика</translation>
    </message>
    <message>
        <source>Voicing entries...</source>
        <translation>Озвучивание вводов...</translation>
    </message>
    <message>
        <source>Encoding files...</source>
        <translation>Кодировка файлов...</translation>
    </message>
    <message>
        <source>Voicing aborted</source>
        <translation>Озвучивание отменено</translation>
    </message>
    <message>
        <source>Voicing of %1 failed: %2</source>
        <translation>Сбой озвучивания %1 : %2</translation>
    </message>
    <message>
        <source>Encoding aborted</source>
        <translation>Кодировка отменена</translation>
    </message>
    <message>
        <source>Encoding of %1 failed</source>
        <translation>Сбой кодировки %1</translation>
    </message>
</context>
<context>
    <name>ThemeInstallFrm</name>
    <message>
        <source>Theme Installation</source>
        <translation>Установка тем</translation>
    </message>
    <message>
        <source>Selected Theme</source>
        <translation>Выбранная тема</translation>
    </message>
    <message>
        <source>Description</source>
        <translation>Описание</translation>
    </message>
    <message>
        <source>Download size:</source>
        <translation>Объём скачивания :</translation>
    </message>
    <message>
        <source>&amp;Cancel</source>
        <translation>&amp;Отмена</translation>
    </message>
    <message>
        <source>&amp;Install</source>
        <translation>&amp;Установить</translation>
    </message>
    <message>
        <source>Hold Ctrl to select multiple item, Shift for a range</source>
        <translation>Нажать и держать Ctrl для выделения нескольких элеметнов, Shift для выделения ряда элементов</translation>
    </message>
</context>
<context>
    <name>ThemesInstallWindow</name>
    <message>
        <source>no theme selected</source>
        <translation>Тема не выделена</translation>
    </message>
    <message>
        <source>Network error: %1.
Please check your network and proxy settings.</source>
        <translation>Ошибка сети: %1.
Проверьте настройки сети и прокси.</translation>
    </message>
    <message>
        <source>the following error occured:
%1</source>
        <translation>Произошла следующая ошибка :
%1</translation>
    </message>
    <message>
        <source>done.</source>
        <translation>выполнено.</translation>
    </message>
    <message>
        <source>fetching details for %1</source>
        <translation>получаю подробности о %1</translation>
    </message>
    <message>
        <source>fetching preview ...</source>
        <translation>Получаю предпросмотр ...</translation>
    </message>
    <message>
        <source>&lt;b&gt;Author:&lt;/b&gt; %1&lt;hr/&gt;</source>
        <translation>&lt;b&gt;Автор :&lt;/b&gt; %1&lt;hr/&gt;</translation>
    </message>
    <message>
        <source>unknown</source>
        <translation>неизвестный</translation>
    </message>
    <message>
        <source>&lt;b&gt;Version:&lt;/b&gt; %1&lt;hr/&gt;</source>
        <translation>&lt;b&gt;Версия :&lt;/b&gt; %1&lt;hr/&gt;</translation>
    </message>
    <message>
        <source>&lt;b&gt;Description:&lt;/b&gt; %1&lt;hr/&gt;</source>
        <translation>&lt;b&gt;Описание:&lt;/b&gt; %1&lt;hr/&gt;</translation>
    </message>
    <message>
        <source>no description</source>
        <translation>нет описания</translation>
    </message>
    <message>
        <source>no theme preview</source>
        <translation>предпросмотр недоступен</translation>
    </message>
    <message>
        <source>getting themes information ...</source>
        <translation>получаю информацию о темах ...</translation>
    </message>
    <message>
        <source>Mount point is wrong!</source>
        <translation>Неправильная точка монтирования!</translation>
    </message>
    <message>
        <source>no selection</source>
        <translation>нет выделения</translation>
    </message>
    <message>
        <source>Information</source>
        <translation>Информация</translation>
    </message>
    <message numerus="yes">
        <source>Download size %L1 kiB (%n item(s))</source>
        <translation>
            <numerusform>Размер загрузки %L1 КиБ (%n штука)</numerusform>
            <numerusform>Размер загрузки %L1 КиБ (%n штуки)</numerusform>
            <numerusform>Размер загрузки %L1 КиБ (%n штук)</numerusform>
        </translation>
    </message>
    <message>
        <source>Retrieving theme preview failed.
HTTP response code: %1</source>
        <translation>Сбой при получении предпросмотра темы.
Код ответа HTTP : %1</translation>
    </message>
</context>
<context>
    <name>UninstallFrm</name>
    <message>
        <source>Uninstall Rockbox</source>
        <translation>Удалить Rockbox</translation>
    </message>
    <message>
        <source>Please select the Uninstallation Method</source>
        <translation>Выберите способ удаления</translation>
    </message>
    <message>
        <source>Uninstallation Method</source>
        <translation>Способ удаления</translation>
    </message>
    <message>
        <source>Complete Uninstallation</source>
        <translation>Полное удаление</translation>
    </message>
    <message>
        <source>Smart Uninstallation</source>
        <translation>Выборочное удаление</translation>
    </message>
    <message>
        <source>Please select what you want to uninstall</source>
        <translation>Выберите, что вы желаете удалить</translation>
    </message>
    <message>
        <source>Installed Parts</source>
        <translation>Установленные части</translation>
    </message>
    <message>
        <source>&amp;Cancel</source>
        <translation>&amp;Отмена</translation>
    </message>
    <message>
        <source>&amp;Uninstall</source>
        <translation>&amp;Удалить</translation>
    </message>
</context>
<context>
    <name>Uninstaller</name>
    <message>
        <source>Starting Uninstallation</source>
        <translation>Начало удаления</translation>
    </message>
    <message>
        <source>Finished Uninstallation</source>
        <translation>Удаление завершено</translation>
    </message>
    <message>
        <source>Uninstallation finished</source>
        <translation>Удаление завершено</translation>
    </message>
    <message>
        <source>Uninstalling %1...</source>
        <translation>Удаляется %1...</translation>
    </message>
    <message>
        <source>Could not delete %1</source>
        <translation>Не могу удалить %1</translation>
    </message>
</context>
<context>
    <name>Utils</name>
    <message>
        <source>&lt;li&gt;Permissions insufficient for bootloader installation.
Administrator priviledges are necessary.&lt;/li&gt;</source>
        <translation>&lt;li&gt;Недостаточные полномочия для установки загрузчика.
Нужны полномочия администратора.&lt;/li&gt;</translation>
    </message>
    <message>
        <source>Problem detected:</source>
        <translation>Обнаружена проблема:</translation>
    </message>
    <message>
        <source>&lt;li&gt;Target mismatch detected.&lt;br/&gt;Installed target: %1&lt;br/&gt;Selected target: %2.&lt;/li&gt;</source>
        <translation>&lt;li&gt;Обнаруженно несовпадение устройств.&lt;br/&gt;Установленное устройство: %1&lt;br/&gt;Выбранное устройство : %2.&lt;/li&gt;</translation>
    </message>
</context>
<context>
    <name>VoiceFileCreator</name>
    <message>
        <source>Starting Voicefile generation</source>
        <translation>Начинаю вырабатывание голосового файла</translation>
    </message>
    <message>
        <source>Download error: received HTTP error %1.</source>
        <translation>Ошибка скачивания : получена ошибка HTTP %1.</translation>
    </message>
    <message>
        <source>Cached file used.</source>
        <translation>Использован файл из кэша.</translation>
    </message>
    <message>
        <source>Download error: %1</source>
        <translation>Ошибка скачивания : %1</translation>
    </message>
    <message>
        <source>Download finished.</source>
        <translation>Скачивание завершено.</translation>
    </message>
    <message>
        <source>failed to open downloaded file</source>
        <translation>Сбой при открытии скаченного файла</translation>
    </message>
    <message>
        <source>The downloaded file was empty!</source>
        <translation>Скачаный файл пуст!</translation>
    </message>
    <message>
        <source>Error opening downloaded file</source>
        <translation>Сбой при открытии скаченного файла</translation>
    </message>
    <message>
        <source>Error opening output file</source>
        <translation>Сбой при открытии выводного файла</translation>
    </message>
    <message>
        <source>successfully created.</source>
        <translation>успешно создано.</translation>
    </message>
    <message>
        <source>could not find rockbox-info.txt</source>
        <translation>Не могу найти rockbox-info.txt</translation>
    </message>
    <message>
        <source>Downloading voice info...</source>
        <translation>Получаю информацию о голосе...</translation>
    </message>
    <message>
        <source>Reading strings...</source>
        <translation>Читаются значения...</translation>
    </message>
    <message>
        <source>Creating voicefiles...</source>
        <translation>Создаются голосовые файлы...</translation>
    </message>
    <message>
        <source>Cleaning up...</source>
        <translation>Навожу порядок...</translation>
    </message>
    <message>
        <source>Finished</source>
        <translation>Всё</translation>
    </message>
</context>
<context>
    <name>ZipInstaller</name>
    <message>
        <source>done.</source>
        <translation>выполнено.</translation>
    </message>
    <message>
        <source>Installation finished successfully.</source>
        <translation>Установка успешно закончена.</translation>
    </message>
    <message>
        <source>Downloading file %1.%2</source>
        <translation>Скачиваю файл %1.%2</translation>
    </message>
    <message>
        <source>Download error: received HTTP error %1.</source>
        <translation>Сбой скачивания. Ошибка HTTP %1.</translation>
    </message>
    <message>
        <source>Cached file used.</source>
        <translation>Используется файл из кэша.</translation>
    </message>
    <message>
        <source>Download error: %1</source>
        <translation>Сбой скачивания: %1</translation>
    </message>
    <message>
        <source>Download finished.</source>
        <translation>Скачивание завершено.</translation>
    </message>
    <message>
        <source>Extracting file.</source>
        <translation>Извлечение файла.</translation>
    </message>
    <message>
        <source>Installing file.</source>
        <translation>Установка файла.</translation>
    </message>
    <message>
        <source>Installing file failed.</source>
        <translation>Сбой установки файла.</translation>
    </message>
    <message>
        <source>Creating installation log</source>
        <translation>Создаю журнал установки</translation>
    </message>
    <message>
        <source>Not enough disk space! Aborting.</source>
        <translation>Не достаточно дискового пространства! Отмена.</translation>
    </message>
    <message>
        <source>Extraction failed!</source>
        <translation>Ошибка распаковки!</translation>
    </message>
</context>
<context>
    <name>ZipUtil</name>
    <message>
        <source>Creating output path failed</source>
        <translation>Ошибка создания выходной папки</translation>
    </message>
    <message>
        <source>Creating output file failed</source>
        <translation>Ошибка создания выходного файла</translation>
    </message>
    <message>
        <source>Error during Zip operation</source>
        <translation>Ошибка при выполнении операции упаковки\\распаковки архива</translation>
    </message>
</context>
<context>
    <name>aboutBox</name>
    <message>
        <source>About Rockbox Utility</source>
        <translation>О мастере Rockbox</translation>
    </message>
    <message>
        <source>The Rockbox Utility</source>
        <translation>Мастер Rockbox</translation>
    </message>
    <message>
        <source>&amp;Credits</source>
        <translation>&amp;Благодарности</translation>
    </message>
    <message>
        <source>&amp;License</source>
        <translation>&amp;Лицензия</translation>
    </message>
    <message>
        <source>&amp;Ok</source>
        <translation>&amp;OK</translation>
    </message>
    <message utf8="true">
        <source>Installer and housekeeping utility for the Rockbox open source digital audio player firmware.&lt;br/&gt;© 2005 - 2012 The Rockbox Team.&lt;br/&gt;Released under the GNU General Public License v2.&lt;br/&gt;Uses icons by the &lt;a href=&quot;http://tango.freedesktop.org/&quot;&gt;Tango Project&lt;/a&gt;.&lt;br/&gt;&lt;center&gt;&lt;a href=&quot;http://www.rockbox.org&quot;&gt;http://www.rockbox.org&lt;/a&gt;&lt;/center&gt;</source>
        <translation>Мастер установки и администратирования Rockbox, микропрограммы с открытым исходным кодом для цифровых аудиоплееров.&lt;br/&gt;© 2005 - 2012 Команда Rockbox.&lt;br/&gt;Раздаётся по лицензии GNU General Public License v2.&lt;br/&gt;Используются иконки из &lt;a href=&quot;http://tango.freedesktop.org/&quot;&gt;проекта Tango&lt;/a&gt;.&lt;br/&gt;&lt;center&gt;&lt;a href=&quot;http://www.rockbox.org&quot;&gt;http://www.rockbox.org&lt;/a&gt;&lt;/center&gt;</translation>
    </message>
    <message>
        <source>&amp;Speex License</source>
        <translation>&amp;Лицензия Speex</translation>
    </message>
</context>
</TS>
