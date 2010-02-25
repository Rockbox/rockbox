/*
Written by Matt Wright, The Center for New Music and Audio Technologies,
University of California, Berkeley.  Copyright (c) 1996,97,98,99,2000,01,02,03
The Regents of the University of California (Regents).  

Permission to use, copy, modify, distribute, and distribute modified versions
of this software and its documentation without fee and without a signed
licensing agreement, is hereby granted, provided that the above copyright
notice, this paragraph and the following two paragraphs appear in all copies,
modifications, and distributions.

IN NO EVENT SHALL REGENTS BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT,
SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS, ARISING
OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF REGENTS HAS
BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

REGENTS SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE. THE SOFTWARE AND ACCOMPANYING DOCUMENTATION, IF ANY, PROVIDED
HEREUNDER IS PROVIDED "AS IS". REGENTS HAS NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


The OSC webpage is http://cnmat.cnmat.berkeley.edu/OpenSoundControl
*/


/* sendOSC.c

    Matt Wright, 6/3/97
    based on sendOSC.c, which was based on a version by Adrian Freed

    Text-based OpenSoundControl client.  User can enter messages via command
    line arguments or standard input.

    Version 0.1: "play" feature
    Version 0.2: Message type tags.
   
    pd version branched from http://www.cnmat.berkeley.edu/OpenSoundControl/src/sendOSC/sendOSC.c
    -------------
    -- added bundle stuff to send. jdl 20020416
    -- tweaks for Win32    www.zeggz.com/raf	13-April-2002
    -- ost_at_test.at + i22_at_test.at, 2000-2002
       modified to compile as pd externel
*/

#define MAX_ARGS 2000
#define SC_BUFFER_SIZE 64000

#include "../src/m_pd.h"
#include "OSC-client.h"

#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef WIN32
#include <winsock2.h>	
#include <io.h>    
#include <errno.h>
#include <fcntl.h>
#include <winsock2.h>	
#include <ctype.h>
#include <signal.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <rpc/rpc.h>
#include <sys/times.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <netdb.h>
#endif

#ifdef __APPLE__
  #include <string.h>
#endif

#define UNIXDG_PATH "/tmp/htm"
#define UNIXDG_TMP "/tmp/htm.XXXXXX"



OSCTimeTag OSCTT_Immediately(void) {
    OSCTimeTag result;
    result.seconds = 0;
    result.fraction = 1;
    return result;
}


OSCTimeTag OSCTT_CurrentTime(void) {
    OSCTimeTag result;
    result.seconds = 0;
    result.fraction = 1;
    return result;
}

OSCTimeTag OSCTT_PlusSeconds(OSCTimeTag original, float secondsOffset) {
    OSCTimeTag result;
    result.seconds = 0;
    result.fraction = 1;
    return result;
}


typedef int bool;

typedef struct
{
	float srate;

	struct sockaddr_in serv_addr; /* udp socket */
	#ifndef WIN32
		struct sockaddr_un userv_addr; /* UNIX socket */
	#endif
	int sockfd;		/* socket file descriptor */
	int index, len,uservlen;
	void *addr;
	int id;
} desc;


/* open a socket for HTM communication to given  host on given portnumber */
/* if host is 0 then UNIX protocol is used (i.e. local communication */
void *OpenHTMSocket(char *host, int portnumber)
{
	struct sockaddr_in  cl_addr;
	#ifndef WIN32
		int sockfd;
		struct sockaddr_un ucl_addr;
	#else
		unsigned int sockfd;
	#endif

	desc *o;
	int oval = 1;
	o = malloc(sizeof(*o));
	if(!o) return 0;

  #ifndef WIN32

	if(!host)
	{
		char *mktemp(char *);
		int clilen;
		  o->len = sizeof(ucl_addr);
		/*
		         * Fill in the structure "userv_addr" with the address of the
		         * server that we want to send to.
		*/
		
		bzero((char *) &o->userv_addr, sizeof(o->userv_addr));
		       o->userv_addr.sun_family = AF_UNIX;
		strcpy(o->userv_addr.sun_path, UNIXDG_PATH);
			sprintf(o->userv_addr.sun_path+strlen(o->userv_addr.sun_path), "%d", portnumber);
	        o->uservlen = sizeof(o->userv_addr.sun_family) + strlen(o->userv_addr.sun_path);
		o->addr = &(o->userv_addr);
		/*
		* Open a socket (a UNIX domain datagram socket).
		*/
		
		if ( (sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) >= 0)
		{
			/*
			 * Bind a local address for us.
			 * In the UNIX domain we have to choose our own name (that
			 * should be unique).  We'll use mktemp() to create a unique
			 * pathname, based on our process id.
			 */
		
			bzero((char *) &ucl_addr, sizeof(ucl_addr));    /* zero out */
			ucl_addr.sun_family = AF_UNIX;
			strcpy(ucl_addr.sun_path, UNIXDG_TMP);

			mktemp(ucl_addr.sun_path);
			clilen = sizeof(ucl_addr.sun_family) + strlen(ucl_addr.sun_path);
		
			if (bind(sockfd, (struct sockaddr *) &ucl_addr, clilen) < 0)
			{
			  perror("client: can't bind local address");
			  close(sockfd);
			  sockfd = -1;
			}
		}
		else
		  perror("unable to make socket\n");
		
	}else

  #endif

	{
		/*
		         * Fill in the structure "serv_addr" with the address of the
		         * server that we want to send to.
		*/
		o->len = sizeof(cl_addr);

		#ifdef WIN32
			ZeroMemory((char *)&o->serv_addr, sizeof(o->serv_addr));
		#else
			bzero((char *)&o->serv_addr, sizeof(o->serv_addr));
		#endif

		o->serv_addr.sin_family = AF_INET;

	    /* MW 6/6/96: Call gethostbyname() instead of inet_addr(),
	       so that host can be either an Internet host name (e.g.,
	       "les") or an Internet address in standard dot notation 
	       (e.g., "128.32.122.13") */
	    {
			struct hostent *hostsEntry;
			unsigned long address;

			hostsEntry = gethostbyname(host);
			if (hostsEntry == NULL) {
				fprintf(stderr, "Couldn't decipher host name \"%s\"\n", host);
				#ifndef WIN32
				herror(NULL);
				#endif
				return 0;
			}			
			address = *((unsigned long *) hostsEntry->h_addr_list[0]);
			o->serv_addr.sin_addr.s_addr = address;
	    }

	    /* was: o->serv_addr.sin_addr.s_addr = inet_addr(host); */

	    /* End MW changes */

		/*
		 * Open a socket (a UDP domain datagram socket).
		 */


		#ifdef WIN32
			o->serv_addr.sin_port = htons((USHORT)portnumber);
			o->addr = &(o->serv_addr);
			if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) != INVALID_SOCKET) {
				ZeroMemory((char *)&cl_addr, sizeof(cl_addr));
				cl_addr.sin_family = AF_INET;
				cl_addr.sin_addr.s_addr = htonl(INADDR_ANY);
				cl_addr.sin_port = htons(0);
				
				// enable broadcast: jdl ~2003
				if(setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &oval, sizeof(int)) == -1) {
				  perror("setsockopt");
				}

				if(bind(sockfd, (struct sockaddr *) &cl_addr, sizeof(cl_addr)) < 0) {
					perror("could not bind\n");
					closesocket(sockfd);
					sockfd = -1;
				}
			}
			else { perror("unable to make socket\n");}
		#else
			o->serv_addr.sin_port = htons(portnumber);
			o->addr = &(o->serv_addr);
			if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0) {
					bzero((char *)&cl_addr, sizeof(cl_addr));
				cl_addr.sin_family = AF_INET;
				cl_addr.sin_addr.s_addr = htonl(INADDR_ANY);
				cl_addr.sin_port = htons(0);
				
				// enable broadcast: jdl ~2003
				if(setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &oval, sizeof(int)) == -1) {
				  perror("setsockopt");
				}

				if(bind(sockfd, (struct sockaddr *) &cl_addr, sizeof(cl_addr)) < 0) {
					perror("could not bind\n");
					close(sockfd);
					sockfd = -1;
				}
			}
			else { perror("unable to make socket\n");}
		#endif
	}
	#ifdef WIN32
		if(sockfd == INVALID_SOCKET) {
	#else
		if(sockfd < 0) {
	#endif
			free(o); 
			o = 0;
		}
		else
			o->sockfd = sockfd;
	return o;
}

static  bool sendudp(const struct sockaddr *sp, int sockfd,int length, int count, void  *b)
{
	int rcount;
	if((rcount=sendto(sockfd, b, count, 0, sp, length)) != count)
	{
		printf("sockfd %d count %d rcount %dlength %d\n", sockfd,count,rcount,length); 
		return FALSE;
	}
	return TRUE;
}

bool SendHTMSocket(void *htmsendhandle, int length_in_bytes, void *buffer)
{
	desc *o = (desc *)htmsendhandle;
	return sendudp(o->addr, o->sockfd, o->len, length_in_bytes, buffer);
}
void CloseHTMSocket(void *htmsendhandle)
{
	desc *o = (desc *)htmsendhandle;
	#ifdef WIN32
	if(SOCKET_ERROR == closesocket(o->sockfd)) {
		perror("CloseHTMSocket::closesocket failed\n");
		return;
	}
	#else
	if(close(o->sockfd) == -1)
	  {
	    perror("CloseHTMSocket::closesocket failed");
	    return;
	  }
	#endif

	free(o);
}


///////////////////////
// from sendOSC

typedef struct {
    //enum {INT, FLOAT, STRING} type;
    enum {INT_osc, FLOAT_osc, STRING_osc} type;
    union {
        int i;
        float f;
        char *s;
    } datum;
} typedArg;

void CommandLineMode(int argc, char *argv[], void *htmsocket);
OSCTimeTag ParseTimeTag(char *s);
void ParseInteractiveLine(OSCbuf *buf, char *mesg);
typedArg ParseToken(char *token);
int WriteMessage(OSCbuf *buf, char *messageName, int numArgs, typedArg *args);
void SendBuffer(void *htmsocket, OSCbuf *buf);
void SendData(void *htmsocket, int size, char *data);
/* defined in OSC-system-dependent.c now */

//static void *htmsocket;
static int exitStatus = 0;  
static int useTypeTags = 0;

static char bufferForOSCbuf[SC_BUFFER_SIZE];


/////////
// end from sendOSC

static t_class *sendOSC_class;

typedef struct _sendOSC
{
  t_object x_obj;
  int x_protocol;      // UDP/TCP (udp only atm)
  t_int x_typetags;    // typetag flag
  void *x_htmsocket;   // sending socket
  int x_bundle;        // bundle open flag
  OSCbuf x_oscbuf[1];  // OSCbuffer
  t_outlet *x_bdpthout;// bundle-depth floatoutlet
} t_sendOSC;

static void *sendOSC_new(t_floatarg udpflag)
{
    t_sendOSC *x = (t_sendOSC *)pd_new(sendOSC_class);
    outlet_new(&x->x_obj, &s_float);
    x->x_htmsocket = 0;		// {{raf}}
    // set udp
    x->x_protocol = SOCK_STREAM;
    // set typetags to 1 by default
    x->x_typetags = 1;
    // bunlde is closed
    x->x_bundle   = 0;
    OSC_initBuffer(x->x_oscbuf, SC_BUFFER_SIZE, bufferForOSCbuf);
    x->x_bdpthout = outlet_new(&x->x_obj, 0); // outlet_float();
    //x->x_oscbuf   =
    return (x);
}


void sendOSC_openbundle(t_sendOSC *x)
{
  if (x->x_oscbuf->bundleDepth + 1 >= MAX_BUNDLE_NESTING ||
      OSC_openBundle(x->x_oscbuf, OSCTT_Immediately()))
    {
    post("Problem opening bundle: %s\n", OSC_errorMessage);
    return;
  }
  x->x_bundle = 1;
  outlet_float(x->x_bdpthout, (float)x->x_oscbuf->bundleDepth);
}

static void sendOSC_closebundle(t_sendOSC *x)
{
  if (OSC_closeBundle(x->x_oscbuf)) {
    post("Problem closing bundle: %s\n", OSC_errorMessage);
    return;
  }
  outlet_float(x->x_bdpthout, (float)x->x_oscbuf->bundleDepth);
  // in bundle mode we send when bundle is closed?
  if(!OSC_isBufferEmpty(x->x_oscbuf) > 0 && OSC_isBufferDone(x->x_oscbuf)) {
    // post("x_oscbuf: something inside me?");
    if (x->x_htmsocket) {
      SendBuffer(x->x_htmsocket, x->x_oscbuf);
    } else {
      post("sendOSC: not connected");
    }
    OSC_initBuffer(x->x_oscbuf, SC_BUFFER_SIZE, bufferForOSCbuf);
    x->x_bundle = 0;
    return;
  }
  // post("x_oscbuf: something went wrong");
}

static void sendOSC_settypetags(t_sendOSC *x, t_float *f)
 {
   x->x_typetags = (int)f;
   post("sendOSC.c: setting typetags %d",x->x_typetags);
 }


static void sendOSC_connect(t_sendOSC *x, t_symbol *hostname,
			    t_floatarg fportno)
{
	int portno = fportno;
	/* create a socket */

	//	make sure handle is available
	if(x->x_htmsocket == 0) {
		//
		x->x_htmsocket = OpenHTMSocket(hostname->s_name, portno);
		if (!x->x_htmsocket)
			post("Couldn't open socket: ");
		else {
			post("connected to port %s:%d (hSock=%d)",  hostname->s_name, portno, x->x_htmsocket);
			outlet_float(x->x_obj.ob_outlet, 1);
		}
	}
	else 
		perror("call to sendOSC_connect() against UNavailable socket handle");
}

void sendOSC_disconnect(t_sendOSC *x)
{
  if (x->x_htmsocket)
    {
      post("disconnecting htmsock (hSock=%d)...", x->x_htmsocket);
      CloseHTMSocket(x->x_htmsocket);
	  x->x_htmsocket = 0;	// {{raf}}  semi-quasi-semaphorize this
      outlet_float(x->x_obj.ob_outlet, 0);
    }
  else {
	perror("call to sendOSC_disconnect() against unused socket handle");
  }
}

void sendOSC_senduntyped(t_sendOSC *x, t_symbol *s, int argc, t_atom *argv)
{
  char* targv[MAXPDARG];
  char tmparg[MAXPDSTRING];
  char* tmp = tmparg;
  //char testarg[MAXPDSTRING];
  int c;

  post("sendOSC: use typetags 0/1 message and plain send method so send untypetagged...");
  return;

  //atom_string(argv,testarg, MAXPDSTRING);
  for (c=0;c<argc;c++) {
    atom_string(argv+c,tmp, 80);
    targv[c] = tmp;
    tmp += strlen(tmp)+1;
  }

  // this sock needs to be larger than 0, not >= ..
  if (x->x_htmsocket)
    {
      CommandLineMode(argc, targv, x->x_htmsocket);
      //      post("test %d", c);
    }
  else {
    post("sendOSC: not connected");
  }
}

//////////////////////////////////////////////////////////////////////
// this is the real and only sending routine now, for both typed and
// undtyped mode.

static void sendOSC_sendtyped(t_sendOSC *x, t_symbol *s, int argc, t_atom *argv)
{
  char* targv[MAX_ARGS];
  char tmparg[MAXPDSTRING];
  char* tmp = tmparg;
  int c;

  char *messageName;
  char *token;
  typedArg args[MAX_ARGS];
  int i,j;
  int numArgs = 0;

  messageName = "";
#ifdef DEBUG
  post ("sendOSC: messageName: %s", messageName);
#endif


  
  for (c=0;c<argc;c++) {
    atom_string(argv+c,tmp, 80);

#ifdef DEBUG
    // post ("sendOSC: %d, %s",c, tmp);
#endif

    targv[c] = tmp;
    tmp += strlen(tmp)+1;

#ifdef DEBUG
    // post ("sendOSC: %d, %s",c, targv[c]);
#endif
  }

  // this sock needs to be larger than 0, not >= ..
  if (x->x_htmsocket > 0)
    { 
#ifdef DEBUG
    post ("sendOSC: type tags? %d", useTypeTags);
#endif 

      messageName = strtok(targv[0], ",");
      j = 1;
      for (i = j; i < argc; i++) {
	token = strtok(targv[i],",");
	args[i-j] = ParseToken(token);
#ifdef DEBUG
	printf("cell-cont: %s\n", targv[i]);
	printf("  type-id: %d\n", args[i-j]);
#endif
	numArgs = i;
      }
      

      if(WriteMessage(x->x_oscbuf, messageName, numArgs, args)) {
	post("sendOSC: usage error, write-msg failed: %s", OSC_errorMessage);
	return;
      }
      
      if(!x->x_bundle) {
	SendBuffer(x->x_htmsocket, x->x_oscbuf);
	OSC_initBuffer(x->x_oscbuf, SC_BUFFER_SIZE, bufferForOSCbuf);
      }
      
      //CommandLineMode(argc, targv, x->x_htmsocket);
      //useTypeTags = 0;
    }
  else {
    post("sendOSC: not connected");
  }
}

void sendOSC_send(t_sendOSC *x, t_symbol *s, int argc, t_atom *argv) 
{
  if(!argc) {
    post("not sending empty message.");
    return;
  }
  if(x->x_typetags) {
    useTypeTags = 1;
    sendOSC_sendtyped(x,s,argc,argv);
    useTypeTags = 0;
  } else {
    sendOSC_sendtyped(x,s,argc,argv);
  }
}

static void sendOSC_free(t_sendOSC *x)
{
    sendOSC_disconnect(x);
}

#ifdef WIN32
	OSC_API void sendOSC_setup(void) { 
#else
	void sendOSC_setup(void) {
#endif
    sendOSC_class = class_new(gensym("sendOSC"), (t_newmethod)sendOSC_new,
			      (t_method)sendOSC_free,
			      sizeof(t_sendOSC), 0, A_DEFFLOAT, 0);
    class_addmethod(sendOSC_class, (t_method)sendOSC_connect,
		    gensym("connect"), A_SYMBOL, A_FLOAT, 0);
    class_addmethod(sendOSC_class, (t_method)sendOSC_disconnect,
		    gensym("disconnect"), 0);
    class_addmethod(sendOSC_class, (t_method)sendOSC_settypetags,
		    gensym("typetags"),
		    A_FLOAT, 0);
    class_addmethod(sendOSC_class, (t_method)sendOSC_send,
		    gensym("send"),
		    A_GIMME, 0);
    class_addmethod(sendOSC_class, (t_method)sendOSC_send,
		    gensym("senduntyped"),
		    A_GIMME, 0);
    class_addmethod(sendOSC_class, (t_method)sendOSC_send,
		    gensym("sendtyped"),
		    A_GIMME, 0);
    class_addmethod(sendOSC_class, (t_method)sendOSC_openbundle,
		    gensym("["),
		    0, 0);
    class_addmethod(sendOSC_class, (t_method)sendOSC_closebundle,
		    gensym("]"),
		    0, 0);
    class_sethelpsymbol(sendOSC_class, gensym("sendOSC-help.pd"));
}





/* Exit status codes:
    0: successful
    2: Message(s) dropped because of buffer overflow
    3: Socket error
    4: Usage error
    5: Internal error
*/

void CommandLineMode(int argc, char *argv[], void *htmsocket) {
    char *messageName;
    char *token;
    typedArg args[MAX_ARGS];
    int i,j, numArgs;
    OSCbuf buf[1];

  OSC_initBuffer(buf, SC_BUFFER_SIZE, bufferForOSCbuf);

  if (argc > 1) {
    post("argc (%d) > 1", argc);
    }

  //    ParseInteractiveLine(buf, argv);
  messageName = strtok(argv[0], ",");

    j = 1;
    for (i = j; i < argc; i++) {
      token = strtok(argv[i],",");
      args[i-j] = ParseToken(token);
#ifdef DEBUG
      printf("cell-cont: %s\n", argv[i]);
      printf("  type-id: %d\n", args[i-j]);
#endif
      numArgs = i;
    }

    if(WriteMessage(buf, messageName, numArgs, args)) {
      post("sendOSC: usage error. write-msg failed: %s", OSC_errorMessage);
      return;
    }

    SendBuffer(htmsocket, buf);
}

#define MAXMESG 2048

void InteractiveMode(void *htmsocket) {
    char mesg[MAXMESG];
    OSCbuf buf[1];
    int bundleDepth = 0;    /* At first, we haven't seen "[". */

    OSC_initBuffer(buf, SC_BUFFER_SIZE, bufferForOSCbuf);

    while (fgets(mesg, MAXMESG, stdin) != NULL) {
        if (mesg[0] == '\n') {
	  if (bundleDepth > 0) {
	    /* Ignore blank lines inside a group. */
	  } else {
            /* blank line => repeat previous send */
            SendBuffer(htmsocket, buf);
	  }
	  continue;
        }

	if (bundleDepth == 0) {
	    OSC_resetBuffer(buf);
	}

	if (mesg[0] == '[') {
	    OSCTimeTag tt = ParseTimeTag(mesg+1);
	    if (OSC_openBundle(buf, tt)) {
		post("Problem opening bundle: %s\n", OSC_errorMessage);
		OSC_resetBuffer(buf);
		bundleDepth = 0;
		continue;
	    }
	    bundleDepth++;
        } else if (mesg[0] == ']' && mesg[1] == '\n' && mesg[2] == '\0') {
            if (bundleDepth == 0) {
                post("Unexpected ']': not currently in a bundle.\n");
            } else {
		if (OSC_closeBundle(buf)) {
		    post("Problem closing bundle: %s\n", OSC_errorMessage);
		    OSC_resetBuffer(buf);
		    bundleDepth = 0;
		    continue;
		}

		bundleDepth--;
		if (bundleDepth == 0) {
		    SendBuffer(htmsocket, buf);
		}
            }
        } else {
            ParseInteractiveLine(buf, mesg);
            if (bundleDepth != 0) {
                /* Don't send anything until we close all bundles */
            } else {
                SendBuffer(htmsocket, buf);
            }
        }
    }
}

OSCTimeTag ParseTimeTag(char *s) {
    char *p, *newline;
    typedArg arg;

    p = s;
    while (isspace(*p)) p++;
    if (*p == '\0') return OSCTT_Immediately();

    if (*p == '+') {
	/* Time tag is for some time in the future.  It should be a
           number of seconds as an int or float */

	newline = strchr(s, '\n');
	if (newline != NULL) *newline = '\0';

	p++; /* Skip '+' */
	while (isspace(*p)) p++;

	arg = ParseToken(p);
	if (arg.type == STRING_osc) {
	    post("warning: inscrutable time tag request: %s\n", s);
	    return OSCTT_Immediately();
	} else if (arg.type == INT_osc) {
	    return OSCTT_PlusSeconds(OSCTT_CurrentTime(),
				     (float) arg.datum.i);
	} else if (arg.type == FLOAT_osc) {
	    return OSCTT_PlusSeconds(OSCTT_CurrentTime(), arg.datum.f);
	} else {
	    error("This can't happen!");
	}
    }

    if (isdigit(*p) || (*p >= 'a' && *p <='f') || (*p >= 'A' && *p <='F')) {
	/* They specified the 8-byte tag in hex */
	OSCTimeTag tt;
	if (sscanf(p, "%llx", &tt) != 1) {
	    post("warning: couldn't parse time tag %s\n", s);
	    return OSCTT_Immediately();
	}
#ifndef	HAS8BYTEINT
	if (ntohl(1) != 1) {
	    /* tt is a struct of seconds and fractional part,
	       and this machine is little-endian, so sscanf
	       wrote each half of the time tag in the wrong half
	       of the struct. */
	    int temp;
	    temp = tt.seconds;
	    tt.seconds = tt.fraction ;
	    tt.fraction = temp;
	}
#endif
	return tt;
    }

    post("warning: invalid time tag: %s\n", s);
    return OSCTT_Immediately();
}
	    

void ParseInteractiveLine(OSCbuf *buf, char *mesg) {
    char *messageName, *token, *p;
    typedArg args[MAX_ARGS];
    int thisArg;

    p = mesg;
    while (isspace(*p)) p++;
    if (*p == '\0') return;

    messageName = p;

    if (strcmp(messageName, "play\n") == 0) {
	/* Special kludge feature to save typing */
	typedArg arg;

	if (OSC_openBundle(buf, OSCTT_Immediately())) {
	    post("Problem opening bundle: %s\n", OSC_errorMessage);
	    return;
	}

	arg.type = INT_osc;
	arg.datum.i = 0;
	WriteMessage(buf, "/voices/0/tp/timbre_index", 1, &arg);

	arg.type = FLOAT_osc;
	arg.datum.i = 0.0f;
	WriteMessage(buf, "/voices/0/tm/goto", 1, &arg);

	if (OSC_closeBundle(buf)) {
	    post("Problem closing bundle: %s\n", OSC_errorMessage);
	}

	return;
    }

    while (!isspace(*p) && *p != '\0') p++;
    if (isspace(*p)) {
        *p = '\0';
        p++;
    }

    thisArg = 0;
    while (*p != '\0') {
        /* flush leading whitespace */
        while (isspace(*p)) p++;
        if (*p == '\0') break;

        if (*p == '"') {
            /* A string argument: scan for close quotes */
            p++;
            args[thisArg].type = STRING_osc;
            args[thisArg].datum.s = p;

            while (*p != '"') {
                if (*p == '\0') {
                    post("Unterminated quote mark: ignoring line\n");
                    return;
                }
                p++;
            }
            *p = '\0';
            p++;
        } else {
            token = p;
            while (!isspace(*p) && (*p != '\0')) p++;
            if (isspace(*p)) {
                *p = '\0';
                p++;
            }
            args[thisArg] = ParseToken(token);
        }
        thisArg++;
	if (thisArg >= MAX_ARGS) {
	  post("Sorry, your message has more than MAX_ARGS (%d) arguments; ignoring the rest.\n",
		   MAX_ARGS);
	  break;
	}
    }

    if (WriteMessage(buf, messageName, thisArg, args) != 0)  {
	post("Problem sending message: %s\n", OSC_errorMessage);
    }
}

typedArg ParseToken(char *token) {
    char *p = token;
    typedArg returnVal;

    /* It might be an int, a float, or a string */

    if (*p == '-') p++;

    if (isdigit(*p) || *p == '.') {
        while (isdigit(*p)) p++;
        if (*p == '\0') {
            returnVal.type = INT_osc;
            returnVal.datum.i = atoi(token);
            return returnVal;
        }
        if (*p == '.') {
            p++;
            while (isdigit(*p)) p++;
            if (*p == '\0') {
                returnVal.type = FLOAT_osc;
                returnVal.datum.f = atof(token);
                return returnVal;
            }
        }
    }

    returnVal.type = STRING_osc;
    returnVal.datum.s = token;
    return returnVal;
}

int WriteMessage(OSCbuf *buf, char *messageName, int numArgs, typedArg *args) {
  int j, returnVal;
  const int wmERROR = -1;

  returnVal = 0;

#ifdef DEBUG
  printf("WriteMessage: %s ", messageName);

  for (j = 0; j < numArgs; j++) {
    switch (args[j].type) {
    case INT_osc:
      printf("%d ", args[j].datum.i);
      break;
      
    case FLOAT_osc:
      printf("%f ", args[j].datum.f);
      break;
      
    case STRING_osc:
      printf("%s ", args[j].datum.s);
      break;
      
    default:
      error("Unrecognized arg type, (not exiting)");
      return(wmERROR);
    }
  }
  printf("\n");
#endif
  
  if (!useTypeTags) {
    returnVal = OSC_writeAddress(buf, messageName);
    if (returnVal) {
      post("Problem writing address: %s\n", OSC_errorMessage);
    }
  } else {
    /* First figure out the type tags */
    char typeTags[MAX_ARGS+2];
    int i;
    
    typeTags[0] = ',';
    
    for (i = 0; i < numArgs; ++i) {
      switch (args[i].type) {
      case INT_osc:
	typeTags[i+1] = 'i';
	break;
	
      case FLOAT_osc:
	typeTags[i+1] = 'f';
	break;
	
      case STRING_osc:
	typeTags[i+1] = 's';
	break;
	
      default:
	error("Unrecognized arg type (not exiting)");
	return(wmERROR);
      }
    }
    typeTags[i+1] = '\0';
    
    returnVal = OSC_writeAddressAndTypes(buf, messageName, typeTags);
    if (returnVal) {
      post("Problem writing address: %s\n", OSC_errorMessage);
    }
  }

  for (j = 0; j < numArgs; j++) {
    switch (args[j].type) {
    case INT_osc:
      if ((returnVal = OSC_writeIntArg(buf, args[j].datum.i)) != 0) {
	return returnVal;
      }
      break;
      
    case FLOAT_osc:
      if ((returnVal = OSC_writeFloatArg(buf, args[j].datum.f)) != 0) {
	return returnVal;
      }
      break;
      
    case STRING_osc:
      if ((returnVal = OSC_writeStringArg(buf, args[j].datum.s)) != 0) {
	return returnVal;
      }
      break;
      
    default:
      error("Unrecognized arg type (not exiting)");
      returnVal = wmERROR;
    }
  }
  return returnVal;
}

void SendBuffer(void *htmsocket, OSCbuf *buf) {
#ifdef DEBUG
  printf("Sending buffer...\n");
#endif
  if (OSC_isBufferEmpty(buf)) {
		post("SendBuffer() called but buffer empty");
		return;
  }
  if (!OSC_isBufferDone(buf)) {
		error("SendBuffer() called but buffer not ready!, not exiting");
		return;	//{{raf}}
  }
  SendData(htmsocket, OSC_packetSize(buf), OSC_getPacket(buf));
}

void SendData(void *htmsocket, int size, char *data) {
  if (!SendHTMSocket(htmsocket, size, data)) {
    post("SendData::SendHTMSocket()failure -- not connected");
    CloseHTMSocket(htmsocket);
  }
}



/* ----------------------
   OSC-client code
 
 */

/* Here are the possible values of the state field: */

#define EMPTY 0	       /* Nothing written to packet yet */
#define ONE_MSG_ARGS 1 /* Packet has a single message; gathering arguments */
#define NEED_COUNT 2   /* Just opened a bundle; must write message name or
			  open another bundle */
#define GET_ARGS 3     /* Getting arguments to a message.  If we see a message
			  name or a bundle open/close then the current message
			  will end. */
#define DONE 4         /* All open bundles have been closed, so can't write 
		          anything else */

#ifdef WIN32
	#include	<winsock2.h>
	#include 	<io.h>    
	#include 	<stdio.h>    
	#include 	<errno.h>
	#include 	<fcntl.h>
	#include 	<sys/types.h>
	#include 	<sys/stat.h>
#endif

#ifdef __APPLE__
  #include <sys/types.h>
#endif

#ifdef unix
  #include <netinet/in.h>
  #include <stdio.h>
#endif
	

char *OSC_errorMessage;

static int OSC_padString(char *dest, char *str);
static int OSC_padStringWithAnExtraStupidComma(char *dest, char *str);
static int OSC_WritePadding(char *dest, int i);
static int CheckTypeTag(OSCbuf *buf, char expectedType);

void OSC_initBuffer(OSCbuf *buf, int size, char *byteArray) {
    buf->buffer = byteArray;
    buf->size = size;
    OSC_resetBuffer(buf);
}

void OSC_resetBuffer(OSCbuf *buf) {	
    buf->bufptr = buf->buffer;
    buf->state = EMPTY;
    buf->bundleDepth = 0;
    buf->prevCounts[0] = 0;
    buf->gettingFirstUntypedArg = 0;
    buf->typeStringPtr = 0;
}

int OSC_isBufferEmpty(OSCbuf *buf) {
    return buf->bufptr == buf->buffer;
}

int OSC_freeSpaceInBuffer(OSCbuf *buf) {
    return buf->size - (buf->bufptr - buf->buffer);
}

int OSC_isBufferDone(OSCbuf *buf) {
    return (buf->state == DONE || buf->state == ONE_MSG_ARGS);
}

char *OSC_getPacket(OSCbuf *buf) {
#ifdef ERROR_CHECK_GETPACKET
    if (buf->state == DONE || buf->state == ONE_MSG_ARGS) {
	return buf->buffer;
    } else {
	OSC_errorMessage = "Packet has unterminated bundles";
	return 0;
    }
#else
    return buf->buffer;
#endif
}

int OSC_packetSize(OSCbuf *buf) {
#ifdef ERROR_CHECK_PACKETSIZE
    if (buf->state == DONE || buf->state == ONE_MSG_ARGS) {
	return (buf->bufptr - buf->buffer);
    } else {
        OSC_errorMessage = "Packet has unterminated bundles";
        return 0;
    }
#else
    return (buf->bufptr - buf->buffer);
#endif
}

#define CheckOverflow(buf, bytesNeeded) { if ((bytesNeeded) > OSC_freeSpaceInBuffer(buf)) {OSC_errorMessage = "buffer overflow"; return 1;}}

static void PatchMessageSize(OSCbuf *buf) {
    int4byte size;
    size = buf->bufptr - ((char *) buf->thisMsgSize) - 4;
    *(buf->thisMsgSize) = htonl(size);
}

int OSC_openBundle(OSCbuf *buf, OSCTimeTag tt) {
    if (buf->state == ONE_MSG_ARGS) {
	OSC_errorMessage = "Can't open a bundle in a one-message packet";
	return 3;
    }

    if (buf->state == DONE) {
	OSC_errorMessage = "This packet is finished; can't open a new bundle";
	return 4;
    }

    if (++(buf->bundleDepth) >= MAX_BUNDLE_NESTING) {
	OSC_errorMessage = "Bundles nested too deeply; change MAX_BUNDLE_NESTING in OpenSoundControl.h";
	return 2;
    }

    if (CheckTypeTag(buf, '\0')) return 9;

    if (buf->state == GET_ARGS) {
	PatchMessageSize(buf);
    }

    if (buf->state == EMPTY) {
	/* Need 16 bytes for "#bundle" and time tag */
	CheckOverflow(buf, 16);
    } else {
	/* This bundle is inside another bundle, so we need to leave
	   a blank size count for the size of this current bundle. */
	CheckOverflow(buf, 20);
	*((int4byte *)buf->bufptr) = 0xaaaaaaaa;
        buf->prevCounts[buf->bundleDepth] = (int4byte *)buf->bufptr;

	buf->bufptr += 4;
    }

    buf->bufptr += OSC_padString(buf->bufptr, "#bundle");


    *((OSCTimeTag *) buf->bufptr) = tt;

    if (htonl(1) != 1) {
	/* Byte swap the 8-byte integer time tag */
	int4byte *intp = (int4byte *)buf->bufptr;
	intp[0] = htonl(intp[0]);
	intp[1] = htonl(intp[1]);

#ifdef HAS8BYTEINT
	{ /* tt is a 64-bit int so we have to swap the two 32-bit words. 
	    (Otherwise tt is a struct of two 32-bit words, and even though
	     each word was wrong-endian, they were in the right order
	     in the struct.) */
	    int4byte temp = intp[0];
	    intp[0] = intp[1];
	    intp[1] = temp;
	}
#endif
    }

    buf->bufptr += sizeof(OSCTimeTag);

    buf->state = NEED_COUNT;

    buf->gettingFirstUntypedArg = 0;
    buf->typeStringPtr = 0;
    return 0;
}


int OSC_closeBundle(OSCbuf *buf) {
    if (buf->bundleDepth == 0) {
	/* This handles EMPTY, ONE_MSG, ARGS, and DONE */
	OSC_errorMessage = "Can't close bundle; no bundle is open!";
	return 5;
    }

    if (CheckTypeTag(buf, '\0')) return 9;

    if (buf->state == GET_ARGS) {
        PatchMessageSize(buf);
    }

    if (buf->bundleDepth == 1) {
	/* Closing the last bundle: No bundle size to patch */
	buf->state = DONE;
    } else {
	/* Closing a sub-bundle: patch bundle size */
	int size = buf->bufptr - ((char *) buf->prevCounts[buf->bundleDepth]) - 4;
	*(buf->prevCounts[buf->bundleDepth]) = htonl(size);
	buf->state = NEED_COUNT;
    }

    --buf->bundleDepth;
    buf->gettingFirstUntypedArg = 0;
    buf->typeStringPtr = 0;
    return 0;
}


int OSC_closeAllBundles(OSCbuf *buf) {
    if (buf->bundleDepth == 0) {
        /* This handles EMPTY, ONE_MSG, ARGS, and DONE */
        OSC_errorMessage = "Can't close all bundles; no bundle is open!";
        return 6;
    }

    if (CheckTypeTag(buf, '\0')) return 9;

    while (buf->bundleDepth > 0) {
	OSC_closeBundle(buf);
    }
    buf->typeStringPtr = 0;
    return 0;
}

int OSC_writeAddress(OSCbuf *buf, char *name) {
    int4byte paddedLength;

    if (buf->state == ONE_MSG_ARGS) {
	OSC_errorMessage = "This packet is not a bundle, so you can't write another address";
	return 7;
    }

    if (buf->state == DONE) {
        OSC_errorMessage = "This packet is finished; can't write another address";
        return 8;
    }

    if (CheckTypeTag(buf, '\0')) return 9;

    paddedLength = OSC_effectiveStringLength(name);

    if (buf->state == EMPTY) {
	/* This will be a one-message packet, so no sizes to worry about */
	CheckOverflow(buf, paddedLength);
	buf->state = ONE_MSG_ARGS;
    } else {
	/* GET_ARGS or NEED_COUNT */
	CheckOverflow(buf, 4+paddedLength);
	if (buf->state == GET_ARGS) {
	    /* Close the old message */
	    PatchMessageSize(buf);
	}
	buf->thisMsgSize = (int4byte *)buf->bufptr;
	*(buf->thisMsgSize) = 0xbbbbbbbb;
	buf->bufptr += 4;
	buf->state = GET_ARGS;
    }

    /* Now write the name */
    buf->bufptr += OSC_padString(buf->bufptr, name);
    buf->typeStringPtr = 0;
    buf->gettingFirstUntypedArg = 1;

    return 0;
}

int OSC_writeAddressAndTypes(OSCbuf *buf, char *name, char *types) {
    int result;
    int4byte paddedLength;

    if (CheckTypeTag(buf, '\0')) return 9;

    result = OSC_writeAddress(buf, name);

    if (result) return result;

    paddedLength = OSC_effectiveStringLength(types);

    CheckOverflow(buf, paddedLength);    

    buf->typeStringPtr = buf->bufptr + 1; /* skip comma */
    buf->bufptr += OSC_padString(buf->bufptr, types);

    buf->gettingFirstUntypedArg = 0;
    return 0;
}

static int CheckTypeTag(OSCbuf *buf, char expectedType) {
    if (buf->typeStringPtr) {
	if (*(buf->typeStringPtr) != expectedType) {
	    if (expectedType == '\0') {
		OSC_errorMessage =
		    "According to the type tag I expected more arguments.";
	    } else if (*(buf->typeStringPtr) == '\0') {
		OSC_errorMessage =
		    "According to the type tag I didn't expect any more arguments.";
	    } else {
		OSC_errorMessage =
		    "According to the type tag I expected an argument of a different type.";
		printf("* Expected %c, string now %s\n", expectedType, buf->typeStringPtr);
	    }
	    return 9; 
	}
	++(buf->typeStringPtr);
    }
    return 0;
}


int OSC_writeFloatArg(OSCbuf *buf, float arg) {
    int4byte *intp;
    //int result;

    CheckOverflow(buf, 4);

    if (CheckTypeTag(buf, 'f')) return 9;

    /* Pretend arg is a long int so we can use htonl() */
    intp = ((int4byte *) &arg);
    *((int4byte *) buf->bufptr) = htonl(*intp);

    buf->bufptr += 4;

    buf->gettingFirstUntypedArg = 0;
    return 0;
}



int OSC_writeFloatArgs(OSCbuf *buf, int numFloats, float *args) {
    int i;
    int4byte *intp;

    CheckOverflow(buf, 4 * numFloats);

    /* Pretend args are long ints so we can use htonl() */
    intp = ((int4byte *) args);

    for (i = 0; i < numFloats; i++) {
	if (CheckTypeTag(buf, 'f')) return 9;
	*((int4byte *) buf->bufptr) = htonl(intp[i]);
	buf->bufptr += 4;
    }

    buf->gettingFirstUntypedArg = 0;
    return 0;
}

int OSC_writeIntArg(OSCbuf *buf, int4byte arg) {
    CheckOverflow(buf, 4);
    if (CheckTypeTag(buf, 'i')) return 9;

    *((int4byte *) buf->bufptr) = htonl(arg);
    buf->bufptr += 4;

    buf->gettingFirstUntypedArg = 0;
    return 0;
}

int OSC_writeStringArg(OSCbuf *buf, char *arg) {
    int len;

    if (CheckTypeTag(buf, 's')) return 9;

    len = OSC_effectiveStringLength(arg);

    if (buf->gettingFirstUntypedArg && arg[0] == ',') {
	/* This un-type-tagged message starts with a string
	   that starts with a comma, so we have to escape it
	   (with a double comma) so it won't look like a type
	   tag string. */

	CheckOverflow(buf, len+4); /* Too conservative */
	buf->bufptr += 
	    OSC_padStringWithAnExtraStupidComma(buf->bufptr, arg);

    } else {
	CheckOverflow(buf, len);
	buf->bufptr += OSC_padString(buf->bufptr, arg);
    }

    buf->gettingFirstUntypedArg = 0;
    return 0;

}

/* String utilities */

#define STRING_ALIGN_PAD 4
int OSC_effectiveStringLength(char *string) {
    int len = strlen(string) + 1;  /* We need space for the null char. */
    
    /* Round up len to next multiple of STRING_ALIGN_PAD to account for alignment padding */
    if ((len % STRING_ALIGN_PAD) != 0) {
        len += STRING_ALIGN_PAD - (len % STRING_ALIGN_PAD);
    }
    return len;
}

static int OSC_padString(char *dest, char *str) {
    int i;
    
    for (i = 0; str[i] != '\0'; i++) {
        dest[i] = str[i];
    }
    
    return OSC_WritePadding(dest, i);
}

static int OSC_padStringWithAnExtraStupidComma(char *dest, char *str) {
    int i;
    
    dest[0] = ',';
    for (i = 0; str[i] != '\0'; i++) {
        dest[i+1] = str[i];
    }

    return OSC_WritePadding(dest, i+1);
}
 
static int OSC_WritePadding(char *dest, int i) {
    dest[i] = '\0';
    i++;

    for (; (i % STRING_ALIGN_PAD) != 0; i++) {
	dest[i] = '\0';
    }

    return i;
}


