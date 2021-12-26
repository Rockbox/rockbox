/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2013 by Dominik Riebeling
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "changelog.h"
#include "rbsettings.h"
#include "ui_changelogfrm.h"

Changelog::Changelog(QWidget *parent) : QDialog(parent)
{
    ui.setupUi(this);
    ui.browserChangelog->setOpenExternalLinks(true);
    // FIXME: support translated changelog file (changelog.de.txt etc)
    ui.browserChangelog->setHtml(parseChangelogFile(":/docs/changelog.txt"));
    ui.browserChangelog->moveCursor(QTextCursor::End, QTextCursor::MoveAnchor);
    ui.checkBoxShowAlways->setChecked(RbSettings::value(RbSettings::ShowChangelog).toBool());
    connect(ui.buttonOk, &QAbstractButton::clicked, this, &Changelog::accept);
}


void Changelog::accept(void)
{
    RbSettings::setValue(RbSettings::ShowChangelog, ui.checkBoxShowAlways->isChecked());
    this->hide();
    this->deleteLater();
}


QString Changelog::parseChangelogFile(QString filename)
{
    QFile changelog(filename);
    changelog.open(QIODevice::ReadOnly);
    QTextStream c(&changelog);
#if QT_VERSION < 0x060000
    c.setCodec(QTextCodec::codecForName("UTF-8"));
#else
    c.setEncoding(QStringConverter::Utf8);
#endif
    QString text;
    while(!c.atEnd()) {
        QString line = c.readLine();
        if(line.startsWith("#"))
            continue;
        if(line.startsWith("Version")) {
            text.append(QString("<h4>Rockbox Utility %1</h4>").arg(line.remove("Version")));
            line = c.readLine();
            text.append("<ul>");
            while(line.startsWith("*")) {
                QString t = line.remove(QRegularExpression("^\\*"));
                t.replace(QRegularExpression("FS#(\\d+)"),
                          "<a href='https://www.rockbox.org/tracker/task/\\1'>FS#\\1</a>");
                t.replace(QRegularExpression("G#(\\d+)"),
                          "<a href='https://gerrit.rockbox.org/r/\\1'>G#\\1</a>");
                text.append(QString("<li>%1</li>").arg(t));
                line = c.readLine();
                if(line.startsWith("#"))
                    line = c.readLine();
            }
            text.append("</ul>");
        }
    }
    changelog.close();
    return text;
}
