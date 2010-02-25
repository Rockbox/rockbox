/* Copyright (c) 1997-1999 Miller Puckette.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */


/* IOhannes :
 * changed the canvas_restore in "g_canvas.c", so that it might accept $args as well (like "pd $0_test")
 * so you can make multiple & distinguishable templates
 * 1511:forum::für::umläute:2001
 * change marked with    IOhannes
 */

#ifdef ROCKBOX
#include "plugin.h"
#include "../../pdbox.h"
#ifdef SIMULATOR
int printf(const char *fmt, ...);
void perror(const char*);
#endif
#else /* ROCKBOX */
#include <stdlib.h>
#include <stdio.h>
#ifdef UNIX
#include <unistd.h>
#endif
#ifdef MSW
#include <io.h>
#endif
#include <fcntl.h>

#include <string.h>
#include <stdarg.h>
#endif /* ROCKBOX */

#include "m_pd.h"
#include "s_stuff.h"

struct _binbuf
{
    int b_n;
    t_atom *b_vec;
};

t_binbuf *binbuf_new(void)
{
    t_binbuf *x = (t_binbuf *)t_getbytes(sizeof(*x));
    x->b_n = 0;
    x->b_vec = t_getbytes(0);
    return (x);
}

void binbuf_free(t_binbuf *x)
{
    t_freebytes(x->b_vec, x->b_n * sizeof(*x->b_vec));
    t_freebytes(x,  sizeof(*x));
}

t_binbuf *binbuf_duplicate(t_binbuf *y)
{
    t_binbuf *x = (t_binbuf *)t_getbytes(sizeof(*x));
    x->b_n = y->b_n;
    x->b_vec = t_getbytes(x->b_n * sizeof(*x->b_vec));
    memcpy(x->b_vec, y->b_vec, x->b_n * sizeof(*x->b_vec));
    return (x);
}

void binbuf_clear(t_binbuf *x)
{
    x->b_vec = t_resizebytes(x->b_vec, x->b_n * sizeof(*x->b_vec), 0);
    x->b_n = 0;
}

    /* convert text to a binbuf */
void binbuf_text(t_binbuf *x, char *text, size_t size)
{
    char buf[MAXPDSTRING+1], *bufp, *ebuf = buf+MAXPDSTRING;
    const char *textp = text, *etext = text+size;
    t_atom *ap;
    int nalloc = 16, natom = 0;
    t_freebytes(x->b_vec, x->b_n * sizeof(*x->b_vec));
    x->b_vec = t_getbytes(nalloc * sizeof(*x->b_vec));
    ap = x->b_vec;
    x->b_n = 0;
    while (1)
    {
#ifndef ROCKBOX
	int type;
#endif
	    /* skip leading space */
	while ((textp != etext) && (*textp == ' ' || *textp == '\n'
	    || *textp == '\r' || *textp == '\t')) textp++;
	if (textp == etext) break;
	if (*textp == ';') SETSEMI(ap), textp++;
	else if (*textp == ',') SETCOMMA(ap), textp++;
	else
	{
	    	/* it's an atom other than a comma or semi */
	    char c;
	    int floatstate = 0, slash = 0, lastslash = 0,
	    	firstslash = (*textp == '\\');
	    bufp = buf;
	    do
	    {
		c = *bufp = *textp++;
		lastslash = slash;
		slash = (c == '\\');

		if (floatstate >= 0)
		{
		    int digit = (c >= '0' && c <= '9'),
			dot = (c == '.'), minus = (c == '-'),
			plusminus = (minus || (c == '+')),
			expon = (c == 'e' || c == 'E');
		    if (floatstate == 0)    /* beginning */
		    {
			if (minus) floatstate = 1;
			else if (digit) floatstate = 2;
			else if (dot) floatstate = 3;
			else floatstate = -1;
		    }
		    else if (floatstate == 1)	/* got minus */
		    {
			if (digit) floatstate = 2;
			else if (dot) floatstate = 3;
			else floatstate = -1;
		    }
		    else if (floatstate == 2)	/* got digits */
		    {
			if (dot) floatstate = 4;
			else if (expon) floatstate = 6;
			else if (!digit) floatstate = -1;
		    }
		    else if (floatstate == 3)	/* got '.' without digits */
		    {
			if (digit) floatstate = 5;
			else floatstate = -1;
		    }
		    else if (floatstate == 4)	/* got '.' after digits */
		    {
			if (digit) floatstate = 5;
			else if (expon) floatstate = 6;
			else floatstate = -1;
		    }
		    else if (floatstate == 5)	/* got digits after . */
		    {
			if (expon) floatstate = 6;
			else if (!digit) floatstate = -1;
		    }
		    else if (floatstate == 6)	/* got 'e' */
		    {
			if (plusminus) floatstate = 7;
			else if (digit) floatstate = 8;
			else floatstate = -1;
		    }
		    else if (floatstate == 7)	/* got plus or minus */
		    {
			if (digit) floatstate = 8;
			else floatstate = -1;
		    }
		    else if (floatstate == 8)	/* got digits */
		    {
			if (!digit) floatstate = -1;
		    }
		}
		if (!slash) bufp++;
	    }
	    while (textp != etext && bufp != ebuf && 
		(slash || (*textp != ' ' && *textp != '\n' && *textp != '\r'
		    && *textp != '\t' &&*textp != ',' && *textp != ';')));
	    *bufp = 0;
#if 0
	    post("buf %s", buf);
#endif

	    if (*buf == '$' && buf[1] >= '0' && buf[1] <= '9' && !firstslash)
	    {
		for (bufp = buf+2; *bufp; bufp++)
		    if (*bufp < '0' || *bufp > '9')
		{
		    SETDOLLSYM(ap, gensym(buf+1));
		    goto didit;
		}
		SETDOLLAR(ap, atoi(buf+1));
	    didit: ;
	    }
	    else
	    {
		if (floatstate == 2 || floatstate == 4 || floatstate == 5 ||
		    floatstate == 8)
		    	SETFLOAT(ap, atof(buf));
		else SETSYMBOL(ap, gensym(buf));
	    }
	}
	ap++;
	natom++;
	if (natom == nalloc)
	{
	    x->b_vec = t_resizebytes(x->b_vec, nalloc * sizeof(*x->b_vec),
		nalloc * (2*sizeof(*x->b_vec)));
	    nalloc = nalloc * 2;
	    ap = x->b_vec + natom;
	}
	if (textp == etext) break;
    }
    /* reallocate the vector to exactly the right size */
    x->b_vec = t_resizebytes(x->b_vec, nalloc * sizeof(*x->b_vec),
	natom * sizeof(*x->b_vec));
    x->b_n = natom;
}

    /* convert a binbuf to text; no null termination. */
void binbuf_gettext(t_binbuf *x, char **bufp, int *lengthp)
{
    char *buf = getbytes(0), *newbuf;
    int length = 0;
    char string[MAXPDSTRING];
    t_atom *ap;
    int indx;

    for (ap = x->b_vec, indx = x->b_n; indx--; ap++)
    {
    	int newlength;
    	if ((ap->a_type == A_SEMI || ap->a_type == A_COMMA) &&
    	    	length && buf[length-1] == ' ') length--;
    	atom_string(ap, string, MAXPDSTRING);
    	newlength = length + strlen(string) + 1;
    	if (!(newbuf = resizebytes(buf, length, newlength))) break;
    	buf = newbuf;
    	strcpy(buf + length, string);
    	length = newlength;
    	if (ap->a_type == A_SEMI) buf[length-1] = '\n';
    	else buf[length-1] = ' ';
    }
    if (length && buf[length-1] == ' ')
    {
    	if((newbuf = t_resizebytes(buf, length, length-1)))
    	{
    	    buf = newbuf;
    	    length--;
    	}
    }
    *bufp = buf;
    *lengthp = length;
}

/* LATER improve the out-of-space behavior below.  Also fix this so that
writing to file doesn't buffer everything together. */

void binbuf_add(t_binbuf *x, int argc, t_atom *argv)
{
    int newsize = x->b_n + argc, i;
    t_atom *ap;
    if((ap = t_resizebytes(x->b_vec, x->b_n * sizeof(*x->b_vec),
	newsize * sizeof(*x->b_vec))))
	    x->b_vec = ap;
    else
    {
    	error("binbuf_addmessage: out of space");
    	return;
    }
#if 0
    startpost("binbuf_add: ");
    postatom(argc, argv);
    endpost();
#endif
    for (ap = x->b_vec + x->b_n, i = argc; i--; ap++)
    	*ap = *(argv++);
    x->b_n = newsize;
}

#define MAXADDMESSV 100
void binbuf_addv(t_binbuf *x, char *fmt, ...)
{
    va_list ap;
    t_atom arg[MAXADDMESSV], *at =arg;
    int nargs = 0;
    char *fp = fmt;

    va_start(ap, fmt);
    while (1)
    {
    	if (nargs >= MAXADDMESSV)
    	{
    	    error("binbuf_addmessv: only %d allowed", MAXADDMESSV);
    	    break;
    	}
    	switch(*fp++)
    	{
    	case 'i': SETFLOAT(at, va_arg(ap, t_int)); break;
    	case 'f': SETFLOAT(at, va_arg(ap, double)); break;
    	case 's': SETSYMBOL(at, va_arg(ap, t_symbol *)); break;
    	case ';': SETSEMI(at); break;
    	case ',': SETCOMMA(at); break;
    	default: goto done;
    	}
    	at++;
    	nargs++;
    }
done:
    va_end(ap);
    binbuf_add(x, nargs, arg);
}

/* add a binbuf to another one for saving.  Semicolons and commas go to
symbols ";", "'",; the symbol ";" goes to "\;", etc. */

void binbuf_addbinbuf(t_binbuf *x, t_binbuf *y)
{
    t_binbuf *z = binbuf_new();
    int i;
    t_atom *ap;
    binbuf_add(z, y->b_n, y->b_vec);
    for (i = 0, ap = z->b_vec; i < z->b_n; i++, ap++)
    {
    	char tbuf[MAXPDSTRING];
    	switch (ap->a_type)
    	{
    	case A_FLOAT:
    	    break;
    	case A_SEMI:
    	    SETSYMBOL(ap, gensym(";"));
    	    break;
    	case A_COMMA:
    	    SETSYMBOL(ap, gensym(","));
    	    break;
    	case A_DOLLAR:
#ifdef ROCKBOX
            snprintf(tbuf, sizeof(tbuf), "$%d", ap->a_w.w_index);
#else /* ROCKBOX */
    	    sprintf(tbuf, "$%d", ap->a_w.w_index);
#endif /* ROCKBOX */
    	    SETSYMBOL(ap, gensym(tbuf));
    	    break;
    	case A_DOLLSYM:
#ifdef ROCKBOX
            snprintf(tbuf, sizeof(tbuf), "$%s", ap->a_w.w_symbol->s_name);
#else /* ROCKBOX */
    	    sprintf(tbuf, "$%s", ap->a_w.w_symbol->s_name);
#endif /* ROCKBOX */
    	    SETSYMBOL(ap, gensym(tbuf));
    	    break;
    	case A_SYMBOL:
    	    	/* FIXME make this general */
    	    if (!strcmp(ap->a_w.w_symbol->s_name, ";"))
    	    	SETSYMBOL(ap, gensym(";"));
    	    else if (!strcmp(ap->a_w.w_symbol->s_name, ","))
    	    	SETSYMBOL(ap, gensym(","));
    	    break;
    	default:
    	    bug("binbuf_addbinbuf");
    	}
    }
    
    binbuf_add(x, z->b_n, z->b_vec);
}

void binbuf_addsemi(t_binbuf *x)
{
    t_atom a;
    SETSEMI(&a);
    binbuf_add(x, 1, &a);
}

/* Supply atoms to a binbuf from a message, making the opposite changes
from binbuf_addbinbuf.  The symbol ";" goes to a semicolon, etc. */

void binbuf_restore(t_binbuf *x, int argc, t_atom *argv)
{
    int newsize = x->b_n + argc, i;
    t_atom *ap;
    if((ap = t_resizebytes(x->b_vec, x->b_n * sizeof(*x->b_vec),
	newsize * sizeof(*x->b_vec))))
	    x->b_vec = ap;
    else
    {
    	error("binbuf_addmessage: out of space");
    	return;
    }

    for (ap = x->b_vec + x->b_n, i = argc; i--; ap++)
    {
    	if (argv->a_type == A_SYMBOL)
    	{
	    char *str = argv->a_w.w_symbol->s_name;
    	    if (!strcmp(str, ";")) SETSEMI(ap);
    	    else if (!strcmp(str, ",")) SETCOMMA(ap);
    	    else if (str[0] == '$' && str[1] >= '0' && str[1] <= '9')
    	    {
	    	int dollsym = 0;
	    	char *str2;
		for (str2 = str + 2; *str2; str2++)
		    if (*str2 < '0' || *str2 > '9')
		    	dollsym = 1;
    	    	if (dollsym)
		    SETDOLLSYM(ap, gensym(str + 1));
		else
		{
		    int dollar = 0;
#ifdef ROCKBOX
                    dollar = atoi(argv->a_w.w_symbol->s_name + 1);
#else
    	    	    sscanf(argv->a_w.w_symbol->s_name + 1, "%d", &dollar);
#endif
    	    	    SETDOLLAR(ap, dollar);
    	    	}
	    }
    	    else *ap = *argv;
    	    argv++;
    	}
    	else *ap = *(argv++);
    }
    x->b_n = newsize;
}


#define MSTACKSIZE 2048

void binbuf_print(t_binbuf *x)
{
    int i, startedpost = 0, newline = 1;
    for (i = 0; i < x->b_n; i++)
    {
    	if (newline)
	{
	    if (startedpost) endpost();
	    startpost("");
	    startedpost = 1;
    	}
	postatom(1, x->b_vec + i);
	if (x->b_vec[i].a_type == A_SEMI)
	    newline = 1;
	else newline = 0; 
    }
    if (startedpost) endpost();
}

int binbuf_getnatom(t_binbuf *x)
{
    return (x->b_n);
}

t_atom *binbuf_getvec(t_binbuf *x)
{
    return (x->b_vec);
}

int canvas_getdollarzero( void);

t_symbol *binbuf_realizedollsym(t_symbol *s, int ac, t_atom *av, int tonew)
{
    int argno = atol(s->s_name), lastnum;
    char buf[MAXPDSTRING], c, *sp;
    for (lastnum = 0, sp = s->s_name; ((c = *sp) && c >= '0' && c <= '9');
    	sp++, lastnum++)
    if (!c || argno < 0 || argno > ac)
    {
    	if (!tonew)
    	    return (0);
#ifdef ROCKBOX
        else snprintf(buf, sizeof(buf), "$%d", argno);
#else /* ROCKBOX */
	else sprintf(buf, "$%d", argno);
#endif /* ROCKBOX */
    }
    else if (argno == 0)
#ifdef ROCKBOX
        snprintf(buf, sizeof(buf), "%d", canvas_getdollarzero());
#else /* ROCKBOX */
    	sprintf(buf, "%d", canvas_getdollarzero());
#endif /* ROCKBOX */
    else
    	atom_string(av+(argno-1), buf, MAXPDSTRING/2-1);
    strncat(buf, sp, MAXPDSTRING/2-1);
    return (gensym(buf));
}

void binbuf_eval(t_binbuf *x, t_pd *target, int argc, t_atom *argv)
{
    static t_atom mstack[MSTACKSIZE], *msp = mstack, *ems = mstack+MSTACKSIZE;
    t_atom *stackwas = msp;
    t_atom *at = x->b_vec;
    int ac = x->b_n;
    int nargs;
    while (1)
    {
    	t_pd *nexttarget;
	    /* get a target. */
	while (!target)
	{
	    t_symbol *s;
	    while (ac && (at->a_type == A_SEMI || at->a_type == A_COMMA))
		ac--,  at++;
	    if (!ac) break;
	    if (at->a_type == A_DOLLAR)
	    {
	    	if (at->a_w.w_index <= 0 || at->a_w.w_index > argc)
	    	{
		    error("$%d: not enough arguments supplied",
			    at->a_w.w_index);
		    goto cleanup; 
		}
    	    	else if (argv[at->a_w.w_index-1].a_type != A_SYMBOL)
	    	{
		    error("$%d: symbol needed as message destination",
		    	at->a_w.w_index);
		    goto cleanup; 
		}
		else s = argv[at->a_w.w_index-1].a_w.w_symbol;
	    }
	    else if (at->a_type == A_DOLLSYM)
	    {
	    	if (!(s = binbuf_realizedollsym(at->a_w.w_symbol,
		    argc, argv, 0)))
		{
		    error("$%s: not enough arguments supplied",
		    	at->a_w.w_symbol->s_name);
		    goto cleanup;
	    	}
	    }
	    else s = atom_getsymbol(at);
	    if (!(target = s->s_thing))
	    {
		error("%s: no such object", s->s_name);
	    cleanup:
		do at++, ac--;
		while (ac && at->a_type != A_SEMI);
		    /* LATER eat args until semicolon and continue */
		continue;
	    }
	    else
	    {
	    	at++, ac--;
	    	break;
	    }
	}
	if (!ac) break;
	nargs = 0;
	nexttarget = target;
	while (1)
	{
	    t_symbol *s9;
	    if (!ac) goto gotmess;
	    if (msp >= ems)
	    {
		error("message stack overflow");
		goto broken;
	    }
	    switch (at->a_type)
	    {
	    case A_SEMI:
	    	    /* semis and commas in new message just get bashed to
		    a symbol.  This is needed so you can pass them to "expr." */
	    	if (target == &pd_objectmaker)
		{
		    SETSYMBOL(msp, gensym(";"));
		    break;
		}
		else
		{
	    	    nexttarget = 0;
		    goto gotmess;
	    	}
	    case A_COMMA:
	    	if (target == &pd_objectmaker)
		{
		    SETSYMBOL(msp, gensym(","));
		    break;
		}
		else goto gotmess;
	    case A_FLOAT:
	    case A_SYMBOL:
		*msp = *at;
		break;
	    case A_DOLLAR:
		if (at->a_w.w_index > 0 && at->a_w.w_index <= argc)
		    *msp = argv[at->a_w.w_index-1];
	    	else if (at->a_w.w_index == 0)
		    SETFLOAT(msp, canvas_getdollarzero());
		else
		{
		    if (target == &pd_objectmaker)
		    	SETFLOAT(msp, 0);
		    else
		    {
		    	error("$%d: argument number out of range",
			    at->a_w.w_index);
		    	SETFLOAT(msp, 0);
		    }
		}
		break;
	    case A_DOLLSYM:
	    	s9 = binbuf_realizedollsym(at->a_w.w_symbol, argc, argv,
		    target == &pd_objectmaker);
		if (!s9)
		    goto broken;
	    	SETSYMBOL(msp, s9);
		break;
	    default:
		bug("bad item in binbuf");
		goto broken;
	    }
	    msp++;
	    ac--;
	    at++;
	    nargs++;
	}
    gotmess:
	if (nargs)
	{
	    switch (stackwas->a_type)
	    {
	    case A_SYMBOL:
		typedmess(target, stackwas->a_w.w_symbol, nargs-1, stackwas+1);
		break;
	    case A_FLOAT:
		if (nargs == 1) pd_float(target, stackwas->a_w.w_float);
		else pd_list(target, 0, nargs, stackwas);
		break;
#ifdef ROCKBOX
            default:
                break;
#endif
	    }
	}
	msp = stackwas;
	if (!ac) break;
	target = nexttarget;
	at++;
	ac--;
    }

    return;
broken:
    msp = stackwas;
}

static int binbuf_doopen(char *s, int mode)
{
    char namebuf[MAXPDSTRING];
#ifdef MSW
    mode |= O_BINARY;
#endif
    sys_bashfilename(s, namebuf);
    return (open(namebuf, mode));
}

#ifndef ROCKBOX
static FILE *binbuf_dofopen(char *s, char *mode)
{
    char namebuf[MAXPDSTRING];
    sys_bashfilename(s, namebuf);
    return (fopen(namebuf, mode));
}
#endif

int binbuf_read(t_binbuf *b, char *filename, char *dirname, int crflag)
{
    long length;
    int fd;
    int readret;
    char *buf;
    char namebuf[MAXPDSTRING];

    namebuf[0] = 0;
    if (*dirname)
    	strcat(namebuf, dirname), strcat(namebuf, "/");
    strcat(namebuf, filename);

    if ((fd = binbuf_doopen(namebuf, 0)) < 0)
    {
#ifdef ROCKBOX
#ifdef SIMULATOR
        printf("open: ");
    	perror(namebuf);
#endif /* SIMULATOR */
#else /* ROCKBOX */
    	fprintf(stderr, "open: ");
    	perror(namebuf);
#endif /* ROCKBOX */
    	return (1);
    }
    if ((length = lseek(fd, 0, SEEK_END)) < 0 || lseek(fd, 0, SEEK_SET) < 0 
    	|| !(buf = t_getbytes(length)))
    {
#ifdef ROCKBOX
#ifdef SIMULATOR
        printf("lseek: ");
        perror(namebuf);
#endif /* SIMULATOR */
#else /* ROCKBOX */
    	fprintf(stderr, "lseek: ");
    	perror(namebuf);
#endif /* ROCKBOX */
    	close(fd);
    	return(1);
    }
    if ((readret = read(fd, buf, length)) < length)
    {
#ifdef ROCKBOX
#ifdef SIMULATOR
    	printf("read (%d %ld) -> %d\n", fd, length, readret);
    	perror(namebuf);
#endif /* SIMULATOR */
#else /* ROCKBOX */
    	fprintf(stderr, "read (%d %ld) -> %d\n", fd, length, readret);
    	perror(namebuf);
#endif /* ROCKBOX */
    	close(fd);
    	t_freebytes(buf, length);
    	return(1);
    }
    	/* optionally map carriage return to semicolon */
    if (crflag)
    {
    	int i;
    	for (i = 0; i < length; i++)
    	    if (buf[i] == '\n')
    	    	buf[i] = ';';
    }
    binbuf_text(b, buf, length);

#if 0
    startpost("binbuf_read "); postatom(b->b_n, b->b_vec); endpost();
#endif

    t_freebytes(buf, length);
    close(fd);
    return (0);
}

int binbuf_read_via_path(t_binbuf *b, char *filename, char *dirname,
    int crflag)
{
    int filedesc;
    char buf[MAXPDSTRING], *bufptr;
    if ((filedesc = open_via_path(
    	dirname, filename, "", buf, &bufptr, MAXPDSTRING, 0)) < 0)
    {
    	error("%s: can't open", filename);
    	return (1);
    }
    else close (filedesc);
    if (binbuf_read(b, bufptr, buf, crflag))
	return (1);
    else return (0);
}

#define WBUFSIZE 4096
static t_binbuf *binbuf_convert(t_binbuf *oldb, int maxtopd);

    /* write a binbuf to a text file.  If "crflag" is set we suppress
    semicolons. */
int binbuf_write(t_binbuf *x, char *filename, char *dir, int crflag)
{
#ifdef ROCKBOX
    int f = 0;
#else /* ROCKBOX */
    FILE *f = 0;
#endif /* ROCKBOX */
    char sbuf[WBUFSIZE], fbuf[MAXPDSTRING], *bp = sbuf, *ep = sbuf + WBUFSIZE;
    t_atom *ap;
    int indx, deleteit = 0;
    int ncolumn = 0;

    fbuf[0] = 0;
    if (*dir)
    	strcat(fbuf, dir), strcat(fbuf, "/");
    strcat(fbuf, filename);
    if (!strcmp(filename + strlen(filename) - 4, ".pat"))
    {
    	x = binbuf_convert(x, 0);
	deleteit = 1;
    }
    
#ifdef ROCKBOX
    if(!(f = binbuf_doopen(fbuf, O_WRONLY|O_CREAT|O_TRUNC)))
#else /* ROCKBOX */
    if (!(f = binbuf_dofopen(fbuf, "w")))
#endif /* ROCKBOX */
    {
#ifdef ROCKBOX
#ifdef SIMULATOR
        printf("open: ");
#endif /* SIMULATOR */
#else /* ROCKBOX */
    	fprintf(stderr, "open: ");
#endif /* ROCKBOX */
    	sys_unixerror(fbuf);
    	goto fail;
    }
    for (ap = x->b_vec, indx = x->b_n; indx--; ap++)
    {
    	int length;
	    /* estimate how many characters will be needed.  Printing out
	    symbols may need extra characters for inserting backslashes. */
    	if (ap->a_type == A_SYMBOL || ap->a_type == A_DOLLSYM)
    	    length = 80 + strlen(ap->a_w.w_symbol->s_name);
    	else length = 40;
    	if (ep - bp < length)
    	{
#ifdef ROCKBOX
            if(write(f, sbuf, bp-sbuf) < 1)
#else /* ROCKBOX */
    	    if (fwrite(sbuf, bp-sbuf, 1, f) < 1)
#endif /* ROCKBOX */
    	    {
    	    	sys_unixerror(fbuf);
    	    	goto fail;
    	    }
    	    bp = sbuf;
    	}
    	if ((ap->a_type == A_SEMI || ap->a_type == A_COMMA) &&
    	    bp > sbuf && bp[-1] == ' ') bp--;
    	if (!crflag || ap->a_type != A_SEMI)
    	{
    	    atom_string(ap, bp, (ep-bp)-2);
	    length = strlen(bp);
    	    bp += length;
	    ncolumn += length;
    	}
    	if (ap->a_type == A_SEMI || (!crflag && ncolumn > 65))
	{
	    *bp++ = '\n';
	    ncolumn = 0;
	}
    	else
	{
	    *bp++ = ' ';
	    ncolumn++;
	}
    }
#ifdef ROCKBOX
    if(write(f, sbuf, bp-sbuf) < 1)
#else /* ROCKBOX */
    if (fwrite(sbuf, bp-sbuf, 1, f) < 1)
#endif /* ROCKBOX */
    {
    	sys_unixerror(fbuf);
    	goto fail;
    }
    if (deleteit)
    	binbuf_free(x);
#ifdef ROCKBOX
    close(f);
#else /* ROCKBOX */
    fclose(f);
#endif /* ROCKBOX */
    return (0);
fail:
    if (deleteit)
    	binbuf_free(x);
    if (f)
#ifdef ROCKBOX
        close(f);
#else /* ROCKBOX */
        fclose(f);
#endif /* ROCKBOX */
    return (1);
}

/* The following routine attempts to convert from max to pd or back.  The
max to pd direction is working OK but you will need to make lots of 
abstractions for objects like "gate" which don't exist in Pd.  conversion
from Pd to Max hasn't been tested for patches with subpatches yet!  */

#define MAXSTACK 1000

#define ISSYMBOL(a, b) ((a)->a_type == A_SYMBOL && \
    !strcmp((a)->a_w.w_symbol->s_name, (b)))

static t_binbuf *binbuf_convert(t_binbuf *oldb, int maxtopd)
{
    t_binbuf *newb = binbuf_new();
    t_atom *vec = oldb->b_vec;
    t_int n = oldb->b_n, nextindex, stackdepth = 0, stack[MAXSTACK],
    	nobj = 0, i;
    t_atom outmess[MAXSTACK], *nextmess;
    if (!maxtopd)
    	binbuf_addv(newb, "ss;", gensym("max"), gensym("v2"));
    for (nextindex = 0; nextindex < n; )
    {
    	int endmess, natom;
	char *first, *second;
    	for (endmess = nextindex; endmess < n && vec[endmess].a_type != A_SEMI;
	    endmess++)
	    	;
	if (endmess == n) break;
	if (endmess == nextindex || endmess == nextindex + 1
	    || vec[nextindex].a_type != A_SYMBOL ||
	    	vec[nextindex+1].a_type != A_SYMBOL)
	{
	    nextindex = endmess + 1;
	    continue;
	}
	natom = endmess - nextindex;
	if (natom > MAXSTACK-10) natom = MAXSTACK-10;
	nextmess = vec + nextindex;
	first = nextmess->a_w.w_symbol->s_name;
	second = (nextmess+1)->a_w.w_symbol->s_name;
	if (maxtopd)
	{
	    	/* case 1: importing a ".pat" file into Pd. */
		
		/* dollar signs in file translate to symbols */
	    for (i = 0; i < natom; i++)
	    {
		if (nextmess[i].a_type == A_DOLLAR)
		{
	    	    char buf[100];
#ifdef ROCKBOX
                    snprintf(buf, sizeof(buf), "$%d", nextmess[i].a_w.w_index);
#else /* ROCKBOX */
		    sprintf(buf, "$%d", nextmess[i].a_w.w_index);
#endif /* ROCKBOX */
		    SETSYMBOL(nextmess+i, gensym(buf));
		}
		else if (nextmess[i].a_type == A_DOLLSYM)
		{
	    	    char buf[100];
#ifdef ROCKBOX
                    snprintf(buf, sizeof(buf), "$%s", nextmess[i].a_w.w_symbol->s_name);
#else /* ROCKBOX */
		    sprintf(buf, "$%s", nextmess[i].a_w.w_symbol->s_name);
#endif /* ROCKBOX */
		    SETSYMBOL(nextmess+i, gensym(buf));
		}
	    }
	    if (!strcmp(first, "#N"))
	    {
	    	if (!strcmp(second, "vpatcher"))
		{
		    if (stackdepth >= MAXSTACK)
		    {
		    	post("too many embedded patches");
			return (newb);
		    }
		    stack[stackdepth] = nobj;
		    stackdepth++;
		    nobj = 0;
		    binbuf_addv(newb, "ssfffff;", 
		    	gensym("#N"), gensym("canvas"),
			    atom_getfloatarg(2, natom, nextmess),
			    atom_getfloatarg(3, natom, nextmess),
			    atom_getfloatarg(4, natom, nextmess) -
			    	atom_getfloatarg(2, natom, nextmess),
			    atom_getfloatarg(5, natom, nextmess) -
			    	atom_getfloatarg(3, natom, nextmess),
#ifdef ROCKBOX
                            10.0);
#else
			    (float)sys_defaultfont);
#endif
	    	}
	    }
	    if (!strcmp(first, "#P"))
	    {
	    	    /* drop initial "hidden" flag */
	    	if (!strcmp(second, "hidden"))
		{
		    nextmess++;
		    natom--;
		    second = (nextmess+1)->a_w.w_symbol->s_name;
		}
	    	if (natom >= 7 && !strcmp(second, "newobj")
		    && (ISSYMBOL(&nextmess[6], "patcher") ||
		    	ISSYMBOL(&nextmess[6], "p")))
		{
		    binbuf_addv(newb, "ssffss;",
		    	gensym("#X"), gensym("restore"),
			atom_getfloatarg(2, natom, nextmess),
			atom_getfloatarg(3, natom, nextmess),
			gensym("pd"), atom_getsymbolarg(7, natom, nextmess));
		    if (stackdepth) stackdepth--;
		    nobj = stack[stackdepth];
		    nobj++;
		}
		else if (!strcmp(second, "newex") || !strcmp(second, "newobj"))
		{
		    t_symbol *classname =
		    	atom_getsymbolarg(6, natom, nextmess);
		    if (classname == gensym("trigger") ||
		    	classname == gensym("t"))
		    {
		    	for (i = 7; i < natom; i++)
			    if (nextmess[i].a_type == A_SYMBOL &&
			    	nextmess[i].a_w.w_symbol == gensym("i"))
				    nextmess[i].a_w.w_symbol = gensym("f");
		    }
		    if (classname == gensym("table"))
		    	classname = gensym("TABLE");
	    	    SETSYMBOL(outmess, gensym("#X"));
	    	    SETSYMBOL(outmess + 1, gensym("obj"));
		    outmess[2] = nextmess[2];
		    outmess[3] = nextmess[3];
		    SETSYMBOL(outmess+4, classname);
		    for (i = 7; i < natom; i++)
			outmess[i-2] = nextmess[i];
	    	    SETSEMI(outmess + natom - 2);
		    binbuf_add(newb, natom - 1, outmess);
		    nobj++;
		}
		else if (!strcmp(second, "message") || 
	    	    !strcmp(second, "comment"))
		{
	    	    SETSYMBOL(outmess, gensym("#X"));
	    	    SETSYMBOL(outmess + 1, gensym(
			(strcmp(second, "message") ? "text" : "msg")));
		    outmess[2] = nextmess[2];
		    outmess[3] = nextmess[3];
		    for (i = 6; i < natom; i++)
			outmess[i-2] = nextmess[i];
	    	    SETSEMI(outmess + natom - 2);
		    binbuf_add(newb, natom - 1, outmess);
		    nobj++;
		}
		else if (!strcmp(second, "button"))
		{
		    binbuf_addv(newb, "ssffs;",
			gensym("#X"), gensym("obj"),
			atom_getfloatarg(2, natom, nextmess),
			atom_getfloatarg(3, natom, nextmess),
			gensym("bng"));
		    nobj++;
		}
		else if (!strcmp(second, "number") || !strcmp(second, "flonum"))
		{
		    binbuf_addv(newb, "ssff;",
			gensym("#X"), gensym("floatatom"),
			atom_getfloatarg(2, natom, nextmess),
			atom_getfloatarg(3, natom, nextmess));
		    nobj++;
		}
		else if (!strcmp(second, "slider"))
		{
		    float inc = atom_getfloatarg(7, natom, nextmess);
		    if (inc <= 0)
		    	inc = 1;
		    binbuf_addv(newb, "ssffsffffffsssfffffffff;",
			gensym("#X"), gensym("obj"),
			atom_getfloatarg(2, natom, nextmess),
			atom_getfloatarg(3, natom, nextmess),
			gensym("vsl"),
			atom_getfloatarg(4, natom, nextmess),
			atom_getfloatarg(5, natom, nextmess),
			atom_getfloatarg(6, natom, nextmess),
			atom_getfloatarg(6, natom, nextmess)
			    + (atom_getfloatarg(5, natom, nextmess) - 1) * inc,
		    	0., 0.,
			gensym("empty"), gensym("empty"), gensym("empty"),
			0., -8., 0., 8., -262144., -1., -1., 0., 1.);
		    nobj++;
		}
		else if (!strcmp(second, "toggle"))
		{
		    binbuf_addv(newb, "ssffs;",
			gensym("#X"), gensym("obj"),
			atom_getfloatarg(2, natom, nextmess),
			atom_getfloatarg(3, natom, nextmess),
			gensym("tgl"));
		    nobj++;
		}
		else if (!strcmp(second, "inlet"))
		{
		    binbuf_addv(newb, "ssffs;",
			gensym("#X"), gensym("obj"),
			atom_getfloatarg(2, natom, nextmess),
			atom_getfloatarg(3, natom, nextmess),
			gensym((natom > 5 ? "inlet~" : "inlet"))); 
		    nobj++;
		}
		else if (!strcmp(second, "outlet"))
		{
		    binbuf_addv(newb, "ssffs;",
			gensym("#X"), gensym("obj"),
			atom_getfloatarg(2, natom, nextmess),
			atom_getfloatarg(3, natom, nextmess),
			gensym((natom > 5 ? "outlet~" : "outlet"))); 
		    nobj++;
		}
	    	else if (!strcmp(second, "user"))
		{
		    binbuf_addv(newb, "ssffs;",
			gensym("#X"), gensym("obj"),
			atom_getfloatarg(3, natom, nextmess),
			atom_getfloatarg(4, natom, nextmess),
			atom_getsymbolarg(2, natom, nextmess)); 
		    nobj++;
		}
		else if (!strcmp(second, "connect")||
		    !strcmp(second, "fasten"))
		{
		    binbuf_addv(newb, "ssffff;",
			gensym("#X"), gensym("connect"),
			nobj - atom_getfloatarg(2, natom, nextmess) - 1,
			atom_getfloatarg(3, natom, nextmess),
			nobj - atom_getfloatarg(4, natom, nextmess) - 1,
			atom_getfloatarg(5, natom, nextmess)); 
		}
	    }
	}
	else	    /* Pd to Max */
	{
	    if (!strcmp(first, "#N"))
	    {
	    	if (!strcmp(second, "canvas"))
		{
		    if (stackdepth >= MAXSTACK)
		    {
		    	post("too many embedded patches");
			return (newb);
		    }
		    stack[stackdepth] = nobj;
		    stackdepth++;
		    nobj = 0;
		    binbuf_addv(newb, "ssffff;", 
		    	gensym("#N"), gensym("vpatcher"),
			    atom_getfloatarg(2, natom, nextmess),
			    atom_getfloatarg(3, natom, nextmess),
			    atom_getfloatarg(4, natom, nextmess),
			    atom_getfloatarg(5, natom, nextmess));
	    	}
	    }
	    if (!strcmp(first, "#X"))
	    {
	    	if (natom >= 5 && !strcmp(second, "restore")
		    && (ISSYMBOL (&nextmess[4], "pd")))
		{
		    binbuf_addv(newb, "ss;", gensym("#P"), gensym("pop"));
		    binbuf_addv(newb, "ssffffss;",
		    	gensym("#P"), gensym("newobj"),
			atom_getfloatarg(2, natom, nextmess),
			atom_getfloatarg(3, natom, nextmess), 50., 1.,
			gensym("patcher"),
			    atom_getsymbolarg(5, natom, nextmess));
		    if (stackdepth) stackdepth--;
		    nobj = stack[stackdepth];
		    nobj++;
		}
		else if (!strcmp(second, "obj"))
		{
		    t_symbol *classname =
		    	atom_getsymbolarg(4, natom, nextmess);
		    if (classname == gensym("inlet"))
		    	binbuf_addv(newb, "ssfff;", gensym("#P"),
			    gensym("inlet"),
			    atom_getfloatarg(2, natom, nextmess),
			    atom_getfloatarg(3, natom, nextmess),
			    15.);
		    else if (classname == gensym("inlet~"))
		    	binbuf_addv(newb, "ssffff;", gensym("#P"),
			    gensym("inlet"),
			    atom_getfloatarg(2, natom, nextmess),
			    atom_getfloatarg(3, natom, nextmess),
			    15., 1.);
		    else if (classname == gensym("outlet"))
		    	binbuf_addv(newb, "ssfff;", gensym("#P"),
			    gensym("outlet"),
			    atom_getfloatarg(2, natom, nextmess),
			    atom_getfloatarg(3, natom, nextmess),
			    15.);
		    else if (classname == gensym("outlet~"))
		    	binbuf_addv(newb, "ssffff;", gensym("#P"),
			    gensym("outlet"),
			    atom_getfloatarg(2, natom, nextmess),
			    atom_getfloatarg(3, natom, nextmess),
			    15., 1.);
		    else if (classname == gensym("bng"))
		    	binbuf_addv(newb, "ssffff;", gensym("#P"),
			    gensym("button"),
			    atom_getfloatarg(2, natom, nextmess),
			    atom_getfloatarg(3, natom, nextmess),
			    atom_getfloatarg(5, natom, nextmess), 0.);
		    else if (classname == gensym("tgl"))
		    	binbuf_addv(newb, "ssffff;", gensym("#P"),
			    gensym("toggle"),
			    atom_getfloatarg(2, natom, nextmess),
			    atom_getfloatarg(3, natom, nextmess),
			    atom_getfloatarg(5, natom, nextmess), 0.);
		    else if (classname == gensym("vsl"))
		    	binbuf_addv(newb, "ssffffff;", gensym("#P"),
			    gensym("slider"),
			    atom_getfloatarg(2, natom, nextmess),
			    atom_getfloatarg(3, natom, nextmess),
			    atom_getfloatarg(5, natom, nextmess),
			    atom_getfloatarg(6, natom, nextmess),
			    (atom_getfloatarg(8, natom, nextmess) -
			    	atom_getfloatarg(7, natom, nextmess)) /
				    (atom_getfloatarg(6, natom, nextmess) == 1? 1 :
				    	 atom_getfloatarg(6, natom, nextmess) - 1),
			    atom_getfloatarg(7, natom, nextmess));
		    else
		    {
	    	    	SETSYMBOL(outmess, gensym("#P"));
	    	    	SETSYMBOL(outmess + 1, gensym("newex"));
		    	outmess[2] = nextmess[2];
		    	outmess[3] = nextmess[3];
			SETFLOAT(outmess + 4, 50);
			SETFLOAT(outmess + 5, 1);
		    	for (i = 4; i < natom; i++)
			    outmess[i+2] = nextmess[i];
	    	    	SETSEMI(outmess + natom + 2);
		    	binbuf_add(newb, natom + 3, outmess);
		    }
		    nobj++;
		
		}
		else if (!strcmp(second, "msg") || 
	    	    !strcmp(second, "text"))
		{
	    	    SETSYMBOL(outmess, gensym("#P"));
	    	    SETSYMBOL(outmess + 1, gensym(
			(strcmp(second, "msg") ? "comment" : "message")));
		    outmess[2] = nextmess[2];
		    outmess[3] = nextmess[3];
		    SETFLOAT(outmess + 4, 50);
		    SETFLOAT(outmess + 5, 1);
		    for (i = 4; i < natom; i++)
			outmess[i+2] = nextmess[i];
	    	    SETSEMI(outmess + natom + 2);
		    binbuf_add(newb, natom + 3, outmess);
		    nobj++;
		}
		else if (!strcmp(second, "floatatom"))
		{
		    binbuf_addv(newb, "ssfff;",
			gensym("#P"), gensym("flonum"),
			atom_getfloatarg(2, natom, nextmess),
			atom_getfloatarg(3, natom, nextmess), 35);
		    nobj++;
		}
		else if (!strcmp(second, "connect"))
		{
		    binbuf_addv(newb, "ssffff;",
			gensym("#P"), gensym("connect"),
			nobj - atom_getfloatarg(2, natom, nextmess) - 1,
			atom_getfloatarg(3, natom, nextmess),
			nobj - atom_getfloatarg(4, natom, nextmess) - 1,
			atom_getfloatarg(5, natom, nextmess)); 
		}
	    }
	}
	nextindex = endmess + 1;
    }
    if (!maxtopd)
    	binbuf_addv(newb, "ss;", gensym("#P"), gensym("pop"));
#if 0
    binbuf_write(newb, "import-result.pd", "/tmp", 0);
#endif
    return (newb);
}

    /* function to support searching */
int binbuf_match(t_binbuf *inbuf, t_binbuf *searchbuf)
{
    int indexin, nmatched;
    for (indexin = 0; indexin <= inbuf->b_n - searchbuf->b_n; indexin++)
    {
    	for (nmatched = 0; nmatched < searchbuf->b_n; nmatched++)
	{
	    t_atom *a1 = &inbuf->b_vec[indexin + nmatched], 
	    	*a2 = &searchbuf->b_vec[nmatched];
	    if (a1->a_type != a2->a_type ||
	    	(a1->a_type == A_SYMBOL && a1->a_w.w_symbol != a2->a_w.w_symbol)
		    ||
	    	(a1->a_type == A_FLOAT && a1->a_w.w_float != a2->a_w.w_float)
		    ||
	    	(a1->a_type == A_DOLLAR && a1->a_w.w_index != a2->a_w.w_index)
		    ||
	    	(a1->a_type == A_DOLLSYM && a1->a_w.w_symbol != a2->a_w.w_symbol))
	    	    goto nomatch;
	}
	return (1);
    nomatch: ;
    }
    return (0);
}

void pd_doloadbang(void);

/* LATER make this evaluate the file on-the-fly. */
/* LATER figure out how to log errors */
void binbuf_evalfile(t_symbol *name, t_symbol *dir)
{
    t_binbuf *b = binbuf_new();
    int import = !strcmp(name->s_name + strlen(name->s_name) - 4, ".pat");
    	/* set filename so that new canvases can pick them up */
    int dspstate = canvas_suspend_dsp();
    glob_setfilename(0, name, dir);

    if (binbuf_read(b, name->s_name, dir->s_name, 0))
    {
#if !defined(ROCKBOX) || (defined(ROCKBOX) && defined(SIMULATOR))
    	perror(name->s_name);
#endif
    }
    else
    {
	if (import)
	{
    	    t_binbuf *newb = binbuf_convert(b, 1);
	    binbuf_free(b);
	    b = newb;
	}
	binbuf_eval(b, 0, 0, 0);
    }
    glob_setfilename(0, &s_, &s_);	/* bug fix by Krzysztof Czaja */
    binbuf_free(b);
    canvas_resume_dsp(dspstate);
}

void glob_evalfile(t_pd *ignore, t_symbol *name, t_symbol *dir)
{
    t_pd *x = 0;

#ifdef ROCKBOX
    (void) ignore;
#endif
    	/* even though binbuf_evalfile appears to take care of dspstate,
    	we have to do it again here, because canvas_startdsp() assumes
    	that all toplevel canvases are visible.  LATER check if this
	is still necessary -- probably not. */

    int dspstate = canvas_suspend_dsp();
    binbuf_evalfile(name, dir);
    while ((x != s__X.s_thing) && (x = s__X.s_thing))
    	vmess(x, gensym("pop"), "i", 1);
    pd_doloadbang();
    canvas_resume_dsp(dspstate);
}

