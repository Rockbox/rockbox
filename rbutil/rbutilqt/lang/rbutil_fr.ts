<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.0" language="fr">
<context>
    <name>BootloaderInstallAms</name>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="33"/>
        <source>Bootloader installation requires you to provide a firmware file of the original firmware (bin file). You need to download this file yourself due to legal reasons. Please browse the &lt;a href=&apos;http://forums.sandisk.com/sansa/&apos;&gt;Sansa Forums&apos;&lt;/a&gt; or refer to the &lt;a href=&apos;http://www.rockbox.org/manual.shtml&apos;&gt;manual&lt;/a&gt; and the &lt;a href=&apos;http://www.rockbox.org/wiki/SansaAMS&apos;&gt;SansaAMS&lt;/a&gt; wiki page on how to obtain this file.&lt;br/&gt;Press Ok to continue and browse your computer for the firmware file.</source>
        <translation>L&apos;installation du bootloader nécessite que vous fournissiez le firmware originel (fichier bin). Vous devez le télécharger vous même pour des raisons légales. Veuillez vous référer aux &lt;a href=&apos;http://forums.sandisk.com/sansa/&apos;&gt;Forums Sansa&lt;/a&gt; ou au &lt;a href=&apos;http://www.rockbox.org/manual.shtml&apos;&gt;manuel&lt;/a&gt; et à la page wiki &lt;a href=&apos;http://www.rockbox.org/wiki/SansaAMS&apos;&gt;SansaAMS&lt;/a&gt; pour obtenir ce fichier. Appuyez sur Ok pour continuer and sélectionner le fichier du firmware.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="54"/>
        <source>Downloading bootloader file</source>
        <translation>Téléchargement du bootloader</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="94"/>
        <location filename="../base/bootloaderinstallams.cpp" line="107"/>
        <source>Could not load %1</source>
        <translation>Impossible de charger %1</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="122"/>
        <source>No room to insert bootloader, try another firmware version</source>
        <translation>Il n&apos;y a pas assez de place pour insérer le bootloader, veuillez essayer avec une autre version du firmware</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="132"/>
        <source>Patching Firmware...</source>
        <translation>Modification du firmware...</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="143"/>
        <source>Could not open %1 for writing</source>
        <translation>Impossible d&apos;ouvrir %1 en écriture</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="156"/>
        <source>Could not write firmware file</source>
        <translation>Impossible d&apos;écrire le firmware</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="172"/>
        <source>Success: modified firmware file created</source>
        <translation>Le firmware a été modifié avec succès</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallams.cpp" line="180"/>
        <source>To uninstall, perform a normal upgrade with an unmodified original firmware</source>
        <translation>Pour le désinstaller, faites une mise à jour normale avec un firmware non modifié</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallBase</name>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="75"/>
        <source>Download error: received HTTP error %1.</source>
        <translation>Erreur lors du téléchargement: erreur HTTP %1.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="81"/>
        <source>Download error: %1</source>
        <translation>Erreur lors du téléchargement : %1</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="87"/>
        <source>Download finished (cache used).</source>
        <translation>Téléchargement terminé (la version du cache a été utilisée).</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="89"/>
        <source>Download finished.</source>
        <translation>Téléchargement terminé.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="110"/>
        <source>Creating backup of original firmware file.</source>
        <translation>Création d&apos;un sauvegarde du firmware.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="112"/>
        <source>Creating backup folder failed</source>
        <translation>La création du dossier de sauvegarde a échoué</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="118"/>
        <source>Creating backup copy failed.</source>
        <translation>La création de la sauvegarde a échoué.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="121"/>
        <source>Backup created.</source>
        <translation>Sauvegarde effectuée.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="134"/>
        <source>Creating installation log</source>
        <translation>Création du journal d&apos;installation</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="158"/>
        <source>Bootloader installation is almost complete. Installation &lt;b&gt;requires&lt;/b&gt; you to perform the following steps manually:</source>
        <translation>L&apos;installation du bootloader a presque complète. Vous &lt;b&gt;devez&lt;/b&gt; réaliser les étapes suivantes manuellement:</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="163"/>
        <source>&lt;li&gt;Safely remove your player.&lt;/li&gt;</source>
        <translation>&lt;li&gt;Débranchez votre lecteur audio en toute sécurité.&lt;/li&gt;</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="167"/>
        <source>&lt;li&gt;Reboot your player into the original firmware.&lt;/li&gt;&lt;li&gt;Perform a firmware upgrade using the update functionality of the original firmware. Please refer to your player&apos;s manual on details.&lt;/li&gt;&lt;li&gt;After the firmware has been updated reboot your player.&lt;/li&gt;</source>
        <translation>&lt;li&gt;Redémarrez votre lecteur audio dans le firmware originel.&lt;/li&gt;&lt;li&gt;Faites une mise à jour de celui-ci. Veuillez vous référer au manuel de votre lecteur audio pour plus d&apos;informations.&lt;/li&gt;&lt;li&gt;Une fois le firmware mis à jour, redémarrez votre lecteur audio.&lt;/li&gt;</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="176"/>
        <source>&lt;li&gt;Turn the player off&lt;/li&gt;&lt;li&gt;Insert the charger&lt;/li&gt;</source>
        <translation>&lt;li&gt;Éteignez votre lecteur audio&lt;/li&gt;&lt;li&gt;Insérer le chargeur&lt;/li&gt;</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="181"/>
        <source>&lt;li&gt;Unplug USB and power adaptors&lt;/li&gt;&lt;li&gt;Hold &lt;i&gt;Power&lt;/i&gt; to turn the player off&lt;/li&gt;&lt;li&gt;Toggle the battery switch on the player&lt;/li&gt;&lt;li&gt;Hold &lt;i&gt;Power&lt;/i&gt; to boot into Rockbox&lt;/li&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="188"/>
        <source>&lt;p&gt;&lt;b&gt;Note:&lt;/b&gt; You can safely install other parts first, but the above steps are &lt;b&gt;required&lt;/b&gt; to finish the installation!&lt;/p&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="202"/>
        <source>Waiting for system to remount player</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="232"/>
        <source>Player remounted</source>
        <translation>Le lecteur audio a été remonté</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="237"/>
        <source>Timeout on remount</source>
        <translation>Le délai d&apos;attente lors du remontage a expiré</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallbase.cpp" line="146"/>
        <source>Installation log created</source>
        <translation>Le journal d&apos;installation a été crée</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallChinaChip</name>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="34"/>
        <source>Bootloader installation requires you to provide a firmware file of the original firmware (HXF file). You need to download this file yourself due to legal reasons. Please refer to the &lt;a href=&apos;http://www.rockbox.org/manual.shtml&apos;&gt;manual&lt;/a&gt; and the &lt;a href=&apos;http://www.rockbox.org/wiki/OndaVX747#Download_and_extract_a_recent_ve&apos;&gt;OndaVX747&lt;/a&gt; wiki page on how to obtain this file.&lt;br/&gt;Press Ok to continue and browse your computer for the firmware file.</source>
        <translation>L&apos;installation du bootloader nécessite que vous fournissiez un fichier contenant le firmware originel (fichier HXF). Vous devez télécharger ce fichier vous-même pour des raisons légales. Veuillez vous référer au &lt;a href=&apos;http://www.rockbox.org/manual.shtml&apos;&gt;manuel&lt;/a&gt; et à la page wiki &lt;a href=&apos;http://www.rockbox.org/wiki/OndaVX747#Download_and_extract_a_recent_ve&apos;&gt;OndaVX747&lt;/a&gt; pour obtenir ce fichier.&lt;br/&gt;Appuyez sur Ok pour continuer et parcourir les fichiers votre ordinateur pour indiquer le fichier du firmware.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallchinachip.cpp" line="79"/>
        <source>Downloading bootloader file</source>
        <translation>Téléchargement du bootloader</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallFile</name>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="35"/>
        <source>Downloading bootloader</source>
        <translation>Téléchargement du bootloader</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="44"/>
        <source>Installing Rockbox bootloader</source>
        <translation>Installation du bootloader de Rockbox</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="76"/>
        <source>Error accessing output folder</source>
        <translation>Erreur lors de l&apos;accès au dossier de sortie</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="89"/>
        <source>Bootloader successful installed</source>
        <translation>Le bootloader a été installé avec succès</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="99"/>
        <source>Removing Rockbox bootloader</source>
        <translation>Suppression du bootloader de Rockbox</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="103"/>
        <source>No original firmware file found.</source>
        <translation>Aucun firmware originel n&apos;a pas été trouvé.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="109"/>
        <source>Can&apos;t remove Rockbox bootloader file.</source>
        <translation>Impossible de supprime le bootloader de Rockbox.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="114"/>
        <source>Can&apos;t restore bootloader file.</source>
        <translation>Impossible de restaurer le bootloader.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallfile.cpp" line="118"/>
        <source>Original bootloader restored successfully.</source>
        <translation>Le bootloadezr originel a été restauré avec succès.</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallHex</name>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="68"/>
        <source>checking MD5 hash of input file ...</source>
        <translation>vérification de l&apos;empreinte MD5 du fichier d&apos;entrée...</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="79"/>
        <source>Could not verify original firmware file</source>
        <translation>Impossible de vérifier le firmware originel</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="94"/>
        <source>Firmware file not recognized.</source>
        <translation>Le firmware n&apos;a pas été reconnu.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="98"/>
        <source>MD5 hash ok</source>
        <translation>L&apos;empreinte MD5 est correcte</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="105"/>
        <source>Firmware file doesn&apos;t match selected player.</source>
        <translation>Le firmware ne correspond pas au lecteur audio choisi.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="110"/>
        <source>Descrambling file</source>
        <translation>Débrouillage du fichier</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="118"/>
        <source>Error in descramble: %1</source>
        <translation>Error lors du débrouillage : %1</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="123"/>
        <source>Downloading bootloader file</source>
        <translation>Téléchargement du bootloader</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="133"/>
        <source>Adding bootloader to firmware file</source>
        <translation>Ajout du bootloader au firmware</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="171"/>
        <source>could not open input file</source>
        <translation>impossible d&apos;ouvrir le fichier d&apos;entrée</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="172"/>
        <source>reading header failed</source>
        <translation>la lecture de l&apos;en-tête a échoué</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="173"/>
        <source>reading firmware failed</source>
        <translation>la lecture du firmware a échoué</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="174"/>
        <source>can&apos;t open bootloader file</source>
        <translation>impossible d&apos;ouvrir le fichier de bootloader</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="175"/>
        <source>reading bootloader file failed</source>
        <translation>la lecture du fichier du bootloader a échoué</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="176"/>
        <source>can&apos;t open output file</source>
        <translation>impossible d&apos;ouvrir le fichier de sortie</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="177"/>
        <source>writing output file failed</source>
        <translation>l&apos;écriture du fichier de sortie a échoué</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="179"/>
        <source>Error in patching: %1</source>
        <translation>Erreur lors de la modification : %1</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="190"/>
        <source>Error in scramble: %1</source>
        <translation>Erreur lors du brouillage: %1</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="205"/>
        <source>Checking modified firmware file</source>
        <translation>Vérification du firmware modifié</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="207"/>
        <source>Error: modified file checksum wrong</source>
        <translation>Erreur: la somme de contrôle du fichier est incorrecte</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="215"/>
        <source>Success: modified firmware file created</source>
        <translation>Le firmware a été modifié avec succès</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="246"/>
        <source>Can&apos;t open input file</source>
        <translation>Impossible d&apos;ouvrir le fichier d&apos;entrée</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="247"/>
        <source>Can&apos;t open output file</source>
        <translation>Impossible d&apos;ouvrir le fichier de sortie</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="248"/>
        <source>invalid file: header length wrong</source>
        <translation>fichier invalide : longueur de l&apos;en-tête incorrecte</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="249"/>
        <source>invalid file: unrecognized header</source>
        <translation>fichier invalide : en-tête non reconnu</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="250"/>
        <source>invalid file: &quot;length&quot; field wrong</source>
        <translation>fichier invalide : champ &quot;longueur&quot; erroné</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="251"/>
        <source>invalid file: &quot;length2&quot; field wrong</source>
        <translation>fichier invalide : champ &quot;longueur2&quot; erroné</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="252"/>
        <source>invalid file: internal checksum error</source>
        <translation>fichier invalide : erreur interne de somme de contrôle</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="253"/>
        <source>invalid file: &quot;length3&quot; field wrong</source>
        <translation>fichier invalide : champ &quot;longueur3&quot; erroné</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="254"/>
        <source>unknown</source>
        <translation>inconnue</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallhex.cpp" line="49"/>
        <source>Bootloader installation requires you to provide a firmware file of the original firmware (hex file). You need to download this file yourself due to legal reasons. Please refer to the &lt;a href=&apos;http://www.rockbox.org/manual.shtml&apos;&gt;manual&lt;/a&gt; and the &lt;a href=&apos;http://www.rockbox.org/wiki/IriverBoot#Download_and_extract_a_recent_ve&apos;&gt;IriverBoot&lt;/a&gt; wiki page on how to obtain this file.&lt;br/&gt;Press Ok to continue and browse your computer for the firmware file.</source>
        <translation>L&apos;installation du bootloader nécessite que vous fournissiez un fichier contenant le firmware originel (fichier hex). Vous devez télécharger ce fichier vous-même pour des raisons légales. Veuillez vous référer au &lt;a href=&apos;http://www.rockbox.org/manual.shtml&apos;&gt;manuel&lt;/a&gt; et à la page wiki &lt;a href=&apos;http://www.rockbox.org/wiki/IriverBoot#Download_and_extract_a_recent_ve&apos;&gt;IriverBoot&lt;/a&gt; pour obtenir ce fichier.&lt;br/&gt;Appuyez sur Ok pour continuer et parcourir les fichiers votre ordinateur pour indiquer le fichier du firmware.</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallIpod</name>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="54"/>
        <source>Error: can&apos;t allocate buffer memory!</source>
        <translation>Erreur: impossible d&apos;allouer de la mémoire !</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="82"/>
        <source>Downloading bootloader file</source>
        <translation>Téléchargement du bootloader</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="66"/>
        <location filename="../base/bootloaderinstallipod.cpp" line="153"/>
        <source>Failed to read firmware directory</source>
        <translation>La lecture du dossier du firmware a échoué</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="71"/>
        <location filename="../base/bootloaderinstallipod.cpp" line="158"/>
        <source>Unknown version number in firmware (%1)</source>
        <translation>Numéro de version inconnu dans le firmware (%1)</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="77"/>
        <source>Warning: This is a MacPod, Rockbox only runs on WinPods. 
See http://www.rockbox.org/wiki/IpodConversionToFAT32</source>
        <translatorcomment>C&apos;est techniquement incorrect mais je ne sais pas trop comment le dire sinon.</translatorcomment>
        <translation>Attention: le lecteur utilise un système de fichier MacPod, Rockbox ne peut fonctionner qu&apos;avec un système de fichier WinPods.
Veuillez vous référer à http://www.rockbox.org/wiki/IpodConversionToFAT32</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="96"/>
        <location filename="../base/bootloaderinstallipod.cpp" line="165"/>
        <source>Could not open Ipod in R/W mode</source>
        <translation>Impossible d&apos;accéder à l&apos;Ipod en lecture et écriture</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="106"/>
        <source>Successfull added bootloader</source>
        <translation>Le bootloader a été ajouté avec succès</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="117"/>
        <source>Failed to add bootloader</source>
        <translation>L&apos;ajout du bootloader a échoué</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="129"/>
        <source>Bootloader Installation complete.</source>
        <translation>L&apos;installation du bootloader est terminée.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="134"/>
        <source>Writing log aborted</source>
        <translation>L&apos;écriture de l&apos;historique a été interrompue</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="171"/>
        <source>No bootloader detected.</source>
        <translation>Aucun bootloader détecté.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="177"/>
        <source>Successfully removed bootloader</source>
        <translation>Le bootloader a été supprimé avec succès</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="184"/>
        <source>Removing bootloader failed.</source>
        <translation>La suppression du bootloader a échoué.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="245"/>
        <source>Error: no mountpoint specified!</source>
        <translation>Erreur: aucun point de montage n&apos;a été spécifié !</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="250"/>
        <source>Could not open Ipod: permission denied</source>
        <translation>Impossible d&apos;accéder à l&apos;Ipod: accès refusé</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="254"/>
        <source>Could not open Ipod</source>
        <translation>Impossible d&apos;accéder à l&apos;Ipod</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="265"/>
        <source>No firmware partition on disk</source>
        <translation>La partition du firmware n&apos;est pas présente sur le disque</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="92"/>
        <source>Installing Rockbox bootloader</source>
        <translation>Installation du bootloader de Rockbox</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="144"/>
        <source>Uninstalling bootloader</source>
        <translation>Suppression du bootloader</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallipod.cpp" line="259"/>
        <source>Error reading partition table - possibly not an Ipod</source>
        <translation>Erreur lors de la lecteur de la table des partitions; ce lecteur audio n&apos;est peut-être pas un Ipod</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallMi4</name>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="34"/>
        <source>Downloading bootloader</source>
        <translation>Téléchargement du bootloader</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="43"/>
        <source>Installing Rockbox bootloader</source>
        <translation>Installation du bootloader de Rockbox</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="66"/>
        <source>Bootloader successful installed</source>
        <translation>Le bootloader a été installé avec succès</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="78"/>
        <source>Checking for Rockbox bootloader</source>
        <translation>Vérification du bootloader de Rockbox</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="80"/>
        <source>No Rockbox bootloader found</source>
        <translation>Le bootloader de Rockbox n&apos;a pas été trouvé</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="85"/>
        <source>Checking for original firmware file</source>
        <translation>Vérification du firmware originel</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="90"/>
        <source>Error finding original firmware file</source>
        <translation>Erreur lors de la rechercher du firmware originel</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallmi4.cpp" line="100"/>
        <source>Rockbox bootloader successful removed</source>
        <translation>Le bootloader de Rockbox a été supprimé avec succès</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallSansa</name>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="56"/>
        <source>Error: can&apos;t allocate buffer memory!</source>
        <translation>Erreur: impossible d&apos;allouer de la mémoire !</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="61"/>
        <source>Searching for Sansa</source>
        <translation>Recherche de lecteurs Sansa</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="67"/>
        <source>Permission for disc access denied!
This is required to install the bootloader</source>
        <translation>L&apos;accès au disque a été refusé !
Ceci est nécessaire à l&apos;installation du bootloader</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="74"/>
        <source>No Sansa detected!</source>
        <translation>Aucun lecteur Sansa n&apos;a été détecté !</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="87"/>
        <source>Downloading bootloader file</source>
        <translation>Téléchargement du bootloader</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="79"/>
        <location filename="../base/bootloaderinstallsansa.cpp" line="187"/>
        <source>OLD ROCKBOX INSTALLATION DETECTED, ABORTING.
You must reinstall the original Sansa firmware before running
sansapatcher for the first time.
See http://www.rockbox.org/wiki/SansaE200Install
</source>
        <translation>UNE ANCIENNE INSTALLATION DE ROCKBOX A ÉTÉ DÉTECTÉE, ABANDON.
Vous devez réinstaller le firmware originel de Sansa avant le lancer
sansapatcher pour la première fois.
Veuillez vous référer à http://www.rockbox.org/wiki/SansaE200Install
</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="109"/>
        <location filename="../base/bootloaderinstallsansa.cpp" line="197"/>
        <source>Could not open Sansa in R/W mode</source>
        <translation>Impossible d&apos;accéder au lecteur Sansa en lecture et écriture</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="136"/>
        <source>Successfully installed bootloader</source>
        <translation>Le bootloader a été installé avec succès</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="147"/>
        <source>Failed to install bootloader</source>
        <translation>L&apos;installation du bootloader a échoué</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="160"/>
        <source>Bootloader Installation complete.</source>
        <translation>L&apos;installation du bootloader est terminée.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="165"/>
        <source>Writing log aborted</source>
        <translation>L&apos;écriture de l&apos;historique a été interrompue</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="259"/>
        <source>Can&apos;t find Sansa</source>
        <translation>Impossible de trouver un lecteur Sansa</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="264"/>
        <source>Could not open Sansa</source>
        <translation>Impossible d&apos;accéder au lecteur Sansa</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="269"/>
        <source>Could not read partition table</source>
        <translation>Impossible de lire la table des partitions</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="276"/>
        <source>Disk is not a Sansa (Error %1), aborting.</source>
        <translation>Le disque n&apos;appartient pas à un lecteur Sansa (Erreur %1); abandon.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="203"/>
        <source>Successfully removed bootloader</source>
        <translation>Le bootloader a été supprimé averc succès</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="210"/>
        <source>Removing bootloader failed.</source>
        <translation>La suppression du bootloader a échoué.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="101"/>
        <source>Installing Rockbox bootloader</source>
        <translation>Installation du bootloader de Rockbox</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="118"/>
        <source>Checking downloaded bootloader</source>
        <translation>Vérification du bootloader téléchargé</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="126"/>
        <source>Bootloader mismatch! Aborting.</source>
        <translation>Le bootloader ne correspond pas ! Abandon.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstallsansa.cpp" line="178"/>
        <source>Uninstalling bootloader</source>
        <translation>Suppression du bootloader</translation>
    </message>
</context>
<context>
    <name>BootloaderInstallTcc</name>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="34"/>
        <source>Bootloader installation requires you to provide a firmware file of the original firmware (bin file). You need to download this file yourself due to legal reasons. Please refer to the &lt;a href=&apos;http://www.rockbox.org/manual.shtml&apos;&gt;manual&lt;/a&gt; and the &lt;a href=&apos;http://www.rockbox.org/wiki/CowonD2Info&apos;&gt;CowonD2Info&lt;/a&gt; wiki page on how to obtain the file.&lt;br/&gt;Press Ok to continue and browse your computer for the firmware file.</source>
        <translation>L&apos;installation du bootloader nécessite que vous fournissiez un fichier contenant le firmware originel (fichier hin). Vous devez télécharger ce fichier vous-même pour des raisons légales. Veuillez vous référer au &lt;a href=&apos;http://www.rockbox.org/manual.shtml&apos;&gt;manuel&lt;/a&gt; et à la page wiki &lt;a href=&apos;http://www.rockbox.org/wiki/CowonD2Info&apos;&gt;CowonD2Info&lt;/a&gt; pour obtenir ce fichier.&lt;br/&gt;Appuyez sur Ok pour continuer et parcourir les fichiers votre ordinateur pour indiquer le fichier du firmware.</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="51"/>
        <source>Downloading bootloader file</source>
        <translation>Téléchargement du bootloader</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="83"/>
        <location filename="../base/bootloaderinstalltcc.cpp" line="100"/>
        <source>Could not load %1</source>
        <translation>Impossible de charger %1</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="91"/>
        <source>Unknown OF file used: %1</source>
        <translation>Le fichier du firmware originel n&apos;est pas valide : %1</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="105"/>
        <source>Patching Firmware...</source>
        <translation>Modification du firmware...</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="112"/>
        <source>Could not patch firmware</source>
        <translation>Impossible de modifier le firmware</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="118"/>
        <source>Could not open %1 for writing</source>
        <translation>Impossible d&apos;ouvrir %1 en écriture</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="127"/>
        <source>Could not write firmware file</source>
        <translation>Impossible d&apos;écriture le firmware</translation>
    </message>
    <message>
        <location filename="../base/bootloaderinstalltcc.cpp" line="132"/>
        <source>Success: modified firmware file created</source>
        <translation>Le firmware a été modifié avec succès</translation>
    </message>
</context>
<context>
    <name>BrowseDirtreeFrm</name>
    <message>
        <location filename="../browsedirtreefrm.ui" line="13"/>
        <source>Find Directory</source>
        <translation>Ouverture du répertoire</translation>
    </message>
    <message>
        <location filename="../browsedirtreefrm.ui" line="19"/>
        <source>Browse to the destination folder</source>
        <translation>Indiquez le répertoire de destination</translation>
    </message>
    <message>
        <location filename="../browsedirtreefrm.ui" line="47"/>
        <source>&amp;Ok</source>
        <translation>&amp;OK</translation>
    </message>
    <message>
        <location filename="../browsedirtreefrm.ui" line="57"/>
        <source>&amp;Cancel</source>
        <translation>&amp;Annuler</translation>
    </message>
</context>
<context>
    <name>Config</name>
    <message>
        <location filename="../configure.cpp" line="135"/>
        <source>Language changed</source>
        <translation>Changement de langue</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="136"/>
        <source>You need to restart the application for the changed language to take effect.</source>
        <translation>Vous devez redémarrer l&apos;application pour que le changement de langue prenne effet.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="290"/>
        <source>Showing disabled targets</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="291"/>
        <source>You just enabled showing targets that are marked disabled. Disabled targets are not recommended to end users. Please use this option only if you know what you are doing.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="556"/>
        <source>Set Cache Path</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="624"/>
        <source>%1 &quot;MacPod&quot; found!
Rockbox needs a FAT formatted Ipod (so-called &quot;WinPod&quot;) to run. </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="635"/>
        <source>Detected an unsupported player:
%1
Sorry, Rockbox doesn&apos;t run on your player.</source>
        <translation>Le lecteur audio détecté n&apos;est pas supporté:
%1
Désolé, Rockbox ne fonctionne pas sur votre lecteur audio.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="640"/>
        <source>Fatal: player incompatible</source>
        <translation>Erreur fatale: le lecteur audio est incompatible</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="651"/>
        <location filename="../configure.cpp" line="660"/>
        <source>Autodetection</source>
        <translation>Autodétection</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="652"/>
        <source>Could not detect a Mountpoint.
Select your Mountpoint manually.</source>
        <translation>Impossible de détecter un point de montage.
Veuillez sélectionner manuellement le point de montage.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="661"/>
        <source>Could not detect a device.
Select your device and Mountpoint manually.</source>
        <translation>Impossible de détecter le périphérique.
Veuillez sélectionner manuellement le type de périphérique et le point de montage.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="672"/>
        <source>Really delete cache?</source>
        <translation>Supprimer le cache ?</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="673"/>
        <source>Do you really want to delete the cache? Make absolutely sure this setting is correct as it will remove &lt;b&gt;all&lt;/b&gt; files in this folder!</source>
        <translation>Voulez-vous supprimer le cache ? Vérifier bien que ce paramètre est correct car il supprimera &lt;b&gt;tous&lt;/b&gt; les fichiers de ce répertoire !</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="681"/>
        <source>Path wrong!</source>
        <translation>Chemin incorrect !</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="682"/>
        <source>The cache path is invalid. Aborting.</source>
        <translation>Le chemin du cache est invalide. Annulation.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="716"/>
        <source>TTS configuration invalid</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="717"/>
        <source>TTS configuration invalid. 
 Please configure TTS engine.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="723"/>
        <source>Could not start TTS engine.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="724"/>
        <source>Could not start TTS engine.
</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="725"/>
        <location filename="../configure.cpp" line="739"/>
        <source>
Please configure TTS engine.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="734"/>
        <source>Rockbox Utility Voice Test</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="737"/>
        <source>Could not voice test string.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="738"/>
        <source>Could not voice test string.
</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="531"/>
        <source>Select your device</source>
        <translation>Sélectionnez votre périphérique</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="617"/>
        <source>Sansa e200 in MTP mode found!
You need to change your player to MSC mode for installation. </source>
        <translation>Sansa e200 en mode MTP trouvé !
Vous devez mettre votre lecteur audio en mode MSC pour l&apos;installation. </translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="620"/>
        <source>H10 20GB in MTP mode found!
You need to change your player to UMS mode for installation. </source>
        <translation>H10 20 Go en mode MTP trouvé !
Vous devez mettre votre lecteur en mode UMS pour l&apos;installation. </translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="628"/>
        <source>Unless you changed this installation will fail!</source>
        <translation>Si vous n&apos;avez pas changé ceci, l&apos;installation échouera !</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="630"/>
        <source>Fatal error</source>
        <translation>Erreur fatale</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="281"/>
        <source>Current cache size is %L1 kiB.</source>
        <translation>La taille actuelle du cache est %L1 ko.</translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="398"/>
        <location filename="../configure.cpp" line="426"/>
        <source>Configuration OK</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="403"/>
        <location filename="../configure.cpp" line="431"/>
        <source>Configuration INVALID</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="111"/>
        <source>The following errors occurred:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="143"/>
        <source>No mountpoint given</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="147"/>
        <source>Mountpoint does not exist</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="151"/>
        <source>Mountpoint is not a directory.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="155"/>
        <source>Mountpoint is not writeable</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="169"/>
        <source>No player selected</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="176"/>
        <source>Cache path not writeable. Leave path empty to default to systems temporary path.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="195"/>
        <source>You need to fix the above errors before you can continue.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../configure.cpp" line="198"/>
        <source>Configuration error</source>
        <translation type="unfinished">Erreur de configuration</translation>
    </message>
</context>
<context>
    <name>ConfigForm</name>
    <message>
        <location filename="../configurefrm.ui" line="14"/>
        <source>Configuration</source>
        <translation>Configuration</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="20"/>
        <source>Configure Rockbox Utility</source>
        <translation>Configuration de l&apos;utilitaire Rockbox</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="35"/>
        <source>&amp;Device</source>
        <translation>&amp;Périphérique</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="41"/>
        <source>Select your device in the &amp;filesystem</source>
        <translation>Sélectionnez votre périphérique dans le &amp;Système de fichiers</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="56"/>
        <location filename="../configurefrm.ui" line="303"/>
        <source>&amp;Browse</source>
        <translation>&amp;Parcourir</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="72"/>
        <source>&amp;Select your audio player</source>
        <translation>&amp;Sélectionnez votre lecteur audio</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="105"/>
        <source>&amp;Autodetect</source>
        <translation>&amp;Autodétection</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="127"/>
        <source>&amp;Proxy</source>
        <translation>&amp;Proxy</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="133"/>
        <source>&amp;No Proxy</source>
        <translation>P&amp;as de proxy</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="143"/>
        <source>Use S&amp;ystem values</source>
        <translation>Utilier les valeurs du s&amp;ystème</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="150"/>
        <source>&amp;Manual Proxy settings</source>
        <translation>Paramètres &amp;manuels pour le proxy</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="157"/>
        <source>Proxy Values</source>
        <translation>Valeurs du proxy</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="163"/>
        <source>&amp;Host:</source>
        <translation>&amp;Hôte:</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="180"/>
        <source>&amp;Port:</source>
        <translation>&amp;Port:</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="203"/>
        <source>&amp;Username</source>
        <translation>&amp;Nom d&apos;utilisateur</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="213"/>
        <source>Pass&amp;word</source>
        <translation>&amp;Mot de passe</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="244"/>
        <source>&amp;Language</source>
        <translation>&amp;Langue</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="258"/>
        <source>Cac&amp;he</source>
        <translation>Cac&amp;he</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="261"/>
        <source>Download cache settings</source>
        <translation>Paramètres du cache de téléchargement</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="267"/>
        <source>Rockbox Utility uses a local download cache to save network traffic. You can change the path to the cache and use it as local repository by enabling Offline mode.</source>
        <translation>L&apos;utilitaire Rockbox utilise un cache local de téléchargement pour économiser de la bande passante. Vous pouvez modifier le chemin du cache et l&apos;utiliser comme un dépôt local en activant le mode hors-ligne.</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="277"/>
        <source>Current cache size is %1</source>
        <translation>La taille actuelle du cache est %1</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="286"/>
        <source>P&amp;ath</source>
        <translation>&amp;Chemin</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="296"/>
        <source>Entering an invalid folder will reset the path to the systems temporary path.</source>
        <translation>Entrer un répertoire invalide remettra la valeur du chemin à celle du répertoire temporaire du système.</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="318"/>
        <source>Disable local &amp;download cache</source>
        <translation>Désactiver le cache local de &amp;téléchargement</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="325"/>
        <source>This will try to use all information from the cache, even information about updates. Only use this option if you want to install without network connection. Note: you need to do the same install you want to perform later with network access first to download all required files to the cache.</source>
        <translation>Ceci essaiera d&apos;utiliser toutes les informations du cache, même les informations de mise à jour.
Utilisez cette option si vous voulez installer sans connexion réseau.
Note : l&apos;installation que vous voudrez réaliser plus tard doit d&apos;abord être faite avec un accès réseau
pour télécharger tous les fichiers néecessaires dans le cache.</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="328"/>
        <source>O&amp;ffline mode</source>
        <translation>Mode &amp;hors-ligne</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="363"/>
        <source>Clean cache &amp;now</source>
        <translation>Nettoyer le cache &amp;maintenant</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="379"/>
        <source>&amp;TTS &amp;&amp; Encoder</source>
        <translation>&amp;TTS &amp;&amp; Encodeur</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="385"/>
        <source>TTS Engine</source>
        <translation>Moteur TTS</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="391"/>
        <source>&amp;Select TTS Engine</source>
        <translation>&amp;Choisir un moteur TTS</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="439"/>
        <source>Test TTS</source>
        <translation>Tester le moteur</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="449"/>
        <source>Encoder Engine</source>
        <translation>Moteur d&apos;encodage</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="523"/>
        <source>&amp;Ok</source>
        <translation>&amp;OK</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="534"/>
        <source>&amp;Cancel</source>
        <translation>&amp;Annuler</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="404"/>
        <source>Configure TTS Engine</source>
        <translation>Configurer le moteur TTS</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="95"/>
        <source>Show disabled targets</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="411"/>
        <location filename="../configurefrm.ui" line="455"/>
        <source>Configuration invalid!</source>
        <translation>Configuration invalide !</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="428"/>
        <source>Configure &amp;TTS</source>
        <translation>Configurer le moteur &amp;TTS</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="472"/>
        <source>Configure &amp;Enc</source>
        <translation>Configurer l&apos;&amp;encodage</translation>
    </message>
    <message>
        <location filename="../configurefrm.ui" line="483"/>
        <source>encoder name</source>
        <translation>nom de l&apos;encodeur</translation>
    </message>
</context>
<context>
    <name>Configure</name>
    <message>
        <location filename="../configure.cpp" line="514"/>
        <source>English</source>
        <comment>This is the localized language name, i.e. your language.</comment>
        <translation>Français</translation>
    </message>
</context>
<context>
    <name>CreateVoiceFrm</name>
    <message>
        <location filename="../createvoicefrm.ui" line="16"/>
        <source>Create Voice File</source>
        <translation>Créer un fichier de voix</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="41"/>
        <source>Select the Language you want to generate a voicefile for:</source>
        <translation>Sélectionner la langue pour laquelle vous souhaitez générer un fichier de voix :</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="48"/>
        <source>Language</source>
        <translation>Langue</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="55"/>
        <source>Generation settings</source>
        <translation>Paramètres de génération</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="61"/>
        <source>Encoder profile:</source>
        <translation>Profil d&apos;encodage :</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="68"/>
        <source>TTS profile:</source>
        <translation>Profil TTS :</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="81"/>
        <source>Change</source>
        <translation>Modifier</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="132"/>
        <source>&amp;Install</source>
        <translation>&amp;Installer</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="142"/>
        <source>&amp;Cancel</source>
        <translation>&amp;Annuler</translation>
    </message>
    <message>
        <location filename="../createvoicefrm.ui" line="156"/>
        <location filename="../createvoicefrm.ui" line="163"/>
        <source>Wavtrim Threshold</source>
        <translation>Seuil pour Wavtrim</translation>
    </message>
</context>
<context>
    <name>CreateVoiceWindow</name>
    <message>
        <location filename="../createvoicewindow.cpp" line="95"/>
        <location filename="../createvoicewindow.cpp" line="98"/>
        <source>Selected TTS engine: &lt;b&gt;%1&lt;/b&gt;</source>
        <translation>Moteur TTS sélectionné : &lt;b&gt;%1&lt;/b&gt;</translation>
    </message>
    <message>
        <location filename="../createvoicewindow.cpp" line="106"/>
        <location filename="../createvoicewindow.cpp" line="109"/>
        <location filename="../createvoicewindow.cpp" line="113"/>
        <source>Selected encoder: &lt;b&gt;%1&lt;/b&gt;</source>
        <translation>Encodeur sélectionné : &lt;b&gt;%1&lt;/b&gt;</translation>
    </message>
</context>
<context>
    <name>EncExes</name>
    <message>
        <location filename="../base/encoders.cpp" line="95"/>
        <source>Path to Encoder:</source>
        <translation>Chemin vers le moteur d&apos;encodage:</translation>
    </message>
    <message>
        <location filename="../base/encoders.cpp" line="97"/>
        <source>Encoder options:</source>
        <translation>Options du moteur d&apos;encodage:</translation>
    </message>
</context>
<context>
    <name>EncRbSpeex</name>
    <message>
        <location filename="../base/encoders.cpp" line="161"/>
        <source>Volume:</source>
        <translation>Volume:</translation>
    </message>
    <message>
        <location filename="../base/encoders.cpp" line="163"/>
        <source>Quality:</source>
        <translation>Qualité:</translation>
    </message>
    <message>
        <location filename="../base/encoders.cpp" line="165"/>
        <source>Complexity:</source>
        <translation>Complexité:</translation>
    </message>
    <message>
        <location filename="../base/encoders.cpp" line="167"/>
        <source>Use Narrowband:</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>EncTtsCfgGui</name>
    <message>
        <location filename="../encttscfggui.cpp" line="32"/>
        <source>Waiting for engine...</source>
        <translation>En attnte du moteur d&apos;encodage...</translation>
    </message>
    <message>
        <location filename="../encttscfggui.cpp" line="82"/>
        <source>Ok</source>
        <translation>Ok</translation>
    </message>
    <message>
        <location filename="../encttscfggui.cpp" line="85"/>
        <source>Cancel</source>
        <translation>Annuler</translation>
    </message>
    <message>
        <location filename="../encttscfggui.cpp" line="184"/>
        <source>Browse</source>
        <translation>Parcourir</translation>
    </message>
    <message>
        <location filename="../encttscfggui.cpp" line="192"/>
        <source>Refresh</source>
        <translation>Rafraîchir</translation>
    </message>
    <message>
        <location filename="../encttscfggui.cpp" line="364"/>
        <source>Select excutable</source>
        <translation>Sélectionner l&apos;exécutable</translation>
    </message>
</context>
<context>
    <name>InstallTalkFrm</name>
    <message>
        <location filename="../installtalkfrm.ui" line="17"/>
        <source>Install Talk Files</source>
        <translation>Installation des fichiers de voix</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="36"/>
        <source>Select the Folder to generate Talkfiles for.</source>
        <translation>Sélectionnez le dossier pour lequel générer des fichiser de voix.</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="43"/>
        <source>Talkfile Folder</source>
        <translation>Dossier des fichiers d&apos;annonce</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="50"/>
        <source>&amp;Browse</source>
        <translation>&amp;Parcourir</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="61"/>
        <source>Generation settings</source>
        <translation>Paramètres de génération</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="67"/>
        <source>Encoder profile:</source>
        <translation>Profil d&apos;encodage :</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="74"/>
        <source>TTS profile:</source>
        <translation>Profil TTS :</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="162"/>
        <source>Generation options</source>
        <translation>Options de génération</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="171"/>
        <source>Ignore files (comma seperated Wildcards):</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="201"/>
        <source>Run recursive</source>
        <translation>Parcours récursif</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="211"/>
        <source>Strip Extensions</source>
        <translation>Suppression des extensions</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="221"/>
        <source>Create only new Talkfiles</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="149"/>
        <source>&amp;Cancel</source>
        <translation>&amp;Annuler</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="138"/>
        <source>&amp;Install</source>
        <translation>&amp;Installer</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="87"/>
        <source>Change</source>
        <translation>Modifier</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="191"/>
        <source>Generate .talk files for Folders</source>
        <translation>Générer des fichiers .talk pour les dossiers</translation>
    </message>
    <message>
        <location filename="../installtalkfrm.ui" line="178"/>
        <source>Generate .talk files for Files</source>
        <translation>Générer des fichiers .talk pour les fichiers</translation>
    </message>
</context>
<context>
    <name>InstallTalkWindow</name>
    <message>
        <location filename="../installtalkwindow.cpp" line="56"/>
        <source>Select folder to create talk files</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../installtalkwindow.cpp" line="91"/>
        <source>The Folder to Talk is wrong!</source>
        <translation>Le dossier à dire est faux !</translation>
    </message>
    <message>
        <location filename="../installtalkwindow.cpp" line="124"/>
        <location filename="../installtalkwindow.cpp" line="127"/>
        <source>Selected TTS engine: &lt;b&gt;%1&lt;/b&gt;</source>
        <translation>Moteur TTS sélectionné : &lt;b&gt;%1&lt;/b&gt;</translation>
    </message>
    <message>
        <location filename="../installtalkwindow.cpp" line="134"/>
        <location filename="../installtalkwindow.cpp" line="137"/>
        <location filename="../installtalkwindow.cpp" line="141"/>
        <source>Selected encoder: &lt;b&gt;%1&lt;/b&gt;</source>
        <translation>Encodeur sélectionné : &lt;b&gt;%1&lt;/b&gt;</translation>
    </message>
</context>
<context>
    <name>InstallWindow</name>
    <message>
        <location filename="../installwindow.cpp" line="107"/>
        <source>Backup to %1</source>
        <translation>Sauvegarde vers %1</translation>
    </message>
    <message>
        <location filename="../installwindow.cpp" line="137"/>
        <source>Mount point is wrong!</source>
        <translation>Le point de montage est incorrect !</translation>
    </message>
    <message>
        <location filename="../installwindow.cpp" line="174"/>
        <source>Really continue?</source>
        <translation>Êtes vous sûr de vouloir poursuivre ?</translation>
    </message>
    <message>
        <location filename="../installwindow.cpp" line="178"/>
        <source>Aborted!</source>
        <translation>Abandon !</translation>
    </message>
    <message>
        <location filename="../installwindow.cpp" line="187"/>
        <source>Beginning Backup...</source>
        <translation>Début de la sauvegarde...</translation>
    </message>
    <message>
        <location filename="../installwindow.cpp" line="203"/>
        <source>Backup successful</source>
        <translation>Sauvegarde effectuée avec succès</translation>
    </message>
    <message>
        <location filename="../installwindow.cpp" line="207"/>
        <source>Backup failed!</source>
        <translation>La sauvegarde a échoué !</translation>
    </message>
    <message>
        <location filename="../installwindow.cpp" line="238"/>
        <source>Select Backup Filename</source>
        <translation>Sélectionnez un fichier de sauvegarde</translation>
    </message>
    <message>
        <location filename="../installwindow.cpp" line="271"/>
        <source>This is the absolute up to the minute Rockbox built. A current build will get updated every time a change is made. Latest version is r%1 (%2).</source>
        <translation>C&apos;est la version la plus récente possible. Elle est mise à jour à chaque fois qu&apos;un changement est fait. La version actuelle est r%1 (%2).</translation>
    </message>
    <message>
        <location filename="../installwindow.cpp" line="277"/>
        <source>&lt;b&gt;This is the recommended version.&lt;/b&gt;</source>
        <translation>&lt;b&gt;C&apos;est la version recommandée.&lt;/b&gt;</translation>
    </message>
    <message>
        <location filename="../installwindow.cpp" line="288"/>
        <source>This is the last released version of Rockbox.</source>
        <translation>C&apos;est la dernière version publique officielle de Rockbox.</translation>
    </message>
    <message>
        <location filename="../installwindow.cpp" line="291"/>
        <source>&lt;b&gt;Note:&lt;/b&gt; The lastest released version is %1. &lt;b&gt;This is the recommended version.&lt;/b&gt;</source>
        <translation>&lt;b&gt;Remarque:&lt;/b&gt;La dernière version publique officielle est %1. &lt;b&gt;C&apos;est la version recommandée.&lt;/b&gt;</translation>
    </message>
    <message>
        <location filename="../installwindow.cpp" line="303"/>
        <source>These are automatically built each day from the current development source code. This generally has more features than the last stable release but may be much less stable. Features may change regularly.</source>
        <translation>Celles-ci sont compilées chaque jour à partir du code source courant. Il y a généralement plus de fonctionnalités que dans la dernière version publique officielle mais le stabilité n&apos;est pas garantie. Les fonctionnalités peuvent changer régulièrement.</translation>
    </message>
    <message>
        <location filename="../installwindow.cpp" line="307"/>
        <source>&lt;b&gt;Note:&lt;/b&gt; archived version is r%1 (%2).</source>
        <translation>&lt;b&gt;Remarque :&lt;/b&gt; la version archivée est r%1 (%2).</translation>
    </message>
</context>
<context>
    <name>InstallWindowFrm</name>
    <message>
        <location filename="../installwindowfrm.ui" line="16"/>
        <source>Install Rockbox</source>
        <translation>Installer Rockbox</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="35"/>
        <source>Please select the Rockbox version you want to install on your player:</source>
        <translation>Veuillez sélectionner la version de Rockbox que vous souhaitez installer sur votre lecteur audio :</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="45"/>
        <source>Version</source>
        <translation>Version</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="51"/>
        <source>Rockbox &amp;stable</source>
        <translation>Rockbox &amp;stable</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="58"/>
        <source>&amp;Archived Build</source>
        <translation>Version &amp;archivée</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="65"/>
        <source>&amp;Current Build</source>
        <translation>Version a&amp;ctuelle</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="75"/>
        <source>Details</source>
        <translation>Détails</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="81"/>
        <source>Details about the selected version</source>
        <translation>Détails à propos de la version sélectionnée</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="91"/>
        <source>Note</source>
        <translation>Remarques</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="119"/>
        <source>&amp;Install</source>
        <translation>&amp;Installer</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="130"/>
        <source>&amp;Cancel</source>
        <translation>&amp;Annuler</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="156"/>
        <source>Backup</source>
        <translation>Sauvegarde</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="162"/>
        <source>Backup before installing</source>
        <translation>Sauvegarder avant d&apos;installer</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="169"/>
        <source>Backup location</source>
        <translation>Lieu de la sauvegarde</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="188"/>
        <source>Change</source>
        <translation>Modifier</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="198"/>
        <source>Rockbox Utility stores copies of Rockbox it has downloaded on the local hard disk to save network traffic. If your local copy is no longer working, tick this box to download a fresh copy.</source>
        <translation>L&apos;utilitaire Rockbox stocke les copies de Rockbox téléchargées sur le disque dur pour économiser de la bande passante. Si votre copie locale ne fonctionne plus, cochez cette casez pour télécharger une nouvelle copie.</translation>
    </message>
    <message>
        <location filename="../installwindowfrm.ui" line="201"/>
        <source>&amp;Don&apos;t use locally cached copy</source>
        <translation>&amp;Ne pas utiliser la version en cache</translation>
    </message>
</context>
<context>
    <name>PreviewFrm</name>
    <message>
        <location filename="../previewfrm.ui" line="16"/>
        <source>Preview</source>
        <translation>Prévisualisation</translation>
    </message>
</context>
<context>
    <name>ProgressLoggerFrm</name>
    <message>
        <location filename="../progressloggerfrm.ui" line="13"/>
        <location filename="../progressloggerfrm.ui" line="19"/>
        <source>Progress</source>
        <translation>Avancement</translation>
    </message>
    <message>
        <location filename="../progressloggerfrm.ui" line="58"/>
        <source>Save Log</source>
        <translation>Sauvegarder l&apos;historique</translation>
    </message>
    <message>
        <location filename="../progressloggerfrm.ui" line="82"/>
        <source>&amp;Abort</source>
        <translation>&amp;Annuler</translation>
    </message>
    <message>
        <location filename="../progressloggerfrm.ui" line="32"/>
        <source>progresswindow</source>
        <translation>Progression</translation>
    </message>
</context>
<context>
    <name>ProgressLoggerGui</name>
    <message>
        <location filename="../progressloggergui.cpp" line="122"/>
        <source>&amp;Ok</source>
        <translation>&amp;OK</translation>
    </message>
    <message>
        <location filename="../progressloggergui.cpp" line="145"/>
        <source>Save system trace log</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../progressloggergui.cpp" line="104"/>
        <source>&amp;Abort</source>
        <translation>&amp;Annuler</translation>
    </message>
</context>
<context>
    <name>QObject</name>
    <message>
        <location filename="../base/system.cpp" line="115"/>
        <source>Guest</source>
        <translation>Invité</translation>
    </message>
    <message>
        <location filename="../base/system.cpp" line="118"/>
        <source>Admin</source>
        <translation>Administrateur</translation>
    </message>
    <message>
        <location filename="../base/system.cpp" line="121"/>
        <source>User</source>
        <translation>Utilisateur</translation>
    </message>
    <message>
        <location filename="../base/system.cpp" line="124"/>
        <source>Error</source>
        <translation>Erreur</translation>
    </message>
    <message>
        <location filename="../base/system.cpp" line="225"/>
        <location filename="../base/system.cpp" line="270"/>
        <source>(no description available)</source>
        <translation>(aucune description disponible)</translation>
    </message>
    <message>
        <location filename="../base/utils.cpp" line="180"/>
        <source>&lt;li&gt;Permissions insufficient for bootloader installation.
Administrator priviledges are necessary.&lt;/li&gt;</source>
        <translation>&lt;li&gt;Permissions insuffisantes pour installation le bootloader.
Les privilèges administrateurs sont recquis.&lt;/li&gt;</translation>
    </message>
    <message>
        <location filename="../base/utils.cpp" line="192"/>
        <source>&lt;li&gt;Target mismatch detected.
Installed target: %1, selected target: %2.&lt;/li&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/utils.cpp" line="199"/>
        <source>Problem detected:</source>
        <translation>Problème détecté:</translation>
    </message>
</context>
<context>
    <name>RbUtilQt</name>
    <message>
        <location filename="../rbutilqt.cpp" line="95"/>
        <source>File</source>
        <translation>Fichier</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="95"/>
        <source>Version</source>
        <translation>Version</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="197"/>
        <location filename="../rbutilqt.cpp" line="229"/>
        <source>Downloading build information, please wait ...</source>
        <translation>Téléchargement des informations de version, veuillez patienter...</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="208"/>
        <source>Can&apos;t get version information!</source>
        <translation>Impossible de récupérer les informations de version !</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="244"/>
        <source>Download build information finished.</source>
        <translation>Le téléchargement des informations de version est terminé.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="257"/>
        <source>Network error: %1. Please check your network and proxy settings.</source>
        <translation>Erreur réseau: %1. Veuillez vérifier vos paramètres réseau et proxy.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="383"/>
        <source>&lt;b&gt;%1 %2&lt;/b&gt; at &lt;b&gt;%3&lt;/b&gt;</source>
        <translation>&lt;b&gt;%1 %2&lt;/b&gt; sur &lt;b&gt;%3&lt;/b&gt;</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="410"/>
        <source>&lt;a href=&apos;%1&apos;&gt;PDF Manual&lt;/a&gt;</source>
        <translation>&lt;a href=&apos;%1&apos;&gt;Manuel PDF&lt;/a&gt;</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="412"/>
        <source>&lt;a href=&apos;%1&apos;&gt;HTML Manual (opens in browser)&lt;/a&gt;</source>
        <translation>&lt;a href=&apos;%1&apos;&gt;Manuel HTML (ouvre un navigateur)&lt;/a&gt;</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="416"/>
        <source>Select a device for a link to the correct manual</source>
        <translation>Sélectionnez un périphérique pour obtenir un lien vers le manuel approprié</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="417"/>
        <source>&lt;a href=&apos;%1&apos;&gt;Manual Overview&lt;/a&gt;</source>
        <translation>&lt;a href=&apos;%1&apos;&gt;Tous les manuels&lt;/a&gt;</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="426"/>
        <location filename="../rbutilqt.cpp" line="482"/>
        <location filename="../rbutilqt.cpp" line="650"/>
        <location filename="../rbutilqt.cpp" line="831"/>
        <location filename="../rbutilqt.cpp" line="880"/>
        <location filename="../rbutilqt.cpp" line="919"/>
        <source>Confirm Installation</source>
        <translation>Confirmation de l&apos;installation</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="505"/>
        <location filename="../rbutilqt.cpp" line="1116"/>
        <source>Mount point is wrong!</source>
        <translation>Le point de montage est erroné !</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="651"/>
        <source>Do you really want to install the Bootloader?</source>
        <translation>Etes-vous sûr de vouloir installer le bootloader ?</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="832"/>
        <source>Do you really want to install the fonts package?</source>
        <translation>Etes-vous sûr de vouloir installer le pack de polices de caractères ?</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="881"/>
        <source>Do you really want to install the voice file?</source>
        <translation>Etes-vous sûr de vouloir installer le fichier de voix ?</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="920"/>
        <source>Do you really want to install the game addon files?</source>
        <translation>Etes-vous sûr de vouloir installer les fichiers additionnels de jeu ?</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="998"/>
        <source>Confirm Uninstallation</source>
        <translation>Confirmation de la désinstallation</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="999"/>
        <source>Do you really want to uninstall the Bootloader?</source>
        <translation>Etes-vous sûr de vouloir désinstaller le bootloader ?</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1054"/>
        <source>Confirm download</source>
        <translation>Confirmation du téléchargement</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1055"/>
        <source>Do you really want to download the manual? The manual will be saved to the root folder of your player.</source>
        <translation>Etes-vous sûr de vouloir télécharger le manuel ? Il sera sauvegardé dans le dossier racine de votre lecteur audio.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1102"/>
        <source>Confirm installation</source>
        <translation>Confirmation de l&apos;installation</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1103"/>
        <source>Do you really want to install Rockbox Utility to your player? After installation you can run it from the players hard drive.</source>
        <translation>Etes-vous sûr de vouloir installer l&apos;utilitaire Rockbox sur votre lecteur audio ? Après l&apos;installation vous pourrez le lancer depuis le disque dur de votre lecteur audio.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1112"/>
        <source>Installing Rockbox Utility</source>
        <translation>Installation de l&apos;utilitaire Rockbox</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1130"/>
        <source>Error installing Rockbox Utility</source>
        <translation>Erreur pendant l&apos;installation de l&apos;utilitaire Rockbox</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1134"/>
        <source>Installing user configuration</source>
        <translation>Installation de la configuration utilisateur</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1138"/>
        <source>Error installing user configuration</source>
        <translation>Erreur pendant l&apos;installation de la configuration utilisateur</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1142"/>
        <source>Successfully installed Rockbox Utility.</source>
        <translation>Installation de l&apos;utilitaire Rockbox terminée avec succès.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1316"/>
        <source>RockboxUtility Update available</source>
        <translation>Une mise à jour de l&apos;utilitaire Rockbox est disponible</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1317"/>
        <source>&lt;b&gt;New RockboxUtility Version available.&lt;/b&gt; &lt;br&gt;&lt;br&gt;Download it from here: &lt;a href=&apos;%1&apos;&gt;%2&lt;/a&gt;</source>
        <translation>&lt;b&gt;Une nouvelle version de l&apos;utilitaire de Rockbox est disponiel.&lt;/b&gt; &lt;br&gt;&lt;br&gt;Vous pouvez la télécharger ici: &lt;a href=&apos;%1&apos;&gt;%2&lt;/a&gt;</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="343"/>
        <location filename="../rbutilqt.cpp" line="1232"/>
        <source>Configuration error</source>
        <translation>Erreur de configuration</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="914"/>
        <source>Error</source>
        <translation>Erreur</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="915"/>
        <source>Your device doesn&apos;t have a doom plugin. Aborting.</source>
        <translation>Votre périphérique n&apos;a pas de le plugin Doom. Annulation.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1233"/>
        <source>Your configuration is invalid. Please go to the configuration dialog and make sure the selected values are correct.</source>
        <translation>Votre configuration est invalide. Veuillez vérifier que les valeurs sélectionnées sont correctes dans la fenêtre de configuration.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="336"/>
        <source>This is a new installation of Rockbox Utility, or a new version. The configuration dialog will now open to allow you to setup the program,  or review your settings.</source>
        <translation>Ceci est une nouvelle installation de l&apos;utilitaire Rockbox, ou l&apos;installation d&apos;une nouvelle version. La fenêtre de configuration va s&apos;ouvrir pour vous permettre de paramétrer le programme ou de vérifier les réglages.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="573"/>
        <source>Aborted!</source>
        <translation>Abandonné !</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="583"/>
        <source>Installed Rockbox detected</source>
        <translation>Installation de Rockbox détectée</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="584"/>
        <source>Rockbox installation detected. Do you want to backup first?</source>
        <translation>Une installation de Rockox a été détectée. Souhaitez-vous la sauvegarder avant de continuer ?</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="609"/>
        <source>Backup failed!</source>
        <translation>Sauvegarde échouée !</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="874"/>
        <source>Warning</source>
        <translation>Avertissement</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="875"/>
        <source>The Application is still downloading Information about new Builds. Please try again shortly.</source>
        <translation>L&apos;application est encore en train de télécharger les informations sur les dernières versions. Veuillez réessayer dans un instant.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="587"/>
        <source>Starting backup...</source>
        <translation>Début de la sauvegarde...</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="335"/>
        <source>New installation</source>
        <translation>Nouvelle installation</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="344"/>
        <source>Your configuration is invalid. This is most likely due to a changed device path. The configuration dialog will now open to allow you to correct the problem.</source>
        <translation>Votre configuration est invalide. Ceci est probablement dû à un changement du chemin du périphérique. La fenêtre de configuration va s&apos;ouvrir pour vous permettre de corriger le problème.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="605"/>
        <source>Backup successful</source>
        <translation>Sauvegarde effectuée avec succès</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="209"/>
        <source>Network error</source>
        <translation>Erreur de réseau</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="210"/>
        <source>Can&apos;t get version information.</source>
        <translation>Impossible de récupérer les informations de version.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="569"/>
        <source>Really continue?</source>
        <translation>Êtes vous sûr de vouloir poursuivre ?</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="694"/>
        <source>No install method known.</source>
        <translation>Aucune méthode d&apos;installation n&apos;est connue.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="714"/>
        <source>Bootloader detected</source>
        <translation>Le bootloader a été détecté</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="715"/>
        <source>Bootloader already installed. Do you want to reinstall the bootloader?</source>
        <translation>Le bootloader est déjà installé. Voulez-vous le réinstaller ?</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="738"/>
        <source>Create Bootloader backup</source>
        <translation>Sauvegarde du bootloader</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="739"/>
        <source>You can create a backup of the original bootloader file. Press &quot;Yes&quot; to select an output folder on your computer to save the file to. The file will get placed in a new folder &quot;%1&quot; created below the selected folder.
Press &quot;No&quot; to skip this step.</source>
        <translation>Vous pouvez créer une sauvegarde du bootloader originel. Appuyez sur &quot;Oui&quot; pour sélectionner un dossier de sortie sur votre ordinateur où le fichier sera enregistré. Le fichier sera placé dans le sous-dossier &quot;%1&quot;.
Appuyez sur &quot;Non&quot; pour ignorez cette étape.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="746"/>
        <source>Browse backup folder</source>
        <translation>Parcourir le dossier de sauvegarde</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="758"/>
        <source>Prerequisites</source>
        <translation>Prérecquis</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="771"/>
        <source>Select firmware file</source>
        <translation>Sélectionnez le fichier du firmware</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="773"/>
        <source>Error opening firmware file</source>
        <translation>Impossible d&apos;ouvrir le firmware</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="791"/>
        <source>Backup error</source>
        <translation>Erreur de la sauvegarde</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="792"/>
        <source>Could not create backup file. Continue?</source>
        <translation>Impossible de créer une sauvegarde. Voulez-vous poursuivre ?</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="822"/>
        <source>Manual steps required</source>
        <translation>Des étapes manuelles sont nécessaire</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="1027"/>
        <source>No uninstall method known.</source>
        <translation>Aucune méthode de suppression n&apos;est connue.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="427"/>
        <source>Do you really want to perform a complete installation?

This will install Rockbox %1. To install the most recent development build available press &quot;Cancel&quot; and use the &quot;Installation&quot; tab.</source>
        <translation>Êtes vous sûr de vouloir procéder à une installation complète ?

Ceci installera Rockbox %1. Pour installer la version en cours de développement la plus récente, appuyez sur &quot;Annuler&quot; et utilise l&apos;onglet &quot;Installation&quot;.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="483"/>
        <source>Do you really want to perform a minimal installation? A minimal installation will contain only the absolutely necessary parts to run Rockbox.

This will install Rockbox %1. To install the most recent development build available press &quot;Cancel&quot; and use the &quot;Installation&quot; tab.</source>
        <translation>Êtes vous sûr de vouloir procéder à une installation minimale ? Une installation minimale ne contient que l&apos;absolu nécessaire pour faire fonctionner Rockbox.

Ceci installera Rockbox %1. Pour installer la version en cours de développement la plus récente, appuyez sur &quot;Annuler&quot; et utilise l&apos;onglet &quot;Installation&quot;.</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="720"/>
        <source>Bootloader installation skipped</source>
        <translation>Installation du bootloader ignorée</translation>
    </message>
    <message>
        <location filename="../rbutilqt.cpp" line="764"/>
        <source>Bootloader installation aborted</source>
        <translation>Installation du bootloader annulée</translation>
    </message>
</context>
<context>
    <name>RbUtilQtFrm</name>
    <message>
        <location filename="../rbutilqtfrm.ui" line="14"/>
        <source>Rockbox Utility</source>
        <translation>Utilitaire Rockbox</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="71"/>
        <source>Device</source>
        <translation>Périphérique</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="83"/>
        <source>Selected device:</source>
        <translation>Périphérique sélectionné :</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="110"/>
        <source>&amp;Change</source>
        <translation>&amp;Modifier</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="128"/>
        <location filename="../rbutilqtfrm.ui" line="832"/>
        <source>&amp;Quick Start</source>
        <translation>&amp;Démarrage rapide</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="131"/>
        <source>Welcome</source>
        <translation>Bienvenue</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="224"/>
        <location filename="../rbutilqtfrm.ui" line="825"/>
        <source>&amp;Installation</source>
        <translation>&amp;Installation</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="227"/>
        <source>Basic Rockbox installation</source>
        <translation>Installation basique de Rockbox</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="233"/>
        <source>Install Bootloader</source>
        <translation>Installation du bootloader</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="260"/>
        <source>Install Rockbox</source>
        <translation>Installer Rockbox</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="320"/>
        <location filename="../rbutilqtfrm.ui" line="839"/>
        <source>&amp;Extras</source>
        <translation>&amp;Extras</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="323"/>
        <source>Install extras for Rockbox</source>
        <translation>Installer des extras pour Rockbox</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="329"/>
        <source>Install Fonts package</source>
        <translation>Installer le pack des polices de caractères</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="356"/>
        <source>Install themes</source>
        <translation>Installer des thèmes</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="383"/>
        <source>Install game files</source>
        <translation>Installer des fichier de jeu</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="437"/>
        <location filename="../rbutilqtfrm.ui" line="847"/>
        <source>&amp;Accessibility</source>
        <translation>&amp;Accessibilité</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="440"/>
        <source>Install accessibility add-ons</source>
        <translation>Installer des add-ons pour l&apos;accessibilité</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="446"/>
        <source>Install Voice files</source>
        <translation>Installer des fichiers de voix</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="473"/>
        <source>Install Talk files</source>
        <translation>Installer des fichiers de voix</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="552"/>
        <location filename="../rbutilqtfrm.ui" line="855"/>
        <source>&amp;Uninstallation</source>
        <translation>&amp;Désinstallation</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="555"/>
        <location filename="../rbutilqtfrm.ui" line="588"/>
        <source>Uninstall Rockbox</source>
        <translation>Désinstaller Rockbox</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="561"/>
        <source>Uninstall Bootloader</source>
        <translation>Désinstaller le bootloader</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="648"/>
        <source>&amp;Manual</source>
        <translation>&amp;Manuel</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="651"/>
        <source>View and download the manual</source>
        <translation>Voir et télécharger le manuel</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="657"/>
        <source>Read the manual</source>
        <translation>Lire le manuel</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="663"/>
        <source>PDF manual</source>
        <translation>Manuel PDF</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="676"/>
        <source>HTML manual</source>
        <translation>Manuel HTML</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="692"/>
        <source>Download the manual</source>
        <translation>Télécharger le manuel</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="700"/>
        <source>&amp;PDF version</source>
        <translation>Version &amp;PDF</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="707"/>
        <source>&amp;HTML version (zip file)</source>
        <translation>Version &amp;HTML (fichier zip)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="729"/>
        <source>Down&amp;load</source>
        <translation>Télé&amp;charger</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="753"/>
        <source>Inf&amp;o</source>
        <translation>Inf&amp;o</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="773"/>
        <source>1</source>
        <translation>1</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="795"/>
        <source>&amp;File</source>
        <translation>&amp;Fichier</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="808"/>
        <source>&amp;Troubleshoot</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="896"/>
        <source>&amp;About</source>
        <translation>&amp;A propos</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="1052"/>
        <source>System &amp;Trace</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="873"/>
        <source>Empty local download cache</source>
        <translation>Vider le cache local de téléchargement</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="878"/>
        <source>Install Rockbox Utility on player</source>
        <translation>Installer l&apos;utilitaire Rockbox sur le lecteur</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="883"/>
        <source>&amp;Configure</source>
        <translation>&amp;Configuration</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="888"/>
        <source>E&amp;xit</source>
        <translation>&amp;Quitter</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="891"/>
        <source>Ctrl+Q</source>
        <translation>Ctrl+Q</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="901"/>
        <source>About &amp;Qt</source>
        <translation>A propos de &amp;Qt</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="804"/>
        <location filename="../rbutilqtfrm.ui" line="906"/>
        <source>&amp;Help</source>
        <translation>&amp;Aide</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="137"/>
        <source>Complete Installation</source>
        <translation>Installation complète</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="821"/>
        <source>Action&amp;s</source>
        <translation>Action&amp;s</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="911"/>
        <source>Info</source>
        <translation>Information</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="1015"/>
        <source>Read PDF manual</source>
        <translation>Lire le manuel PDF</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="1020"/>
        <source>Read HTML manual</source>
        <translation>Lire le manuel HTML</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="1025"/>
        <source>Download PDF manual</source>
        <translation>Télécharger le manuel PDF</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="1030"/>
        <source>Download HTML manual (zip)</source>
        <translation>Télécharger le manuel HTML (zip)</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="523"/>
        <source>Create Voice files</source>
        <translation>Créer des fichiers de voix</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="1042"/>
        <source>Create Voice File</source>
        <translation>Créer un fichier de voix</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="154"/>
        <source>&lt;b&gt;Complete Installation&lt;/b&gt;&lt;br/&gt;This installs the bootloader, a current build and the extras package. This is the recommended method for new installations.</source>
        <translation>&lt;b&gt;Installation complète&lt;/b&gt;&lt;br/&gt;Ceci installe le bootloader, une version actuelle et le paquet des extras. C&apos;est la méthode recommandée pour les nouvelles installations.</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="250"/>
        <source>&lt;b&gt;Install the bootloader&lt;/b&gt;&lt;br/&gt;Before Rockbox can be run on your audio player, you may have to install a bootloader. This is only necessary the first time Rockbox is installed.</source>
        <translation>&lt;b&gt;Installer le bootloader&lt;/b&gt;&lt;br/&gt;Avant de pouvoir utiliser Rockbox sur votre lecteur audio, vous devez installer un bootloader. Ceci n&apos;est nécessaire que la première fois que vous installez Rockbox.</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="277"/>
        <source>&lt;b&gt;Install Rockbox&lt;/b&gt; on your audio player</source>
        <translation>&lt;b&gt;Installer Rockbox&lt;/b&gt; sur votre lecteur audio</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="346"/>
        <source>&lt;b&gt;Fonts Package&lt;/b&gt;&lt;br/&gt;The Fonts Package contains a couple of commonly used fonts. Installation is highly recommended.</source>
        <translation>&lt;b&gt;Paquet des polices de caractères&lt;/b&gt;&lt;br/&gt;Il contient des polices communément utilisées. Son installation est fortement recommandée.</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="373"/>
        <source>&lt;b&gt;Install Themes&lt;/b&gt;&lt;br/&gt;Rockbox&apos; look can be customized by themes. You can choose and install several officially distributed themes.</source>
        <translation>&lt;b&gt;Installer des thèmes&lt;/b&gt;&lt;br/&gt;L&apos;apparence de Rockbox peut être personnalisée avec des thèmes. Vous pouvez choisir et installer plusieurs thèmes officiellement distribués.</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="400"/>
        <source>&lt;b&gt;Install Game Files&lt;/b&gt;&lt;br/&gt;Doom needs a base wad file to run.</source>
        <translation>&lt;b&gt;Installer les fichiers de jeu&lt;/b&gt;&lt;br/&gt;Doom nécessite un fichier wad pour pouvoir être utilisé.</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="463"/>
        <source>&lt;b&gt;Install Voice file&lt;/b&gt;&lt;br/&gt;Voice files are needed to make Rockbox speak the user interface. Speaking is enabled by default, so if you installed the voice file Rockbox will speak.</source>
        <translation>&lt;b&gt;Installer le fichier de voix&lt;/b&gt;&lt;br/&gt;Celui-ci est requis pour que Rockbox parle. La synthèse vocale est activée par défaut, donc si vous installez le fichier de voix, Rockbox parlera.</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="490"/>
        <source>&lt;b&gt;Create Talk Files&lt;/b&gt;&lt;br/&gt;Talkfiles are needed to let Rockbox speak File and Foldernames</source>
        <translation>&lt;b&gt;Créer des fichiers d&apos;annonce&lt;/b&gt;&lt;br/&gt;Ceux-ci sont nécessaires pour que Rockbox énonce les noms des fichiers et des dossiers</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="540"/>
        <source>&lt;b&gt;Create Voice file&lt;/b&gt;&lt;br/&gt;Voice files are needed to make Rockbox speak the  user interface. Speaking is enabled by default, so
 if you installed the voice file Rockbox will speak.</source>
        <translation>&lt;b&gt;Créer le fichier de voix&lt;/b&gt;&lt;br/&gt;celui-ci est requis pour que Rockbox parle. La synthèse vocale est activée par défaut, doncsi vous installez le fichier de voix, Rockbox parlera.</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="578"/>
        <source>&lt;b&gt;Remove the bootloader&lt;/b&gt;&lt;br/&gt;After removing the bootloader you won&apos;t be able to start Rockbox.</source>
        <translation>&lt;b&gt;Désinstaller le bootloader&lt;/b&gt;&lt;br/&gt;Après avoir supprimé le bootloader, vous ne pourrez plus démarrer Rockbox.</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="605"/>
        <source>&lt;b&gt;Uninstall Rockbox from your audio player.&lt;/b&gt;&lt;br/&gt;This will leave the bootloader in place (you need to remove it manually).</source>
        <translation>&lt;b&gt;Désinstaller Rockbox de votre lecteur audio&lt;/b&gt;&lt;br/&gt;Ceci laissera le bootloader (vous devrez le supprimer manuellement).</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="759"/>
        <source>Currently installed packages.&lt;br/&gt;&lt;b&gt;Note:&lt;/b&gt; if you manually installed packages this might not be correct!</source>
        <translation>Paquets actuellement installés.&lt;br/&gt;&lt;b&gt;Note :&lt;/b&gt;Si vous avez installé manuellement des paquets, ceci ne sera peut-être pas correct !</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="938"/>
        <source>Install &amp;Bootloader</source>
        <translation>Installer le &amp;bootloader</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="947"/>
        <source>Install &amp;Rockbox</source>
        <translation>Installer &amp;Rockbox</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="956"/>
        <source>Install &amp;Fonts Package</source>
        <translation>Installer le pack des &amp;polices de caractères</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="965"/>
        <source>Install &amp;Themes</source>
        <translation>Installer des &amp;thèmes</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="974"/>
        <source>Install &amp;Game Files</source>
        <translation>Installer les fichiers de &amp;jeu</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="983"/>
        <source>&amp;Install Voice File</source>
        <translation>&amp;Installer le fichier de voix</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="992"/>
        <source>Create &amp;Talk Files</source>
        <translation>Installer les fichiers d&apos;&amp;annonce</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="1001"/>
        <source>Remove &amp;bootloader</source>
        <translation>Désinstaller le &amp;bootloader</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="1010"/>
        <source>Uninstall &amp;Rockbox</source>
        <translation>Désinstaller &amp;Rockbox</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="1039"/>
        <source>Create &amp;Voice File</source>
        <translation>&amp;Créer le fichier de voix</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="1047"/>
        <source>&amp;System Info</source>
        <translation>Informations &amp;système</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="920"/>
        <source>&amp;Complete Installation</source>
        <translation>Installation &amp;complète</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="90"/>
        <source>device / mountpoint unknown or invalid</source>
        <translation>périphérique / point de montage inconnu ou invalide</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="167"/>
        <source>Minimal Installation</source>
        <translation>Installation minimale</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="184"/>
        <source>&lt;b&gt;Minimal installation&lt;/b&gt;&lt;br/&gt;This installs bootloader and the current build of Rockbox. If you don&apos;t want the extras package, choose this option.</source>
        <translation>&lt;b&gt;Installation minimale&lt;/b&gt;&lt;br/&gt;Ceci installe the bootloader et une version actuelle de Rockbox. Si vous ne désirez aucun paquet supplémentaire, choisissez cette option.</translation>
    </message>
    <message>
        <location filename="../rbutilqtfrm.ui" line="929"/>
        <source>&amp;Minimal Installation</source>
        <translation>Installation &amp;minimale</translation>
    </message>
</context>
<context>
    <name>ServerInfo</name>
    <message>
        <location filename="../base/serverinfo.cpp" line="72"/>
        <source>Unknown</source>
        <translation>Inconnu</translation>
    </message>
    <message>
        <location filename="../base/serverinfo.cpp" line="76"/>
        <source>Unusable</source>
        <translation>Inutilisable</translation>
    </message>
    <message>
        <location filename="../base/serverinfo.cpp" line="79"/>
        <source>Unstable</source>
        <translation>Instable</translation>
    </message>
    <message>
        <location filename="../base/serverinfo.cpp" line="82"/>
        <source>Stable</source>
        <translation>Stable</translation>
    </message>
</context>
<context>
    <name>SysTrace</name>
    <message>
        <location filename="../systrace.cpp" line="71"/>
        <location filename="../systrace.cpp" line="80"/>
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
        <translation>&amp;Fermer</translation>
    </message>
    <message>
        <location filename="../systracefrm.ui" line="57"/>
        <source>&amp;Save</source>
        <translation>&amp;Enregistrer</translation>
    </message>
    <message>
        <location filename="../systracefrm.ui" line="68"/>
        <source>&amp;Refresh</source>
        <translation>&amp;Rafraîchir</translation>
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
        <location filename="../sysinfo.cpp" line="45"/>
        <source>&lt;b&gt;OS&lt;/b&gt;&lt;br/&gt;</source>
        <translation>&lt;b&gt;Système d&apos;exploitation&lt;/b&gt;&lt;br/&gt;</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="46"/>
        <source>&lt;b&gt;Username&lt;/b&gt;&lt;br/&gt;%1&lt;hr/&gt;</source>
        <translation>&lt;b&gt;Nom d&apos;utilisateur&lt;/b&gt;&lt;br/&gt;%1&lt;hr/&gt;</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="48"/>
        <source>&lt;b&gt;Permissions&lt;/b&gt;&lt;br/&gt;%1&lt;hr/&gt;</source>
        <translation>&lt;b&gt;Permissions&lt;/b&gt;&lt;br/&gt;%1&lt;hr/&gt;</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="50"/>
        <source>&lt;b&gt;Attached USB devices&lt;/b&gt;&lt;br/&gt;</source>
        <translation>&lt;b&gt;Périphériques USB branchés&lt;/b&gt;&lt;br/&gt;</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="54"/>
        <source>VID: %1 PID: %2, %3</source>
        <translation>VID: %1 PID: %2, %3</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="63"/>
        <source>Filesystem</source>
        <translation>Système de fichier</translation>
    </message>
    <message>
        <location filename="../sysinfo.cpp" line="66"/>
        <source>%1, %2 MiB available</source>
        <translation>%1, %2 Mio disponibles</translation>
    </message>
</context>
<context>
    <name>SysinfoFrm</name>
    <message>
        <location filename="../sysinfofrm.ui" line="13"/>
        <source>System Info</source>
        <translation>Informations système</translation>
    </message>
    <message>
        <location filename="../sysinfofrm.ui" line="22"/>
        <source>&amp;Refresh</source>
        <translation>&amp;Rafraîchir</translation>
    </message>
    <message>
        <location filename="../sysinfofrm.ui" line="45"/>
        <source>&amp;OK</source>
        <translation>&amp;OK</translation>
    </message>
</context>
<context>
    <name>TTSCarbon</name>
    <message>
        <location filename="../base/ttscarbon.cpp" line="130"/>
        <source>Voice:</source>
        <translation>Voix:</translation>
    </message>
    <message>
        <location filename="../base/ttscarbon.cpp" line="136"/>
        <source>Speed (words/min):</source>
        <translation>Vitesse (mots/min):</translation>
    </message>
    <message>
        <location filename="../base/ttscarbon.cpp" line="200"/>
        <source>Could not voice string</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/ttscarbon.cpp" line="209"/>
        <source>Could not convert intermediate file</source>
        <translation>Impossible de convertir le fichier intermédiaire</translation>
    </message>
</context>
<context>
    <name>TTSExes</name>
    <message>
        <location filename="../base/ttsexes.cpp" line="68"/>
        <source>TTS executable not found</source>
        <translation>Exécutable TTS non trouvé</translation>
    </message>
    <message>
        <location filename="../base/ttsexes.cpp" line="40"/>
        <source>Path to TTS engine:</source>
        <translation>Chemin vers le moteur TTS:</translation>
    </message>
    <message>
        <location filename="../base/ttsexes.cpp" line="42"/>
        <source>TTS engine options:</source>
        <translation>Option du moteur TTS:</translation>
    </message>
</context>
<context>
    <name>TTSFestival</name>
    <message>
        <location filename="../base/ttsfestival.cpp" line="197"/>
        <source>engine could not voice string</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/ttsfestival.cpp" line="280"/>
        <source>No description available</source>
        <translation>Aucune description n&apos;est disponible</translation>
    </message>
    <message>
        <location filename="../base/ttsfestival.cpp" line="45"/>
        <source>Path to Festival client:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/ttsfestival.cpp" line="50"/>
        <source>Voice:</source>
        <translation>Voix:</translation>
    </message>
    <message>
        <location filename="../base/ttsfestival.cpp" line="59"/>
        <source>Voice description:</source>
        <translation>Description de la voix:</translation>
    </message>
</context>
<context>
    <name>TTSSapi</name>
    <message>
        <location filename="../base/ttssapi.cpp" line="99"/>
        <source>Could not copy the Sapi-script</source>
        <translation>Impossible de copier le script Sapi</translation>
    </message>
    <message>
        <location filename="../base/ttssapi.cpp" line="120"/>
        <source>Could not start the Sapi-script</source>
        <translation>Impossible de démarrer le script Sapi</translation>
    </message>
    <message>
        <location filename="../base/ttssapi.cpp" line="39"/>
        <source>Language:</source>
        <translation>Langue:</translation>
    </message>
    <message>
        <location filename="../base/ttssapi.cpp" line="45"/>
        <source>Voice:</source>
        <translation>Voix:</translation>
    </message>
    <message>
        <location filename="../base/ttssapi.cpp" line="52"/>
        <source>Speed:</source>
        <translation>Vitesse:</translation>
    </message>
    <message>
        <location filename="../base/ttssapi.cpp" line="55"/>
        <source>Options:</source>
        <translation>Options:</translation>
    </message>
</context>
<context>
    <name>TalkFileCreator</name>
    <message>
        <location filename="../base/talkfile.cpp" line="36"/>
        <source>Starting Talk file generation</source>
        <translation>Démarrage de la génération des fichiers d&apos;annonce</translation>
    </message>
    <message>
        <location filename="../base/talkfile.cpp" line="66"/>
        <source>Copying Talkfiles...</source>
        <translation>Copie des fichiers d&apos;annonce...</translation>
    </message>
    <message>
        <location filename="../base/talkfile.cpp" line="228"/>
        <source>File copy aborted</source>
        <translation>Copie du fichier annulée</translation>
    </message>
    <message>
        <location filename="../base/talkfile.cpp" line="267"/>
        <source>Cleaning up...</source>
        <translation>Nettoyage...</translation>
    </message>
    <message>
        <location filename="../base/talkfile.cpp" line="278"/>
        <source>Finished</source>
        <translation>Terminé</translation>
    </message>
    <message>
        <location filename="../base/talkfile.cpp" line="44"/>
        <source>Talk file creation aborted</source>
        <translation>Création des fichiers d&apos;annonce abandonnée</translation>
    </message>
    <message>
        <location filename="../base/talkfile.cpp" line="78"/>
        <source>Finished creating Talk files</source>
        <translation>Création des fichiers d&apos;annonce terminée</translation>
    </message>
    <message>
        <location filename="../base/talkfile.cpp" line="41"/>
        <source>Reading Filelist...</source>
        <translation>Lecture de la liste des fichiers...</translation>
    </message>
    <message>
        <location filename="../base/talkfile.cpp" line="246"/>
        <source>Copying of %1 to %2 failed</source>
        <translation>La copie de %1 vers %2 a échoué</translation>
    </message>
</context>
<context>
    <name>TalkGenerator</name>
    <message>
        <location filename="../base/talkgenerator.cpp" line="39"/>
        <source>Starting TTS Engine</source>
        <translation>Démarrage du moteur TTS</translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="44"/>
        <source>Init of TTS engine failed</source>
        <translation>L&apos;initialisation du moteur TTS a échoué</translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="51"/>
        <source>Starting Encoder Engine</source>
        <translation>Démarrage du moteur d&apos;encodage</translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="55"/>
        <source>Init of Encoder engine failed</source>
        <translation>L&apos;initialisation de l&apos;encodeur a échoué</translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="65"/>
        <source>Voicing entries...</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="80"/>
        <source>Encoding files...</source>
        <translation>Encodage des fichiers...</translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="119"/>
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
        <location filename="../base/talkgenerator.cpp" line="197"/>
        <source>Encoding aborted</source>
        <translation>Encodage interrompu</translation>
    </message>
    <message>
        <location filename="../base/talkgenerator.cpp" line="223"/>
        <source>Encoding of %1 failed</source>
        <translation>L&apos;encodage de %1 a échoué</translation>
    </message>
</context>
<context>
    <name>ThemeInstallFrm</name>
    <message>
        <location filename="../themesinstallfrm.ui" line="13"/>
        <source>Theme Installation</source>
        <translation>Installation de thèmes</translation>
    </message>
    <message>
        <location filename="../themesinstallfrm.ui" line="48"/>
        <source>Selected Theme</source>
        <translation>Thème sélectionné</translation>
    </message>
    <message>
        <location filename="../themesinstallfrm.ui" line="73"/>
        <source>Description</source>
        <translation>Description</translation>
    </message>
    <message>
        <location filename="../themesinstallfrm.ui" line="83"/>
        <source>Download size:</source>
        <translation>Taille du téléchargement :</translation>
    </message>
    <message>
        <location filename="../themesinstallfrm.ui" line="125"/>
        <source>&amp;Cancel</source>
        <translation>&amp;Annuler</translation>
    </message>
    <message>
        <location filename="../themesinstallfrm.ui" line="115"/>
        <source>&amp;Install</source>
        <translation>&amp;Installer</translation>
    </message>
    <message>
        <location filename="../themesinstallfrm.ui" line="93"/>
        <source>Hold Ctrl to select multiple item, Shift for a range</source>
        <translation>Maintenez Ctrl appuyé pour sélectionner plusieurs éléments, et Shift pour sélectionner des éléments consécutifs</translation>
    </message>
</context>
<context>
    <name>ThemesInstallWindow</name>
    <message>
        <location filename="../themesinstallwindow.cpp" line="37"/>
        <source>no theme selected</source>
        <translation>pas de thème sélectionné</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="104"/>
        <source>Network error: %1.
Please check your network and proxy settings.</source>
        <translation>Erreur réseau: %1.
Veuillez vérifier vos paramètres réseau et proxy.</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="117"/>
        <source>the following error occured:
%1</source>
        <translation>L&apos;erreur suivante s&apos;est produite :
%1</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="123"/>
        <source>done.</source>
        <translation>terminé.</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="190"/>
        <source>fetching details for %1</source>
        <translation>récupération des détails pour %1</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="193"/>
        <source>fetching preview ...</source>
        <translation>récupération de l&apos;aperçu ...</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="206"/>
        <source>&lt;b&gt;Author:&lt;/b&gt; %1&lt;hr/&gt;</source>
        <translation>&lt;b&gt;Auteur :&lt;/b&gt; %1&lt;hr/&gt;</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="207"/>
        <location filename="../themesinstallwindow.cpp" line="209"/>
        <source>unknown</source>
        <translation>inconnu</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="208"/>
        <source>&lt;b&gt;Version:&lt;/b&gt; %1&lt;hr/&gt;</source>
        <translation>&lt;b&gt;Version:&lt;/b&gt; %1&lt;hr/&gt;</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="210"/>
        <source>&lt;b&gt;Description:&lt;/b&gt; %1&lt;hr/&gt;</source>
        <translation>&lt;b&gt;Description:&lt;/b&gt; %1&lt;hr/&gt;</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="211"/>
        <source>no description</source>
        <translation>pas de description</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="254"/>
        <source>no theme preview</source>
        <translation>pas d&apos;aperçu du thème</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="285"/>
        <source>getting themes information ...</source>
        <translation>récupération des informations sur le thème ...</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="333"/>
        <source>Mount point is wrong!</source>
        <translation>Le point de montage est incorrect !</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="38"/>
        <source>no selection</source>
        <translation>Aucune sélection</translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="160"/>
        <source>Information</source>
        <translation>Informations</translation>
    </message>
    <message numerus="yes">
        <location filename="../themesinstallwindow.cpp" line="177"/>
        <source>Download size %L1 kiB (%n item(s))</source>
        <translation>
            <numerusform>Taille du téléchargement: %L1 Kio (%n élément)</numerusform>
            <numerusform>Taille du téléchargement: %L1 Kio (%n élément(s))</numerusform>
        </translation>
    </message>
    <message>
        <location filename="../themesinstallwindow.cpp" line="242"/>
        <source>Retrieving theme preview failed.
HTTP response code: %1</source>
        <translation>La récupération d&apos;un aperçu du thème a échoué.
Code de réponse HHTP: %1</translation>
    </message>
</context>
<context>
    <name>UnZip</name>
    <message>
        <location filename="../zip/unzip.cpp" line="250"/>
        <source>ZIP operation completed successfully.</source>
        <translation>Opération ZIP terminée avec succès.</translation>
    </message>
    <message>
        <location filename="../zip/unzip.cpp" line="251"/>
        <source>Failed to initialize or load zlib library.</source>
        <translation>Impossible d&apos;initialiser ou de charger la librairie zlib.</translation>
    </message>
    <message>
        <location filename="../zip/unzip.cpp" line="252"/>
        <source>zlib library error.</source>
        <translation>Erreur de la librairie zlib.</translation>
    </message>
    <message>
        <location filename="../zip/unzip.cpp" line="253"/>
        <source>Unable to create or open file.</source>
        <translation>Impossible de créer ou d&apos;ouvrir le fichier.</translation>
    </message>
    <message>
        <location filename="../zip/unzip.cpp" line="254"/>
        <source>Partially corrupted archive. Some files might be extracted.</source>
        <translation>Archive partiellement corrompue. Certains fichiers pourraient être extraits.</translation>
    </message>
    <message>
        <location filename="../zip/unzip.cpp" line="255"/>
        <source>Corrupted archive.</source>
        <translation>Archive corrompue.</translation>
    </message>
    <message>
        <location filename="../zip/unzip.cpp" line="256"/>
        <source>Wrong password.</source>
        <translation>Mot de passe incorrect.</translation>
    </message>
    <message>
        <location filename="../zip/unzip.cpp" line="257"/>
        <source>No archive has been created yet.</source>
        <translation>Aucune archive n&apos;a encore été crée.</translation>
    </message>
    <message>
        <location filename="../zip/unzip.cpp" line="258"/>
        <source>File or directory does not exist.</source>
        <translation>Le fichier ou le répertoire n&apos;existe pas.</translation>
    </message>
    <message>
        <location filename="../zip/unzip.cpp" line="259"/>
        <source>File read error.</source>
        <translation>Erreur de lecture du fichier.</translation>
    </message>
    <message>
        <location filename="../zip/unzip.cpp" line="260"/>
        <source>File write error.</source>
        <translation>Erreur d&apos;écriture du fichier.</translation>
    </message>
    <message>
        <location filename="../zip/unzip.cpp" line="261"/>
        <source>File seek error.</source>
        <translation>Erreur de parcours du fichier.</translation>
    </message>
    <message>
        <location filename="../zip/unzip.cpp" line="262"/>
        <source>Unable to create a directory.</source>
        <translation>Impossible de créer un dossier.</translation>
    </message>
    <message>
        <location filename="../zip/unzip.cpp" line="263"/>
        <source>Invalid device.</source>
        <translation>Périphérique invalide.</translation>
    </message>
    <message>
        <location filename="../zip/unzip.cpp" line="264"/>
        <source>Invalid or incompatible zip archive.</source>
        <translation>Archive zip invalide ou incompatible.</translation>
    </message>
    <message>
        <location filename="../zip/unzip.cpp" line="265"/>
        <source>Inconsistent headers. Archive might be corrupted.</source>
        <translation>Les en-têtes ne correspondent pas. L&apos;archive est peut-être corrompue.</translation>
    </message>
    <message>
        <location filename="../zip/unzip.cpp" line="269"/>
        <source>Unknown error.</source>
        <translation>Erreur inconnue.</translation>
    </message>
</context>
<context>
    <name>UninstallFrm</name>
    <message>
        <location filename="../uninstallfrm.ui" line="16"/>
        <source>Uninstall Rockbox</source>
        <translation>Désinstaller Rockbox</translation>
    </message>
    <message>
        <location filename="../uninstallfrm.ui" line="35"/>
        <source>Please select the Uninstallation Method</source>
        <translation>Veuillez choisir la méthode de désinstallation</translation>
    </message>
    <message>
        <location filename="../uninstallfrm.ui" line="45"/>
        <source>Uninstallation Method</source>
        <translation>Méthode de désinstallation</translation>
    </message>
    <message>
        <location filename="../uninstallfrm.ui" line="51"/>
        <source>Complete Uninstallation</source>
        <translation>Désinstallation complète</translation>
    </message>
    <message>
        <location filename="../uninstallfrm.ui" line="58"/>
        <source>Smart Uninstallation</source>
        <translation>Désinstallation intelligente</translation>
    </message>
    <message>
        <location filename="../uninstallfrm.ui" line="68"/>
        <source>Please select what you want to uninstall</source>
        <translation>Veuillez sélectionner ce que vous désirer désinstaller</translation>
    </message>
    <message>
        <location filename="../uninstallfrm.ui" line="78"/>
        <source>Installed Parts</source>
        <translation>Composants installés</translation>
    </message>
    <message>
        <location filename="../uninstallfrm.ui" line="138"/>
        <source>&amp;Cancel</source>
        <translation>&amp;Annuler</translation>
    </message>
    <message>
        <location filename="../uninstallfrm.ui" line="128"/>
        <source>&amp;Uninstall</source>
        <translation>&amp;Désinstaller</translation>
    </message>
</context>
<context>
    <name>Uninstaller</name>
    <message>
        <location filename="../base/uninstall.cpp" line="33"/>
        <location filename="../base/uninstall.cpp" line="46"/>
        <source>Starting Uninstallation</source>
        <translation>Démarrage de la désinstallation</translation>
    </message>
    <message>
        <location filename="../base/uninstall.cpp" line="38"/>
        <source>Finished Uninstallation</source>
        <translation>Désinstallation terminée</translation>
    </message>
    <message>
        <location filename="../base/uninstall.cpp" line="52"/>
        <source>Uninstalling %1...</source>
        <translation>Suppression de %1...</translation>
    </message>
    <message>
        <location filename="../base/uninstall.cpp" line="83"/>
        <source>Could not delete %1</source>
        <translation>Impossible de supprimer %1</translation>
    </message>
    <message>
        <location filename="../base/uninstall.cpp" line="113"/>
        <source>Uninstallation finished</source>
        <translation>Désinstallation terminée</translation>
    </message>
</context>
<context>
    <name>VoiceFileCreator</name>
    <message>
        <location filename="../base/voicefile.cpp" line="40"/>
        <source>Starting Voicefile generation</source>
        <translation>Démarrage de la génération du fichier de voix</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="82"/>
        <source>Downloading voice info...</source>
        <translation>Téléchargement des informations d&apos;annonces</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="95"/>
        <source>Download error: received HTTP error %1.</source>
        <translation>Erreur de téléchargement : Erreur HTTP %1.</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="101"/>
        <source>Cached file used.</source>
        <translation>Fichier en cache utilisé.</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="104"/>
        <source>Download error: %1</source>
        <translation>Erreur de téléchargement : %1</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="109"/>
        <source>Download finished.</source>
        <translation>Téléchargement terminé.</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="117"/>
        <source>failed to open downloaded file</source>
        <translation>Impossible d&apos;ouvrir le fichier téléchargé</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="125"/>
        <source>Reading strings...</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="192"/>
        <source>Creating voicefiles...</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="233"/>
        <source>Cleaning up...</source>
        <translation>Nettoyage...</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="244"/>
        <source>Finished</source>
        <translation>Terminé</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="169"/>
        <source>The downloaded file was empty!</source>
        <translation>Le fichier téléchargé est vide !</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="197"/>
        <source>Error opening downloaded file</source>
        <translation>Erreur lors de l&apos;ouverture du fichier téléchargé</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="206"/>
        <source>Error opening output file</source>
        <translation>Erreur lors de l&apos;ouverture du fichier de sortie</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="224"/>
        <source>successfully created.</source>
        <translation>Créé avec succes.</translation>
    </message>
    <message>
        <location filename="../base/voicefile.cpp" line="53"/>
        <source>could not find rockbox-info.txt</source>
        <translation>Impossible de trouver rockbox-info.txt</translation>
    </message>
</context>
<context>
    <name>Zip</name>
    <message>
        <location filename="../zip/zip.cpp" line="481"/>
        <source>ZIP operation completed successfully.</source>
        <translation>Opération ZIP terminée avec succès.</translation>
    </message>
    <message>
        <location filename="../zip/zip.cpp" line="482"/>
        <source>Failed to initialize or load zlib library.</source>
        <translation>Impossible d&apos;initialiser ou de charger la librairie zlib.</translation>
    </message>
    <message>
        <location filename="../zip/zip.cpp" line="483"/>
        <source>zlib library error.</source>
        <translation>Erreur de la librairie zlib.</translation>
    </message>
    <message>
        <location filename="../zip/zip.cpp" line="484"/>
        <source>Unable to create or open file.</source>
        <translation>Impossible de créer ou d&apos;ouvrir le fichier.</translation>
    </message>
    <message>
        <location filename="../zip/zip.cpp" line="485"/>
        <source>No archive has been created yet.</source>
        <translation>Aucune archive n&apos;a encore été crée.</translation>
    </message>
    <message>
        <location filename="../zip/zip.cpp" line="486"/>
        <source>File or directory does not exist.</source>
        <translation>Le fichier ou le répertoire n&apos;existe pas.</translation>
    </message>
    <message>
        <location filename="../zip/zip.cpp" line="487"/>
        <source>File read error.</source>
        <translation>Erreur de lecture du fichier.</translation>
    </message>
    <message>
        <location filename="../zip/zip.cpp" line="488"/>
        <source>File write error.</source>
        <translation>Erreur d&apos;écriture du fichier.</translation>
    </message>
    <message>
        <location filename="../zip/zip.cpp" line="489"/>
        <source>File seek error.</source>
        <translation>Erreur de parcours du fichier.</translation>
    </message>
    <message>
        <location filename="../zip/zip.cpp" line="493"/>
        <source>Unknown error.</source>
        <translation>Erreur inconnue.</translation>
    </message>
</context>
<context>
    <name>ZipInstaller</name>
    <message>
        <location filename="../base/zipinstaller.cpp" line="59"/>
        <source>done.</source>
        <translation>terminé.</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="67"/>
        <source>Installation finished successfully.</source>
        <translation>Installation terminée avec succès.</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="80"/>
        <source>Downloading file %1.%2</source>
        <translation>Téléchargement du fichier %1.%2</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="114"/>
        <source>Download error: received HTTP error %1.</source>
        <translation>Erreur de téléchargement. Erreur HTTP %1.</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="120"/>
        <source>Cached file used.</source>
        <translation>Fichier en cache utilisé.</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="122"/>
        <source>Download error: %1</source>
        <translation>Erreur de téléchargement : %1</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="126"/>
        <source>Download finished.</source>
        <translation>Téléchargement terminé.</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="132"/>
        <source>Extracting file.</source>
        <translation>Extraction du fichier.</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="141"/>
        <source>Opening archive failed: %1.</source>
        <translation>L&apos;ouverture de l&apos;archive a échoué : %1.</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="160"/>
        <source>Extracting failed: %1.</source>
        <translation>L&apos;extraction a échoué : %1.</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="171"/>
        <source>Installing file.</source>
        <translation>Installation du fichier.</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="182"/>
        <source>Installing file failed.</source>
        <translation>L&apos;installation du fichier a échoué.</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="191"/>
        <source>Creating installation log</source>
        <translation>Création du journal d&apos;installation</translation>
    </message>
    <message>
        <location filename="../base/zipinstaller.cpp" line="152"/>
        <source>Not enough disk space! Aborting.</source>
        <translation>Espace disque insuffisant ! Abandon.</translation>
    </message>
</context>
<context>
    <name>aboutBox</name>
    <message>
        <location filename="../aboutbox.ui" line="13"/>
        <source>About Rockbox Utility</source>
        <translation>A propos de l&apos;utilitaire Rockbox</translation>
    </message>
    <message>
        <location filename="../aboutbox.ui" line="35"/>
        <source>The Rockbox Utility</source>
        <translation>L&apos;utilitaire Rockbox</translation>
    </message>
    <message utf8="true">
        <location filename="../aboutbox.ui" line="57"/>
        <source>Installer and housekeeping utility for the Rockbox open source digital audio player firmware.&lt;br/&gt;© 2005 - 2009 The Rockbox Team.&lt;br/&gt;Released under the GNU General Public License v2.&lt;br/&gt;Uses icons by the &lt;a href=&quot;http://tango.freedesktop.org/&quot;&gt;Tango Project&lt;/a&gt;.&lt;br/&gt;&lt;center&gt;&lt;a href=&quot;http://www.rockbox.org&quot;&gt;http://www.rockbox.org&lt;/a&gt;&lt;/center&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../aboutbox.ui" line="77"/>
        <source>&amp;Credits</source>
        <translation>&amp;Crédits</translation>
    </message>
    <message>
        <location filename="../aboutbox.ui" line="109"/>
        <source>&amp;License</source>
        <translation>&amp;Licence</translation>
    </message>
    <message>
        <location filename="../aboutbox.ui" line="151"/>
        <source>&amp;Ok</source>
        <translation>&amp;OK</translation>
    </message>
</context>
</TS>
