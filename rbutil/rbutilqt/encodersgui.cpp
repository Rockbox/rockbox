/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Wenger
 *   $Id$
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
 
#include "encodersgui.h"

#include "rbsettings.h"
#include "browsedirtree.h"

EncExesGui::EncExesGui(QDialog* parent) : QDialog(parent)
{
    ui.setupUi(this);
    this->hide();
    connect(ui.reset,SIGNAL(clicked()),this,SLOT(reset()));
    connect(ui.browse,SIGNAL(clicked()),this,SLOT(browse()));
}   

void EncExesGui::showCfg(QString name)
{
    m_name = name;
    // try to get config from settings
    QString exepath =settings->encoderPath(m_name);
    ui.encoderoptions->setText(settings->encoderOptions(m_name));   
    
    if(exepath == "")
    {
     
        // try to autodetect encoder
#if defined(Q_OS_LINUX) || defined(Q_OS_MACX)
        QStringList path = QString(getenv("PATH")).split(":", QString::SkipEmptyParts);
#elif defined(Q_OS_WIN)
        QStringList path = QString(getenv("PATH")).split(";", QString::SkipEmptyParts);
#endif
        qDebug() << path;
        
        for(int i = 0; i < path.size(); i++) 
        {
            QString executable = QDir::fromNativeSeparators(path.at(i)) + "/" + m_name;
#if defined(Q_OS_WIN)
            executable += ".exe";
            QStringList ex = executable.split("\"", QString::SkipEmptyParts);
            executable = ex.join("");
#endif
            if(QFileInfo(executable).isExecutable()) 
            {
                qDebug() << "found:" << executable;
                exepath = QDir::toNativeSeparators(executable);
                break;
            }
        }
    }
    
    ui.encoderpath->setText(exepath);
    
     //show dialog
    this->exec();
    
}

void EncExesGui::accept(void)
{
    //save settings in user config
    settings->setEncoderPath(m_name,ui.encoderpath->text());
    settings->setEncoderOptions(m_name,ui.encoderoptions->text());
    
    // sync settings
    settings->sync();
    this->done(0);
}

void EncExesGui::reject(void)
{
    this->done(0);
}

void EncExesGui::reset()
{
    ui.encoderpath->setText("");
    ui.encoderoptions->setText("");   
}

void EncExesGui::browse()
{
    BrowseDirtree browser(this);
    browser.setFilter(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);

    if(QFileInfo(ui.encoderpath->text()).isDir())
    {
        browser.setDir(ui.encoderpath->text());
    }
    if(browser.exec() == QDialog::Accepted)
    {
        qDebug() << browser.getSelected();
        QString exe = browser.getSelected();
        if(!QFileInfo(exe).isExecutable())
            return;
        ui.encoderpath->setText(exe);
    }
}


EncRbSpeexGui::EncRbSpeexGui(QDialog* parent) : QDialog(parent)
{
    ui.setupUi(this);
    this->hide();
    connect(ui.reset,SIGNAL(clicked()),this,SLOT(reset()));

}

void EncRbSpeexGui::showCfg(float defQ,float defV,int defC, bool defB)
{
    defaultQuality =defQ;
    defaultVolume = defV;
    defaultComplexity = defC;
    defaultBand =defB;
    
    //fill in the usersettings
    ui.volume->setValue(settings->encoderVolume("rbspeex"));
    ui.quality->setValue(settings->encoderQuality("rbspeex"));
    ui.complexity->setValue(settings->encoderComplexity("rbspeex"));
    
    if(settings->encoderNarrowband("rbspeex"))
        ui.narrowband->setCheckState(Qt::Checked);
    else
        ui.narrowband->setCheckState(Qt::Unchecked);

    //show dialog
    this->exec();
}

void EncRbSpeexGui::accept(void)
{
    //save settings in user config
    settings->setEncoderVolume("rbspeex",ui.volume->value());
    settings->setEncoderQuality("rbspeex",ui.quality->value());
    settings->setEncoderComplexity("rbspeex",ui.complexity->value());
    settings->setEncoderNarrowband("rbspeex",ui.narrowband->isChecked() ? true : false);

    // sync settings
    settings->sync();
    this->done(0);
}

void EncRbSpeexGui::reject(void)
{
    this->done(0);
}

void EncRbSpeexGui::reset()
{
    ui.volume->setValue(defaultVolume);
    ui.quality->setValue(defaultQuality);
    ui.complexity->setValue(defaultComplexity);
    ui.narrowband->setChecked(Qt::Unchecked); 
}

