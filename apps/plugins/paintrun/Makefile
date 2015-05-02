CC = gcc

OBJ = src/main.o

CFLAGS = -Isrc/ -O3 -std=gnu99 -g

SDLOBJ = platforms/sdl/sdl.o

SDLFLAGS = -lm -lSDL -lSDL_ttf -Iplatforms/sdl

paintrun-sdl: $(OBJ) $(SDLOBJ) Makefile
	@echo "LD $@"
	@$(CC) $(OBJ) $(SDLOBJ) -o $@ $(CFLAGS) $(SDLFLAGS) $(EXTRAFLAGS)

%.o: %.c Makefile
	@echo "CC $<"
	@$(CC) $(CFLAGS) $(SDLFLAGS) -c $< -o $@ $(EXTRAFLAGS)

clean:
	@echo "Cleaning build directory..."
	@rm -f $(OBJ) $(SDLOBJ) paintrun-sdl
