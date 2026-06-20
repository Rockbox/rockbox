<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="zh_CN">
<context>
    <name>BackupDialog</name>
    <message>
        <location filename="../gui/backupdialogfrm.ui" line="17"/>
        <location filename="../gui/backupdialogfrm.ui" line="43"/>
        <source>Backup</source>
        <translation>备份</translation>
    </message>
    <message>
        <location filename="../gui/backupdialogfrm.ui" line="33"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;This dialog will create a backup by archiving the contents of the Rockbox installation on the player into a zip file. This will include installed themes and settings stored below the .rockbox folder on the player.&lt;/p&gt;&lt;p&gt;The backup filename will be created based on the installed version. &lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <translatorcomment>我真的服了，上一个翻译者到底是谁，随便在中文汉字前面加一个&amp;就当快捷键用，在这种情况下用户必须用输入法选择一个汉字才能触发快捷键，一点也不考虑实际使用场景。我全部改成了和原版一样的英文字母快捷键，远比之前那种方便，而且界面看起来也不会别扭了。——Meduhedan 2026.6.21</translatorcomment>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;此对话框将通过将播放器上安装的 Rockbox 内容存档到 zip 文件中来创建备份。这将包括已安装的主题和存储在播放器的 .rockbox 文件夹下的设置。&lt;/p&gt;&lt;p&gt;备份文件名将根据已安装的版本创建。 &lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location filename="../gui/backupdialogfrm.ui" line="49"/>
        <source>Size: unknown</source>
        <translation>大小：未知</translation>
    </message>
    <message>
        <location filename="../gui/backupdialogfrm.ui" line="56"/>
        <source>Backup to: unknown</source>
        <translation>备份到：未知</translation>
    </message>
    <message>
        <location filename="../gui/backupdialogfrm.ui" line="76"/>
        <source>&amp;Change</source>
        <translation>更改(&amp;C)</translation>
    </message>
    <message>
        <location filename="../gui/backupdialogfrm.ui" line="116"/>
        <source>&amp;Backup</source>
        <translation>备份(&amp;B)</translation>
    </message>
    <message>
        <location filename="../gui/backupdialogfrm.ui" line="127"/>
        <source>&amp;Cancel</source>
        <translation>取消(&amp;C)</translation>
    </message>
    <message>
        <location filename="../gui/backupdialog.cpp" line="69"/>
        <source>Installation size: calculating ...</source>
        <translation>安装大小：计算中……</translation>
    </message>
    <message>
        <location filename="../gui/backupdialog.cpp" line="96"/>
        <source>Select Backup Filename</source>
        <translation>选择备份文件名</translation>
    </message>
    <message>
        <location filename="../gui/backupdialog.cpp" line="116"/>
        <source>Installation size: %L1 %2</source>
        <translation>安装大小 %L1 %2</translation>
    </message>
    <message>
        <location filename="../gui/backupdialog.cpp" line="123"/>
        <source>File exists</source>
        <translation>文件存在</translation>
    </message>
    <message>
        <location filename="../gui/backupdialog.cpp" line="124"/>
        <source>The selected backup file already exists. Overwrite?</source>
        <translation>选定的备份文件已经存在。要覆盖吗？</translation>
    </message>
    <message>
        <location filename="../gui/backupdialog.cpp" line="132"/>
        <source>Starting backup ...</source>
        <translation>开始备份…</translation>
    </message>
    <message>
        <location filename="../gui/backupdialog.cpp" line="151"/>
        <source>Backup successful.</source>
        <translation>备份成功。</translation>
    </message>
    <message>
        <location filename="../gui/backupdialog.cpp" line="154"/>
        <source>Backup failed!</source>
        <translation>备份失败！</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallAms</name>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="33"/>
        <source>Bootloader installation requires you to provide a copy of the original Sandisk firmware (bin file). This firmware file will be patched and then installed to your player along with the rockbox bootloader. You need to download this file yourself due to legal reasons. Please browse the &lt;a href=&apos;http://forums.sandisk.com/sansa/&apos;&gt;Sansa Forums&lt;/a&gt; or refer to the &lt;a href=&apos;https://www.rockbox.org/manual.shtml&apos;&gt;manual&lt;/a&gt; and the &lt;a href=&apos;https://www.rockbox.org/wiki/SansaAMS&apos;&gt;SansaAMS&lt;/a&gt; wiki page on how to obtain this file.&lt;br/&gt;&lt;b&gt;Note:&lt;/b&gt; This file is not present on your player and will disappear automatically after installing it.&lt;br/&gt;&lt;br/&gt;Press Ok to continue and browse your computer for the firmware file.</source>
        <translation>Bootloader 安装需要您提供原始 Sandisk 固件的副本（bin 文件）。此固件文件将被修补，然后与rockbox bootloader一起安装到您的播放器中。由于法律原因，您需要自行下载此文件。请浏览&lt;a href=&apos;http://forums.sandisk.com/sansa/&apos;&gt;Sansa Forums&lt;/a&gt; 或参阅 &lt;a href=&apos;https://www.rockbox.org/manual.shtml&apos;&gt;manual&lt;/a&gt; 和 &lt;a href=&apos;https://www.rockbox.org/wiki/SansaAMS&apos;&gt;SansaAMS&lt;/a&gt; wiki 页面以获取此文件。&lt;br/&gt;&lt;b&gt;Note:&lt;/b&gt;此文件在您的播放器上不保存，安装后将自动消失。&lt;br/&gt;&lt;br/&gt;按确定继续并浏览您的计算机以查找固件文件。</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="58"/>
        <source>Downloading bootloader file</source>
        <translation>正在下载bootloader文件</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="100"/>
        <location filename="../base/bootloaderinstallams.cpp" line="113"/>
        <source>Could not load %1</source>
        <translation>无法加载 %1</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="127"/>
        <source>No room to insert bootloader, try another firmware version</source>
        <translation>没有空间插入bootloader，请尝试另一个固件版本</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="137"/>
        <source>Patching Firmware...</source>
        <translation>正在修补固件…</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="148"/>
        <source>Could not open %1 for writing</source>
        <translation>无法打开 %1 来写入</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="161"/>
        <source>Could not write firmware file</source>
        <translation>无法写固件文件</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="177"/>
        <source>Success: modified firmware file created</source>
        <translation>成功：已创建修改的固件文件</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="185"/>
        <source>To uninstall, perform a normal upgrade with an unmodified original firmware</source>
        <translation>要卸载，请使用未修改的原始固件执行正常升级</translation>
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
        <translation type="unfinished">无法读取原始固件文件</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbspatch.cpp" line="90"/>
        <source>Downloading bootloader file</source>
        <translation type="unfinished">正在下载bootloader文件</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbspatch.cpp" line="99"/>
        <source>Patching file...</source>
        <translation type="unfinished">修补文件…</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbspatch.cpp" line="124"/>
        <source>Patching the original firmware failed</source>
        <translation type="unfinished">修补原始固件失败</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbspatch.cpp" line="130"/>
        <source>Succesfully patched firmware file</source>
        <translation type="unfinished">修补原始固件成功</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbspatch.cpp" line="145"/>
        <source>Bootloader successful installed</source>
        <translation type="unfinished">Bootloader安装成功</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbspatch.cpp" line="151"/>
        <source>Patched bootloader could not be installed</source>
        <translation type="unfinished">修补的bootloader无法被安装</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbspatch.cpp" line="161"/>
        <source>To uninstall, perform a normal upgrade with an unmodified original firmware.</source>
        <translation type="unfinished">要卸载，请使用未修改的原始固件执行正常升级。</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallBase</name>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="69"/>
        <source>Download error: received HTTP error %1.</source>
        <translation>下载错误: 接到 HTTP 错误 %1。</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="75"/>
        <source>Download error: %1</source>
        <translation>下载错误: %1</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="81"/>
        <source>Download finished (cache used).</source>
        <translation>完成下载（已使用缓存）。</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="83"/>
        <source>Download finished.</source>
        <translation>完成下载。</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="112"/>
        <source>Creating backup of original firmware file.</source>
        <translation>正在创建原始固件文件备份。</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="114"/>
        <source>Creating backup folder failed</source>
        <translation>创建备份文件夹失败</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="120"/>
        <source>Creating backup copy failed.</source>
        <translation>创建备份副本失败。</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="123"/>
        <source>Backup created.</source>
        <translation>备份已创建。</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="140"/>
        <source>Creating installation log</source>
        <translation>正在建立安装日志</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="245"/>
        <source>Zip file format detected</source>
        <translation>检测到ZIP文件格式</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="257"/>
        <source>CAB file format detected</source>
        <translation>检测到CAB文件格式</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="278"/>
        <source>Extracting firmware %1 from archive</source>
        <translation>从存档中提取固件 %1</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="285"/>
        <source>Error extracting firmware from archive</source>
        <translation>从存档中提取固件时失败</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="294"/>
        <source>Could not find firmware in archive</source>
        <translation>无法在存档中找到固件</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="162"/>
        <source>Waiting for system to remount player</source>
        <translation>等待系统卸载播放器</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="192"/>
        <source>Player remounted</source>
        <translation>播放器已卸载</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="197"/>
        <source>Timeout on remount</source>
        <translation>卸载超时</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="152"/>
        <source>Installation log created</source>
        <translation>安装日志已创建</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallChinaChip</name>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="33"/>
        <source>Bootloader installation requires you to provide a firmware file of the original firmware (HXF file). You need to download this file yourself due to legal reasons. Please refer to the &lt;a href=&apos;https://www.rockbox.org/manual.shtml&apos;&gt;manual&lt;/a&gt; and the &lt;a href=&apos;https://www.rockbox.org/wiki/OndaVX747#Download_and_extract_a_recent_ve&apos;&gt;OndaVX747&lt;/a&gt; wiki page on how to obtain this file.&lt;br/&gt;Press Ok to continue and browse your computer for the firmware file.</source>
        <translation>Bootloader 安装需要您提供原始固件的固件文件（HXF 文件）。由于法律原因，您需要自行下载此文件。有关如何获取此文件，请参阅 &lt;a href=&apos;https://www.rockbox.org/manual.shtml&apos;&gt;manual&lt;/a&gt; 和 &lt;a href=&apos;https://www.rockbox.org/wiki/OndaVX747#Download_and_extract_a_recent_ve&apos;&gt;OndaVX747&lt;/a&gt; wiki 页面。&lt;br/&gt;按确定继续并浏览您的计算机以查找固件文件。</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="50"/>
        <source>Downloading bootloader file</source>
        <translation>正在下载bootloader文件</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="75"/>
        <source>Could not open firmware file</source>
        <translation>无法打开固件文件</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="78"/>
        <source>Could not open bootloader file</source>
        <translation>无法打开bootloader文件</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="81"/>
        <source>Could not allocate memory</source>
        <translation>无法分配内存</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="84"/>
        <source>Could not load firmware file</source>
        <translation>无法加载固件文件</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="87"/>
        <source>File is not a valid ChinaChip firmware</source>
        <translation>所选文件并非有效的ChinaChip固件</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="90"/>
        <source>Could not find ccpmp.bin in input file</source>
        <translation>无法在输入文件中找到ccpmp.bin</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="93"/>
        <source>Could not open backup file for ccpmp.bin</source>
        <translation>无法为ccpmp.bin打开备份文件</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="96"/>
        <source>Could not write backup file for ccpmp.bin</source>
        <translation>无法为ccpmp.bin写备份文件</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="99"/>
        <source>Could not load bootloader file</source>
        <translation>无法加载bootloader文件</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="102"/>
        <source>Could not get current time</source>
        <translation>无法获取当前时间</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="105"/>
        <source>Could not open output file</source>
        <translation>无法打开输出文件</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="108"/>
        <source>Could not write output file</source>
        <translation>无法写输出文件</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="111"/>
        <source>Unexpected error from chinachippatcher</source>
        <translation>来自chinachippatcher的意外错误</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallFile</name>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="34"/>
        <source>Downloading bootloader</source>
        <translation>正在下载bootloader</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="43"/>
        <source>Installing Rockbox bootloader</source>
        <translation>正在安装Rockbox bootloader</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="75"/>
        <source>Error accessing output folder</source>
        <translation>访问输出文件夹时出错</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="89"/>
        <source>A firmware file is already present on player</source>
        <translation>固件文件已存在于播放器上</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="94"/>
        <source>Bootloader successful installed</source>
        <translation>Bootloader安装成功</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="97"/>
        <source>Copying modified firmware file failed</source>
        <translation>复制修改后的固件文件失败</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="111"/>
        <source>Removing Rockbox bootloader</source>
        <translation>正在移除Rockbox bootloader</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="115"/>
        <source>No original firmware file found.</source>
        <translation>未找到原始固件文件。</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="121"/>
        <source>Can&apos;t remove Rockbox bootloader file.</source>
        <translation>无法移除Rockbox bootloader文件。</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="126"/>
        <source>Can&apos;t restore bootloader file.</source>
        <translation>无法恢复bootloader文件。</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="130"/>
        <source>Original bootloader restored successfully.</source>
        <translation>原始bootloader恢复成功。</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallHex</name>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="69"/>
        <source>checking MD5 hash of input file ...</source>
        <translation>检查输入文件MD5…</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="80"/>
        <source>Could not verify original firmware file</source>
        <translation>无法验证原始固件文件</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="95"/>
        <source>Firmware file not recognized.</source>
        <translation>固件文件无法识别。</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="99"/>
        <source>MD5 hash ok</source>
        <translation>MD5检查通过</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="106"/>
        <source>Firmware file doesn&apos;t match selected player.</source>
        <translation>固件文件与选定的播放器不匹配。</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="111"/>
        <source>Descrambling file</source>
        <translation>解密文件</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="119"/>
        <source>Error in descramble: %1</source>
        <translation>解密错误：%1</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="124"/>
        <source>Downloading bootloader file</source>
        <translation>正在下载bootloader文件</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="134"/>
        <source>Adding bootloader to firmware file</source>
        <translation>正在给固件文件添加bootloader</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="172"/>
        <source>could not open input file</source>
        <translation>无法打开输入文件</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="173"/>
        <source>reading header failed</source>
        <translation>读取标头失败</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="174"/>
        <source>reading firmware failed</source>
        <translation>读取固件失败</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="175"/>
        <source>can&apos;t open bootloader file</source>
        <translation>无法打开bootloader文件</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="176"/>
        <source>reading bootloader file failed</source>
        <translation>读取bootloader文件失败</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="177"/>
        <source>can&apos;t open output file</source>
        <translation>无法创建输出文件</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="178"/>
        <source>writing output file failed</source>
        <translation>写出文件失败</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="180"/>
        <source>Error in patching: %1</source>
        <translation>修补时出错： %1</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="191"/>
        <source>Error in scramble: %1</source>
        <translation>解密时出错： %1</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="206"/>
        <source>Checking modified firmware file</source>
        <translation>正在检查修改后的固件文件</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="208"/>
        <source>Error: modified file checksum wrong</source>
        <translation>错误：修改的文件校验和错误</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="215"/>
        <source>A firmware file is already present on player</source>
        <translation>固件文件已存在于播放器上</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="220"/>
        <source>Success: modified firmware file created</source>
        <translation>成功：已创建修改后的固件文件</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="223"/>
        <source>Copying modified firmware file failed</source>
        <translation>复制修改后的固件文件失败</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="237"/>
        <source>Uninstallation not possible, only installation info removed</source>
        <translation>无法卸载，仅删除安装信息</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="259"/>
        <source>Can&apos;t open input file</source>
        <translation>无法打开输入文件</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="260"/>
        <source>Can&apos;t open output file</source>
        <translation>无法打开输出文件</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="261"/>
        <source>invalid file: header length wrong</source>
        <translation>无效文件：标头长度错误</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="262"/>
        <source>invalid file: unrecognized header</source>
        <translation>无效文件：标头无法识别</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="263"/>
        <source>invalid file: &quot;length&quot; field wrong</source>
        <translation>无效文件：“length”区域错误</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="264"/>
        <source>invalid file: &quot;length2&quot; field wrong</source>
        <translation>无效文件：“length2”区域错误</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="265"/>
        <source>invalid file: internal checksum error</source>
        <translation>无效文件：内部校验和出错</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="266"/>
        <source>invalid file: &quot;length3&quot; field wrong</source>
        <translation>无效文件：“length3”区域错误</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="267"/>
        <source>unknown</source>
        <translation>不明</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="50"/>
        <source>Bootloader installation requires you to provide a firmware file of the original firmware (hex file). You need to download this file yourself due to legal reasons. Please refer to the &lt;a href=&apos;https://www.rockbox.org/manual.shtml&apos;&gt;manual&lt;/a&gt; and the &lt;a href=&apos;https://www.rockbox.org/wiki/IriverBoot#Download_and_extract_a_recent_ve&apos;&gt;IriverBoot&lt;/a&gt; wiki page on how to obtain this file.&lt;br/&gt;Press Ok to continue and browse your computer for the firmware file.</source>
        <translation>Bootloader安装需要提供一个原始固件的文件（二进制文件）。由于法律原因，您需要自行下载此文件。有关如何获取此文件，请参阅&lt;a href=&apos;https://www.rockbox.org/manual.shtml&apos;&gt;manual&lt;/a&gt;和&lt;a href=&apos;https://www.rockbox.org/wiki/IriverBoot#Download_and_extract_a_recent_ve&apos;&gt;IriverBoot&lt;/a&gt;wiki页面。按确定继续并浏览您的计算机以获取固件文件。</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallImx</name>
    <message>
        <source>Bootloader installation requires you to provide a copy of the original Sandisk firmware (firmware.sb file). This file will be patched with the Rockbox bootloader and installed to your player. You need to download this file yourself due to legal reasons. Please browse the &lt;a href=&apos;http://forums.sandisk.com/sansa/&apos;&gt;Sansa Forums&lt;/a&gt; or refer to the &lt;a href= &apos;https://www.rockbox.org/wiki/SansaFuzePlus&apos;&gt;SansaFuzePlus&lt;/a&gt; wiki page on how to obtain this file.&lt;br/&gt;Press Ok to continue and browse your computer for the firmware file.</source>
        <translation type="vanished">Bootloader 安装需要您提供原始 Sandisk 固件的副本（firmware.sb 文件）。此文件将使用 Rockbox 引导加载程序进行修补并安装到您的播放器中。由于法律原因，您需要自行下载此文件。请浏览 &lt;a href=&apos;http://forums.sandisk.com/sansa/&apos;&gt;Sansa Forums&lt;/a&gt; 或参阅 &lt;a href= &apos;https://www.rockbox.org/wiki/SansaFuzePlus&apos;&gt;SansaFuzePlus&lt;/a&gt; wiki 页面以获取此文件。&lt;br/&gt;按确定继续并浏览您的计算机以获取固件文件。</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallimx.cpp" line="72"/>
        <source>Bootloader installation requires you to provide a copy of the original Sandisk firmware (firmware.sb file). This file will be patched with the Rockbox bootloader and installed to your player. You need to download this file yourself due to legal reasons. Please browse the &lt;a href=&apos;http://forums.sandisk.com/sansa/&apos;&gt;Sansa Forums&lt;/a&gt; or refer to the &lt;a href=&apos;https://www.rockbox.org/manual.shtml&apos;&gt;manual&lt;/a&gt; and &lt;a href= &apos;https://www.rockbox.org/wiki/SansaFuzePlus&apos;&gt;SansaFuzePlus&lt;/a&gt; wiki page on how to obtain this file.&lt;br/&gt;Press Ok to continue and browse your computer for the firmware file.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallimx.cpp" line="95"/>
        <source>Could not read original firmware file</source>
        <translation>无法读取原始固件文件</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallimx.cpp" line="101"/>
        <source>Downloading bootloader file</source>
        <translation>正在下载bootloader文件</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallimx.cpp" line="111"/>
        <source>Patching file...</source>
        <translation>修补文件…</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallimx.cpp" line="137"/>
        <source>Patching the original firmware failed</source>
        <translation>修补原始固件失败</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallimx.cpp" line="143"/>
        <source>Succesfully patched firmware file</source>
        <translation>修补原始固件成功</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallimx.cpp" line="158"/>
        <source>Bootloader successful installed</source>
        <translation>Bootloader安装成功</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallimx.cpp" line="164"/>
        <source>Patched bootloader could not be installed</source>
        <translation>修补的bootloader无法被安装</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallimx.cpp" line="175"/>
        <source>To uninstall, perform a normal upgrade with an unmodified original firmware.</source>
        <translation>要卸载，请使用未修改的原始固件执行正常升级。</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallIpod</name>
    <message>
        <source>Error: can&apos;t allocate buffer memory!</source>
        <translation type="vanished">错误：无法分配缓冲区内存！</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="72"/>
        <source>Downloading bootloader file</source>
        <translation>正在下载bootloader文件</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="56"/>
        <location filename="../base/bootloaderinstallipod.cpp" line="143"/>
        <source>Failed to read firmware directory</source>
        <translation>读取固件文件夹失败</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="61"/>
        <location filename="../base/bootloaderinstallipod.cpp" line="148"/>
        <source>Unknown version number in firmware (%1)</source>
        <translation>固件版本不明 (%1)</translation>
    </message>
    <message>
        <source>Warning: This is a MacPod, Rockbox only runs on WinPods.
See https://www.rockbox.org/wiki/IpodConversionToFAT32</source>
        <translation type="vanished">警告：这是苹果格式的iPod，Rockbox仅能运行于Windows格式的iPod。参阅https://www.rockbox.org/wiki/IpodConversionToFAT32</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="86"/>
        <location filename="../base/bootloaderinstallipod.cpp" line="155"/>
        <source>Could not open Ipod in R/W mode</source>
        <translation>无法打开iPod读写模式</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="96"/>
        <source>Successfull added bootloader</source>
        <translation>追加bootloader成功</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="107"/>
        <source>Failed to add bootloader</source>
        <translation>追加bootloader失败</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="119"/>
        <source>Bootloader Installation complete.</source>
        <translation>Bootloader安装完成。</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="124"/>
        <source>Writing log aborted</source>
        <translation>写入日志已中止</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="161"/>
        <source>No bootloader detected.</source>
        <translation>找不到启动程序.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="167"/>
        <source>Successfully removed bootloader</source>
        <translation>移除bootloader成功</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="175"/>
        <source>Removing bootloader failed.</source>
        <translation>移除bootloader失败。</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="241"/>
        <source>Error: could not retrieve device name</source>
        <translation>错误：无法检索设备名称</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="257"/>
        <source>Error: no mountpoint specified!</source>
        <translation>错误：未指定挂载点！</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="262"/>
        <source>Could not open Ipod: permission denied</source>
        <translation>无法打开iPod：拒绝访问</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="266"/>
        <source>Could not open Ipod</source>
        <translation>无法打开iPod</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="277"/>
        <source>No firmware partition on disk</source>
        <translation>硬盘上没有固件分区</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="82"/>
        <source>Installing Rockbox bootloader</source>
        <translation>正在安装Rockbox bootloader</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="67"/>
        <source>Warning: This is a MacPod, Rockbox only runs on WinPods. 
See https://www.rockbox.org/wiki/IpodConversionToFAT32</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="134"/>
        <source>Uninstalling bootloader</source>
        <translation>正在卸载bootloader</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="271"/>
        <source>Error reading partition table - possibly not an Ipod</source>
        <translation>读取分区表时出错 - 可能不是 iPod</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallMi4</name>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="34"/>
        <source>Downloading bootloader</source>
        <translation>正在下载bootloader</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="43"/>
        <source>Installing Rockbox bootloader</source>
        <translation>正在安装Rockbox bootloader</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="66"/>
        <source>A firmware file is already present on player</source>
        <translation>固件文件已存在于播放器上</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="71"/>
        <location filename="../base/bootloaderinstallmi4.cpp" line="79"/>
        <source>Bootloader successful installed</source>
        <translation>Bootloader安装成功</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="74"/>
        <source>Copying modified firmware file failed</source>
        <translation>复制修改后的固件文件失败</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="91"/>
        <source>Checking for Rockbox bootloader</source>
        <translation>检查 Rockbox 引导加载程序</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="93"/>
        <source>No Rockbox bootloader found</source>
        <translation>未找到 Rockbox 引导加载程序</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="99"/>
        <source>Checking for original firmware file</source>
        <translation>检查原始固件文件</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="104"/>
        <source>Error finding original firmware file</source>
        <translation>寻找原始固件文件时出错</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="115"/>
        <source>Rockbox bootloader successful removed</source>
        <translation>Rockbox bootloader移除成功</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallMpio</name>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="33"/>
        <source>Bootloader installation requires you to provide a firmware file of the original firmware (bin file). You need to download this file yourself due to legal reasons. Please refer to the &lt;a href=&apos;https://www.rockbox.org/manual.shtml&apos;&gt;manual&lt;/a&gt; and the &lt;a href=&apos;https://www.rockbox.org/wiki/MPIOHD200Port&apos;&gt;MPIOHD200Port&lt;/a&gt; wiki page on how to obtain this file.&lt;br/&gt;Press Ok to continue and browse your computer for the firmware file.</source>
        <translation>Bootloader 安装需要您提供原始固件的固件文件（bin 文件）。由于法律原因，您需要自行下载此文件。有关如何获取此文件，请参阅 &lt;a href=&apos;https://www.rockbox.org/manual.shtml&apos;&gt;manual&lt;/a&gt; 和 &lt;a href=&apos;https://www.rockbox.org/wiki/MPIOHD200Port&apos;&gt;MPIOHD200Port&lt;/a&gt; wiki页面。&lt;br/&gt;按确定继续并浏览您的计算机以查找固件文件。</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="52"/>
        <source>Downloading bootloader file</source>
        <translation>正在下载bootloader文件</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="79"/>
        <source>Could not open the original firmware.</source>
        <translation>无法打开原始固件文件。</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="82"/>
        <source>Could not read the original firmware.</source>
        <translation>无法读取原始挂件文件。</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="85"/>
        <source>Loaded firmware file does not look like MPIO original firmware file.</source>
        <translation>加载的固件文件似乎并不是MPIO原始固件文件。</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="100"/>
        <source>Could not open output file.</source>
        <translation>无法打开输出文件。</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="103"/>
        <source>Could not write output file.</source>
        <translation>无法写出输出文件。</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="106"/>
        <source>Unknown error number: %1</source>
        <translation>未知错误码: %1</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="88"/>
        <source>Could not open downloaded bootloader.</source>
        <translation>无法打开已下载的bootloader。</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="91"/>
        <source>Place for bootloader in OF file not empty.</source>
        <translation>在 OF 文件中放置bootloader的位置不为空。</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="94"/>
        <source>Could not read the downloaded bootloader.</source>
        <translation>无法读取已下载的bootloader。</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="97"/>
        <source>Bootloader checksum error.</source>
        <translation>Bootloader校验出错。</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="111"/>
        <source>Patching original firmware failed: %1</source>
        <translation>修改原始固件出错: %1</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="118"/>
        <source>Success: modified firmware file created</source>
        <translation>成功：已创建魔改固件文件</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="126"/>
        <source>To uninstall, perform a normal upgrade with an unmodified original firmware</source>
        <translation>要卸载，请用未修改的原始固件进行正常升级</translation>
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
        <translation>错误：无法分配缓冲区内存！</translation>
    </message>
    <message>
        <source>Searching for Sansa</source>
        <translation type="vanished">寻找Sansa</translation>
    </message>
    <message>
        <source>Permission for disc access denied!
This is required to install the bootloader</source>
        <translation type="vanished">磁盘拒绝访问！
安装启动引导程序时这是必需的</translation>
    </message>
    <message>
        <source>No Sansa detected!</source>
        <translation type="vanished">未检测到Sansa设备！</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="68"/>
        <source>Downloading bootloader file</source>
        <translation>正在下载bootloader文件</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="60"/>
        <location filename="../base/bootloaderinstallsansa.cpp" line="164"/>
        <source>OLD ROCKBOX INSTALLATION DETECTED, ABORTING.
You must reinstall the original Sansa firmware before running
sansapatcher for the first time.
See https://www.rockbox.org/wiki/SansaE200Install
</source>
        <translation>检测到旧的 ROCKBOX 安装，安装中止。
在首次运行Sansapatcher之前，您必须重新安装原始的 Sansa 固件
请参阅 https://www.rockbox.org/wiki/SansaE200Install
</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="87"/>
        <location filename="../base/bootloaderinstallsansa.cpp" line="174"/>
        <source>Could not open Sansa in R/W mode</source>
        <translation>无法以读写模式打开Sansa</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="114"/>
        <source>Successfully installed bootloader</source>
        <translation>成功安装Bootloader</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="125"/>
        <source>Failed to install bootloader</source>
        <translation>无法安装bootloader</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="138"/>
        <source>Bootloader Installation complete.</source>
        <translation>Bootloader安装完成。</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="143"/>
        <source>Writing log aborted</source>
        <translation>写日志中断</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="244"/>
        <source>Error: could not retrieve device name</source>
        <translation>错误：无法检索设备名称</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="260"/>
        <source>Can&apos;t find Sansa</source>
        <translation>无法找到Sansa</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="265"/>
        <source>Could not open Sansa</source>
        <translation>无法打开Sansa</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="270"/>
        <source>Could not read partition table</source>
        <translation>无法读取分区表</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="277"/>
        <source>Disk is not a Sansa (Error %1), aborting.</source>
        <translation>磁盘并非Sansa (Error %1)，安装中断。</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="180"/>
        <source>Successfully removed bootloader</source>
        <translation>成功移除bootloader</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="188"/>
        <source>Removing bootloader failed.</source>
        <translation>移除bootloader失败。</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="83"/>
        <source>Installing Rockbox bootloader</source>
        <translation>正在安装Rockbox bootloader</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="96"/>
        <source>Checking downloaded bootloader</source>
        <translation>正在校验下载的bootloader文件</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="104"/>
        <source>Bootloader mismatch! Aborting.</source>
        <translation>Bootloader不匹配！安装中止。</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="155"/>
        <source>Uninstalling bootloader</source>
        <translation>正在卸载bootloader</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallTcc</name>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="33"/>
        <source>Bootloader installation requires you to provide a firmware file of the original firmware (bin file). You need to download this file yourself due to legal reasons. Please refer to the &lt;a href=&apos;https://www.rockbox.org/manual.shtml&apos;&gt;manual&lt;/a&gt; and the &lt;a href=&apos;https://www.rockbox.org/wiki/CowonD2Info&apos;&gt;CowonD2Info&lt;/a&gt; wiki page on how to obtain the file.&lt;br/&gt;Press Ok to continue and browse your computer for the firmware file.</source>
        <translation>Bootloader 安装需要您提供原始固件的固件文件（bin 文件）。由于法律原因，您需要自行下载此文件。有关如何获取文件，请参阅 &lt;a href=&apos;https://www.rockbox.org/manual.shtml&apos;&gt;manual&lt;/a&gt; 或 &lt;a href=&apos;https://www.rockbox.org/wiki/CowonD2Info&apos;&gt;CowonD2Info&lt;/a&gt; wiki页面。&lt;br/&gt;按确定键继续并浏览您的计算机以获取固件文件。</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="50"/>
        <source>Downloading bootloader file</source>
        <translation>正在下载bootloader文件</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="82"/>
        <location filename="../base/bootloaderinstalltcc.cpp" line="99"/>
        <source>Could not load %1</source>
        <translation>无法加载 %1</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="90"/>
        <source>Unknown OF file used: %1</source>
        <translation>使用了未知OF文件： %1</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="104"/>
        <source>Patching Firmware...</source>
        <translation>修补固件…</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="111"/>
        <source>Could not patch firmware</source>
        <translation>无法修补固件</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="117"/>
        <source>Could not open %1 for writing</source>
        <translation>无法打开 %1 进行写入</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="126"/>
        <source>Could not write firmware file</source>
        <translation>无法写固件文件</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="131"/>
        <source>Success: modified firmware file created</source>
        <translation>成功：已创建魔改固件文件</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="151"/>
        <source>To uninstall, perform a normal upgrade with an unmodified original firmware</source>
        <translation>要卸载，请用未修改的原始固件进行正常升级</translation>
    </message>
</context>
<context>
    <name>Changelog</name>
    <message>
        <location filename="../gui/changelogfrm.ui" line="17"/>
        <source>Changelog</source>
        <translation>变更日志</translation>
    </message>
    <message>
        <location filename="../gui/changelogfrm.ui" line="39"/>
        <source>Show on startup</source>
        <translation>启动时展示</translation>
    </message>
    <message>
        <location filename="../gui/changelogfrm.ui" line="46"/>
        <source>&amp;Ok</source>
        <translation>&amp;确定</translation>
    </message>
</context>
<context>
    <name>Config</name>
    <message>
        <location filename="../configure.cpp" line="333"/>
        <source>Showing disabled targets</source>
        <translation>展示禁用目标</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="334"/>
        <source>You just enabled showing targets that are marked disabled. Disabled targets are not recommended to end users. Please use this option only if you know what you are doing.</source>
        <translation>您刚刚启用了显示标记为禁用的目标的功能。不建议最终用户禁用目标。请仅在您知道自己在做什么时才使用此选项。</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="528"/>
        <source>Proxy Detection</source>
        <translation>代理检测</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="529"/>
        <source>The System Proxy settings are invalid!
Rockbox Utility can&apos;t work with this proxy settings. Make sure the system proxy is set correctly. Note that &quot;proxy auto-config (PAC)&quot; scripts are not supported by Rockbox Utility. If your system uses this you need to use manual proxy settings.</source>
        <translation>系统代理设置无效！
Rockbox Utility 无法使用此代理设置。请确保系统代理设置正确。请注意，Rockbox Utility 不支持“代理自动配置 （PAC）”脚本。如果您的系统使用此功能，则需要使用手动代理设置。</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="639"/>
        <source>Set Cache Path</source>
        <translation>设置缓存路径</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="787"/>
        <source>%1 &quot;MacPod&quot; found!
Rockbox needs a FAT formatted Ipod (so-called &quot;WinPod&quot;) to run. </source>
        <translation>%1 发现Mac格式的iPod！！
Rockbox需要FAT格式的iPod才能运行。 </translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="778"/>
        <source>%1 in MTP mode found!
You need to change your player to MSC mode for installation. </source>
        <translation>%1 发现处于MTP模式！
你需要将你的播放器置于MSC模式才能安装。 </translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="771"/>
        <source>Detected an unsupported player:
%1
Sorry, Rockbox doesn&apos;t run on your player.</source>
        <translation>检测到不支持的播放器：
%1
抱歉，Rockbox无法在此播放器上运行。</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="858"/>
        <source>Autodetection</source>
        <translation>自动识别</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="859"/>
        <source>Could not detect a Mountpoint.
Select your Mountpoint manually.</source>
        <translation>找不到挂载点
请手动选择你的挂载点。</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="761"/>
        <source>Could not detect a device.
Select your device and Mountpoint manually.</source>
        <translation>无法检测到设备
请手动选择你的设备和挂载点。</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="443"/>
        <location filename="../configure.cpp" line="909"/>
        <source>TTS error</source>
        <translation>TTS错误</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="444"/>
        <location filename="../configure.cpp" line="910"/>
        <source>The selected TTS failed to initialize. You can&apos;t use this TTS.</source>
        <translation>选定的TTS无法初始化。你不能使用此TTS。</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="661"/>
        <source>%1 (%2 GiB of %3 GiB free)</source>
        <translation>%1 (%2 GB 共 %3 GB 空闲)</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="735"/>
        <source>Multiple devices have been detected. Please disconnect all players but one and try again.</source>
        <translation>检测到多个设备。请断开其它播放器的连接并再试一次。</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="738"/>
        <source>Detected devices:</source>
        <translation>检测到设备：</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="743"/>
        <source>(unknown)</source>
        <translation>（未知）</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="745"/>
        <source>%1 at %2</source>
        <translation>%1 位于 %2</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="752"/>
        <source>Note: detecting connected devices might be ambiguous. You might have less devices connected than listed. In this case it might not be possible to detect your player unambiguously.</source>
        <translation>注意：检测连接的设备可能不明确。您连接的设备可能少于列出的设备数量。在这种情况下，可能无法明确地检测到您的播放器。</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="756"/>
        <location filename="../configure.cpp" line="760"/>
        <location filename="../configure.cpp" line="805"/>
        <source>Device Detection</source>
        <translation>设备检测</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="794"/>
        <source>The player contains an incompatible filesystem.
Make sure you selected the correct mountpoint and the player is set up to use a filesystem compatible with Rockbox.</source>
        <translation>播放器包含不兼容的文件系统。
确保您选择了正确的挂载点，并且播放器已设置为使用与 Rockbox 兼容的文件系统。</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="802"/>
        <source>An unknown error occured during player detection.</source>
        <translation>检测设备时发生未知错误。</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="869"/>
        <source>Really delete cache?</source>
        <translation>确定删除缓存吗?</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="870"/>
        <source>Do you really want to delete the cache? Make absolutely sure this setting is correct as it will remove &lt;b&gt;all&lt;/b&gt; files in this folder!</source>
        <translation>你真的确定要删除缓存吗? 请确认你的设定是正确的因为这会删除文件夹中 &lt;b&gt;全部&lt;/b&gt; 的文件 !</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="878"/>
        <source>Path wrong!</source>
        <translation>路径错误!</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="879"/>
        <source>The cache path is invalid. Aborting.</source>
        <translation>缓冲路径错误. 正在取消.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="916"/>
        <source>TTS configuration invalid</source>
        <translation>TTS配置无效</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="917"/>
        <source>TTS configuration invalid. 
 Please configure TTS engine.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>TTS configuration invalid.
 Please configure TTS engine.</source>
        <translation type="vanished">TTS 配置无效。
 请配置 TTS 引擎。</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="922"/>
        <source>Could not start TTS engine.</source>
        <translation>无法启动TTS引擎。</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="923"/>
        <source>Could not start TTS engine.
</source>
        <translation>无法启动TTS引擎。
</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="924"/>
        <location filename="../configure.cpp" line="943"/>
        <source>
Please configure TTS engine.</source>
        <translation>
请配置TTS引擎。</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="938"/>
        <source>Rockbox Utility Voice Test</source>
        <translation>Rockbox实用程序语音测试</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="941"/>
        <source>Could not voice test string.</source>
        <translation>无法读出测试字段。</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="942"/>
        <source>Could not voice test string.
</source>
        <translation>无法读出测试字段。
</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="315"/>
        <source>Current cache size is %L1 kiB.</source>
        <translation>当前缓存大小 %L1 kB。</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="450"/>
        <location filename="../configure.cpp" line="484"/>
        <source>Configuration OK</source>
        <translation>配置正常</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="460"/>
        <location filename="../configure.cpp" line="489"/>
        <source>Configuration INVALID</source>
        <translation>配置无效</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="125"/>
        <source>The following errors occurred:</source>
        <translation>下述错误发生：</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="170"/>
        <source>No mountpoint given</source>
        <translation>未指定挂载点</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="174"/>
        <source>Mountpoint does not exist</source>
        <translation>挂载点不存在</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="178"/>
        <source>Mountpoint is not a directory.</source>
        <translation>挂载点不是一个目录。</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="182"/>
        <source>Mountpoint is not writeable</source>
        <translation>挂载点无法写入</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="196"/>
        <source>No player selected</source>
        <translation>未指定播放器</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="209"/>
        <source>Cache path not writeable. Leave path empty to default to systems temporary path.</source>
        <translation>缓存路径不可写。将路径留空以默认为系统临时路径。</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="228"/>
        <source>You need to fix the above errors before you can continue.</source>
        <translation>继续之前你需要解决以上错误。</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="231"/>
        <source>Configuration error</source>
        <translation>配置错误</translation>
    </message>
</context>
<context>
    <name>ConfigForm</name>
    <message>
        <location filename="../configurefrm.ui" line="14"/>
        <source>Configuration</source>
        <translation>设置</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="20"/>
        <source>Configure Rockbox Utility</source>
        <translation>设置 Rockbox 安装程序</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="35"/>
        <source>&amp;Device</source>
        <translation>设备(&amp;D)</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="41"/>
        <source>Select your device in the &amp;filesystem</source>
        <translation>请在文件系统中选择你的播放器（&amp;F）</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="326"/>
        <source>&amp;Browse</source>
        <translation>浏览(&amp;B)</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="72"/>
        <source>&amp;Select your audio player</source>
        <translation>选择你的音乐播放器(&amp;S)</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="58"/>
        <source>&amp;Refresh</source>
        <translation>刷新(&amp;R)</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="114"/>
        <source>&amp;Autodetect</source>
        <translation>自动识别(&amp;A)</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="136"/>
        <source>&amp;Proxy</source>
        <translation>网络代理服务(&amp;P)</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="142"/>
        <source>&amp;No Proxy</source>
        <translation>没有网络代理服务(&amp;N)</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="152"/>
        <source>Use S&amp;ystem values</source>
        <translation>使用系统值(&amp;S)</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="159"/>
        <source>&amp;Manual Proxy settings</source>
        <translation>手动设置代理服务(&amp;M)</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="166"/>
        <source>Proxy Values</source>
        <translation>代理服务值</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="172"/>
        <source>&amp;Host:</source>
        <translation>主机(&amp;H):</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="182"/>
        <source>&amp;Port:</source>
        <translation>接口(&amp;P):</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="199"/>
        <source>&amp;Username</source>
        <translation>用户名(&amp;U)</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="209"/>
        <source>Pass&amp;word</source>
        <translation>密码(&amp;w)</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="219"/>
        <source>Show</source>
        <translation>展示</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="267"/>
        <source>&amp;Language</source>
        <translation>语言(&amp;L)</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="281"/>
        <source>Cac&amp;he</source>
        <translation>缓存(&amp;h)</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="284"/>
        <source>Download cache settings</source>
        <translation>下载缓冲设置</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="290"/>
        <source>Rockbox Utility uses a local download cache to save network traffic. You can change the path to the cache and use it as local repository by enabling Offline mode.</source>
        <translation>Rockbox 安装程序使用本机缓冲来保存网络资料. 你可以改变这个缓冲的路径. 启动离线模式后, 你还可以用路径来保存文件.</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="300"/>
        <source>Current cache size is %1</source>
        <translation>现在缓冲大小是 %1</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="309"/>
        <source>P&amp;ath</source>
        <translation>路径(&amp;a)</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="319"/>
        <source>Entering an invalid folder will reset the path to the systems temporary path.</source>
        <translation>输入无效地址会重设到系统临时文件夹.</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="341"/>
        <source>Disable local &amp;download cache</source>
        <translation>&amp;不使用本机缓存</translation>
    </message>
    <message>
        <source>&lt;p&gt;This will try to use all information from the cache, even information about updates. Only use this option if you want to install without network connection. Note: you need to do the same install you want to perform later with network access first to download all required files to the cache.&lt;/p&gt;</source>
        <translation type="obsolete">&lt;p&gt;这将会使用所有缓存数据，即使是更新信息。仅当你想要断网安装时才使用此选项。注意：您需要先执行要稍后通过网络访问执行的相同安装，以将所有必需的文件下载到缓存中。&lt;/p&gt;</translation>
    </message>
    <message>
        <source>O&amp;ffline mode</source>
        <translation type="obsolete">离线模式(&amp;f)</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="376"/>
        <source>Clean cache &amp;now</source>
        <translation>现在清除缓冲文件夹(&amp;n)</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="392"/>
        <source>&amp;TTS &amp;&amp; Encoder</source>
        <translation>&amp;TTS &amp;&amp; 信号转换器</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="398"/>
        <source>TTS Engine</source>
        <translation>TTS 引擎</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="452"/>
        <source>Test TTS</source>
        <translation>测试TTS</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="463"/>
        <source>&amp;Use string corrections for TTS</source>
        <translation>为TTS使用字段修正(&amp;U)</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="473"/>
        <source>Encoder Engine</source>
        <translation>编码器</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="547"/>
        <source>&amp;Ok</source>
        <translation>确定(&amp;O)</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="558"/>
        <source>&amp;Cancel</source>
        <translation>取消(&amp;C)</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="404"/>
        <source>&amp;Select TTS Engine</source>
        <translation>选择TTS引擎(&amp;S)</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="95"/>
        <source>Show disabled targets</source>
        <translation>展示禁用目标</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="417"/>
        <source>Configure TTS Engine</source>
        <translation>配置TTS引擎</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="424"/>
        <location filename="../configurefrm.ui" line="479"/>
        <source>Configuration invalid!</source>
        <translation>配置无效！</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="441"/>
        <source>Configure &amp;TTS</source>
        <translation>配置&amp;TTS</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="496"/>
        <source>Configure &amp;Enc</source>
        <translation>配置编码器（&amp;E）</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="507"/>
        <source>encoder name</source>
        <translation>编码器名称</translation>
    </message>
</context>
<context>
    <name>Configure</name>
    <message>
        <location filename="../configure.cpp" line="586"/>
        <source>English</source>
        <comment>This is the localized language name, i.e. your language.</comment>
        <translation>简体中文 (Chinese Simplified)</translation>
    </message>
</context>
<context>
    <name>CreateVoiceFrm</name>
    <message>
        <location filename="../createvoicefrm.ui" line="17"/>
        <source>Create Voice File</source>
        <translation>创建语音文件</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="42"/>
        <source>Select the Language you want to generate a voicefile for:</source>
        <translation>选择你想要创建语音文件的语言：</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="55"/>
        <source>TTS:</source>
        <translation>TTS：</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="105"/>
        <source>Silence threshold</source>
        <translation>静默阈值</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="167"/>
        <source>Language</source>
        <translation>语言</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="49"/>
        <source>Generation settings</source>
        <translation>语音合成设置</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="72"/>
        <source>Change</source>
        <translation>变更</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="143"/>
        <source>&amp;Install</source>
        <translation>安装(&amp;I)</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="154"/>
        <source>&amp;Cancel</source>
        <translation>取消(&amp;C)</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="92"/>
        <source>Wavtrim Threshold</source>
        <translation>波形修剪阈值</translation>
    </message>
</context>
<context>
    <name>CreateVoiceWindow</name>
    <message>
        <location filename="../createvoicewindow.cpp" line="115"/>
        <source>TTS error</source>
        <translation>TTS错误</translation>
    </message>
    <message>
        <location filename="../createvoicewindow.cpp" line="116"/>
        <source>The selected TTS failed to initialize. You can&apos;t use this TTS.</source>
        <translation>选定的TTS无法初始化。您无法使用此TTS。</translation>
    </message>
    <message>
        <location filename="../createvoicewindow.cpp" line="120"/>
        <location filename="../createvoicewindow.cpp" line="123"/>
        <source>Engine: &lt;b&gt;%1&lt;/b&gt;</source>
        <translation>引擎 : &lt;b&gt;%1&lt;/b&gt;</translation>
    </message>
</context>
<context>
    <name>EncTtsCfgGui</name>
    <message>
        <location filename="../encttscfggui.cpp" line="43"/>
        <source>Waiting for engine...</source>
        <translation>等待引擎…</translation>
    </message>
    <message>
        <location filename="../encttscfggui.cpp" line="90"/>
        <source>Ok</source>
        <translation>确定</translation>
    </message>
    <message>
        <location filename="../encttscfggui.cpp" line="93"/>
        <source>Cancel</source>
        <translation>取消</translation>
    </message>
    <message>
        <location filename="../encttscfggui.cpp" line="256"/>
        <source>Browse</source>
        <translation>浏览</translation>
    </message>
    <message>
        <location filename="../encttscfggui.cpp" line="271"/>
        <source>Refresh</source>
        <translation>刷新</translation>
    </message>
    <message>
        <location filename="../encttscfggui.cpp" line="262"/>
        <source>Select executable</source>
        <translation>选择可执行文件</translation>
    </message>
</context>
<context>
    <name>EncoderExe</name>
    <message>
        <location filename="../base/encoderexe.cpp" line="37"/>
        <source>Path to Encoder:</source>
        <translation>编码器路径：</translation>
    </message>
    <message>
        <location filename="../base/encoderexe.cpp" line="39"/>
        <source>Encoder options:</source>
        <translation>编码器选项：</translation>
    </message>
</context>
<context>
    <name>EncoderLame</name>
    <message>
        <location filename="../base/encoderlame.cpp" line="75"/>
        <location filename="../base/encoderlame.cpp" line="85"/>
        <source>LAME</source>
        <translation>编码器LAME</translation>
    </message>
    <message>
        <location filename="../base/encoderlame.cpp" line="77"/>
        <source>Volume</source>
        <translation>音量</translation>
    </message>
    <message>
        <location filename="../base/encoderlame.cpp" line="81"/>
        <source>Quality</source>
        <translation>质量</translation>
    </message>
    <message>
        <location filename="../base/encoderlame.cpp" line="85"/>
        <source>Could not find libmp3lame!</source>
        <translation>无法找到libmp3lame！</translation>
    </message>
</context>
<context>
    <name>EncoderRbSpeex</name>
    <message>
        <location filename="../base/encoderrbspeex.cpp" line="34"/>
        <source>Volume:</source>
        <translation>音量：</translation>
    </message>
    <message>
        <location filename="../base/encoderrbspeex.cpp" line="36"/>
        <source>Quality:</source>
        <translation>质量：</translation>
    </message>
    <message>
        <location filename="../base/encoderrbspeex.cpp" line="38"/>
        <source>Complexity:</source>
        <translation>复杂性：</translation>
    </message>
    <message>
        <location filename="../base/encoderrbspeex.cpp" line="40"/>
        <source>Use Narrowband:</source>
        <translation>使用窄带：</translation>
    </message>
</context>
<context>
    <name>InfoWidget</name>
    <message>
        <location filename="../gui/infowidget.cpp" line="30"/>
        <location filename="../gui/infowidget.cpp" line="108"/>
        <source>File</source>
        <translation>文件</translation>
    </message>
    <message>
        <location filename="../gui/infowidget.cpp" line="30"/>
        <location filename="../gui/infowidget.cpp" line="108"/>
        <source>Version</source>
        <translation>版本</translation>
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
        <location filename="../gui/infowidgetfrm.ui" line="14"/>
        <source>Info</source>
        <translation>信息</translation>
    </message>
    <message>
        <location filename="../gui/infowidgetfrm.ui" line="20"/>
        <source>Currently installed packages.&lt;br/&gt;&lt;b&gt;Note:&lt;/b&gt; if you manually installed packages this might not be correct!</source>
        <translation>当前已安装的包。&lt;br/&gt;&lt;b&gt;注意：&lt;/b&gt;如果你手动安装了包这将可能不正确！</translation>
    </message>
    <message>
        <location filename="../gui/infowidgetfrm.ui" line="34"/>
        <source>Package</source>
        <translation>包</translation>
    </message>
</context>
<context>
    <name>InstallTalkFrm</name>
    <message>
        <location filename="../installtalkfrm.ui" line="17"/>
        <source>Install Talk Files</source>
        <translation>安装说话文件</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="52"/>
        <source>Generate for files</source>
        <translation>为文件生成</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="85"/>
        <source>Generate for folders</source>
        <translation>为文件夹生成</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="95"/>
        <source>Recurse into folders</source>
        <translation>递归到文件夹中</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="122"/>
        <source>Ignore files</source>
        <translation>忽略文件</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="132"/>
        <source>Skip existing</source>
        <translation>跳过已存在项目</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="174"/>
        <source>Select folders for Talkfile generation (Ctrl for multiselect)</source>
        <translation>选择Talk文件生成目录（Ctrl多选）</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="78"/>
        <source>TTS profile:</source>
        <translation>TTS 设置:</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="36"/>
        <source>Generation options</source>
        <translation>语音合成设置</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="42"/>
        <source>Strip Extensions</source>
        <translation>除去后缀</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="158"/>
        <source>&amp;Cancel</source>
        <translation>取消(&amp;C)</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="115"/>
        <source>Change</source>
        <translation>变更</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="147"/>
        <source>&amp;Install</source>
        <translation>安装（&amp;I）</translation>
    </message>
</context>
<context>
    <name>InstallTalkWindow</name>
    <message>
        <location filename="../installtalkwindow.cpp" line="95"/>
        <source>Empty selection</source>
        <translation>空选择</translation>
    </message>
    <message>
        <location filename="../installtalkwindow.cpp" line="96"/>
        <source>No files or folders selected. Please select files or folders first.</source>
        <translation>未选定文件夹或文件。请先选择文件或文件夹。</translation>
    </message>
    <message>
        <location filename="../installtalkwindow.cpp" line="140"/>
        <source>TTS error</source>
        <translation>TTS错误</translation>
    </message>
    <message>
        <location filename="../installtalkwindow.cpp" line="141"/>
        <source>The selected TTS failed to initialize. You can&apos;t use this TTS.</source>
        <translation>选定的TTS无法初始化。你无法使用这个TTS。</translation>
    </message>
</context>
<context>
    <name>ManualWidget</name>
    <message>
        <source>&lt;a href=&apos;%1&apos;&gt;PDF Manual&lt;/a&gt;</source>
        <translation type="vanished">&lt;a href=&apos;%1&apos;&gt;PDF 用户手册(英文)&lt;/a&gt;</translation>
    </message>
    <message>
        <source>&lt;a href=&apos;%1&apos;&gt;HTML Manual (opens in browser)&lt;/a&gt;</source>
        <translation type="vanished">&lt;a href=&apos;%1&apos;&gt;HTML 用户手册(英文,在浏览器打开)&lt;/a&gt;</translation>
    </message>
    <message>
        <source>Select a device for a link to the correct manual</source>
        <translation type="vanished">请选择你的播放器</translation>
    </message>
    <message>
        <source>&lt;a href=&apos;%1&apos;&gt;Manual Overview&lt;/a&gt;</source>
        <translation type="vanished">&lt;a href=&apos;%1&apos;&gt;用户手册总观&lt;/a&gt;</translation>
    </message>
    <message>
        <source>Confirm download</source>
        <translation type="vanished">确认下载</translation>
    </message>
    <message>
        <source>Do you really want to download the manual? The manual will be saved to the root folder of your player.</source>
        <translation type="vanished">你确认要下载用户手册吗? 用户手册将会被放在你播放器的主目录里.</translation>
    </message>
</context>
<context>
    <name>ManualWidgetFrm</name>
    <message>
        <source>Manual</source>
        <translation type="vanished">用户手册</translation>
    </message>
    <message>
        <source>Read the manual</source>
        <translation type="vanished">查看用户手册</translation>
    </message>
    <message>
        <source>PDF manual</source>
        <translation type="vanished">PDF 用户手册</translation>
    </message>
    <message>
        <source>HTML manual</source>
        <translation type="vanished">HTML 用户手册</translation>
    </message>
    <message>
        <source>Download the manual</source>
        <translation type="vanished">下载用户手册</translation>
    </message>
    <message>
        <source>&amp;PDF version</source>
        <translation type="vanished">&amp;PDF 版本</translation>
    </message>
    <message>
        <source>&amp;HTML version (zip file)</source>
        <translation type="vanished">&amp;HTML 版本 (ZIP文件不)</translation>
    </message>
    <message>
        <source>Down&amp;load</source>
        <translation type="vanished">下载(&amp;L)</translation>
    </message>
</context>
<context>
    <name>MsPackUtil</name>
    <message>
        <location filename="../base/mspackutil.cpp" line="101"/>
        <source>Creating output path failed</source>
        <translation>创建输出目录失败</translation>
    </message>
    <message>
        <location filename="../base/mspackutil.cpp" line="109"/>
        <source>Error during CAB operation</source>
        <translation>CAB操作时出错</translation>
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
        <translation type="unfinished">不可用</translation>
    </message>
    <message>
        <location filename="../base/playerbuildinfo.cpp" line="364"/>
        <source>Unstable</source>
        <translation type="unfinished">不稳定</translation>
    </message>
    <message>
        <location filename="../base/playerbuildinfo.cpp" line="367"/>
        <source>Stable</source>
        <translation type="unfinished">稳定</translation>
    </message>
    <message>
        <location filename="../base/playerbuildinfo.cpp" line="370"/>
        <source>Unknown</source>
        <translation type="unfinished">未知</translation>
    </message>
</context>
<context>
    <name>PreviewFrm</name>
    <message>
        <location filename="../previewfrm.ui" line="16"/>
        <source>Preview</source>
        <translation>预览</translation>
    </message>
</context>
<context>
    <name>ProgressLoggerFrm</name>
    <message>
        <location filename="../progressloggerfrm.ui" line="18"/>
        <location filename="../progressloggerfrm.ui" line="24"/>
        <source>Progress</source>
        <translation>进程</translation>
    </message>
    <message>
        <location filename="../progressloggerfrm.ui" line="63"/>
        <source>Save Log</source>
        <translation>保存日志</translation>
    </message>
    <message>
        <location filename="../progressloggerfrm.ui" line="87"/>
        <source>&amp;Abort</source>
        <translation>&amp;取消</translation>
    </message>
    <message>
        <location filename="../progressloggerfrm.ui" line="37"/>
        <source>progresswindow</source>
        <translation>处理窗口</translation>
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
        <location filename="../progressloggergui.cpp" line="141"/>
        <source>Save system trace log</source>
        <translation>保存系统跟踪日志</translation>
    </message>
    <message>
        <location filename="../progressloggergui.cpp" line="99"/>
        <source>&amp;Abort</source>
        <translation>&amp;取消</translation>
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
        <translation>（未知供应商名称） </translation>
    </message>
    <message>
        <location filename="../base/system.cpp" line="351"/>
        <source>(unknown product name)</source>
        <translation>（产品名未知）</translation>
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
        <translation>Bootloader的安装即将完成。安装 &lt;b&gt;需要&lt;/b&gt; 你手动执行下列步骤：</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhelper.cpp" line="141"/>
        <source>&lt;li&gt;Safely remove your player.&lt;/li&gt;</source>
        <translation>&lt;li&gt;安全地移除你的播放器。&lt;/li&gt;</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhelper.cpp" line="148"/>
        <source>&lt;li&gt;Reboot your player into the original firmware.&lt;/li&gt;&lt;li&gt;Perform a firmware upgrade using the update functionality of the original firmware. Please refer to your player&apos;s manual on details.&lt;br/&gt;&lt;b&gt;Important:&lt;/b&gt; updating the firmware is a critical process that must not be interrupted. &lt;b&gt;Make sure the player is charged before starting the firmware update process.&lt;/b&gt;&lt;/li&gt;&lt;li&gt;After the firmware has been updated reboot your player.&lt;/li&gt;</source>
        <translation>&lt;li&gt;将你的播放器重启到原始固件。&lt;/li&gt;&lt;li&gt;请用原始固件的升级功能进行一次升级。参阅厂商的说明书以获得更多信息。&lt;br/&gt;&lt;b&gt;重要：&lt;/b&gt; 升级过程绝对不可以被中断，&lt;b&gt;进行固件升级之前必须确认播放器已充电。&lt;/b&gt;&lt;/li&gt;&lt;li&gt;固件升级后，重启你的播放器。&lt;/li&gt;</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhelper.cpp" line="159"/>
        <source>&lt;li&gt;Remove any previously inserted microSD card&lt;/li&gt;</source>
        <translation>&lt;li&gt;移除所有先前插入的microSD卡&lt;/li&gt;</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhelper.cpp" line="160"/>
        <source>&lt;li&gt;Disconnect your player. The player will reboot and perform an update of the original firmware. Please refer to your players manual on details.&lt;br/&gt;&lt;b&gt;Important:&lt;/b&gt; updating the firmware is a critical process that must not be interrupted. &lt;b&gt;Make sure the player is charged before disconnecting the player.&lt;/b&gt;&lt;/li&gt;&lt;li&gt;After the firmware has been updated reboot your player.&lt;/li&gt;</source>
        <translation>&lt;li&gt;断开你的播放器。播放器将重启并进行一次原始固件的升级。参阅厂商的说明书以获得更多信息。&lt;br/&gt;&lt;b&gt;重要：&lt;/b&gt; 升级过程绝对不可以被中断，&lt;b&gt;进行固件升级之前必须确认播放器已充电。&lt;/b&gt;&lt;/li&gt;&lt;li&gt;固件升级后，重启你的播放器。&lt;/li&gt;</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhelper.cpp" line="171"/>
        <source>&lt;li&gt;Turn the player off&lt;/li&gt;&lt;li&gt;Insert the charger&lt;/li&gt;</source>
        <translation>&lt;li&gt;关闭播放器&lt;/li&gt;&lt;li&gt;插入充电器&lt;/li&gt;</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhelper.cpp" line="176"/>
        <source>&lt;li&gt;Unplug USB and power adaptors&lt;/li&gt;&lt;li&gt;Hold &lt;i&gt;Power&lt;/i&gt; to turn the player off&lt;/li&gt;&lt;li&gt;Toggle the battery switch on the player&lt;/li&gt;&lt;li&gt;Hold &lt;i&gt;Power&lt;/i&gt; to boot into Rockbox&lt;/li&gt;</source>
        <translation>&lt;li&gt;移除USB和电源适配器。&lt;/li&gt;&lt;li&gt;按住 &lt;i&gt;电源键&lt;/i&gt; 以关闭播放器&lt;/li&gt;&lt;li&gt;在播放器上切换电池开关&lt;/li&gt;&lt;li&gt;按住&lt;i&gt;电源键&lt;/i&gt; 来启动Rockbox。&lt;/li&gt;</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhelper.cpp" line="182"/>
        <source>&lt;p&gt;&lt;b&gt;Note:&lt;/b&gt; You can safely install other parts first, but the above steps are &lt;b&gt;required&lt;/b&gt; to finish the installation!&lt;/p&gt;</source>
        <translation>&lt;p&gt;&lt;b&gt;注意：&lt;/b&gt;你可以先安全地安装其他部分，但是要完成安装，以上的部分是必需的！&lt;/p&gt;</translation>
    </message>
</context>
<context>
    <name>QuaZipFile</name>
    <message>
        <location filename="../quazip/quazipfile.cpp" line="251"/>
        <source>ZIP/UNZIP API error %1</source>
        <translation>ZIP/UNZIP API 出错 %1</translation>
    </message>
</context>
<context>
    <name>RbUtilQt</name>
    <message>
        <location filename="../rbutilqt.cpp" line="212"/>
        <source>Downloading build information, please wait ...</source>
        <translation>正在下载构建信息，请稍后…</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="262"/>
        <source>Can&apos;t get version information!</source>
        <translation>无法取得版本信息！</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="276"/>
        <source>Download build information finished.</source>
        <translation>下载构建信息完成。</translation>
    </message>
    <message>
        <source>Confirm Installation</source>
        <translation type="vanished">确认安装</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="564"/>
        <source>Mount point is wrong!</source>
        <translation>挂载点错误!</translation>
    </message>
    <message>
        <source>No Rockbox installation found</source>
        <translation type="vanished">未找到Rockbox安装</translation>
    </message>
    <message>
        <source>Could not determine the installed Rockbox version. Please install a Rockbox build before installing voice files.</source>
        <translation type="vanished">无法确认已安装的Rockbox版本。请在构建语音文件之前安装一个Rockbox构建。</translation>
    </message>
    <message>
        <source>Do you really want to install the voice file?</source>
        <translation type="vanished">你确认要安装语音文件吗?</translation>
    </message>
    <message>
        <source>No voice file available</source>
        <translation type="vanished">无语音文件可用</translation>
    </message>
    <message>
        <source>The installed version of Rockbox is a development version. Pre-built voices are only available for release versions of Rockbox. Please generate a voice yourself using the &quot;Create voice file&quot; functionality.</source>
        <translation type="vanished">已安装的Rockbox是开发版。预构建的语音文件仅对稳定版可用。请使用&quot;创建语音文件&quot;功能自行生成语音文件。</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="495"/>
        <source>Confirm Uninstallation</source>
        <translation>确认安装</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="496"/>
        <source>Do you really want to uninstall the Bootloader?</source>
        <translation>你确认要卸载启动程序吗?</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="508"/>
        <source>No uninstall method for this target known.</source>
        <translation>对于此目标无已知卸载方法。</translation>
    </message>
    <message>
        <source>Rockbox Utility can not uninstall the bootloader on this target. Try a normal firmware update to remove the booloader.</source>
        <translation type="vanished">Rockbox实用程序无法在此目标上卸载bootloader。请尝试进行普通固件升级来移除bootloader。</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="531"/>
        <source>No Rockbox bootloader found.</source>
        <translation>未找到Rockbox bootloader。</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="550"/>
        <source>Confirm installation</source>
        <translation>确认安装</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="551"/>
        <source>Do you really want to install Rockbox Utility to your player? After installation you can run it from the players hard drive.</source>
        <translation>你确认要安装Rockbox安装程序到你的播放器上吗? 安装后你可以从你播放器硬盘上运行此程序.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="560"/>
        <source>Installing Rockbox Utility</source>
        <translation>安装 Rockbox安装程序</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="578"/>
        <source>Error installing Rockbox Utility</source>
        <translation>安装 Rockbox安装程序错误</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="582"/>
        <source>Installing user configuration</source>
        <translation>安装用户设置</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="586"/>
        <source>Error installing user configuration</source>
        <translation>安装用户设置错误</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="590"/>
        <source>Successfully installed Rockbox Utility.</source>
        <translation>成功安装 Rockbox安装程序.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="646"/>
        <source>Checking for update ...</source>
        <translation>检查更新…</translation>
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
    <message>
        <source>RockboxUtility Update available</source>
        <translation type="vanished">RockboxUtility更新可用</translation>
    </message>
    <message>
        <source>&lt;b&gt;New RockboxUtility Version available.&lt;/b&gt; &lt;br&gt;&lt;br&gt;Download it from here: &lt;a href=&apos;%1&apos;&gt;%2&lt;/a&gt;</source>
        <translation type="vanished">&lt;b&gt;RockboxUtility新版本可用。&lt;/b&gt; &lt;br&gt;&lt;br&gt;请从此处下载：&lt;a href=&apos;%1&apos;&gt;%2&lt;/a&gt;</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="715"/>
        <source>New version of Rockbox Utility available.</source>
        <translation>Rockbox Utility的新版本可用。</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="718"/>
        <source>Rockbox Utility is up to date.</source>
        <translation>Rockbox Utility已更新。</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="741"/>
        <source>Device ejected</source>
        <translation>设备已弹出</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="742"/>
        <source>Device successfully ejected. You may now disconnect the player from the PC.</source>
        <translation>设备已成功弹出。你现在可以从PC上断开播放器了。</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="746"/>
        <source>Ejecting failed</source>
        <translation>弹出失败</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="747"/>
        <source>Ejecting the device failed. Please make sure no programs are accessing files on the device. If ejecting still fails please use your computers eject funtionality.</source>
        <translation>弹出设备失败。请确认没有程序正在设备上存取文件。如果弹出仍然失败，请用你电脑上的弹出功能。</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="393"/>
        <location filename="../rbutilqt.cpp" line="624"/>
        <source>Configuration error</source>
        <translation>配置错误</translation>
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
        <location filename="../rbutilqt.cpp" line="625"/>
        <source>Your configuration is invalid. Please go to the configuration dialog and make sure the selected values are correct.</source>
        <translation>你的配置无效。请去配置对话框并确保选定的值是正确的。</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="386"/>
        <source>This is a new installation of Rockbox Utility, or a new version. The configuration dialog will now open to allow you to setup the program,  or review your settings.</source>
        <translation>这是Rockbox Utility的新安装或者一个新版本。配置对话框现在将打开以允许你设置此程序，或回顾你的设置。</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="104"/>
        <source>Wine detected!</source>
        <translation>检测到Wine！</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="105"/>
        <source>It seems you are trying to run this program under Wine. Please don&apos;t do this, running under Wine will fail. Use the native Linux binary instead.</source>
        <translation>看起来你正在用Wine运行此程序。请勿这样做，否则将会导致错误。请使用Linux构建版本。</translation>
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
        <location filename="../rbutilqt.cpp" line="264"/>
        <source>Can&apos;t get version information.
Network error: %1. Please check your network and proxy settings.</source>
        <translation>无法取得版本信息。
网络错误： %1。请检查你的网络和代理设置。</translation>
    </message>
    <message>
        <source>Warning</source>
        <translation type="vanished">警告</translation>
    </message>
    <message>
        <source>The Application is still downloading Information about new Builds. Please try again shortly.</source>
        <translation type="vanished">应用程序仍在下载新构建的信息。请稍后再试。</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="385"/>
        <source>New installation</source>
        <translation>新安装</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="394"/>
        <source>Your configuration is invalid. This is most likely due to a changed device path. The configuration dialog will now open to allow you to correct the problem.</source>
        <translation>你的配置无效。这很可能是设备路径的改变导致的。配置对话框现在将打开以允许你修正此错误。</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="263"/>
        <source>Network error</source>
        <translation>网络错误</translation>
    </message>
</context>
<context>
    <name>RbUtilQtFrm</name>
    <message>
        <location filename="../rbutilqtfrm.ui" line="14"/>
        <source>Rockbox Utility</source>
        <translation>Rockbox 安装程序</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="59"/>
        <source>Device</source>
        <translation>播放器</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="120"/>
        <source>&amp;Change</source>
        <translation>更改(&amp;C)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="166"/>
        <source>Welcome</source>
        <translation>欢迎</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="163"/>
        <location filename="../rbutilqtfrm.ui" line="620"/>
        <source>&amp;Installation</source>
        <translation>安装(&amp;I)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="171"/>
        <location filename="../rbutilqtfrm.ui" line="413"/>
        <source>&amp;Accessibility</source>
        <translation>无障碍(&amp;A)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="174"/>
        <source>Install accessibility add-ons</source>
        <translation>安装辅助功能</translation>
    </message>
    <message>
        <source>Install Voice files</source>
        <translation type="vanished">安装语音文件</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="180"/>
        <source>Install Talk files</source>
        <translation>安装说话文件</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="420"/>
        <source>&amp;Uninstallation</source>
        <translation>卸载(&amp;U)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="262"/>
        <location filename="../rbutilqtfrm.ui" line="295"/>
        <source>Uninstall Rockbox</source>
        <translation>卸载 Rockbox</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="149"/>
        <source>mountpoint unknown or invalid</source>
        <translation>挂载点未知或无效</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="142"/>
        <source>Mountpoint:</source>
        <translation>挂载点：</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="100"/>
        <source>device unknown or invalid</source>
        <translation>设备未知或无效</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="93"/>
        <source>Device:</source>
        <translation>设备：</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="268"/>
        <source>Uninstall Bootloader</source>
        <translation>卸载启动程序</translation>
    </message>
    <message>
        <source>&amp;Manual</source>
        <translation type="vanished">用户指南(&amp;M)</translation>
    </message>
    <message>
        <source>View and download the manual</source>
        <translation type="vanished">查看和下载用户手册</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="369"/>
        <source>Inf&amp;o</source>
        <translation>信息(&amp;O)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="387"/>
        <source>&amp;File</source>
        <translation>文件(&amp;F)</translation>
    </message>
    <message>
        <source>&amp;Troubleshoot</source>
        <translation type="vanished">疑难解答(&amp;T)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="459"/>
        <source>&amp;About</source>
        <translation>关于(&amp;A)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="615"/>
        <source>System &amp;Trace</source>
        <translation>系统跟踪(&amp;T)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="436"/>
        <source>Empty local download cache</source>
        <translation>清除本机下载缓存</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="441"/>
        <source>Install Rockbox Utility on player</source>
        <translation>在播放器上安装Rockbox Utility</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="446"/>
        <source>&amp;Configure</source>
        <translation>配置(&amp;C)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="451"/>
        <source>E&amp;xit</source>
        <translation>退出(&amp;X)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="454"/>
        <source>Ctrl+Q</source>
        <translation>Ctrl+Q</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="464"/>
        <source>About &amp;Qt</source>
        <translation>关于&amp;Qt</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="396"/>
        <location filename="../rbutilqtfrm.ui" line="469"/>
        <source>&amp;Help</source>
        <translation>帮助(&amp;H)</translation>
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
    <message>
        <location filename="../rbutilqtfrm.ui" line="409"/>
        <source>Action&amp;s</source>
        <translation>操作(&amp;S)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="474"/>
        <source>Info</source>
        <translation>信息</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="578"/>
        <source>Read PDF manual</source>
        <translation>阅读PDF手册</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="583"/>
        <source>Read HTML manual</source>
        <translation>阅读HTML手册</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="588"/>
        <source>Download PDF manual</source>
        <translation>下载PDF手册</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="593"/>
        <source>Download HTML manual (zip)</source>
        <translation>下载HTML手册（ZIP）</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="230"/>
        <source>Create Voice files</source>
        <translation>创建语音文件</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="605"/>
        <source>Create Voice File</source>
        <translation>创建语音文件</translation>
    </message>
    <message>
        <source>&lt;b&gt;Install Voice file&lt;/b&gt;&lt;br/&gt;Voice files are needed to make Rockbox speak the user interface. Speaking is enabled by default, so if you installed the voice file Rockbox will speak.</source>
        <translation type="vanished">&lt;b&gt;安装语音文件&lt;/b&gt;&lt;br/&gt;Rockbox需要语音文件来读出用户界面。语音默认开启，所以一旦你安装了语音文件Rockbox就会说话。</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="131"/>
        <source>&amp;Eject</source>
        <translation>弹出(&amp;E)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="197"/>
        <source>&lt;b&gt;Create Talk Files&lt;/b&gt;&lt;br/&gt;Talkfiles are needed to let Rockbox speak File and Foldernames</source>
        <translation>&lt;b&gt;创建说话文件&lt;/b&gt;&lt;br/&gt;Rockbox需要说话文件来读出文件和文件夹名称</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="247"/>
        <source>&lt;b&gt;Create Voice file&lt;/b&gt;&lt;br/&gt;Voice files are needed to make Rockbox speak the  user interface. Speaking is enabled by default, so
 if you installed the voice file Rockbox will speak.</source>
        <translation>&lt;b&gt;创建语音文件&lt;/b&gt;&lt;br/&gt;Rockbox需要语音文件才能读出用户界面。语音功能默认开启，所以
一旦你安装了语音文件Rockbox就会说话。</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="259"/>
        <source>Backup &amp;&amp; &amp;Uninstallation</source>
        <translation>备份 &amp;&amp; 卸载(&amp;U)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="285"/>
        <source>&lt;b&gt;Remove the bootloader&lt;/b&gt;&lt;br/&gt;After removing the bootloader you won&apos;t be able to start Rockbox.</source>
        <translation>&lt;b&gt;移除bootloader&lt;/b&gt;&lt;br/&gt;移除bootloader之后你将再也无法启动Rockbox。</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="312"/>
        <source>&lt;b&gt;Uninstall Rockbox from your audio player.&lt;/b&gt;&lt;br/&gt;This will leave the bootloader in place (you need to remove it manually).</source>
        <translation>&lt;b&gt;从你的音频播放器上卸载Rockbox。&lt;/b&gt;&lt;br/&gt;这将会保留bootloader (需要手动卸载)。</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="325"/>
        <source>Backup</source>
        <translation>备份</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="342"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;&lt;span style=&quot; font-weight:600;&quot;&gt;Backup current installation.&lt;/span&gt;&lt;/p&gt;&lt;p&gt;Create a backup by archiving the contents of the Rockbox installation folder.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;&lt;span style=&quot; font-weight:600;&quot;&gt;备份当前安装。&lt;/span&gt;&lt;/p&gt;&lt;p&gt;通过压缩Rockbox安装目录中的内容来备份。&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="501"/>
        <source>Install &amp;Bootloader</source>
        <translation>安装Bootloader(&amp;B)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="510"/>
        <source>Install &amp;Rockbox</source>
        <translation>安装Rockbox(&amp;R)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="519"/>
        <source>Install &amp;Fonts Package</source>
        <translation>安装字体包(&amp;F)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="528"/>
        <source>Install &amp;Themes</source>
        <translation>安装主题(&amp;T)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="537"/>
        <source>Install &amp;Game Files</source>
        <translation>安装游戏文件(&amp;G)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="546"/>
        <source>&amp;Install Voice File</source>
        <translation>安装语音文件(&amp;I)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="555"/>
        <source>Create &amp;Talk Files</source>
        <translation>创建说话文件(&amp;T)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="564"/>
        <source>Remove &amp;bootloader</source>
        <translation>移除Bootloader(&amp;B)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="573"/>
        <source>Uninstall &amp;Rockbox</source>
        <translation>卸载Rockbox(&amp;R)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="602"/>
        <source>Create &amp;Voice File</source>
        <translation>创建语音文件(&amp;V)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="610"/>
        <source>&amp;System Info</source>
        <translation>系统信息(&amp;S)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="625"/>
        <source>Show &amp;Changelog</source>
        <translation>显示变更日志(&amp;C)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="483"/>
        <source>&amp;Complete Installation</source>
        <translation>完成安装(&amp;C)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="492"/>
        <source>&amp;Minimal Installation</source>
        <translation>最小安装(&amp;M)</translation>
    </message>
</context>
<context>
    <name>SelectiveInstallWidget</name>
    <message>
        <location filename="../gui/selectiveinstallwidgetfrm.ui" line="20"/>
        <source>Selective Installation</source>
        <translation>可选安装</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidgetfrm.ui" line="26"/>
        <source>Rockbox version to install</source>
        <translation>要安装的Rockbox版本</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidgetfrm.ui" line="35"/>
        <source>Version information not available yet.</source>
        <translation>版本信息还不可用。</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidgetfrm.ui" line="54"/>
        <source>Rockbox components to install</source>
        <translation>要安装的 Rockbox 组件</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidgetfrm.ui" line="60"/>
        <source>&amp;Bootloader</source>
        <translation>&amp;Bootloader</translation>
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
        <translation type="unfinished">用户指南(&amp;M)</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidgetfrm.ui" line="233"/>
        <source>&amp;Voice File</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidgetfrm.ui" line="253"/>
        <source>The main Rockbox firmware.</source>
        <translation>主要的Rockbox固件。</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidgetfrm.ui" line="115"/>
        <source>Fonts</source>
        <translation>字体</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidgetfrm.ui" line="74"/>
        <source>&amp;Rockbox</source>
        <translation>&amp;Rockbox</translation>
    </message>
    <message>
        <source>Some game plugins require additional files.</source>
        <translation type="vanished">一些游戏插件需要额外文件。</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidgetfrm.ui" line="178"/>
        <source>Additional fonts for the User Interface.</source>
        <translation>用户界面的额外字体。</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidgetfrm.ui" line="135"/>
        <source>The bootloader is required for starting Rockbox. Only necessary for first time install.</source>
        <translation>Bootloader是启动Rockbox所必需的。仅第一次安装需要。</translation>
    </message>
    <message>
        <source>Game Files</source>
        <translation type="vanished">游戏文件</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidgetfrm.ui" line="161"/>
        <source>Customize</source>
        <translation>定制</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidgetfrm.ui" line="104"/>
        <source>Themes</source>
        <translation>主题</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidgetfrm.ui" line="94"/>
        <source>Themes allow adjusting the user interface of Rockbox. Use &quot;Customize&quot; to select themes.</source>
        <translation>主题允许更改Rockbox的用户界面。使用定制选项来选择主题。</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidgetfrm.ui" line="263"/>
        <source>Save a copy of the manual on the player.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidgetfrm.ui" line="292"/>
        <source>&amp;Install</source>
        <translation>&amp;安装</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="78"/>
        <source>This is the latest stable release available.</source>
        <translation>这是可用的最新稳定发行版。</translation>
    </message>
    <message>
        <source>The development version is updated on every code change. Last update was on %1</source>
        <translation type="vanished">开发版在每次代码变更时构建。最后一次更新于%1</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="88"/>
        <source>This will eventually become the next Rockbox version. Install it to help testing.</source>
        <translation>这最终将成为下一个 Rockbox 版本。安装它以帮助测试。</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="128"/>
        <source>Stable Release (Version %1)</source>
        <translation>稳定发行版(版本 %1)</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="132"/>
        <source>Development Version (Revison %1)</source>
        <translation>开发板(修订 %1)</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="130"/>
        <source>Release Candidate (Revison %1)</source>
        <translation>候选版本(修订版%1)</translation>
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
        <location filename="../gui/selectiveinstallwidget.cpp" line="159"/>
        <source>The selected player doesn&apos;t need a bootloader.</source>
        <translation>选定的播放器不需要bootloader。</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="164"/>
        <source>The bootloader is required for starting Rockbox. Installation of the bootloader is only necessary on first time installation.</source>
        <translation>Bootloader是启动Rockbox所必需的。仅在第一次安装时才需要安装bootloader。</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="237"/>
        <source>Mountpoint is wrong</source>
        <translation>挂载点错误</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="295"/>
        <source>No install method known.</source>
        <translation>无已知安装方法。</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="319"/>
        <source>Bootloader detected</source>
        <translation>检测到bootloader</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="320"/>
        <source>Bootloader already installed. Do you want to reinstall the bootloader?</source>
        <translation>Bootloader已安装。你想要重新安装他吗？</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="324"/>
        <source>Bootloader installation skipped</source>
        <translation>已跳过Bootloader安装</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="338"/>
        <source>Create Bootloader backup</source>
        <translation>创建Bootloader备份</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="339"/>
        <source>You can create a backup of the original bootloader file. Press &quot;Yes&quot; to select an output folder on your computer to save the file to. The file will get placed in a new folder &quot;%1&quot; created below the selected folder.
Press &quot;No&quot; to skip this step.</source>
        <translation>你可以创建一个原始bootloader的备份。按 &quot;是&quot; 来选定一个保存此文件的目录。文件将会被放入新文件夹&quot;%1&quot;中。
按 &quot;否&quot; 来跳过这一步。</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="346"/>
        <source>Browse backup folder</source>
        <translation>浏览备份文件夹</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="358"/>
        <source>Prerequisites</source>
        <translation>前提条件</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="363"/>
        <source>Bootloader installation aborted</source>
        <translation>Bootloader安装中断</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="373"/>
        <source>Bootloader files (%1)</source>
        <translation>Bootloader 文件 (%1)</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="375"/>
        <source>All files (*)</source>
        <translation>所有文件（*）</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="377"/>
        <source>Select firmware file</source>
        <translation>选择固件文件</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="379"/>
        <source>Error opening firmware file</source>
        <translation>打开固件文件时出错</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="385"/>
        <source>Error reading firmware file</source>
        <translation>读取固件文件时出错</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="395"/>
        <source>Backup error</source>
        <translation>备份出错</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="396"/>
        <source>Could not create backup file. Continue?</source>
        <translation>无法创建备份文件。要继续吗？</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="419"/>
        <location filename="../gui/selectiveinstallwidget.cpp" line="431"/>
        <source>Manual steps required</source>
        <translation>需要手动步骤</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="667"/>
        <source>Your installation doesn&apos;t require any plugin data files, skipping.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="225"/>
        <source>Continue with installation?</source>
        <translation>继续安装？</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="226"/>
        <source>Really continue?</source>
        <translation>真的继续？</translation>
    </message>
    <message>
        <source>Your installation doesn&apos;t require game files, skipping.</source>
        <translation type="vanished">你的安装不需要游戏文件，已跳过。</translation>
    </message>
</context>
<context>
    <name>ServerInfo</name>
    <message>
        <source>Unknown</source>
        <translation type="vanished">未知</translation>
    </message>
    <message>
        <source>Unusable</source>
        <translation type="vanished">不可用</translation>
    </message>
    <message>
        <source>Unstable</source>
        <translation type="vanished">不稳定</translation>
    </message>
    <message>
        <source>Stable</source>
        <translation type="vanished">稳定</translation>
    </message>
</context>
<context>
    <name>SysTrace</name>
    <message>
        <location filename="../systrace.cpp" line="100"/>
        <location filename="../systrace.cpp" line="109"/>
        <source>Save system trace log</source>
        <translation>保存系统追踪日志</translation>
    </message>
</context>
<context>
    <name>SysTraceFrm</name>
    <message>
        <location filename="../systracefrm.ui" line="14"/>
        <source>System Trace</source>
        <translation>系统跟踪</translation>
    </message>
    <message>
        <location filename="../systracefrm.ui" line="20"/>
        <source>System State trace</source>
        <translation>系统状态跟踪</translation>
    </message>
    <message>
        <location filename="../systracefrm.ui" line="46"/>
        <source>&amp;Close</source>
        <translation>&amp;关闭</translation>
    </message>
    <message>
        <location filename="../systracefrm.ui" line="57"/>
        <source>&amp;Save</source>
        <translation>&amp;保存</translation>
    </message>
    <message>
        <location filename="../systracefrm.ui" line="68"/>
        <source>&amp;Refresh</source>
        <translation>&amp;刷新</translation>
    </message>
    <message>
        <location filename="../systracefrm.ui" line="79"/>
        <source>Save &amp;previous</source>
        <translation>保存&amp;先前</translation>
    </message>
</context>
<context>
    <name>Sysinfo</name>
    <message>
        <location filename="../sysinfo.cpp" line="46"/>
        <source>&lt;b&gt;OS&lt;/b&gt;&lt;br/&gt;</source>
        <translation>&lt;b&gt;操作系统&lt;/b&gt;&lt;br/&gt;</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="47"/>
        <source>&lt;b&gt;Username&lt;/b&gt;&lt;br/&gt;%1&lt;hr/&gt;</source>
        <translation>&lt;b&gt;用户名&lt;/b&gt;&lt;br/&gt;%1&lt;hr/&gt;</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="49"/>
        <source>&lt;b&gt;Permissions&lt;/b&gt;&lt;br/&gt;%1&lt;hr/&gt;</source>
        <translation>&lt;b&gt;许可&lt;/b&gt;&lt;br/&gt;%1&lt;hr/&gt;</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="51"/>
        <source>&lt;b&gt;Attached USB devices&lt;/b&gt;&lt;br/&gt;</source>
        <translation>&lt;b&gt;连接的USB设备&lt;/b&gt;&lt;br/&gt;</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="55"/>
        <source>VID: %1 PID: %2, %3</source>
        <translation>VID: %1 PID: %2, %3</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="64"/>
        <source>Filesystem</source>
        <translation>文件系统</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="67"/>
        <source>Mountpoint</source>
        <translation>挂载点</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="67"/>
        <source>Label</source>
        <translation>标签</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="68"/>
        <source>Free</source>
        <translation>剩余空间</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="68"/>
        <source>Total</source>
        <translation>总空间</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="69"/>
        <source>Type</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Cluster Size</source>
        <translation type="vanished">集群大小</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="71"/>
        <source>&lt;tr&gt;&lt;td&gt;%1&lt;/td&gt;&lt;td&gt;%4&lt;/td&gt;&lt;td&gt;%2 GiB&lt;/td&gt;&lt;td&gt;%3 GiB&lt;/td&gt;&lt;td&gt;%5&lt;/td&gt;&lt;/tr&gt;</source>
        <translation>&lt;tr&gt;&lt;td&gt;%1&lt;/td&gt;&lt;td&gt;%4&lt;/td&gt;&lt;td&gt;%2 GiB&lt;/td&gt;&lt;td&gt;%3 GiB&lt;/td&gt;&lt;td&gt;%5&lt;/td&gt;&lt;/tr&gt;</translation>
    </message>
</context>
<context>
    <name>SysinfoFrm</name>
    <message>
        <location filename="../sysinfofrm.ui" line="13"/>
        <source>System Info</source>
        <translation>系统信息</translation>
    </message>
    <message>
        <location filename="../sysinfofrm.ui" line="22"/>
        <source>&amp;Refresh</source>
        <translation>&amp;刷新</translation>
    </message>
    <message>
        <location filename="../sysinfofrm.ui" line="46"/>
        <source>&amp;OK</source>
        <translation>&amp;确定</translation>
    </message>
</context>
<context>
    <name>System</name>
    <message>
        <location filename="../base/system.cpp" line="117"/>
        <source>Guest</source>
        <translation>游客</translation>
    </message>
    <message>
        <location filename="../base/system.cpp" line="120"/>
        <source>Admin</source>
        <translation>管理员</translation>
    </message>
    <message>
        <location filename="../base/system.cpp" line="123"/>
        <source>User</source>
        <translation>用户</translation>
    </message>
    <message>
        <location filename="../base/system.cpp" line="126"/>
        <source>Error</source>
        <translation>错误</translation>
    </message>
    <message>
        <location filename="../base/system.cpp" line="273"/>
        <source>(no description available)</source>
        <translation>（无可用描述）</translation>
    </message>
</context>
<context>
    <name>TTSBase</name>
    <message>
        <location filename="../base/ttsbase.cpp" line="47"/>
        <source>Espeak TTS Engine</source>
        <translation>Espaek TTS引擎</translation>
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
    <message>
        <location filename="../base/ttsbase.cpp" line="51"/>
        <source>Flite TTS Engine</source>
        <translation>Flite TTS引擎</translation>
    </message>
    <message>
        <location filename="../base/ttsbase.cpp" line="52"/>
        <source>Swift TTS Engine</source>
        <translation>Swift TTS引擎</translation>
    </message>
    <message>
        <location filename="../base/ttsbase.cpp" line="55"/>
        <source>SAPI4 TTS Engine</source>
        <translation>SAPI4 TTS引擎</translation>
    </message>
    <message>
        <location filename="../base/ttsbase.cpp" line="57"/>
        <source>SAPI5 TTS Engine</source>
        <translation>SAPI5 TTS引擎</translation>
    </message>
    <message>
        <location filename="../base/ttsbase.cpp" line="58"/>
        <source>MS Speech Platform</source>
        <translation>MS Speech平台</translation>
    </message>
    <message>
        <location filename="../base/ttsbase.cpp" line="61"/>
        <source>Festival TTS Engine</source>
        <translation>Festival TTS引擎</translation>
    </message>
    <message>
        <location filename="../base/ttsbase.cpp" line="64"/>
        <source>OS X System Engine</source>
        <translation>OSX系统引擎</translation>
    </message>
</context>
<context>
    <name>TTSCarbon</name>
    <message>
        <location filename="../base/ttscarbon.cpp" line="139"/>
        <source>Voice:</source>
        <translation>语音：</translation>
    </message>
    <message>
        <location filename="../base/ttscarbon.cpp" line="145"/>
        <source>Speed (words/min):</source>
        <translation>速度（词/分）：</translation>
    </message>
    <message>
        <location filename="../base/ttscarbon.cpp" line="152"/>
        <source>Pitch (0 for default):</source>
        <translation>音高（默认0）：</translation>
    </message>
    <message>
        <location filename="../base/ttscarbon.cpp" line="222"/>
        <source>Could not voice string</source>
        <translation>无法读出字段</translation>
    </message>
    <message>
        <location filename="../base/ttscarbon.cpp" line="232"/>
        <source>Could not convert intermediate file</source>
        <translation>无法转换中间文件</translation>
    </message>
</context>
<context>
    <name>TTSExes</name>
    <message>
        <location filename="../base/ttsexes.cpp" line="78"/>
        <source>TTS executable not found</source>
        <translation>TTS可执行文件未找到</translation>
    </message>
    <message>
        <location filename="../base/ttsexes.cpp" line="44"/>
        <source>Path to TTS engine:</source>
        <translation>TTS引擎路径：</translation>
    </message>
    <message>
        <location filename="../base/ttsexes.cpp" line="46"/>
        <source>TTS engine options:</source>
        <translation>TTS引擎选项：</translation>
    </message>
</context>
<context>
    <name>TTSFestival</name>
    <message>
        <location filename="../base/ttsfestival.cpp" line="207"/>
        <source>engine could not voice string</source>
        <translation>引擎无法读出字段</translation>
    </message>
    <message>
        <location filename="../base/ttsfestival.cpp" line="290"/>
        <source>No description available</source>
        <translation>无可用描述</translation>
    </message>
    <message>
        <location filename="../base/ttsfestival.cpp" line="53"/>
        <source>Path to Festival client:</source>
        <translation>Festival客户端路径：</translation>
    </message>
    <message>
        <location filename="../base/ttsfestival.cpp" line="58"/>
        <source>Voice:</source>
        <translation>语音：</translation>
    </message>
    <message>
        <location filename="../base/ttsfestival.cpp" line="69"/>
        <source>Voice description:</source>
        <translation>语音描述：</translation>
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
        <translation>语言：</translation>
    </message>
    <message>
        <location filename="../base/ttssapi.cpp" line="53"/>
        <source>Voice:</source>
        <translation>语音：</translation>
    </message>
    <message>
        <location filename="../base/ttssapi.cpp" line="65"/>
        <source>Speed:</source>
        <translation>语速：</translation>
    </message>
    <message>
        <location filename="../base/ttssapi.cpp" line="68"/>
        <source>Options:</source>
        <translation>选项：</translation>
    </message>
    <message>
        <location filename="../base/ttssapi.cpp" line="112"/>
        <source>Could not copy the SAPI script</source>
        <translation>无法复制SAPI脚本</translation>
    </message>
    <message>
        <location filename="../base/ttssapi.cpp" line="130"/>
        <source>Could not start SAPI process</source>
        <translation>无法启动SAPI进程</translation>
    </message>
</context>
<context>
    <name>TalkFileCreator</name>
    <message>
        <location filename="../base/talkfile.cpp" line="66"/>
        <source>Copying Talkfiles...</source>
        <translation>复制说话文件…</translation>
    </message>
    <message>
        <location filename="../base/talkfile.cpp" line="257"/>
        <source>File copy aborted</source>
        <translation>文件复制中断</translation>
    </message>
    <message>
        <location filename="../base/talkfile.cpp" line="298"/>
        <source>Cleaning up...</source>
        <translation>清理…</translation>
    </message>
    <message>
        <location filename="../base/talkfile.cpp" line="309"/>
        <source>Finished</source>
        <translation>完成</translation>
    </message>
    <message>
        <location filename="../base/talkfile.cpp" line="45"/>
        <source>Talk file creation aborted</source>
        <translation>说话文件生成中止</translation>
    </message>
    <message>
        <location filename="../base/talkfile.cpp" line="36"/>
        <source>Starting Talk file generation for folder %1</source>
        <translation>开始为文件夹%1生成说话文件</translation>
    </message>
    <message>
        <location filename="../base/talkfile.cpp" line="78"/>
        <source>Finished creating Talk files</source>
        <translation>创建说话文件完成</translation>
    </message>
    <message>
        <location filename="../base/talkfile.cpp" line="42"/>
        <source>Reading Filelist...</source>
        <translation>正在读取文件列表…</translation>
    </message>
    <message>
        <location filename="../base/talkfile.cpp" line="276"/>
        <source>Copying of %1 to %2 failed</source>
        <translation>%1 到 %2 复制失败</translation>
    </message>
</context>
<context>
    <name>TalkGenerator</name>
    <message>
        <location filename="../base/talkgenerator.cpp" line="38"/>
        <source>Starting TTS Engine</source>
        <translation>开始TTS引擎</translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="43"/>
        <location filename="../base/talkgenerator.cpp" line="50"/>
        <source>Init of TTS engine failed</source>
        <translation>TTS引擎初始化失败</translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="57"/>
        <source>Starting Encoder Engine</source>
        <translation>开始编码引擎</translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="62"/>
        <source>Init of Encoder engine failed</source>
        <translation>编码引擎初始化失败</translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="72"/>
        <source>Voicing entries...</source>
        <translation>正在读出字段…</translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="87"/>
        <source>Encoding files...</source>
        <translation>编码文件…</translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="126"/>
        <source>Voicing aborted</source>
        <translation>发音中止</translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="163"/>
        <location filename="../base/talkgenerator.cpp" line="168"/>
        <source>Voicing of %1 failed: %2</source>
        <translation>%1发音出错：%2</translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="212"/>
        <source>Encoding aborted</source>
        <translation>编码中止</translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="240"/>
        <source>Encoding of %1 failed</source>
        <translation>%1编码失败</translation>
    </message>
</context>
<context>
    <name>ThemeInstallFrm</name>
    <message>
        <location filename="../themesinstallfrm.ui" line="13"/>
        <source>Theme Installation</source>
        <translation>主题安装</translation>
    </message>
    <message>
        <location filename="../themesinstallfrm.ui" line="48"/>
        <source>Selected Theme</source>
        <translation>选定的主题</translation>
    </message>
    <message>
        <location filename="../themesinstallfrm.ui" line="73"/>
        <source>Description</source>
        <translation>描述</translation>
    </message>
    <message>
        <location filename="../themesinstallfrm.ui" line="83"/>
        <source>Download size:</source>
        <translation>下载大小:</translation>
    </message>
    <message>
        <location filename="../themesinstallfrm.ui" line="126"/>
        <source>&amp;Cancel</source>
        <translation>&amp;取消</translation>
    </message>
    <message>
        <location filename="../themesinstallfrm.ui" line="115"/>
        <source>&amp;Install</source>
        <translation>&amp;安装</translation>
    </message>
    <message>
        <location filename="../themesinstallfrm.ui" line="93"/>
        <source>Hold Ctrl to select multiple item, Shift for a range</source>
        <translation>按住Ctrl选择多个项目，Shift选择范围项目</translation>
    </message>
</context>
<context>
    <name>ThemesInstallWindow</name>
    <message>
        <location filename="../themesinstallwindow.cpp" line="41"/>
        <source>no theme selected</source>
        <translation>未选择主题</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="127"/>
        <source>Network error: %1.
Please check your network and proxy settings.</source>
        <translation>网络错误： %1.
请检查你的网络和代理设置.</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="146"/>
        <source>done.</source>
        <translation>完成.</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="219"/>
        <source>fetching details for %1</source>
        <translation>正在抓取 %1 的详细信息</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="222"/>
        <source>fetching preview ...</source>
        <translation>正在拿取预览...</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="235"/>
        <source>&lt;b&gt;Author:&lt;/b&gt; %1&lt;hr/&gt;</source>
        <translation>&lt;b&gt;作者:&lt;/b&gt; %1&lt;hr/&gt;</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="236"/>
        <location filename="../themesinstallwindow.cpp" line="238"/>
        <source>unknown</source>
        <translation>未知</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="237"/>
        <source>&lt;b&gt;Version:&lt;/b&gt; %1&lt;hr/&gt;</source>
        <translation>&lt;b&gt;版本:&lt;/b&gt; %1&lt;hr/&gt;</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="239"/>
        <source>&lt;b&gt;Description:&lt;/b&gt; %1&lt;hr/&gt;</source>
        <translation>&lt;b&gt;描述:&lt;/b&gt; %1&lt;hr/&gt;</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="240"/>
        <source>no description</source>
        <translation>无描述</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="267"/>
        <source>no theme preview</source>
        <translation>无主题预览</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="295"/>
        <source>Select</source>
        <translation>选择</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="302"/>
        <source>getting themes information ...</source>
        <translation>正在拿取主题信息...</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="330"/>
        <source>No themes selected, skipping</source>
        <translation>未选择主题，已跳过</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="360"/>
        <source>Mount point is wrong!</source>
        <translation>挂载点错误!</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="140"/>
        <source>the following error occured:
%1</source>
        <translation>发生下述错误：
%1</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="42"/>
        <source>no selection</source>
        <translation>无选择</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="183"/>
        <source>Information</source>
        <translation>信息</translation>
    </message>
    <message numerus="yes">
        <location filename="../themesinstallwindow.cpp" line="203"/>
        <source>Download size %L1 kiB (%n item(s))</source>
        <translation>
            <numerusform>下载大小%L1 KB（%n 个项目）</numerusform>
        </translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="256"/>
        <source>Retrieving theme preview failed.
HTTP response code: %1</source>
        <translation>拿取主题预览失败。
HTTP返回代码：%1</translation>
    </message>
</context>
<context>
    <name>UninstallFrm</name>
    <message>
        <location filename="../uninstallfrm.ui" line="16"/>
        <source>Uninstall Rockbox</source>
        <translation>卸载 Rockbox</translation>
    </message>
    <message>
        <location filename="../uninstallfrm.ui" line="35"/>
        <source>Please select the Uninstallation Method</source>
        <translation>请选择卸载方法</translation>
    </message>
    <message>
        <location filename="../uninstallfrm.ui" line="45"/>
        <source>Uninstallation Method</source>
        <translation>卸载方法</translation>
    </message>
    <message>
        <location filename="../uninstallfrm.ui" line="51"/>
        <source>Complete Uninstallation</source>
        <translation>完全卸载</translation>
    </message>
    <message>
        <location filename="../uninstallfrm.ui" line="58"/>
        <source>Smart Uninstallation</source>
        <translation>智能卸载</translation>
    </message>
    <message>
        <location filename="../uninstallfrm.ui" line="68"/>
        <source>Please select what you want to uninstall</source>
        <translation>请选择卸载部分</translation>
    </message>
    <message>
        <location filename="../uninstallfrm.ui" line="78"/>
        <source>Installed Parts</source>
        <translation>已安装的部分</translation>
    </message>
    <message>
        <location filename="../uninstallfrm.ui" line="139"/>
        <source>&amp;Cancel</source>
        <translation>&amp;取消</translation>
    </message>
    <message>
        <location filename="../uninstallfrm.ui" line="128"/>
        <source>&amp;Uninstall</source>
        <translation>&amp;卸载</translation>
    </message>
</context>
<context>
    <name>Uninstaller</name>
    <message>
        <location filename="../base/uninstall.cpp" line="32"/>
        <location filename="../base/uninstall.cpp" line="43"/>
        <source>Starting Uninstallation</source>
        <translation>开始卸载</translation>
    </message>
    <message>
        <location filename="../base/uninstall.cpp" line="36"/>
        <source>Finished Uninstallation</source>
        <translation>完成卸载</translation>
    </message>
    <message>
        <location filename="../base/uninstall.cpp" line="49"/>
        <source>Uninstalling %1...</source>
        <translation>正在卸载%1…</translation>
    </message>
    <message>
        <location filename="../base/uninstall.cpp" line="81"/>
        <source>Could not delete %1</source>
        <translation>无法删除%1</translation>
    </message>
    <message>
        <location filename="../base/uninstall.cpp" line="115"/>
        <source>Uninstallation finished</source>
        <translation>卸载完成</translation>
    </message>
</context>
<context>
    <name>Utils</name>
    <message>
        <location filename="../base/utils.cpp" line="375"/>
        <source>&lt;li&gt;Permissions insufficient for bootloader installation.
Administrator priviledges are necessary.&lt;/li&gt;</source>
        <translation>&lt;li&gt;bootloader安装权限不够
需要管理员权限。&lt;/li&gt;</translation>
    </message>
    <message>
        <location filename="../base/utils.cpp" line="387"/>
        <source>&lt;li&gt;Target mismatch detected.&lt;br/&gt;Installed target: %1&lt;br/&gt;Selected target: %2.&lt;/li&gt;</source>
        <translation>&lt;li&gt;检测到目标不匹配。&lt;br/&gt;已安装目标： %1&lt;br/&gt;选定目标： %2。&lt;/li&gt;</translation>
    </message>
    <message>
        <location filename="../base/utils.cpp" line="396"/>
        <source>Problem detected:</source>
        <translation>检测到问题：</translation>
    </message>
</context>
<context>
    <name>VoiceFileCreator</name>
    <message>
        <location filename="../base/voicefile.cpp" line="43"/>
        <source>Starting Voicefile generation</source>
        <translation>开始语音文件生成</translation>
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
        <location filename="../base/voicefile.cpp" line="133"/>
        <source>Extracted voice strings from installation</source>
        <translation>已从安装中提取语音字段</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="143"/>
        <source>Extracted voice strings incompatible</source>
        <translation>提取的语音字符串不兼容</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="194"/>
        <source>Could not retrieve strings from installation, downloading</source>
        <translation>无法从安装检索字符串，正在下载</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="221"/>
        <source>Downloading voice info...</source>
        <translation>下载语音信息…</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="234"/>
        <source>Download error: received HTTP error %1.</source>
        <translation>下载错误: 接到 HTTP 错误 %1。</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="241"/>
        <source>Cached file used.</source>
        <translation>已使用缓存文件。</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="244"/>
        <source>Download error: %1</source>
        <translation>下载错误: %1</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="249"/>
        <source>Download finished.</source>
        <translation>下载完成。</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="262"/>
        <source>failed to open downloaded file</source>
        <translation>无法打开下载的文件</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="268"/>
        <source>Reading strings...</source>
        <translation>正在读出字段…</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="350"/>
        <source>Creating voicefiles...</source>
        <translation>正在创建语音文件…</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="399"/>
        <source>Cleaning up...</source>
        <translation>正在清理…</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="410"/>
        <source>Finished</source>
        <translation>已完成</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="324"/>
        <source>The downloaded file was empty!</source>
        <translation>下载文件是空的！</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="355"/>
        <source>Error opening downloaded file</source>
        <translation>打开下载的文件时出错</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="366"/>
        <source>Error opening output file</source>
        <translation>打开输出文件时出错</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="390"/>
        <source>successfully created.</source>
        <translation>成功创建。</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="56"/>
        <source>could not find rockbox-info.txt</source>
        <translation>找不到rockbox-info.txt</translation>
    </message>
</context>
<context>
    <name>ZipInstaller</name>
    <message>
        <location filename="../base/zipinstaller.cpp" line="60"/>
        <source>done.</source>
        <translation>完成.</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="68"/>
        <source>Package installation finished successfully.</source>
        <translation>已成功完成包安装。</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="79"/>
        <source>Downloading file %1.%2</source>
        <translation>正在下载文件 %1.%2</translation>
    </message>
    <message>
        <source>Download error: received HTTP error %1.</source>
        <translation type="vanished">下载错误: 接到 HTTP 错误 %1。</translation>
    </message>
    <message>
        <source>Cached file used.</source>
        <translation type="vanished">已使用缓存文件。</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="114"/>
        <source>Download error: received HTTP error %1
%2</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="121"/>
        <source>Download error: %1</source>
        <translation>下载错误: %1</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="126"/>
        <source>Download finished (cache used).</source>
        <translation type="unfinished">完成下载（已使用缓存）。</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="129"/>
        <source>Download finished.</source>
        <translation>完成下载.</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="136"/>
        <source>Extracting file.</source>
        <translation>正在解压文件.</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="157"/>
        <source>Extraction failed!</source>
        <translation>解压失败！</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="169"/>
        <source>Installing file.</source>
        <translation>正在安装文件.</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="181"/>
        <source>Installing file failed.</source>
        <translation>安装文件失败.</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="194"/>
        <source>Creating installation log</source>
        <translation>正在建立安装日志</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="150"/>
        <source>Not enough disk space! Aborting.</source>
        <translation>没有足够的硬盘空间了！已中断。</translation>
    </message>
</context>
<context>
    <name>ZipUtil</name>
    <message>
        <location filename="../base/ziputil.cpp" line="125"/>
        <source>Creating output path failed</source>
        <translation>创建输出路径出错</translation>
    </message>
    <message>
        <location filename="../base/ziputil.cpp" line="132"/>
        <source>Creating output file failed</source>
        <translation>创建输出文件出错</translation>
    </message>
    <message>
        <location filename="../base/ziputil.cpp" line="141"/>
        <source>Error during Zip operation</source>
        <translation>ZIP操作出错</translation>
    </message>
</context>
<context>
    <name>aboutBox</name>
    <message>
        <location filename="../aboutbox.ui" line="14"/>
        <source>About Rockbox Utility</source>
        <translation>关于Rockbox安装程序</translation>
    </message>
    <message>
        <location filename="../aboutbox.ui" line="32"/>
        <source>The Rockbox Utility</source>
        <translation>Rockbox安装程序</translation>
    </message>
    <message>
        <location filename="../aboutbox.ui" line="54"/>
        <source>Installer and housekeeping utility for the Rockbox open source digital audio player firmware.&lt;br/&gt;© The Rockbox Team.&lt;br/&gt;Released under the GNU General Public License v2.&lt;br/&gt;Uses icons by the &lt;a href=&quot;http://tango.freedesktop.org/&quot;&gt;Tango Project&lt;/a&gt;.&lt;br/&gt;&lt;center&gt;&lt;a href=&quot;https://www.rockbox.org&quot;&gt;https://www.rockbox.org&lt;/a&gt;&lt;/center&gt;</source>
        <translation>Rockbox 开源数字音频播放器固件的安装程序和管理实用程序。&lt;br/&gt;© The Rockbox Team.&lt;br/&gt;在 GNU 通用公共许可证 v2 下发布。&lt;br/&gt;使用的图标来自 &lt;a href=&quot;http://tango.freedesktop.org/&quot;&gt;Tango Project&lt;/a&gt;.&lt;br/&gt;&lt;center&gt;&lt;a href=&quot;https://www.rockbox.org&quot;&gt;https://www.rockbox.org&lt;/a&gt;&lt;/center&gt;</translation>
    </message>
    <message>
        <location filename="../aboutbox.ui" line="74"/>
        <source>&amp;Credits</source>
        <translation>&amp;特别鸣谢</translation>
    </message>
    <message>
        <location filename="../aboutbox.ui" line="106"/>
        <source>&amp;License</source>
        <translation>&amp;授权</translation>
    </message>
    <message>
        <location filename="../aboutbox.ui" line="132"/>
        <source>L&amp;ibraries</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>&amp;Speex License</source>
        <translation type="vanished">&amp;Speex 许可证</translation>
    </message>
    <message>
        <location filename="../aboutbox.ui" line="158"/>
        <source>&amp;Ok</source>
        <translation>&amp;OK</translation>
    </message>
</context>
</TS>
