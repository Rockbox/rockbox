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

#include "qtrackstate.h"
#include <stdlib.h>

//
QTrackState::QTrackState(  )
        : QObject() {
    memset(&state,0,sizeof(state));
    state.title = (char*)"title";
    state.artist = (char*)"artist";
    state.album = (char*)"album";
    state.length = 100;
    state.elapsed = 50;
}

void QTrackState::setTitle(const QString& name) {
    state.title = new char[name.length()];
    strcpy(state.title,name.toAscii());
    emit stateChanged(state);
}

void QTrackState::setArtist(const QString& name) {
    state.artist = new char[name.length()];
    strcpy(state.artist,name.toAscii());
    emit stateChanged(state);
}

void QTrackState::setAlbum(const QString& name) {
    state.album = new char[name.length()];
    strcpy(state.album,name.toAscii());
    emit stateChanged(state);
}

void QTrackState::setLength(int le) {
    state.length = le;
    emit stateChanged(state);
}

void QTrackState::setElapsed(int le) {
    state.elapsed = le;
    emit stateChanged(state);
}
