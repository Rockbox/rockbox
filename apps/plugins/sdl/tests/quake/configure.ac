#define true 1
#define false 0
#define true 1
#define false 0
dnl Process this file with autoconf to produce a configure script.
AC_PREREQ([2.69])
AC_INIT([SDL Quake],[1.0.9],[])
AC_CONFIG_AUX_DIR([.])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([README.SDL])
AC_CONFIG_HEADERS([config.h])

dnl Detect the canonical target build environment
dnl AC_CANONICAL_TARGET
AC_CANONICAL_HOST

dnl Setup for automake
AM_INIT_AUTOMAKE([tar-pax subdir-objects])

dnl Check for programs
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AM_PROG_AS
PKG_PROG_PKG_CONFIG

# Checks for libraries.

# Checks for header files.
AC_PATH_X
AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h netdb.h netinet/in.h stddef.h stdlib.h string.h sys/file.h sys/ioctl.h sys/param.h sys/socket.h sys/time.h termios.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_ALLOCA
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_CHECK_FUNCS([atexit floor gethostbyaddr gethostbyname gethostname gethrtime getpagesize gettimeofday inet_ntoa memset mkdir munmap pow select socket sqrt strcasecmp strchr strerror strstr])

#AS_IF([test "$GCC" = "yes"], [CFLAGS="-g -O6"], [])

dnl The alpha architecture needs special flags for binary portability
AS_CASE(
	[$host],
	[alpha*-*-linux*],[CFLAGS="$CFLAGS -mcpu=ev4 -Wa,-mall"],
	[x86_64-*-linux*],[CHECK_M32="yes" CFLAGS="$CFLAGS -ffast-math -funroll-loops -fomit-frame-pointer -fexpensive-optimizations"],
	[*-linux*],[CFLAGS="$CFLAGS -ffast-math -funroll-loops -fomit-frame-pointer -fexpensive-optimizations"],
	AC_MSG_ERROR(["Unrecognized platform $host."]))

#AS_IF([test "$CHECK_M32" = "yes"],
#	AX_CHECK_COMPILE_FLAG(
#		[-m32], 
#		[CFLAGS="-m32 $CFLAGS" CCASFLAGS="-m32 $CCASFLAGS"], 
#		AC_MSG_ERROR(["$CC does not build 32-bit programs correctly. See your distribution's documentation on cross-compiling to i386."])))

dnl Figure out which math and networking libraries to use
AS_CASE(
	[$host],
    	[*-*-mingw32*],[MATHLIB=''; INETLIB='-lwsock32'],
	[*-*-beos*],[MATHLIB=''; INETLIB=''],
	[MATHLIB='-lm'; INETLIB=''])
AC_SUBST(MATHLIB)
AC_SUBST(INETLIB)

dnl Check for SDL
PKG_CHECK_MODULES([SDL],
	[sdl >= 1.2.0],
	[CFLAGS="$CFLAGS $SDL_CFLAGS"; CPPFLAGS="$CPPFLAGS -DSDL"; LIBS="$LIBS $SDL_LIBS"],
	AC_MSG_ERROR([*** SDL version 1.2 not found!]))

dnl Set up the icon object file, for Mingw32
AS_CASE(
	[$host],
	[*-*-mingw32*],
	[cp ico_o ico.o; ICONOBJ="ico.o"],
	[ICONOBJ=""])
AC_SUBST(ICONOBJ)

dnl Enable/disable the i686 asm
AC_ARG_ENABLE(
	asm,
	AS_HELP_STRING(
		[--enable-asm],
		[enable i686 assembly routines (default=no)]))
AM_CONDITIONAL([MAKE_ASM], [test "$enable_asm" = yes])
AM_COND_IF([MAKE_ASM], 
	AC_DEFINE([USE_ASM],[],[Define to use i686 assembly sources]))

CPPFLAGS="$CPPFLAGS -DELF"

# Finally create all the generated files
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
