/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2012 by Jean-Louis Biasini
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <QtGui>
#include <QDebug>
#include "rbutilqt.h"
#include "rbsettings.h"
#include "changelogwidget.h"

ChangelogWidget::ChangelogWidget(QWidget *parent) : QWidget(parent)
{
    QApplication::processEvents();
    QDialog *window = new QDialog(this);
    Ui::ChangelogWidgetFrm changelogObj;
    changelogObj.setupUi(window);
    changelogObj.changelogLabelText1->setText(tr("This is a new installation "
    "of Rockbox Utility, or a new version.\r Read this changelog to learn"
    "about new features and bugs fixes (this changelog is also available under"
    "Help > About.):"));
    changelogObj.changelogLabelText2->setText(tr("The configuration dialog "
    "will now open to allow you to setup the program,  or review your"
    "settings."));
    changelogObj.alwaysShowBox->setText(tr("Always show when starting from "
    "a new version"));
    window->setLayoutDirection(Qt::LeftToRight);
    window->setModal(true);
    window->show();
    /* checkbox choose if we display the changelog screen on startup through
    rbsettings enum ChangelogHintOnBoot */
    connect(changelogObj.alwaysShowBox,
            SIGNAL(clicked(bool)),
            this,
            SLOT(setShowChangelog(bool)));
    /* we close the Dialog when the user press the button Ok */
    connect(changelogObj.okButton, SIGNAL(clicked()), this, SIGNAL(changelogClosed()));
}

void ChangelogWidget::setShowChangelog(bool val)
{
    QVariant valtoset;
    if(val)
        valtoset = "true";
    else
        valtoset = "false";
    RbSettings::setSubValue("show_changelog",
                            RbSettings::ShowChangelog,
                            valtoset);
}
