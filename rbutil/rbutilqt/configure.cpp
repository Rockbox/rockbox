/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Riebeling
 *   $Id:$
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <QtGui>

#include "configure.h"
#include "ui_configurefrm.h"

Config::Config(QWidget *parent) : QDialog(parent)
{
    ui.setupUi(this);
    ui.radioManualProxy->setChecked(true);
    QRegExpValidator *proxyValidator = new QRegExpValidator(this);
    QRegExp validate("[0-9]*");
    proxyValidator->setRegExp(validate);
    ui.proxyPort->setValidator(proxyValidator);

    ui.radioSystemProxy->setEnabled(false); // not implemented yet

    connect(ui.buttonOk, SIGNAL(clicked()), this, SLOT(accept()));
    connect(ui.buttonCancel, SIGNAL(clicked()), this, SLOT(abort()));
    connect(ui.radioNoProxy, SIGNAL(toggled(bool)), this, SLOT(setNoProxy(bool)));
}


void Config::accept()
{
    qDebug() << "Config::accept()";
    QUrl proxy;
    proxy.setScheme("http");
    proxy.setUserName(ui.proxyUser->text());
    proxy.setPassword(ui.proxyPass->text());
    proxy.setHost(ui.proxyHost->text());
    proxy.setPort(ui.proxyPort->text().toInt());

    userSettings->setValue("defaults/proxy", proxy.toString());
    qDebug() << "new proxy:" << proxy;

    QString proxyType;
    if(ui.radioNoProxy->isChecked()) proxyType = "none";
    else if(ui.radioSystemProxy->isChecked()) proxyType = "system";
    else proxyType = "manual";
    userSettings->setValue("defaults/proxytype", proxyType);

    userSettings->sync();
    this->close();
    emit settingsUpdated();
}


void Config::abort()
{
    qDebug() << "Config::abort()";
    this->close();
}


void Config::setUserSettings(QSettings *user)
{
    userSettings = user;
    QUrl proxy = userSettings->value("defaults/proxy").toString();

    ui.proxyPort->insert(QString("%1").arg(proxy.port()));
    ui.proxyHost->insert(proxy.host());
    ui.proxyUser->insert(proxy.userName());
    ui.proxyPass->insert(proxy.password());

    QString proxyType = userSettings->value("defaults/proxytype").toString();
    if(proxyType == "manual") ui.radioManualProxy->setChecked(true);
    else if(proxyType == "system") ui.radioSystemProxy->setChecked(true);
    else if(proxyType == "none") ui.radioNoProxy->setChecked(true);

}


void Config::setNoProxy(bool checked)
{
    bool i = !checked;
    ui.proxyPort->setEnabled(i);
    ui.proxyHost->setEnabled(i);
    ui.proxyUser->setEnabled(i);
    ui.proxyPass->setEnabled(i);
}

