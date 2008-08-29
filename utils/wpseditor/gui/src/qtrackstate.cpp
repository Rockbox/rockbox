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
