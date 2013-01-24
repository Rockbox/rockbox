/*
** Note that Microsoft's own header files won't compile without
** "language extensions" (anonymous structs/unions) turned on.
** And even with that, it still gives a warning in rpcasync.h:
**    warning C4115: '_RPC_ASYNC_STATE' : named type definition in parentheses
** It compiles clean in C++.  Oy vey.  So I turned off the warning. --lch
*/
#pragma warning(disable: 4115)
#include <windows.h>
#pragma warning(default: 4115)
#include <direct.h>

#define FICL_WANT_PLATFORM (1)

#define FICL_PLATFORM_OS             "Win32"
#define FICL_PLATFORM_ARCHITECTURE   "x86"

#define FICL_PLATFORM_BASIC_TYPES    (1)
#define FICL_PLATFORM_ALIGNMENT      (4)
#define FICL_PLATFORM_INLINE	     __inline

#define FICL_PLATFORM_HAS_2INTEGER   (1)
#define FICL_PLATFORM_HAS_FTRUNCATE  (1)

#define fstat       _fstat
#define stat        _stat
#define getcwd      _getcwd
#define chdir       _chdir
#define fileno      _fileno


extern int ftruncate(int fileno, size_t size);

typedef char ficlInteger8;
typedef unsigned char ficlUnsigned8;
typedef short ficlInteger16;
typedef unsigned short ficlUnsigned16;
typedef long ficlInteger32;
typedef unsigned long ficlUnsigned32;
typedef __int64 ficlInteger64;
typedef unsigned __int64 ficlUnsigned64;

typedef ficlInteger32 ficlInteger;
typedef ficlUnsigned32 ficlUnsigned;
typedef float ficlFloat;

typedef ficlInteger64 ficl2Integer;
typedef ficlUnsigned64 ficl2Unsigned;


#define FICL_MULTICALL_CALLTYPE_FUNCTION        (0)
#define FICL_MULTICALL_CALLTYPE_METHOD          (1)
#define FICL_MULTICALL_CALLTYPE_VIRTUAL_METHOD	(2)
#define FICL_MULTICALL_GET_CALLTYPE(flags)      ((flags) & 0x0f)

#define FICL_MULTICALL_RETURNTYPE_VOID          (0)
#define FICL_MULTICALL_RETURNTYPE_INTEGER       (16)
#define FICL_MULTICALL_RETURNTYPE_CSTRING       (32)
#define FICL_MULTICALL_RETURNTYPE_FLOAT         (48)
#define FICL_MULTICALL_GET_RETURNTYPE(flags)    ((flags) & 0xf0)

#define FICL_MULTICALL_REVERSE_ARGUMENTS        (1<<8)
#define FICL_MULTICALL_EXPLICIT_VTABLE          (1<<9) /* the vtable is specified on the stack */

