/* 
** win32.c
** submitted to Ficl by Larry Hastings, larry@hastings.org
**/

#include <sys/stat.h>
#include "ficl.h"


/*
**
** Heavy, undocumented wizardry here.
**
** In Win32, like most OSes, the buffered file I/O functions in the
** C API (functions that take a FILE * like fopen()) are implemented
** on top of the raw file I/O functions (functions that take an int,
** like open()).  However, in Win32, these functions in turn are
** implemented on top of the Win32 native file I/O functions (functions
** that take a HANDLE, like CreateFile()).  This behavior is undocumented
** but easy to deduce by reading the CRT/SRC directory.
**
** The below mishmash of typedefs and defines were copied from
** CRT/SRC/INTERNAL.H from MSVC.
**
** --lch
*/
typedef struct {
        long osfhnd;    /* underlying OS file HANDLE */
        char osfile;    /* attributes of file (e.g., open in text mode?) */
        char pipech;    /* one char buffer for handles opened on pipes */
#ifdef _MT
        int lockinitflag;
        CRITICAL_SECTION lock;
#endif  /* _MT */
    }   ioinfo;
extern _CRTIMP ioinfo * __pioinfo[];

#define IOINFO_L2E          5
#define IOINFO_ARRAY_ELTS   (1 << IOINFO_L2E)
#define _pioinfo(i) ( __pioinfo[(i) >> IOINFO_L2E] + ((i) & (IOINFO_ARRAY_ELTS - \
                              1)) )
#define _osfhnd(i)  ( _pioinfo(i)->osfhnd )


int ficlFileTruncate(ficlFile *ff, ficlUnsigned size)
{
    HANDLE hFile = (HANDLE)_osfhnd(_fileno(ff->f));
    if (SetFilePointer(hFile, size, NULL, FILE_BEGIN) != size)
        return 0;
    return !SetEndOfFile(hFile);
}


int ficlFileStatus(char *filename, int *status)
{
    /*
	** The Windows documentation for GetFileAttributes() says it returns
    ** INVALID_FILE_ATTRIBUTES on error.  There's no such #define.  The
    ** return value for error is -1, so we'll just use that.
	*/
    DWORD attributes = GetFileAttributes(filename);
	if (attributes == -1)
	{
		*status = GetLastError();
		return -1;
	}
    *status = attributes;
    return 0;
}


long ficlFileSize(ficlFile *ff)
{
    struct stat statbuf;
    if (ff == NULL)
        return -1;
	
    statbuf.st_size = -1;
    if (fstat(fileno(ff->f), &statbuf) != 0)
        return -1;
	
    return statbuf.st_size;
}





void *ficlMalloc(size_t size)
{
    return malloc(size);
}

void *ficlRealloc(void *p, size_t size)
{
    return realloc(p, size);
}

void ficlFree(void *p)
{
    free(p);
}

void  ficlCallbackDefaultTextOut(ficlCallback *callback, char *message)
{
    FICL_IGNORE(callback);
    if (message != NULL)
        fputs(message, stdout);
    else
        fflush(stdout);
    return;
}



/*
**
** Platform-specific functions
**
*/


/* 
** m u l t i c a l l
**
** The be-all, end-all, swiss-army-chainsaw of native function call methods in Ficl.
**
** Usage:
** ( x*argumentCount [this] [vtable] argumentCount floatArgumentBitfield cstringArgumentBitfield functionAddress flags -- returnValue | )
** Note that any/all of the arguments (x*argumentCount) and the return value can use the
** float stack instead of the data stack.
**
** To call a simple native function:
**   call with flags = MULTICALL_CALLTYPE_FUNCTION
** To call a method on an object:
**   pass in the "this" pointer just below argumentCount,
**   call with flags = MULTICALL_CALLTYPE_METHOD
**   *do not* include the "this" pointer for the purposes of argumentCount
** To call a virtual method on an object:
**   pass in the "this" pointer just below argumentCount,
**   call with flags = MULTICALL_CALLTYPE_VIRTUAL_METHOD
**   *do not* include the "this" pointer for the purposes of argumentCount
**   the function address must be the offset into the vtable for that function
** It doesn't matter whether the function you're calling is "stdcall" (caller pops
** the stack) or "fastcall" (callee pops the stack); for robustness, multicall
** always restores the original stack pointer anyway.
**
**
** To handle floating-point arguments:
**   To thunk an argument from the float stack instead of the data stack, set the corresponding bit
**   in the "floatArgumentBitfield" argument.  Argument zero is bit 0 (1), argument one is bit 1 (2),
**   argument 2 is is bit 2 (4), argument 3 is bit 3 (8), etc.  For instance, to call this function:
**      float greasyFingers(int a, float b, int c, float d)
**   you would call
**      4  \ argumentCount
**      2 8 or \ floatArgumentBitfield, thunk argument 2 (2) and 4 (8)
**      0 \ cstringArgumentBitfield, don't thunk any arguments
**      (addressOfGreasyFingers)  MULTICALL-CALLTYPE-FUNCTION MULTICALL-RETURNTYPE-FLOAT or multicall
**
** To handle automatic conversion of addr-u arguments to C-style strings:
**   This is much like handling float arguments.   The bit set in cstringArgumentBitfield specifies
**   the *length* argument (the higher of the two arguments) for each addr-u you want converted.
**   You must count *both* arguments for the purposes of the argumentCount parameter.
**   For instance, to call the Win32 function MessageBoxA:
**	
**      0 "Howdy there!" "Title" 0
**      6  \ argument count is 6!  flags text-addr text-u title-addr title-u hwnd 
**      0  \ floatArgumentBitfield, don't thunk any float arguments
**      2 8 or  \ cstringArgumentBitfield, thunk for title-u (argument 2, 2) and text-u (argument 4, 8)
**      (addressOfMessageBoxA)  MULTICALL-CALLTYPE-FUNCTION MULTICALL-RETURNTYPE-INTEGER or multicall
**   The strings are copied to temporary storage and appended with a zero.  These strings are freed
**   before multicall returns.  If you need to call functions that write to these string buffers,
**   you'll need to handle thunking those arguments yourself.
**
** (If you want to call a function with more than 32 parameters, and do thunking, you need to hit somebody
**  in the head with a rock.  Note: this could be you!)
**
** Note that, big surprise, this function is really really really dependent
** on predefined behavior of Win32 and MSVC.  It would be non-zero amounts of
** work to port to Win64, Linux, other compilers, etc.
** 
** --lch
*/
static void ficlPrimitiveMulticall(ficlVm *vm)
{
    int flags;
    int functionAddress;
    int argumentCount;
    int *thisPointer;
    int integerReturnValue;
#if FICL_WANT_FLOAT
    float floatReturnValue;
#endif /* FICL_WANT_FLOAT */
    int cstringArguments;
    int floatArguments;
    int i;
    char **fixups;
    int fixupCount;
    int fixupIndex;
    int *argumentPointer;
    int finalArgumentCount;
    int argumentDirection;
    int *adjustedArgumentPointer;
    int originalESP;
    int vtable;

    flags = ficlStackPopInteger(vm->dataStack);

    functionAddress = ficlStackPopInteger(vm->dataStack);
    if (FICL_MULTICALL_GET_CALLTYPE(flags) == FICL_MULTICALL_CALLTYPE_VIRTUAL_METHOD)
        functionAddress *= 4;

    cstringArguments = ficlStackPopInteger(vm->dataStack);
    floatArguments = ficlStackPopInteger(vm->dataStack);
#if !FICL_WANT_FLOAT
    FICL_VM_ASSERT(vm, !floatArguments);
    FICL_VM_ASSERT(vm, FICL_MULTICALL_GET_RETURNTYPE(flags) != FICL_MULTICALL_RETURNTYPE_FLOAT);
#endif /* !FICL_WANT_FLOAT */
    argumentCount = ficlStackPopInteger(vm->dataStack);

    fixupCount = 0;
    if (cstringArguments)
    {
        for (i = 0; i < argumentCount; i++)
            if (cstringArguments & (1 << i))
                fixupCount++;
        fixups = (char **)malloc(fixupCount * sizeof(char *));
    }
    else
    {
        fixups = NULL;
    }


    /* argumentCount does *not* include the *this* pointer! */
    if (FICL_MULTICALL_GET_CALLTYPE(flags) != FICL_MULTICALL_CALLTYPE_FUNCTION)
    {
        if (flags & FICL_MULTICALL_EXPLICIT_VTABLE)
            vtable = ficlStackPopInteger(vm->dataStack);

        __asm push ecx
        thisPointer = (int *)ficlStackPopPointer(vm->dataStack);

        if ((flags & FICL_MULTICALL_EXPLICIT_VTABLE) == 0)
            vtable = *thisPointer;
	}


    __asm mov originalESP, esp

    fixupIndex = 0;
    finalArgumentCount = argumentCount - fixupCount;
    __asm mov argumentPointer, esp
    adjustedArgumentPointer = argumentPointer - finalArgumentCount;
    __asm mov esp, adjustedArgumentPointer
    if (flags & FICL_MULTICALL_REVERSE_ARGUMENTS)
    {
        argumentDirection = -1;
        argumentPointer--;
    }
    else
    {
        argumentPointer = adjustedArgumentPointer;
        argumentDirection = 1;
    }

    for (i = 0; i < argumentCount; i++)
    {
        int argument;

        /* a single argument can't be both a float and a cstring! */
        FICL_VM_ASSERT(vm, !((floatArguments & 1) && (cstringArguments & 1)));

#if FICL_WANT_FLOAT
        if (floatArguments & 1)
            argument = ficlStackPopInteger(vm->floatStack);
        else
#endif /* FICL_WANT_FLOAT */
            argument = ficlStackPopInteger(vm->dataStack);

        if (cstringArguments & 1)
        {
            int length;
            char *address;
            char *buffer;
            address = ficlStackPopPointer(vm->dataStack);
            length = argument;
            buffer = malloc(length + 1);
            memcpy(buffer, address, length);
            buffer[length] = 0;
            fixups[fixupIndex++] = buffer;
            argument = (int)buffer;
            argumentCount--;
            floatArguments >>= 1;
            cstringArguments >>= 1;
        }

        *argumentPointer = argument;
        argumentPointer += argumentDirection;

        floatArguments >>= 1;
        cstringArguments >>= 1;
    }


    /*
    ** note! leave the "mov ecx, thisPointer" code where it is.
    ** yes, it's duplicated in two spots.
    ** however, MSVC likes to use ecx as a scratch variable,
    ** so we want to set it as close as possible before the call.
    */
    if (FICL_MULTICALL_GET_CALLTYPE(flags) == FICL_MULTICALL_CALLTYPE_VIRTUAL_METHOD)
    {
        __asm
        {
            /* push thisPointer */
            mov ecx, thisPointer
            /* put vtable into eax. */
            mov eax, vtable
            /* pull out the address of the function we want... */
            add eax, functionAddress
            /* and call it. */
            call [eax]
        }
    }
    else
    {
        FICL_VM_ASSERT(vm, functionAddress != 0);
        if (FICL_MULTICALL_GET_CALLTYPE(flags))
        {
            __asm mov ecx, thisPointer
        }
        __asm call functionAddress
    }

    /* save off the return value, if there is one */
    __asm mov integerReturnValue, eax
#if FICL_WANT_FLOAT
    __asm fst floatReturnValue
#endif /* FICL_WANT_FLOAT */

    __asm mov esp, originalESP

    if (FICL_MULTICALL_GET_CALLTYPE(flags))
    {
        __asm pop ecx
    }

    if (FICL_MULTICALL_GET_RETURNTYPE(flags) == FICL_MULTICALL_RETURNTYPE_INTEGER)
        ficlStackPushInteger(vm->dataStack, integerReturnValue);
    else if (FICL_MULTICALL_GET_RETURNTYPE(flags) == FICL_MULTICALL_RETURNTYPE_CSTRING)
    {
        char *str = (char *)(void *)integerReturnValue;
        ficlStackPushInteger(vm->dataStack, integerReturnValue);
        ficlStackPushInteger(vm->dataStack, strlen(str));
    }
#if FICL_WANT_FLOAT
    else if (FICL_MULTICALL_GET_RETURNTYPE(flags) == FICL_MULTICALL_RETURNTYPE_FLOAT)
        ficlStackPushFloat(vm->floatStack, floatReturnValue);
#endif /* FICL_WANT_FLOAT */

    if (fixups != NULL)
    {
        for (i = 0; i < fixupCount; i++)
            if (fixups[i] != NULL)
                free(fixups[i]);
        free(fixups);
    }

    return;
}




/**************************************************************************
                        f i c l C o m p i l e P l a t f o r m
** Build Win32 platform extensions into the system dictionary
**************************************************************************/
void ficlSystemCompilePlatform(ficlSystem *system)
{
    HMODULE hModule;
    ficlDictionary *dictionary = system->dictionary;
    FICL_SYSTEM_ASSERT(system, dictionary);
    
    /*
    ** one native function call to rule them all, one native function call to find them,
    ** one native function call to bring them all and in the darkness bind them.
    ** --lch (with apologies to j.r.r.t.)
    */
    ficlDictionarySetPrimitive(dictionary, "multicall",      ficlPrimitiveMulticall,      FICL_WORD_DEFAULT);
    ficlDictionarySetConstant(dictionary, "multicall-calltype-function", FICL_MULTICALL_CALLTYPE_FUNCTION);
    ficlDictionarySetConstant(dictionary, "multicall-calltype-method", FICL_MULTICALL_CALLTYPE_METHOD);
    ficlDictionarySetConstant(dictionary, "multicall-calltype-virtual-method", FICL_MULTICALL_CALLTYPE_VIRTUAL_METHOD);
    ficlDictionarySetConstant(dictionary, "multicall-returntype-void", FICL_MULTICALL_RETURNTYPE_VOID);
    ficlDictionarySetConstant(dictionary, "multicall-returntype-integer", FICL_MULTICALL_RETURNTYPE_INTEGER);
    ficlDictionarySetConstant(dictionary, "multicall-returntype-cstring", FICL_MULTICALL_RETURNTYPE_CSTRING);
    ficlDictionarySetConstant(dictionary, "multicall-returntype-float", FICL_MULTICALL_RETURNTYPE_FLOAT);
    ficlDictionarySetConstant(dictionary, "multicall-reverse-arguments", FICL_MULTICALL_REVERSE_ARGUMENTS);
    ficlDictionarySetConstant(dictionary, "multicall-explit-vtable", FICL_MULTICALL_EXPLICIT_VTABLE);

    /*
    ** Every other Win32-specific word is implemented in Ficl, with multicall or whatnot.
    ** (Give me a lever, and a place to stand, and I will move the Earth.)
    ** See softcore/win32.fr for details.  --lch
    */
    hModule = LoadLibrary("kernel32.dll");
    ficlDictionarySetConstantPointer(dictionary, "kernel32.dll", hModule);
    ficlDictionarySetConstantPointer(dictionary, "(get-proc-address)", GetProcAddress(hModule, "GetProcAddress"));
    FreeLibrary(hModule);

    return;
}
