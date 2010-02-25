#ifdef ROCKBOX
#include "plugin.h"
#include "../../pdbox.h"
#else /* ROCKBOX */
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#endif /* ROCKBOX */

#include "../src/m_pd.h"
#include <../src/m_fixed.h>
#include "../src/g_canvas.h"

/* ------------------------ sfread~ ----------------------------- */

#include "sformat.h"

static t_class *sfread_class;


typedef struct _sfread
{
     t_object x_obj;
     void*     x_mapaddr;
     int       x_fd;

     int    x_play;
     int    x_channels;
     long long    x_size;
     int    x_loop;
     t_time x_pos;

     t_sample x_skip;
     t_sample x_speed;

     t_glist * x_glist;
     t_outlet *x_bangout;
} t_sfread;


void sfread_open(t_sfread *x,t_symbol *filename)
{
#ifndef ROCKBOX
     struct stat  fstate;
#endif
     char fname[MAXPDSTRING];

     if (filename == &s_) {
	  post("sfread: open without filename");
	  return;
     }

     canvas_makefilename(glist_getcanvas(x->x_glist), filename->s_name,
			 fname, MAXPDSTRING);


     /* close the old file */

#ifdef ROCKBOX
     if (x->x_mapaddr) freebytes(x->x_mapaddr, x->x_size);
#else
     if (x->x_mapaddr) munmap(x->x_mapaddr,x->x_size);
#endif
     if (x->x_fd >= 0) close(x->x_fd);

     if ((x->x_fd = open(fname,O_RDONLY)) < 0)
     {
	  error("can't open %s",fname);
	  x->x_play = 0;
	  x->x_mapaddr = NULL;
	  return;
     }

     /* get the size */
#ifdef ROCKBOX
     x->x_size = rb->filesize(x->x_fd);
#else
     fstat(x->x_fd,&fstate);
     x->x_size = fstate.st_size;
#endif

     /* map the file into memory */

#ifdef ROCKBOX
     x->x_mapaddr = getbytes(x->x_size);
     if (!x->x_mapaddr)
     {
	  error("can't mmap %s",fname);
	  return;
     }
     int r = read(x->x_fd, x->x_mapaddr, x->x_size);
     if (r != x->x_size)
     {
	  error("can't mmap %s",fname);
	  return;
     }
#else
     if (!(x->x_mapaddr = mmap(NULL,x->x_size,PROT_READ,MAP_PRIVATE,x->x_fd,0)))
     {
	  error("can't mmap %s",fname);
	  return;
     }
#endif
}

#define MAX_CHANS 4

static t_int *sfread_perform(t_int *w)
{
     t_sfread* x = (t_sfread*)(w[1]);
     short* buf = x->x_mapaddr;
     t_time tmp;
     int c = x->x_channels;
     t_time pos = x->x_pos;
     t_sample speed = x->x_speed;
     t_time end =  itofix(x->x_size/sizeof(short)/c);
     int i,n;
     t_sample* out[MAX_CHANS];
     
     for (i=0;i<c;i++)  
	  out[i] = (t_sample *)(w[3+i]);
     n = (int)(w[3+c]);
     
     /* loop */

     if (pos >  end)
	  pos = end;

     if (pos + n*speed >= end) { // playing forward end
	  if (!x->x_loop) {
	       x->x_play=0;
	  }
	  pos = x->x_skip;
     }
     tmp = n*speed;

     if (pos + n*speed <= 0) {  // playing backwards end
	  if (!x->x_loop) {
	       x->x_play=0;
	  }
          pos = end;

     }
     if (x->x_play && x->x_mapaddr) {
       t_time aoff = fixtoi(pos)*c;
       while (n--) {
	 long frac = (long long)((1<<fix1)-1)&pos;

	 for (i=0;i<c;i++)  {
	   t_sample bs = *(buf+aoff+i)<<(fix1-16);
	   t_sample cs = *(buf+aoff+i+1)<<(fix1-16);
	   *out[i]++ = bs + mult((cs - bs),frac);
	  	   
	 }
	 pos += speed;
	 aoff = fixtoi(pos)*c;
	 if (aoff > end) { 
	   if (x->x_loop) pos = x->x_skip;
	   else break;
	 }
	 if (aoff < x->x_skip) {
	   if (x->x_loop) pos = end;
	   else break;
	 }
       }
       /* Fill with zero in case of end */ 
       n++;
       while (n--) 
	 for (i=0;i<c;i++)  
	   *out[i]++ = 0;
     }
     else {
       while (n--) {
	 for (i=0;i<c;i++)
	   *out[i]++ = 0.;
       }
     }
     
     x->x_pos = pos;
     return (w+c+4);
}


static void sfread_float(t_sfread *x, t_floatarg f)
{
     int t = f;
     if (t && x->x_mapaddr) {
	  x->x_play=1;
     }
     else {
	  x->x_play=0;
     }

}

static void sfread_loop(t_sfread *x, t_floatarg f)
{
     x->x_loop = f;
}



static void sfread_size(t_sfread* x)
{
     t_atom a;

     SETFLOAT(&a,x->x_size*0.5/x->x_channels);
     outlet_list(x->x_bangout, gensym("size"),1,&a);
}

static void sfread_state(t_sfread* x)
{
     t_atom a;

     SETFLOAT(&a,x->x_play);
     outlet_list(x->x_bangout, gensym("state"),1,&a);
}




static void sfread_bang(t_sfread* x)
{
     x->x_pos = x->x_skip;
     sfread_float(x,1.0);
}


static void sfread_dsp(t_sfread *x, t_signal **sp)
{
/*     post("sfread: dsp"); */
     switch (x->x_channels) {
     case 1:
	  dsp_add(sfread_perform, 4, x, sp[0]->s_vec, 
		  sp[1]->s_vec, sp[0]->s_n);
	  break;
     case 2:
	  dsp_add(sfread_perform, 5, x, sp[0]->s_vec, 
		  sp[1]->s_vec,sp[2]->s_vec, sp[0]->s_n);
	  break;
     case 4:
	  dsp_add(sfread_perform, 6, x, sp[0]->s_vec, 
		  sp[1]->s_vec,sp[2]->s_vec,
		  sp[3]->s_vec,sp[4]->s_vec,
		  sp[0]->s_n);
	  break;
     }
}


static void sfread_speed(t_sfread* x, t_floatarg f) 
{
  x->x_speed = ftofix(f);
}

static void sfread_offset(t_sfread* x, t_floatarg f) 
{
  x->x_pos = (int)f;
}

static void *sfread_new(t_floatarg chan,t_floatarg skip)
{
#ifdef ROCKBOX
    (void) skip;
#endif
    t_sfread *x = (t_sfread *)pd_new(sfread_class);
    t_int c = chan;

    x->x_glist = (t_glist*) canvas_getcurrent();

    if (c<1 || c > MAX_CHANS) c = 1;
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("ft1"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("ft2"));


    x->x_fd = -1;
    x->x_mapaddr = NULL;

    x->x_size = 0;
    x->x_loop = 0;
    x->x_channels = c;
    x->x_mapaddr=NULL;
    x->x_pos = 0;
    x->x_skip = 0;
    x->x_speed = ftofix(1.0);
    x->x_play = 0;

    while (c--) {
	 outlet_new(&x->x_obj, gensym("signal"));
    }

     x->x_bangout = outlet_new(&x->x_obj, &s_float);

    return (x);
}

void sfread_tilde_setup(void)
{
     /* sfread */

    sfread_class = class_new(gensym("sfread~"), (t_newmethod)sfread_new, 0,
    	sizeof(t_sfread), 0,A_DEFFLOAT,A_DEFFLOAT,0);
    class_addmethod(sfread_class, nullfn, gensym("signal"), 0);
    class_addmethod(sfread_class, (t_method) sfread_dsp, gensym("dsp"), 0);
    class_addmethod(sfread_class, (t_method) sfread_open, gensym("open"), A_SYMBOL,A_NULL);
    class_addmethod(sfread_class, (t_method) sfread_size, gensym("size"), 0);
    class_addmethod(sfread_class, (t_method) sfread_offset, gensym("ft1"), A_FLOAT, A_NULL);
    class_addmethod(sfread_class, (t_method) sfread_speed, gensym("ft2"), A_FLOAT, A_NULL);
    class_addmethod(sfread_class, (t_method) sfread_state, gensym("state"), 0);
    class_addfloat(sfread_class, sfread_float);
    class_addbang(sfread_class,sfread_bang);
    class_addmethod(sfread_class,(t_method)sfread_loop,gensym("loop"),A_FLOAT,A_NULL);

}

