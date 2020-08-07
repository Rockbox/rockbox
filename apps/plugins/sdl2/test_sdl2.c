#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>

int my_main(int argc, char *argv[]) {
    rb->splashf(HZ, "point 0");
        if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
                fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
                return EXIT_FAILURE;
        }
    rb->splashf(HZ, "point 1");

        SDL_Window *win = SDL_CreateWindow("Hello World!", 100, 100, 620, 387, SDL_WINDOW_SHOWN);
        if (win == NULL) {
                fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
                return EXIT_FAILURE;
        }
    rb->splashf(HZ, "point 2");

        SDL_Renderer *ren = SDL_CreateRenderer(win, -1,SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (ren == NULL) {
                fprintf(stderr, "SDL_CreateRenderer Error: %s\n", SDL_GetError());
                if (win != NULL) {
                        SDL_DestroyWindow(win);
                }
                SDL_Quit();
                return EXIT_FAILURE;
        }
    rb->splashf(HZ, "point 3");

        SDL_Surface *bmp = SDL_LoadBMP("../img/grumpy-cat.bmp");
        if (bmp == NULL) {
                fprintf(stderr, "SDL_LoadBMP Error: %s\n", SDL_GetError());
                if (ren != NULL) {
                        SDL_DestroyRenderer(ren);
                }
                if (win != NULL) {
                        SDL_DestroyWindow(win);
                }
                SDL_Quit();
                return EXIT_FAILURE;
        }
    rb->splashf(HZ, "point 4");

        SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, bmp);
        if (tex == NULL) {
                fprintf(stderr, "SDL_CreateTextureFromSurface Error: %s\n", SDL_GetError());
                if (bmp != NULL) {
                        SDL_FreeSurface(bmp);
                }
                if (ren != NULL) {
                        SDL_DestroyRenderer(ren);
                }
                if (win != NULL) {
                        SDL_DestroyWindow(win);
                }
                SDL_Quit();
                return EXIT_FAILURE;
        }
        SDL_FreeSurface(bmp);
    rb->splashf(HZ, "point 5");

        for (int i=0; i < 20; i++) {
                        SDL_RenderClear(ren);
                        SDL_RenderCopy(ren, tex, NULL, NULL);
                        SDL_RenderPresent(ren);
                        SDL_Delay(100);
        }

        SDL_DestroyTexture(tex);
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        SDL_Quit();

        return EXIT_SUCCESS;
}
