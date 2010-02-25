/* Copyright (c) 1997-1999 Miller Puckette.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#ifdef ROCKBOX
#include "plugin.h"
#include "../../pdbox.h"
#else /* ROCKBOX */
#ifdef DL_OPEN
#include <dlfcn.h>
#endif
#ifdef UNIX
#include <stdlib.h>
#include <unistd.h>
#endif
#ifdef MSW
#include <io.h>
#include <windows.h>
#endif
#ifdef MACOSX
#include <mach-o/dyld.h> 
#endif
#include <string.h>
#include <stdio.h>
#endif /* ROCKBOX */
#include "m_pd.h"
#include "s_stuff.h"

typedef void (*t_xxx)(void);

#ifndef ROCKBOX
static char sys_dllextent[] = 
#ifdef __FreeBSD__
    ".pd_freebsd";
#endif
#ifdef IRIX
#ifdef N32
    ".pd_irix6";
#else
    ".pd_irix5";
#endif
#endif
#ifdef __linux__
    ".pd_linux";
#endif
#ifdef MACOSX
    ".pd_darwin";
#endif
#ifdef MSW
    ".dll";
#endif
#endif /* ROCKBOX */

void class_set_extern_dir(t_symbol *s);

#ifdef STATIC
int sys_load_lib(char *dirname, char *classname)
#ifdef ROCKBOX
{
    (void) dirname;
    (void) classname;

    return 0;
}
#else /* ROCKBOX */
{ return 0;}
#endif /* ROCKBOX */
#else
int sys_load_lib(char *dirname, char *classname)
{
    char symname[MAXPDSTRING], filename[MAXPDSTRING], dirbuf[MAXPDSTRING],
    	classname2[MAXPDSTRING], *nameptr, *lastdot;
    void *dlobj;
    t_xxx makeout = NULL;
    int fd;
#ifdef MSW
    HINSTANCE ntdll;
#endif
#if 0
    fprintf(stderr, "lib %s %s\n", dirname, classname);
#endif
    	/* try looking in the path for (classname).(sys_dllextent) ... */
    if ((fd = open_via_path(dirname, classname, sys_dllextent,
    	dirbuf, &nameptr, MAXPDSTRING, 1)) < 0)
    {
    	    /* next try (classname)/(classname).(sys_dllextent) ... */
	strncpy(classname2, classname, MAXPDSTRING);
	filename[MAXPDSTRING-2] = 0;
	strcat(classname2, "/");
	strncat(classname2, classname, MAXPDSTRING-strlen(classname2));
	filename[MAXPDSTRING-1] = 0;
	if ((fd = open_via_path(dirname, classname2, sys_dllextent,
    	    dirbuf, &nameptr, MAXPDSTRING, 1)) < 0)
	{
    	    return (0);
    	}
    }


    close(fd);
    class_set_extern_dir(gensym(dirbuf));

    	/* refabricate the pathname */
    strncpy(filename, dirbuf, MAXPDSTRING);
    filename[MAXPDSTRING-2] = 0;
    strcat(filename, "/");
    strncat(filename, nameptr, MAXPDSTRING-strlen(filename));
    filename[MAXPDSTRING-1] = 0;
    	/* extract the setup function name */
    if (lastdot = strrchr(nameptr, '.'))
	*lastdot = 0;

#ifdef MACOSX
    strcpy(symname, "_");
    strcat(symname, nameptr);
#else
    strcpy(symname, nameptr);
#endif
	/* if the last character is a tilde, replace with "_tilde" */
    if (symname[strlen(symname) - 1] == '~')
	strcpy(symname + (strlen(symname) - 1), "_tilde");
	/* and append _setup to form the C setup function name */
    strcat(symname, "_setup");
#ifdef DL_OPEN
    dlobj = dlopen(filename, RTLD_NOW | RTLD_GLOBAL);
    if (!dlobj)
    {
	post("%s: %s", filename, dlerror());
    	class_set_extern_dir(&s_);
	return (0);
    }
    makeout = (t_xxx)dlsym(dlobj,  symname);
#endif
#ifdef MSW
    sys_bashfilename(filename, filename);
    ntdll = LoadLibrary(filename);
    if (!ntdll)
    {
	post("%s: couldn't load", filename);
    	class_set_extern_dir(&s_);
	return (0);
    }
    makeout = (t_xxx)GetProcAddress(ntdll, symname);  
#endif
#ifdef MACOSX
    {
        NSObjectFileImage image; 
        void *ret;
        NSSymbol s; 
        if ( NSCreateObjectFileImageFromFile( filename, &image) != NSObjectFileImageSuccess )
        {
            post("%s: couldn't load", filename);
    	    class_set_extern_dir(&s_);
            return 0;
        }
	ret = NSLinkModule( image, filename, 
	       NSLINKMODULE_OPTION_BINDNOW + NSLINKMODULE_OPTION_PRIVATE); 

        s = NSLookupSymbolInModule(ret, symname); 

        if (s)
            makeout = (t_xxx)NSAddressOfSymbol( s);
        else makeout = 0;
    }
#endif

    if (!makeout)
    {
    	post("load_object: Symbol \"%s\" not found", symname);
    	class_set_extern_dir(&s_);
    	return 0;
    }
    (*makeout)();
    class_set_extern_dir(&s_);
    return (1);
}

#endif

