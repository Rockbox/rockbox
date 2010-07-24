/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Robert Bieber
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef PROJECTEXPORTER_H
#define PROJECTEXPORTER_H

#include <QDialog>
#include <QCloseEvent>
#include <QFile>

#include "quazip.h"

#include "projectmodel.h"

namespace Ui {
    class ProjectExporter;
}

class ProjectExporter : public QDialog {
    Q_OBJECT
public:
    ProjectExporter(QString path, ProjectModel* project, QWidget *parent = 0);
    ~ProjectExporter();

protected:
    void changeEvent(QEvent *e);
    void closeEvent(QCloseEvent *event);

private slots:
    void close();

private:
    void writeZip(QString path, QString base = "");
    void checkRes(ProjectModel* project);
    void checkWPS(ProjectModel* project, QString file);

    void addSuccess(QString text);
    void addWarning(QString text);
    void addError(QString text);

    Ui::ProjectExporter *ui;
    QuaZip zipFile;
    QString html;
};

#endif // PROJECTEXPORTER_H
