/* In MSW, this is all there is to pd; the rest sits in a "pdlib" dll so
that externs can link back to functions defined in pd. */

#include <stdio.h>

int sys_main(int argc, char **argv);

	/* WINBASEAPI PVOID WINAPI AddVectoredExceptionHandler(
    		ULONG FirstHandler,
    		PVECTORED_EXCEPTION_HANDLER VectoredHandler ); */

#ifdef MSW
#if 0
#incldue "winbase.h"

LONG NTAPI VectoredExceptionHandler(void *PEXCEPTION_POINTERS)
{
	fprintf(stderr, "caught exception\n");
	return(EXCEPTION_CONTINUE_SEARCH);
}


int main(int argc, char **argv)
{
    printf("Pd entry point\n");
    AddVectoredExceptionHandler(
    ULONG FirstHandler,
    PVECTORED_EXCEPTION_HANDLER VectoredHandler );


#endif

#if 1
int main(int argc, char **argv)
{
    __try
    {
        sys_main(argc, argv);
    }
    __finally
    {
        printf("caught an exception; stopping\n");
    }
}
#endif
#else /* not MSW */
int main(int argc, char **argv)
{
    return (sys_main(argc, argv));
}
#endif
/* In MSW, this is all there is to pd; the rest sits in a "pdlib" dll so
that externs can link back to functions defined in pd. */

#include <stdio.h>

int sys_main(int argc, char **argv);

	/* WINBASEAPI PVOID WINAPI AddVectoredExceptionHandler(
    		ULONG FirstHandler,
    		PVECTORED_EXCEPTION_HANDLER VectoredHandler ); */

#ifdef MSW
#if 0
#incldue "winbase.h"

LONG NTAPI VectoredExceptionHandler(void *PEXCEPTION_POINTERS)
{
	fprintf(stderr, "caught exception\n");
	return(EXCEPTION_CONTINUE_SEARCH);
}


int main(int argc, char **argv)
{
    printf("Pd entry point\n");
    AddVectoredExceptionHandler(
    ULONG FirstHandler,
    PVECTORED_EXCEPTION_HANDLER VectoredHandler );


#endif

#if 1
int main(int argc, char **argv)
{
    __try
    {
        sys_main(argc, argv);
    }
    __finally
    {
        printf("caught an exception; stopping\n");
    }
}
#endif
#else /* not MSW */
int main(int argc, char **argv)
{
    return (sys_main(argc, argv));
}
#endif
