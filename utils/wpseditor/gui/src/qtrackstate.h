/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Rostilav Checkan
 *   $Id$
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

#ifndef __QTRACKSTATE_H__
#define __QTRACKSTATE_H__

#include "wpsstate.h"
#include <QObject>

class QTrackState : public QObject {
    Q_OBJECT
    Q_CLASSINFO ( "QTrackState", "Track State" );
    Q_PROPERTY ( QString Title READ title WRITE setTitle DESIGNABLE true USER true )
    Q_PROPERTY ( QString Artist READ artist WRITE setArtist DESIGNABLE true USER true )
    Q_PROPERTY ( QString Album READ album WRITE setAlbum DESIGNABLE true USER true )
    Q_PROPERTY ( int Length READ length WRITE setLength DESIGNABLE true USER true )
    Q_CLASSINFO("Length", "readOnly=true;value=100");
    Q_PROPERTY ( int Elapsed READ elapsed WRITE setElapsed DESIGNABLE true USER true )
    Q_CLASSINFO("Elapsed", "minimum=0;maximum=100;value=50");

    trackstate state;

public:
    QTrackState();

public slots:
    QString title() const {
        return state.title;
    }
    void setTitle ( const QString& name );

    QString artist() const {
        return state.artist;
    }
    void setArtist ( const QString& name );

    QString album() const {
        return state.album;
    }
    void setAlbum ( const QString& name );

    int length() const {
        return state.length;
    }
    void setLength ( int l );

    int elapsed() const {
        return state.elapsed;
    }
    void setElapsed ( int l );

signals:
    void stateChanged ( trackstate state );

};

#endif // __QTRACKSTATE_H__
