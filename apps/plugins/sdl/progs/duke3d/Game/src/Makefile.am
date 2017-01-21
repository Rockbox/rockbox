SUBDIRS = audiolib midi

noinst_LIBRARIES = libGame.a

libGame_a_SOURCES = \
	actors.c animlib.c config.c console.c control.c cvar_defs.c cvars.c dummy_audiolib.c game.c \
	gamedef.c global.c keyboard.c menues.c player.c rts.c scriplib.c sector.c sounds.c
libGame_a_LIBADD = premap.o
libGame_a_CFLAGS = -I$(top_srcdir)/Engine/src

# Starting a new game will lead to a crash if premap.o was built with -O1/-O2/-O3,
# that's why we're compiling it seperately with -O0
premap.o: premap.c
	$(AM_V_CC)$(CC) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS) -I$(top_srcdir)/Engine/src -O0 -c $^ -o $@
