
#ifdef ROCKBOX
#include "plugin.h"
#include "../../pdbox.h"
#else /* ROCKBOX */
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <stdio.h>
#endif /* ROCKBOX */

#include "m_pd.h"
#include "m_imp.h"

static t_class *ipod_class = 0;

typedef struct _ipod
{
    t_object x_obj;
    t_symbol* x_what;
} t_ipod;

static t_ipod* ipod;
static  t_int x_fd = -1;



static void ipod_connect(void)
{
#ifdef ROCKBOX
    if (x_fd >= 0)
    {
        error("ipod_connect: already connected");
        return;
    }
    else
    {
        x_fd++;
    }
#else /* ROCKBOX */
    struct sockaddr_in server;
    struct hostent *hp;
    int sockfd;
    int portno = 3334;
    char hostname[] = "127.0.0.1";
    int intarg;
    if (x_fd >= 0)
    {
    	error("ipod_connect: already connected");
    	return;
    }

    	/* create a socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0)
    {
    	sys_sockerror("socket");
    	return;
    }

    /* connect socket using hostname provided in command line */

    server.sin_family = AF_INET;
    hp = gethostbyname(hostname);
    if (hp == 0)
    {
	post("bad host?\n");
	return;
    }

    memcpy((char *)&server.sin_addr, (char *)hp->h_addr, hp->h_length);

    server.sin_port = htons((u_short)portno);
    if (connect(sockfd, (struct sockaddr *) &server, sizeof (server)) < 0)
    {
    	sys_sockerror("connecting stream socket");
    	sys_closesocket(sockfd);
    	return;
    }
    post("connected %s %d",hostname,portno);
    x_fd = sockfd;
#endif /* ROCKBOX */
}



static void ipod_bang(t_ipod *x)
{
    static char sendme[200];
#ifdef ROCKBOX
    snprintf(sendme, sizeof(sendme), "%s bang;\n", x->x_what->s_name);
    SEND_FROM_CORE(sendme);
#else /* ROCKBOX */
    sprintf(sendme,"%s bang;\n",x->x_what->s_name);
    send(x_fd,sendme,strlen(sendme),0);
#endif /*ROCKBOX */

//    if (x->x_sym->s_thing) pd_bang(x->x_sym->s_thing);
}

static void ipod_float(t_ipod *x, t_float f)
{
    static char sendme[200];
#ifdef ROCKBOX
    char f_buf[32];

    ftoan(f, f_buf, sizeof(f_buf)-1);
    strcpy(sendme, x->x_what->s_name);
    strcat(sendme, " ");
    strcat(sendme, f_buf);

    SEND_FROM_CORE(sendme);
#else /* ROCKBOX */
    sprintf(sendme,"%s %f;\n",x->x_what->s_name,f);
    send(x_fd,sendme,strlen(sendme),0);
#endif /* ROCKBOX */

//    post("forwarding float %s",x->x_what->s_name);
//    if (x->x_sym->s_thing) pd_float(x->x_sym->s_thing, f);
}

static void *ipod_new(t_symbol* what)
{
    t_ipod *x = (t_ipod *)pd_new(ipod_class);
    post("new ipod %s",what->s_name);
    x->x_what = what;
    return (x);
}

static void ipod_setup(void)
{
    ipod_class = class_new(gensym("ipod"), (t_newmethod)ipod_new, 0,
    	sizeof(t_ipod), 0, A_DEFSYM, 0);
    class_addbang(ipod_class, ipod_bang);
    class_addfloat(ipod_class, ipod_float);
    ipod_connect();
}

void pd_checkgui(t_pd *x, t_symbol *s)
{
    if (!strncmp(s->s_name,"pod_",4))
        if (!strcmp((*x)->c_name->s_name,"gatom") ||
            !strcmp((*x)->c_name->s_name,"vsl") ||
            !strcmp((*x)->c_name->s_name,"hsl") ||
            !strcmp((*x)->c_name->s_name,"bng") ||
            !strcmp((*x)->c_name->s_name,"vradio") ||
            !strcmp((*x)->c_name->s_name,"hradio")) {
            
            post("binding %s to %s",s->s_name,(*x)->c_name->s_name);
            if (!ipod_class) ipod_setup();
            ipod = ipod_new(s);
            pd_bind(&ipod->x_obj.ob_pd,s);
        }
}

