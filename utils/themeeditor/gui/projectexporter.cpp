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

#include "projectexporter.h"
#include "ui_projectexporter.h"

#include "tag_table.h"
#include "skin_parser.h"

#include "quazipfile.h"

#include <QTextStream>
#include <QDir>
#include <QSettings>
#include <QDebug>

ProjectExporter::ProjectExporter(QString path, ProjectModel* project,
                                 QWidget *parent)
                                     :QDialog(parent),
                                     ui(new Ui::ProjectExporter), zipFile(path)
{
    ui->setupUi(this);

    QObject::connect(ui->closeButton, SIGNAL(clicked()),
                     this, SLOT(close()));

    if(zipFile.open(QuaZip::mdCreate))
    {

        checkRes(project);
        writeZip(project->getSetting("themebase", ""));
        zipFile.close();

        addSuccess(tr("Project exported successfully"));
    }
    else
    {
        addError(tr("Couldn't open zip file"));
    }
}

ProjectExporter::~ProjectExporter()
{
    delete ui;
}

void ProjectExporter::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void ProjectExporter::closeEvent(QCloseEvent *event)
{
    close();
    event->accept();
}

void ProjectExporter::close()
{
    deleteLater();
    hide();
}

void ProjectExporter::writeZip(QString path, QString base)
{
    if(base == "")
        base = path;
    if(path == "")
    {
        addError(tr("Couldn't locate project directory"));
        return;
    }

    QDir dir(path);

    /* First adding any files in the directory */
    QFileInfoList files = dir.entryInfoList(QDir::Files);
    for(int i = 0; i < files.count(); i++)
    {
        QFileInfo current = files[i];

        QString newPath = current.absoluteFilePath().replace(base, "/.rockbox");

        QuaZipFile fout(&zipFile);
        QFile fin(current.absoluteFilePath());

        fin.open(QFile::ReadOnly | QFile::Text);
        fout.open(QIODevice::WriteOnly,
                  QuaZipNewInfo(newPath, current.absoluteFilePath()));

        fout.write(fin.readAll());

        fin.close();
        fout.close();
    }

    /* Then recursively adding any directories */
    QFileInfoList dirs = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for(int i = 0; i < dirs.count(); i++)
    {
        QFileInfo current = dirs[i];
        writeZip(current.absoluteFilePath(), base);
    }
}

void ProjectExporter::checkRes(ProjectModel *project)
{
    QMap<QString, QString> settings = project->getSettings();
    QMap<QString, QString>::iterator i;

    for(i = settings.begin(); i != settings.end(); i++)
    {
        if(i.key() == "wps" || i.key() == "rwps" || i.key() == "sbs"
           || i.key() == "rsbs" || i.key() == "fms" || i.key() == "rfms")
        {
            checkWPS(project, i.value());
        }
        else if(i.value().contains("/.rockbox")
                && i.key() != "configfile" && i.key() != "themebase")
        {
            QString absPath = i.value().replace("/.rockbox",
                                                settings.value("themebase"));
            if(QFile::exists(absPath))
            {
                addSuccess(i.key() + tr(" found"));
            }
            else
            {
                if(i.key() == "font")
                {
                    QSettings qset;
                    qset.beginGroup("RBFont");
                    QString fontDir = qset.value("fontDir", "").toString();
                    qset.endGroup();

                    QString newDir = fontDir + "/" + absPath.split("/").last();

                    if(QFile::exists(newDir))
                    {
                        addSuccess(tr("font found in font pack"));
                    }
                    else
                    {
                        addWarning(tr("font not found"));
                    }

                }
                else
                {
                    addWarning(i.key() + tr(" not found"));
                }
            }
        }
    }
}

void ProjectExporter::checkWPS(ProjectModel* project, QString file)
{
    /* Set this to false if any resource checks fail */
    bool check = true;

    QSettings settings;
    settings.beginGroup("RBFont");
    QString fontPack = settings.value("fontDir", "").toString() + "/";
    settings.endGroup();

    QString fontDir = project->getSetting("themebase", "") + "/fonts/";
    QString wpsName = file.split("/").last().split(".").first();
    QString imDir = project->getSetting("themebase", "") + "/wps/" + wpsName +
                    "/";

    QFile fin(file.replace("/.rockbox", project->getSetting("themebase", "")));
    if(!fin.open(QFile::ReadOnly | QFile::Text))
    {
        addWarning(tr("Couldn't open ") + file.split("/").last());
    }

    QString contents(fin.readAll());
    fin.close();

    skin_element* root;
    root = skin_parse(contents.toLatin1());
    if(!root)
    {
        addWarning(tr("Couldn't parse ") + file.split("/").last());
        return;
    }

    /* Now we scan through the tree to check all the resources */
    /* Outer loop scans through all viewports */
    while(root)
    {
        skin_element* line;
        if(root->children_count == 0)
            line = 0;
        else
            line = root->children[0];

        /* Next loop scans through logical lines */
        while(line)
        {

            /* Innermost loop gives top-level tags */
            skin_element* current;
            if(line->children_count == 0)
                current = 0;
            else
                current = line->children[0];
            while(current)
            {
                if(current->type == TAG)
                {
                    if(QString(current->tag->name) == "Fl")
                    {
                        QString font = current->params[1].data.text;
                        if(!QFile::exists(fontDir + font)
                            && !QFile::exists(fontPack + font))
                        {
                            check = false;
                            addWarning(font + tr(" not found"));
                        }
                    }
                    else if(QString(current->tag->name) == "X")
                    {
                        QString backdrop = current->params[0].data.text;
                        if(!QFile::exists(imDir + backdrop))
                        {
                            check = false;
                            addWarning(backdrop + tr(" not found"));
                        }
                    }
                    else if(QString(current->tag->name) == "xl")
                    {
                        QString image = current->params[1].data.text;
                        if(!QFile::exists(imDir + image))
                        {
                            check = false;
                            addWarning(image + tr(" not found"));
                        }
                    }
                }
                current = current->next;
            }

            line = line->next;
        }

        root = root->next;
    }

    if(check)
        addSuccess(file.split("/").last() + tr(" passed resource check"));
    else
        addWarning(file.split("/").last() + tr(" failed resource check"));

}

void ProjectExporter::addSuccess(QString text)
{
    html += tr("<span style =\"color:green\">") + text + tr("</span><br>");
    ui->statusBox->document()->setHtml(html);
}

void ProjectExporter::addWarning(QString text)
{
    html += tr("<span style =\"color:orange\">Warning: ") + text +
            tr("</span><br>");
    ui->statusBox->document()->setHtml(html);
}

void ProjectExporter::addError(QString text)
{
    html += tr("<span style =\"color:red\">Error: ") + text +
            tr("</span><br>");
    ui->statusBox->document()->setHtml(html);
}
