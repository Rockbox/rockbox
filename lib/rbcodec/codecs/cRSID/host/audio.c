
#ifdef CRSID_PLATFORM_PC

#include <SDL/SDL.h>


void cRSID_soundCallback(void* userdata, unsigned char *buf, int len) {
 cRSID_generateSound( (cRSID_C64instance*)userdata, buf, len );
}


void* cRSID_initSound(cRSID_C64instance* C64, unsigned short samplerate, unsigned short buflen) {
 static SDL_AudioSpec soundspec;
 if ( SDL_Init(SDL_INIT_AUDIO) < 0 ) {
  fprintf(stderr, "Couldn't initialize SDL: %s\n",SDL_GetError()); return NULL;
 }
 soundspec.freq=samplerate; soundspec.channels=1; soundspec.format=AUDIO_S16;
 soundspec.samples=buflen; soundspec.userdata=C64; soundspec.callback=cRSID_soundCallback;
 if ( SDL_OpenAudio(&soundspec, NULL) < 0 ) {
  fprintf(stderr, "Couldn't open audio: %s\n", SDL_GetError()); return NULL;
 }
 return (void*)&soundspec;
}


void cRSID_closeSound (void) {
 SDL_PauseAudio(1); SDL_CloseAudio();
}


void cRSID_startSound (void) {
 SDL_PauseAudio(0);
}


void cRSID_stopSound (void) {
 SDL_PauseAudio(1);
}


void cRSID_generateSound(cRSID_C64instance* C64instance, unsigned char *buf, unsigned short len) {
 static unsigned short i;
 static int Output;
 for(i=0;i<len;i+=2) {
  Output=cRSID_generateSample(C64instance); //cRSID_emulateC64(C64instance);
  //if (Output>=32767) Output=32767; else if (Output<=-32768) Output=-32768; //saturation logic on overflow
  buf[i]=Output&0xFF; buf[i+1]=Output>>8;
 }
}


#endif


static inline signed short cRSID_generateSample (cRSID_C64instance* C64) { //call this from custom buffer-filler
 static int Output;
 Output=cRSID_emulateC64(C64);
 if (C64->PSIDdigiMode) Output += cRSID_playPSIDdigi(C64);
 if (Output>=32767) Output=32767; else if (Output<=-32768) Output=-32768; //saturation logic on overflow
 return (signed short) Output;
}

