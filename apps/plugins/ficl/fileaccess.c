#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "ficl.h"

#if FICL_WANT_FILE
/*
**
** fileaccess.c
**
** Implements all of the File Access word set that can be implemented in portable C.
**
*/

static void pushIor(ficlVm *vm, int success)
{
    int ior;
    if (success)
        ior = 0;
    else
        ior = errno;
    ficlStackPushInteger(vm->dataStack, ior);
}



static void ficlFileOpen(ficlVm *vm, char *writeMode) /* ( c-addr u fam -- fileid ior ) */
{
    int fam = ficlStackPopInteger(vm->dataStack);
    int length = ficlStackPopInteger(vm->dataStack);
    void *address = (void *)ficlStackPopPointer(vm->dataStack);
    char mode[4];
    FILE *f;
    char *filename = (char *)malloc(length + 1);
    memcpy(filename, address, length);
    filename[length] = 0;

    *mode = 0;

    switch (FICL_FAM_OPEN_MODE(fam))
        {
        case 0:
            ficlStackPushPointer(vm->dataStack, NULL);
            ficlStackPushInteger(vm->dataStack, EINVAL);
            goto EXIT;
        case FICL_FAM_READ:
            strcat(mode, "r");
            break;
        case FICL_FAM_WRITE:
            strcat(mode, writeMode);
            break;
        case FICL_FAM_READ | FICL_FAM_WRITE:
            strcat(mode, writeMode);
            strcat(mode, "+");
            break;
        }

    strcat(mode, (fam & FICL_FAM_BINARY) ? "b" : "t");

    f = fopen(filename, mode);
    if (f == NULL)
        ficlStackPushPointer(vm->dataStack, NULL);
    else
        {
        ficlFile *ff = (ficlFile *)malloc(sizeof(ficlFile));
        strcpy(ff->filename, filename);
        ff->f = f;
        ficlStackPushPointer(vm->dataStack, ff);

        fseek(f, 0, SEEK_SET);
        }
    pushIor(vm, f != NULL);
	
EXIT:
	free(filename);
}



static void ficlPrimitiveOpenFile(ficlVm *vm) /* ( c-addr u fam -- fileid ior ) */
{
    ficlFileOpen(vm, "a");
}


static void ficlPrimitiveCreateFile(ficlVm *vm) /* ( c-addr u fam -- fileid ior ) */
{
    ficlFileOpen(vm, "w");
}


static int ficlFileClose(ficlFile *ff) /* ( fileid -- ior ) */
{
    FILE *f = ff->f;
    free(ff);
    return !fclose(f);
}

static void ficlPrimitiveCloseFile(ficlVm *vm) /* ( fileid -- ior ) */
{
    ficlFile *ff = (ficlFile *)ficlStackPopPointer(vm->dataStack);
    pushIor(vm, ficlFileClose(ff));
}

static void ficlPrimitiveDeleteFile(ficlVm *vm) /* ( c-addr u -- ior ) */
{
    int length = ficlStackPopInteger(vm->dataStack);
    void *address = (void *)ficlStackPopPointer(vm->dataStack);

    char *filename = (char *)malloc(length + 1);
    memcpy(filename, address, length);
    filename[length] = 0;

    pushIor(vm, !unlink(filename));
	free(filename);
}

static void ficlPrimitiveRenameFile(ficlVm *vm) /* ( c-addr1 u1 c-addr2 u2 -- ior ) */
{
    int length;
    void *address;
    char *from;
    char *to;

    length = ficlStackPopInteger(vm->dataStack);
    address = (void *)ficlStackPopPointer(vm->dataStack);
    to = (char *)malloc(length + 1);
    memcpy(to, address, length);
    to[length] = 0;

    length = ficlStackPopInteger(vm->dataStack);
    address = (void *)ficlStackPopPointer(vm->dataStack);

    from = (char *)malloc(length + 1);
    memcpy(from, address, length);
    from[length] = 0;

    pushIor(vm, !rename(from, to));

	free(from);
	free(to);
}

static void ficlPrimitiveFileStatus(ficlVm *vm) /* ( c-addr u -- x ior ) */
{
	int status;
	int ior;
	
    int length = ficlStackPopInteger(vm->dataStack);
    void *address = (void *)ficlStackPopPointer(vm->dataStack);

    char *filename = (char *)malloc(length + 1);
    memcpy(filename, address, length);
    filename[length] = 0;

	ior = ficlFileStatus(filename, &status);
	free(filename);

    ficlStackPushInteger(vm->dataStack, status);
    ficlStackPushInteger(vm->dataStack, ior);
}


static void ficlPrimitiveFilePosition(ficlVm *vm) /* ( fileid -- ud ior ) */
{
    ficlFile *ff = (ficlFile *)ficlStackPopPointer(vm->dataStack);
    long ud = ftell(ff->f);
    ficlStackPushInteger(vm->dataStack, ud);
    pushIor(vm, ud != -1);
}



static void ficlPrimitiveFileSize(ficlVm *vm) /* ( fileid -- ud ior ) */
{
    ficlFile *ff = (ficlFile *)ficlStackPopPointer(vm->dataStack);
    long ud = ficlFileSize(ff);
    ficlStackPushInteger(vm->dataStack, ud);
    pushIor(vm, ud != -1);
}



#define nLINEBUF 256
static void ficlPrimitiveIncludeFile(ficlVm *vm) /* ( i*x fileid -- j*x ) */
{
    ficlFile *ff = (ficlFile *)ficlStackPopPointer(vm->dataStack);
    ficlCell id = vm->sourceId;
    int  except = FICL_VM_STATUS_OUT_OF_TEXT;
    long currentPosition, totalSize;
    long size;
	ficlString s;
    vm->sourceId.p = (void *)ff;

    currentPosition = ftell(ff->f);
    totalSize = ficlFileSize(ff);
    size = totalSize - currentPosition;

    if ((totalSize != -1) && (currentPosition != -1) && (size > 0))
    {
        char *buffer = (char *)malloc(size);
        long got = fread(buffer, 1, size, ff->f);
        if (got == size)
		{
			FICL_STRING_SET_POINTER(s, buffer);
			FICL_STRING_SET_LENGTH(s, size);
            except = ficlVmExecuteString(vm, s);
		}
    }

    if ((except < 0) && (except != FICL_VM_STATUS_OUT_OF_TEXT))
        ficlVmThrow(vm, except);
	
    /*
    ** Pass an empty line with SOURCE-ID == -1 to flush
    ** any pending REFILLs (as required by FILE wordset)
    */
    vm->sourceId.i = -1;
	FICL_STRING_SET_FROM_CSTRING(s, "");
    ficlVmExecuteString(vm, s);

    vm->sourceId = id;
    ficlFileClose(ff);
}



static void ficlPrimitiveReadFile(ficlVm *vm) /* ( c-addr u1 fileid -- u2 ior ) */
{
    ficlFile *ff = (ficlFile *)ficlStackPopPointer(vm->dataStack);
    int length = ficlStackPopInteger(vm->dataStack);
    void *address = (void *)ficlStackPopPointer(vm->dataStack);
    int result;

    clearerr(ff->f);
    result = fread(address, 1, length, ff->f);

    ficlStackPushInteger(vm->dataStack, result);
    pushIor(vm, ferror(ff->f) == 0);
}



static void ficlPrimitiveReadLine(ficlVm *vm) /* ( c-addr u1 fileid -- u2 flag ior ) */
{
    ficlFile *ff = (ficlFile *)ficlStackPopPointer(vm->dataStack);
    int length = ficlStackPopInteger(vm->dataStack);
    char *address = (char *)ficlStackPopPointer(vm->dataStack);
    int error;
    int flag;

    if (feof(ff->f))
        {
        ficlStackPushInteger(vm->dataStack, -1);
        ficlStackPushInteger(vm->dataStack, 0);
        ficlStackPushInteger(vm->dataStack, 0);
        return;
        }

    clearerr(ff->f);
    *address = 0;
    fgets(address, length, ff->f);

    error = ferror(ff->f);
    if (error != 0)
        {
        ficlStackPushInteger(vm->dataStack, -1);
        ficlStackPushInteger(vm->dataStack, 0);
        ficlStackPushInteger(vm->dataStack, error);
        return;
        }

    length = strlen(address);
    flag = (length > 0);
    if (length && ((address[length - 1] == '\r') || (address[length - 1] == '\n')))
        length--;
    
    ficlStackPushInteger(vm->dataStack, length);
    ficlStackPushInteger(vm->dataStack, flag);
    ficlStackPushInteger(vm->dataStack, 0); /* ior */
}



static void ficlPrimitiveWriteFile(ficlVm *vm) /* ( c-addr u1 fileid -- ior ) */
{
    ficlFile *ff = (ficlFile *)ficlStackPopPointer(vm->dataStack);
    int length = ficlStackPopInteger(vm->dataStack);
    void *address = (void *)ficlStackPopPointer(vm->dataStack);

    clearerr(ff->f);
    fwrite(address, 1, length, ff->f);
    pushIor(vm, ferror(ff->f) == 0);
}



static void ficlPrimitiveWriteLine(ficlVm *vm) /* ( c-addr u1 fileid -- ior ) */
{
    ficlFile *ff = (ficlFile *)ficlStackPopPointer(vm->dataStack);
    size_t length = (size_t)ficlStackPopInteger(vm->dataStack);
    void *address = (void *)ficlStackPopPointer(vm->dataStack);

    clearerr(ff->f);
    if (fwrite(address, 1, length, ff->f) == length)
        fwrite("\n", 1, 1, ff->f);
    pushIor(vm, ferror(ff->f) == 0);
}



static void ficlPrimitiveRepositionFile(ficlVm *vm) /* ( ud fileid -- ior ) */
{
    ficlFile *ff = (ficlFile *)ficlStackPopPointer(vm->dataStack);
    size_t ud = (size_t)ficlStackPopInteger(vm->dataStack);

    pushIor(vm, fseek(ff->f, ud, SEEK_SET) == 0);
}



static void ficlPrimitiveFlushFile(ficlVm *vm) /* ( fileid -- ior ) */
{
    ficlFile *ff = (ficlFile *)ficlStackPopPointer(vm->dataStack);
    pushIor(vm, fflush(ff->f) == 0);
}



#if FICL_PLATFORM_HAS_FTRUNCATE

static void ficlPrimitiveResizeFile(ficlVm *vm) /* ( ud fileid -- ior ) */
{
    ficlFile *ff = (ficlFile *)ficlStackPopPointer(vm->dataStack);
    size_t ud = (size_t)ficlStackPopInteger(vm->dataStack);

    pushIor(vm, ficlFileTruncate(ff, ud) == 0);
}

#endif /* FICL_PLATFORM_HAS_FTRUNCATE */

#endif /* FICL_WANT_FILE */



void ficlSystemCompileFile(ficlSystem *system)
{
#if !FICL_WANT_FILE
    FICL_IGNORE(system);
#else
    ficlDictionary *dictionary = ficlSystemGetDictionary(system);
    ficlDictionary *environment = ficlSystemGetEnvironment(system);

    FICL_SYSTEM_ASSERT(system, dictionary);
    FICL_SYSTEM_ASSERT(system, environment);

    ficlDictionarySetPrimitive(dictionary, "create-file", ficlPrimitiveCreateFile,  FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "open-file", ficlPrimitiveOpenFile,  FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "close-file", ficlPrimitiveCloseFile,  FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "include-file", ficlPrimitiveIncludeFile,  FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "read-file", ficlPrimitiveReadFile,  FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "read-line", ficlPrimitiveReadLine,  FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "write-file", ficlPrimitiveWriteFile,  FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "write-line", ficlPrimitiveWriteLine,  FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "file-position", ficlPrimitiveFilePosition,  FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "file-size", ficlPrimitiveFileSize,  FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "reposition-file", ficlPrimitiveRepositionFile,  FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "file-status", ficlPrimitiveFileStatus,  FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "flush-file", ficlPrimitiveFlushFile,  FICL_WORD_DEFAULT);

    ficlDictionarySetPrimitive(dictionary, "delete-file", ficlPrimitiveDeleteFile,  FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "rename-file", ficlPrimitiveRenameFile,  FICL_WORD_DEFAULT);

#if FICL_PLATFORM_HAS_FTRUNCATE
    ficlDictionarySetPrimitive(dictionary, "resize-file", ficlPrimitiveResizeFile,  FICL_WORD_DEFAULT);

    ficlDictionarySetConstant(environment, "file", FICL_TRUE);
    ficlDictionarySetConstant(environment, "file-ext", FICL_TRUE);
#else /*  FICL_PLATFORM_HAS_FTRUNCATE */
    ficlDictionarySetConstant(environment, "file", FICL_FALSE);
    ficlDictionarySetConstant(environment, "file-ext", FICL_FALSE);
#endif /* FICL_PLATFORM_HAS_FTRUNCATE */

#endif /* !FICL_WANT_FILE */
}
