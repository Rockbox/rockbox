/* config.h.  Generated automatically by configure.  */
/* config.h.in --- xscreensaver, Copyright (c) 1998 Jamie Zawinski.
 *
 *  The best way to set these parameters is by running the included `configure'
 *  script.  That examines your system, and generates `config.h' from 
 *  `config.h.in'.
 *
 *  If something goes very wrong, you can edit `config.h' directly, but beware
 *  that your changes will be lost if you ever run `configure' again.
 */


/* *************************************************************************
                          CONFIGURING SERVER EXTENSIONS
   ************************************************************************* */

/* Define this if you have the XReadDisplay extension (I think this is an
   SGI-only thing; it's in <X11/extensions/readdisplay.h>.)  A few of the
   screenhacks will take advantage of this if it's available.
 */
/* #undef HAVE_READ_DISPLAY_EXTENSION */

/* Define this if you have the Iris Video Library (dmedia/vl.h on SGI.)
   A few of the screenhacks will take advantage of this if it's available.
 */
/* #undef HAVE_SGI_VIDEO */

/* Define this if you have the XHPDisableReset function (an HP only thing.)
 */
/* #undef HAVE_XHPDISABLERESET */

/*  First, some background: there are three distinct server extensions which
 *  are useful to a screen saver program: they are XIDLE, MIT-SCREEN-SAVER, 
 *  and SCREEN_SAVER.
 *
 *  The XIDLE extension resides in .../contrib/extensions/xidle/ on the X11R5
 *  contrib tape.  This extension lets the client get accurate idle-time 
 *  information from the X server in a potentially more reliable way than by
 *  simply watching for keyboard and mouse activity.  However, the XIDLE 
 *  extension has apparently not been ported to X11R6.
 *
 *  The SCREEN_SAVER extension is found (as far as I know) only in the SGI
 *  X server, and it exists in all releases since (at least) Irix 5.  The
 *  relevant header file is /usr/include/X11/extensions/XScreenSaver.h.
 *
 *  The similarly-named MIT-SCREEN-SAVER extension came into existence long
 *  after the SGI SCREEN_SAVER extension was already in use, and resides in
 *  .../contrib/extensions/screensaver/ on the X11R6 contrib tape.  It is
 *  also found in certain recent X servers built in to NCD X terminals.
 *
 *     The MIT extension does basically the same thing that the XIDLE extension
 *     does, but there are two things wrong with it: first, because of the way
 *     the extension was designed, the `fade' option to XScreenSaver will be
 *     uglier: just before the screen fades out, there will be an unattractive
 *     flicker to black, because this extension blanks the screen *before*
 *     telling us that it is time to do so.  Second, this extension is known to
 *     be buggy; on the systems I use, it works, but some people have reported
 *     X server crashes as a result of using it.  XScreenSaver uses this
 *     extension rather conservatively, because when I tried to use any of its
 *     more complicated features, I could get it to crash the server at the
 *     drop of a hat.
 *
 *     In short, the MIT-SCREEN-SAVER extension is a piece of junk.  The older
 *     SGI SCREEN_SAVER extension works great, as does XIDLE.  It would be nice
 *     If those two existed on more systems, that is, would be adopted by the
 *     X Consortium in favor of their inferior "not-invented-here" entry.
 */

/*  Define this if you have the XIDLE extension installed. If you have the
 *  XIDLE extension, this is recommended.  (You have this extension if the
 *  file /usr/include/X11/extensions/xidle.h exists.)  Turning on this flag
 *  lets XScreenSaver work better with servers which support this extension; 
 *  but it will still work with servers which do not suport it, so it's a good
 *  idea to compile in support for it if you can.
 */
/* #undef HAVE_XIDLE_EXTENSION */

/*  Define this if you have the MIT-SCREEN-SAVER extension installed.  See the
 *  caveats about this extension, above.  (It's available if the file
 *  /usr/include/X11/extensions/scrnsaver.h exists.)
 */
#define HAVE_MIT_SAVER_EXTENSION 1

/*  Define this if you have the SGI SCREEN_SAVER extension.  This is standard
 *  on Irix systems, and not available elsewhere.
 */
/* #undef HAVE_SGI_SAVER_EXTENSION */

/*  Define this if you have the SGI-VIDEO-CONTROL extension.  This is standard
 *  on Irix systems, and not available elsewhere.
 */
/* #undef HAVE_SGI_VC_EXTENSION */

/*  Define this if you have the XDPMS extension.  This is standard on
 *  sufficiently-recent XFree86 systems, and possibly elsewhere.  (It's
 *  available if the file /usr/include/X11/extensions/dpms.h exists.)
 */
/* #undef HAVE_DPMS_EXTENSION */

/*  Define this if you have the functions XF86VidModeGetModeLine() and
 *  XF86VidModeGetViewPort(), in support of virtual desktops where the
 *  X server's root window is bigger than the actual screen.  This is
 *  an XFree86 thing, and probably doesn't exist elsewhere.  (It's
 *  available if the file /usr/include/X11/extensions/xf86vmode.h exists.)
 */
/* #undef HAVE_XF86VMODE */

/*  Define this if you have a Linux-like /proc/interrupts file which can be
 *  examined to determine when keyboard activity has occurred.
 */
/* #undef HAVE_PROC_INTERRUPTS */



/* *************************************************************************
                          CONFIGURING GRAPHICS TOOLKITS
   ************************************************************************* */

/*  Define this if you have Motif.
 */
#define HAVE_MOTIF 1

/*  Define this if you have Gtk.
 */
/* #undef HAVE_GTK */

/*  Define this if you have Athena (-Xaw).
 */
#define HAVE_ATHENA 1

/*  Define this if you have Athena, and the version you have includes the
 *  XawViewportSetCoordinates function in Viewport.h (some old versions of
 *  the library didn't have this function.)
 */
#define HAVE_XawViewportSetCoordinates 1

/*  Define this if you have the XPM library installed.  Some of the demos can
 *  make use of this if it is available.
 */
#define HAVE_XPM 1

/*  Define this if you have the Xmu library.  This is standard part of X, and
 *  if your vendor doesn't ship it, you should report that as a bug.
 */
#define HAVE_XMU 1

/*  Define this if you have OpenGL.  Some of the demos require it, so if you
 *  don't have it, then those particular demos won't be built.  (This won't
 *  affect the screen saver as a whole.)
 */
/* #undef HAVE_GL */

/*  Define this if you have OpenGL, but it's the MesaGL variant.  (The 
    libraries have different names.)  (HAVE_GL should be defined too.)
 */
/* #undef HAVE_MESA_GL */

/*  Define this if your version of OpenGL has the glBindTexture() routine.
    This is the case for OpenGL 1.1, but not for OpenGL 1.0.
 */
/* #undef HAVE_GLBINDTEXTURE */

/*  Define this if you have the -lgle and -lmatrix libraries (GL extrusion.)
 */
/* #undef HAVE_GLE */

/*  Define this if the `xscreensaver' process itself (the driver process)
    should be linked against GL.  Most systems won't want this (in particular,
    if you're using Linux and/or Mesa, you don't want this) but SGI systems
    do want this.  It may also be useful on other systems that have serious
    GL support -- you only need this if you have a lot of different visuals,
    not all of which work with GL programs.
 */
/* #undef DAEMON_USE_GL */

/*  Define this if you have the X Shared Memory Extension.
 */
#define HAVE_XSHM_EXTENSION 1

/*  Define this if you have the X Double Buffer Extension.
 */
#define HAVE_DOUBLE_BUFFER_EXTENSION 1

/*  Some screenhacks like to run an external program to generate random pieces
    of text; set this to the one you like ("yow" and "fortune" are the most
    likely prospects.)  Note that this is just the default; X resources can
    be used to override it.
 */
#define ZIPPY_PROGRAM "/usr/local/libexec/emacs/20.4/sparc-sun-solaris2.6/yow"



/* *************************************************************************
                       CONFIGURING PASSWORD AUTHENTICATION
   ************************************************************************* */

/* Define this to remove the option of locking the screen at all.
 */
/* #undef NO_LOCKING */

/*  Define this if you want to use Kerberos authentication to lock/unlock the
 *  screen instead of your local password.  This currently uses Kerberos V4, 
 *  but a V5 server with V4 compatibility will work.  WARNING: DO NOT USE AFS
 *  string-to-key passwords with this option. This option currently *only* 
 *  works with standard Kerberos des_string_to_key.  If your password is an
 *  AFS password and not a kerberos password, it will not authenticate 
 *  properly. See the comments in driver/kpasswd.c for more information if you
 *  need it. 
 */
/* #undef HAVE_KERBEROS */

/*  Define this if you want to use PAM (Pluggable Authentication Modules)
 *  to lock/unlock the screen, instead of standard /etc/passwd authentication.
 */
/* #undef HAVE_PAM */

/*  If PAM is being used, this is the name of the PAM service that xscreensaver
 *  will authenticate as.  The default is "xscreensaver", which means that the
 *  PAM library will look for an "xscreensaver" line in /etc/pam.conf, or (on
 *  recent Linux systems) will look for a file called /etc/pam.d/xscreensaver.
 *  Some systems might already have a PAM installation that is configured for
 *  xlock, so setting this to "xlock" would also work in that case.
 */
#define PAM_SERVICE_NAME "xscreensaver"

/* Define if you have PAM and pam_strerror() requires two arguments.  */
/* #undef PAM_STRERROR_TWO_ARGS */

/*  Define this if your system uses `shadow' passwords, that is, the passwords
 *  live in /etc/shadow instead of /etc/passwd, and one reads them with
 *  getspnam() instead of getpwnam().  (Note that SCO systems do some random
 *  other thing; others might as well.  See the ifdefs in driver/passwd-pwent.c
 *  if you're having trouble related to reading passwords.)
 */
#define HAVE_SHADOW_PASSWD 1

/*  Define this if your system is Digital or SCO Unix with so-called ``Enhanced
    Security'', that is, the passwords live in /tcb/files/auth/<x>/<xyz> 
    instead of in /etc/passwd, and one reads them with getprpwnam() instead 
    of getpwnam().
 */
/* #undef HAVE_ENHANCED_PASSWD */

/*  Define this if your system is Solaris with ``adjunct'' passwords (this is
    the version where one gets at the passwords with getpwanam() instead of
    getpwnam().)  I haven't tested this one, let me know if it works.
 */
/* #undef HAVE_ADJUNCT_PASSWD */

/*  Define this if you are running HPUX with so-called ``Secure Passwords'' 
    (if you have /usr/include/hpsecurity.h, you probably have this.)  I
    haven't tested this one, let me know if it works.
 */
/* #undef HAVE_HPUX_PASSWD */

/*  Define this if you are on a system that supports the VT_LOCKSWITCH and
    VT_UNLOCKSWITCH ioctls.  If this is defined, then when the screen is
    locked, switching to another virtual terminal will also be prevented.
    That is, the whole console will be locked, rather than just the VT on
    which X is running.  (Well, that's the theory anyway -- in practice,
    I haven't yet figured out how to make that work.)
 */
/* #undef HAVE_VT_LOCKSWITCH */


/* Define this if you the openlog(), syslog(), and closelog() functions.
   This is used for logging failed login attempts.
 */
#define HAVE_SYSLOG 1


/* *************************************************************************
                            OTHER C ENVIRONMENT JUNK
   ************************************************************************* */

/* Define this to void* if you're using X11R4 or earlier.  */
/* #undef XPointer */

/* Define if you have the nice function.  */
#define HAVE_NICE 1

/* Define if you have the setpriority function.  */
#define HAVE_SETPRIORITY 1

/* Define to empty if the keyword does not work.  */
/* #undef const */

/* Define if you have <sys/wait.h> that is POSIX.1 compatible.  */
#define HAVE_SYS_WAIT_H 1

/* Define as __inline if that's what the C compiler calls it.  */
/* #undef inline */

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef mode_t */

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef pid_t */

/* Define as the return type of signal handlers (int or void).  */
#define RETSIGTYPE void

/* Define to `unsigned' if <sys/types.h> doesn't define.  */
/* #undef size_t */

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* Define if you can safely include both <sys/time.h> and <time.h>.  */
#define TIME_WITH_SYS_TIME 1

/* Define if you have the gettimeofday function.  */
#define HAVE_GETTIMEOFDAY 1

/* Define if gettimeofday requires two arguments.  */
#define GETTIMEOFDAY_TWO_ARGS 1

/* Define if you have the putenv function.  */
#define HAVE_PUTENV 1

/* Define if you have the select function.  */
#define HAVE_SELECT 1

/* Define if you have the getcwd function.  */
#define HAVE_GETCWD 1

/* Define if you have the getcwd function.  */
#define HAVE_GETWD 1

/* Define if you have the realpath function.  */
#define HAVE_REALPATH 1

/* Define if you have the uname function.  */
#define HAVE_UNAME 1

/* Define if you have the fcntl function.  */
#define HAVE_FCNTL 1

/* Define if you have the sigaction function.  */
#define HAVE_SIGACTION 1

/* Define if you have the <unistd.h> header file.  */
#define HAVE_UNISTD_H 1

/* Define if you have the <crypt.h> header file.  */
#define HAVE_CRYPT_H 1

/* Define if you have <sys/select.h> that defines fd_set and FD_SET.  */
#define HAVE_SYS_SELECT_H 1

/* Define to use sigaction() instead of signal() for SIGCHLD-related activity.
   This is necessary at least on SCO OpenServer 5, due to a Unix kernel bug.
 */
/* #undef USE_SIGACTION */

/* Define this if you do pings with a `struct icmp' and  a `icmp_id' slot.
 */
#define HAVE_ICMP 1

/* Define this if you do pings with a `struct icmphdr' and a `un.echo.id' slot.
 */
/* #undef HAVE_ICMPHDR */
