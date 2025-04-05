<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="ko">
<context>
    <name>BackupDialog</name>
    <message>
        <location filename="../gui/backupdialogfrm.ui" line="17"/>
        <location filename="../gui/backupdialogfrm.ui" line="43"/>
        <source>Backup</source>
        <translation>백업</translation>
    </message>
    <message>
        <location filename="../gui/backupdialogfrm.ui" line="33"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;This dialog will create a backup by archiving the contents of the Rockbox installation on the player into a zip file. This will include installed themes and settings stored below the .rockbox folder on the player.&lt;/p&gt;&lt;p&gt;The backup filename will be created based on the installed version. &lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;이 대화 상자는 플레이어의 록박스 설치 내용을 zip 파일로 보관하여 백업을 만듭니다. 여기에는 플레이어의 .rockbox 폴더 아래에 저장된 설치된 테마와 설정이 포함됩니다.&lt;/p&gt;&lt;p&gt;백업 파일 이름은 설치된 버전에 따라 생성됩니다.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location filename="../gui/backupdialogfrm.ui" line="49"/>
        <source>Size: unknown</source>
        <translation>크기: 알 수 없음</translation>
    </message>
    <message>
        <location filename="../gui/backupdialogfrm.ui" line="56"/>
        <source>Backup to: unknown</source>
        <translation>백업 대상: 알 수 없음</translation>
    </message>
    <message>
        <location filename="../gui/backupdialogfrm.ui" line="76"/>
        <source>&amp;Change</source>
        <translation>변경(&amp;C)</translation>
    </message>
    <message>
        <location filename="../gui/backupdialogfrm.ui" line="116"/>
        <source>&amp;Backup</source>
        <translation>백업(&amp;B)</translation>
    </message>
    <message>
        <location filename="../gui/backupdialogfrm.ui" line="127"/>
        <source>&amp;Cancel</source>
        <translation>취소(&amp;C)</translation>
    </message>
    <message>
        <location filename="../gui/backupdialog.cpp" line="69"/>
        <source>Installation size: calculating ...</source>
        <translation>설치 크기: 계산 중...</translation>
    </message>
    <message>
        <location filename="../gui/backupdialog.cpp" line="88"/>
        <source>Select Backup Filename</source>
        <translation>백업 파일이름 선택</translation>
    </message>
    <message>
        <location filename="../gui/backupdialog.cpp" line="108"/>
        <source>Installation size: %L1 %2</source>
        <translation>설치 크기: %L1 %2</translation>
    </message>
    <message>
        <location filename="../gui/backupdialog.cpp" line="115"/>
        <source>File exists</source>
        <translation>파일 존재함</translation>
    </message>
    <message>
        <location filename="../gui/backupdialog.cpp" line="116"/>
        <source>The selected backup file already exists. Overwrite?</source>
        <translation>선택한 백업 파일이 이미 존재합니다. 덮어쓸까요?</translation>
    </message>
    <message>
        <location filename="../gui/backupdialog.cpp" line="124"/>
        <source>Starting backup ...</source>
        <translation>백업 시작 중...</translation>
    </message>
    <message>
        <location filename="../gui/backupdialog.cpp" line="143"/>
        <source>Backup successful.</source>
        <translation>백업에 성공했습니다.</translation>
    </message>
    <message>
        <location filename="../gui/backupdialog.cpp" line="146"/>
        <source>Backup failed!</source>
        <translation>백업에 실패했습니다!</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallAms</name>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="33"/>
        <source>Bootloader installation requires you to provide a copy of the original Sandisk firmware (bin file). This firmware file will be patched and then installed to your player along with the rockbox bootloader. You need to download this file yourself due to legal reasons. Please browse the &lt;a href=&apos;http://forums.sandisk.com/sansa/&apos;&gt;Sansa Forums&lt;/a&gt; or refer to the &lt;a href=&apos;https://www.rockbox.org/manual.shtml&apos;&gt;manual&lt;/a&gt; and the &lt;a href=&apos;https://www.rockbox.org/wiki/SansaAMS&apos;&gt;SansaAMS&lt;/a&gt; wiki page on how to obtain this file.&lt;br/&gt;&lt;b&gt;Note:&lt;/b&gt; This file is not present on your player and will disappear automatically after installing it.&lt;br/&gt;&lt;br/&gt;Press Ok to continue and browse your computer for the firmware file.</source>
        <translation>부트로더를 설치하려면 원래 Sandisk 펌웨어(bin 파일) 사본을 제공해야 합니다. 이 펌웨어 파일은 패치된 후 rockbox 부트로더와 함께 플레이어에 설치됩니다. 법적 이유로 이 파일은 직접 다운로드해야 합니다. 이 파일을 얻는 방법은 &lt;a href=&apos;http://forums.sandisk.com/sansa/&apos;&gt;Sansa 포럼&lt;/a&gt;을 찾아보거나 &lt;a href=&apos;https://www.rockbox.org/manual.shtml&apos;&gt;설명서&lt;/a&gt; 및 &lt;a href=&apos;https://www.rockbox.org/wiki/SansaAMS&apos;&gt;SansaAMS&lt;/a&gt; 위키 페이지를 참조하세요.&lt;br/&gt;&lt;b&gt;참조:&lt;/b&gt;이 파일은 플레이어에 존재하지 않으며 설치 후 자동으로 사라집니다.&lt;br/&gt;&lt;br/&gt; 계속해서 컴퓨터에서 펌웨어 파일을 찾으려면 확인을 누르세요.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="58"/>
        <source>Downloading bootloader file</source>
        <translation>부트로더 파일 다운로드 중</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="100"/>
        <location filename="../base/bootloaderinstallams.cpp" line="113"/>
        <source>Could not load %1</source>
        <translation>%1을(를) 로드할 수 없음</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="127"/>
        <source>No room to insert bootloader, try another firmware version</source>
        <translation>부트로더를 삽입할 공간이 없으므로, 다른 펌웨어 버전을 사용해 보세요</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="137"/>
        <source>Patching Firmware...</source>
        <translation>펌웨어 패치 중...</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="148"/>
        <source>Could not open %1 for writing</source>
        <translation>쓰기 위해 %1을(를) 열 수 없음</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="161"/>
        <source>Could not write firmware file</source>
        <translation>펌웨어 파일을 쓸 수 없음</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="177"/>
        <source>Success: modified firmware file created</source>
        <translation>성공: 수정된 펌웨어 파일이 생성됨</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="185"/>
        <source>To uninstall, perform a normal upgrade with an unmodified original firmware</source>
        <translation>제거하려면 수정되지 않은 기존 펌웨어로 일반 업그레이드를 수행하세요.</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallBSPatch</name>
    <message>
        <location filename="../base/bootloaderinstallbspatch.cpp" line="65"/>
        <source>Bootloader installation requires you to provide the correct verrsion of the original firmware file. This file will be patched with the Rockbox bootloader and installed to your player. You need to download this file yourself due to legal reasons. Please refer to the &lt;a href=&apos;https://www.rockbox.org/wiki/&apos;&gt;rockbox wiki&lt;/a&gt; pages on how to obtain this file.&lt;br/&gt;Press Ok to continue and browse your computer for the firmware file.</source>
        <translation>부트로더를 설치하려면 기존 펌웨어 파일의 올바른 버전을 제공해야 합니다. 이 파일은 록박스 부트로더로 패치되어 플레이어에 설치됩니다. 법적인 이유로 이 파일을 직접 다운로드해야 합니다. 이 파일을 얻는 방법은 &lt;a href=&apos;https://www.rockbox.org/wiki/&apos;&gt;록박스 위키&lt;/a&gt; 페이지를 참조하세요.&lt;br/&gt; 계속해서 컴퓨터에서 펌웨어 파일을 검색하려면 확인을 누르세요.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbspatch.cpp" line="84"/>
        <source>Could not read original firmware file</source>
        <translation>기존 펌웨어 파일을 읽을 수 없음</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbspatch.cpp" line="90"/>
        <source>Downloading bootloader file</source>
        <translation>부트로더 파일 다운로드 중</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbspatch.cpp" line="99"/>
        <source>Patching file...</source>
        <translation>파일 패치 중...</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbspatch.cpp" line="124"/>
        <source>Patching the original firmware failed</source>
        <translation>기존 펌웨어 패치 실패함</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbspatch.cpp" line="130"/>
        <source>Succesfully patched firmware file</source>
        <translation>성공적으로 패치된 펌웨어 파일</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbspatch.cpp" line="145"/>
        <source>Bootloader successful installed</source>
        <translation>부트로더 성공적으로 설치됨</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbspatch.cpp" line="151"/>
        <source>Patched bootloader could not be installed</source>
        <translation>패치된 부트로더를 설치할 수 없음</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbspatch.cpp" line="161"/>
        <source>To uninstall, perform a normal upgrade with an unmodified original firmware.</source>
        <translation>제거하려면 수정되지 않은 기존 펌웨어로 일반적인 업그레이드를 실행하세요.</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallBase</name>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="69"/>
        <source>Download error: received HTTP error %1.</source>
        <translation>다운로드 오류: HTTP 오류 %1을(를) 받았습니다.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="75"/>
        <source>Download error: %1</source>
        <translation>다운로드 오류: %1</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="81"/>
        <source>Download finished (cache used).</source>
        <translation>다운로드가 완료되었습니다. (캐시 사용)</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="83"/>
        <source>Download finished.</source>
        <translation>다운로드가 완료되었습니다.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="112"/>
        <source>Creating backup of original firmware file.</source>
        <translation>원본 펌웨어 파일의 백업을 생성합니다.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="114"/>
        <source>Creating backup folder failed</source>
        <translation>백업 폴더 생성 실패함</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="120"/>
        <source>Creating backup copy failed.</source>
        <translation>백업 복사본을 생성하지 못했습니다.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="123"/>
        <source>Backup created.</source>
        <translation>백업이 생성되었습니다.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="140"/>
        <source>Creating installation log</source>
        <translation>설치 로그 생성</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="245"/>
        <source>Zip file format detected</source>
        <translation>Zip 파일 형식이 감지됨</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="257"/>
        <source>CAB file format detected</source>
        <translation>CAB 파일 형식이 감지됨</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="278"/>
        <source>Extracting firmware %1 from archive</source>
        <translation>파일에서 펌웨어 %1 추출</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="285"/>
        <source>Error extracting firmware from archive</source>
        <translation>파일에서 펌웨어 추출 오류</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="294"/>
        <source>Could not find firmware in archive</source>
        <translation>파일에서 펌웨어를 찾을 수 없음</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="162"/>
        <source>Waiting for system to remount player</source>
        <translation>시스템이 플레이어를 다시 마운트할 때까지 기다리는 중</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="192"/>
        <source>Player remounted</source>
        <translation>플레이어가 다시 마운트되었음</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="197"/>
        <source>Timeout on remount</source>
        <translation>다시 마운트 시 시간 초과</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="152"/>
        <source>Installation log created</source>
        <translation>설치 로그가 생성됨</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallChinaChip</name>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="33"/>
        <source>Bootloader installation requires you to provide a firmware file of the original firmware (HXF file). You need to download this file yourself due to legal reasons. Please refer to the &lt;a href=&apos;https://www.rockbox.org/manual.shtml&apos;&gt;manual&lt;/a&gt; and the &lt;a href=&apos;https://www.rockbox.org/wiki/OndaVX747#Download_and_extract_a_recent_ve&apos;&gt;OndaVX747&lt;/a&gt; wiki page on how to obtain this file.&lt;br/&gt;Press Ok to continue and browse your computer for the firmware file.</source>
        <translation>부트로더를 설치하려면 원본 펌웨어의 펌웨어 파일(HXF 파일)을 제공해야 합니다. 법적인 이유로 이 파일을 직접 다운로드해야 합니다. 이 파일을 얻는 방법은 &lt;a href=&apos;https://www.rockbox.org/manual.shtml&apos;&gt;설명서&lt;/a&gt; 및 &lt;a href=&apos;https://www.rockbox.org/wiki/OndaVX747#Download_and_extract_a_recent_ve&apos;&gt;OndaVX747&lt;/a&gt; 위키 페이지를 참조하세요.&lt;br/&gt;계속해서 컴퓨터에서 펌웨어 파일을 찾으려면 확인을 누르세요.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="50"/>
        <source>Downloading bootloader file</source>
        <translation>부트로더 파일 다운로드 중</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="75"/>
        <source>Could not open firmware file</source>
        <translation>펌웨어 파일을 열 수 없음</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="78"/>
        <source>Could not open bootloader file</source>
        <translation>부트로더 파일을 열 수 없음</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="81"/>
        <source>Could not allocate memory</source>
        <translation>메모리를 할당할 수 없음</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="84"/>
        <source>Could not load firmware file</source>
        <translation>펌웨어 파일을 로드할 수 없음</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="87"/>
        <source>File is not a valid ChinaChip firmware</source>
        <translation>파일이 유효한 ChinaChip 펌웨어가 아님</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="90"/>
        <source>Could not find ccpmp.bin in input file</source>
        <translation>입력 파일에서 ccpmp.bin을 찾을 수 없음</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="93"/>
        <source>Could not open backup file for ccpmp.bin</source>
        <translation>ccpmp.bin에 대한 백업 파일을 열 수 없음</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="96"/>
        <source>Could not write backup file for ccpmp.bin</source>
        <translation>ccpmp.bin에 대한 백업 파일을 쓸 수 없음</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="99"/>
        <source>Could not load bootloader file</source>
        <translation>부트로더 파일을 로드할 수 없음</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="102"/>
        <source>Could not get current time</source>
        <translation>현재 시간을 가져올 수 없음</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="105"/>
        <source>Could not open output file</source>
        <translation>출력 파일을 열 수 없음</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="108"/>
        <source>Could not write output file</source>
        <translation>출력 파일을 쓸 수 없음</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="111"/>
        <source>Unexpected error from chinachippatcher</source>
        <translation>chinachippatcher에서 예기치 않은 오류가 발생함</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallFile</name>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="34"/>
        <source>Downloading bootloader</source>
        <translation>부트로더 다운로드 중</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="43"/>
        <source>Installing Rockbox bootloader</source>
        <translation>록박스 부트로더 설치</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="75"/>
        <source>Error accessing output folder</source>
        <translation>출력 폴더에 접속하는 중 오류 발생</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="89"/>
        <source>A firmware file is already present on player</source>
        <translation>플레이어에 펌웨어 파일이 이미 존재함</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="94"/>
        <source>Bootloader successful installed</source>
        <translation>부트로더가 성공적으로 설치됨</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="97"/>
        <source>Copying modified firmware file failed</source>
        <translation>수정된 펌웨어 파일 복사에 실패함</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="111"/>
        <source>Removing Rockbox bootloader</source>
        <translation>록박스 부트로더 제거</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="115"/>
        <source>No original firmware file found.</source>
        <translation>원본 펌웨어 파일을 찾을 수 없습니다.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="121"/>
        <source>Can&apos;t remove Rockbox bootloader file.</source>
        <translation>록박스 부트로더 파일을 제거할 수 없습니다.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="126"/>
        <source>Can&apos;t restore bootloader file.</source>
        <translation>부트로더 파일을 복원할 수 없습니다.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="130"/>
        <source>Original bootloader restored successfully.</source>
        <translation>기존 부트로더가 성공적으로 복원되었습니다.</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallHex</name>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="69"/>
        <source>checking MD5 hash of input file ...</source>
        <translation>입력 파일의 MD5 해시를 확인하는 중...</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="80"/>
        <source>Could not verify original firmware file</source>
        <translation>원본 펌웨어 파일을 확인할 수 없음</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="95"/>
        <source>Firmware file not recognized.</source>
        <translation>펌웨어 파일이 인식되지 않습니다.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="99"/>
        <source>MD5 hash ok</source>
        <translation>MD5 해시 확인</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="106"/>
        <source>Firmware file doesn&apos;t match selected player.</source>
        <translation>펌웨어 파일이 선택한 플레이어와 일치하지 않습니다.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="111"/>
        <source>Descrambling file</source>
        <translation>파일을 해독하는 중</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="119"/>
        <source>Error in descramble: %1</source>
        <translation>디스크램블 오류: %1</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="124"/>
        <source>Downloading bootloader file</source>
        <translation>부트로더 파일 다운로드 중</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="134"/>
        <source>Adding bootloader to firmware file</source>
        <translation>펌웨어 파일에 부트로더 추가</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="172"/>
        <source>could not open input file</source>
        <translation>입력 파일을 열 수 없음</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="173"/>
        <source>reading header failed</source>
        <translation>헤더 읽기 실패함</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="174"/>
        <source>reading firmware failed</source>
        <translation>펌웨어 읽기 실패함</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="175"/>
        <source>can&apos;t open bootloader file</source>
        <translation>부트로더 파일을 열 수 없음</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="176"/>
        <source>reading bootloader file failed</source>
        <translation>부트로더 파일 읽기 실패함</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="177"/>
        <source>can&apos;t open output file</source>
        <translation>출력 파일을 열 수 없음</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="178"/>
        <source>writing output file failed</source>
        <translation>출력 파일 쓰기 실패함</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="180"/>
        <source>Error in patching: %1</source>
        <translation>패치 중 오류: %1</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="191"/>
        <source>Error in scramble: %1</source>
        <translation>스크램블 오류: %1</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="206"/>
        <source>Checking modified firmware file</source>
        <translation>수정된 펌웨어 파일 확인 중</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="208"/>
        <source>Error: modified file checksum wrong</source>
        <translation>오류: 수정된 파일 체크섬이 잘못됨</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="215"/>
        <source>A firmware file is already present on player</source>
        <translation>플레이어에 펌웨어 파일이 이미 존재함</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="220"/>
        <source>Success: modified firmware file created</source>
        <translation>성공: 수정된 펌웨어 파일이 생성됨</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="223"/>
        <source>Copying modified firmware file failed</source>
        <translation>수정된 펌웨어 파일 복사에 실패함</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="237"/>
        <source>Uninstallation not possible, only installation info removed</source>
        <translation>설치 제거가 불가능하며 설치 정보만 제거됨</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="259"/>
        <source>Can&apos;t open input file</source>
        <translation>입력 파일을 열 수 없음</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="260"/>
        <source>Can&apos;t open output file</source>
        <translation>출력 파일을 열 수 없음</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="261"/>
        <source>invalid file: header length wrong</source>
        <translation>잘못된 파일: 헤더 길이가 잘못됨</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="262"/>
        <source>invalid file: unrecognized header</source>
        <translation>잘못된 파일: 인식할 수 없는 헤더</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="263"/>
        <source>invalid file: &quot;length&quot; field wrong</source>
        <translation>잘못된 파일: 길이 필드가 잘못됨</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="264"/>
        <source>invalid file: &quot;length2&quot; field wrong</source>
        <translation>잘못된 파일: 길이 필드2가 잘못됨</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="265"/>
        <source>invalid file: internal checksum error</source>
        <translation>잘못된 파일: 내부 체크섬 오류</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="266"/>
        <source>invalid file: &quot;length3&quot; field wrong</source>
        <translation>잘못된 파일: 길이 필드3가 잘못됨</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="267"/>
        <source>unknown</source>
        <translation>알 수 없음</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="50"/>
        <source>Bootloader installation requires you to provide a firmware file of the original firmware (hex file). You need to download this file yourself due to legal reasons. Please refer to the &lt;a href=&apos;https://www.rockbox.org/manual.shtml&apos;&gt;manual&lt;/a&gt; and the &lt;a href=&apos;https://www.rockbox.org/wiki/IriverBoot#Download_and_extract_a_recent_ve&apos;&gt;IriverBoot&lt;/a&gt; wiki page on how to obtain this file.&lt;br/&gt;Press Ok to continue and browse your computer for the firmware file.</source>
        <translation>부트로더를 설치하려면 원본 펌웨어의 펌웨어 파일(hex 파일)을 제공해야 합니다. 법적인 이유로 이 파일을 직접 다운로드해야 합니다. 이 파일을 얻는 방법은 &lt;a href=&apos;https://www.rockbox.org/manual.shtml&apos;&gt;설명서&lt;/a&gt; 및  &lt;a href=&apos;https://www.rockbox.org/wiki/IriverBoot#Download_and_extract_a_recent_ve&apos;&gt;IriverBoot&lt;/a&gt; 위키 페이지에서 이 파일을 얻는 방법을 알아보세요. &lt;br/&gt; 계속해서 컴퓨터에서 펌웨어 파일을 찾으려면 확인을 누르세요.</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallImx</name>
    <message>
        <location filename="../base/bootloaderinstallimx.cpp" line="72"/>
        <source>Bootloader installation requires you to provide a copy of the original Sandisk firmware (firmware.sb file). This file will be patched with the Rockbox bootloader and installed to your player. You need to download this file yourself due to legal reasons. Please browse the &lt;a href=&apos;http://forums.sandisk.com/sansa/&apos;&gt;Sansa Forums&lt;/a&gt; or refer to the &lt;a href= &apos;https://www.rockbox.org/wiki/SansaFuzePlus&apos;&gt;SansaFuzePlus&lt;/a&gt; wiki page on how to obtain this file.&lt;br/&gt;Press Ok to continue and browse your computer for the firmware file.</source>
        <translation>부트로더를 설치하려면 원래 Sandisk 펌웨어(firmware.sb 파일) 사본을 제공해야 합니다. 이 파일은 Rockbox 부트로더로 패치되어 플레이어에 설치됩니다. 법적 이유로 이 파일은 직접 다운로드해야 합니다. 이 파일을 얻는 방법은 &lt;a href=&apos;http://forums.sandisk.com/sansa/&apos;&gt;Sansa 포럼&lt;/a&gt;을 찾아보거나 &lt;a href=&apos;https://www.rockbox.org/wiki/SansaFuzePlus&apos;&gt;SansaFuzePlus&lt;/a&gt; 위키 페이지를 참조하세요.&lt;br/&gt; 계속해서 컴퓨터에서 펌웨어 파일을 검색하려면 확인을 누르세요.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallimx.cpp" line="94"/>
        <source>Could not read original firmware file</source>
        <translation>기존 펌웨어 파일을 읽을 수 없음</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallimx.cpp" line="100"/>
        <source>Downloading bootloader file</source>
        <translation>부트로더 파일 다운로드</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallimx.cpp" line="110"/>
        <source>Patching file...</source>
        <translation>파일 패치 중 ...</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallimx.cpp" line="136"/>
        <source>Patching the original firmware failed</source>
        <translation>기존 펌웨어 패치에 실패함</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallimx.cpp" line="142"/>
        <source>Succesfully patched firmware file</source>
        <translation>펌웨어 파일을 성공적으로 패치함</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallimx.cpp" line="157"/>
        <source>Bootloader successful installed</source>
        <translation>부트로더가 성공적으로 설치됨</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallimx.cpp" line="163"/>
        <source>Patched bootloader could not be installed</source>
        <translation>패치된 부트로더를 설치할 수 없음</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallimx.cpp" line="174"/>
        <source>To uninstall, perform a normal upgrade with an unmodified original firmware.</source>
        <translation>설치 제거하려면 수정되지 않은 기존 펌웨어로 일반적인 업그레이드를 실행하세요.</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallIpod</name>
    <message>
        <source>Error: can&apos;t allocate buffer memory!</source>
        <translation>오류: 버퍼 메모리를 할당할 수 없습니다!</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="72"/>
        <source>Downloading bootloader file</source>
        <translation>부트로더 파일 다운로드 중</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="56"/>
        <location filename="../base/bootloaderinstallipod.cpp" line="143"/>
        <source>Failed to read firmware directory</source>
        <translation>펌웨어 디렉터리 읽기 실패함</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="61"/>
        <location filename="../base/bootloaderinstallipod.cpp" line="148"/>
        <source>Unknown version number in firmware (%1)</source>
        <translation>펌웨어 (%1)의 알 수 없는 버전 번호</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="67"/>
        <source>Warning: This is a MacPod, Rockbox only runs on WinPods.
See https://www.rockbox.org/wiki/IpodConversionToFAT32</source>
        <translation>경고: 이것은 MacPod이며, 록박스는 WinPod에서만 실행됩니다.
https://www.rockbox.org/wiki/IpodConversionToFAT32 참고</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="86"/>
        <location filename="../base/bootloaderinstallipod.cpp" line="155"/>
        <source>Could not open Ipod in R/W mode</source>
        <translation>R/W 모드에서 iPod을 열 수 없음</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="96"/>
        <source>Successfull added bootloader</source>
        <translation>부트로더 추가 성공</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="107"/>
        <source>Failed to add bootloader</source>
        <translation>부트로더 추가 실패함</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="119"/>
        <source>Bootloader Installation complete.</source>
        <translation>부트로더 설치가 완료되었습니다.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="124"/>
        <source>Writing log aborted</source>
        <translation>로그 쓰기가 중단됨</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="161"/>
        <source>No bootloader detected.</source>
        <translation>부트로더가 감지되지 않았습니다.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="241"/>
        <source>Error: could not retrieve device name</source>
        <translation>오류: 장치 이름을 검색할 수 없음</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="257"/>
        <source>Error: no mountpoint specified!</source>
        <translation>오류: 마운트 지점이 지정되지 않았습니다!</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="262"/>
        <source>Could not open Ipod: permission denied</source>
        <translation>iPod을 열 수 없음: 권한이 거부됨</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="266"/>
        <source>Could not open Ipod</source>
        <translation>Ipod을 열 수 없음</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="277"/>
        <source>No firmware partition on disk</source>
        <translation>디스크에 펌웨어 파티션이 없음</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="167"/>
        <source>Successfully removed bootloader</source>
        <translation>부트로더를 성공적으로 제거함</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="175"/>
        <source>Removing bootloader failed.</source>
        <translation>부트로더 제거를 실패하였습니다.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="82"/>
        <source>Installing Rockbox bootloader</source>
        <translation>록박스 부트로더 설치</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="134"/>
        <source>Uninstalling bootloader</source>
        <translation>부트로더 설치 제거 중</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="271"/>
        <source>Error reading partition table - possibly not an Ipod</source>
        <translation>파티션 테이블을 읽는 중 오류 발생 - 아마도 Ipod이 아닐 수도 있음</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallMi4</name>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="34"/>
        <source>Downloading bootloader</source>
        <translation>부트로더 다운로드 중</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="43"/>
        <source>Installing Rockbox bootloader</source>
        <translation>록박스 부트로더 설치</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="66"/>
        <source>A firmware file is already present on player</source>
        <translation>플레이어에 펌웨어 파일이 이미 존재함</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="71"/>
        <location filename="../base/bootloaderinstallmi4.cpp" line="79"/>
        <source>Bootloader successful installed</source>
        <translation>부트로더가 성공적으로 설치됨</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="74"/>
        <source>Copying modified firmware file failed</source>
        <translation>수정된 펌웨어 파일 복사에 실패함</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="91"/>
        <source>Checking for Rockbox bootloader</source>
        <translation>록박스 부트로더 확인 중</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="93"/>
        <source>No Rockbox bootloader found</source>
        <translation>록박스 부트로더를 찾을 수 없음</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="99"/>
        <source>Checking for original firmware file</source>
        <translation>원본 펌웨어 파일 확인 중</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="104"/>
        <source>Error finding original firmware file</source>
        <translation>원본 펌웨어 파일을 찾는 중 오류 발생</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="115"/>
        <source>Rockbox bootloader successful removed</source>
        <translation>록박스 부트로더가 성공적으로 제거됨</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallMpio</name>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="34"/>
        <source>Bootloader installation requires you to provide a firmware file of the original firmware (bin file). You need to download this file yourself due to legal reasons. Please refer to the &lt;a href=&apos;https://www.rockbox.org/manual.shtml&apos;&gt;manual&lt;/a&gt; and the &lt;a href=&apos;https://www.rockbox.org/wiki/MPIOHD200Port&apos;&gt;MPIOHD200Port&lt;/a&gt; wiki page on how to obtain this file.&lt;br/&gt;Press Ok to continue and browse your computer for the firmware file.</source>
        <translation>부트로더를 설치하려면 원본 펌웨어의 펌웨어 파일(bin 파일)을 제공해야 합니다. 법적인 이유로 이 파일을 직접 다운로드해야 합니다. 이 파일을 얻는 방법은 &lt;a href=&apos;https://www.rockbox.org/manual.shtml&apos;&gt;설명서&lt;/a&gt; 및 &lt;a href=&apos;https://www.rockbox.org/wiki/MPIOHD200Port&apos;&gt;MPIOHD200Port&lt;/a&gt; 위키 페이지를 참조하세요. &lt;br/&gt; 계속해서 컴퓨터에서 펌웨어 파일을 검색하려면 확인을 누르세요.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="53"/>
        <source>Downloading bootloader file</source>
        <translation>부트로더 파일 다운로드 중</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="80"/>
        <source>Could not open the original firmware.</source>
        <translation>기존 펌웨어를 열 수 없습니다.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="83"/>
        <source>Could not read the original firmware.</source>
        <translation>원래 펌웨어를 읽을 수 없습니다.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="86"/>
        <source>Loaded firmware file does not look like MPIO original firmware file.</source>
        <translation>로드된 펌웨어 파일이 MPIO 기존 펌웨어 파일과 다릅니다.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="101"/>
        <source>Could not open output file.</source>
        <translation>출력 파일을 열 수 없습니다.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="104"/>
        <source>Could not write output file.</source>
        <translation>출력 파일을 쓸 수 없습니다.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="107"/>
        <source>Unknown error number: %1</source>
        <translation>알 수 없는 오류 번호: %1</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="89"/>
        <source>Could not open downloaded bootloader.</source>
        <translation>다운로드한 부트로더를 열 수 없습니다.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="92"/>
        <source>Place for bootloader in OF file not empty.</source>
        <translation>OF 파일의 부트로더 위치가 비어 있지 않습니다.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="95"/>
        <source>Could not read the downloaded bootloader.</source>
        <translation>다운로드한 부트로더를 읽을 수 없습니다.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="98"/>
        <source>Bootloader checksum error.</source>
        <translation>부트로더 체크섬 오류입니다.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="112"/>
        <source>Patching original firmware failed: %1</source>
        <translation>기존 펌웨어 패치 실패: %1</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="119"/>
        <source>Success: modified firmware file created</source>
        <translation>성공: 수정된 펌웨어 파일이 생성됨</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="127"/>
        <source>To uninstall, perform a normal upgrade with an unmodified original firmware</source>
        <translation>제거하려면 수정되지 않은 기존 펌웨어로 일반 업그레이드를 수행</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallS5l</name>
    <message>
        <location filename="../base/bootloaderinstalls5l.cpp" line="61"/>
        <source>Could not find mounted iPod.</source>
        <translation>마운트된 iPod을 찾을 수 없습니다.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalls5l.cpp" line="68"/>
        <source>Downloading bootloader file...</source>
        <translation>부트로더 파일 다운로드 중...</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalls5l.cpp" line="113"/>
        <source>Could not make DFU image.</source>
        <translation>DFU 이미지를 만들 수 없습니다.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalls5l.cpp" line="119"/>
        <source>Ejecting iPod...</source>
        <translation>iPod을 꺼내는 중...</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalls5l.cpp" line="151"/>
        <source>Device successfully ejected.</source>
        <translation>장치가 성공적으로 꺼내졌습니다.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalls5l.cpp" line="179"/>
        <source>iTunes closed.</source>
        <translation>iTunes가 닫혔습니다.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalls5l.cpp" line="201"/>
        <source>Waiting for HDD spin-down...</source>
        <translation>HDD 스핀다운을 기다리는 중...</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalls5l.cpp" line="217"/>
        <source>Waiting for DFU mode...</source>
        <translation>DFU 모드를 기다리는 중...</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalls5l.cpp" line="218"/>
        <source>Action required:

Press and hold SELECT+MENU buttons, after about 12 seconds a new action will require you to release the buttons, DO IT QUICKLY, otherwise the process could fail.</source>
        <translation>필요한 작업:

선택+메뉴 버튼을 길게 누르고, 약 12초 후에 새로운 동작이 발생하면 버튼을 놓아야 합니다. 빨리 하세요. 그렇지 않으면 프로세스가 실패할 수 있습니다.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalls5l.cpp" line="241"/>
        <source>DFU mode detected.</source>
        <translation>DFU 모드가 감지되었습니다.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalls5l.cpp" line="171"/>
        <source>Action required:

Quit iTunes application.</source>
        <translation>필요한 작업:

iTunes 응용 프로그램을 종료합니다.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalls5l.cpp" line="192"/>
        <source>Could not suspend iTunesHelper. Stop it using the Task Manager, and try again.</source>
        <translation>iTunesHelper를 일시 중지할 수 없습니다. 작업 관리자를 사용하여 중지하고 다시 시도하세요.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalls5l.cpp" line="243"/>
        <source>Action required:

Release SELECT+MENU buttons and wait...</source>
        <translation>필요한 작업:

선택+메뉴 버튼에서 손을 떼고 기다리세요...</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalls5l.cpp" line="275"/>
        <source>Transfering DFU image...</source>
        <translation>DFU 이미지 전송 중...</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalls5l.cpp" line="268"/>
        <source>Device is not in DFU mode. It seems that the previous required action failed, please try again.</source>
        <translation>장치가 DFU 모드에 있지 않습니다. 이전 필수 작업이 실패한 것 같습니다. 다시 시도해 주세요.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalls5l.cpp" line="141"/>
        <source>Action required:

Please make sure no programs are accessing files on the device. If ejecting still fails please use your computers eject functionality.</source>
        <translation>필요한 작업:

어떤 프로그램도 장치의 파일에 접속하지 않는지 확인하세요. 여전히 꺼내기가 실패하면 컴퓨터의 꺼내기 기능을 사용하세요.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalls5l.cpp" line="285"/>
        <source>No valid DFU USB driver found.

Install iTunes (or the Apple Device Driver) and try again.</source>
        <translation>유효한 DFU USB 드라이버를 찾을 수 없습니다.

iTunes(또는 Apple 장치 드라이버)를 설치하고 다시 시도하세요.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalls5l.cpp" line="294"/>
        <source>Could not transfer DFU image.</source>
        <translation>DFU 이미지를 전송할 수 없습니다.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalls5l.cpp" line="299"/>
        <source>DFU transfer completed.</source>
        <translation>DFU 전송이 완료되었습니다.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalls5l.cpp" line="302"/>
        <source>Restarting iPod, waiting for remount...</source>
        <translation>iPod를 재시작하고 다시 마운트할 때까지 기다리는 중...</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalls5l.cpp" line="321"/>
        <source>Action required:

Could not remount the device, try to do it manually. If the iPod didn&apos;t restart, force a reset by pressing SELECT+MENU buttons for about 5 seconds. If the problem could not be solved then click &apos;Abort&apos; to cancel.</source>
        <translation>필요한 작업:

장치를 다시 마운트할 수 없습니다. 수동으로 시도해 보세요. iPod가 재시작되지 않으면 선택+메뉴 버튼을 약 5초 동안 눌러 강제로 재설정하세요. 문제가 해결되지 않으면 &apos;중단&apos;을 클릭하여 취소하세요.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalls5l.cpp" line="333"/>
        <source>Device remounted.</source>
        <translation>장치가 다시 마운트되었습니다.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalls5l.cpp" line="336"/>
        <source>Bootloader successfully installed.</source>
        <translation>부트로더가 성공적으로 설치되었습니다.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalls5l.cpp" line="338"/>
        <source>Bootloader successfully uninstalled.</source>
        <translation>부트로더가 성공적으로 설치 제거되었습니다.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalls5l.cpp" line="368"/>
        <source>Install aborted by user.</source>
        <translation>사용자에 의해 설치가 중단되었습니다.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalls5l.cpp" line="370"/>
        <source>Uninstall aborted by user.</source>
        <translation>사용자에 의해 설치 제거가 중단되었습니다.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalls5l.cpp" line="350"/>
        <source>Could not resume iTunesHelper.</source>
        <translation>iTunesHelper를 다시 시작할 수 없습니다.</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallSansa</name>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="54"/>
        <source>Error: can&apos;t allocate buffer memory!</source>
        <translation>오류: 버퍼 메모리를 할당할 수 없습니다!</translation>
    </message>
    <message>
        <source>Searching for Sansa</source>
        <translation>Sansa를 찾는 중</translation>
    </message>
    <message>
        <source>Permission for disc access denied!
This is required to install the bootloader</source>
        <translation>디스크 접근 권한이 거부되었습니다!
부트로더를 설치하는 데 필요함</translation>
    </message>
    <message>
        <source>No Sansa detected!</source>
        <translation>Sansa가 감지되지 않았습니다!</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="68"/>
        <source>Downloading bootloader file</source>
        <translation>부트로더 파일 다운로드 중</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="60"/>
        <location filename="../base/bootloaderinstallsansa.cpp" line="164"/>
        <source>OLD ROCKBOX INSTALLATION DETECTED, ABORTING.
You must reinstall the original Sansa firmware before running
sansapatcher for the first time.
See https://www.rockbox.org/wiki/SansaE200Install
</source>
        <translation>오래된 록박스 설치가 감지되어 중단됩니다.
sansapatcher를 처음 실행하기 전에
기존 Sansa 펌웨어를 다시 설치해야 합니다.
https://www.rockbox.org/wiki/SansaE200Install 참고
</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="87"/>
        <location filename="../base/bootloaderinstallsansa.cpp" line="174"/>
        <source>Could not open Sansa in R/W mode</source>
        <translation>R/W 모드에서 Sansa를 열 수 없습니다.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="114"/>
        <source>Successfully installed bootloader</source>
        <translation>부트로더가 성공적으로 설치됨</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="125"/>
        <source>Failed to install bootloader</source>
        <translation>부트로더를 설치 실패함</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="138"/>
        <source>Bootloader Installation complete.</source>
        <translation>부트로더 설치가 완료되었습니다.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="143"/>
        <source>Writing log aborted</source>
        <translation>로그 쓰기가 중단됨</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="244"/>
        <source>Error: could not retrieve device name</source>
        <translation>오류: 장치 이름을 검색할 수 없음</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="260"/>
        <source>Can&apos;t find Sansa</source>
        <translation>Sansa를 찾을 수 없음</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="265"/>
        <source>Could not open Sansa</source>
        <translation>Sansa를 열 수 없음</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="270"/>
        <source>Could not read partition table</source>
        <translation>파티션 테이블을 읽을 수 없음</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="277"/>
        <source>Disk is not a Sansa (Error %1), aborting.</source>
        <translation>디스크가 Sansa가 아니므로 (오류 %1), 중단합니다.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="180"/>
        <source>Successfully removed bootloader</source>
        <translation>부트로더를 성공적으로 제거함</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="188"/>
        <source>Removing bootloader failed.</source>
        <translation>부트로더를 제거하지 못했습니다.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="83"/>
        <source>Installing Rockbox bootloader</source>
        <translation>록박스 부트로더 설치</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="155"/>
        <source>Uninstalling bootloader</source>
        <translation>부트로더 설치 제거 중</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="96"/>
        <source>Checking downloaded bootloader</source>
        <translation>다운로드된 부트로더 확인 중</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="104"/>
        <source>Bootloader mismatch! Aborting.</source>
        <translation>부트로더 불일치합니다! 중단하는 중입니다.</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallTcc</name>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="33"/>
        <source>Bootloader installation requires you to provide a firmware file of the original firmware (bin file). You need to download this file yourself due to legal reasons. Please refer to the &lt;a href=&apos;https://www.rockbox.org/manual.shtml&apos;&gt;manual&lt;/a&gt; and the &lt;a href=&apos;https://www.rockbox.org/wiki/CowonD2Info&apos;&gt;CowonD2Info&lt;/a&gt; wiki page on how to obtain the file.&lt;br/&gt;Press Ok to continue and browse your computer for the firmware file.</source>
        <translation>부트로더를 설치하려면 원본 펌웨어의 펌웨어 파일(bin 파일)을 제공해야 합니다. 법적인 이유로 이 파일을 직접 다운로드해야 합니다. 이 파일을 얻는 방법은 &lt;a href=&apos;https://www.rockbox.org/manual.shtml&apos;&gt;설명서&lt;/a&gt; 및 &lt;a href=&apos;https://www.rockbox.org/wiki/CowonD2Info&apos;&gt;CowonD2Info&lt;/a&gt; 위키 페이지를 참조하세요. &lt;br/&gt; 계속해서 컴퓨터에서 펌웨어 파일을 찾으려면 확인을 누르세요.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="50"/>
        <source>Downloading bootloader file</source>
        <translation>부트로더 파일 다운로드 중</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="82"/>
        <location filename="../base/bootloaderinstalltcc.cpp" line="99"/>
        <source>Could not load %1</source>
        <translation>%1을(를) 로드할 수 없음</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="90"/>
        <source>Unknown OF file used: %1</source>
        <translation>알 수 없는 OF 파일이 사용됨: %1</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="104"/>
        <source>Patching Firmware...</source>
        <translation>펌웨어 패치 중...</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="111"/>
        <source>Could not patch firmware</source>
        <translation>펌웨어를 패치할 수 없음</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="117"/>
        <source>Could not open %1 for writing</source>
        <translation>쓰기 위해 %1을(를) 열 수 없음</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="126"/>
        <source>Could not write firmware file</source>
        <translation>펌웨어 파일을 쓸 수 없음</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="131"/>
        <source>Success: modified firmware file created</source>
        <translation>성공: 수정된 펌웨어 파일이 생성됨</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="151"/>
        <source>To uninstall, perform a normal upgrade with an unmodified original firmware</source>
        <translation>설치 제거하려면, 수정되지 않은 기존 펌웨어로 일반 업그레이드 수행</translation>
    </message>
</context>
<context>
    <name>Changelog</name>
    <message>
        <location filename="../gui/changelogfrm.ui" line="17"/>
        <source>Changelog</source>
        <translation>변경 이력</translation>
    </message>
    <message>
        <location filename="../gui/changelogfrm.ui" line="39"/>
        <source>Show on startup</source>
        <translation>시작시 표시</translation>
    </message>
    <message>
        <location filename="../gui/changelogfrm.ui" line="46"/>
        <source>&amp;Ok</source>
        <translation>확인(&amp;O)</translation>
    </message>
</context>
<context>
    <name>Config</name>
    <message>
        <location filename="../configure.cpp" line="853"/>
        <source>Autodetection</source>
        <translation>자동 감지</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="854"/>
        <source>Could not detect a Mountpoint.
Select your Mountpoint manually.</source>
        <translation>마운트 지점을 감지할 수 없습니다.
마운트 지점을 수동으로 선택하세요.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="756"/>
        <source>Could not detect a device.
Select your device and Mountpoint manually.</source>
        <translation>장치를 감지할 수 없습니다.
장치와 마운트 지점을 수동으로 선택하세요.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="789"/>
        <source>The player contains an incompatible filesystem.
Make sure you selected the correct mountpoint and the player is set up to use a filesystem compatible with Rockbox.</source>
        <translation>플레이어에 호환되지 않는 파일시스템이 포함되어 있습니다.
올바른 마운트 지점을 선택했는지, 플레이어가 록박스와 호환되는 파일 시스템을 사용하도록 설정되었는지 확인하세요.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="797"/>
        <source>An unknown error occured during player detection.</source>
        <translation>플레이어 감지 중에 알 수 없는 오류가 발생했습니다.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="864"/>
        <source>Really delete cache?</source>
        <translation>정말 캐시를 삭제합니까?</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="865"/>
        <source>Do you really want to delete the cache? Make absolutely sure this setting is correct as it will remove &lt;b&gt;all&lt;/b&gt; files in this folder!</source>
        <translation>캐시를 삭제합니까? 이 폴더의 &lt;b&gt;모든&lt;/b&gt; 파일이 제거되므로 이 설정이 올바른지 반드시 확인하세요!</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="873"/>
        <source>Path wrong!</source>
        <translation>경로가 잘못되었습니다!</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="874"/>
        <source>The cache path is invalid. Aborting.</source>
        <translation>캐시 경로가 잘못되었습니다. 중단하는 중입니다.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="310"/>
        <source>Current cache size is %L1 kiB.</source>
        <translation>현재 캐시 크기는 %L1 kiB입니다.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="445"/>
        <location filename="../configure.cpp" line="479"/>
        <source>Configuration OK</source>
        <translation>구성 확인</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="455"/>
        <location filename="../configure.cpp" line="484"/>
        <source>Configuration INVALID</source>
        <translation>구성이 잘못됨</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="125"/>
        <source>The following errors occurred:</source>
        <translation>다음 오류가 발생했습니다:</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="170"/>
        <source>No mountpoint given</source>
        <translation>마운트 지점이 지정되지 않았음</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="174"/>
        <source>Mountpoint does not exist</source>
        <translation>마운트 지점이 존재하지 않음</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="178"/>
        <source>Mountpoint is not a directory.</source>
        <translation>마운트 지점은 디렉터리가 아닙니다.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="182"/>
        <source>Mountpoint is not writeable</source>
        <translation>마운트 지점에 쓸 수 없음</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="197"/>
        <source>No player selected</source>
        <translation>선택한 플레이어가 없음</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="204"/>
        <source>Cache path not writeable. Leave path empty to default to systems temporary path.</source>
        <translation>캐시 경로에 쓸 수 없습니다. 시스템 임시 경로를 기본값으로 지정하려면 경로를 비워 두세요.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="223"/>
        <source>You need to fix the above errors before you can continue.</source>
        <translation>계속하려면 위의 오류를 수정해야 합니다.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="226"/>
        <source>Configuration error</source>
        <translation>구성 오류</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="328"/>
        <source>Showing disabled targets</source>
        <translation>비활성화된 대상 표시</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="329"/>
        <source>You just enabled showing targets that are marked disabled. Disabled targets are not recommended to end users. Please use this option only if you know what you are doing.</source>
        <translation>비활성화된 것으로 표시된 대상 표시를 활성화했습니다. 비활성화된 대상은 최종 사용자에게 권장되지 않습니다. 현재 수행 중인 작업을 알고 있는 경우에만 이 옵션을 사용하세요.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="438"/>
        <location filename="../configure.cpp" line="904"/>
        <source>TTS error</source>
        <translation>TTS 오류</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="439"/>
        <location filename="../configure.cpp" line="905"/>
        <source>The selected TTS failed to initialize. You can&apos;t use this TTS.</source>
        <translation>선택한 TTS를 초기화하는 데 실패했습니다. 이 TTS를 사용할 수 없습니다.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="523"/>
        <source>Proxy Detection</source>
        <translation>프록시 감지</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="524"/>
        <source>The System Proxy settings are invalid!
Rockbox Utility can&apos;t work with this proxy settings. Make sure the system proxy is set correctly. Note that &quot;proxy auto-config (PAC)&quot; scripts are not supported by Rockbox Utility. If your system uses this you need to use manual proxy settings.</source>
        <translation>시스템 프록시 설정이 잘못되었습니다!
록박스 유틸리티는 이 프록시 설정으로 작동할 수 없습니다. 시스템 프록시가 올바르게 설정되었는지 확인하세요. 스크립트를 지원하지 않습니다. 록박스 유틸리티는 &quot;프록시 자동 구성(PAC)&quot; 시스템에서 이를 사용하는 경우 수동 프록시 설정을 사용해야 합니다.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="634"/>
        <source>Set Cache Path</source>
        <translation>캐시 경로 설정</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="656"/>
        <source>%1 (%2 GiB of %3 GiB free)</source>
        <translation>%1 (%3 GiB 중 %2 GiB 여유)</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="730"/>
        <source>Multiple devices have been detected. Please disconnect all players but one and try again.</source>
        <translation>여러 장치가 감지되었습니다. 하나를 제외한 모든 플레이어를 분리하고 다시 시도하세요.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="733"/>
        <source>Detected devices:</source>
        <translation>감지된 장치:</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="738"/>
        <source>(unknown)</source>
        <translation>(알 수 없음)</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="740"/>
        <source>%1 at %2</source>
        <translation>%2의 1%</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="747"/>
        <source>Note: detecting connected devices might be ambiguous. You might have less devices connected than listed. In this case it might not be possible to detect your player unambiguously.</source>
        <translation>참고: 연결된 장치를 감지하는 것은 모호할 수 있습니다. 나열된 것보다 연결된 장치가 적을 수 있습니다. 이 경우 플레이어를 모호하지 않게 감지할 수 없습니다.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="751"/>
        <location filename="../configure.cpp" line="755"/>
        <location filename="../configure.cpp" line="800"/>
        <source>Device Detection</source>
        <translation>장치 감지</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="782"/>
        <source>%1 &quot;MacPod&quot;를 찾았습니다!
Rockbox needs a FAT formatted Ipod (so-called &quot;WinPod&quot;) to run. </source>
        <translation>%1 &quot;MacPod&quot; gefunden!
록박스를 실행하려면 FAT 형식의 아이팟(소위 &quot;WinPod&quot;)이 필요합니다.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="773"/>
        <source>%1 in MTP mode found!
You need to change your player to MSC mode for installation. </source>
        <translation>MTP 모드에서 %1 찾았습니다!
설치를 위해 플레이어를 MSC 모드로 변경해야 합니다.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="766"/>
        <source>Detected an unsupported player:
%1
Sorry, Rockbox doesn&apos;t run on your player.</source>
        <translation>지원되지 않는 플레이어를 감지했습니다:
%1
죄송합니다. 록박스가 플레이어에서 실행되지 않습니다.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="911"/>
        <source>TTS configuration invalid</source>
        <translation>TTS 구성이 잘못됨</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="912"/>
        <source>TTS configuration invalid.
 Please configure TTS engine.</source>
        <translation>TTS 구성이 잘못되었습니다.
 TTS 엔진을 구성하세요.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="917"/>
        <source>Could not start TTS engine.</source>
        <translation>TTS 엔진을 시작할 수 없습니다.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="918"/>
        <source>Could not start TTS engine.
</source>
        <translation>TTS 엔진을 시작할 수 없습니다.
</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="919"/>
        <location filename="../configure.cpp" line="938"/>
        <source>
Please configure TTS engine.</source>
        <translation>
TTS 엔진을 구성하세요.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="933"/>
        <source>Rockbox Utility Voice Test</source>
        <translation>록박스 유틸리티 음성 테스트</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="936"/>
        <source>Could not voice test string.</source>
        <translation>테스트 문자열을 음성으로 출력할 수 없습니다.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="937"/>
        <source>Could not voice test string.
</source>
        <translation>테스트 문자열을 음성으로 출력할 수 없습니다.
</translation>
    </message>
</context>
<context>
    <name>ConfigForm</name>
    <message>
        <location filename="../configurefrm.ui" line="14"/>
        <source>Configuration</source>
        <translation>구성</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="20"/>
        <source>Configure Rockbox Utility</source>
        <translation>록박스 유틸리티 구성</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="547"/>
        <source>&amp;Ok</source>
        <translation>확인(&amp;O)</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="558"/>
        <source>&amp;Cancel</source>
        <translation>취소(&amp;C)</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="136"/>
        <source>&amp;Proxy</source>
        <translation>프록시(&amp;P)</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="95"/>
        <source>Show disabled targets</source>
        <translation>비활성화된 대상 표시</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="142"/>
        <source>&amp;No Proxy</source>
        <translation>프록시 없음(&amp;N)</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="159"/>
        <source>&amp;Manual Proxy settings</source>
        <translation>수동 프록시 설정(&amp;M)</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="166"/>
        <source>Proxy Values</source>
        <translation>프록시 값</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="172"/>
        <source>&amp;Host:</source>
        <translation>호스트(&amp;H):</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="182"/>
        <source>&amp;Port:</source>
        <translation>포트(&amp;P):</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="199"/>
        <source>&amp;Username</source>
        <translation>사용자이름(&amp;U)</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="267"/>
        <source>&amp;Language</source>
        <translation>언어(&amp;L)</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="35"/>
        <source>&amp;Device</source>
        <translation>장치(&amp;D)</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="41"/>
        <source>Select your device in the &amp;filesystem</source>
        <translation>파일 시스템에서 장치 선택(&amp;F)</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="326"/>
        <source>&amp;Browse</source>
        <translation>찾아보기(&amp;B)</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="72"/>
        <source>&amp;Select your audio player</source>
        <translation>오디오 플레이어 선택(&amp;S)</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="58"/>
        <source>&amp;Refresh</source>
        <translation>새로고침(&amp;R)</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="114"/>
        <source>&amp;Autodetect</source>
        <translation>자동 감지(&amp;A)</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="152"/>
        <source>Use S&amp;ystem values</source>
        <translation>시스템 값 사용(&amp;Y)</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="209"/>
        <source>Pass&amp;word</source>
        <translation>비밀번호(&amp;A)</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="219"/>
        <source>Show</source>
        <translation>표시</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="281"/>
        <source>Cac&amp;he</source>
        <translation>캐시(&amp;A)</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="284"/>
        <source>Download cache settings</source>
        <translation>캐시 설정 다운로드</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="290"/>
        <source>Rockbox Utility uses a local download cache to save network traffic. You can change the path to the cache and use it as local repository by enabling Offline mode.</source>
        <translation>록박스 유틸리티는 로컬 다운로드 캐시를 사용하여 네트워크 트래픽을 절약합니다. 오프라인 모드를 활성화하면 캐시 경로를 변경하고 이를 로컬 저장소로 사용할 수 있습니다.</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="300"/>
        <source>Current cache size is %1</source>
        <translation>현재 캐시 크기는 %1</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="309"/>
        <source>P&amp;ath</source>
        <translation>경로(&amp;A)</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="341"/>
        <source>Disable local &amp;download cache</source>
        <translation>로컬 다운로드 캐시 비활성화(&amp;D)</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="376"/>
        <source>Clean cache &amp;now</source>
        <translation>지금 캐시 지우기(&amp;N)</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="319"/>
        <source>Entering an invalid folder will reset the path to the systems temporary path.</source>
        <translation>잘못된 폴더를 입력하면 경로가 시스템 임시 경로로 재설정합니다.</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="392"/>
        <source>&amp;TTS &amp;&amp; Encoder</source>
        <translation>&amp;TTS &amp;&amp; 인코더</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="398"/>
        <source>TTS Engine</source>
        <translation>TTS 엔진</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="473"/>
        <source>Encoder Engine</source>
        <translation>인코더 엔진</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="404"/>
        <source>&amp;Select TTS Engine</source>
        <translation>TTS 엔진 선택(&amp;S)</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="417"/>
        <source>Configure TTS Engine</source>
        <translation>TTS 엔진 구성</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="424"/>
        <location filename="../configurefrm.ui" line="479"/>
        <source>Configuration invalid!</source>
        <translation>구성이 잘못되었습니다!</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="441"/>
        <source>Configure &amp;TTS</source>
        <translation>TTS 구성(&amp;T)</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="463"/>
        <source>&amp;Use string corrections for TTS</source>
        <translation>Verwende &amp;Wortkorrektur für TTS</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="496"/>
        <source>Configure &amp;Enc</source>
        <translation>인코더 구성(&amp;E)</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="507"/>
        <source>encoder name</source>
        <translation>인코더 이름</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="452"/>
        <source>Test TTS</source>
        <translation>TTS 테스트</translation>
    </message>
</context>
<context>
    <name>Configure</name>
    <message>
        <location filename="../configure.cpp" line="581"/>
        <source>English</source>
        <comment>This is the localized language name, i.e. your language.</comment>
        <translation>한국어</translation>
    </message>
</context>
<context>
    <name>CreateVoiceFrm</name>
    <message>
        <location filename="../createvoicefrm.ui" line="17"/>
        <source>Create Voice File</source>
        <translation>음성 파일 생성</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="42"/>
        <source>Select the Language you want to generate a voicefile for:</source>
        <translation>음성 파일을 생성하려는 언어를 선택하세요:</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="49"/>
        <source>Generation settings</source>
        <translation>일반 설정</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="72"/>
        <source>Change</source>
        <translation>변경</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="105"/>
        <source>Silence threshold</source>
        <translation>Silence 임계값</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="143"/>
        <source>&amp;Install</source>
        <translation>설치(&amp;I)</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="154"/>
        <source>&amp;Cancel</source>
        <translation>취소(&amp;C)</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="92"/>
        <source>Wavtrim Threshold</source>
        <translation>Wavtrim 임계값</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="55"/>
        <source>TTS:</source>
        <translation>TTS:</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="167"/>
        <source>Language</source>
        <translation>언어</translation>
    </message>
</context>
<context>
    <name>CreateVoiceWindow</name>
    <message>
        <location filename="../createvoicewindow.cpp" line="106"/>
        <source>TTS error</source>
        <translation>TTS 오류</translation>
    </message>
    <message>
        <location filename="../createvoicewindow.cpp" line="107"/>
        <source>The selected TTS failed to initialize. You can&apos;t use this TTS.</source>
        <translation>선택한 TTS를 초기화하지 못했습니다. 이 TTS를 사용할 수 없습니다.</translation>
    </message>
    <message>
        <location filename="../createvoicewindow.cpp" line="111"/>
        <location filename="../createvoicewindow.cpp" line="114"/>
        <source>Engine: &lt;b&gt;%1&lt;/b&gt;</source>
        <translation>TTS 엔진: &lt;b&gt;%1&lt;/b&gt;</translation>
    </message>
</context>
<context>
    <name>EncTtsCfgGui</name>
    <message>
        <location filename="../encttscfggui.cpp" line="44"/>
        <source>Waiting for engine...</source>
        <translation>엔진을 기다리는 중...</translation>
    </message>
    <message>
        <location filename="../encttscfggui.cpp" line="91"/>
        <source>Ok</source>
        <translation>확인</translation>
    </message>
    <message>
        <location filename="../encttscfggui.cpp" line="94"/>
        <source>Cancel</source>
        <translation>취소</translation>
    </message>
    <message>
        <location filename="../encttscfggui.cpp" line="257"/>
        <source>Browse</source>
        <translation>찾아보기</translation>
    </message>
    <message>
        <location filename="../encttscfggui.cpp" line="272"/>
        <source>Refresh</source>
        <translation>새로고침</translation>
    </message>
    <message>
        <location filename="../encttscfggui.cpp" line="263"/>
        <source>Select excutable</source>
        <translation>실행 선택</translation>
    </message>
</context>
<context>
    <name>EncoderExe</name>
    <message>
        <location filename="../base/encoderexe.cpp" line="37"/>
        <source>Path to Encoder:</source>
        <translation>인코더 경로:</translation>
    </message>
    <message>
        <location filename="../base/encoderexe.cpp" line="39"/>
        <source>Encoder options:</source>
        <translation>인코더 옵션:</translation>
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
        <translation>볼륨</translation>
    </message>
    <message>
        <location filename="../base/encoderlame.cpp" line="81"/>
        <source>Quality</source>
        <translation>품질</translation>
    </message>
    <message>
        <location filename="../base/encoderlame.cpp" line="85"/>
        <source>Could not find libmp3lame!</source>
        <translation>libmp3lame을 찾을 수 없습니다!</translation>
    </message>
</context>
<context>
    <name>EncoderRbSpeex</name>
    <message>
        <location filename="../base/encoderrbspeex.cpp" line="34"/>
        <source>Volume:</source>
        <translation>볼륨:</translation>
    </message>
    <message>
        <location filename="../base/encoderrbspeex.cpp" line="36"/>
        <source>Quality:</source>
        <translation>품질:</translation>
    </message>
    <message>
        <location filename="../base/encoderrbspeex.cpp" line="38"/>
        <source>Complexity:</source>
        <translation>복잡성:</translation>
    </message>
    <message>
        <location filename="../base/encoderrbspeex.cpp" line="40"/>
        <source>Use Narrowband:</source>
        <translation>협대역 사용:</translation>
    </message>
</context>
<context>
    <name>InfoWidget</name>
    <message>
        <location filename="../gui/infowidget.cpp" line="30"/>
        <location filename="../gui/infowidget.cpp" line="99"/>
        <source>File</source>
        <translation>파일</translation>
    </message>
    <message>
        <location filename="../gui/infowidget.cpp" line="30"/>
        <location filename="../gui/infowidget.cpp" line="99"/>
        <source>Version</source>
        <translation>버전</translation>
    </message>
    <message>
        <location filename="../gui/infowidget.cpp" line="47"/>
        <source>Loading, please wait ...</source>
        <translation>로딩 중 입니다. 잠시만 기다려주세요...</translation>
    </message>
</context>
<context>
    <name>InfoWidgetFrm</name>
    <message>
        <location filename="../gui/infowidgetfrm.ui" line="14"/>
        <source>Info</source>
        <translation>정보</translation>
    </message>
    <message>
        <location filename="../gui/infowidgetfrm.ui" line="20"/>
        <source>Currently installed packages.&lt;br/&gt;&lt;b&gt;Note:&lt;/b&gt; if you manually installed packages this might not be correct!</source>
        <translation>현재 설치된 패키지입니다.&lt;br/&gt;&lt;b&gt;참고:&lt;/b&gt; 패키지를 수동으로 설치한 경우 이것이 정확하지 않을 수 있습니다!</translation>
    </message>
    <message>
        <location filename="../gui/infowidgetfrm.ui" line="34"/>
        <source>Package</source>
        <translation>패키지</translation>
    </message>
</context>
<context>
    <name>InstallTalkFrm</name>
    <message>
        <location filename="../installtalkfrm.ui" line="17"/>
        <source>Install Talk Files</source>
        <translation>토크 파일 설치</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="42"/>
        <source>Strip Extensions</source>
        <translation>확장자 제거</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="52"/>
        <source>Generate for files</source>
        <translation>파일 생성</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="85"/>
        <source>Generate for folders</source>
        <translation>폴더 생성</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="95"/>
        <source>Recurse into folders</source>
        <translation>폴더로 재귀</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="122"/>
        <source>Ignore files</source>
        <translation>파일 무시</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="132"/>
        <source>Skip existing</source>
        <translation>기존 항목 건너뛰기</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="158"/>
        <source>&amp;Cancel</source>
        <translation>취소(&amp;C)</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="174"/>
        <source>Select folders for Talkfile generation (Ctrl for multiselect)</source>
        <translation>토크 파일 생성을 위한 폴더 선택(다중 선택은 Ctrl)</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="78"/>
        <source>TTS profile:</source>
        <translation>TTS 프로파일:</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="36"/>
        <source>Generation options</source>
        <translation>일반 옵션</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="115"/>
        <source>Change</source>
        <translation>변경</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="147"/>
        <source>&amp;Install</source>
        <translation>설치(&amp;I)</translation>
    </message>
</context>
<context>
    <name>InstallTalkWindow</name>
    <message>
        <location filename="../installtalkwindow.cpp" line="95"/>
        <source>Empty selection</source>
        <translation>빈 선택</translation>
    </message>
    <message>
        <location filename="../installtalkwindow.cpp" line="96"/>
        <source>No files or folders selected. Please select files or folders first.</source>
        <translation>파일이나 폴더가 선택되지 않았습니다. 먼저 파일이나 폴더를 선택하세요.</translation>
    </message>
    <message>
        <location filename="../installtalkwindow.cpp" line="140"/>
        <source>TTS error</source>
        <translation>TTS 오류</translation>
    </message>
    <message>
        <location filename="../installtalkwindow.cpp" line="141"/>
        <source>The selected TTS failed to initialize. You can&apos;t use this TTS.</source>
        <translation>선택한 TTS를 초기화하는 데 실패했습니다. 이 TTS를 사용할 수 없습니다.</translation>
    </message>
</context>
<context>
    <name>MsPackUtil</name>
    <message>
        <location filename="../base/mspackutil.cpp" line="101"/>
        <source>Creating output path failed</source>
        <translation>출력 경로 생성 실패함</translation>
    </message>
    <message>
        <location filename="../base/mspackutil.cpp" line="109"/>
        <source>Error during CAB operation</source>
        <translation>CAB 작동 중 오류</translation>
    </message>
</context>
<context>
    <name>PlayerBuildInfo</name>
    <message>
        <location filename="../base/playerbuildinfo.cpp" line="337"/>
        <source>Stable (Retired)</source>
        <translation>안정 (사용 중지됨)</translation>
    </message>
    <message>
        <location filename="../base/playerbuildinfo.cpp" line="340"/>
        <source>Unusable</source>
        <translation>사용 불가r</translation>
    </message>
    <message>
        <location filename="../base/playerbuildinfo.cpp" line="343"/>
        <source>Unstable</source>
        <translation>불안정</translation>
    </message>
    <message>
        <location filename="../base/playerbuildinfo.cpp" line="346"/>
        <source>Stable</source>
        <translation>안정</translation>
    </message>
    <message>
        <location filename="../base/playerbuildinfo.cpp" line="349"/>
        <source>Unknown</source>
        <translation>알 수 없음</translation>
    </message>
</context>
<context>
    <name>PreviewFrm</name>
    <message>
        <location filename="../previewfrm.ui" line="16"/>
        <source>Preview</source>
        <translation>미리보기</translation>
    </message>
</context>
<context>
    <name>ProgressLoggerFrm</name>
    <message>
        <location filename="../progressloggerfrm.ui" line="18"/>
        <location filename="../progressloggerfrm.ui" line="24"/>
        <source>Progress</source>
        <translation>진행</translation>
    </message>
    <message>
        <location filename="../progressloggerfrm.ui" line="87"/>
        <source>&amp;Abort</source>
        <translation>중단(&amp;A)</translation>
    </message>
    <message>
        <location filename="../progressloggerfrm.ui" line="37"/>
        <source>progresswindow</source>
        <translation>진행창</translation>
    </message>
    <message>
        <location filename="../progressloggerfrm.ui" line="63"/>
        <source>Save Log</source>
        <translation>로그 저장</translation>
    </message>
</context>
<context>
    <name>ProgressLoggerGui</name>
    <message>
        <location filename="../progressloggergui.cpp" line="117"/>
        <source>&amp;Ok</source>
        <translation>확인(&amp;O)</translation>
    </message>
    <message>
        <location filename="../progressloggergui.cpp" line="99"/>
        <source>&amp;Abort</source>
        <translation>중단(&amp;A)</translation>
    </message>
    <message>
        <location filename="../progressloggergui.cpp" line="141"/>
        <source>Save system trace log</source>
        <translation>시스템 추적 로그 저장</translation>
    </message>
</context>
<context>
    <name>QObject</name>
    <message>
        <location filename="../configure.cpp" line="616"/>
        <location filename="../main.cpp" line="102"/>
        <source>LTR</source>
        <extracomment>This string is used to indicate the writing direction. Translate it to &quot;RTL&quot; (without quotes) for RTL languages. Anything else will get treated as LTR language.</extracomment>
        <translation>LTR</translation>
    </message>
    <message>
        <location filename="../base/system.cpp" line="333"/>
        <source>(unknown vendor name) </source>
        <translation>(알 수 없는 제조사) </translation>
    </message>
    <message>
        <location filename="../base/system.cpp" line="351"/>
        <source>(unknown product name)</source>
        <translation>(알 수 없는 제품 이름)</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhelper.cpp" line="107"/>
        <source>Bootloader installation is almost complete. Installation &lt;b&gt;requires&lt;/b&gt; you to perform the following steps manually:</source>
        <translation>부트로더 설치가 거의 완료되었습니다. 설치를 위해서는 다음 단계를 수동으로 수행하는 것이 &lt;b&gt;필요&lt;/b&gt;합니다:</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhelper.cpp" line="113"/>
        <source>&lt;li&gt;Safely remove your player.&lt;/li&gt;</source>
        <translation>&lt;li&gt;플레이어를 안전하게 제거하세요.&lt;/li&gt;</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhelper.cpp" line="119"/>
        <source>&lt;li&gt;Reboot your player into the original firmware.&lt;/li&gt;&lt;li&gt;Perform a firmware upgrade using the update functionality of the original firmware. Please refer to your player&apos;s manual on details.&lt;br/&gt;&lt;b&gt;Important:&lt;/b&gt; updating the firmware is a critical process that must not be interrupted. &lt;b&gt;Make sure the player is charged before starting the firmware update process.&lt;/b&gt;&lt;/li&gt;&lt;li&gt;After the firmware has been updated reboot your player.&lt;/li&gt;</source>
        <translation>&lt;li&gt;플레이어를 기존 펌웨어로 재부팅합니다.&lt;/li&gt;&lt;li&gt;원래 펌웨어의 업데이트 기능을 사용하여 펌웨어 업그레이드를 수행합니다. 자세한 내용은 플레이어 설명서를 참조하세요.&lt;/li&gt;&lt;b&gt;중요:&lt;/b&gt;펌웨어 업데이트는 중단되어서는 안 되는 중요한 프로세스입니다. &lt;b&gt;펌웨어 업데이트 프로세스를 시작하기 전에 플레이어가 충전되어 있는지 확인하십시오.&lt;/b&gt;&lt;/li&gt;&lt;li&gt;펌웨어가 업데이트된 후 플레이어를 재부팅합니다.&lt;/li&gt;</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhelper.cpp" line="130"/>
        <source>&lt;li&gt;Remove any previously inserted microSD card&lt;/li&gt;</source>
        <translation>&lt;li&gt;이전에 연결한 마이크로SD 카드 제거&lt;/li&gt;</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhelper.cpp" line="131"/>
        <source>&lt;li&gt;Disconnect your player. The player will reboot and perform an update of the original firmware. Please refer to your players manual on details.&lt;br/&gt;&lt;b&gt;Important:&lt;/b&gt; updating the firmware is a critical process that must not be interrupted. &lt;b&gt;Make sure the player is charged before disconnecting the player.&lt;/b&gt;&lt;/li&gt;&lt;li&gt;After the firmware has been updated reboot your player.&lt;/li&gt;</source>
        <translation>&lt;li&gt;플레이어를 분리합니다. 플레이어가 재부팅되고 원래 펌웨어가 업데이트됩니다. 자세한 내용은 플레이어 설명서를 참조하세요.&lt;br/&gt;&lt;b&gt;중요:&lt;/b&gt;펌웨어 업데이트는 중단되어서는 안 되는 중요한 프로세스입니다. &lt;b&gt;플레이어를 분리하기 전에 플레이어가 충전되었는지 확인하세요.&lt;/b&gt;&lt;/li&gt;&lt;li&gt;펌웨어가 업데이트되면 플레이어를 재부팅하세요.&lt;/li&gt;</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhelper.cpp" line="142"/>
        <source>&lt;li&gt;Turn the player off&lt;/li&gt;&lt;li&gt;Insert the charger&lt;/li&gt;</source>
        <translation>&lt;li&gt;플레이어 끄기&lt;/li&gt;&lt;li&gt;충전기 연결&lt;/li&gt;</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhelper.cpp" line="147"/>
        <source>&lt;li&gt;Unplug USB and power adaptors&lt;/li&gt;&lt;li&gt;Hold &lt;i&gt;Power&lt;/i&gt; to turn the player off&lt;/li&gt;&lt;li&gt;Toggle the battery switch on the player&lt;/li&gt;&lt;li&gt;Hold &lt;i&gt;Power&lt;/i&gt; to boot into Rockbox&lt;/li&gt;</source>
        <translation>&lt;li&gt;USB와 전원 어댑터를 분리&lt;/li&gt;&lt;li&gt;플레이어를 끄려면 &lt;i&gt;전원&lt;/i&gt; 버튼을 길게 누름&lt;/li&gt;&lt;li&gt;플레이어의 배터리 스위치 전환&lt;/li&gt;&lt;li&gt;록박스로 부팅하려면 &lt;i&gt;전원&lt;/i&gt; 버튼을 을 누름&lt;/li&gt;</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhelper.cpp" line="153"/>
        <source>&lt;p&gt;&lt;b&gt;Note:&lt;/b&gt; You can safely install other parts first, but the above steps are &lt;b&gt;required&lt;/b&gt; to finish the installation!&lt;/p&gt;</source>
        <translation>&lt;p&gt;&lt;b&gt;참고:&lt;/b&gt; 다른 부분을 먼저 안전하게 설치할 수 있지만, 위의 단계는 설치를 완료하는데 &lt;b&gt;필요&lt;/b&gt;합니다!&lt;/p&gt;</translation>
    </message>
</context>
<context>
    <name>QuaGzipFile</name>
    <message>
        <location filename="../quazip/quagzipfile.cpp" line="60"/>
        <location filename="../quazip-1.2/quazip/quagzipfile.cpp" line="60"/>
        <source>QIODevice::Append is not supported for GZIP</source>
        <translation>QIODevice::Append는 GZIP에서 지원되지 않음</translation>
    </message>
    <message>
        <location filename="../quazip/quagzipfile.cpp" line="66"/>
        <location filename="../quazip-1.2/quazip/quagzipfile.cpp" line="66"/>
        <source>Opening gzip for both reading and writing is not supported</source>
        <translation>읽기와 쓰기를 위해 gzip을 여는 것은 지원되지 않음</translation>
    </message>
    <message>
        <location filename="../quazip/quagzipfile.cpp" line="74"/>
        <location filename="../quazip-1.2/quazip/quagzipfile.cpp" line="74"/>
        <source>You can open a gzip either for reading or for writing. Which is it?</source>
        <translation>gzip은 읽기 또는 쓰기를 위해 열 수 있습니다. 어떤 것이 맞나요?</translation>
    </message>
    <message>
        <location filename="../quazip/quagzipfile.cpp" line="80"/>
        <location filename="../quazip-1.2/quazip/quagzipfile.cpp" line="80"/>
        <source>Could not gzopen() file</source>
        <translation>gzopen() 파일을 열 수 없음</translation>
    </message>
</context>
<context>
    <name>QuaZIODevice</name>
    <message>
        <location filename="../quazip/quaziodevice.cpp" line="188"/>
        <location filename="../quazip-1.2/quazip/quaziodevice.cpp" line="188"/>
        <source>QIODevice::Append is not supported for QuaZIODevice</source>
        <translation>QIODevice::Append는 QuaZIODevice에서 지원되지 않음</translation>
    </message>
    <message>
        <location filename="../quazip/quaziodevice.cpp" line="193"/>
        <location filename="../quazip-1.2/quazip/quaziodevice.cpp" line="193"/>
        <source>QIODevice::ReadWrite is not supported for QuaZIODevice</source>
        <translation>QIODevice::ReadWrite는 QuaZIODevice에서 지원되지 않음</translation>
    </message>
</context>
<context>
    <name>QuaZipFile</name>
    <message>
        <location filename="../quazip/quazipfile.cpp" line="251"/>
        <location filename="../quazip-1.2/quazip/quazipfile.cpp" line="251"/>
        <location filename="../quazip_/quazipfile.cpp" line="251"/>
        <source>ZIP/UNZIP API error %1</source>
        <translation>ZIP/Unzip API 오류 %1</translation>
    </message>
</context>
<context>
    <name>RbUtilQt</name>
    <message>
        <location filename="../rbutilqt.cpp" line="493"/>
        <source>Confirm Uninstallation</source>
        <translation>설치 제거 확인</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="494"/>
        <source>Do you really want to uninstall the Bootloader?</source>
        <translation>정말 부트로더를 제거할까요?</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="506"/>
        <source>No uninstall method for this target known.</source>
        <translation>이 대상에 대한 제거 방법은 알려져 있지 않습니다.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="529"/>
        <source>No Rockbox bootloader found.</source>
        <translation>록박스 부트로더를 찾을 수 없습니다.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="548"/>
        <source>Confirm installation</source>
        <translation>설치 확인</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="549"/>
        <source>Do you really want to install Rockbox Utility to your player? After installation you can run it from the players hard drive.</source>
        <translation>플레이어에 록박스 유틸리티를 설치하시겠습니까? 설치 후 플레이어의 하드 드라이브에서 실행할 수 있습니다.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="558"/>
        <source>Installing Rockbox Utility</source>
        <translation>록박스 유틸리티 설치</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="708"/>
        <source>Rockbox Utility Update available</source>
        <translation>록박스 유틸리티 업데이트 사용 가능</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="709"/>
        <source>&lt;b&gt;New Rockbox Utility version available.&lt;/b&gt;&lt;br&gt;&lt;br&gt;You are currently using version %1. Get version %2 at &lt;a href=&apos;%3&apos;&gt;%3&lt;/a&gt;</source>
        <translation>&lt;b&gt;새로운 록박스 유틸리티 버전이 출시되었습니다.&lt;/b&gt;&lt;br&gt;&lt;br&gt;현재 버전 %1을 사용하고 있습니다. 버전 %2를 &lt;a href=&apos;%3&apos;&gt;%3&lt;/a&gt;에서 받으세요.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="713"/>
        <source>New version of Rockbox Utility available.</source>
        <translation>록박스 유틸리티의 새 버전이 출시되었습니다.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="716"/>
        <source>Rockbox Utility is up to date.</source>
        <translation>록박스 유틸리티가 최신 버전입니다.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="739"/>
        <source>Device ejected</source>
        <translation>장치 꺼내짐</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="740"/>
        <source>Device successfully ejected. You may now disconnect the player from the PC.</source>
        <translation>장치가 성공적으로 꺼졌습니다. 이제 PC에서 플레이어 연결을 끊을 수 있습니다.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="744"/>
        <source>Ejecting failed</source>
        <translation>꺼내기 실패</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="745"/>
        <source>Ejecting the device failed. Please make sure no programs are accessing files on the device. If ejecting still fails please use your computers eject funtionality.</source>
        <translation>장치를 꺼내지 못했습니다. 장치의 파일에 접속하는 프로그램이 없는지 확인하세요. 여전히 꺼내기에 실패하면 컴퓨터의 꺼내기 기능을 사용하세요.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="562"/>
        <source>Mount point is wrong!</source>
        <translation>마운트 지점이 잘못되었습니다!</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="227"/>
        <source>Certificate error</source>
        <translation>인증서 오류</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="229"/>
        <source>%1

Issuer: %2
Subject: %3
Valid since: %4
Valid until: %5

Temporarily trust certificate?</source>
        <translation>%1

Aussteller: %2
Eigentümer: %3
유효 기간: %4
만료일: %5

인증서를 일시적으로 신뢰할까요?</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="518"/>
        <source>Rockbox Utility can not uninstall the bootloader on your player. Please perform a firmware update using your player vendors firmware update process.</source>
        <translation>록박스 유틸리티는 플레이어의 부트로더를 제거할 수 없습니다. 플레이어 공급업체의 펌웨어 업데이트 프로세스를 사용하여 펌웨어 업데이트를 수행하세요.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="521"/>
        <source>Important: make sure to boot your player into the original firmware before using the vendors firmware update process.</source>
        <translation>중요: 공급업체의 펌웨어 업데이트 프로세스를 사용하기 전에 플레이어를 원래 펌웨어로 부팅해야 합니다.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="576"/>
        <source>Error installing Rockbox Utility</source>
        <translation>록박스 유틸리티 설치 오류</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="580"/>
        <source>Installing user configuration</source>
        <translation>사용자 구성 설치</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="584"/>
        <source>Error installing user configuration</source>
        <translation>사용자 구성 설치 오류</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="588"/>
        <source>Successfully installed Rockbox Utility.</source>
        <translation>록박스 유틸리티를 성공적으로 설치했습니다.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="391"/>
        <location filename="../rbutilqt.cpp" line="622"/>
        <source>Configuration error</source>
        <translation>구성 오류</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="623"/>
        <source>Your configuration is invalid. Please go to the configuration dialog and make sure the selected values are correct.</source>
        <translation>구성이 잘못되었습니다. 구성 대화 상자로 이동하여 선택한 값이 올바른지 확인하세요.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="384"/>
        <source>This is a new installation of Rockbox Utility, or a new version. The configuration dialog will now open to allow you to setup the program,  or review your settings.</source>
        <translation>록박스 유틸리티를 새로 설치하거나 새 버전을 설치하는 것입니다. 이제 프로그램을 설정하거나 설정을 검토할 수 있는 구성 대화 상자가 열립니다.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="104"/>
        <source>Wine detected!</source>
        <translation>Wine이 감지되었습니다!</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="105"/>
        <source>It seems you are trying to run this program under Wine. Please don&apos;t do this, running under Wine will fail. Use the native Linux binary instead.</source>
        <translation>이 프로그램을 Wine에서 실행하려는 것 같습니다. 이렇게 하지 마십시오. Wine에서 실행하면 실패합니다. 대신 기본 리눅스 바이너리를 사용하세요.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="263"/>
        <source>Can&apos;t get version information.
Network error: %1. Please check your network and proxy settings.</source>
        <translation>버전 정보를 가져올 수 없습니다.
네트워크 오류: %1입니다. 네트워크 및 프록시 설정을 확인하세요.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="383"/>
        <source>New installation</source>
        <translation>새로운 설치</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="392"/>
        <source>Your configuration is invalid. This is most likely due to a changed device path. The configuration dialog will now open to allow you to correct the problem.</source>
        <translation>구성이 잘못되었습니다. 이는 장치 경로가 변경되었기 때문일 가능성이 높습니다. 이제 문제를 해결할 수 있는 구성 대화 상자가 열립니다.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="262"/>
        <source>Network error</source>
        <translation>네트워크 오류</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="211"/>
        <source>Downloading build information, please wait ...</source>
        <translation>빌드 정보를 다운로드하는 중이므로, 잠시 기다려 주세요...</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="261"/>
        <source>Can&apos;t get version information!</source>
        <translation>버전 정보를 얻을 수 없습니다!</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="275"/>
        <source>Download build information finished.</source>
        <translation>빌드 정보 다운로드가 완료되었습니다.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="304"/>
        <source>Libraries used</source>
        <translation>사용된 라이브러리</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="644"/>
        <source>Checking for update ...</source>
        <translation>업데이트 확인 중...</translation>
    </message>
</context>
<context>
    <name>RbUtilQtFrm</name>
    <message>
        <location filename="../rbutilqtfrm.ui" line="14"/>
        <source>Rockbox Utility</source>
        <translation>록박스 유틸리티</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="163"/>
        <location filename="../rbutilqtfrm.ui" line="620"/>
        <source>&amp;Installation</source>
        <translation>설치(&amp;I)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="420"/>
        <source>&amp;Uninstallation</source>
        <translation>설치 제거(&amp;U)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="387"/>
        <source>&amp;File</source>
        <translation>파일(&amp;F)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="459"/>
        <source>&amp;About</source>
        <translation>정보(&amp;A)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="436"/>
        <source>Empty local download cache</source>
        <translation>로컬 다운로드 캐시 비어 있음</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="149"/>
        <source>mountpoint unknown or invalid</source>
        <translation>마운트 지점을 알 수 없거나 잘못됨</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="142"/>
        <source>Mountpoint:</source>
        <translation>마운트 지점:</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="100"/>
        <source>device unknown or invalid</source>
        <translation>알 수 없는 장치이거나 유효하지 않은 장치</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="93"/>
        <source>Device:</source>
        <translation>장치:</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="441"/>
        <source>Install Rockbox Utility on player</source>
        <translation>플레이어에 록박스 유틸리티 설치</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="446"/>
        <source>&amp;Configure</source>
        <translation>구성(&amp;C)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="451"/>
        <source>E&amp;xit</source>
        <translation>나가기(&amp;E)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="454"/>
        <source>Ctrl+Q</source>
        <translation>Ctrl+Q</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="464"/>
        <source>About &amp;Qt</source>
        <translation>Qt 정보(&amp;Q)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="268"/>
        <source>Uninstall Bootloader</source>
        <translation>부트로더 설치 제거</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="262"/>
        <location filename="../rbutilqtfrm.ui" line="295"/>
        <source>Uninstall Rockbox</source>
        <translation>록박스 설치 제거</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="59"/>
        <source>Device</source>
        <translation>장치</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="120"/>
        <source>&amp;Change</source>
        <translation>변경(&amp;C)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="166"/>
        <source>Welcome</source>
        <translation>환영합니다</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="171"/>
        <location filename="../rbutilqtfrm.ui" line="413"/>
        <source>&amp;Accessibility</source>
        <translation>편의성(&amp;A)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="174"/>
        <source>Install accessibility add-ons</source>
        <translation>접근성 애드온 설치</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="180"/>
        <source>Install Talk files</source>
        <translation>토크 파일 설치</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="369"/>
        <source>Inf&amp;o</source>
        <translation>정보(&amp;O)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="396"/>
        <location filename="../rbutilqtfrm.ui" line="469"/>
        <source>&amp;Help</source>
        <translation>도움말(&amp;H)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="409"/>
        <source>Action&amp;s</source>
        <translation>작업(&amp;S)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="474"/>
        <source>Info</source>
        <translation>정보</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="578"/>
        <source>Read PDF manual</source>
        <translation>PDF 설명서 읽기</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="583"/>
        <source>Read HTML manual</source>
        <translation>HTML 설명서 읽기</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="588"/>
        <source>Download PDF manual</source>
        <translation>PDF 설명서 다운로드</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="593"/>
        <source>Download HTML manual (zip)</source>
        <translation>HTMP 설명서 (ZIP) 다운로드</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="230"/>
        <source>Create Voice files</source>
        <translation>음성 파일 생성</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="605"/>
        <source>Create Voice File</source>
        <translation>음성 파일 생성</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="131"/>
        <source>&amp;Eject</source>
        <translation>꺼내기(&amp;E)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="43"/>
        <source>Welcome to Rockbox Utility, the installation and housekeeping tool for Rockbox.</source>
        <translation>록박스의 설치 및 정리 도구인 록박스 유틸리티에 오신 것을 환영합니다.</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="46"/>
        <source>Rockbox Logo</source>
        <translation>록박스 로고</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="197"/>
        <source>&lt;b&gt;Create Talk Files&lt;/b&gt;&lt;br/&gt;Talkfiles are needed to let Rockbox speak File and Foldernames</source>
        <translation>&lt;b&gt;Talk 파일 생성&lt;/b&gt;&lt;br/&gt;토크 파일은 록박스가 파일 및 폴더 이름을 말할 수 있도록 하는 데 필요합니다.</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="247"/>
        <source>&lt;b&gt;Create Voice file&lt;/b&gt;&lt;br/&gt;Voice files are needed to make Rockbox speak the  user interface. Speaking is enabled by default, so
 if you installed the voice file Rockbox will speak.</source>
        <translation>&lt;b&gt;음성 파일 생성&lt;/b&gt;&lt;br&gt; 음성 파일은 록박스가 사용자 인터페이스를 말하게 하는 데 필요합니다. 말하기는 기본적으로 활성화되어 있으므로 음성 파일을 설치하면 록박스가 말합니다.</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="259"/>
        <source>Backup &amp;&amp; &amp;Uninstallation</source>
        <translation>백업 &amp;&amp; 설치 제거(&amp;U)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="285"/>
        <source>&lt;b&gt;Remove the bootloader&lt;/b&gt;&lt;br/&gt;After removing the bootloader you won&apos;t be able to start Rockbox.</source>
        <translation>&lt;b&gt;부트로더 제거&lt;/b&gt;&lt;br/&gt;부트로더를 제거한 후에는 록박스를 시작할 수 없습니다.</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="312"/>
        <source>&lt;b&gt;Uninstall Rockbox from your audio player.&lt;/b&gt;&lt;br/&gt;This will leave the bootloader in place (you need to remove it manually).</source>
        <translation>&lt;b&gt;오디오 플레이어에서 록박스를 제거합니다.&lt;/b&gt;&lt;br/&gt;그러면 부트로더가 그대로 유지됩니다. (수동으로 제거해야 함)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="325"/>
        <source>Backup</source>
        <translation>백업</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="342"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;&lt;span style=&quot; font-weight:600;&quot;&gt;Backup current installation.&lt;/span&gt;&lt;/p&gt;&lt;p&gt;Create a backup by archiving the contents of the Rockbox installation folder.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;&lt;span style=&quot; font-weight:600;&quot;&gt;현재 설치를 백업합니다.&lt;/span&gt;&lt;/p&gt;&lt;p&gt;록박스 설치 폴더의 내용을 보관하여 백업을 만듭니다.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="501"/>
        <source>Install &amp;Bootloader</source>
        <translation>부트로더 설치(&amp;B)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="510"/>
        <source>Install &amp;Rockbox</source>
        <translation>록박스 설치(&amp;R)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="519"/>
        <source>Install &amp;Fonts Package</source>
        <translation>글꼴 패키지 설치(&amp;F)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="528"/>
        <source>Install &amp;Themes</source>
        <translation>테마 설치(&amp;T)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="537"/>
        <source>Install &amp;Game Files</source>
        <translation>게임 파일 설치(&amp;G)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="546"/>
        <source>&amp;Install Voice File</source>
        <translation>음성 파일 설치(&amp;I)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="555"/>
        <source>Create &amp;Talk Files</source>
        <translation>토크 파일 생성(&amp;T)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="564"/>
        <source>Remove &amp;bootloader</source>
        <translation>부트로더 제거(&amp;R)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="573"/>
        <source>Uninstall &amp;Rockbox</source>
        <translation>록박스 설치 제거(&amp;R)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="602"/>
        <source>Create &amp;Voice File</source>
        <translation>음성 파일 생성(&amp;V)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="610"/>
        <source>&amp;System Info</source>
        <translation>시스템 정보(&amp;S)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="625"/>
        <source>Show &amp;Changelog</source>
        <translation>변경 이력 표시(&amp;C)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="483"/>
        <source>&amp;Complete Installation</source>
        <translation>전체 설치(&amp;C)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="492"/>
        <source>&amp;Minimal Installation</source>
        <translation>최소 설치(&amp;M)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="615"/>
        <source>System &amp;Trace</source>
        <translation>시스템 추적(&amp;T)</translation>
    </message>
</context>
<context>
    <name>SelectiveInstallWidget</name>
    <message>
        <location filename="../gui/selectiveinstallwidgetfrm.ui" line="20"/>
        <source>Selective Installation</source>
        <translation>선택적 설치</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidgetfrm.ui" line="26"/>
        <source>Rockbox version to install</source>
        <translation>설치할 록박스 버전</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidgetfrm.ui" line="35"/>
        <source>Version information not available yet.</source>
        <translation>버전 정보가 아직 제공되지 않습니다.</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidgetfrm.ui" line="54"/>
        <source>Rockbox components to install</source>
        <translation>설치할 록박스 구성 요소</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidgetfrm.ui" line="60"/>
        <source>&amp;Bootloader</source>
        <translation>부트로더(&amp;B)</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidgetfrm.ui" line="151"/>
        <source>Some plugins require additional data files.</source>
        <translation>일부 플러그인에는 추가 데이터 파일이 필요합니다.</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidgetfrm.ui" line="188"/>
        <source>Install prerendered voice file.</source>
        <translation>사전 렌더링된 음성 파일을 설치합니다.</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidgetfrm.ui" line="195"/>
        <source>Plugin Data</source>
        <translation>플러그인 데이터</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidgetfrm.ui" line="222"/>
        <source>&amp;Manual</source>
        <translation>설명서(&amp;M)</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidgetfrm.ui" line="233"/>
        <source>&amp;Voice File</source>
        <translation>음성 파일(&amp;V)</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidgetfrm.ui" line="253"/>
        <source>The main Rockbox firmware.</source>
        <translation>록박스의 주요 펌웨어입니다.</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidgetfrm.ui" line="115"/>
        <source>Fonts</source>
        <translation>글꼴</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidgetfrm.ui" line="74"/>
        <source>&amp;Rockbox</source>
        <translation>록박스(&amp;R)</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidgetfrm.ui" line="178"/>
        <source>Additional fonts for the User Interface.</source>
        <translation>사용자 인터페이스용 추가 글꼴입니다.</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidgetfrm.ui" line="135"/>
        <source>The bootloader is required for starting Rockbox. Only necessary for first time install.</source>
        <translation>부트로더는 록박스를 시작하는 데 필요합니다. 처음 설치할 때만 필요합니다.</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidgetfrm.ui" line="161"/>
        <source>Customize</source>
        <translation>커스텀</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidgetfrm.ui" line="104"/>
        <source>Themes</source>
        <translation>테마</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidgetfrm.ui" line="94"/>
        <source>Themes allow adjusting the user interface of Rockbox. Use &quot;Customize&quot; to select themes.</source>
        <translation>테마를 사용하면 록박스의 사용자 인터페이스를 조정할 수 있습니다. 테마를 선택하려면 &quot;커스텀&quot;을 사용하세요.</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidgetfrm.ui" line="263"/>
        <source>Save a copy of the manual on the player.</source>
        <translation>플레이어에 설명서 사본을 저장하세요.</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidgetfrm.ui" line="292"/>
        <source>&amp;Install</source>
        <translation>설치(&amp;I)</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="78"/>
        <source>This is the latest stable release available.</source>
        <translation>이것은 사용 가능한 최신 안정된 릴리스입니다.</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="88"/>
        <source>This will eventually become the next Rockbox version. Install it to help testing.</source>
        <translation>이것은 결국 다음 록박스 버전이 될 것입니다. 테스트에 도움이 되도록 설치하세요.</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="127"/>
        <source>Stable Release (Version %1)</source>
        <translation>안정적 릴리스 (버전 %1)</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="131"/>
        <source>Development Version (Revison %1)</source>
        <translation>개발 버전 (리비전 %1)</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="129"/>
        <source>Release Candidate (Revison %1)</source>
        <translation>릴리스 후보 (리비전 %1)</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="83"/>
        <source>The development version is updated on every code change.</source>
        <translation>개발 버전은 코드가 변경될 때마다 업데이트됩니다.</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="93"/>
        <source>Daily updated development version.</source>
        <translation>매일 업데이트되는 개발 버전입니다.</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="100"/>
        <source>Not available for the selected version</source>
        <translation>선택한 버전에서는 사용할 수 없음</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="130"/>
        <source>Daily Build (%1)</source>
        <translation>일일 빌드 (%1)</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="158"/>
        <source>The selected player doesn&apos;t need a bootloader.</source>
        <translation>선택한 플레이어에는 부트로더가 필요하지 않습니다.</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="163"/>
        <source>The bootloader is required for starting Rockbox. Installation of the bootloader is only necessary on first time installation.</source>
        <translation>부트로더는 록박스를 시작하는 데 필요합니다. 부트로더 설치는 처음 설치할 때만 필요합니다.</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="236"/>
        <source>Mountpoint is wrong</source>
        <translation>마운트 지점이 잘못됨</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="294"/>
        <source>No install method known.</source>
        <translation>알려진 설치 방법이 없습니다.</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="319"/>
        <source>Bootloader detected</source>
        <translation>부트로더가 감지함</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="320"/>
        <source>Bootloader already installed. Do you want to reinstall the bootloader?</source>
        <translation>부트로더가 이미 설치되어 있습니다. 부트로더를 다시 설치할까요?</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="324"/>
        <source>Bootloader installation skipped</source>
        <translation>부트로더 설치 건너뜀</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="338"/>
        <source>Create Bootloader backup</source>
        <translation>부트로더 백업 생성</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="339"/>
        <source>You can create a backup of the original bootloader file. Press &quot;Yes&quot; to select an output folder on your computer to save the file to. The file will get placed in a new folder &quot;%1&quot; created below the selected folder.
Press &quot;No&quot; to skip this step.</source>
        <translation>원래 부트로더 파일의 백업을 만들 수 있습니다. &quot;예&quot;를 눌러 파일을 저장할 컴퓨터의 출력 폴더를 선택합니다. 선택한 폴더 아래에 생성된 새 폴더 &quot;%1&quot;에 파일이 저장됩니다.
이 단계를 건너뛰려면 &quot;아니오&quot;를 누르세요.</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="346"/>
        <source>Browse backup folder</source>
        <translation>백업 폴더 찾아보기</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="358"/>
        <source>Prerequisites</source>
        <translation>필수 조건</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="363"/>
        <source>Bootloader installation aborted</source>
        <translation>부트로더 설치가 중단됨</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="373"/>
        <source>Bootloader files (%1)</source>
        <translation>부트로더 파일 (%1)</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="375"/>
        <source>All files (*)</source>
        <translation>모든 파일 (*)</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="377"/>
        <source>Select firmware file</source>
        <translation>펌웨어 파일 선택</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="379"/>
        <source>Error opening firmware file</source>
        <translation>펌웨어 파일 열기 오류</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="385"/>
        <source>Error reading firmware file</source>
        <translation>펌웨어 파일 읽기 오류</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="395"/>
        <source>Backup error</source>
        <translation>백업 오류</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="396"/>
        <source>Could not create backup file. Continue?</source>
        <translation>백업 파일을 생성할 수 없습니다. 계속할까요?</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="420"/>
        <source>Manual steps required</source>
        <translation>수동 단계 필요</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="641"/>
        <source>Your installation doesn&apos;t require any plugin data files, skipping.</source>
        <translation>설치에는 플러그인 데이터 파일이 필요하지 않으므로, 건너뜁니다.</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="224"/>
        <source>Continue with installation?</source>
        <translation>설치를 계속할까요?</translation>
    </message>
    <message>
        <location filename="../gui/selectiveinstallwidget.cpp" line="225"/>
        <source>Really continue?</source>
        <translation>정말 계속할까요?</translation>
    </message>
</context>
<context>
    <name>SysTrace</name>
    <message>
        <location filename="../systrace.cpp" line="100"/>
        <location filename="../systrace.cpp" line="109"/>
        <source>Save system trace log</source>
        <translation>시스템 추적 로그 저장</translation>
    </message>
</context>
<context>
    <name>SysTraceFrm</name>
    <message>
        <location filename="../systracefrm.ui" line="14"/>
        <source>System Trace</source>
        <translation>시스템 추적</translation>
    </message>
    <message>
        <location filename="../systracefrm.ui" line="20"/>
        <source>System State trace</source>
        <translation>시스템 상태 추적</translation>
    </message>
    <message>
        <location filename="../systracefrm.ui" line="46"/>
        <source>&amp;Close</source>
        <translation>닫기(&amp;C)</translation>
    </message>
    <message>
        <location filename="../systracefrm.ui" line="57"/>
        <source>&amp;Save</source>
        <translation>저장(&amp;S)</translation>
    </message>
    <message>
        <location filename="../systracefrm.ui" line="68"/>
        <source>&amp;Refresh</source>
        <translation>새로고침(&amp;R)</translation>
    </message>
    <message>
        <location filename="../systracefrm.ui" line="79"/>
        <source>Save &amp;previous</source>
        <translation>이전 저장(&amp;P)</translation>
    </message>
</context>
<context>
    <name>Sysinfo</name>
    <message>
        <location filename="../sysinfo.cpp" line="46"/>
        <source>&lt;b&gt;OS&lt;/b&gt;&lt;br/&gt;</source>
        <translation>&lt;b&gt;운영체제&lt;/b&gt;&lt;br/&gt;</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="47"/>
        <source>&lt;b&gt;Username&lt;/b&gt;&lt;br/&gt;%1&lt;hr/&gt;</source>
        <translation>&lt;b&gt;사용자이름&lt;/b&gt;&lt;br/&gt;%1&lt;hr/&gt;</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="49"/>
        <source>&lt;b&gt;Permissions&lt;/b&gt;&lt;br/&gt;%1&lt;hr/&gt;</source>
        <translation>&lt;b&gt;권한&lt;/b&gt;&lt;br/&gt;%1&lt;hr/&gt;</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="51"/>
        <source>&lt;b&gt;Attached USB devices&lt;/b&gt;&lt;br/&gt;</source>
        <translation>&lt;b&gt;연결된 USB 장치&lt;/b&gt;&lt;br/&gt;</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="55"/>
        <source>VID: %1 PID: %2, %3</source>
        <translation>VID: %1 PID: %2, %3</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="64"/>
        <source>Filesystem</source>
        <translation>파일시스템</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="67"/>
        <source>Mountpoint</source>
        <translation>마운트 지점</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="67"/>
        <source>Label</source>
        <translation>레이블</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="68"/>
        <source>Free</source>
        <translation>여유</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="68"/>
        <source>Total</source>
        <translation>전체</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="69"/>
        <source>Type</source>
        <translation>유형</translation>
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
        <translation>시스템 정보</translation>
    </message>
    <message>
        <location filename="../sysinfofrm.ui" line="22"/>
        <source>&amp;Refresh</source>
        <translation>새로고침(&amp;S)</translation>
    </message>
    <message>
        <location filename="../sysinfofrm.ui" line="46"/>
        <source>&amp;OK</source>
        <translation>확인(&amp;O)</translation>
    </message>
</context>
<context>
    <name>System</name>
    <message>
        <location filename="../base/system.cpp" line="117"/>
        <source>Guest</source>
        <translation>게스트</translation>
    </message>
    <message>
        <location filename="../base/system.cpp" line="120"/>
        <source>Admin</source>
        <translation>관리자</translation>
    </message>
    <message>
        <location filename="../base/system.cpp" line="123"/>
        <source>User</source>
        <translation>사용자</translation>
    </message>
    <message>
        <location filename="../base/system.cpp" line="126"/>
        <source>Error</source>
        <translation>오류</translation>
    </message>
    <message>
        <location filename="../base/system.cpp" line="273"/>
        <source>(no description available)</source>
        <translation>(가능한 설명이 없음)</translation>
    </message>
</context>
<context>
    <name>TTSBase</name>
    <message>
        <location filename="../base/ttsbase.cpp" line="47"/>
        <source>Espeak TTS Engine</source>
        <translation>Espeak TTS 엔진</translation>
    </message>
    <message>
        <location filename="../base/ttsbase.cpp" line="48"/>
        <source>Espeak-ng TTS Engine</source>
        <translation>Espeak-ng TTS 엔진</translation>
    </message>
    <message>
        <location filename="../base/ttsbase.cpp" line="49"/>
        <source>Mimic TTS Engine</source>
        <translation>Mimic TTS 엔진</translation>
    </message>
    <message>
        <location filename="../base/ttsbase.cpp" line="51"/>
        <source>Flite TTS Engine</source>
        <translation>Flite TTS 엔진</translation>
    </message>
    <message>
        <location filename="../base/ttsbase.cpp" line="52"/>
        <source>Swift TTS Engine</source>
        <translation>Swift TTS 엔진</translation>
    </message>
    <message>
        <location filename="../base/ttsbase.cpp" line="55"/>
        <source>SAPI4 TTS Engine</source>
        <translation>SAPI4 TTS 엔진</translation>
    </message>
    <message>
        <location filename="../base/ttsbase.cpp" line="57"/>
        <source>SAPI5 TTS Engine</source>
        <translation>SAPI5 TTS 엔진</translation>
    </message>
    <message>
        <location filename="../base/ttsbase.cpp" line="58"/>
        <source>MS Speech Platform</source>
        <translation>MS 음성 플랫폼</translation>
    </message>
    <message>
        <location filename="../base/ttsbase.cpp" line="61"/>
        <source>Festival TTS Engine</source>
        <translation>Festival TTS 엔진</translation>
    </message>
    <message>
        <location filename="../base/ttsbase.cpp" line="64"/>
        <source>OS X System Engine</source>
        <translation>Mac OS X TTS 엔진</translation>
    </message>
</context>
<context>
    <name>TTSCarbon</name>
    <message>
        <location filename="../base/ttscarbon.cpp" line="139"/>
        <source>Voice:</source>
        <translation>음성:</translation>
    </message>
    <message>
        <location filename="../base/ttscarbon.cpp" line="145"/>
        <source>Speed (words/min):</source>
        <translation>속도 (단어/분):</translation>
    </message>
    <message>
        <location filename="../base/ttscarbon.cpp" line="152"/>
        <source>Pitch (0 for default):</source>
        <translation>피치 (기본값은 0):</translation>
    </message>
    <message>
        <location filename="../base/ttscarbon.cpp" line="222"/>
        <source>Could not voice string</source>
        <translation>문자열을 음성으로 출력할 수 없음</translation>
    </message>
    <message>
        <location filename="../base/ttscarbon.cpp" line="232"/>
        <source>Could not convert intermediate file</source>
        <translation>중간 파일을 변환할 수 없음</translation>
    </message>
</context>
<context>
    <name>TTSExes</name>
    <message>
        <location filename="../base/ttsexes.cpp" line="78"/>
        <source>TTS executable not found</source>
        <translation>TTS 실행 파일을 찾을 수 없음</translation>
    </message>
    <message>
        <location filename="../base/ttsexes.cpp" line="44"/>
        <source>Path to TTS engine:</source>
        <translation>TTS 엔진 경로:</translation>
    </message>
    <message>
        <location filename="../base/ttsexes.cpp" line="46"/>
        <source>TTS engine options:</source>
        <translation>TTS 엔진 옵션:</translation>
    </message>
</context>
<context>
    <name>TTSFestival</name>
    <message>
        <location filename="../base/ttsfestival.cpp" line="207"/>
        <source>engine could not voice string</source>
        <translation>엔진이 문자열을 음성으로 출력할 수 없음</translation>
    </message>
    <message>
        <location filename="../base/ttsfestival.cpp" line="290"/>
        <source>No description available</source>
        <translation>가능한 설명이 없음</translation>
    </message>
    <message>
        <location filename="../base/ttsfestival.cpp" line="53"/>
        <source>Path to Festival client:</source>
        <translation>페스티벌 클라이언트 경로:</translation>
    </message>
    <message>
        <location filename="../base/ttsfestival.cpp" line="58"/>
        <source>Voice:</source>
        <translation>음성:</translation>
    </message>
    <message>
        <location filename="../base/ttsfestival.cpp" line="69"/>
        <source>Voice description:</source>
        <translation>음성 설명:</translation>
    </message>
    <message>
        <location filename="../base/ttsfestival.cpp" line="173"/>
        <source>Festival could not be started</source>
        <translation>Festival을 시작할 수 없음</translation>
    </message>
</context>
<context>
    <name>TTSSapi</name>
    <message>
        <location filename="../base/ttssapi.cpp" line="46"/>
        <source>Language:</source>
        <translation>언어:</translation>
    </message>
    <message>
        <location filename="../base/ttssapi.cpp" line="53"/>
        <source>Voice:</source>
        <translation>음성:</translation>
    </message>
    <message>
        <location filename="../base/ttssapi.cpp" line="65"/>
        <source>Speed:</source>
        <translation>속도:</translation>
    </message>
    <message>
        <location filename="../base/ttssapi.cpp" line="68"/>
        <source>Options:</source>
        <translation>옵션:</translation>
    </message>
    <message>
        <location filename="../base/ttssapi.cpp" line="112"/>
        <source>Could not copy the SAPI script</source>
        <translation>SAPI 스크립트를 복사할 수 없음</translation>
    </message>
    <message>
        <location filename="../base/ttssapi.cpp" line="130"/>
        <source>Could not start SAPI process</source>
        <translation>SAPI 프로세스를 시작할 수 없음</translation>
    </message>
</context>
<context>
    <name>TalkFileCreator</name>
    <message>
        <location filename="../base/talkfile.cpp" line="45"/>
        <source>Talk file creation aborted</source>
        <translation>Talk 파일 생성 중단됨</translation>
    </message>
    <message>
        <location filename="../base/talkfile.cpp" line="78"/>
        <source>Finished creating Talk files</source>
        <translation>토크 파일 생성 완료됨</translation>
    </message>
    <message>
        <location filename="../base/talkfile.cpp" line="42"/>
        <source>Reading Filelist...</source>
        <translation>파일목록 읽는 중...</translation>
    </message>
    <message>
        <location filename="../base/talkfile.cpp" line="261"/>
        <source>Copying of %1 to %2 failed</source>
        <translation>%1을(를) %2(으)로 복사 실패함</translation>
    </message>
    <message>
        <location filename="../base/talkfile.cpp" line="66"/>
        <source>Copying Talkfiles...</source>
        <translation>토크 파일 복사 중...</translation>
    </message>
    <message>
        <location filename="../base/talkfile.cpp" line="36"/>
        <source>Starting Talk file generation for folder %1</source>
        <translation>%1 폴더에 대한 Talk 파일 생성 시작</translation>
    </message>
    <message>
        <location filename="../base/talkfile.cpp" line="242"/>
        <source>File copy aborted</source>
        <translation>파일 복사 중단됨</translation>
    </message>
    <message>
        <location filename="../base/talkfile.cpp" line="283"/>
        <source>Cleaning up...</source>
        <translation>지우기 중...</translation>
    </message>
    <message>
        <location filename="../base/talkfile.cpp" line="294"/>
        <source>Finished</source>
        <translation>완료함</translation>
    </message>
</context>
<context>
    <name>TalkGenerator</name>
    <message>
        <location filename="../base/talkgenerator.cpp" line="38"/>
        <source>Starting TTS Engine</source>
        <translation>TTS 시스템 시작</translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="43"/>
        <location filename="../base/talkgenerator.cpp" line="50"/>
        <source>Init of TTS engine failed</source>
        <translation>TTS 엔진 초기화 실패</translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="57"/>
        <source>Starting Encoder Engine</source>
        <translation>인코더 엔진 시작</translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="62"/>
        <source>Init of Encoder engine failed</source>
        <translation>인코더 엔진 초기화 실패</translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="72"/>
        <source>Voicing entries...</source>
        <translation>항목 음성 처리 중...</translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="87"/>
        <source>Encoding files...</source>
        <translation>파일 인코딩 중...</translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="126"/>
        <source>Voicing aborted</source>
        <translation>음성 중단됨</translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="163"/>
        <location filename="../base/talkgenerator.cpp" line="168"/>
        <source>Voicing of %1 failed: %2</source>
        <translation>%1 음성 녹음 실패: %2</translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="212"/>
        <source>Encoding aborted</source>
        <translation>인코딩 중단됨</translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="240"/>
        <source>Encoding of %1 failed</source>
        <translation>%1 인코딩 실패함</translation>
    </message>
</context>
<context>
    <name>ThemeInstallFrm</name>
    <message>
        <location filename="../themesinstallfrm.ui" line="13"/>
        <source>Theme Installation</source>
        <translation>테마 설치</translation>
    </message>
    <message>
        <location filename="../themesinstallfrm.ui" line="48"/>
        <source>Selected Theme</source>
        <translation>선택된 테마</translation>
    </message>
    <message>
        <location filename="../themesinstallfrm.ui" line="73"/>
        <source>Description</source>
        <translation>설명</translation>
    </message>
    <message>
        <location filename="../themesinstallfrm.ui" line="83"/>
        <source>Download size:</source>
        <translation>다운로드 크기:</translation>
    </message>
    <message>
        <location filename="../themesinstallfrm.ui" line="126"/>
        <source>&amp;Cancel</source>
        <translation>취소(&amp;C)</translation>
    </message>
    <message>
        <location filename="../themesinstallfrm.ui" line="115"/>
        <source>&amp;Install</source>
        <translation>설치(&amp;I)</translation>
    </message>
    <message>
        <location filename="../themesinstallfrm.ui" line="93"/>
        <source>Hold Ctrl to select multiple item, Shift for a range</source>
        <translation>여러 항목을 선택하려면 Ctrl을 누르고, 범위를 선택하려면 Shift를 누르세요</translation>
    </message>
</context>
<context>
    <name>ThemesInstallWindow</name>
    <message>
        <location filename="../themesinstallwindow.cpp" line="41"/>
        <source>no theme selected</source>
        <translation>선택한 테마 없음</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="122"/>
        <source>Network error: %1.
Please check your network and proxy settings.</source>
        <translation>네트워크 오류: %1입니다.
네트워크 및 프록시 설정을 확인하세요.</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="135"/>
        <source>the following error occured:
%1</source>
        <translation>다음 오류가 발생했습니다:
%1</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="141"/>
        <source>done.</source>
        <translation>완료하였습니다.</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="214"/>
        <source>fetching details for %1</source>
        <translation>%1 세부 정보를 가져오는 중</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="217"/>
        <source>fetching preview ...</source>
        <translation>미리보기를 가져오는 중...</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="230"/>
        <source>&lt;b&gt;Author:&lt;/b&gt; %1&lt;hr/&gt;</source>
        <translation>&lt;b&gt;제작자:&lt;/b&gt; %1&lt;시간/&gt;</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="231"/>
        <location filename="../themesinstallwindow.cpp" line="233"/>
        <source>unknown</source>
        <translation>알 수 없음</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="232"/>
        <source>&lt;b&gt;Version:&lt;/b&gt; %1&lt;hr/&gt;</source>
        <translation>&lt;b&gt;버전:&lt;/b&gt; %1&lt;시간/&gt;</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="235"/>
        <source>no description</source>
        <translation>설명 없음</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="262"/>
        <source>no theme preview</source>
        <translation>테마 미리보기 없음</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="290"/>
        <source>Select</source>
        <translation>선택</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="297"/>
        <source>getting themes information ...</source>
        <translation>테마 정보를 가져오는 중...</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="325"/>
        <source>No themes selected, skipping</source>
        <translation>선택된 테마가 없으므로, 건너뜀</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="354"/>
        <source>Mount point is wrong!</source>
        <translation>마운트 지점이 잘못되었습니다!</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="234"/>
        <source>&lt;b&gt;Description:&lt;/b&gt; %1&lt;hr/&gt;</source>
        <translation>&lt;b&gt;설명:&lt;/b&gt; %1&lt;시간/&gt;</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="42"/>
        <source>no selection</source>
        <translation>선택 없음</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="178"/>
        <source>Information</source>
        <translation>정보</translation>
    </message>
    <message numerus="yes">
        <location filename="../themesinstallwindow.cpp" line="198"/>
        <source>Download size %L1 kiB (%n item(s))</source>
	<translation>
            <numerusform>다운로드 크기 %L1 kiB (%n개 항목)</numerusform>
	</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="251"/>
        <source>Retrieving theme preview failed.
HTTP response code: %1</source>
        <translation>테마 미리보기를 가져오지 못했습니다.
HTTP 응답 코드: %1</translation>
    </message>
</context>
<context>
    <name>UninstallFrm</name>
    <message>
        <location filename="../uninstallfrm.ui" line="16"/>
        <source>Uninstall Rockbox</source>
        <translation>록박스 설치 제거</translation>
    </message>
    <message>
        <location filename="../uninstallfrm.ui" line="35"/>
        <source>Please select the Uninstallation Method</source>
        <translation>설치 제거 방법을 선택하세요</translation>
    </message>
    <message>
        <location filename="../uninstallfrm.ui" line="45"/>
        <source>Uninstallation Method</source>
        <translation>설치 제거 방법</translation>
    </message>
    <message>
        <location filename="../uninstallfrm.ui" line="51"/>
        <source>Complete Uninstallation</source>
        <translation>완전한 설치 제거</translation>
    </message>
    <message>
        <location filename="../uninstallfrm.ui" line="58"/>
        <source>Smart Uninstallation</source>
        <translation>스마트 설치 제거</translation>
    </message>
    <message>
        <location filename="../uninstallfrm.ui" line="68"/>
        <source>Please select what you want to uninstall</source>
        <translation>설치 제거할 항목을 선택하세요</translation>
    </message>
    <message>
        <location filename="../uninstallfrm.ui" line="78"/>
        <source>Installed Parts</source>
        <translation>설치된 항목</translation>
    </message>
    <message>
        <location filename="../uninstallfrm.ui" line="139"/>
        <source>&amp;Cancel</source>
        <translation>취소(&amp;C)</translation>
    </message>
    <message>
        <location filename="../uninstallfrm.ui" line="128"/>
        <source>&amp;Uninstall</source>
        <translation>설치 제거(&amp;U)</translation>
    </message>
</context>
<context>
    <name>Uninstaller</name>
    <message>
        <location filename="../base/uninstall.cpp" line="32"/>
        <location filename="../base/uninstall.cpp" line="43"/>
        <source>Starting Uninstallation</source>
        <translation>설치 제거 시작</translation>
    </message>
    <message>
        <location filename="../base/uninstall.cpp" line="36"/>
        <source>Finished Uninstallation</source>
        <translation>설치 제거 완료</translation>
    </message>
    <message>
        <location filename="../base/uninstall.cpp" line="49"/>
        <source>Uninstalling %1...</source>
        <translation>%1 설치 제거 중...</translation>
    </message>
    <message>
        <location filename="../base/uninstall.cpp" line="81"/>
        <source>Could not delete %1</source>
        <translation type="unfinished">%1을(를) 삭제할 수 없음</translation>
    </message>
    <message>
        <location filename="../base/uninstall.cpp" line="115"/>
        <source>Uninstallation finished</source>
        <translation>설치 제거 완료</translation>
    </message>
</context>
<context>
    <name>Utils</name>
    <message>
        <location filename="../base/utils.cpp" line="375"/>
        <source>&lt;li&gt;Permissions insufficient for bootloader installation.
Administrator priviledges are necessary.&lt;/li&gt;</source>
        <translation>&lt;li&gt;부트로더 설치에 대한 권한이 부족합니다.
관리자 권한이 필요합니다.&lt;/li&gt;</translation>
    </message>
    <message>
        <location filename="../base/utils.cpp" line="387"/>
        <source>&lt;li&gt;Target mismatch detected.&lt;br/&gt;Installed target: %1&lt;br/&gt;Selected target: %2.&lt;/li&gt;</source>
        <translation>&lt;li&gt;대상 불일치가 감지되었습니다.&lt;br/&gt;설치된 대상: %1&lt;br/&gt;선택한 대상: %2&lt;/li&gt;</translation>
    </message>
    <message>
        <location filename="../base/utils.cpp" line="396"/>
        <source>Problem detected:</source>
        <translation>감지된 문제:</translation>
    </message>
</context>
<context>
    <name>VoiceFileCreator</name>
    <message>
        <location filename="../base/voicefile.cpp" line="43"/>
        <source>Starting Voicefile generation</source>
        <translation>음성 파일 생성 시작</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="90"/>
        <source>Extracted voice strings from installation</source>
        <translation>설치에서 추출된 음성 문자열</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="100"/>
        <source>Extracted voice strings incompatible</source>
        <translation>추출된 음성 문자열이 호환되지 않음</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="145"/>
        <source>Could not retrieve strings from installation, downloading</source>
        <translation>설치, 다운로드에서 문자열을 검색할 수 없음</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="185"/>
        <source>Download error: received HTTP error %1.</source>
        <translation>다운로드 오류: HTTP 오류 %1을(를) 받았습니다.</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="192"/>
        <source>Cached file used.</source>
        <translation>캐시된 파일이 사용되었습니다.</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="195"/>
        <source>Download error: %1</source>
        <translation>다운로드 오류: %1</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="200"/>
        <source>Download finished.</source>
        <translation>다운로드가 완료되었습니다</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="213"/>
        <source>failed to open downloaded file</source>
        <translation>다운로드한 파일을 열기 실패함</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="276"/>
        <source>The downloaded file was empty!</source>
        <translation>다운로드한 파일이 비어 있었습니다!</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="307"/>
        <source>Error opening downloaded file</source>
        <translation>다운로드한 파일을 여는 동안 오류 발생</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="318"/>
        <source>Error opening output file</source>
        <translation>출력 파일을 여는 동안 오류 발생</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="338"/>
        <source>successfully created.</source>
        <translation>성공적으로 생성되었습니다.</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="56"/>
        <source>could not find rockbox-info.txt</source>
        <translation>rockbox-info.txt를 찾을 수 없음</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="172"/>
        <source>Downloading voice info...</source>
        <translation>음성 정보 다운로드 중...</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="219"/>
        <source>Reading strings...</source>
        <translation>문자열 읽는 중...</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="302"/>
        <source>Creating voicefiles...</source>
        <translation>음성 파일 생성 중...</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="347"/>
        <source>Cleaning up...</source>
        <translation>지우는 중...</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="358"/>
        <source>Finished</source>
        <translation>완료됨</translation>
    </message>
</context>
<context>
    <name>ZipInstaller</name>
    <message>
        <location filename="../base/zipinstaller.cpp" line="59"/>
        <source>done.</source>
        <translation>완료하였습니다.</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="78"/>
        <source>Downloading file %1.%2</source>
        <translation>file %1.%2 다운로드</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="120"/>
        <source>Download error: %1</source>
        <translation>다운로드 오류: %1</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="125"/>
        <source>Download finished (cache used).</source>
        <translation>다운로드가 완료되었습니다. (캐시가 사용됨)</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="128"/>
        <source>Download finished.</source>
        <translation>다운로드가 완료되었습니다.</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="135"/>
        <source>Extracting file.</source>
        <translation>파일 추출 중입니다.</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="156"/>
        <source>Extraction failed!</source>
        <translation>추출에 실패했습니다!</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="168"/>
        <source>Installing file.</source>
        <translation>파일을 설치합니다.</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="180"/>
        <source>Installing file failed.</source>
        <translation>파일 설치에 실패했습니다.</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="193"/>
        <source>Creating installation log</source>
        <translation>설치 로그 생성</translation>
    </message>
    <message>
        <source>Cached file used.</source>
        <translation>캐시된 파일을 사용했습니다.</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="67"/>
        <source>Package installation finished successfully.</source>
        <translation>패키지 설치가 성공적으로 완료되었습니다.</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="113"/>
        <source>Download error: received HTTP error %1
%2</source>
        <translation>다운로드 오류: HTTP 오류 %1 발생
%2</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="149"/>
        <source>Not enough disk space! Aborting.</source>
        <translation>디스크 공간이 부족합니다! 중단합니다.</translation>
    </message>
</context>
<context>
    <name>ZipUtil</name>
    <message>
        <location filename="../base/ziputil.cpp" line="125"/>
        <source>Creating output path failed</source>
        <translation>출력 경로 생성 실패함</translation>
    </message>
    <message>
        <location filename="../base/ziputil.cpp" line="132"/>
        <source>Creating output file failed</source>
        <translation>출력 파일 생성 실패함</translation>
    </message>
    <message>
        <location filename="../base/ziputil.cpp" line="141"/>
        <source>Error during Zip operation</source>
        <translation>Zip 작업 중 오류 발생</translation>
    </message>
</context>
<context>
    <name>aboutBox</name>
    <message>
        <location filename="../aboutbox.ui" line="14"/>
        <source>About Rockbox Utility</source>
        <translation>록박스 유틸리티 정보</translation>
    </message>
    <message>
        <location filename="../aboutbox.ui" line="74"/>
        <source>&amp;Credits</source>
        <translation>크레딧(&amp;C)</translation>
    </message>
    <message>
        <location filename="../aboutbox.ui" line="106"/>
        <source>&amp;License</source>
        <translation>라이선스(&amp;L)</translation>
    </message>
    <message>
        <location filename="../aboutbox.ui" line="132"/>
        <source>L&amp;ibraries</source>
        <translation>라이브러리(&amp;I)</translation>
    </message>
    <message>
        <location filename="../aboutbox.ui" line="158"/>
        <source>&amp;Ok</source>
        <translation>확인(&amp;O)</translation>
    </message>
    <message>
        <location filename="../aboutbox.ui" line="32"/>
        <source>The Rockbox Utility</source>
        <translation>록박스 유틸리티</translation>
    </message>
    <message>
        <location filename="../aboutbox.ui" line="54"/>
        <source>Installer and housekeeping utility for the Rockbox open source digital audio player firmware.&lt;br/&gt;© The Rockbox Team.&lt;br/&gt;Released under the GNU General Public License v2.&lt;br/&gt;Uses icons by the &lt;a href=&quot;http://tango.freedesktop.org/&quot;&gt;Tango Project&lt;/a&gt;.&lt;br/&gt;&lt;center&gt;&lt;a href=&quot;https://www.rockbox.org&quot;&gt;https://www.rockbox.org&lt;/a&gt;&lt;/center&gt;</source>
        <translation>록박스 오픈 소스 디지털 오디오 플레이어 펌웨어용 설치 프로그램 및 관리 유틸리티입니다.&lt;br/&gt;© 록박스 팀입니다.&lt;br/&gt; GNU 일반 공중 사용 허가서 v2에 따라 출시되었습니다.&lt;br/&gt;&lt;a href=&quot;http://tango.freedesktop.org/&quot;&gt;Tango 프로젝트&lt;/a&gt;의 아이콘을 사용합니다.&lt;br/&gt;&lt;center&gt;&lt;a href=&quot;https://www.rockbox.org&quot;&gt;https://www.rockbox.org&lt;/a&gt;&lt;/center&gt;</translation>
    </message>
</context>
</TS>
