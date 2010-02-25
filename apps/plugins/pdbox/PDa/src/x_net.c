/* Copyright (c) 1997-1999 Miller Puckette.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#ifdef ROCKBOX
#include "plugin.h"
#include "../../pdbox.h"
#endif

/* network */

#include "m_pd.h"
#include "s_stuff.h"

#ifndef ROCKBOX
#include <sys/types.h>
#include <string.h>
#ifdef UNIX
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <stdio.h>
#define SOCKET_ERROR -1
#else
#include <winsock.h>
#endif
#endif /* ROCKBOX */

static t_class *netsend_class;

typedef struct _netsend
{
    t_object x_obj;
    int x_fd;
    int x_protocol;
} t_netsend;

static void *netsend_new(t_floatarg udpflag)
{
#ifdef ROCKBOX
    (void) udpflag;
#endif

    t_netsend *x = (t_netsend *)pd_new(netsend_class);
    outlet_new(&x->x_obj, &s_float);
    x->x_fd = -1;
#ifndef ROCKBOX
    x->x_protocol = (udpflag != 0 ? SOCK_DGRAM : SOCK_STREAM);
#endif
    return (x);
}

static void netsend_connect(t_netsend *x, t_symbol *hostname,
    t_floatarg fportno)
{
#ifdef ROCKBOX
    (void) x;
    (void) hostname;
    (void) fportno;
#else /* ROCKBOX */
    struct sockaddr_in server;
    struct hostent *hp;
    int sockfd;
    int portno = fportno;
    int intarg;
    if (x->x_fd >= 0)
    {
    	error("netsend_connect: already connected");
    	return;
    }

    	/* create a socket */
    sockfd = socket(AF_INET, x->x_protocol, 0);
#if 0
    fprintf(stderr, "send socket %d\n", sockfd);
#endif
    if (sockfd < 0)
    {
    	sys_sockerror("socket");
    	return;
    }
    /* connect socket using hostname provided in command line */
    server.sin_family = AF_INET;
    hp = gethostbyname(hostname->s_name);
    if (hp == 0)
    {
	post("bad host?\n");
	return;
    }
#if 0
    intarg = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF,
    	&intarg, sizeof(intarg)) < 0)
    	    post("setsockopt (SO_RCVBUF) failed\n");
#endif
    	/* for stream (TCP) sockets, specify "nodelay" */
    if (x->x_protocol == SOCK_STREAM)
    {
	intarg = 1;
	if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY,
    	    &intarg, sizeof(intarg)) < 0)
    		post("setsockopt (TCP_NODELAY) failed\n");
    }
    memcpy((char *)&server.sin_addr, (char *)hp->h_addr, hp->h_length);

    /* assign client port number */
    server.sin_port = htons((u_short)portno);

    post("connecting to port %d", portno);
	/* try to connect.  LATER make a separate thread to do this
	because it might block */
    if (connect(sockfd, (struct sockaddr *) &server, sizeof (server)) < 0)
    {
    	sys_sockerror("connecting stream socket");
    	sys_closesocket(sockfd);
    	return;
    }
    x->x_fd = sockfd;
    outlet_float(x->x_obj.ob_outlet, 1);
#endif /* ROCKBOX */
}

static void netsend_disconnect(t_netsend *x)
{
    if (x->x_fd >= 0)
    {
#ifndef ROCKBOX
    	sys_closesocket(x->x_fd);
#endif
    	x->x_fd = -1;
    	outlet_float(x->x_obj.ob_outlet, 0);
    }
}

static void netsend_send(t_netsend *x, t_symbol *s, int argc, t_atom *argv)
{
#ifdef ROCKBOX
    (void) x;
    (void) s;
    (void) argc;
    (void) argv;
#else /* ROCKBOX */
    if (x->x_fd >= 0)
    {
	t_binbuf *b = binbuf_new();
	char *buf, *bp;
	int length, sent;
	t_atom at;
	binbuf_add(b, argc, argv);
	SETSEMI(&at);
	binbuf_add(b, 1, &at);
	binbuf_gettext(b, &buf, &length);
	for (bp = buf, sent = 0; sent < length;)
	{
	    static double lastwarntime;
	    static double pleasewarn;
	    double timebefore = sys_getrealtime();
    	    int res = send(x->x_fd, buf, length-sent, 0);
    	    double timeafter = sys_getrealtime();
    	    int late = (timeafter - timebefore > 0.005);
    	    if (late || pleasewarn)
    	    {
    	    	if (timeafter > lastwarntime + 2)
    	    	{
    	    	     post("netsend blocked %d msec",
    	    	     	(int)(1000 * ((timeafter - timebefore) + pleasewarn)));
    	    	     pleasewarn = 0;
    	    	     lastwarntime = timeafter;
    	    	}
    	    	else if (late) pleasewarn += timeafter - timebefore;
    	    }
    	    if (res <= 0)
    	    {
    		sys_sockerror("netsend");
    		netsend_disconnect(x);
    		break;
    	    }
    	    else
    	    {
    		sent += res;
    		bp += res;
    	    }
	}
	t_freebytes(buf, length);
	binbuf_free(b);
    }
    else error("netsend: not connected");
#endif /* ROCKBOX */
}

static void netsend_free(t_netsend *x)
{
#ifdef ROCKBOX
    (void) x;
#else
    netsend_disconnect(x);
#endif
}

static void netsend_setup(void)
{
    netsend_class = class_new(gensym("netsend"), (t_newmethod)netsend_new,
    	(t_method)netsend_free,
    	sizeof(t_netsend), 0, A_DEFFLOAT, 0);
    class_addmethod(netsend_class, (t_method)netsend_connect,
    	gensym("connect"), A_SYMBOL, A_FLOAT, 0);
    class_addmethod(netsend_class, (t_method)netsend_disconnect,
    	gensym("disconnect"), 0);
    class_addmethod(netsend_class, (t_method)netsend_send, gensym("send"),
    	A_GIMME, 0);
}

/* ----------------------------- netreceive ------------------------- */

static t_class *netreceive_class;

typedef struct _netreceive
{
    t_object x_obj;
    t_outlet *x_msgout;
    t_outlet *x_connectout;
    int x_connectsocket;
    int x_nconnections;
    int x_udp;
} t_netreceive;

#ifdef ROCKBOX
static t_netreceive* receiver;
static int receiver_port;
#endif /* ROCKBOX */

#ifndef ROCKBOX
static void netreceive_notify(t_netreceive *x)
{
    outlet_float(x->x_connectout, --x->x_nconnections);
}
#endif /* ROCKBOX */

static void netreceive_doit(void *z, t_binbuf *b)
{
#ifndef ROCKBOX
    t_atom messbuf[1024];
#endif
    t_netreceive *x = (t_netreceive *)z;
    int msg, natom = binbuf_getnatom(b);
    t_atom *at = binbuf_getvec(b);
    for (msg = 0; msg < natom;)
    {
    	int emsg;
	for (emsg = msg; emsg < natom && at[emsg].a_type != A_COMMA
	    && at[emsg].a_type != A_SEMI; emsg++)
	    	;
	if (emsg > msg)
	{
	    int i;
	    for (i = msg; i < emsg; i++)
	    	if (at[i].a_type == A_DOLLAR || at[i].a_type == A_DOLLSYM)
	    {
	    	pd_error(x, "netreceive: got dollar sign in message");
		goto nodice;
	    }
	    if (at[msg].a_type == A_FLOAT)
	    {
	    	if (emsg > msg + 1)
		    outlet_list(x->x_msgout, 0, emsg-msg, at + msg);
		else outlet_float(x->x_msgout, at[msg].a_w.w_float);
	    }
	    else if (at[msg].a_type == A_SYMBOL)
	    	outlet_anything(x->x_msgout, at[msg].a_w.w_symbol,
		    emsg-msg-1, at + msg + 1);
	}
    nodice:
    	msg = emsg + 1;
    }
}

#ifndef ROCKBOX
static void netreceive_connectpoll(t_netreceive *x)
{
    int fd = accept(x->x_connectsocket, 0, 0);
    if (fd < 0) post("netreceive: accept failed");
    else
    {
    	t_socketreceiver *y = socketreceiver_new((void *)x, 
    	    (t_socketnotifier)netreceive_notify,
	    	(x->x_msgout ? netreceive_doit : 0), 0);
    	sys_addpollfn(fd, (t_fdpollfn)socketreceiver_read, y);
    	outlet_float(x->x_connectout, ++x->x_nconnections);
    }
}
#endif

static void *netreceive_new(t_symbol *compatflag,
    t_floatarg fportno, t_floatarg udpflag)
{
    t_netreceive *x;

#ifdef ROCKBOX
    int portno = fportno, udp = (udpflag != 0);

    (void) compatflag;

    /* Look whether callback is already taken. */
    if(receiver)
    {
        post("Receiver callback already taken!\n");
        return NULL;
    }

    /* Look whether TCP sockets are thought to exist. */
    if(!udp)
    {
        post("Trying to create TCP socket!\n");
        return NULL;
    }

    x = (t_netreceive *) pd_new(netreceive_class);
    x->x_msgout = outlet_new(&x->x_obj, &s_anything);
    x->x_connectout = 0;
    x->x_nconnections = 0;
    x->x_udp = udp;

    receiver = x;
    receiver_port = portno;

#else /* ROCKBOX */
    struct sockaddr_in server;
    int sockfd, portno = fportno, udp = (udpflag != 0);
    int old = !strcmp(compatflag->s_name , "old");
    int intarg;
    	/* create a socket */
    sockfd = socket(AF_INET, (udp ? SOCK_DGRAM : SOCK_STREAM), 0);
#if 0
    fprintf(stderr, "receive socket %d\n", sockfd);
#endif
    if (sockfd < 0)
    {
    	sys_sockerror("socket");
    	return (0);
    }
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;

#if 1
    	/* ask OS to allow another Pd to repoen this port after we close it. */
    intarg = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
    	&intarg, sizeof(intarg)) < 0)
    	    post("setsockopt (SO_REUSEADDR) failed\n");
#endif
#if 0
    intarg = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF,
    	&intarg, sizeof(intarg)) < 0)
    	    post("setsockopt (SO_RCVBUF) failed\n");
#endif
    	/* Stream (TCP) sockets are set NODELAY */
    if (!udp)
    {
	intarg = 1;
	if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY,
    	    &intarg, sizeof(intarg)) < 0)
    		post("setsockopt (TCP_NODELAY) failed\n");
    }
    	/* assign server port number */
    server.sin_port = htons((u_short)portno);

    	/* name the socket */
    if (bind(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
    	sys_sockerror("bind");
    	sys_closesocket(sockfd);
    	return (0);
    }
    x = (t_netreceive *)pd_new(netreceive_class);
    if (old)
    {
    	/* old style, nonsecure version */
	x->x_msgout = 0;
    }
    else x->x_msgout = outlet_new(&x->x_obj, &s_anything);

    if (udp)	    /* datagram protocol */
    {
    	t_socketreceiver *y = socketreceiver_new((void *)x, 
    	    (t_socketnotifier)netreceive_notify,
	    	(x->x_msgout ? netreceive_doit : 0), 1);
	sys_addpollfn(sockfd, (t_fdpollfn)socketreceiver_read, y);
	x->x_connectout = 0;
    }
    else    	/* streaming protocol */
    {
	if (listen(sockfd, 5) < 0)
	{
    	    sys_sockerror("listen");
    	    sys_closesocket(sockfd);
	    sockfd = -1;
	}
    	else
	{
	    sys_addpollfn(sockfd, (t_fdpollfn)netreceive_connectpoll, x);
    	    x->x_connectout = outlet_new(&x->x_obj, &s_float);
	}
    }
    x->x_connectsocket = sockfd;
    x->x_nconnections = 0;
    x->x_udp = udp;
#endif /* ROCKBOX */

    return (x);
}

static void netreceive_free(t_netreceive *x)
{
#ifdef ROCKBOX
    if(receiver && receiver == x)
        receiver = NULL;
#else /* ROCKBOX */
    	/* LATER make me clean up open connections */
    if (x->x_connectsocket >= 0)
    {
    	sys_rmpollfn(x->x_connectsocket);
    	sys_closesocket(x->x_connectsocket);
    }
#endif /* ROCKBOX */
}

#ifdef ROCKBOX
/* Basically a reimplementation of socketreceiver_getudp()
   from s_inter.c */
extern t_binbuf* inbinbuf;
extern void outlet_setstacklim(void);

void rockbox_receive_callback(struct datagram* dg)
{
    /* Check whether there is a receiver. */
    if(!receiver)
        return;

    /* Limit string. */
    dg->data[dg->size] = '\0';

    /* If complete line... */
    if(dg->data[dg->size-1] == '\n')
    {
        char* semi = strchr(dg->data, ';');

        /* Limit message. */
        if(semi)
            *semi = '\0';

        /* Create binary buffer. */
        binbuf_text(inbinbuf, dg->data, strlen(dg->data));

        /* Limit outlet stack. */
        outlet_setstacklim();

        /* Execute receive function. */
        netreceive_doit(receiver, inbinbuf);
    }
}
#endif /* ROCKBOX */

static void netreceive_setup(void)
{
    netreceive_class = class_new(gensym("netreceive"),
    	(t_newmethod)netreceive_new, (t_method)netreceive_free,
    	sizeof(t_netreceive), CLASS_NOINLET, A_DEFFLOAT, A_DEFFLOAT, 
	    A_DEFSYM, 0);
}

void x_net_setup(void)
{
    netsend_setup();
    netreceive_setup();
}
