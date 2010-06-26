/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * This file has been copied from Lorenzo Bettini, with minor modifications
 * made available under the LGPL version 3, as the original file was licensed
 *
 ****************************************************************************
 *
 * Copyright (C) 2009  Lorenzo Bettini <http://www.lorenzobettini.it>
 * See COPYING file that comes with this distribution
 */

#ifndef FINDREPLACEDIALOG_H
#define FINDREPLACEDIALOG_H

#include <QDialog>
#include <QPlainTextEdit>

namespace Ui {
    class FindReplaceDialog;
}

class QTextEdit;
class QSettings;

/**
  * A find/replace dialog.
  *
  * It relies on a FindReplaceForm object (see that class for the functionalities provided).
  */
class FindReplaceDialog : public QDialog {
    Q_OBJECT
public:
    FindReplaceDialog(QWidget *parent = 0);
    virtual ~FindReplaceDialog();

    /**
      * Associates the text editor where to perform the search
      * @param textEdit
      */
    void setTextEdit(QPlainTextEdit *textEdit);

    /**
      * Writes the state of the form to the passed settings.
      * @param settings
      * @param prefix the prefix to insert in the settings
      */
    virtual void writeSettings(QSettings &settings, const QString &prefix = "FindReplaceDialog");

    /**
      * Reads the state of the form from the passed settings.
      * @param settings
      * @param prefix the prefix to look for in the settings
      */
    virtual void readSettings(QSettings &settings, const QString &prefix = "FindReplaceDialog");

public slots:
    /**
     * Finds the next occurrence
     */
    void findNext();

    /**
     * Finds the previous occurrence
     */
    void findPrev();

protected:
    void changeEvent(QEvent *e);

    Ui::FindReplaceDialog *ui;
};

#endif // FINDREPLACEDIALOG_H
