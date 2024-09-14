/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2012 Dominik Riebeling
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 * This file is a modified version of the AMS installer by Dominik Wenger
 *
 ****************************************************************************/

#include <QtCore>

#include "bootloaderinstallhelper.h"
#include "bootloaderinstallmi4.h"
#include "bootloaderinstallhex.h"
#include "bootloaderinstallipod.h"
#include "bootloaderinstallsansa.h"
#include "bootloaderinstallfile.h"
#include "bootloaderinstallchinachip.h"
#include "bootloaderinstallams.h"
#include "bootloaderinstalltcc.h"
#include "bootloaderinstallmpio.h"
#include "bootloaderinstallimx.h"
#include "bootloaderinstalls5l.h"
#include "bootloaderinstallbspatch.h"

BootloaderInstallBase* BootloaderInstallHelper::createBootloaderInstaller(QObject* parent, QString type)
{
    if(type == "mi4") {
        return new BootloaderInstallMi4(parent);
    }
    else if(type == "hex") {
        return new BootloaderInstallHex(parent);
    }
    else if(type == "sansa") {
        return new BootloaderInstallSansa(parent);
    }
    else if(type == "ipod") {
        return new BootloaderInstallIpod(parent);
    }
    else if(type == "file") {
        return new BootloaderInstallFile(parent);
    }
    else if(type == "chinachip") {
        return new BootloaderInstallChinaChip(parent);
    }
    else if(type == "ams") {
        return new BootloaderInstallAms(parent);
    }
    else if(type == "tcc") {
        return new BootloaderInstallTcc(parent);
    }
    else if(type == "mpio") {
        return new BootloaderInstallMpio(parent);
    }
    else if(type == "imx") {
        return new BootloaderInstallImx(parent);
    }
    else if(type == "s5l") {
        return new BootloaderInstallS5l(parent);
    }
    else if(type == "bspatch") {
        return new BootloaderInstallBSPatch(parent);
    }
    else {
        return nullptr;
    }
}

BootloaderInstallBase::Capabilities
    BootloaderInstallHelper::bootloaderInstallerCapabilities(QObject *parent, QString type)
{
    /* Note - this is a terrible pattern in general, but in this case
     * it is a much simpler option to just allocate a class instance.
     * This operation is rarely used, anyway. */

    BootloaderInstallBase* bootloaderInstaller =
        createBootloaderInstaller(parent, type);

    BootloaderInstallBase::Capabilities caps = BootloaderInstallBase::Capabilities();

    if(bootloaderInstaller) {
        caps = bootloaderInstaller->capabilities();
        delete bootloaderInstaller;
    }

    return caps;
}

//! @brief Return pre install hints string.
//! @param model model string
//! @return hints.
QString BootloaderInstallHelper::preinstallHints(QString model)
{
    bool hint = false;
    QString msg = QObject::tr("Before Bootloader installation begins, "
            "Please check the following:");

    msg += "<ol>";
    if(model.contains("erosqnative")) {
        hint = true;
        msg += QObject::tr("<li>Ensure your SD card is formatted as FAT. "
                "exFAT is <i>not</i> supported. You can reformat using the "
                "Original Firmware on your player if need be. It is located "
                "under (System Settings --> Reset --> Format TF Card).</li>"
                "<li>Please use a quality SD card from a reputable source. "
                "The SD cards that come bundled with players are often of "
                "substandard quality and may cause issues.</li>");
    }
    msg += "</ol>";

    if(hint)
        return msg;
    else
        return QString();
}


//! @brief Return post install hints string.
//! @param model model string
//! @return hints.
QString BootloaderInstallHelper::postinstallHints(QString model)
{
    bool hint = false;
    QString msg = QObject::tr("Bootloader installation is almost complete. "
            "Installation <b>requires</b> you to perform the "
            "following steps manually:");

    msg += "<ol>";
    if(model != "sansafuzeplus") {
        msg += QObject::tr("<li>Safely remove your player.</li>");
    }
    if(model == "iriverh100" || model == "iriverh120" || model == "iriverh300"
       || model == "ondavx747" || model == "agptekrocker"
       || model == "xduoox3" || model == "xduoox3ii" || model == "xduoox20"
       || model.contains("erosqnative")) {
        hint = true;
        msg += QObject::tr("<li>Reboot your player into the original firmware.</li>"
                "<li>Perform a firmware upgrade using the update functionality "
                "of the original firmware. Please refer to your player's manual "
                "on details.<br/><b>Important:</b> updating the firmware is a "
                "critical process that must not be interrupted. <b>Make sure the "
                "player is charged before starting the firmware update "
                "process.</b></li>"
                "<li>After the firmware has been updated reboot your player.</li>");
    }
    if(model == "sansafuzeplus") {
        hint = true;
        msg += QObject::tr("<li>Remove any previously inserted microSD card</li>");
        msg += QObject::tr("<li>Disconnect your player. The player will reboot and "
                "perform an update of the original firmware. "
                "Please refer to your players manual on details.<br/>"
                "<b>Important:</b> updating the firmware is a "
                "critical process that must not be interrupted. <b>Make sure the "
                "player is charged before disconnecting the player.</b></li>"
                "<li>After the firmware has been updated reboot your player.</li>");
    }
    if(model == "iaudiox5" || model == "iaudiom5"
            || model == "iaudiox5v" || model == "iaudiom3" || model == "mpioh200") {
        hint = true;
        msg += QObject::tr("<li>Turn the player off</li>"
                "<li>Insert the charger</li>");
    }
    if(model == "gigabeatf") {
        hint = true;
        msg += QObject::tr("<li>Unplug USB and power adaptors</li>"
                "<li>Hold <i>Power</i> to turn the player off</li>"
                "<li>Toggle the battery switch on the player</li>"
                "<li>Hold <i>Power</i> to boot into Rockbox</li>");
    }
    msg += "</ol>";
    msg += QObject::tr("<p><b>Note:</b> You can safely install other parts first, but "
            "the above steps are <b>required</b> to finish the installation!</p>");

    if(hint)
        return msg;
    else
        return QString();
}
