/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Wenger
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <QDialog>
#include <QMouseEvent>

#include "preview.h"

PreviewDlg::PreviewDlg(QWidget *parent) : QDialog(parent)
{
    ui.setupUi(this);
    this->setModal(true);
    this->setMouseTracking(true);
    this->setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);

}

void PreviewDlg::setText(QString text)
{
    ui.themePreview->setText(text);
}

void PreviewDlg::setPixmap(QPixmap p)
{
   ui.themePreview->setFixedSize(p.size());
   this->resize(QSize(10,10));
   ui.themePreview->setPixmap(p);
}

void PreviewDlg::mouseMoveEvent(QMouseEvent * event)
{
    (void) event;
    this->close();
}

void PreviewDlg::leaveEvent(QEvent * event)
{
    (void) event;
    this->close();
}


void PreviewDlg::changeEvent(QEvent *e)
{
    if(e->type() == QEvent::LanguageChange) {
        ui.retranslateUi(this);
    } else {
        QWidget::changeEvent(e);
    }
}

PreviewLabel::PreviewLabel(QWidget * parent, Qt::WindowFlags f)
            :QLabel(parent,f)
{
    this->setMouseTracking(true);

    preview = new PreviewDlg(parent);

    hovertimer.setInterval(1500);  // wait for 1.5 seconds before showing the Fullsize Preview
    hovertimer.setSingleShot(true);
    connect(&hovertimer, &QTimer::timeout, this, &PreviewLabel::timeout);
}

void PreviewLabel::mouseMoveEvent(QMouseEvent * event)
{
    hovertimer.start();
    mousepos = event->globalPos();
}
void PreviewLabel::enterEvent(QEvent * event)
{
    (void) event;
    hovertimer.start();
}
void PreviewLabel::leaveEvent(QEvent * event)
{
    (void) event;
    hovertimer.stop();
}

void PreviewLabel::timeout()
{
    preview->move(mousepos.x() - (preview->width() / 2),
                  mousepos.y() - (preview->height() / 2));
    preview->setVisible(true);
}

void PreviewLabel::setPixmap(QPixmap p)
{
    // set the image for the Fullsize Preview
    preview->setPixmap(p);

    //scale the image for use in the label
    QSize img;
    img.setHeight(this->height());
    img.setWidth(this->width());
    QPixmap q;
    q = p.scaled(img, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    this->setScaledContents(false);
    QLabel::setPixmap(q);
}

void PreviewLabel::setText(QString text)
{
    QLabel::setText(text);
    preview->setText(text);
}

