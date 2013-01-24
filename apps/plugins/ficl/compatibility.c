#define FICL_FORCE_COMPATIBILITY 1
#include "ficl.h"


FICL_PLATFORM_EXTERN ficlStack *stackCreate   (unsigned cells) { return ficlStackCreate(NULL, "unknown", cells); }
FICL_PLATFORM_EXTERN void        stackDelete   (ficlStack *stack) { ficlStackDestroy(stack); }
FICL_PLATFORM_EXTERN int         stackDepth    (ficlStack *stack) { return ficlStackDepth(stack); }
FICL_PLATFORM_EXTERN void        stackDrop     (ficlStack *stack, int n) { ficlStackDrop(stack, n); }
FICL_PLATFORM_EXTERN ficlCell    stackFetch    (ficlStack *stack, int n) { return ficlStackFetch(stack, n); }
FICL_PLATFORM_EXTERN ficlCell    stackGetTop   (ficlStack *stack) { return ficlStackFetch(stack, 0); }
#if FICL_WANT_LOCALS
FICL_PLATFORM_EXTERN void        stackLink     (ficlStack *stack, int cells) { ficlStackLink(stack, cells); }
FICL_PLATFORM_EXTERN void        stackUnlink   (ficlStack *stack) { ficlStackUnlink(stack); }
#endif /* FICL_WANT_LOCALS */
FICL_PLATFORM_EXTERN void        stackPick     (ficlStack *stack, int n) { ficlStackPick(stack, n); }
FICL_PLATFORM_EXTERN ficlCell    stackPop      (ficlStack *stack) { return ficlStackPop(stack); }
FICL_PLATFORM_EXTERN void       *stackPopPtr   (ficlStack *stack) { return ficlStackPopPointer(stack); }
FICL_PLATFORM_EXTERN ficlUnsigned stackPopUNS   (ficlStack *stack) { return ficlStackPopUnsigned(stack); }
FICL_PLATFORM_EXTERN ficlInteger stackPopINT   (ficlStack *stack) { return ficlStackPopInteger(stack); }
FICL_PLATFORM_EXTERN void        stackPush     (ficlStack *stack, ficlCell cell) { ficlStackPush(stack, cell); }
FICL_PLATFORM_EXTERN void        stackPushPtr  (ficlStack *stack, void *pointer) { ficlStackPushPointer(stack, pointer); }
FICL_PLATFORM_EXTERN void        stackPushUNS  (ficlStack *stack, ficlUnsigned u) { ficlStackPushUnsigned(stack, u); }
FICL_PLATFORM_EXTERN void        stackPushINT  (ficlStack *stack, ficlInteger i) { ficlStackPushInteger(stack, i); }
FICL_PLATFORM_EXTERN void        stackReset    (ficlStack *stack) { ficlStackReset(stack); }
FICL_PLATFORM_EXTERN void        stackRoll     (ficlStack *stack, int n) { ficlStackRoll(stack, n); }
FICL_PLATFORM_EXTERN void        stackSetTop   (ficlStack *stack, ficlCell cell) { ficlStackSetTop(stack, cell); }
FICL_PLATFORM_EXTERN void        stackStore    (ficlStack *stack, int n, ficlCell cell) { ficlStackStore(stack, n, cell); }

#if (FICL_WANT_FLOAT)
FICL_PLATFORM_EXTERN ficlFloat   stackPopFloat (ficlStack *stack) { return ficlStackPopFloat(stack); }
FICL_PLATFORM_EXTERN void        stackPushFloat(ficlStack *stack, ficlFloat f) { ficlStackPushFloat(stack, f); }
#endif

FICL_PLATFORM_EXTERN int wordIsImmediate(ficlWord *word) { return ficlWordIsImmediate(word); }
FICL_PLATFORM_EXTERN int wordIsCompileOnly(ficlWord *word) { return ficlWordIsCompileOnly(word); }


FICL_PLATFORM_EXTERN void        vmBranchRelative(ficlVm *vm, int offset) { ficlVmBranchRelative(vm, offset); }
FICL_PLATFORM_EXTERN ficlVm     *vmCreate       (ficlVm *vm, unsigned nPStack, unsigned nRStack) { return ficlVmCreate(vm, nPStack, nRStack); }
FICL_PLATFORM_EXTERN void        vmDelete       (ficlVm *vm) { ficlVmDestroy(vm); }
FICL_PLATFORM_EXTERN void        vmExecute      (ficlVm *vm, ficlWord *word) { ficlVmExecuteWord(vm, word); }
FICL_PLATFORM_EXTERN ficlDictionary *vmGetDict  (ficlVm *vm) { return ficlVmGetDictionary(vm); }
FICL_PLATFORM_EXTERN char *      vmGetString    (ficlVm *vm, ficlCountedString *spDest, char delimiter) { return ficlVmGetString(vm, spDest, delimiter); }
FICL_PLATFORM_EXTERN ficlString  vmGetWord      (ficlVm *vm) { return ficlVmGetWord(vm); }
FICL_PLATFORM_EXTERN ficlString  vmGetWord0     (ficlVm *vm) { return ficlVmGetWord0(vm); }
FICL_PLATFORM_EXTERN int         vmGetWordToPad (ficlVm *vm) { return ficlVmGetWordToPad(vm); }
FICL_PLATFORM_EXTERN ficlString  vmParseString  (ficlVm *vm, char delimiter) { return ficlVmParseString(vm, delimiter); }
FICL_PLATFORM_EXTERN ficlString  vmParseStringEx(ficlVm *vm, char delimiter, char skipLeading) { return ficlVmParseStringEx(vm, delimiter, skipLeading); }
FICL_PLATFORM_EXTERN ficlCell    vmPop          (ficlVm *vm) { return ficlVmPop(vm); }
FICL_PLATFORM_EXTERN void        vmPush         (ficlVm *vm, ficlCell cell) { ficlVmPush(vm, cell); }
FICL_PLATFORM_EXTERN void        vmPopIP        (ficlVm *vm) { ficlVmPopIP(vm); }
FICL_PLATFORM_EXTERN void        vmPushIP       (ficlVm *vm, ficlIp newIP) { ficlVmPushIP(vm, newIP); }
FICL_PLATFORM_EXTERN void        vmQuit         (ficlVm *vm) { ficlVmQuit(vm); }
FICL_PLATFORM_EXTERN void        vmReset        (ficlVm *vm) { ficlVmReset(vm); }
FICL_PLATFORM_EXTERN void        vmThrow        (ficlVm *vm, int except) { ficlVmThrow(vm, except); }
FICL_PLATFORM_EXTERN void        vmThrowErr     (ficlVm *vm, char *fmt, ...) { va_list list; va_start(list, fmt); ficlVmThrowErrorVararg(vm, fmt, list); va_end(list); }

FICL_PLATFORM_EXTERN void        vmCheckStack(ficlVm *vm, int popCells, int pushCells) { FICL_IGNORE(vm); FICL_IGNORE(popCells); FICL_IGNORE(pushCells); FICL_STACK_CHECK(vm->dataStack, popCells, pushCells); }
#if FICL_WANT_FLOAT
FICL_PLATFORM_EXTERN void        vmCheckFStack(ficlVm *vm, int popCells, int pushCells) { FICL_IGNORE(vm); FICL_IGNORE(popCells); FICL_IGNORE(pushCells); FICL_STACK_CHECK(vm->floatStack, popCells, pushCells); }
#endif

FICL_PLATFORM_EXTERN void        vmPushTib  (ficlVm *vm, char *text, ficlInteger nChars, ficlTIB *pSaveTib) { ficlVmPushTib(vm, text, nChars, pSaveTib); }
FICL_PLATFORM_EXTERN void        vmPopTib   (ficlVm *vm, ficlTIB *pTib) { ficlVmPopTib(vm, pTib); }

FICL_PLATFORM_EXTERN int        isPowerOfTwo(ficlUnsigned u) { return ficlIsPowerOfTwo(u); }

#if defined(_WIN32)
/* #SHEESH
** Why do Microsoft Meatballs insist on contaminating
** my namespace with their string functions???
*/
#pragma warning(disable: 4273)
#endif
char       *ltoa(ficlInteger value, char *string, int radix ) { return ficlLtoa(value, string, radix); }
char       *ultoa(ficlUnsigned value, char *string, int radix ) { return ficlUltoa(value, string, radix); }
char       *strrev( char *string ) { return ficlStringReverse(string); }
#if defined(_WIN32)
#pragma warning(default: 4273)
#endif
FICL_PLATFORM_EXTERN char        digit_to_char(int value) { return ficlDigitToCharacter(value); }
FICL_PLATFORM_EXTERN char       *skipSpace(char *cp, char *end) { return ficlStringSkipSpace(cp, end); }
FICL_PLATFORM_EXTERN char       *caseFold(char *cp) { return ficlStringCaseFold(cp); }
FICL_PLATFORM_EXTERN int         strincmp(char *cp1, char *cp2, ficlUnsigned count) { return ficlStrincmp(cp1, cp2, count); }

FICL_PLATFORM_EXTERN void        hashForget    (ficlHash *hash, void *where) { ficlHashForget(hash, where); }
FICL_PLATFORM_EXTERN ficlUnsigned16 hashHashCode  (ficlString string) { return ficlHashCode(string); }
FICL_PLATFORM_EXTERN void        hashInsertWord(ficlHash *hash, ficlWord *word) { ficlHashInsertWord(hash, word); }
FICL_PLATFORM_EXTERN ficlWord   *hashLookup    (ficlHash *hash, ficlString string, ficlUnsigned16 hashCode) { return ficlHashLookup(hash, string, hashCode); }
FICL_PLATFORM_EXTERN void        hashReset     (ficlHash *hash) { ficlHashReset(hash); }


FICL_PLATFORM_EXTERN void       *alignPtr(void *ptr) { return ficlAlignPointer(ptr); }
FICL_PLATFORM_EXTERN void        dictAbortDefinition(ficlDictionary *dictionary) { ficlDictionaryAbortDefinition(dictionary); }
FICL_PLATFORM_EXTERN void        dictAlign      (ficlDictionary *dictionary) { ficlDictionaryAlign(dictionary); }
FICL_PLATFORM_EXTERN int         dictAllot      (ficlDictionary *dictionary, int n) { ficlDictionaryAllot(dictionary, n); return 0; }
FICL_PLATFORM_EXTERN int         dictAllotCells (ficlDictionary *dictionary, int cells) { ficlDictionaryAllotCells(dictionary, cells); return 0; }
FICL_PLATFORM_EXTERN void        dictAppendCell (ficlDictionary *dictionary, ficlCell cell) { ficlDictionaryAppendCell(dictionary, cell); }
FICL_PLATFORM_EXTERN void        dictAppendChar (ficlDictionary *dictionary, char c) { ficlDictionaryAppendCharacter(dictionary, c); }
FICL_PLATFORM_EXTERN ficlWord   *dictAppendWord (ficlDictionary *dictionary, 
                           char *name,
                           ficlPrimitive code,
                           ficlUnsigned8 flags)
							{ return ficlDictionaryAppendPrimitive(dictionary, name, code, flags); }
FICL_PLATFORM_EXTERN ficlWord   *dictAppendWord2(ficlDictionary *dictionary, 
                           ficlString name,
                           ficlPrimitive code,
                           ficlUnsigned8 flags)
						   { return ficlDictionaryAppendWord(dictionary, name, code, flags); }
FICL_PLATFORM_EXTERN void        dictAppendUNS  (ficlDictionary *dictionary, ficlUnsigned u) { ficlDictionaryAppendUnsigned(dictionary, u); }
FICL_PLATFORM_EXTERN int         dictCellsAvail (ficlDictionary *dictionary) { return ficlDictionaryCellsAvailable(dictionary); }
FICL_PLATFORM_EXTERN int         dictCellsUsed  (ficlDictionary *dictionary) { return ficlDictionaryCellsUsed(dictionary); }
FICL_PLATFORM_EXTERN void        dictCheck      (ficlDictionary *dictionary, ficlVm *vm, int n) { FICL_IGNORE(dictionary); FICL_IGNORE(vm); FICL_IGNORE(n); FICL_VM_DICTIONARY_CHECK(vm, dictionary, n); }
FICL_PLATFORM_EXTERN ficlDictionary  *dictCreate(unsigned cells) { return ficlDictionaryCreate(NULL, cells); }
FICL_PLATFORM_EXTERN ficlDictionary  *dictCreateHashed(unsigned cells, unsigned hash) { return ficlDictionaryCreateHashed(NULL, cells, hash); }
FICL_PLATFORM_EXTERN ficlHash  *dictCreateWordlist(ficlDictionary *dictionary, int nBuckets) { return ficlDictionaryCreateWordlist(dictionary, nBuckets); }
FICL_PLATFORM_EXTERN void        dictDelete     (ficlDictionary *dictionary) { ficlDictionaryDestroy(dictionary); }
FICL_PLATFORM_EXTERN void        dictEmpty      (ficlDictionary *dictionary, unsigned nHash) { ficlDictionaryEmpty(dictionary, nHash); }
#if FICL_WANT_FLOAT
FICL_PLATFORM_EXTERN  void ficlPrimitiveHashSummary(ficlVm *vm);
FICL_PLATFORM_EXTERN void        dictHashSummary(ficlVm *vm) { ficlPrimitiveHashSummary(vm); }
#endif
FICL_PLATFORM_EXTERN int         dictIncludes   (ficlDictionary *dictionary, void *p) { return ficlDictionaryIncludes(dictionary, p); }
FICL_PLATFORM_EXTERN ficlWord  *dictLookup     (ficlDictionary *dictionary, ficlString name) { return ficlDictionaryLookup(dictionary, name); }
#if FICL_WANT_LOCALS
FICL_PLATFORM_EXTERN ficlWord  *ficlLookupLoc  (ficlSystem *system, ficlString name) { return ficlDictionaryLookup(ficlSystemGetLocals(system), name); }
#endif
FICL_PLATFORM_EXTERN void        dictResetSearchOrder(ficlDictionary *dictionary) { ficlDictionaryResetSearchOrder(dictionary); }
FICL_PLATFORM_EXTERN void        dictSetFlags   (ficlDictionary *dictionary, ficlUnsigned8 set, ficlUnsigned8 clear) { ficlDictionarySetFlags(dictionary, set); ficlDictionaryClearFlags(dictionary, clear); }
FICL_PLATFORM_EXTERN void        dictSetImmediate(ficlDictionary *dictionary) { ficlDictionarySetImmediate(dictionary); }
FICL_PLATFORM_EXTERN void        dictUnsmudge   (ficlDictionary *dictionary) { ficlDictionaryUnsmudge(dictionary); }
FICL_PLATFORM_EXTERN ficlCell   *dictWhere      (ficlDictionary *dictionary) { return ficlDictionaryWhere(dictionary); }

FICL_PLATFORM_EXTERN int  ficlAddParseStep(ficlSystem *system, ficlWord *word) { return ficlSystemAddParseStep(system, word); }
FICL_PLATFORM_EXTERN void ficlAddPrecompiledParseStep(ficlSystem *system, char *name, ficlParseStep pStep) { ficlSystemAddPrimitiveParseStep(system, name, pStep); }
FICL_PLATFORM_EXTERN void ficlPrimitiveParseStepList(ficlVm *vm);
FICL_PLATFORM_EXTERN void ficlListParseSteps(ficlVm *vm) { ficlPrimitiveParseStepList(vm); }

FICL_PLATFORM_EXTERN void       ficlTermSystem(ficlSystem *system) { ficlSystemDestroy(system); }
FICL_PLATFORM_EXTERN int        ficlEvaluate(ficlVm *vm, char *pText) { return ficlVmEvaluate(vm, pText); }
FICL_PLATFORM_EXTERN int        ficlExec (ficlVm *vm, char *pText) { ficlString s; FICL_STRING_SET_FROM_CSTRING(s, pText); return ficlVmExecuteString(vm, s); }
FICL_PLATFORM_EXTERN int        ficlExecC(ficlVm *vm, char *pText, ficlInteger nChars) { ficlString s; FICL_STRING_SET_POINTER(s, pText); FICL_STRING_SET_LENGTH(s, nChars); return ficlVmExecuteString(vm, s); }
FICL_PLATFORM_EXTERN int        ficlExecXT(ficlVm *vm, ficlWord *word) { return ficlVmExecuteXT(vm, word); }
FICL_PLATFORM_EXTERN void ficlFreeVM(ficlVm *vm) { ficlVmDestroy(vm); }





static void thunkTextOut(ficlCallback *callback, char *text)
	{
	ficlCompatibilityOutputFunction outputFunction;
	if ((callback->vm != NULL) && (callback->vm->thunkedTextout != NULL))
		outputFunction = callback->system->thunkedTextout;
	else if (callback->system->thunkedTextout != NULL)
		outputFunction = callback->system->thunkedTextout;
	else
		{
		ficlCallbackDefaultTextOut(callback, text);
		return;
		}
	ficlCompatibilityTextOutCallback(callback, text, outputFunction);
	}


FICL_PLATFORM_EXTERN void    vmSetTextOut(ficlVm *vm, ficlCompatibilityOutputFunction textOut)
	{
	vm->thunkedTextout = textOut;
	ficlVmSetTextOut(vm, thunkTextOut);
	}

FICL_PLATFORM_EXTERN void        vmTextOut      (ficlVm *vm, char *text, int fNewline)
	{
	ficlVmTextOut(vm, text);
	if (fNewline)
		ficlVmTextOut(vm, "\n");
	}


FICL_PLATFORM_EXTERN void        ficlTextOut      (ficlVm *vm, char *text, int fNewline)
	{
	vmTextOut(vm, text, fNewline);
	}

extern ficlSystem *ficlSystemGlobal;
static int defaultStackSize = FICL_DEFAULT_STACK_SIZE;
FICL_PLATFORM_EXTERN int ficlSetStackSize(int nStackCells)
{
	if (defaultStackSize < nStackCells)
		defaultStackSize = nStackCells;
	if ((ficlSystemGlobal != NULL) && (ficlSystemGlobal->stackSize < nStackCells))
		ficlSystemGlobal->stackSize = nStackCells;
	return defaultStackSize;
}


FICL_PLATFORM_EXTERN ficlSystem *ficlInitSystemEx(ficlSystemInformation *fsi)
{
	ficlSystem *returnValue;
	ficlCompatibilityOutputFunction thunkedTextout;
	ficlSystemInformation clone;

	memcpy(&clone, fsi, sizeof(clone));
	thunkedTextout = (ficlCompatibilityOutputFunction)clone.textOut;
	clone.textOut = clone.errorOut = thunkTextOut;

	returnValue = ficlSystemCreate(&clone);
	if (returnValue != NULL)
	{
		returnValue->thunkedTextout = thunkedTextout;
	}
	return returnValue;
}


FICL_PLATFORM_EXTERN ficlSystem *ficlInitSystem(int nDictCells)
{
	ficlSystemInformation fsi;
	ficlSystemInformationInitialize(&fsi);
	fsi.dictionarySize = nDictCells;
	if (fsi.stackSize < defaultStackSize)
		fsi.stackSize = defaultStackSize;
	return ficlSystemCreate(&fsi);
}




FICL_PLATFORM_EXTERN ficlVm    *ficlNewVM(ficlSystem *system)
{
	ficlVm *returnValue = ficlSystemCreateVm(system);
	if (returnValue != NULL)
	{
		if ((returnValue->callback.textOut != NULL) && (returnValue->callback.textOut != thunkTextOut))
		{
			returnValue->thunkedTextout = (ficlCompatibilityOutputFunction)returnValue->callback.textOut;
			returnValue->callback.textOut = thunkTextOut;
		}
		if ((returnValue->callback.errorOut != NULL) && (returnValue->callback.errorOut != thunkTextOut))
		{
			if (returnValue->thunkedTextout == NULL)
				returnValue->thunkedTextout = (ficlCompatibilityOutputFunction)returnValue->callback.errorOut;
			returnValue->callback.errorOut = thunkTextOut;
		}
	}
	return returnValue;
}



FICL_PLATFORM_EXTERN ficlWord  *ficlLookup(ficlSystem *system, char *name) { return ficlSystemLookup(system, name); }
FICL_PLATFORM_EXTERN ficlDictionary *ficlGetDict(ficlSystem *system) { return ficlSystemGetDictionary(system); }
FICL_PLATFORM_EXTERN ficlDictionary *ficlGetEnv (ficlSystem *system) { return ficlSystemGetEnvironment(system); }
FICL_PLATFORM_EXTERN void       ficlSetEnv (ficlSystem *system, char *name, ficlInteger value) { ficlDictionarySetConstant(ficlSystemGetDictionary(system), name, value); }
FICL_PLATFORM_EXTERN void       ficlSetEnvD(ficlSystem *system, char *name, ficlInteger high, ficlInteger low) { ficl2Unsigned value; FICL_2UNSIGNED_SET(low, high, value);  ficlDictionarySet2Constant(ficlSystemGetDictionary(system), name, FICL_2UNSIGNED_TO_2INTEGER(value)); }
#if FICL_WANT_LOCALS
FICL_PLATFORM_EXTERN ficlDictionary *ficlGetLoc (ficlSystem *system) { return ficlSystemGetLocals(system); }
#endif
FICL_PLATFORM_EXTERN int        ficlBuild(ficlSystem *system, char *name, ficlPrimitive code, char flags) { ficlDictionary *dictionary = ficlSystemGetDictionary(system); ficlDictionaryLock(dictionary, FICL_TRUE); ficlDictionaryAppendPrimitive(dictionary, name, code, flags); ficlDictionaryLock(dictionary, FICL_FALSE); return 0; }
FICL_PLATFORM_EXTERN void       ficlCompileCore(ficlSystem *system) { ficlSystemCompileCore(system); }
FICL_PLATFORM_EXTERN void       ficlCompilePrefix(ficlSystem *system) { ficlSystemCompilePrefix(system); }
FICL_PLATFORM_EXTERN void       ficlCompileSearch(ficlSystem *system) { ficlSystemCompileSearch(system); }
FICL_PLATFORM_EXTERN void       ficlCompileSoftCore(ficlSystem *system) { ficlSystemCompileSoftCore(system); }
FICL_PLATFORM_EXTERN void       ficlCompileTools(ficlSystem *system) { ficlSystemCompileTools(system); }
FICL_PLATFORM_EXTERN void       ficlCompileFile(ficlSystem *system) { ficlSystemCompileFile(system); }
#if FICL_WANT_FLOAT
FICL_PLATFORM_EXTERN void       ficlCompileFloat(ficlSystem *system) { ficlSystemCompileFloat(system); }
FICL_PLATFORM_EXTERN int        ficlParseFloatNumber( ficlVm *vm, ficlString string) { return ficlVmParseFloatNumber(vm, string); }
#endif
#if FICL_WANT_PLATFORM
FICL_PLATFORM_EXTERN void       ficlCompilePlatform(ficlSystem *system) { ficlSystemCompilePlatform(system); }
#endif
FICL_PLATFORM_EXTERN int        ficlParsePrefix(ficlVm *vm, ficlString string) { return ficlVmParsePrefix(vm, string); }

FICL_PLATFORM_EXTERN int        ficlParseNumber(ficlVm *vm, ficlString string) { return ficlVmParseNumber(vm, string); }
FICL_PLATFORM_EXTERN void       ficlTick(ficlVm *vm) { ficlPrimitiveTick(vm); }
FICL_PLATFORM_EXTERN void       parseStepParen(ficlVm *vm) { ficlPrimitiveParseStepParen(vm); }

FICL_PLATFORM_EXTERN int        isAFiclWord(ficlDictionary *dictionary, ficlWord *word) { return ficlDictionaryIsAWord(dictionary, word); }


FICL_PLATFORM_EXTERN void buildTestInterface(ficlSystem *system) { ficlSystemCompileExtras(system); }


