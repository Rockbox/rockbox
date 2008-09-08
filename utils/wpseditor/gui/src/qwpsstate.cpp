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

#include "qwpsstate.h"

QWpsState::QWpsState(): QObject() {
    state.fontheight = 8;
    state.fontwidth = 5;
    state.volume = -30;
    state.battery_level = 50;

}

void QWpsState::setFontHeight(int val) {
    state.fontheight = val;
    emit stateChanged(state);
}

void QWpsState::setFontWidth(int val) {
    state.fontwidth = val;
    emit stateChanged(state);
}

void QWpsState::setVolume(int val) {
    state.volume = val;
    emit stateChanged(state);
}

void QWpsState::setBattery(int val) {
    state.battery_level = val;
    emit stateChanged(state);
}
