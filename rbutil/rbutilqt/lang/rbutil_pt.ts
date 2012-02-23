<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.0" language="pt">
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
        <translation type="unfinished">Transferindo rotina de arranque</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="97"/>
        <location filename="../base/bootloaderinstallams.cpp" line="110"/>
        <source>Could not load %1</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="124"/>
        <source>No room to insert bootloader, try another firmware version</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="134"/>
        <source>Patching Firmware...</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="145"/>
        <source>Could not open %1 for writing</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="158"/>
        <source>Could not write firmware file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="174"/>
        <source>Success: modified firmware file created</source>
        <translation type="unfinished">Sucesso: ficheiro de firmware modificado criado</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="182"/>
        <source>To uninstall, perform a normal upgrade with an unmodified original firmware</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>BootloaderInstallBase</name>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="124"/>
        <source>Download error: received HTTP error %1.</source>
        <translation>Erro de transferência: recebido erro de HTTP %1.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="130"/>
        <source>Download error: %1</source>
        <translation>Erro de transferência: %1</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="136"/>
        <source>Download finished (cache used).</source>
        <translation>Tranferência terminada (cache usada).</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="138"/>
        <source>Download finished.</source>
        <translation>Tranferência terminada.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="159"/>
        <source>Creating backup of original firmware file.</source>
        <translation>Criando cópia de segurança do ficheiro de firmware original.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="161"/>
        <source>Creating backup folder failed</source>
        <translation>Falha na criação da directoria da cópia de segurança</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="167"/>
        <source>Creating backup copy failed.</source>
        <translation>Falha na criação da cópia de segurança.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="170"/>
        <source>Backup created.</source>
        <translation>Cópia de segurança criada.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="183"/>
        <source>Creating installation log</source>
        <translation>Criando registo da instalação</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="207"/>
        <source>Bootloader installation is almost complete. Installation &lt;b&gt;requires&lt;/b&gt; you to perform the following steps manually:</source>
        <translation>Instalação da rotina de arranque está quase completa. A instalação &lt;b&gt;necessita&lt;/b&gt; que faça os passos seguintes manualmente:</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="213"/>
        <source>&lt;li&gt;Safely remove your player.&lt;/li&gt;</source>
        <translation>&lt;li&gt;Remova com segurança o seu reprodutor.&lt;/li&gt;</translation>
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
        <translation>&lt;li&gt;Desligue o reprodutor&lt;/li&gt;&lt;li&gt;Insira o carregador&lt;/li&gt;</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="246"/>
        <source>&lt;li&gt;Unplug USB and power adaptors&lt;/li&gt;&lt;li&gt;Hold &lt;i&gt;Power&lt;/i&gt; to turn the player off&lt;/li&gt;&lt;li&gt;Toggle the battery switch on the player&lt;/li&gt;&lt;li&gt;Hold &lt;i&gt;Power&lt;/i&gt; to boot into Rockbox&lt;/li&gt;</source>
        <translation>&lt;li&gt;Desconecte o USB  e carregadores&lt;/li&gt;&lt;li&gt;Pressione &lt;i&gt;Power&lt;/i&gt; para desligar o reprodutor&lt;/li&gt;&lt;li&gt;Alterne o interruptor da bateria no reprodutor&lt;/li&gt;&lt;li&gt;Pressione &lt;i&gt;Power&lt;/i&gt; para arrancar com o Rockbox&lt;/li&gt;</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="252"/>
        <source>&lt;p&gt;&lt;b&gt;Note:&lt;/b&gt; You can safely install other parts first, but the above steps are &lt;b&gt;required&lt;/b&gt; to finish the installation!&lt;/p&gt;</source>
        <translation>&lt;p&gt;&lt;b&gt;Nota:&lt;/b&gt; Pode instalar com segurança outras partes primeiro, mas os passos acima são &lt;b&gt;necessários&lt;/b&gt; para terminar a instalação!&lt;/p&gt;</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="266"/>
        <source>Waiting for system to remount player</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="296"/>
        <source>Player remounted</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="301"/>
        <source>Timeout on remount</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="195"/>
        <source>Installation log created</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>BootloaderInstallChinaChip</name>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="33"/>
        <source>Bootloader installation requires you to provide a firmware file of the original firmware (HXF file). You need to download this file yourself due to legal reasons. Please refer to the &lt;a href=&apos;http://www.rockbox.org/manual.shtml&apos;&gt;manual&lt;/a&gt; and the &lt;a href=&apos;http://www.rockbox.org/wiki/OndaVX747#Download_and_extract_a_recent_ve&apos;&gt;OndaVX747&lt;/a&gt; wiki page on how to obtain this file.&lt;br/&gt;Press Ok to continue and browse your computer for the firmware file.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="50"/>
        <source>Downloading bootloader file</source>
        <translation type="unfinished">Transferindo rotina de arranque</translation>
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
        <translation>Transferindo a rotina de arranque</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="42"/>
        <source>Installing Rockbox bootloader</source>
        <translation>Instalando a rotina de arranque do Rockbox</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="74"/>
        <source>Error accessing output folder</source>
        <translation>Erro ao aceder à directoria de saída</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="87"/>
        <source>Bootloader successful installed</source>
        <translation>Rotina de arranque instalada com sucesso</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="97"/>
        <source>Removing Rockbox bootloader</source>
        <translation>Removendo a rotina de arranque do Rockbox</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="101"/>
        <source>No original firmware file found.</source>
        <translation>Ficheiro firmware original não encontrado.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="107"/>
        <source>Can&apos;t remove Rockbox bootloader file.</source>
        <translation>Falha na remoção da rotina de arranque do Rockbox.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="112"/>
        <source>Can&apos;t restore bootloader file.</source>
        <translation>Falha no restauro da rotina de arranque.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="116"/>
        <source>Original bootloader restored successfully.</source>
        <translation>Rotina de arranque original restaurada com sucesso.</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallHex</name>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="67"/>
        <source>checking MD5 hash of input file ...</source>
        <translation>verificando código MD5 do ficheiro de entrada ...</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="78"/>
        <source>Could not verify original firmware file</source>
        <translation>Mão consegiu verficar o ficheiro do firmware original</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="93"/>
        <source>Firmware file not recognized.</source>
        <translation>Ficherio de firmware não reconhecido.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="97"/>
        <source>MD5 hash ok</source>
        <translation>Código MD5 ok</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="104"/>
        <source>Firmware file doesn&apos;t match selected player.</source>
        <translation>Ficheiro de firmware não corrosponde ao reprodutor seleccionado.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="109"/>
        <source>Descrambling file</source>
        <translation>Desbaralhando ficheiro</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="117"/>
        <source>Error in descramble: %1</source>
        <translation>Erro ao desbaralhar: %1</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="122"/>
        <source>Downloading bootloader file</source>
        <translation>Transferindo rotina de arranque</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="132"/>
        <source>Adding bootloader to firmware file</source>
        <translation>Adicionando rotina de arranque ao ficheiro de firmware</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="170"/>
        <source>could not open input file</source>
        <translation>não consegiu abrir ficheiro de entrada</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="171"/>
        <source>reading header failed</source>
        <translation>falha na leitura do cabeçalho</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="172"/>
        <source>reading firmware failed</source>
        <translation>falha na leitura do firmware</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="173"/>
        <source>can&apos;t open bootloader file</source>
        <translation>falha na abertura do ficheiro da rotina de arranque</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="174"/>
        <source>reading bootloader file failed</source>
        <translation>falha na leitura da rotina de arranque</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="175"/>
        <source>can&apos;t open output file</source>
        <translation>não consegue abrir ficheiro de saída</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="176"/>
        <source>writing output file failed</source>
        <translation>falha na escrita do ficheiro de saída</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="178"/>
        <source>Error in patching: %1</source>
        <translation>Erro ao corrigir: %1</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="189"/>
        <source>Error in scramble: %1</source>
        <translation>Erro ao baralhar: %1</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="204"/>
        <source>Checking modified firmware file</source>
        <translation>Verificando ficheiro do firmware modificado</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="206"/>
        <source>Error: modified file checksum wrong</source>
        <translation>Erro: soma de controlo do ficheiro modificado incorrecta</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="214"/>
        <source>Success: modified firmware file created</source>
        <translation>Sucesso: ficheiro de firmware modificado criado</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="224"/>
        <source>Uninstallation not possible, only installation info removed</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="245"/>
        <source>Can&apos;t open input file</source>
        <translation>Não consegue abrir ficheiro de entrada</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="246"/>
        <source>Can&apos;t open output file</source>
        <translation>Não consegue abrir ficheiro de saída</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="247"/>
        <source>invalid file: header length wrong</source>
        <translation>ficheiro inválido: comprimento do cabeçalho incorrecto</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="248"/>
        <source>invalid file: unrecognized header</source>
        <translation>ficheiro inválido: cabeçalho irreconhecível</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="249"/>
        <source>invalid file: &quot;length&quot; field wrong</source>
        <translation>ficheiro inválido: campo &quot;length&quot;  incorrecto</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="250"/>
        <source>invalid file: &quot;length2&quot; field wrong</source>
        <translation>ficheiro inválido: campo &quot;length2&quot;  incorrecto</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="251"/>
        <source>invalid file: internal checksum error</source>
        <translation>ficheiro inválido: erro na soma de controlo interna</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="252"/>
        <source>invalid file: &quot;length3&quot; field wrong</source>
        <translation>ficheiro inválido: campo &quot;length3&quot;  incorrecto</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="253"/>
        <source>unknown</source>
        <translation>desconhecido</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="48"/>
        <source>Bootloader installation requires you to provide a firmware file of the original firmware (hex file). You need to download this file yourself due to legal reasons. Please refer to the &lt;a href=&apos;http://www.rockbox.org/manual.shtml&apos;&gt;manual&lt;/a&gt; and the &lt;a href=&apos;http://www.rockbox.org/wiki/IriverBoot#Download_and_extract_a_recent_ve&apos;&gt;IriverBoot&lt;/a&gt; wiki page on how to obtain this file.&lt;br/&gt;Press Ok to continue and browse your computer for the firmware file.</source>
        <translation type="unfinished">Instalação da rotina de arranque obriga-o a fornecer um ficheiro de firmware do firmware original (ficheiro hexadecimal). Deve transferi-lo você mesmo devido a razões legais. Por favor veja o &lt;a href=&apos;http://www.rockbox.org/manual.shtml&apos;&gt;manual&lt;/a&gt; e a página wiki &lt;a href=&apos;http://www.rockbox.org/wiki/IriverBoot#Download_and_extract_a_recent_ve&apos;&gt;IriverBoot&lt;/a&gt; para saber como obter este ficheiro.&lt;br/&gt;Pressione Ok para continuar e explorar o seu computador pelo ficheiro de firmware.</translation>
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
        <translation type="unfinished">Transferindo rotina de arranque</translation>
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
        <translation type="unfinished">Rotina de arranque instalada com sucesso</translation>
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
        <translation>Erro: não consegue alocar memória para o buffer!</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="81"/>
        <source>Downloading bootloader file</source>
        <translation>Transferindo rotina de arranque</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="65"/>
        <location filename="../base/bootloaderinstallipod.cpp" line="152"/>
        <source>Failed to read firmware directory</source>
        <translation>Falha na leitura da directoria do firmware</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="70"/>
        <location filename="../base/bootloaderinstallipod.cpp" line="157"/>
        <source>Unknown version number in firmware (%1)</source>
        <translation>Número de versão desconhecido no firmware (%1)</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="76"/>
        <source>Warning: This is a MacPod, Rockbox only runs on WinPods. 
See http://www.rockbox.org/wiki/IpodConversionToFAT32</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="95"/>
        <location filename="../base/bootloaderinstallipod.cpp" line="164"/>
        <source>Could not open Ipod in R/W mode</source>
        <translation>Falha na abertura do Ipod em modo de leitura e escrita (R/W)</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="105"/>
        <source>Successfull added bootloader</source>
        <translation>Adição com sucesso da rotina de arranque</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="116"/>
        <source>Failed to add bootloader</source>
        <translation>Falha ao adicionar rotina de arranque</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="128"/>
        <source>Bootloader Installation complete.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="133"/>
        <source>Writing log aborted</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="170"/>
        <source>No bootloader detected.</source>
        <translation>Nenhuma rotina de arranque detectada.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="176"/>
        <source>Successfully removed bootloader</source>
        <translation>Remoção com sucesso da rotina de arranque</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="183"/>
        <source>Removing bootloader failed.</source>
        <translation>Falha na remoção da rotina de arranque.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="230"/>
        <source>Error: could not retrieve device name</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="246"/>
        <source>Error: no mountpoint specified!</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="251"/>
        <source>Could not open Ipod: permission denied</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="255"/>
        <source>Could not open Ipod</source>
        <translation>Não consegiu abrir o Ipod</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="266"/>
        <source>No firmware partition on disk</source>
        <translation>Nenhuma partição do firmware no disco</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="91"/>
        <source>Installing Rockbox bootloader</source>
        <translation type="unfinished">Instalando a rotina de arranque do Rockbox</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="143"/>
        <source>Uninstalling bootloader</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="260"/>
        <source>Error reading partition table - possibly not an Ipod</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>BootloaderInstallMi4</name>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="33"/>
        <source>Downloading bootloader</source>
        <translation>Transferindo a rotina de arranque</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="42"/>
        <source>Installing Rockbox bootloader</source>
        <translation>Instalando a rotina de arranque do Rockbox</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="65"/>
        <source>Bootloader successful installed</source>
        <translation>Rotina de arranque instalada com sucesso</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="77"/>
        <source>Checking for Rockbox bootloader</source>
        <translation>Procurando pela rotina de arranque do Rockbox</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="79"/>
        <source>No Rockbox bootloader found</source>
        <translation>Não encontrada a rotina de arranque do Rockbox</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="84"/>
        <source>Checking for original firmware file</source>
        <translation>Procurando o ficheiro de firmware original</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="89"/>
        <source>Error finding original firmware file</source>
        <translation>Erro ao procurar o ficheiro firmware original</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="99"/>
        <source>Rockbox bootloader successful removed</source>
        <translation>Rotina de arranque do Rockbox removida com sucesso</translation>
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
        <translation type="unfinished">Transferindo rotina de arranque</translation>
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
        <translation type="unfinished">Sucesso: ficheiro de firmware modificado criado</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmpio.cpp" line="126"/>
        <source>To uninstall, perform a normal upgrade with an unmodified original firmware</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>BootloaderInstallSansa</name>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="55"/>
        <source>Error: can&apos;t allocate buffer memory!</source>
        <translation>Erro: não consegue alocar memória para o buffer!</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="60"/>
        <source>Searching for Sansa</source>
        <translation>Procurando pelo Sansa</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="66"/>
        <source>Permission for disc access denied!
This is required to install the bootloader</source>
        <translation>Permissão para o acesso ao disco negada!
Esta é necessária para instalar a rotina de arranque</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="73"/>
        <source>No Sansa detected!</source>
        <translation>Nenhum Sansa detectado!</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="86"/>
        <source>Downloading bootloader file</source>
        <translation>Transferindo rotina de arranque</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="78"/>
        <location filename="../base/bootloaderinstallsansa.cpp" line="188"/>
        <source>OLD ROCKBOX INSTALLATION DETECTED, ABORTING.
You must reinstall the original Sansa firmware before running
sansapatcher for the first time.
See http://www.rockbox.org/wiki/SansaE200Install
</source>
        <translation>INSTALAÇÃO ANTIGA DO ROCKBOX DETECTADA, A ABORTAR
Deve reinstalar o firmware original do Sansa antes de correr
o sansapatcher pela primeira vez.
Veja http://www.rockbox.org/wiki/SansaE200Install
</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="110"/>
        <location filename="../base/bootloaderinstallsansa.cpp" line="198"/>
        <source>Could not open Sansa in R/W mode</source>
        <translation>Falha na abertura do Sansa em modo de leitura e escrita (R/W)</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="137"/>
        <source>Successfully installed bootloader</source>
        <translation>Rotina de arranque instalada com sucesso</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="148"/>
        <source>Failed to install bootloader</source>
        <translation>Falha ao instalar rotina de arranque</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="161"/>
        <source>Bootloader Installation complete.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="166"/>
        <source>Writing log aborted</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="248"/>
        <source>Error: could not retrieve device name</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="264"/>
        <source>Can&apos;t find Sansa</source>
        <translation>Não consegue encontrar Sansa</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="269"/>
        <source>Could not open Sansa</source>
        <translation>Não consegiu abrir o Sansa</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="274"/>
        <source>Could not read partition table</source>
        <translation>Não consegiu ler tabela de partições</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="281"/>
        <source>Disk is not a Sansa (Error %1), aborting.</source>
        <translation>Disco não é um Sansa (Erro %1), a abortar.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="204"/>
        <source>Successfully removed bootloader</source>
        <translation>Remoção com sucesso da rotina de arranque</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="211"/>
        <source>Removing bootloader failed.</source>
        <translation>Falha na remoção da rotina de arranque.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="102"/>
        <source>Installing Rockbox bootloader</source>
        <translation type="unfinished">Instalando a rotina de arranque do Rockbox</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="119"/>
        <source>Checking downloaded bootloader</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="127"/>
        <source>Bootloader mismatch! Aborting.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="179"/>
        <source>Uninstalling bootloader</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>BootloaderInstallTcc</name>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="33"/>
        <source>Bootloader installation requires you to provide a firmware file of the original firmware (bin file). You need to download this file yourself due to legal reasons. Please refer to the &lt;a href=&apos;http://www.rockbox.org/manual.shtml&apos;&gt;manual&lt;/a&gt; and the &lt;a href=&apos;http://www.rockbox.org/wiki/CowonD2Info&apos;&gt;CowonD2Info&lt;/a&gt; wiki page on how to obtain the file.&lt;br/&gt;Press Ok to continue and browse your computer for the firmware file.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="50"/>
        <source>Downloading bootloader file</source>
        <translation type="unfinished">Transferindo rotina de arranque</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="82"/>
        <location filename="../base/bootloaderinstalltcc.cpp" line="99"/>
        <source>Could not load %1</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="90"/>
        <source>Unknown OF file used: %1</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="104"/>
        <source>Patching Firmware...</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="111"/>
        <source>Could not patch firmware</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="117"/>
        <source>Could not open %1 for writing</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="126"/>
        <source>Could not write firmware file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="131"/>
        <source>Success: modified firmware file created</source>
        <translation type="unfinished">Sucesso: ficheiro de firmware modificado criado</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="151"/>
        <source>To uninstall, perform a normal upgrade with an unmodified original firmware</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Config</name>
    <message>
        <location filename="../configure.cpp" line="301"/>
        <source>Current cache size is %L1 kiB.</source>
        <translation>Tamanho actual da cache é %L1 kiB.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="310"/>
        <source>Showing disabled targets</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="311"/>
        <source>You just enabled showing targets that are marked disabled. Disabled targets are not recommended to end users. Please use this option only if you know what you are doing.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="418"/>
        <location filename="../configure.cpp" line="448"/>
        <source>Configuration OK</source>
        <translation>Configuração OK</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="424"/>
        <location filename="../configure.cpp" line="453"/>
        <source>Configuration INVALID</source>
        <translation>Configuração INVÁLIDA</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="493"/>
        <source>Proxy Detection</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="494"/>
        <source>The System Proxy settings are invalid!
Rockbox Utility can&apos;t work with this proxy settings. Make sure the system proxy is set correctly. Note that &quot;proxy auto-config (PAC)&quot; scripts are not supported by Rockbox Utility. If your system uses this you need to use manual proxy settings.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="607"/>
        <source>Set Cache Path</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="736"/>
        <source>%1 &quot;MacPod&quot; found!
Rockbox needs a FAT formatted Ipod (so-called &quot;WinPod&quot;) to run. </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="750"/>
        <source>Fatal error</source>
        <translation>Erro fatal</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="755"/>
        <source>Detected an unsupported player:
%1
Sorry, Rockbox doesn&apos;t run on your player.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="760"/>
        <source>Fatal: player incompatible</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="840"/>
        <source>TTS configuration invalid</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="841"/>
        <source>TTS configuration invalid. 
 Please configure TTS engine.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="846"/>
        <source>Could not start TTS engine.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="847"/>
        <source>Could not start TTS engine.
</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="848"/>
        <location filename="../configure.cpp" line="867"/>
        <source>
Please configure TTS engine.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="862"/>
        <source>Rockbox Utility Voice Test</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="865"/>
        <source>Could not voice test string.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="866"/>
        <source>Could not voice test string.
</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="771"/>
        <location filename="../configure.cpp" line="780"/>
        <source>Autodetection</source>
        <translation>Auto-detecção</translation>
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
        <translation>Não consegiu detectar um ponto de montagem.
Seleccione um ponto de montagem manualmente.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="781"/>
        <source>Could not detect a device.
Select your device and Mountpoint manually.</source>
        <translation>Não consegiu detectar um dispositivo.
Seleccione um dispositivo e um ponto de montagem manualmente.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="792"/>
        <source>Really delete cache?</source>
        <translation>Eliminar mesmo a cache?</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="793"/>
        <source>Do you really want to delete the cache? Make absolutely sure this setting is correct as it will remove &lt;b&gt;all&lt;/b&gt; files in this folder!</source>
        <translation>Quer mesmo eliminar a cache? Verique que esta opção está correcta já que irá remover &lt;b&gt;todos&lt;/b&gt; os ficheiros nesta directoria!</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="801"/>
        <source>Path wrong!</source>
        <translation>Caminho incorrecto!</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="802"/>
        <source>The cache path is invalid. Aborting.</source>
        <translation>O caminho da cache é inválido. A abortar.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="122"/>
        <source>The following errors occurred:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="156"/>
        <source>No mountpoint given</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="160"/>
        <source>Mountpoint does not exist</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="164"/>
        <source>Mountpoint is not a directory.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="168"/>
        <source>Mountpoint is not writeable</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="183"/>
        <source>No player selected</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="190"/>
        <source>Cache path not writeable. Leave path empty to default to systems temporary path.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="210"/>
        <source>You need to fix the above errors before you can continue.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="213"/>
        <source>Configuration error</source>
        <translation type="unfinished">Erro de Configuração</translation>
    </message>
</context>
<context>
    <name>ConfigForm</name>
    <message>
        <location filename="../configurefrm.ui" line="14"/>
        <source>Configuration</source>
        <translation>Configuração</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="20"/>
        <source>Configure Rockbox Utility</source>
        <translation>Configurar Rockbox Utility</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="35"/>
        <source>&amp;Device</source>
        <translation>&amp;Dispositivo</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="41"/>
        <source>Select your device in the &amp;filesystem</source>
        <translation>Seleccione o seu dispositivo no sistema de &amp;ficheiros</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="312"/>
        <source>&amp;Browse</source>
        <translation>E&amp;xplorar</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="72"/>
        <source>&amp;Select your audio player</source>
        <translation>&amp;Seleccione o seu reprodutor de música</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="58"/>
        <source>&amp;Refresh</source>
        <translation type="unfinished">Actualizar</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="95"/>
        <source>Show disabled targets</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="114"/>
        <source>&amp;Autodetect</source>
        <translation>&amp;Auto-detecção</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="136"/>
        <source>&amp;Proxy</source>
        <translation>&amp;Proxy</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="142"/>
        <source>&amp;No Proxy</source>
        <translation>Sem &amp;Proxy</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="152"/>
        <source>Use S&amp;ystem values</source>
        <translation>Usar valores d&amp;o sistema</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="159"/>
        <source>&amp;Manual Proxy settings</source>
        <translation>Definições &amp;Manuais do Proxy</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="166"/>
        <source>Proxy Values</source>
        <translation>Valores do Proxy</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="172"/>
        <source>&amp;Host:</source>
        <translation>Anfi&amp;trião:</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="189"/>
        <source>&amp;Port:</source>
        <translation>&amp;Porta:</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="212"/>
        <source>&amp;Username</source>
        <translation>&amp;Utilizador</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="222"/>
        <source>Pass&amp;word</source>
        <translation>Palavra-c&amp;have</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="253"/>
        <source>&amp;Language</source>
        <translation>&amp;Linguagem</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="267"/>
        <source>Cac&amp;he</source>
        <translation>Cac&amp;he</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="270"/>
        <source>Download cache settings</source>
        <translation>Definições da transferência da cache</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="276"/>
        <source>Rockbox Utility uses a local download cache to save network traffic. You can change the path to the cache and use it as local repository by enabling Offline mode.</source>
        <translation>Rockbox Utility usa uma cache local para as transferências para poupar tráfego de internet. Pode mudar o caminho para a cache e usá-la como repositório local se activar o Modo Offline.</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="286"/>
        <source>Current cache size is %1</source>
        <translation>Tamanho actual da cache é %1</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="295"/>
        <source>P&amp;ath</source>
        <translation>C&amp;aminho</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="305"/>
        <source>Entering an invalid folder will reset the path to the systems temporary path.</source>
        <translation>Inserção duma directoria inválida irá reiniciar o caminho para o caminho temporário do sistema.</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="327"/>
        <source>Disable local &amp;download cache</source>
        <translation>Desactivar transferência local de cache</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="334"/>
        <source>&lt;p&gt;This will try to use all information from the cache, even information about updates. Only use this option if you want to install without network connection. Note: you need to do the same install you want to perform later with network access first to download all required files to the cache.&lt;/p&gt;</source>
        <translation>&lt;p&gt;Isto irá tentar usar toda a informação da cache, até informação sobre actualizaçẽos. Só use esta opção se desejar instalar sem conecção de internet. Nota: precisa de fazer primeiro a mesma instalação que deseja efectuar mais tarde com acesso à internet para transferir todos os ficheiros necessários para a cache.&lt;/p&gt;</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="337"/>
        <source>O&amp;ffline mode</source>
        <translation>Modo O&amp;ffline</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="372"/>
        <source>Clean cache &amp;now</source>
        <translation>Limpar agora a cac&amp;he</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="388"/>
        <source>&amp;TTS &amp;&amp; Encoder</source>
        <translation>&amp;TTS &amp;&amp; Encoder</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="394"/>
        <source>TTS Engine</source>
        <translation>Motor TTS</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="400"/>
        <source>&amp;Select TTS Engine</source>
        <translation>&amp;Seleccione Motor TTS</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="413"/>
        <source>Configure TTS Engine</source>
        <translation>Configuração do Motor TTS</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="420"/>
        <location filename="../configurefrm.ui" line="471"/>
        <source>Configuration invalid!</source>
        <translation>Configuração inválida!</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="437"/>
        <source>Configure &amp;TTS</source>
        <translation>Configurar &amp;TTS</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="448"/>
        <source>Test TTS</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="455"/>
        <source>&amp;Use string corrections for TTS</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="465"/>
        <source>Encoder Engine</source>
        <translation>Motor do Codificador</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="488"/>
        <source>Configure &amp;Enc</source>
        <translation>Configurar &amp;Cod</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="499"/>
        <source>encoder name</source>
        <translation>nome do codificador</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="539"/>
        <source>&amp;Ok</source>
        <translation>&amp;Ok</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="550"/>
        <source>&amp;Cancel</source>
        <translation>&amp;Cancelar</translation>
    </message>
</context>
<context>
    <name>Configure</name>
    <message>
        <location filename="../configure.cpp" line="553"/>
        <source>English</source>
        <comment>This is the localized language name, i.e. your language.</comment>
        <translation>Português (Portugal)</translation>
    </message>
</context>
<context>
    <name>CreateVoiceFrm</name>
    <message>
        <location filename="../createvoicefrm.ui" line="16"/>
        <source>Create Voice File</source>
        <translation>Criar Ficheiro de Voz</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="41"/>
        <source>Select the Language you want to generate a voicefile for:</source>
        <translation>Seleccione o idioma para o qual deseja gerar um ficheiro de voz:</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="48"/>
        <source>Language</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="55"/>
        <source>Generation settings</source>
        <translation>Definições de geração</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="61"/>
        <source>Encoder profile:</source>
        <translation>Perfil do codificador:</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="68"/>
        <source>TTS profile:</source>
        <translation>Perfil TTS:</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="81"/>
        <source>Change</source>
        <translation>Alterar</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="132"/>
        <source>&amp;Install</source>
        <translation>&amp;Instalar</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="142"/>
        <source>&amp;Cancel</source>
        <translation>&amp;Cancelar</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="156"/>
        <location filename="../createvoicefrm.ui" line="163"/>
        <source>Wavtrim Threshold</source>
        <translation>Limiar do Wavtrim</translation>
    </message>
</context>
<context>
    <name>CreateVoiceWindow</name>
    <message>
        <location filename="../createvoicewindow.cpp" line="97"/>
        <location filename="../createvoicewindow.cpp" line="100"/>
        <source>Selected TTS engine: &lt;b&gt;%1&lt;/b&gt;</source>
        <translation>Motor TTS seleccionado: &lt;b&gt;%1&lt;/b&gt;</translation>
    </message>
    <message>
        <location filename="../createvoicewindow.cpp" line="108"/>
        <location filename="../createvoicewindow.cpp" line="111"/>
        <location filename="../createvoicewindow.cpp" line="115"/>
        <source>Selected encoder: &lt;b&gt;%1&lt;/b&gt;</source>
        <translation>Codificador seleccionado: &lt;b&gt;%1&lt;/b&gt;</translation>
    </message>
</context>
<context>
    <name>EncTtsCfgGui</name>
    <message>
        <location filename="../encttscfggui.cpp" line="31"/>
        <source>Waiting for engine...</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../encttscfggui.cpp" line="81"/>
        <source>Ok</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../encttscfggui.cpp" line="84"/>
        <source>Cancel</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../encttscfggui.cpp" line="183"/>
        <source>Browse</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../encttscfggui.cpp" line="191"/>
        <source>Refresh</source>
        <translation type="unfinished"></translation>
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
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/encoderexe.cpp" line="42"/>
        <source>Encoder options:</source>
        <translation type="unfinished"></translation>
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
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/encoderrbspeex.cpp" line="35"/>
        <source>Quality:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/encoderrbspeex.cpp" line="37"/>
        <source>Complexity:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/encoderrbspeex.cpp" line="39"/>
        <source>Use Narrowband:</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>InfoWidget</name>
    <message>
        <location filename="../gui/infowidget.cpp" line="29"/>
        <source>File</source>
        <translation type="unfinished">Ficheiro</translation>
    </message>
    <message>
        <location filename="../gui/infowidget.cpp" line="29"/>
        <source>Version</source>
        <translation type="unfinished">Versão</translation>
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
        <translation type="unfinished">Pacotes actualmente instalados.&lt;br/&gt;&lt;b&gt;Nota:&lt;/b&gt; se instalou manualmente pacotes, isto pode não estar correcto!</translation>
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
        <translation>Instalar Ficheiros de Leitura</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="36"/>
        <source>Select the Folder to generate Talkfiles for.</source>
        <translation>Seleccione a Directoria para gerar Ficheiros de Leitura.</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="43"/>
        <source>Talkfile Folder</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="50"/>
        <source>&amp;Browse</source>
        <translation>E&amp;xplorar</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="61"/>
        <source>Generation settings</source>
        <translation>Definições de geração</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="67"/>
        <source>Encoder profile:</source>
        <translation>Perfil do codificador:</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="74"/>
        <source>TTS profile:</source>
        <translation>Perfil TTS:</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="87"/>
        <source>Change</source>
        <translation>Alterar</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="162"/>
        <source>Generation options</source>
        <translation>Opções de geração</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="171"/>
        <source>Ignore files (comma seperated Wildcards):</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="201"/>
        <source>Run recursive</source>
        <translation>Correr recursivo</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="211"/>
        <source>Strip Extensions</source>
        <translation>Retirar Extensões</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="221"/>
        <source>Create only new Talkfiles</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="191"/>
        <source>Generate .talk files for Folders</source>
        <translation>Gerar ficheiros .talk para Directorias</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="178"/>
        <source>Generate .talk files for Files</source>
        <translation>Gerar ficheiros .talk para Ficheiros</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="138"/>
        <source>&amp;Install</source>
        <translation>&amp;Instalar</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="149"/>
        <source>&amp;Cancel</source>
        <translation>&amp;Cancelar</translation>
    </message>
</context>
<context>
    <name>InstallTalkWindow</name>
    <message>
        <location filename="../installtalkwindow.cpp" line="54"/>
        <source>Select folder to create talk files</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../installtalkwindow.cpp" line="89"/>
        <source>The Folder to Talk is wrong!</source>
        <translation>A Directoria para Falar está incorrecta!</translation>
    </message>
    <message>
        <location filename="../installtalkwindow.cpp" line="122"/>
        <location filename="../installtalkwindow.cpp" line="125"/>
        <source>Selected TTS engine: &lt;b&gt;%1&lt;/b&gt;</source>
        <translation>Motor TTS seleccionado: &lt;b&gt;%1&lt;/b&gt;</translation>
    </message>
    <message>
        <location filename="../installtalkwindow.cpp" line="132"/>
        <location filename="../installtalkwindow.cpp" line="135"/>
        <location filename="../installtalkwindow.cpp" line="139"/>
        <source>Selected encoder: &lt;b&gt;%1&lt;/b&gt;</source>
        <translation>Codificador seleccionado: &lt;b&gt;%1&lt;/b&gt;</translation>
    </message>
</context>
<context>
    <name>InstallWindow</name>
    <message>
        <location filename="../installwindow.cpp" line="107"/>
        <source>Backup to %1</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../installwindow.cpp" line="137"/>
        <source>Mount point is wrong!</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../installwindow.cpp" line="174"/>
        <source>Really continue?</source>
        <translation type="unfinished">Quer mesmo continuar?</translation>
    </message>
    <message>
        <location filename="../installwindow.cpp" line="178"/>
        <source>Aborted!</source>
        <translation type="unfinished">Abortado!</translation>
    </message>
    <message>
        <location filename="../installwindow.cpp" line="187"/>
        <source>Beginning Backup...</source>
        <translation type="unfinished">Iniciando cópia de segurança...</translation>
    </message>
    <message>
        <location filename="../installwindow.cpp" line="209"/>
        <source>Backup finished.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../installwindow.cpp" line="212"/>
        <source>Backup failed!</source>
        <translation type="unfinished">Falha na cópia de segurança!</translation>
    </message>
    <message>
        <location filename="../installwindow.cpp" line="243"/>
        <source>Select Backup Filename</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../installwindow.cpp" line="276"/>
        <source>This is the absolute up to the minute Rockbox built. A current build will get updated every time a change is made. Latest version is %1 (%2).</source>
        <translation type="unfinished">Esta é compilação do Rockbox actualizada ao minuto. Uma compilação actual será actualizada sempre que houver uma mudança. Última versão é %1 (%2).</translation>
    </message>
    <message>
        <location filename="../installwindow.cpp" line="282"/>
        <source>&lt;b&gt;This is the recommended version.&lt;/b&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../installwindow.cpp" line="293"/>
        <source>This is the last released version of Rockbox.</source>
        <translation type="unfinished">Esta é a última versão lançada do Rockbox.</translation>
    </message>
    <message>
        <location filename="../installwindow.cpp" line="296"/>
        <source>&lt;b&gt;Note:&lt;/b&gt; The lastest released version is %1. &lt;b&gt;This is the recommended version.&lt;/b&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../installwindow.cpp" line="308"/>
        <source>These are automatically built each day from the current development source code. This generally has more features than the last stable release but may be much less stable. Features may change regularly.</source>
        <translation type="unfinished">Estas são compiladas todos os dias do código fonte em actual, desenvolvimento. Este tem normalmente mais capacidades que a última versão estável mas pode ser muito menos estável. Capacidades podem mudar regularmente.</translation>
    </message>
    <message>
        <location filename="../installwindow.cpp" line="312"/>
        <source>&lt;b&gt;Note:&lt;/b&gt; archived version is %1 (%2).</source>
        <translation type="unfinished">&lt;b&gt;Nota:&lt;/b&gt; versão arquivada é %1 (%2).</translation>
    </message>
</context>
<context>
    <name>InstallWindowFrm</name>
    <message>
        <location filename="../installwindowfrm.ui" line="16"/>
        <source>Install Rockbox</source>
        <translation type="unfinished">Instalar Rockbox</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="35"/>
        <source>Please select the Rockbox version you want to install on your player:</source>
        <translation type="unfinished">Por favor, seleccione a versão do Rockbox que desja instalar no seu reprodutor:</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="45"/>
        <source>Version</source>
        <translation type="unfinished">Versão</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="51"/>
        <source>Rockbox &amp;stable</source>
        <translation type="unfinished">Rockbox e&amp;stável</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="58"/>
        <source>&amp;Archived Build</source>
        <translation type="unfinished">Compilação &amp;Arquivada</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="65"/>
        <source>&amp;Current Build</source>
        <translation type="unfinished">&amp;Compilação Actual</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="75"/>
        <source>Details</source>
        <translation type="unfinished">Detalhes</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="81"/>
        <source>Details about the selected version</source>
        <translation type="unfinished">Detlhes sobre a versão seleccionada</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="91"/>
        <source>Note</source>
        <translation type="unfinished">Nota</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="119"/>
        <source>&amp;Install</source>
        <translation type="unfinished">&amp;Instalar</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="130"/>
        <source>&amp;Cancel</source>
        <translation type="unfinished">&amp;Cancelar</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="156"/>
        <source>Backup</source>
        <translation type="unfinished">Cópia de Segurança</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="162"/>
        <source>Backup before installing</source>
        <translation type="unfinished">Cópia de segurança antes de instalar</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="169"/>
        <source>Backup location</source>
        <translation type="unfinished">Localização da Cópia de Segurança</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="188"/>
        <source>Change</source>
        <translation type="unfinished">Alterar</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="198"/>
        <source>Rockbox Utility stores copies of Rockbox it has downloaded on the local hard disk to save network traffic. If your local copy is no longer working, tick this box to download a fresh copy.</source>
        <translation type="unfinished">Rockbox Utility guarda cópias do Rockbox que tenha transferido no disco rígido para poupar tráfego de internet. Se a cópia local já não funciona, selecione esta caixa para transferir uma nova cópia.</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="201"/>
        <source>&amp;Don&apos;t use locally cached copy</source>
        <translation type="unfinished">Não usar cópia &amp;da cache local</translation>
    </message>
</context>
<context>
    <name>ManualWidget</name>
    <message>
        <location filename="../gui/manualwidget.cpp" line="78"/>
        <source>&lt;a href=&apos;%1&apos;&gt;PDF Manual&lt;/a&gt;</source>
        <translation type="unfinished">&lt;a href=&apos;%1&apos;&gt;Manual PDF&lt;/a&gt;</translation>
    </message>
    <message>
        <location filename="../gui/manualwidget.cpp" line="80"/>
        <source>&lt;a href=&apos;%1&apos;&gt;HTML Manual (opens in browser)&lt;/a&gt;</source>
        <translation type="unfinished">&lt;a href=&apos;%1&apos;&gt;Manual HTML (abre num navegador de internet)&lt;/a&gt;</translation>
    </message>
    <message>
        <location filename="../gui/manualwidget.cpp" line="84"/>
        <source>Select a device for a link to the correct manual</source>
        <translation type="unfinished">Seleccione um dispositivo para uma ligação ao manual correcto</translation>
    </message>
    <message>
        <location filename="../gui/manualwidget.cpp" line="85"/>
        <source>&lt;a href=&apos;%1&apos;&gt;Manual Overview&lt;/a&gt;</source>
        <translation type="unfinished">&lt;a href=&apos;%1&apos;&gt;Visão Geral do Manual&lt;/a&gt;</translation>
    </message>
    <message>
        <location filename="../gui/manualwidget.cpp" line="96"/>
        <source>Confirm download</source>
        <translation type="unfinished">Confirmar transferência</translation>
    </message>
    <message>
        <location filename="../gui/manualwidget.cpp" line="97"/>
        <source>Do you really want to download the manual? The manual will be saved to the root folder of your player.</source>
        <translation type="unfinished">Deseja transferir o manual? O manual será guardado na directoria raiz do reprodutor.</translation>
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
        <translation type="unfinished">Ler o manual</translation>
    </message>
    <message>
        <location filename="../gui/manualwidgetfrm.ui" line="26"/>
        <source>PDF manual</source>
        <translation type="unfinished">Manual PDF</translation>
    </message>
    <message>
        <location filename="../gui/manualwidgetfrm.ui" line="39"/>
        <source>HTML manual</source>
        <translation type="unfinished">Manual HTML</translation>
    </message>
    <message>
        <location filename="../gui/manualwidgetfrm.ui" line="55"/>
        <source>Download the manual</source>
        <translation type="unfinished">Transferir o manual</translation>
    </message>
    <message>
        <location filename="../gui/manualwidgetfrm.ui" line="63"/>
        <source>&amp;PDF version</source>
        <translation type="unfinished">Versão &amp;PDF</translation>
    </message>
    <message>
        <location filename="../gui/manualwidgetfrm.ui" line="70"/>
        <source>&amp;HTML version (zip file)</source>
        <translation type="unfinished">Versão &amp;HTML (ficheiro zip)</translation>
    </message>
    <message>
        <location filename="../gui/manualwidgetfrm.ui" line="92"/>
        <source>Down&amp;load</source>
        <translation type="unfinished">&amp;Transferir</translation>
    </message>
</context>
<context>
    <name>PreviewFrm</name>
    <message>
        <location filename="../previewfrm.ui" line="16"/>
        <source>Preview</source>
        <translation>Pré-Visualização</translation>
    </message>
</context>
<context>
    <name>ProgressLoggerFrm</name>
    <message>
        <location filename="../progressloggerfrm.ui" line="13"/>
        <location filename="../progressloggerfrm.ui" line="19"/>
        <source>Progress</source>
        <translation>Progresso</translation>
    </message>
    <message>
        <location filename="../progressloggerfrm.ui" line="32"/>
        <source>progresswindow</source>
        <translation>janela de progresso</translation>
    </message>
    <message>
        <location filename="../progressloggerfrm.ui" line="58"/>
        <source>Save Log</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../progressloggerfrm.ui" line="82"/>
        <source>&amp;Abort</source>
        <translation>&amp;Abortar</translation>
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
        <location filename="../progressloggergui.cpp" line="144"/>
        <source>Save system trace log</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../progressloggergui.cpp" line="103"/>
        <source>&amp;Abort</source>
        <translation>&amp;Abortar</translation>
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
        <translation type="unfinished"></translation>
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
        <location filename="../rbutilqt.cpp" line="239"/>
        <location filename="../rbutilqt.cpp" line="270"/>
        <source>Network error</source>
        <translation>Erro de rede</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="357"/>
        <source>New installation</source>
        <translation>Nova instalação</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="358"/>
        <source>This is a new installation of Rockbox Utility, or a new version. The configuration dialog will now open to allow you to setup the program,  or review your settings.</source>
        <translation>Esta é uma nova instalação do Rockbox Utility, ou uma nova versão. O diálogo de configuração irá agora abrir para permitir-lhe configurar o programa, ou rever as suas definições.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="365"/>
        <location filename="../rbutilqt.cpp" line="1159"/>
        <source>Configuration error</source>
        <translation>Erro de Configuração</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="366"/>
        <source>Your configuration is invalid. This is most likely due to a changed device path. The configuration dialog will now open to allow you to correct the problem.</source>
        <translation>A sua configuração é inválida. Isto é provalvelmente devido a uma mudança do caminho do dispositivo. O diálogo de configuração irá agora abrir para permitir-lhe corrigir o problema.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="227"/>
        <location filename="../rbutilqt.cpp" line="260"/>
        <source>Downloading build information, please wait ...</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="238"/>
        <location filename="../rbutilqt.cpp" line="269"/>
        <source>Can&apos;t get version information!</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="240"/>
        <location filename="../rbutilqt.cpp" line="271"/>
        <source>Can&apos;t get version information.
Network error: %1. Please check your network and proxy settings.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="281"/>
        <source>Download build information finished.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="411"/>
        <source>&lt;b&gt;%1 %2&lt;/b&gt; at &lt;b&gt;%3&lt;/b&gt;</source>
        <translation>&lt;b&gt;%1 %2&lt;/b&gt; em &lt;b&gt;%3&lt;/b&gt;</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="426"/>
        <location filename="../rbutilqt.cpp" line="482"/>
        <location filename="../rbutilqt.cpp" line="659"/>
        <location filename="../rbutilqt.cpp" line="830"/>
        <location filename="../rbutilqt.cpp" line="917"/>
        <location filename="../rbutilqt.cpp" line="961"/>
        <source>Confirm Installation</source>
        <translation>Confirmar Instalação</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="427"/>
        <source>Do you really want to perform a complete installation?

This will install Rockbox %1. To install the most recent development build available press &quot;Cancel&quot; and use the &quot;Installation&quot; tab.</source>
        <translation>Deseja efectuar uma instalação completa?

￼Isto irá instalar Rockbox %1. Para instalar a compilação de desvolvimento mais recente pressione &quot;Cancelar&quot; e use o separador &quot;Instalação&quot;.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="483"/>
        <source>Do you really want to perform a minimal installation? A minimal installation will contain only the absolutely necessary parts to run Rockbox.

This will install Rockbox %1. To install the most recent development build available press &quot;Cancel&quot; and use the &quot;Installation&quot; tab.</source>
        <translation>Deseja efectuar uma instalação mínima? Uma instalação mínima irá conter somente as partes absolutamente necessárias para correr Rockbox.
Isto irá instalar Rockbox %1. Para instalar a compilação de desvolvimento mais recente pressione &quot;Cancelar&quot; e use o separador &quot;Instalação&quot;.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="505"/>
        <location filename="../rbutilqt.cpp" line="1105"/>
        <source>Mount point is wrong!</source>
        <translation>Ponto de montagem está errado!</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="569"/>
        <source>Really continue?</source>
        <translation>Quer mesmo continuar?</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="573"/>
        <source>Aborted!</source>
        <translation>Abortado!</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="583"/>
        <source>Installed Rockbox detected</source>
        <translation>Instalação do Rockbox detectada</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="584"/>
        <source>Rockbox installation detected. Do you want to backup first?</source>
        <translation>Instalação do Rockbox detectada. Deseja criar uma cópia de segurança antes?</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="588"/>
        <source>Starting backup...</source>
        <translation>Iniciando a cópia de segurança...</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="600"/>
        <source>Beginning Backup...</source>
        <translation type="unfinished">Iniciando cópia de segurança...</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="614"/>
        <source>Backup successful</source>
        <translation>Cópia de segurança criada com sucesso</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="618"/>
        <source>Backup failed!</source>
        <translation>Falha na cópia de segurança!</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="660"/>
        <source>Do you really want to install the Bootloader?</source>
        <translation>Deseja instalar a Rotina de Arranque?</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="679"/>
        <source>No install method known.</source>
        <translation>Nenhum método de instalação conhecido.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="706"/>
        <source>Bootloader detected</source>
        <translation>Rotina de arranque detectada</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="707"/>
        <source>Bootloader already installed. Do you want to reinstall the bootloader?</source>
        <translation>Rotina de arranque já instalada. Deseja reinstalá-la?</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="730"/>
        <source>Create Bootloader backup</source>
        <translation>Criar cópia de segurança da rotina de arranque</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="731"/>
        <source>You can create a backup of the original bootloader file. Press &quot;Yes&quot; to select an output folder on your computer to save the file to. The file will get placed in a new folder &quot;%1&quot; created below the selected folder.
Press &quot;No&quot; to skip this step.</source>
        <translation>Pde criar uma cópia de segurança da rotina original de arranque. Pressione &quot;Sim&quot; para seleccionar uma directoria de saída no computador para guardar a cópia. O ficheiro será colocado numa nova directoria &quot;%1&quot; criada dentro da directoria seleccionada.
Pressione &quot;No&quot; para passar este passo.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="738"/>
        <source>Browse backup folder</source>
        <translation>Explorar directoria de cópias de segurança</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="750"/>
        <source>Prerequisites</source>
        <translation>Pré-requesitos</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="763"/>
        <source>Select firmware file</source>
        <translation>Seleccione ficheiro de firmware</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="765"/>
        <source>Error opening firmware file</source>
        <translation>Erro ao abrir o ficheiro de firmware</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="771"/>
        <source>Error reading firmware file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="781"/>
        <source>Backup error</source>
        <translation>Erro na cópia de segurança</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="782"/>
        <source>Could not create backup file. Continue?</source>
        <translation>Falha na criação da cópia de segurança. Continuar?</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="812"/>
        <source>Manual steps required</source>
        <translation>Passos manuais necessários</translation>
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
        <translation>Deseja instalar os pacotes de fontes?</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="888"/>
        <source>Warning</source>
        <translation>Aviso</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="889"/>
        <source>The Application is still downloading Information about new Builds. Please try again shortly.</source>
        <translation>A Aplicação está ainda a transferir informação sobre novas compilações. Por favor, tente de novo em breve.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="903"/>
        <source>Could not determine the installed Rockbox version. Please install a Rockbox build before installing voice files.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="918"/>
        <source>Do you really want to install the voice file?</source>
        <translation>Deseja instalar o ficheiro de voz?</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="956"/>
        <source>Error</source>
        <translation>Erro</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="957"/>
        <source>Your device doesn&apos;t have a doom plugin. Aborting.</source>
        <translation>O seu dispositivo não tem uma extensão do doom. A  abortar.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="962"/>
        <source>Do you really want to install the game addon files?</source>
        <translation>Deseja instalar os ficheiros extras de jogo?</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1040"/>
        <source>Confirm Uninstallation</source>
        <translation>Confirmar Desinstalação</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1041"/>
        <source>Do you really want to uninstall the Bootloader?</source>
        <translation>Quer mesmo desinstalar a Rotina de Arranque?</translation>
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
        <location filename="../rbutilqt.cpp" line="1091"/>
        <source>Confirm installation</source>
        <translation>Confirmar instalação</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1092"/>
        <source>Do you really want to install Rockbox Utility to your player? After installation you can run it from the players hard drive.</source>
        <translation>Deseja instalar o Rockbox Utility no reprodutor? Após a instalação pode corrê-lo do disco rígido do reprodutor.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1101"/>
        <source>Installing Rockbox Utility</source>
        <translation>Instalando Rockbox Utility</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1119"/>
        <source>Error installing Rockbox Utility</source>
        <translation>Erro ao instalar Rockbox Utility</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1123"/>
        <source>Installing user configuration</source>
        <translation>Instalando a configuração do utilizador</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1127"/>
        <source>Error installing user configuration</source>
        <translation>Erro ao instalar a configuração do utilizador</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1131"/>
        <source>Successfully installed Rockbox Utility.</source>
        <translation>Rockbox Utility instalado com sucesso.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1160"/>
        <source>Your configuration is invalid. Please go to the configuration dialog and make sure the selected values are correct.</source>
        <translation>A sua configuração é inválida. Por favor, vá ao diálogo de configuração e verifique que os valores inseridos estão correctos.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1183"/>
        <source>Checking for update ...</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1248"/>
        <source>RockboxUtility Update available</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1249"/>
        <source>&lt;b&gt;New RockboxUtility Version available.&lt;/b&gt; &lt;br&gt;&lt;br&gt;Download it from here: &lt;a href=&apos;%1&apos;&gt;%2&lt;/a&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="712"/>
        <source>Bootloader installation skipped</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="103"/>
        <source>Wine detected!</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="104"/>
        <source>It seems you are trying to run this program under Wine. Please don&apos;t do this, running under Wine will fail. Use the native Linux binary instead.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="756"/>
        <source>Bootloader installation aborted</source>
        <translation type="unfinished"></translation>
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
        <location filename="../rbutilqtfrm.ui" line="71"/>
        <source>Device</source>
        <translation>Dispositivo</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="83"/>
        <source>Selected device:</source>
        <translation>Seleccione dispositivo:</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="90"/>
        <source>device / mountpoint unknown or invalid</source>
        <translation>ponto de montagem ou dispositivo desconhecido ou inválido</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="110"/>
        <source>&amp;Change</source>
        <translation>A&amp;lterar</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="128"/>
        <location filename="../rbutilqtfrm.ui" line="711"/>
        <source>&amp;Quick Start</source>
        <translation>Início Rá&amp;pido</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="131"/>
        <source>Welcome</source>
        <translation>Bem-Vindo</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="137"/>
        <source>Complete Installation</source>
        <translation>Instalação Completa</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="154"/>
        <source>&lt;b&gt;Complete Installation&lt;/b&gt;&lt;br/&gt;This installs the bootloader, a current build and the extras package. This is the recommended method for new installations.</source>
        <translation>&lt;b&gt;Instalação Completa&lt;/b&gt;&lt;br/&gt;Isto instala a rotina de arranque, uma compilação actual e o pacotes de extras. Este é o método recomendado para nova instalações.</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="167"/>
        <source>Minimal Installation</source>
        <translation>Instalação Mínima</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="184"/>
        <source>&lt;b&gt;Minimal installation&lt;/b&gt;&lt;br/&gt;This installs bootloader and the current build of Rockbox. If you don&apos;t want the extras package, choose this option.</source>
        <translation>&lt;b&gt;Instalação Mínima&lt;/b&gt;&lt;br/&gt;Isto instala a rotina de arranque e uma compilação actual do Rockbox. Se não desejar o pacotes de extras, escolha esta opção.</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="224"/>
        <location filename="../rbutilqtfrm.ui" line="704"/>
        <source>&amp;Installation</source>
        <translation>&amp;Instalação</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="227"/>
        <source>Basic Rockbox installation</source>
        <translation>Instalação básica do Rockbox</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="233"/>
        <source>Install Bootloader</source>
        <translation>Instalar Rotina de Arranque</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="250"/>
        <source>&lt;b&gt;Install the bootloader&lt;/b&gt;&lt;br/&gt;Before Rockbox can be run on your audio player, you may have to install a bootloader. This is only necessary the first time Rockbox is installed.</source>
        <translation>&lt;b&gt;Instalar a rotina de arranque&lt;/b&gt;&lt;br/&gt;Antes do Rockbox poder ser executado no reprodutor de música, pode ser necessário a instalação de uma rotina de arranque. Isto só é necessário a primeira vez que o Rockbox é instalado.</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="260"/>
        <source>Install Rockbox</source>
        <translation>Instalar Rockbox</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="277"/>
        <source>&lt;b&gt;Install Rockbox&lt;/b&gt; on your audio player</source>
        <translation>&lt;b&gt;Instalar Rockbox&lt;/b&gt; no seu reprodutor de música</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="320"/>
        <location filename="../rbutilqtfrm.ui" line="718"/>
        <source>&amp;Extras</source>
        <translation>&amp;Extras</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="323"/>
        <source>Install extras for Rockbox</source>
        <translation>Instalar extras para o Rockbox</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="329"/>
        <source>Install Fonts package</source>
        <translation>Instalar pacote de fontes</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="346"/>
        <source>&lt;b&gt;Fonts Package&lt;/b&gt;&lt;br/&gt;The Fonts Package contains a couple of commonly used fonts. Installation is highly recommended.</source>
        <translation>&lt;b&gt;Pacote de Fontes&lt;/b&gt;&lt;br/&gt;O Pacote de Fontes contêm algumas das mais utilizadas fontes. A sua instalação é altamente recomendada.</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="356"/>
        <source>Install themes</source>
        <translation>Instalar temas</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="383"/>
        <source>Install game files</source>
        <translation>Instalar ficheiros de jogo</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="400"/>
        <source>&lt;b&gt;Install Game Files&lt;/b&gt;&lt;br/&gt;Doom needs a base wad file to run.</source>
        <translation>&lt;b&gt;Instalar Ficheiros de Jogo&lt;/b&gt;&lt;br/&gt;Doom precisa de um ficheiro base wad para correr.</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="437"/>
        <location filename="../rbutilqtfrm.ui" line="726"/>
        <source>&amp;Accessibility</source>
        <translation>&amp;Acessibilidade</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="440"/>
        <source>Install accessibility add-ons</source>
        <translation>Instalar extensões de acessibilidade</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="446"/>
        <source>Install Voice files</source>
        <translation>Instalar Ficheiros de Voz</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="463"/>
        <source>&lt;b&gt;Install Voice file&lt;/b&gt;&lt;br/&gt;Voice files are needed to make Rockbox speak the user interface. Speaking is enabled by default, so if you installed the voice file Rockbox will speak.</source>
        <translation>&lt;b&gt;Instalar ficheiro de Voz&lt;/b&gt;&lt;br/&gt;Ficherios de Voz são necessários para fazer o Rockbox ler a interface do utilizador. Falar está activo por omissão, portanto se instalou o ficheiro de voz, o Rockbox irá falar.</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="473"/>
        <source>Install Talk files</source>
        <translation>Instalar Ficheiros de Leitura</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="490"/>
        <source>&lt;b&gt;Create Talk Files&lt;/b&gt;&lt;br/&gt;Talkfiles are needed to let Rockbox speak File and Foldernames</source>
        <translation>&lt;b&gt;Criar Ficheiros de Leitura&lt;/b&gt;&lt;br/&gt;Ficheiros de Leitura são necessários para deixar o Rockbox ler Ficheiros e nomes de Directorias</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="523"/>
        <source>Create Voice files</source>
        <translation>Criar Ficheiros de Voz</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="540"/>
        <source>&lt;b&gt;Create Voice file&lt;/b&gt;&lt;br/&gt;Voice files are needed to make Rockbox speak the  user interface. Speaking is enabled by default, so
 if you installed the voice file Rockbox will speak.</source>
        <translation>&lt;b&gt;Criar ficheiro de Voz&lt;/b&gt;&lt;br/&gt;Ficherios de Voz são necessários para fazer o Rockbox ler a interface do utilizador. Falar está activo por omissão, portanto
se instalou o ficheiro de voz, o Rockbox irá falar.</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="552"/>
        <location filename="../rbutilqtfrm.ui" line="734"/>
        <source>&amp;Uninstallation</source>
        <translation>Desinstala&amp;ção</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="555"/>
        <location filename="../rbutilqtfrm.ui" line="588"/>
        <source>Uninstall Rockbox</source>
        <translation>Desinstalar Rockbox</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="561"/>
        <source>Uninstall Bootloader</source>
        <translation>Desinstalar rotina de arranque</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="578"/>
        <source>&lt;b&gt;Remove the bootloader&lt;/b&gt;&lt;br/&gt;After removing the bootloader you won&apos;t be able to start Rockbox.</source>
        <translation>&lt;b&gt;Remover a rotina de arranque&lt;/b&gt;&lt;br/&gt;Depois de remover a rotina de arranque não poderá iniciar o Rockbox.</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="605"/>
        <source>&lt;b&gt;Uninstall Rockbox from your audio player.&lt;/b&gt;&lt;br/&gt;This will leave the bootloader in place (you need to remove it manually).</source>
        <translation>&lt;b&gt;Desinstalar Rockbox do reprodutor de música.&lt;/b&gt;&lt;br/&gt;Isto irá deixar a rotina de arranque no sítio (terá de removê-la manualmente).</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="648"/>
        <source>&amp;Manual</source>
        <translation>&amp;Manual</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="651"/>
        <source>View and download the manual</source>
        <translation>Ver e transferir o manual</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="656"/>
        <source>Inf&amp;o</source>
        <translation>Inf&amp;ormação</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="674"/>
        <source>&amp;File</source>
        <translation>&amp;Ficheiro</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="931"/>
        <source>System &amp;Trace</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="700"/>
        <source>Action&amp;s</source>
        <translation>Acçõe&amp;s</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="752"/>
        <source>Empty local download cache</source>
        <translation></translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="757"/>
        <source>Install Rockbox Utility on player</source>
        <translation>Instalar o Rockbox Utility no reprodutor</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="762"/>
        <source>&amp;Configure</source>
        <translation>&amp;Configurar</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="767"/>
        <source>E&amp;xit</source>
        <translation>Sai&amp;r</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="770"/>
        <source>Ctrl+Q</source>
        <translation>Ctrl+Q</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="775"/>
        <source>&amp;About</source>
        <translation>S&amp;obre</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="780"/>
        <source>About &amp;Qt</source>
        <translation>Sobre &amp;Qt</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="683"/>
        <location filename="../rbutilqtfrm.ui" line="785"/>
        <source>&amp;Help</source>
        <translation>&amp;Ajuda</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="373"/>
        <source>&lt;b&gt;Install Themes&lt;/b&gt;&lt;br/&gt;Rockbox&apos;s look can be customized by themes. You can choose and install several officially distributed themes.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="687"/>
        <source>&amp;Troubleshoot</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="790"/>
        <source>Info</source>
        <translation>Informação</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="799"/>
        <source>&amp;Complete Installation</source>
        <translation>Instalação &amp;Completa</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="808"/>
        <source>&amp;Minimal Installation</source>
        <translation>Instalação &amp;Mínima</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="817"/>
        <source>Install &amp;Bootloader</source>
        <translation>Instalar Rotina &amp;de Arranque</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="826"/>
        <source>Install &amp;Rockbox</source>
        <translation>Instalar &amp;Rockbox</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="835"/>
        <source>Install &amp;Fonts Package</source>
        <translation>Instalar pacote de &amp;fontes</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="844"/>
        <source>Install &amp;Themes</source>
        <translation>Instalar &amp;temas</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="853"/>
        <source>Install &amp;Game Files</source>
        <translation>Instalar Ficheiros de &amp;Jogo</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="862"/>
        <source>&amp;Install Voice File</source>
        <translation>&amp;Instalar Ficheiro de Voz</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="871"/>
        <source>Create &amp;Talk Files</source>
        <translation>Criar Ficheiros de &amp;Leitura</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="880"/>
        <source>Remove &amp;bootloader</source>
        <translation>Remover rotina de arran&amp;que</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="889"/>
        <source>Uninstall &amp;Rockbox</source>
        <translation>Desinstalar &amp;Rockbox</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="894"/>
        <source>Read PDF manual</source>
        <translation>Ler manual PDF</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="899"/>
        <source>Read HTML manual</source>
        <translation>Ler manual HTML</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="904"/>
        <source>Download PDF manual</source>
        <translation>Transferir manual PDF</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="909"/>
        <source>Download HTML manual (zip)</source>
        <translation>Transferir manual HTML (zip)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="918"/>
        <source>Create &amp;Voice File</source>
        <translation>Criar Ficheiro de &amp;Voz</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="921"/>
        <source>Create Voice File</source>
        <translation>Criar Ficheiro de Voz</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="926"/>
        <source>&amp;System Info</source>
        <translation>Informação do &amp;sistema</translation>
    </message>
</context>
<context>
    <name>ServerInfo</name>
    <message>
        <location filename="../base/serverinfo.cpp" line="71"/>
        <source>Unknown</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/serverinfo.cpp" line="75"/>
        <source>Unusable</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/serverinfo.cpp" line="78"/>
        <source>Unstable</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/serverinfo.cpp" line="81"/>
        <source>Stable</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>SysTrace</name>
    <message>
        <location filename="../systrace.cpp" line="75"/>
        <location filename="../systrace.cpp" line="84"/>
        <source>Save system trace log</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>SysTraceFrm</name>
    <message>
        <location filename="../systracefrm.ui" line="14"/>
        <source>System Trace</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../systracefrm.ui" line="20"/>
        <source>System State trace</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../systracefrm.ui" line="46"/>
        <source>&amp;Close</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../systracefrm.ui" line="57"/>
        <source>&amp;Save</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../systracefrm.ui" line="68"/>
        <source>&amp;Refresh</source>
        <translation type="unfinished">Actualizar</translation>
    </message>
    <message>
        <location filename="../systracefrm.ui" line="79"/>
        <source>Save &amp;previous</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Sysinfo</name>
    <message>
        <location filename="../sysinfo.cpp" line="44"/>
        <source>&lt;b&gt;OS&lt;/b&gt;&lt;br/&gt;</source>
        <translation>&lt;b&gt;Sistema Operativo&lt;/b&gt;&lt;br/&gt;</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="45"/>
        <source>&lt;b&gt;Username&lt;/b&gt;&lt;br/&gt;%1&lt;hr/&gt;</source>
        <translation>&lt;b&gt;Utilizador&lt;/b&gt;&lt;br/&gt;%1&lt;hr/&gt;</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="47"/>
        <source>&lt;b&gt;Permissions&lt;/b&gt;&lt;br/&gt;%1&lt;hr/&gt;</source>
        <translation>&lt;b&gt;Permissões&lt;/b&gt;&lt;br/&gt;%1&lt;hr/&gt;</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="49"/>
        <source>&lt;b&gt;Attached USB devices&lt;/b&gt;&lt;br/&gt;</source>
        <translation>&lt;b&gt;Dispositivos USB conectados&lt;/b&gt;&lt;br/&gt;</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="53"/>
        <source>VID: %1 PID: %2, %3</source>
        <translation>VID: %1 PID: %2, %3</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="62"/>
        <source>Filesystem</source>
        <translation>Sistema de Ficheiros</translation>
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
        <translation>Informação do sistema</translation>
    </message>
    <message>
        <location filename="../sysinfofrm.ui" line="22"/>
        <source>&amp;Refresh</source>
        <translation>Actualizar</translation>
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
        <translation type="unfinished">Convidado</translation>
    </message>
    <message>
        <location filename="../base/system.cpp" line="123"/>
        <source>Admin</source>
        <translation type="unfinished">Administrador</translation>
    </message>
    <message>
        <location filename="../base/system.cpp" line="126"/>
        <source>User</source>
        <translation type="unfinished">Utilizador</translation>
    </message>
    <message>
        <location filename="../base/system.cpp" line="129"/>
        <source>Error</source>
        <translation type="unfinished">Erro</translation>
    </message>
    <message>
        <location filename="../base/system.cpp" line="277"/>
        <location filename="../base/system.cpp" line="322"/>
        <source>(no description available)</source>
        <translation type="unfinished">(sem descrição disponível)</translation>
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
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/ttscarbon.cpp" line="144"/>
        <source>Speed (words/min):</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/ttscarbon.cpp" line="151"/>
        <source>Pitch (0 for default):</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/ttscarbon.cpp" line="221"/>
        <source>Could not voice string</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/ttscarbon.cpp" line="231"/>
        <source>Could not convert intermediate file</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>TTSExes</name>
    <message>
        <location filename="../base/ttsexes.cpp" line="77"/>
        <source>TTS executable not found</source>
        <translation>Executável TTS não encontrado</translation>
    </message>
    <message>
        <location filename="../base/ttsexes.cpp" line="42"/>
        <source>Path to TTS engine:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/ttsexes.cpp" line="44"/>
        <source>TTS engine options:</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>TTSFestival</name>
    <message>
        <location filename="../base/ttsfestival.cpp" line="201"/>
        <source>engine could not voice string</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/ttsfestival.cpp" line="284"/>
        <source>No description available</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/ttsfestival.cpp" line="49"/>
        <source>Path to Festival client:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/ttsfestival.cpp" line="54"/>
        <source>Voice:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/ttsfestival.cpp" line="63"/>
        <source>Voice description:</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>TTSSapi</name>
    <message>
        <location filename="../base/ttssapi.cpp" line="43"/>
        <source>Language:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/ttssapi.cpp" line="49"/>
        <source>Voice:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/ttssapi.cpp" line="59"/>
        <source>Speed:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/ttssapi.cpp" line="62"/>
        <source>Options:</source>
        <translation type="unfinished"></translation>
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
        <translation>Iniciando geração dos Ficheiros de Leitura</translation>
    </message>
    <message>
        <location filename="../base/talkfile.cpp" line="40"/>
        <source>Reading Filelist...</source>
        <translation>Lendo lista de ficheiros...</translation>
    </message>
    <message>
        <location filename="../base/talkfile.cpp" line="43"/>
        <source>Talk file creation aborted</source>
        <translation>Criação dos Ficheiros de Leitura abortada</translation>
    </message>
    <message>
        <location filename="../base/talkfile.cpp" line="66"/>
        <source>Copying Talkfiles...</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/talkfile.cpp" line="229"/>
        <source>File copy aborted</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/talkfile.cpp" line="268"/>
        <source>Cleaning up...</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/talkfile.cpp" line="279"/>
        <source>Finished</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/talkfile.cpp" line="78"/>
        <source>Finished creating Talk files</source>
        <translation>Terminado a criação dos ficherios de Leitura</translation>
    </message>
    <message>
        <location filename="../base/talkfile.cpp" line="247"/>
        <source>Copying of %1 to %2 failed</source>
        <translation>Cópia de %1 para %2 falhou</translation>
    </message>
</context>
<context>
    <name>TalkGenerator</name>
    <message>
        <location filename="../base/talkgenerator.cpp" line="38"/>
        <source>Starting TTS Engine</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="43"/>
        <source>Init of TTS engine failed</source>
        <translation type="unfinished">Falha na inicialização do motor TTS</translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="50"/>
        <source>Starting Encoder Engine</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="54"/>
        <source>Init of Encoder engine failed</source>
        <translation type="unfinished">Falha na inicialização do motor codificador</translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="64"/>
        <source>Voicing entries...</source>
        <translation type="unfinished">Entradas de vocalização...</translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="79"/>
        <source>Encoding files...</source>
        <translation type="unfinished">Codificando ficheiros...</translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="118"/>
        <source>Voicing aborted</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="154"/>
        <location filename="../base/talkgenerator.cpp" line="159"/>
        <source>Voicing of %1 failed: %2</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="203"/>
        <source>Encoding aborted</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="230"/>
        <source>Encoding of %1 failed</source>
        <translation type="unfinished">Codificação de %1 falhou</translation>
    </message>
</context>
<context>
    <name>ThemeInstallFrm</name>
    <message>
        <location filename="../themesinstallfrm.ui" line="13"/>
        <source>Theme Installation</source>
        <translation>Instalação de Tema</translation>
    </message>
    <message>
        <location filename="../themesinstallfrm.ui" line="48"/>
        <source>Selected Theme</source>
        <translation>Tema Seleccionado</translation>
    </message>
    <message>
        <location filename="../themesinstallfrm.ui" line="73"/>
        <source>Description</source>
        <translation>Descrição</translation>
    </message>
    <message>
        <location filename="../themesinstallfrm.ui" line="83"/>
        <source>Download size:</source>
        <translation>Tamanho da transferência:</translation>
    </message>
    <message>
        <location filename="../themesinstallfrm.ui" line="93"/>
        <source>Hold Ctrl to select multiple item, Shift for a range</source>
        <translation>Pressiona Ctrl para seleccionar múltiplos itens, Shift para um intervalo</translation>
    </message>
    <message>
        <location filename="../themesinstallfrm.ui" line="115"/>
        <source>&amp;Install</source>
        <translation>&amp;Instalar</translation>
    </message>
    <message>
        <location filename="../themesinstallfrm.ui" line="125"/>
        <source>&amp;Cancel</source>
        <translation>&amp;Cancelar</translation>
    </message>
</context>
<context>
    <name>ThemesInstallWindow</name>
    <message>
        <location filename="../themesinstallwindow.cpp" line="38"/>
        <source>no theme selected</source>
        <translation>nehum tema seleccionado</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="110"/>
        <source>Network error: %1.
Please check your network and proxy settings.</source>
        <translation>Erro de rede:%1
Por favor, verifique as suas definições de rede e do proxy.</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="123"/>
        <source>the following error occured:
%1</source>
        <translation>o erro seguinte ocorreu:
%1</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="129"/>
        <source>done.</source>
        <translation>completo.</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="196"/>
        <source>fetching details for %1</source>
        <translation>transferindo detalhes para %1</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="199"/>
        <source>fetching preview ...</source>
        <translation>transferindo pré-visualização ...</translation>
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
        <translation>desconhecido</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="214"/>
        <source>&lt;b&gt;Version:&lt;/b&gt; %1&lt;hr/&gt;</source>
        <translation>&lt;b&gt;Versão:&lt;/b&gt; %1&lt;hr/&gt;</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="216"/>
        <source>&lt;b&gt;Description:&lt;/b&gt; %1&lt;hr/&gt;</source>
        <translation>&lt;b&gt;Descrição:&lt;/b&gt; %1&lt;hr/&gt;</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="217"/>
        <source>no description</source>
        <translation>sem descrição</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="260"/>
        <source>no theme preview</source>
        <translation>sem pré-visualização do tema</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="291"/>
        <source>getting themes information ...</source>
        <translation>transferindo informação dos temas ...</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="339"/>
        <source>Mount point is wrong!</source>
        <translation>Ponto de omntagem está incorrecto!</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="39"/>
        <source>no selection</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="166"/>
        <source>Information</source>
        <translation type="unfinished"></translation>
    </message>
    <message numerus="yes">
        <location filename="../themesinstallwindow.cpp" line="183"/>
        <source>Download size %L1 kiB (%n item(s))</source>
        <translation type="unfinished">
            <numerusform></numerusform>
            <numerusform></numerusform>
        </translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="248"/>
        <source>Retrieving theme preview failed.
HTTP response code: %1</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>UninstallFrm</name>
    <message>
        <location filename="../uninstallfrm.ui" line="16"/>
        <source>Uninstall Rockbox</source>
        <translation></translation>
    </message>
    <message>
        <location filename="../uninstallfrm.ui" line="35"/>
        <source>Please select the Uninstallation Method</source>
        <translation>Por favor seleccione o Método de Desinstalação</translation>
    </message>
    <message>
        <location filename="../uninstallfrm.ui" line="45"/>
        <source>Uninstallation Method</source>
        <translation>Método de desinstalação</translation>
    </message>
    <message>
        <location filename="../uninstallfrm.ui" line="51"/>
        <source>Complete Uninstallation</source>
        <translation>Desinstalação Completa</translation>
    </message>
    <message>
        <location filename="../uninstallfrm.ui" line="58"/>
        <source>Smart Uninstallation</source>
        <translation>Desinstalação Inteligente</translation>
    </message>
    <message>
        <location filename="../uninstallfrm.ui" line="68"/>
        <source>Please select what you want to uninstall</source>
        <translation>Por favor seleccione o que deseja desinstalar</translation>
    </message>
    <message>
        <location filename="../uninstallfrm.ui" line="78"/>
        <source>Installed Parts</source>
        <translation>Partes Instaladas</translation>
    </message>
    <message>
        <location filename="../uninstallfrm.ui" line="128"/>
        <source>&amp;Uninstall</source>
        <translation>Deinstala&amp;ção</translation>
    </message>
    <message>
        <location filename="../uninstallfrm.ui" line="138"/>
        <source>&amp;Cancel</source>
        <translation>&amp;Cancelar</translation>
    </message>
</context>
<context>
    <name>Uninstaller</name>
    <message>
        <location filename="../base/uninstall.cpp" line="31"/>
        <location filename="../base/uninstall.cpp" line="42"/>
        <source>Starting Uninstallation</source>
        <translation>Iniciando Desinstalação</translation>
    </message>
    <message>
        <location filename="../base/uninstall.cpp" line="35"/>
        <source>Finished Uninstallation</source>
        <translation>Desinstalação terminada</translation>
    </message>
    <message>
        <location filename="../base/uninstall.cpp" line="48"/>
        <source>Uninstalling %1...</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/uninstall.cpp" line="79"/>
        <source>Could not delete %1</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/uninstall.cpp" line="108"/>
        <source>Uninstallation finished</source>
        <translation>Desinstalação terminada</translation>
    </message>
</context>
<context>
    <name>Utils</name>
    <message>
        <location filename="../base/utils.cpp" line="309"/>
        <source>&lt;li&gt;Permissions insufficient for bootloader installation.
Administrator priviledges are necessary.&lt;/li&gt;</source>
        <translation type="unfinished">&lt;li&gt;Permissões insuficientes para a instalação da rotina de arranque
Previlégios de administrador são necessários.&lt;/li&gt;</translation>
    </message>
    <message>
        <location filename="../base/utils.cpp" line="321"/>
        <source>&lt;li&gt;Target mismatch detected.&lt;br/&gt;Installed target: %1&lt;br/&gt;Selected target: %2.&lt;/li&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/utils.cpp" line="328"/>
        <source>Problem detected:</source>
        <translation type="unfinished">Problema detectado:</translation>
    </message>
</context>
<context>
    <name>VoiceFileCreator</name>
    <message>
        <location filename="../base/voicefile.cpp" line="40"/>
        <source>Starting Voicefile generation</source>
        <translation>Iniciando geração dos ficheiros de voz</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="85"/>
        <source>Downloading voice info...</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="98"/>
        <source>Download error: received HTTP error %1.</source>
        <translation>Erro de transferência: recebido erro de HTTP %1.</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="104"/>
        <source>Cached file used.</source>
        <translation>Ficheiro da cache usado.</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="107"/>
        <source>Download error: %1</source>
        <translation>Erro de transferência: %1</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="112"/>
        <source>Download finished.</source>
        <translation>Tranferência terminada.</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="120"/>
        <source>failed to open downloaded file</source>
        <translation>falha ao abrir ficheiro transferido</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="128"/>
        <source>Reading strings...</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="200"/>
        <source>Creating voicefiles...</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="245"/>
        <source>Cleaning up...</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="256"/>
        <source>Finished</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="174"/>
        <source>The downloaded file was empty!</source>
        <translation>O ficheiro transferido estava vazio!</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="205"/>
        <source>Error opening downloaded file</source>
        <translation>Erro ao abrir ficheiro transferido</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="216"/>
        <source>Error opening output file</source>
        <translation>Erro ao abrir o ficheiro de saída</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="236"/>
        <source>successfully created.</source>
        <translation>criado com sucesso.</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="53"/>
        <source>could not find rockbox-info.txt</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>ZipInstaller</name>
    <message>
        <location filename="../base/zipinstaller.cpp" line="58"/>
        <source>done.</source>
        <translation>completo.</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="66"/>
        <source>Installation finished successfully.</source>
        <translation>Instalação terminada com sucesso.</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="79"/>
        <source>Downloading file %1.%2</source>
        <translation>Transferindo  ficheiro %1.%2</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="113"/>
        <source>Download error: received HTTP error %1.</source>
        <translation>Erro de transferência: recebido erro de HTTP %1.</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="119"/>
        <source>Cached file used.</source>
        <translation>Ficheiro de cache usado.</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="121"/>
        <source>Download error: %1</source>
        <translation>Erro de transferência: %1</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="125"/>
        <source>Download finished.</source>
        <translation>Tranferência terminada.</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="131"/>
        <source>Extracting file.</source>
        <translation>Extraindo ficheiro.</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="151"/>
        <source>Extraction failed!</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="144"/>
        <source>Not enough disk space! Aborting.</source>
        <translation>Sem espaço suficiente em disco! A abortar.</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="160"/>
        <source>Installing file.</source>
        <translation>instanlando ficheiro.</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="171"/>
        <source>Installing file failed.</source>
        <translation>Falha na instalação do ficheiro.</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="180"/>
        <source>Creating installation log</source>
        <translation>Criando registo da instalação</translation>
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
        <translation>Sobre Rockbox Utility</translation>
    </message>
    <message>
        <location filename="../aboutbox.ui" line="32"/>
        <source>The Rockbox Utility</source>
        <translation>A Rockbox Utility</translation>
    </message>
    <message utf8="true">
        <location filename="../aboutbox.ui" line="54"/>
        <source>Installer and housekeeping utility for the Rockbox open source digital audio player firmware.&lt;br/&gt;© 2005 - 2012 The Rockbox Team.&lt;br/&gt;Released under the GNU General Public License v2.&lt;br/&gt;Uses icons by the &lt;a href=&quot;http://tango.freedesktop.org/&quot;&gt;Tango Project&lt;/a&gt;.&lt;br/&gt;&lt;center&gt;&lt;a href=&quot;http://www.rockbox.org&quot;&gt;http://www.rockbox.org&lt;/a&gt;&lt;/center&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../aboutbox.ui" line="74"/>
        <source>&amp;Credits</source>
        <translation>&amp;Créditos</translation>
    </message>
    <message>
        <location filename="../aboutbox.ui" line="106"/>
        <source>&amp;License</source>
        <translation>&amp;Licença</translation>
    </message>
    <message>
        <location filename="../aboutbox.ui" line="132"/>
        <source>&amp;Speex License</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../aboutbox.ui" line="158"/>
        <source>&amp;Ok</source>
        <translation>&amp;Ok</translation>
    </message>
</context>
</TS>
