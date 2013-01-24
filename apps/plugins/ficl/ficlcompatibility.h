#ifndef FICL_FORCE_COMPATIBILITY

struct ficl_word;
typedef struct ficl_word FICL_WORD;
struct vm;
typedef struct vm FICL_VM;
struct ficl_dict;
typedef struct ficl_dict FICL_DICT;
struct ficl_system;
typedef struct ficl_system FICL_SYSTEM;
struct ficl_system_info;
typedef struct ficl_system_info FICL_SYSTEM_INFO;
#define ficlFILE ficlFile

typedef ficlUnsigned FICL_UNS;
typedef ficlInteger FICL_INT;
typedef ficlFloat FICL_FLOAT;
typedef ficlUnsigned16 UNS16;
typedef ficlUnsigned8 UNS8;

#define _cell ficlCell
#define CELL ficlCell

#define LVALUEtoCELL(v) (*(ficlCell *)&v)
#define PTRtoCELL (ficlCell *)(void *)
#define PTRtoSTRING (ficlCountedString *)(void *)

typedef unsigned char FICL_COUNT;
#define FICL_STRING_MAX UCHAR_MAX
typedef struct _ficl_string
{
    ficlUnsigned8 count;
    char text[1];
} FICL_STRING;

typedef struct 
{
    ficlUnsigned count;
    char *cp;
} STRINGINFO;

#define SI_COUNT(si) (si.count)
#define SI_PTR(si)   (si.cp)
#define SI_SETLEN(si, len) (si.count = (FICL_UNS)(len))
#define SI_SETPTR(si, ptr) (si.cp = (char *)(ptr))
#define SI_PSZ(si, psz) \
            {si.cp = psz; si.count = (FICL_COUNT)strlen(psz);}
#define SI_PFS(si, pfs) \
            {si.cp = pfs->text; si.count = pfs->count;}

typedef struct
{
    ficlInteger index;
    char *end;
    char *cp;
} TIB;


typedef struct _ficlStack
{
    ficlUnsigned nCells;    /* size of the stack */
    CELL *pFrame;       /* link reg for stack frame */
    CELL *sp;           /* stack pointer */
	ficlVm *vm;
	char *name;
    CELL base[1];       /* Top of stack */
} FICL_STACK;

FICL_STACK *stackCreate   (unsigned nCells);
void        stackDelete   (FICL_STACK *pStack);
int         stackDepth    (FICL_STACK *pStack);
void        stackDrop     (FICL_STACK *pStack, int n);
CELL        stackFetch    (FICL_STACK *pStack, int n);
CELL        stackGetTop   (FICL_STACK *pStack);
void        stackLink     (FICL_STACK *pStack, int nCells);
void        stackPick     (FICL_STACK *pStack, int n);
CELL        stackPop      (FICL_STACK *pStack);
void       *stackPopPtr   (FICL_STACK *pStack);
FICL_UNS    stackPopUNS   (FICL_STACK *pStack);
FICL_INT    stackPopINT   (FICL_STACK *pStack);
void        stackPush     (FICL_STACK *pStack, CELL c);
void        stackPushPtr  (FICL_STACK *pStack, void *ptr);
void        stackPushUNS  (FICL_STACK *pStack, FICL_UNS u);
void        stackPushINT  (FICL_STACK *pStack, FICL_INT i);
void        stackReset    (FICL_STACK *pStack);
void        stackRoll     (FICL_STACK *pStack, int n);
void        stackSetTop   (FICL_STACK *pStack, CELL c);
void        stackStore    (FICL_STACK *pStack, int n, CELL c);
void        stackUnlink   (FICL_STACK *pStack);

#if (FICL_WANT_FLOAT)
float       stackPopFloat (FICL_STACK *pStack);
void        stackPushFloat(FICL_STACK *pStack, FICL_FLOAT f);
#endif

#define PUSHPTR(p)   stackPushPtr(pVM->pStack,p)
#define PUSHUNS(u)   stackPushUNS(pVM->pStack,u)
#define PUSHINT(i)   stackPushINT(pVM->pStack,i)
#define PUSHFLOAT(f) stackPushFloat(pVM->fStack,f)
#define PUSH(c)      stackPush(pVM->pStack,c)
#define POPPTR()     stackPopPtr(pVM->pStack)
#define POPUNS()     stackPopUNS(pVM->pStack)
#define POPINT()     stackPopINT(pVM->pStack)
#define POPFLOAT()   stackPopFloat(pVM->fStack)
#define POP()        stackPop(pVM->pStack)
#define GETTOP()     stackGetTop(pVM->pStack)
#define SETTOP(c)    stackSetTop(pVM->pStack,LVALUEtoCELL(c))
#define GETTOPF()    stackGetTop(pVM->fStack)
#define SETTOPF(c)   stackSetTop(pVM->fStack,LVALUEtoCELL(c))
#define STORE(n,c)   stackStore(pVM->pStack,n,LVALUEtoCELL(c))
#define DEPTH()      stackDepth(pVM->pStack)
#define DROP(n)      stackDrop(pVM->pStack,n)
#define DROPF(n)     stackDrop(pVM->fStack,n)
#define FETCH(n)     stackFetch(pVM->pStack,n)
#define PICK(n)      stackPick(pVM->pStack,n)
#define PICKF(n)     stackPick(pVM->fStack,n)
#define ROLL(n)      stackRoll(pVM->pStack,n)
#define ROLLF(n)     stackRoll(pVM->fStack,n)

typedef FICL_WORD ** IPTYPE; /* the VM's instruction pointer */
typedef void (*OUTFUNC)(FICL_VM *pVM, char *text, int fNewline);

/* values of STATE */
#define INTERPRET FICL_STATE_INTERPRET
#define COMPILE   FICL_STATE_COMPILE

#if !defined nPAD
#define nPAD FICL_PAD_SIZE
#endif

#if !defined nFICLNAME
#define nFICLNAME       FICL_NAME_LENGTH
#endif

#define FICL_DEFAULT_STACK FICL_DEFAULT_STACK_SIZE
#define FICL_DEFAULT_DICT FICL_DEFAULT_DICTIONARY_SIZE
#define FICL_DEFAULT_ENV FICL_DEFAULT_ENVIRONMENT_SIZE
#define FICL_DEFAULT_VOCS FICL_MAX_WORDLISTS





struct vm
{
	void *pExtend;
	ficlOutputFunction textOut;
	ficlOutputFunction errorOut;
	ficlSystem *pSys;
	ficlVm *pVM;
    FICL_VM        *link;       /* Ficl keeps a VM list for simple teardown */
    jmp_buf        *pState;     /* crude exception mechanism...     */
    short           fRestart;   /* Set TRUE to restart runningWord  */
    IPTYPE          ip;         /* instruction pointer              */
    FICL_WORD      *runningWord;/* address of currently running word (often just *(ip-1) ) */
    FICL_UNS        state;      /* compiling or interpreting        */
    FICL_UNS        base;       /* number conversion base           */
    FICL_STACK     *pStack;     /* param stack                      */
    FICL_STACK     *rStack;     /* return stack                     */
#if FICL_WANT_FLOAT
    FICL_STACK     *fStack;     /* float stack (optional)           */
#endif
    CELL            sourceID;   /* -1 if EVALUATE, 0 if normal input */
    TIB             tib;        /* address of incoming text string  */
#if FICL_WANT_USER
    CELL            user[FICL_USER_CELLS];
#endif
    char            pad[nPAD];  /* the scratch area (see above)     */
};

/*
** A FICL_CODE points to a function that gets called to help execute
** a word in the dictionary. It always gets passed a pointer to the
** running virtual machine, and from there it can get the address
** of the parameter area of the word it's supposed to operate on.
** For precompiled words, the code is all there is. For user defined
** words, the code assumes that the word's parameter area is a list
** of pointers to the code fields of other words to execute, and
** may also contain inline data. The first parameter is always
** a pointer to a code field.
*/
typedef void (*FICL_CODE)(FICL_VM *pVm);

#if 0
#define VM_ASSERT(pVM) assert((*(pVM->ip - 1)) == pVM->runningWord)
#else
#define VM_ASSERT(pVM) 
#endif

#define nName length
#define ficl_word ficlWord
#define FICL_WORD ficlWord

#define CELLS_PER_WORD  \
    ( (sizeof (FICL_WORD) + nFICLNAME + sizeof (CELL)) \
                          / (sizeof (CELL)) )

int wordIsImmediate(FICL_WORD *pFW);
int wordIsCompileOnly(FICL_WORD *pFW);

#define FW_IMMEDIATE    FICL_WORD_IMMEDIATE
#define FW_COMPILE      FICL_WORD_COMPILE_ONLY
#define FW_SMUDGE       FICL_WORD_SMUDGED
#define FW_ISOBJECT     FICL_WORD_OBJECT

#define FW_COMPIMMED    (FW_IMMEDIATE | FW_COMPILE_ONLY)
#define FW_DEFAULT      0


/*
** Exit codes for vmThrow
*/
#define VM_INNEREXIT FICL_VM_STATUS_INNER_EXIT
#define VM_OUTOFTEXT FICL_VM_STATUS_OUT_OF_TEXT
#define VM_RESTART   FICL_VM_STATUS_RESTART
#define VM_USEREXIT  FICL_VM_STATUS_USER_EXIT
#define VM_ERREXIT   FICL_VM_STATUS_ERROR_EXIT
#define VM_BREAK     FICL_VM_STATUS_BREAK
#define VM_ABORT     FICL_VM_STATUS_ABORT
#define VM_ABORTQ    FICL_VM_STATUS_ABORTQ
#define VM_QUIT      FICL_VM_STATUS_QUIT


void        vmBranchRelative(FICL_VM *pVM, int offset);
FICL_VM *   vmCreate       (FICL_VM *pVM, unsigned nPStack, unsigned nRStack);
void        vmDelete       (FICL_VM *pVM);
void        vmExecute      (FICL_VM *pVM, FICL_WORD *pWord);
FICL_DICT  *vmGetDict      (FICL_VM *pVM);
char *      vmGetString    (FICL_VM *pVM, FICL_STRING *spDest, char delimiter);
STRINGINFO  vmGetWord      (FICL_VM *pVM);
STRINGINFO  vmGetWord0     (FICL_VM *pVM);
int         vmGetWordToPad (FICL_VM *pVM);
STRINGINFO  vmParseString  (FICL_VM *pVM, char delimiter);
STRINGINFO  vmParseStringEx(FICL_VM *pVM, char delimiter, char fSkipLeading);
CELL        vmPop          (FICL_VM *pVM);
void        vmPush         (FICL_VM *pVM, CELL c);
void        vmPopIP        (FICL_VM *pVM);
void        vmPushIP       (FICL_VM *pVM, IPTYPE newIP);
void        vmQuit         (FICL_VM *pVM);
void        vmReset        (FICL_VM *pVM);
void        vmSetTextOut   (FICL_VM *pVM, OUTFUNC textOut);
void        vmTextOut      (FICL_VM *pVM, char *text, int fNewline);
void        vmThrow        (FICL_VM *pVM, int except);
void        vmThrowErr     (FICL_VM *pVM, char *fmt, ...);

#define vmGetRunningWord(pVM) ((pVM)->runningWord)


#define M_VM_STEP(pVM) \
        FICL_WORD *tempFW = *(pVM)->ip++; \
        ficlVmInnerLoop((ficlVm *)pVM, (ficlWord *)tempFW); \

#define M_INNER_LOOP(pVM) \
    ficlVmInnerLoop((ficlVm *)pVm);


void        vmCheckStack(FICL_VM *pVM, int popCells, int pushCells);
#if FICL_WANT_FLOAT
void        vmCheckFStack(FICL_VM *pVM, int popCells, int pushCells);
#endif

void        vmPushTib  (FICL_VM *pVM, char *text, FICL_INT nChars, TIB *pSaveTib);
void        vmPopTib   (FICL_VM *pVM, TIB *pTib);
#define     vmGetInBuf(pVM)      ((pVM)->tib.cp + (pVM)->tib.index)
#define     vmGetInBufLen(pVM)   ((pVM)->tib.end - (pVM)->tib.cp)
#define     vmGetInBufEnd(pVM)   ((pVM)->tib.end)
#define     vmGetTibIndex(pVM)    (pVM)->tib.index
#define     vmSetTibIndex(pVM, i) (pVM)->tib.index = i
#define     vmUpdateTib(pVM, str) (pVM)->tib.index = (str) - (pVM)->tib.cp

#if defined(_WIN32)
/* #SHEESH
** Why do Microsoft Meatballs insist on contaminating
** my namespace with their string functions???
*/
#pragma warning(disable: 4273)
#endif

int        isPowerOfTwo(FICL_UNS u);

char       *ltoa( FICL_INT value, char *string, int radix );
char       *ultoa(FICL_UNS value, char *string, int radix );
char        digit_to_char(int value);
char       *strrev( char *string );
char       *skipSpace(char *cp, char *end);
char       *caseFold(char *cp);
int         strincmp(char *cp1, char *cp2, FICL_UNS count);

#if defined(_WIN32)
#pragma warning(default: 4273)
#endif

#if !defined HASHSIZE /* Default size of hash table. For most uniform */
#define HASHSIZE FICL_HASHSIZE  /*   performance, use a prime number!   */
#endif

#define ficl_hash ficlHash
#define FICL_HASH ficlHash

void        hashForget    (FICL_HASH *pHash, void *where);
UNS16       hashHashCode  (STRINGINFO si);
void        hashInsertWord(FICL_HASH *pHash, FICL_WORD *pFW);
FICL_WORD  *hashLookup    (FICL_HASH *pHash, STRINGINFO si, UNS16 hashCode);
void        hashReset     (FICL_HASH *pHash);

struct ficl_dict
{
    CELL *here;
	void *context;
    FICL_WORD *smudge;
    FICL_HASH *pForthWords;
    FICL_HASH *pCompile;
    FICL_HASH *pSearch[FICL_DEFAULT_VOCS];
    int        nLists;
    unsigned   size;    /* Number of cells in dict (total)*/
	ficlSystem *system;
    CELL       dict[1]; /* Base of dictionary memory      */
};

void       *alignPtr(void *ptr);
void        dictAbortDefinition(FICL_DICT *pDict);
void        dictAlign      (FICL_DICT *pDict);
int         dictAllot      (FICL_DICT *pDict, int n);
int         dictAllotCells (FICL_DICT *pDict, int nCells);
void        dictAppendCell (FICL_DICT *pDict, CELL c);
void        dictAppendChar (FICL_DICT *pDict, char c);
FICL_WORD  *dictAppendWord (FICL_DICT *pDict, 
                           char *name, 
                           FICL_CODE pCode, 
                           UNS8 flags);
FICL_WORD  *dictAppendWord2(FICL_DICT *pDict, 
                           STRINGINFO si, 
                           FICL_CODE pCode, 
                           UNS8 flags);
void        dictAppendUNS  (FICL_DICT *pDict, FICL_UNS u);
int         dictCellsAvail (FICL_DICT *pDict);
int         dictCellsUsed  (FICL_DICT *pDict);
void        dictCheck      (FICL_DICT *pDict, FICL_VM *pVM, int n);
FICL_DICT  *dictCreate(unsigned nCELLS);
FICL_DICT  *dictCreateHashed(unsigned nCells, unsigned nHash);
FICL_HASH  *dictCreateWordlist(FICL_DICT *dp, int nBuckets);
void        dictDelete     (FICL_DICT *pDict);
void        dictEmpty      (FICL_DICT *pDict, unsigned nHash);
#if FICL_WANT_FLOAT
void        dictHashSummary(FICL_VM *pVM);
#endif
int         dictIncludes   (FICL_DICT *pDict, void *p);
FICL_WORD  *dictLookup     (FICL_DICT *pDict, STRINGINFO si);
#if FICL_WANT_LOCALS
FICL_WORD  *ficlLookupLoc  (FICL_SYSTEM *pSys, STRINGINFO si);
#endif
void        dictResetSearchOrder(FICL_DICT *pDict);
void        dictSetFlags   (FICL_DICT *pDict, UNS8 set, UNS8 clr);
void        dictSetImmediate(FICL_DICT *pDict);
void        dictUnsmudge   (FICL_DICT *pDict);
CELL       *dictWhere      (FICL_DICT *pDict);

typedef int (*FICL_PARSE_STEP)(FICL_VM *pVM, STRINGINFO si);

int  ficlAddParseStep(FICL_SYSTEM *pSys, FICL_WORD *pFW); /* ficl.c */
void ficlAddPrecompiledParseStep(FICL_SYSTEM *pSys, char *name, FICL_PARSE_STEP pStep);
void ficlListParseSteps(FICL_VM *pVM);

typedef struct FICL_BREAKPOINT
{
    void      *address;
    FICL_WORD *origXT;
} FICL_BREAKPOINT;


struct ficl_system 
{
	void *pExtend;
	ficlOutputFunction textOut;
	ficlOutputFunction errorTextOut;
	ficlSystem *pSys;
	ficlVm *vm;
    FICL_SYSTEM *link;
    FICL_VM *vmList;
    FICL_DICT *dp;
    FICL_DICT *envp;
    FICL_WORD *pInterp[3];
    FICL_WORD *parseList[FICL_MAX_PARSE_STEPS];

	FICL_WORD *pExitInner;
	FICL_WORD *pInterpret;

#if FICL_WANT_LOCALS
    FICL_DICT *localp;
	FICL_INT   nLocals;
	CELL *pMarkLocals;
#endif

	ficlInteger stackSize;

	FICL_BREAKPOINT bpStep;
};

struct ficl_system_info
{
	int size;           /* structure size tag for versioning */
	void *pExtend;      /* Initializes VM's pExtend pointer - for application use */
	int nDictCells;     /* Size of system's Dictionary */
	int stackSize;     /* Size of system's Dictionary */
	OUTFUNC textOut;    /* default textOut function */
    int nEnvCells;      /* Size of Environment dictionary */
};


#define ficlInitInfo(x) { memset((x), 0, sizeof(FICL_SYSTEM_INFO)); \
         (x)->size = sizeof(FICL_SYSTEM_INFO); }

FICL_SYSTEM *ficlInitSystemEx(FICL_SYSTEM_INFO *fsi);
FICL_SYSTEM *ficlInitSystem(int nDictCells);
void       ficlTermSystem(FICL_SYSTEM *pSys);
int        ficlEvaluate(FICL_VM *pVM, char *pText);
int        ficlExec (FICL_VM *pVM, char *pText);
int        ficlExecC(FICL_VM *pVM, char *pText, FICL_INT nChars);
int        ficlExecXT(FICL_VM *pVM, FICL_WORD *pWord);
FICL_VM   *ficlNewVM(FICL_SYSTEM *pSys);
void ficlFreeVM(FICL_VM *pVM);
int ficlSetStackSize(int nStackCells);
FICL_WORD *ficlLookup(FICL_SYSTEM *pSys, char *name);
FICL_DICT *ficlGetDict(FICL_SYSTEM *pSys);
FICL_DICT *ficlGetEnv (FICL_SYSTEM *pSys);
void       ficlSetEnv (FICL_SYSTEM *pSys, char *name, FICL_UNS value);
void       ficlSetEnvD(FICL_SYSTEM *pSys, char *name, FICL_UNS hi, FICL_UNS lo);
#if FICL_WANT_LOCALS
FICL_DICT *ficlGetLoc (FICL_SYSTEM *pSys);
#endif
int        ficlBuild(FICL_SYSTEM *pSys, char *name, FICL_CODE code, char flags);
void       ficlCompileCore(FICL_SYSTEM *pSys);
void       ficlCompilePrefix(FICL_SYSTEM *pSys);
void       ficlCompileSearch(FICL_SYSTEM *pSys);
void       ficlCompileSoftCore(FICL_SYSTEM *pSys);
void       ficlCompileTools(FICL_SYSTEM *pSys);
void       ficlCompileFile(FICL_SYSTEM *pSys);
#if FICL_WANT_FLOAT
void       ficlCompileFloat(FICL_SYSTEM *pSys);
int        ficlParseFloatNumber( FICL_VM *pVM, STRINGINFO si ); /* float.c */
#endif
#if FICL_WANT_PLATFORM
void       ficlCompilePlatform(FICL_SYSTEM *pSys);
#endif
int        ficlParsePrefix(FICL_VM *pVM, STRINGINFO si);

void       constantParen(FICL_VM *pVM);
void       twoConstParen(FICL_VM *pVM);
int        ficlParseNumber(FICL_VM *pVM, STRINGINFO si);
void       ficlTick(FICL_VM *pVM);
void       parseStepParen(FICL_VM *pVM);

int        isAFiclWord(FICL_DICT *pd, FICL_WORD *pFW);



/* we define it ourselves, for naughty programs that call it directly. */
void        ficlTextOut      (FICL_VM *pVM, char *text, int fNewline);
/* but you can use this one! */
void        ficlTextOutLocal (FICL_VM *pVM, char *text, int fNewline);


#endif /* FICL_FORCE_COMPATIBILITY */
