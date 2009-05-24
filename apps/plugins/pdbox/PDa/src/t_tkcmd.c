/* Copyright (c) 1997-1999 Miller Puckette.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#ifdef UNIX 	/* in unix this only works first; in NT it only works last. */
#include "tk.h"
#endif

#include "t_tk.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#ifdef UNIX
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#ifdef HAVE_BSTRING_H
#include <bstring.h>
#endif
#include <sys/time.h>
#include <errno.h>
#endif
#ifdef MSW
#include <winsock.h>
#include <io.h>
#endif
#ifdef MSW
#pragma warning( disable : 4305 )  /* uncast const double to float */
#pragma warning( disable : 4244 )  /* uncast double to float */
#pragma warning( disable : 4101 )  /* unused local variables */
#endif

#ifdef MSW
#include "tk.h"
#endif

void tcl_mess(char *s);

/***************** the socket setup code ********************/

static int portno = 5400;

    	/* some installations of linux don't know about "localhost" so give
	the loopback address; NT, on the other hand, can't understand the
	hostname "127.0.0.1". */
char hostname[100] =
#ifdef __linux__
    "127.0.0.1";
#else
    "localhost";
#endif

void pdgui_setsock(int port)
{
    portno = port;
}

void pdgui_sethost(char *name)
{
    strncpy(hostname, name, 100);
    hostname[99] = 0;
}

static void pdgui_sockerror(char *s)
{
#ifdef MSW
    int err = WSAGetLastError();
#endif
#ifdef UNIX
    int err = errno;
#endif

    fprintf(stderr, "%s: %s (%d)\n", s, strerror(err), err);
    tcl_mess("exit\n");
    exit(1);
}

static int sockfd;

/* The "pd_suck" command, which polls the socket.
    FIXME: if Pd sends something bigger than SOCKSIZE we're in trouble!
    This has to be set bigger than any array update message for instance.
*/
#define SOCKSIZE 20000

static char pd_tkbuf[SOCKSIZE+1];
int pd_spillbytes = 0;

static void pd_readsocket(ClientData cd, int mask)
{
    int ngot;
    fd_set readset, writeset, exceptset;
    struct timeval timout;

    timout.tv_sec = 0;
    timout.tv_usec = 0;
    FD_ZERO(&writeset);
    FD_ZERO(&readset);
    FD_ZERO(&exceptset);
    FD_SET(sockfd, &readset);
    FD_SET(sockfd, &exceptset);
    
    if (select(sockfd+1, &readset, &writeset, &exceptset, &timout) < 0)
    	perror("select");
    if (FD_ISSET(sockfd, &exceptset) || FD_ISSET(sockfd, &readset))
    {
	int ret;
	ret = recv(sockfd, pd_tkbuf + pd_spillbytes,
	    SOCKSIZE - pd_spillbytes, 0);
	if (ret < 0) pdgui_sockerror("socket receive error");
	else if (ret == 0)
	{
	    /* fprintf(stderr, "read %d\n", SOCKSIZE - pd_spillbytes); */
    	    fprintf(stderr, "pd_gui: pd process exited\n");
    	    tcl_mess("exit\n");
	}
	else
	{
	    char *lastcr = 0, *bp = pd_tkbuf, *ep = bp + (pd_spillbytes + ret);
	    int brace = 0;
	    char lastc = 0;
	    while (bp < ep)
	    {
	    	char c = *bp;
	    	if (c == '}' && brace) brace--;
	    	else if (c == '{') brace++;
	    	else if (!brace && c == '\n' && lastc != '\\') lastcr = bp;
	    	lastc = c;
	    	bp++;
	    }
	    if (lastcr)
	    {
	    	int xtra = pd_tkbuf + pd_spillbytes + ret - (lastcr+1);
	    	char bashwas = lastcr[1];
	    	lastcr[1] = 0;
	    	tcl_mess(pd_tkbuf);
	    	lastcr[1] = bashwas;
	    	if (xtra)
	    	{
	    	    /* fprintf(stderr, "x %d\n", xtra); */
	    	    memmove(pd_tkbuf, lastcr+1, xtra);
	    	}
	    	pd_spillbytes = xtra;
	    }
	    else
	    {
	    	pd_spillbytes += ret;
	    }
	}
    }
}

#ifndef UNIX
    /* if we aren't UNIX, we add a tcl command to poll the
    socket for data.  */
static int pd_pollsocketCmd(ClientData cd, Tcl_Interp *interp,
    int argc, char **argv)
{
    pd_readsocket(cd, 0);
    return (TCL_OK);
}
#endif

void pdgui_setupsocket(void)
{
    struct sockaddr_in server;
    struct hostent *hp;
#ifdef UNIX
    int retry = 10;
#else
    int retry = 1;
#endif
#ifdef MSW
    short version = MAKEWORD(2, 0);
    WSADATA nobby;

    if (WSAStartup(version, &nobby)) pdgui_sockerror("setup");
#endif

    /* create a socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) pdgui_sockerror("socket");

    /* connect socket using hostname provided in command line */
    server.sin_family = AF_INET;

    hp = gethostbyname(hostname);

    if (hp == 0)
    {
	fprintf(stderr,
	    "localhost not found (inet protocol not installed?)\n");
	exit(1);
    }
    memcpy((char *)&server.sin_addr, (char *)hp->h_addr, hp->h_length);

    /* assign client port number */
    server.sin_port = htons((unsigned short)portno);

	/* try to connect */
    while (1)
    {
    	if (connect(sockfd, (struct sockaddr *) &server, sizeof (server)) >= 0)
	    goto gotit;
	retry--;
	if (retry <= 0)
	    break;
    	  /* In UNIX there's a race condition; the child won't be
	  able to connect before the parent (pd) has shed its
	  setuid-ness.  In case this is the problem, sleep and
	  retry. */
    	else
	{
#ifdef UNIX
    	    fd_set readset, writeset, exceptset;
    	    struct timeval timout;

    	    timout.tv_sec = 0;
    	    timout.tv_usec = 100000;
    	    FD_ZERO(&writeset);
    	    FD_ZERO(&readset);
    	    FD_ZERO(&exceptset);
	    fprintf(stderr, "retrying connect...\n");
    	    if (select(1, &readset, &writeset, &exceptset, &timout) < 0)
    		perror("select");
#endif /* UNIX */
    	}
    }
    pdgui_sockerror("connecting stream socket");
gotit: ;
#ifdef UNIX
    	/* in unix we ask TK to call us back.  In NT we have to poll. */
    Tk_CreateFileHandler(sockfd, TK_READABLE | TK_EXCEPTION,
    	pd_readsocket, 0);
#endif /* UNIX */
}

/**************************** commands ************************/
static char *pdgui_path;

/* The "pd" command, which cats its args together and throws the result
* at the Pd interpreter.
*/
#define MAXWRITE 1024

static int pdCmd(ClientData cd, Tcl_Interp *interp, int argc,  char **argv)
{
    if (argc == 2)
    {
    	int n = strlen(argv[1]);
	if (send(sockfd, argv[1], n, 0) < n)
	{
    	    perror("stdout");
    	    tcl_mess("exit\n");
	}
    }
    else
    {
    	int i;
    	char buf[MAXWRITE];
    	buf[0] = 0;
    	for (i = 1; i < argc; i++)
    	{
    	    if (strlen(argv[i]) + strlen(buf) + 2 > MAXWRITE)
	    {
		interp->result = "pd: arg list too long";
		return (TCL_ERROR);	
	    }
	    if (i > 1) strcat(buf, " ");
	    strcat(buf, argv[i]);
	}
	if (send(sockfd, buf, strlen(buf), 0) < 0)
	{
    	    perror("stdout");
    	    tcl_mess("exit\n");
	}
    }
    return (TCL_OK);
}

/***********  "c" level access to tk functions. ******************/

static Tcl_Interp *tk_myinterp;

void tcl_mess(char *s)
{
    int result;
    result = Tcl_Eval(tk_myinterp,  s);
    if (result != TCL_OK)
    {
    	if (*tk_myinterp->result) printf("%s\n",  tk_myinterp->result);
    }
}

/* LATER should do a bounds check -- but how do you get printf to do that? */
void tcl_vmess(char *fmt, ...)
{
    int result, i;
    char buf[MAXWRITE];
    va_list ap;
    
    va_start(ap, fmt);

    vsprintf(buf, fmt, ap);
    result = Tcl_Eval(tk_myinterp, buf);
    if (result != TCL_OK)
    {
    	if (*tk_myinterp->result) printf("%s\n",  tk_myinterp->result);
    }
    va_end(ap);
}

#ifdef UNIX
void pdgui_doevalfile(Tcl_Interp *interp, char *s)
{
    char buf[GUISTRING];
    sprintf(buf, "set pd_guidir \"%s\"\n", pdgui_path);
    tcl_mess(buf);
    strcpy(buf, pdgui_path);
    strcat(buf, "/bin/");
    strcat(buf, s);
    if (Tcl_EvalFile(interp, buf) != TCL_OK)
    {
    	char buf2[1000];
    	sprintf(buf2, "puts [concat tcl: %s: can't open script]\n",
    	    buf);
    	tcl_mess(buf2);
    }
}

void pdgui_evalfile(char *s)
{
    pdgui_doevalfile(tk_myinterp, s);
}
#endif

void pdgui_startup(Tcl_Interp *interp)
{

    	/* save pointer to the main interpreter */
    tk_myinterp = interp;


    	/* add our own TK commands */
    Tcl_CreateCommand(interp, "pd",  (Tcl_CmdProc*)pdCmd, (ClientData)NULL, 
	(Tcl_CmdDeleteProc *)NULL); 
#ifndef UNIX
    Tcl_CreateCommand(interp, "pd_pollsocket",(Tcl_CmdProc*)  pd_pollsocketCmd,
    	(ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);
#endif
    pdgui_setupsocket();
    
	/* read in the startup file */
#if !defined(MSW) && !defined(MACOSX)
    pdgui_evalfile("pd.tk");
#endif
}

#ifdef UNIX
void pdgui_setname(char *s)
{
    char *t;
    char *str;
    int n;
    if (t = strrchr(s, '/')) str = s, n = (t-s) + 1;
    else str = "./", n = 2;
    if (n > GUISTRING-100) n = GUISTRING-100;
    pdgui_path = malloc(n+9);

    strncpy(pdgui_path, str, n);
    while (strlen(pdgui_path) > 0 && pdgui_path[strlen(pdgui_path)-1] == '/')
    	pdgui_path[strlen(pdgui_path)-1] = 0;
    if (t = strrchr(pdgui_path, '/'))
    	*t = 0;
}
#endif

int Pdtcl_Init(Tcl_Interp *interp)
{
	const char *myvalue = Tcl_GetVar(interp, "argv", 0);
	int myportno;
	if (myvalue && (myportno = atoi(myvalue)) > 1)
			pdgui_setsock(myportno);
	tk_myinterp = interp;
	pdgui_startup(interp); 
	interp->result = "loaded pdtcl_init";

	return (TCL_OK);
}

int Pdtcl_SafeInit(Tcl_Interp *interp) {
    fprintf(stderr, "Pdtcl_Safeinit 51\n");
	return (TCL_OK);
}

