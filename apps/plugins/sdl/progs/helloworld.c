#include "SDL.h"

#ifdef ROCKBOX
#ifndef COMBINED_SDL
#define main my_main
#else
#define main testsound_main
#endif
#else
#include <math.h>
#endif

double phase = 0;

void fill_audio(void *udata, Uint8 *stream, int len)
{
    for(int i = 0; i < len; ++i)
    {
        *stream++ = 127 * sin(phase);
        phase += M_PI / 100;
        if(phase > 2 * M_PI)
            phase -= 2 * M_PI;
    }
}

int main(int argc, char *argv[])
{
        SDL_Surface *screen;
        SDL_bool quit = SDL_FALSE, first_time = SDL_TRUE;
        SDL_Cursor *cursor[3];
        int current;

        /* Load the SDL library */
        if ( SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0 ) {
                fprintf(stderr, "Couldn't initialize SDL: %s\n",SDL_GetError());
                return(1);
        }

        screen = SDL_SetVideoMode(LCD_WIDTH,LCD_HEIGHT,24,SDL_ANYFORMAT);
        if (screen==NULL) {
                fprintf(stderr, "Couldn't initialize video mode: %s\n",SDL_GetError());
                return(1);
        }

        SDL_FillRect(screen, NULL, 0xff00ff);

        SDL_AudioSpec wanted;

        /* Set the audio format */
        wanted.freq = 44100;
        wanted.format = AUDIO_S8;
        wanted.channels = 2;    /* 1 = mono, 2 = stereo */
        wanted.samples = 1024;  /* Good low-latency value for callback */
        wanted.callback = fill_audio;
        wanted.userdata = NULL;


        /* Open the audio device, forcing the desired format */
        if ( SDL_OpenAudio(&wanted, NULL) < 0 ) {
            fprintf(stderr, "Couldn't open audio: %s\n", SDL_GetError());
            return(-1);
        }

        SDL_PauseAudio(0);

        while (!quit) {
                SDL_Event       event;
                while (SDL_PollEvent(&event)) {
                        switch(event.type) {
                        case SDL_MOUSEBUTTONDOWN:
                        case SDL_KEYDOWN:
                        case SDL_QUIT:
                            quit = SDL_TRUE;
                            break;
                        }
                }
                SDL_Flip(screen);
                SDL_Delay(10);
        }
        SDL_CloseAudio();

        SDL_Quit();
        return(0);
}
