/*******************************************************************
** w o r d s . c
** Forth Inspired Command Language
** ANS Forth CORE word-set written in C
** Author: John Sadler (john_sadler@alum.mit.edu)
** Created: 19 July 1997
** $Id: primitives.c,v 1.4 2010/09/13 18:43:04 asau Exp $
*******************************************************************/
/*
** Copyright (c) 1997-2001 John Sadler (john_sadler@alum.mit.edu)
** All rights reserved.
**
** Get the latest Ficl release at http://ficl.sourceforge.net
**
** I am interested in hearing from anyone who uses Ficl. If you have
** a problem, a success story, a defect, an enhancement request, or
** if you would like to contribute to the Ficl release, please
** contact me by email at the address above.
**
** L I C E N S E  and  D I S C L A I M E R
** 
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
** OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
** HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
** LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
** OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
** SUCH DAMAGE.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "ficl.h"


/*
** Control structure building words use these
** strings' addresses as markers on the stack to 
** check for structure completion.
*/
static char doTag[]    = "do";
static char colonTag[] = "colon";
static char leaveTag[] = "leave";

static char destTag[]  = "target";
static char origTag[]  = "origin";

static char caseTag[]  = "case";
static char ofTag[]  = "of";
static char fallthroughTag[]  = "fallthrough";

/*
** C O N T R O L   S T R U C T U R E   B U I L D E R S
**
** Push current dictionary location for later branch resolution.
** The location may be either a branch target or a patch address...
*/
static void markBranch(ficlDictionary *dictionary, ficlVm *vm, char *tag)
{
    ficlStackPushPointer(vm->dataStack, dictionary->here);
    ficlStackPushPointer(vm->dataStack, tag);
    return;
}

static void markControlTag(ficlVm *vm, char *tag)
{
    ficlStackPushPointer(vm->dataStack, tag);
    return;
}

static void matchControlTag(ficlVm *vm, char *wantTag)
{
    char *tag;

    FICL_STACK_CHECK(vm->dataStack, 1, 0);

    tag = (char *)ficlStackPopPointer(vm->dataStack);
    /*
    ** Changed the code below to compare the pointers first (by popular demand)
    */
    if ( (tag != wantTag) && strcmp(tag, wantTag) )
    {
        ficlVmThrowError(vm, "Error -- unmatched control structure \"%s\"", wantTag);
    }

    return;
}

/*
** Expect a branch target address on the param stack,
** FICL_VM_STATE_COMPILE a literal offset from the current dictionary location
** to the target address
*/
static void resolveBackBranch(ficlDictionary *dictionary, ficlVm *vm, char *tag)
{
    ficlInteger offset;
    ficlCell *patchAddr;

    matchControlTag(vm, tag);

    FICL_STACK_CHECK(vm->dataStack, 1, 0);

    patchAddr = (ficlCell *)ficlStackPopPointer(vm->dataStack);
    offset = patchAddr - dictionary->here;
    ficlDictionaryAppendCell(dictionary, FICL_LVALUE_TO_CELL(offset));

    return;
}


/*
** Expect a branch patch address on the param stack,
** FICL_VM_STATE_COMPILE a literal offset from the patch location
** to the current dictionary location
*/
static void resolveForwardBranch(ficlDictionary *dictionary, ficlVm *vm, char *tag)
{
    ficlInteger offset;
    ficlCell *patchAddr;

    matchControlTag(vm, tag);

    FICL_STACK_CHECK(vm->dataStack, 1, 0);

    patchAddr = (ficlCell *)ficlStackPopPointer(vm->dataStack);
    offset = dictionary->here - patchAddr;
    *patchAddr = FICL_LVALUE_TO_CELL(offset);

    return;
}

/*
** Match the tag to the top of the stack. If success,
** sopy "here" address into the ficlCell whose address is next
** on the stack. Used by do..leave..loop.
*/
static void resolveAbsBranch(ficlDictionary *dictionary, ficlVm *vm, char *wantTag)
{
    ficlCell *patchAddr;
    char *tag;

    FICL_STACK_CHECK(vm->dataStack, 2, 0);

    tag = ficlStackPopPointer(vm->dataStack);
    /*
    ** Changed the comparison below to compare the pointers first (by popular demand)
    */
    if ((tag != wantTag) && strcmp(tag, wantTag))
    {
        ficlVmTextOut(vm, "Warning -- Unmatched control word: ");
        ficlVmTextOut(vm, wantTag);
        ficlVmTextOut(vm, "\n");
    }

    patchAddr = (ficlCell *)ficlStackPopPointer(vm->dataStack);
    *patchAddr = FICL_LVALUE_TO_CELL(dictionary->here);

    return;
}


/**************************************************************************
                        c o l o n   d e f i n i t i o n s
** Code to begin compiling a colon definition
** This function sets the state to FICL_VM_STATE_COMPILE, then creates a
** new word whose name is the next word in the input stream
** and whose code is colonParen.
**************************************************************************/

static void ficlPrimitiveColon(ficlVm *vm)
{
    ficlDictionary *dictionary = ficlVmGetDictionary(vm);
    ficlString name = ficlVmGetWord(vm);

    vm->state = FICL_VM_STATE_COMPILE;
    markControlTag(vm, colonTag);
    ficlDictionaryAppendWord(dictionary, name, (ficlPrimitive)ficlInstructionColonParen, FICL_WORD_DEFAULT | FICL_WORD_SMUDGED);
#if FICL_WANT_LOCALS
    vm->callback.system->localsCount = 0;
#endif
    return;
}


       
static void ficlPrimitiveSemicolonCoIm(ficlVm *vm)
{
    ficlDictionary *dictionary = ficlVmGetDictionary(vm);

    matchControlTag(vm, colonTag);

#if FICL_WANT_LOCALS
    if (vm->callback.system->localsCount > 0)
    {
        ficlDictionary *locals = ficlSystemGetLocals(vm->callback.system);
        ficlDictionaryEmpty(locals, locals->forthWordlist->size);
            ficlDictionaryAppendUnsigned(dictionary, ficlInstructionUnlinkParen);
    }
    vm->callback.system->localsCount = 0;
#endif

    ficlDictionaryAppendUnsigned(dictionary, ficlInstructionSemiParen);
    vm->state = FICL_VM_STATE_INTERPRET;
    ficlDictionaryUnsmudge(dictionary);
    return;
}


/**************************************************************************
                        e x i t
** CORE
** This function simply pops the previous instruction
** pointer and returns to the "next" loop. Used for exiting from within
** a definition. Note that exitParen is identical to semiParen - they
** are in two different functions so that "see" can correctly identify
** the end of a colon definition, even if it uses "exit".
**************************************************************************/

static void ficlPrimitiveExitCoIm(ficlVm *vm)
{
    ficlDictionary *dictionary = ficlVmGetDictionary(vm);
    FICL_IGNORE(vm);

#if FICL_WANT_LOCALS
    if (vm->callback.system->localsCount > 0)
    {
		ficlDictionaryAppendUnsigned(dictionary, ficlInstructionUnlinkParen);
    }
#endif
    ficlDictionaryAppendUnsigned(dictionary, ficlInstructionExitParen);
    return;
}


/**************************************************************************
                        c o n s t a n t
** IMMEDIATE
** Compiles a constant into the dictionary. Constants return their
** value when invoked. Expects a value on top of the parm stack.
**************************************************************************/

static void ficlPrimitiveConstant(ficlVm *vm)
{
    ficlDictionary *dictionary = ficlVmGetDictionary(vm);
    ficlString name = ficlVmGetWord(vm);

    FICL_STACK_CHECK(vm->dataStack, 1, 0);

	ficlDictionaryAppendConstantInstruction(dictionary, name, ficlInstructionConstantParen, ficlStackPopInteger(vm->dataStack));
    return;
}


static void ficlPrimitive2Constant(ficlVm *vm)
{
    ficlDictionary *dictionary = ficlVmGetDictionary(vm);
    ficlString name = ficlVmGetWord(vm);
    
    FICL_STACK_CHECK(vm->dataStack, 2, 0);

	ficlDictionaryAppend2ConstantInstruction(dictionary, name, ficlInstruction2ConstantParen, ficlStackPop2Integer(vm->dataStack));
    return;
}


/**************************************************************************
                        d i s p l a y C e l l
** Drop and print the contents of the ficlCell at the top of the param
** stack
**************************************************************************/

static void ficlPrimitiveDot(ficlVm *vm)
{
    ficlCell c;

    FICL_STACK_CHECK(vm->dataStack, 1, 0);

    c = ficlStackPop(vm->dataStack);
    ficlLtoa((c).i, vm->pad, vm->base);
    strcat(vm->pad, " ");
    ficlVmTextOut(vm, vm->pad);
    return;
}

static void ficlPrimitiveUDot(ficlVm *vm)
{
    ficlUnsigned u;

    FICL_STACK_CHECK(vm->dataStack, 1, 0);

    u = ficlStackPopUnsigned(vm->dataStack);
    ficlUltoa(u, vm->pad, vm->base);
    strcat(vm->pad, " ");
    ficlVmTextOut(vm, vm->pad);
    return;
}


static void ficlPrimitiveHexDot(ficlVm *vm)
{
    ficlUnsigned u;

    FICL_STACK_CHECK(vm->dataStack, 1, 0);

    u = ficlStackPopUnsigned(vm->dataStack);
    ficlUltoa(u, vm->pad, 16);
    strcat(vm->pad, " ");
    ficlVmTextOut(vm, vm->pad);
    return;
}


/**************************************************************************
                        s t r l e n
** Ficl   ( c-string -- length )
**
** Returns the length of a C-style (zero-terminated) string.
**
** --lch
**/
static void ficlPrimitiveStrlen(ficlVm *vm)
	{
	char *address = (char *)ficlStackPopPointer(vm->dataStack);
	ficlStackPushInteger(vm->dataStack, strlen(address));
	}


/**************************************************************************
                        s p r i n t f
** Ficl   ( i*x c-addr-fmt u-fmt c-addr-buffer u-buffer -- c-addr-buffer u-written success-flag )
** Similar to the C sprintf() function.  It formats into a buffer based on
** a "format" string.  Each character in the format string is copied verbatim
** to the output buffer, until SPRINTF encounters a percent sign ("%").
** SPRINTF then skips the percent sign, and examines the next character
** (the "format character").  Here are the valid format characters:
**    s - read a C-ADDR U-LENGTH string from the stack and copy it to
**        the buffer
**    d - read a ficlCell from the stack, format it as a string (base-10,
**        signed), and copy it to the buffer
**    x - same as d, except in base-16
**    u - same as d, but unsigned
**    % - output a literal percent-sign to the buffer
** SPRINTF returns the c-addr-buffer argument unchanged, the number of bytes
** written, and a flag indicating whether or not it ran out of space while
** writing to the output buffer (FICL_TRUE if it ran out of space).
**
** If SPRINTF runs out of space in the buffer to store the formatted string,
** it still continues parsing, in an effort to preserve your stack (otherwise
** it might leave uneaten arguments behind).
**
** --lch
**************************************************************************/
static void ficlPrimitiveSprintf(ficlVm *vm) /*  */
{
	int bufferLength = ficlStackPopInteger(vm->dataStack);
	char *buffer = (char *)ficlStackPopPointer(vm->dataStack);
	char *bufferStart = buffer;

	int formatLength = ficlStackPopInteger(vm->dataStack);
	char *format = (char *)ficlStackPopPointer(vm->dataStack);
	char *formatStop = format + formatLength;

	int base = 10;
	int unsignedInteger = 0; /* false */

	int append = 1; /* true */

	while (format < formatStop)
	{
		char scratch[64];
		char *source;
		int actualLength;
		int desiredLength;
		int leadingZeroes;


		if (*format != '%')
		{
			source = format;
			actualLength = desiredLength = 1;
			leadingZeroes = 0;
		}
		else
		{
			format++;
			if (format == formatStop)
				break;

			leadingZeroes = (*format == '0');
			if (leadingZeroes)
				{
				format++;
				if (format == formatStop)
					break;
				}

			desiredLength = isdigit((unsigned char)*format);
			if (desiredLength)
				{
				desiredLength = strtoul(format, &format, 10);
				if (format == formatStop)
					break;
				}
			else if (*format == '*')
				{
				desiredLength = ficlStackPopInteger(vm->dataStack);
				format++;
				if (format == formatStop)
					break;
				}


			switch (*format)
			{
				case 's':
				case 'S':
				{
					actualLength = ficlStackPopInteger(vm->dataStack);
					source = (char *)ficlStackPopPointer(vm->dataStack);
					break;
				}
				case 'x':
				case 'X':
					base = 16;
				case 'u':
				case 'U':
					unsignedInteger = 1; /* true */
				case 'd':
				case 'D':
				{
					int integer = ficlStackPopInteger(vm->dataStack);
					if (unsignedInteger)
						ficlUltoa(integer, scratch, base);
					else
						ficlLtoa(integer, scratch, base);
					base = 10;
					unsignedInteger = 0; /* false */
					source = scratch;
					actualLength = strlen(scratch);
					break;
				}
				case '%':
					source = format;
					actualLength = 1;
				default:
					continue;
			}
		}

		if (append)
		{
			if (!desiredLength)
				desiredLength = actualLength;
			if (desiredLength > bufferLength)
			{
				append = 0; /* false */
				desiredLength = bufferLength;
			}
			while (desiredLength > actualLength)
				{
				*buffer++ = (char)((leadingZeroes) ? '0' : ' ');
				bufferLength--;
				desiredLength--;
				}
			memcpy(buffer, source, actualLength);
			buffer += actualLength;
			bufferLength -= actualLength;
		}

		format++;
	}

	ficlStackPushPointer(vm->dataStack, bufferStart);
	ficlStackPushInteger(vm->dataStack, buffer - bufferStart);
	ficlStackPushInteger(vm->dataStack, append && FICL_TRUE);
}


/**************************************************************************
                        d u p   &   f r i e n d s
** 
**************************************************************************/

static void ficlPrimitiveDepth(ficlVm *vm)
{
    int i;

    FICL_STACK_CHECK(vm->dataStack, 0, 1);

    i = ficlStackDepth(vm->dataStack);
    ficlStackPushInteger(vm->dataStack, i);
    return;
}


/**************************************************************************
                        e m i t   &   f r i e n d s
** 
**************************************************************************/

static void ficlPrimitiveEmit(ficlVm *vm)
{
    char *buffer = vm->pad;
    int i;


    FICL_STACK_CHECK(vm->dataStack, 1, 0);

    i = ficlStackPopInteger(vm->dataStack);
    buffer[0] = (char)i;
    buffer[1] = '\0';
    ficlVmTextOut(vm, buffer);
    return;
}


static void ficlPrimitiveCR(ficlVm *vm)
{
    ficlVmTextOut(vm, "\n");
    return;
}


static void ficlPrimitiveBackslash(ficlVm *vm)
{
    char *trace        = ficlVmGetInBuf(vm);
    char *stop      = ficlVmGetInBufEnd(vm);
    char c = *trace;

    while ((trace != stop) && (c != '\r') && (c != '\n'))
    {
        c = *++trace;
    }

    /*
    ** Cope with DOS or UNIX-style EOLs -
    ** Check for /r, /n, /r/n, or /n/r end-of-line sequences,
    ** and point trace to next char. If EOL is \0, we're done.
    */
    if (trace != stop)
    {
        trace++;

        if ( (trace != stop) && (c != *trace) 
             && ((*trace == '\r') || (*trace == '\n')) )
            trace++;
    }

    ficlVmUpdateTib(vm, trace);
    return;
}


/*
** paren CORE 
** Compilation: Perform the execution semantics given below.
** Execution: ( "ccc<paren>" -- )
** Parse ccc delimited by ) (right parenthesis). ( is an immediate word. 
** The number of characters in ccc may be zero to the number of characters
** in the parse area. 
** 
*/
static void ficlPrimitiveParenthesis(ficlVm *vm)
{
    ficlVmParseStringEx(vm, ')', 0);
    return;
}


/**************************************************************************
                        F E T C H   &   S T O R E
** 
**************************************************************************/

/**************************************************************************
                        i f C o I m
** IMMEDIATE
** Compiles code for a conditional branch into the dictionary
** and pushes the branch patch address on the stack for later
** patching by ELSE or THEN/ENDIF. 
**************************************************************************/

static void ficlPrimitiveIfCoIm(ficlVm *vm)
{
    ficlDictionary *dictionary = ficlVmGetDictionary(vm);

    ficlDictionaryAppendUnsigned(dictionary, ficlInstructionBranch0ParenWithCheck);
    markBranch(dictionary, vm, origTag);
    ficlDictionaryAppendUnsigned(dictionary, 1);
    return;
}




/**************************************************************************
                        e l s e C o I m
** 
** IMMEDIATE -- compiles an "else"...
** 1) FICL_VM_STATE_COMPILE a branch and a patch address; the address gets patched
**    by "endif" to point past the "else" code.
** 2) Pop the the "if" patch address
** 3) Patch the "if" branch to point to the current FICL_VM_STATE_COMPILE address.
** 4) Push the "else" patch address. ("endif" patches this to jump past 
**    the "else" code.
**************************************************************************/

static void ficlPrimitiveElseCoIm(ficlVm *vm)
{
    ficlCell *patchAddr;
    ficlInteger offset;
    ficlDictionary *dictionary = ficlVmGetDictionary(vm);

                                            /* (1) FICL_VM_STATE_COMPILE branch runtime */
    ficlDictionaryAppendUnsigned(dictionary, ficlInstructionBranchParenWithCheck);
    matchControlTag(vm, origTag);
    patchAddr = 
        (ficlCell *)ficlStackPopPointer(vm->dataStack);   /* (2) pop "if" patch addr */
    markBranch(dictionary, vm, origTag);           /* (4) push "else" patch addr */
    ficlDictionaryAppendUnsigned(dictionary, 1);                 /* (1) FICL_VM_STATE_COMPILE patch placeholder */
    offset = dictionary->here - patchAddr;
    *patchAddr = FICL_LVALUE_TO_CELL(offset);      /* (3) Patch "if" */

    return;
}


/**************************************************************************
                        e n d i f C o I m
** 
**************************************************************************/

static void ficlPrimitiveEndifCoIm(ficlVm *vm)
{
    ficlDictionary *dictionary = ficlVmGetDictionary(vm);
    resolveForwardBranch(dictionary, vm, origTag);
    return;
}


/**************************************************************************
                        c a s e C o I m
** IMMEDIATE FICL_VM_STATE_COMPILE-ONLY
**
**
** At FICL_VM_STATE_COMPILE-time, a CASE-SYS (see DPANS94 6.2.0873) looks like this:
**			i*addr i caseTag
** and an OF-SYS (see DPANS94 6.2.1950) looks like this:
**			i*addr i caseTag addr ofTag
** The integer under caseTag is the count of fixup addresses that branch
** to ENDCASE.
**************************************************************************/

static void ficlPrimitiveCaseCoIm(ficlVm *vm)
{
    FICL_STACK_CHECK(vm->dataStack, 0, 2);

	ficlStackPushUnsigned(vm->dataStack, 0);
	markControlTag(vm, caseTag);
    return;
}


/**************************************************************************
                        e n d c a s eC o I m
** IMMEDIATE FICL_VM_STATE_COMPILE-ONLY
**************************************************************************/

static void ficlPrimitiveEndcaseCoIm(ficlVm *vm)
{
	ficlUnsigned fixupCount;
    ficlDictionary *dictionary;
    ficlCell *patchAddr;
    ficlInteger offset;

	/*
	** if the last OF ended with FALLTHROUGH,
	** just add the FALLTHROUGH fixup to the
	** ENDOF fixups
	*/
	if (ficlStackGetTop(vm->dataStack).p == fallthroughTag)
	{
		matchControlTag(vm, fallthroughTag);
		patchAddr = ficlStackPopPointer(vm->dataStack);
	    matchControlTag(vm, caseTag);
		fixupCount = ficlStackPopUnsigned(vm->dataStack);
		ficlStackPushPointer(vm->dataStack, patchAddr);
		ficlStackPushUnsigned(vm->dataStack, fixupCount + 1);
		markControlTag(vm, caseTag);
	}

    matchControlTag(vm, caseTag);

    FICL_STACK_CHECK(vm->dataStack, 1, 0);

	fixupCount = ficlStackPopUnsigned(vm->dataStack);
    FICL_STACK_CHECK(vm->dataStack, fixupCount, 0);

    dictionary = ficlVmGetDictionary(vm);

    ficlDictionaryAppendUnsigned(dictionary, ficlInstructionDrop);

	while (fixupCount--)
	{
		patchAddr = (ficlCell *)ficlStackPopPointer(vm->dataStack);
		offset = dictionary->here - patchAddr;
		*patchAddr = FICL_LVALUE_TO_CELL(offset);
	}
    return;
}


/**************************************************************************
                        o f C o I m
** IMMEDIATE FICL_VM_STATE_COMPILE-ONLY
**************************************************************************/

static void ficlPrimitiveOfCoIm(ficlVm *vm)
{
    ficlDictionary *dictionary = ficlVmGetDictionary(vm);
	ficlCell *fallthroughFixup = NULL;

    FICL_STACK_CHECK(vm->dataStack, 1, 3);

	if (ficlStackGetTop(vm->dataStack).p == fallthroughTag)
	{
		matchControlTag(vm, fallthroughTag);
		fallthroughFixup = ficlStackPopPointer(vm->dataStack);
	}

	matchControlTag(vm, caseTag);

	markControlTag(vm, caseTag);

    ficlDictionaryAppendUnsigned(dictionary, ficlInstructionOfParen);
    markBranch(dictionary, vm, ofTag);
    ficlDictionaryAppendUnsigned(dictionary, 2);

	if (fallthroughFixup != NULL)
	{
		ficlInteger offset = dictionary->here - fallthroughFixup;
		*fallthroughFixup = FICL_LVALUE_TO_CELL(offset);
	}

    return;
}


/**************************************************************************
                    e n d o f C o I m
** IMMEDIATE FICL_VM_STATE_COMPILE-ONLY
**************************************************************************/

static void ficlPrimitiveEndofCoIm(ficlVm *vm)
{
    ficlCell *patchAddr;
    ficlUnsigned fixupCount;
    ficlInteger offset;
    ficlDictionary *dictionary = ficlVmGetDictionary(vm);

    FICL_STACK_CHECK(vm->dataStack, 4, 3);

	/* ensure we're in an OF, */
    matchControlTag(vm, ofTag);
	/* grab the address of the branch location after the OF */
    patchAddr = (ficlCell *)ficlStackPopPointer(vm->dataStack);
	/* ensure we're also in a "case" */
    matchControlTag(vm, caseTag);
	/* grab the current number of ENDOF fixups */
	fixupCount = ficlStackPopUnsigned(vm->dataStack);

    /* FICL_VM_STATE_COMPILE branch runtime */
    ficlDictionaryAppendUnsigned(dictionary, ficlInstructionBranchParenWithCheck);

	/* push a new ENDOF fixup, the updated count of ENDOF fixups, and the caseTag */
    ficlStackPushPointer(vm->dataStack, dictionary->here);
    ficlStackPushUnsigned(vm->dataStack, fixupCount + 1);
	markControlTag(vm, caseTag);

	/* reserve space for the ENDOF fixup */
    ficlDictionaryAppendUnsigned(dictionary, 2);

	/* and patch the original OF */
    offset = dictionary->here - patchAddr;
    *patchAddr = FICL_LVALUE_TO_CELL(offset);
}

/**************************************************************************
                    f a l l t h r o u g h C o I m
** IMMEDIATE FICL_VM_STATE_COMPILE-ONLY
**************************************************************************/

static void ficlPrimitiveFallthroughCoIm(ficlVm *vm)
{
    ficlCell *patchAddr;
    ficlInteger offset;
    ficlDictionary *dictionary = ficlVmGetDictionary(vm);

    FICL_STACK_CHECK(vm->dataStack, 4, 3);

	/* ensure we're in an OF, */
    matchControlTag(vm, ofTag);
	/* grab the address of the branch location after the OF */
    patchAddr = (ficlCell *)ficlStackPopPointer(vm->dataStack);
	/* ensure we're also in a "case" */
    matchControlTag(vm, caseTag);

	/* okay, here we go.  put the case tag back. */
	markControlTag(vm, caseTag);

    /* FICL_VM_STATE_COMPILE branch runtime */
    ficlDictionaryAppendUnsigned(dictionary, ficlInstructionBranchParenWithCheck);

	/* push a new FALLTHROUGH fixup and the fallthroughTag */
    ficlStackPushPointer(vm->dataStack, dictionary->here);
	markControlTag(vm, fallthroughTag);

	/* reserve space for the FALLTHROUGH fixup */
    ficlDictionaryAppendUnsigned(dictionary, 2);

	/* and patch the original OF */
    offset = dictionary->here - patchAddr;
    *patchAddr = FICL_LVALUE_TO_CELL(offset);
}

/**************************************************************************
                        h a s h
** hash ( c-addr u -- code)
** calculates hashcode of specified string and leaves it on the stack
**************************************************************************/

static void ficlPrimitiveHash(ficlVm *vm)
{
    ficlString s;
    FICL_STRING_SET_LENGTH(s, ficlStackPopUnsigned(vm->dataStack));
    FICL_STRING_SET_POINTER(s, ficlStackPopPointer(vm->dataStack));
    ficlStackPushUnsigned(vm->dataStack, ficlHashCode(s));
    return;
}


/**************************************************************************
                        i n t e r p r e t 
** This is the "user interface" of a Forth. It does the following:
**   while there are words in the VM's Text Input Buffer
**     Copy next word into the pad (ficlVmGetWord)
**     Attempt to find the word in the dictionary (ficlDictionaryLookup)
**     If successful, execute the word.
**     Otherwise, attempt to convert the word to a number (isNumber)
**     If successful, push the number onto the parameter stack.
**     Otherwise, print an error message and exit loop...
**   End Loop
**
** From the standard, section 3.4
** Text interpretation (see 6.1.1360 EVALUATE and 6.1.2050 QUIT) shall
** repeat the following steps until either the parse area is empty or an 
** ambiguous condition exists: 
** a) Skip leading spaces and parse a name (see 3.4.1); 
**************************************************************************/

static void ficlPrimitiveInterpret(ficlVm *vm)
{
    ficlString s;
    int i;
    ficlSystem *system;

    FICL_VM_ASSERT(vm, vm);

    system = vm->callback.system;
    s   = ficlVmGetWord0(vm);

    /*
    ** Get next word...if out of text, we're done.
    */
    if (s.length == 0)
    {
        ficlVmThrow(vm, FICL_VM_STATUS_OUT_OF_TEXT);
    }

    /*
    ** Run the parse chain against the incoming token until somebody eats it.
    ** Otherwise emit an error message and give up.
    */
    for (i=0; i < FICL_MAX_PARSE_STEPS; i++)
    {
        ficlWord *word = system->parseList[i];
           
        if (word == NULL)
            break;

        if (word->code == ficlPrimitiveParseStepParen)
        {
            ficlParseStep pStep;
            pStep = (ficlParseStep)(word->param->fn);
            if ((*pStep)(vm, s))
                return;
        }
        else
        {
            ficlStackPushPointer(vm->dataStack, FICL_STRING_GET_POINTER(s));
            ficlStackPushUnsigned(vm->dataStack, FICL_STRING_GET_LENGTH(s));
            ficlVmExecuteXT(vm, word);
            if (ficlStackPopInteger(vm->dataStack))
                return;
        }
    }

    ficlVmThrowError(vm, "%.*s not found", FICL_STRING_GET_LENGTH(s), FICL_STRING_GET_POINTER(s));

    return;                 /* back to inner interpreter */
}


/*
** Surrogate precompiled parse step for ficlParseWord (this step is hard coded in 
** FICL_VM_STATE_INTERPRET)
*/
static void ficlPrimitiveLookup(ficlVm *vm)
{
    ficlString name;
    FICL_STRING_SET_LENGTH(name, ficlStackPopUnsigned(vm->dataStack));
    FICL_STRING_SET_POINTER(name, ficlStackPopPointer(vm->dataStack));
    ficlStackPushInteger(vm->dataStack, ficlVmParseWord(vm, name));
    return;
}


/**************************************************************************
                        p a r e n P a r s e S t e p
** (parse-step)  ( c-addr u -- flag )
** runtime for a precompiled parse step - pop a counted string off the
** stack, run the parse step against it, and push the result flag (FICL_TRUE
** if success, FICL_FALSE otherwise).
**************************************************************************/

void ficlPrimitiveParseStepParen(ficlVm *vm)
{
    ficlString s;
    ficlWord *word = vm->runningWord;
    ficlParseStep pStep = (ficlParseStep)(word->param->fn);

    FICL_STRING_SET_LENGTH(s, ficlStackPopInteger(vm->dataStack));
    FICL_STRING_SET_POINTER(s, ficlStackPopPointer(vm->dataStack));
    
    ficlStackPushInteger(vm->dataStack, (*pStep)(vm, s));

    return;
}


static void ficlPrimitiveAddParseStep(ficlVm *vm)
{
    ficlWord *pStep;
    ficlDictionary *dictionary = ficlVmGetDictionary(vm);

    FICL_STACK_CHECK(vm->dataStack, 1, 0);

    pStep = (ficlWord *)(ficlStackPop(vm->dataStack).p);
    if ((pStep != NULL) && ficlDictionaryIsAWord(dictionary, pStep))
        ficlSystemAddParseStep(vm->callback.system, pStep);
    return;
}



/**************************************************************************
                        l i t e r a l I m
** 
** IMMEDIATE code for "literal". This function gets a value from the stack 
** and compiles it into the dictionary preceded by the code for "(literal)".
** IMMEDIATE
**************************************************************************/

void ficlPrimitiveLiteralIm(ficlVm *vm)
{
    ficlDictionary *dictionary = ficlVmGetDictionary(vm);
	ficlInteger value;

	value = ficlStackPopInteger(vm->dataStack);

	switch (value)
	{
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
		case 16:
			ficlDictionaryAppendUnsigned(dictionary, value);
			break;

		case 0:
		case -1:
		case -2:
		case -3:
		case -4:
		case -5:
		case -6:
		case -7:
		case -8:
		case -9:
		case -10:
		case -11:
		case -12:
		case -13:
		case -14:
		case -15:
		case -16:
			ficlDictionaryAppendUnsigned(dictionary, ficlInstruction0- value);
			break;

		default:
			ficlDictionaryAppendUnsigned(dictionary, ficlInstructionLiteralParen);
			ficlDictionaryAppendUnsigned(dictionary, value);
			break;
	}

    return;
}


static void ficlPrimitive2LiteralIm(ficlVm *vm)
{
    ficlDictionary *dictionary = ficlVmGetDictionary(vm);

    ficlDictionaryAppendUnsigned(dictionary, ficlInstruction2LiteralParen);
    ficlDictionaryAppendCell(dictionary, ficlStackPop(vm->dataStack));
    ficlDictionaryAppendCell(dictionary, ficlStackPop(vm->dataStack));

    return;
}

/**************************************************************************
                               D o  /  L o o p
** do -- IMMEDIATE FICL_VM_STATE_COMPILE ONLY
**    Compiles code to initialize a loop: FICL_VM_STATE_COMPILE (do), 
**    allot space to hold the "leave" address, push a branch
**    target address for the loop.
** (do) -- runtime for "do"
**    pops index and limit from the p stack and moves them
**    to the r stack, then skips to the loop body.
** loop -- IMMEDIATE FICL_VM_STATE_COMPILE ONLY
** +loop
**    Compiles code for the test part of a loop:
**    FICL_VM_STATE_COMPILE (loop), resolve forward branch from "do", and
**    copy "here" address to the "leave" address allotted by "do"
** i,j,k -- FICL_VM_STATE_COMPILE ONLY
**    Runtime: Push loop indices on param stack (i is innermost loop...)
**    Note: each loop has three values on the return stack:
**    ( R: leave limit index )
**    "leave" is the absolute address of the next ficlCell after the loop
**    limit and index are the loop control variables.
** leave -- FICL_VM_STATE_COMPILE ONLY
**    Runtime: pop the loop control variables, then pop the
**    "leave" address and jump (absolute) there.
**************************************************************************/

static void ficlPrimitiveDoCoIm(ficlVm *vm)
{
    ficlDictionary *dictionary = ficlVmGetDictionary(vm);

    ficlDictionaryAppendUnsigned(dictionary, ficlInstructionDoParen);
    /*
    ** Allot space for a pointer to the end
    ** of the loop - "leave" uses this...
    */
    markBranch(dictionary, vm, leaveTag);
    ficlDictionaryAppendUnsigned(dictionary, 0);
    /*
    ** Mark location of head of loop...
    */
    markBranch(dictionary, vm, doTag);

    return;
}


static void ficlPrimitiveQDoCoIm(ficlVm *vm)
{
    ficlDictionary *dictionary = ficlVmGetDictionary(vm);

    ficlDictionaryAppendUnsigned(dictionary, ficlInstructionQDoParen);
    /*
    ** Allot space for a pointer to the end
    ** of the loop - "leave" uses this...
    */
    markBranch(dictionary, vm, leaveTag);
    ficlDictionaryAppendUnsigned(dictionary, 0);
    /*
    ** Mark location of head of loop...
    */
    markBranch(dictionary, vm, doTag);

    return;
}


static void ficlPrimitiveLoopCoIm(ficlVm *vm)
{
    ficlDictionary *dictionary = ficlVmGetDictionary(vm);

    ficlDictionaryAppendUnsigned(dictionary, ficlInstructionLoopParen);
    resolveBackBranch(dictionary, vm, doTag);
    resolveAbsBranch(dictionary, vm, leaveTag);
    return;
}


static void ficlPrimitivePlusLoopCoIm(ficlVm *vm)
{
    ficlDictionary *dictionary = ficlVmGetDictionary(vm);

    ficlDictionaryAppendUnsigned(dictionary, ficlInstructionPlusLoopParen);
    resolveBackBranch(dictionary, vm, doTag);
    resolveAbsBranch(dictionary, vm, leaveTag);
    return;
}



/**************************************************************************
                        v a r i a b l e
** 
**************************************************************************/

static void ficlPrimitiveVariable(ficlVm *vm)
{
    ficlDictionary *dictionary = ficlVmGetDictionary(vm);
    ficlString name = ficlVmGetWord(vm);

    ficlDictionaryAppendWord(dictionary, name, (ficlPrimitive)ficlInstructionVariableParen, FICL_WORD_DEFAULT);
    ficlVmDictionaryAllotCells(vm, dictionary, 1);
    return;
}


static void ficlPrimitive2Variable(ficlVm *vm)
{
    ficlDictionary *dictionary = ficlVmGetDictionary(vm);
    ficlString name = ficlVmGetWord(vm);

    ficlDictionaryAppendWord(dictionary, name, (ficlPrimitive)ficlInstructionVariableParen, FICL_WORD_DEFAULT);
    ficlVmDictionaryAllotCells(vm, dictionary, 2);
    return;
}


/**************************************************************************
                        b a s e   &   f r i e n d s
** 
**************************************************************************/

static void ficlPrimitiveBase(ficlVm *vm)
{
    ficlCell *pBase;

    FICL_STACK_CHECK(vm->dataStack, 0, 1);

    pBase = (ficlCell *)(&vm->base);
    ficlStackPush(vm->dataStack, FICL_LVALUE_TO_CELL(pBase));
    return;
}


static void ficlPrimitiveDecimal(ficlVm *vm)
{
    vm->base = 10;
    return;
}


static void ficlPrimitiveHex(ficlVm *vm)
{
    vm->base = 16;
    return;
}


/**************************************************************************
                        a l l o t   &   f r i e n d s
** 
**************************************************************************/

static void ficlPrimitiveAllot(ficlVm *vm)
{
    ficlDictionary *dictionary;
    ficlInteger i;

    FICL_STACK_CHECK(vm->dataStack, 1, 0);

    dictionary = ficlVmGetDictionary(vm);
    i = ficlStackPopInteger(vm->dataStack);

    FICL_VM_DICTIONARY_CHECK(vm, dictionary, i);

    ficlVmDictionaryAllot(vm, dictionary, i);
    return;
}


static void ficlPrimitiveHere(ficlVm *vm)
{
    ficlDictionary *dictionary;

    FICL_STACK_CHECK(vm->dataStack, 0, 1);

    dictionary = ficlVmGetDictionary(vm);
    ficlStackPushPointer(vm->dataStack, dictionary->here);
    return;
}




/**************************************************************************
                        t i c k
** tick         CORE ( "<spaces>name" -- xt )
** Skip leading space delimiters. Parse name delimited by a space. Find
** name and return xt, the execution token for name. An ambiguous condition
** exists if name is not found. 
**************************************************************************/
void ficlPrimitiveTick(ficlVm *vm)
{
    ficlWord *word = NULL;
    ficlString name = ficlVmGetWord(vm);

    FICL_STACK_CHECK(vm->dataStack, 0, 1);

    word = ficlDictionaryLookup(ficlVmGetDictionary(vm), name);
    if (!word)
        ficlVmThrowError(vm, "%.*s not found", FICL_STRING_GET_LENGTH(name), FICL_STRING_GET_POINTER(name));
    ficlStackPushPointer(vm->dataStack, word);
    return;
}


static void ficlPrimitiveBracketTickCoIm(ficlVm *vm)
{
    ficlPrimitiveTick(vm);
    ficlPrimitiveLiteralIm(vm);
    
    return;
}


/**************************************************************************
                        p o s t p o n e
** Lookup the next word in the input stream and FICL_VM_STATE_COMPILE code to 
** insert it into definitions created by the resulting word
** (defers compilation, even of immediate words)
**************************************************************************/

static void ficlPrimitivePostponeCoIm(ficlVm *vm)
{
    ficlDictionary *dictionary  = ficlVmGetDictionary(vm);
    ficlWord *word;
    ficlWord *pComma = ficlSystemLookup(vm->callback.system, ",");
    FICL_VM_ASSERT(vm, pComma);

    ficlPrimitiveTick(vm);
    word = ficlStackGetTop(vm->dataStack).p;
    if (ficlWordIsImmediate(word))
    {
        ficlDictionaryAppendCell(dictionary, ficlStackPop(vm->dataStack));
    }
    else
    {
        ficlPrimitiveLiteralIm(vm);
        ficlDictionaryAppendCell(dictionary, FICL_LVALUE_TO_CELL(pComma));
    }
    
    return;
}



/**************************************************************************
                        e x e c u t e
** Pop an execution token (pointer to a word) off the stack and
** run it
**************************************************************************/

static void ficlPrimitiveExecute(ficlVm *vm)
{
    ficlWord *word;

    FICL_STACK_CHECK(vm->dataStack, 1, 0);

    word = ficlStackPopPointer(vm->dataStack);
    ficlVmExecuteWord(vm, word);

    return;
}


/**************************************************************************
                        i m m e d i a t e
** Make the most recently compiled word IMMEDIATE -- it executes even
** in FICL_VM_STATE_COMPILE state (most often used for control compiling words
** such as IF, THEN, etc)
**************************************************************************/

static void ficlPrimitiveImmediate(ficlVm *vm)
{
    FICL_IGNORE(vm);
    ficlDictionarySetImmediate(ficlVmGetDictionary(vm));
    return;
}


static void ficlPrimitiveCompileOnly(ficlVm *vm)
{
    FICL_IGNORE(vm);
    ficlDictionarySetFlags(ficlVmGetDictionary(vm), FICL_WORD_COMPILE_ONLY);
    return;
}


static void ficlPrimitiveSetObjectFlag(ficlVm *vm)
{
    FICL_IGNORE(vm);
    ficlDictionarySetFlags(ficlVmGetDictionary(vm), FICL_WORD_OBJECT);
    return;
}

static void ficlPrimitiveIsObject(ficlVm *vm)
{
    int flag;
    ficlWord *word = (ficlWord *)ficlStackPopPointer(vm->dataStack);
    
    flag = ((word != NULL) && (word->flags & FICL_WORD_OBJECT)) ? FICL_TRUE : FICL_FALSE;
    ficlStackPushInteger(vm->dataStack, flag);
    return;
}



static void ficlPrimitiveCountedStringQuoteIm(ficlVm *vm)
{
    ficlDictionary *dictionary = ficlVmGetDictionary(vm);

    if (vm->state == FICL_VM_STATE_INTERPRET)
    {
        ficlCountedString *counted = (ficlCountedString *) dictionary->here;
        ficlVmGetString(vm, counted, '\"');
        ficlStackPushPointer(vm->dataStack, counted);
		/* move HERE past string so it doesn't get overwritten.  --lch */
		ficlVmDictionaryAllot(vm, dictionary, counted->length + sizeof(ficlUnsigned8));
    }
    else    /* FICL_VM_STATE_COMPILE state */
    {
        ficlDictionaryAppendUnsigned(dictionary, ficlInstructionCStringLiteralParen);
        dictionary->here = FICL_POINTER_TO_CELL(ficlVmGetString(vm, (ficlCountedString *)dictionary->here, '\"'));
        ficlDictionaryAlign(dictionary);
    }

    return;
}

/**************************************************************************
                        d o t Q u o t e
** IMMEDIATE word that compiles a string literal for later display
** FICL_VM_STATE_COMPILE fiStringLiteralParen, then copy the bytes of the string from the
** TIB to the dictionary. Backpatch the count byte and align the dictionary.
**************************************************************************/

static void ficlPrimitiveDotQuoteCoIm(ficlVm *vm)
{
    ficlDictionary *dictionary = ficlVmGetDictionary(vm);
    ficlWord *pType = ficlSystemLookup(vm->callback.system, "type");
    FICL_VM_ASSERT(vm, pType);
    ficlDictionaryAppendUnsigned(dictionary, ficlInstructionStringLiteralParen);
    dictionary->here = FICL_POINTER_TO_CELL(ficlVmGetString(vm, (ficlCountedString *)dictionary->here, '\"'));
    ficlDictionaryAlign(dictionary);
    ficlDictionaryAppendCell(dictionary, FICL_LVALUE_TO_CELL(pType));
    return;
}


static void ficlPrimitiveDotParen(ficlVm *vm)
{
    char *from      = ficlVmGetInBuf(vm);
    char *stop      = ficlVmGetInBufEnd(vm);
    char *to     = vm->pad;
    char c;

    /*
    ** Note: the standard does not want leading spaces skipped.
    */
    for (c = *from; (from != stop) && (c != ')'); c = *++from)
        *to++ = c;

    *to = '\0';
    if ((from != stop) && (c == ')'))
        from++;

    ficlVmTextOut(vm, vm->pad);
    ficlVmUpdateTib(vm, from);
        
    return;
}


/**************************************************************************
                        s l i t e r a l
** STRING 
** Interpretation: Interpretation semantics for this word are undefined.
** Compilation: ( c-addr1 u -- )
** Append the run-time semantics given below to the current definition.
** Run-time:       ( -- c-addr2 u )
** Return c-addr2 u describing a string consisting of the characters
** specified by c-addr1 u during compilation. A program shall not alter
** the returned string. 
**************************************************************************/
static void ficlPrimitiveSLiteralCoIm(ficlVm *vm)
{
    ficlDictionary *dictionary;
    char *from;
	char *to;
    ficlUnsigned length;

    FICL_STACK_CHECK(vm->dataStack, 2, 0);

    dictionary = ficlVmGetDictionary(vm);
    length  = ficlStackPopUnsigned(vm->dataStack);
    from = ficlStackPopPointer(vm->dataStack);

    ficlDictionaryAppendUnsigned(dictionary, ficlInstructionStringLiteralParen);
    to    = (char *) dictionary->here;
    *to++ = (char)   length;

    for (; length > 0; --length)
    {
        *to++ = *from++;
    }

    *to++ = 0;
    dictionary->here = FICL_POINTER_TO_CELL(ficlAlignPointer(to));
    return;
}


/**************************************************************************
                        s t a t e
** Return the address of the VM's state member (must be sized the
** same as a ficlCell for this reason)
**************************************************************************/
static void ficlPrimitiveState(ficlVm *vm)
{
    FICL_STACK_CHECK(vm->dataStack, 0, 1);
    ficlStackPushPointer(vm->dataStack, &vm->state);
    return;
}


/**************************************************************************
                        c r e a t e . . . d o e s >
** Make a new word in the dictionary with the run-time effect of 
** a variable (push my address), but with extra space allotted
** for use by does> .
**************************************************************************/


static void ficlPrimitiveCreate(ficlVm *vm)
{
    ficlDictionary *dictionary = ficlVmGetDictionary(vm);
    ficlString name = ficlVmGetWord(vm);

    ficlDictionaryAppendWord(dictionary, name, (ficlPrimitive)ficlInstructionCreateParen, FICL_WORD_DEFAULT);
    ficlVmDictionaryAllotCells(vm, dictionary, 1);
    return;
}


static void ficlPrimitiveDoesCoIm(ficlVm *vm)
{
    ficlDictionary *dictionary = ficlVmGetDictionary(vm);
#if FICL_WANT_LOCALS
    if (vm->callback.system->localsCount > 0)
    {
        ficlDictionary *locals = ficlSystemGetLocals(vm->callback.system);
        ficlDictionaryEmpty(locals, locals->forthWordlist->size);
        ficlDictionaryAppendUnsigned(dictionary, ficlInstructionUnlinkParen);
    }

    vm->callback.system->localsCount = 0;
#endif
    FICL_IGNORE(vm);

    ficlDictionaryAppendUnsigned(dictionary, ficlInstructionDoesParen);
    return;
}


/**************************************************************************
                        t o   b o d y
** to-body      CORE ( xt -- a-addr )
** a-addr is the data-field address corresponding to xt. An ambiguous
** condition exists if xt is not for a word defined via CREATE. 
**************************************************************************/
static void ficlPrimitiveToBody(ficlVm *vm)
{
    ficlWord *word;
    FICL_STACK_CHECK(vm->dataStack, 1, 1);

    word = ficlStackPopPointer(vm->dataStack);
    ficlStackPushPointer(vm->dataStack, word->param + 1);
    return;
}


/*
** from-body       Ficl ( a-addr -- xt )
** Reverse effect of >body
*/
static void ficlPrimitiveFromBody(ficlVm *vm)
{
    char *ptr;
    FICL_STACK_CHECK(vm->dataStack, 1, 1);

    ptr = (char *)ficlStackPopPointer(vm->dataStack) - sizeof (ficlWord);
    ficlStackPushPointer(vm->dataStack, ptr);
    return;
}


/*
** >name        Ficl ( xt -- c-addr u )
** Push the address and length of a word's name given its address
** xt. 
*/
static void ficlPrimitiveToName(ficlVm *vm)
{
    ficlWord *word;

    FICL_STACK_CHECK(vm->dataStack, 1, 2);

    word = ficlStackPopPointer(vm->dataStack);
    ficlStackPushPointer(vm->dataStack, word->name);
    ficlStackPushUnsigned(vm->dataStack, word->length);
    return;
}


static void ficlPrimitiveLastWord(ficlVm *vm)
{
    ficlDictionary *dictionary = ficlVmGetDictionary(vm);
    ficlWord *wp = dictionary->smudge;
    FICL_VM_ASSERT(vm, wp);
    ficlVmPush(vm, FICL_LVALUE_TO_CELL(wp));
    return;
}


/**************************************************************************
                        l b r a c k e t   e t c
** 
**************************************************************************/

static void ficlPrimitiveLeftBracketCoIm(ficlVm *vm)
{
    vm->state = FICL_VM_STATE_INTERPRET;
    return;
}


static void ficlPrimitiveRightBracket(ficlVm *vm)
{
    vm->state = FICL_VM_STATE_COMPILE;
    return;
}


/**************************************************************************
                        p i c t u r e d   n u m e r i c   w o r d s
**
** less-number-sign CORE ( -- )
** Initialize the pictured numeric output conversion process. 
** (clear the pad)
**************************************************************************/
static void ficlPrimitiveLessNumberSign(ficlVm *vm)
{
    ficlCountedString *counted = FICL_POINTER_TO_COUNTED_STRING(vm->pad);
    counted->length = 0;
    return;
}

/*
** number-sign      CORE ( ud1 -- ud2 )
** Divide ud1 by the number in BASE giving the quotient ud2 and the remainder
** n. (n is the least-significant digit of ud1.) Convert n to external form
** and add the resulting character to the beginning of the pictured numeric
** output  string. An ambiguous condition exists if # executes outside of a
** <# #> delimited number conversion. 
*/
static void ficlPrimitiveNumberSign(ficlVm *vm)
{
    ficlCountedString *counted;
    ficl2Unsigned u;
    ficl2UnsignedQR uqr;

    FICL_STACK_CHECK(vm->dataStack, 2, 2);

    counted = FICL_POINTER_TO_COUNTED_STRING(vm->pad);
    u = ficlStackPop2Unsigned(vm->dataStack);
    uqr = ficl2UnsignedDivide(u, (ficlUnsigned16)(vm->base));
    counted->text[counted->length++] = ficlDigitToCharacter(uqr.remainder);
    ficlStackPush2Unsigned(vm->dataStack, uqr.quotient);
    return;
}

/*
** number-sign-greater CORE ( xd -- c-addr u )
** Drop xd. Make the pictured numeric output string available as a character
** string. c-addr and u specify the resulting character string. A program
** may replace characters within the string. 
*/
static void ficlPrimitiveNumberSignGreater(ficlVm *vm)
{
    ficlCountedString *counted;

    FICL_STACK_CHECK(vm->dataStack, 2, 2);

    counted = FICL_POINTER_TO_COUNTED_STRING(vm->pad);
    counted->text[counted->length] = 0;
    ficlStringReverse(counted->text);
    ficlStackDrop(vm->dataStack, 2);
    ficlStackPushPointer(vm->dataStack, counted->text);
    ficlStackPushUnsigned(vm->dataStack, counted->length);
    return;
}

/*
** number-sign-s    CORE ( ud1 -- ud2 )
** Convert one digit of ud1 according to the rule for #. Continue conversion
** until the quotient is zero. ud2 is zero. An ambiguous condition exists if
** #S executes outside of a <# #> delimited number conversion. 
** TO DO: presently does not use ud1 hi ficlCell - use it!
*/
static void ficlPrimitiveNumberSignS(ficlVm *vm)
{
    ficlCountedString *counted;
    ficl2Unsigned u;
    ficl2UnsignedQR uqr;

    FICL_STACK_CHECK(vm->dataStack, 2, 2);

    counted = FICL_POINTER_TO_COUNTED_STRING(vm->pad);
    u = ficlStackPop2Unsigned(vm->dataStack);

    do 
    {
        uqr = ficl2UnsignedDivide(u, (ficlUnsigned16)(vm->base));
        counted->text[counted->length++] = ficlDigitToCharacter(uqr.remainder);
		u = uqr.quotient;
    }
    while (FICL_2UNSIGNED_NOT_ZERO(u));

    ficlStackPush2Unsigned(vm->dataStack, u);
    return;
}

/*
** HOLD             CORE ( char -- )
** Add char to the beginning of the pictured numeric output string. An ambiguous
** condition exists if HOLD executes outside of a <# #> delimited number conversion.
*/
static void ficlPrimitiveHold(ficlVm *vm)
{
    ficlCountedString *counted;
    int i;

    FICL_STACK_CHECK(vm->dataStack, 1, 0);

    counted = FICL_POINTER_TO_COUNTED_STRING(vm->pad);
    i = ficlStackPopInteger(vm->dataStack);
    counted->text[counted->length++] = (char) i;
    return;
}

/*
** SIGN             CORE ( n -- )
** If n is negative, add a minus sign to the beginning of the pictured
** numeric output string. An ambiguous condition exists if SIGN
** executes outside of a <# #> delimited number conversion. 
*/
static void ficlPrimitiveSign(ficlVm *vm)
{
    ficlCountedString *counted;
    int i;

    FICL_STACK_CHECK(vm->dataStack, 1, 0);

    counted = FICL_POINTER_TO_COUNTED_STRING(vm->pad);
    i = ficlStackPopInteger(vm->dataStack);
    if (i < 0)
        counted->text[counted->length++] = '-';
    return;
}


/**************************************************************************
                        t o   N u m b e r
** to-number CORE ( ud1 c-addr1 u1 -- ud2 c-addr2 u2 )
** ud2 is the unsigned result of converting the characters within the
** string specified by c-addr1 u1 into digits, using the number in BASE,
** and adding each into ud1 after multiplying ud1 by the number in BASE.
** Conversion continues left-to-right until a character that is not
** convertible, including any + or -, is encountered or the string is
** entirely converted. c-addr2 is the location of the first unconverted
** character or the first character past the end of the string if the string
** was entirely converted. u2 is the number of unconverted characters in the
** string. An ambiguous condition exists if ud2 overflows during the
** conversion. 
**************************************************************************/
static void ficlPrimitiveToNumber(ficlVm *vm)
{
    ficlUnsigned length;
    char *trace;
    ficl2Unsigned accumulator;
    ficlUnsigned base = vm->base;
    ficlUnsigned c;
    ficlUnsigned digit;

    FICL_STACK_CHECK(vm->dataStack,4,4);

    length = ficlStackPopUnsigned(vm->dataStack);
    trace = (char *)ficlStackPopPointer(vm->dataStack);
    accumulator = ficlStackPop2Unsigned(vm->dataStack);

    for (c = *trace; length > 0; c = *++trace, length--)
    {
        if (c < '0')
            break;

        digit = c - '0';

        if (digit > 9)
            digit = tolower(c) - 'a' + 10;
        /* 
        ** Note: following test also catches chars between 9 and a
        ** because 'digit' is unsigned! 
        */
        if (digit >= base)
            break;

        accumulator = ficl2UnsignedMultiplyAccumulate(accumulator, base, digit);
    }

    ficlStackPush2Unsigned(vm->dataStack, accumulator);
    ficlStackPushPointer(vm->dataStack, trace);
    ficlStackPushUnsigned(vm->dataStack, length);

    return;
}



/**************************************************************************
                        q u i t   &   a b o r t
** quit CORE   ( -- )  ( R:  i*x -- )
** Empty the return stack, store zero in SOURCE-ID if it is present, make
** the user input device the input source, and enter interpretation state. 
** Do not display a message. Repeat the following: 
**
**   Accept a line from the input source into the input buffer, set >IN to
**   zero, and FICL_VM_STATE_INTERPRET. 
**   Display the implementation-defined system prompt if in
**   interpretation state, all processing has been completed, and no
**   ambiguous condition exists. 
**************************************************************************/

static void ficlPrimitiveQuit(ficlVm *vm)
{
    ficlVmThrow(vm, FICL_VM_STATUS_QUIT);
    return;
}


static void ficlPrimitiveAbort(ficlVm *vm)
{
    ficlVmThrow(vm, FICL_VM_STATUS_ABORT);
    return;
}


/**************************************************************************
                        a c c e p t
** accept       CORE ( c-addr +n1 -- +n2 )
** Receive a string of at most +n1 characters. An ambiguous condition
** exists if +n1 is zero or greater than 32,767. Display graphic characters
** as they are received. A program that depends on the presence or absence
** of non-graphic characters in the string has an environmental dependency.
** The editing functions, if any, that the system performs in order to
** construct the string are implementation-defined. 
**
** (Although the standard text doesn't say so, I assume that the intent 
** of 'accept' is to store the string at the address specified on
** the stack.)
** Implementation: if there's more text in the TIB, use it. Otherwise
** throw out for more text. Copy characters up to the max count into the
** address given, and return the number of actual characters copied.
** 
** Note (sobral) this may not be the behavior you'd expect if you're
** trying to get user input at load time!
**************************************************************************/
static void ficlPrimitiveAccept(ficlVm *vm)
{
    ficlUnsigned size;
    char *address;

	ficlUnsigned length;
	char *trace;
	char *end;

    FICL_STACK_CHECK(vm->dataStack, 2, 1);

    trace = ficlVmGetInBuf(vm);
    end = ficlVmGetInBufEnd(vm);
    length = end - trace;
    if (length == 0)
        ficlVmThrow(vm, FICL_VM_STATUS_RESTART);

    /*
    ** Now we have something in the text buffer - use it 
    */
    size = ficlStackPopInteger(vm->dataStack);
    address = ficlStackPopPointer(vm->dataStack);

    length = (size < length) ? size : length;
    strncpy(address, trace, length);
    trace += length;
    ficlVmUpdateTib(vm, trace);
    ficlStackPushInteger(vm->dataStack, length);

    return;
}


/**************************************************************************
                        a l i g n
** 6.1.0705 ALIGN       CORE ( -- )
** If the data-space pointer is not aligned, reserve enough space to
** align it. 
**************************************************************************/
static void ficlPrimitiveAlign(ficlVm *vm)
{
    ficlDictionary *dictionary = ficlVmGetDictionary(vm);
    FICL_IGNORE(vm);
    ficlDictionaryAlign(dictionary);
    return;
}


/**************************************************************************
                        a l i g n e d
** 
**************************************************************************/
static void ficlPrimitiveAligned(ficlVm *vm)
{
    void *addr;

    FICL_STACK_CHECK(vm->dataStack,1,1);

    addr = ficlStackPopPointer(vm->dataStack);
    ficlStackPushPointer(vm->dataStack, ficlAlignPointer(addr));
    return;
}


/**************************************************************************
                        b e g i n   &   f r i e n d s
** Indefinite loop control structures
** A.6.1.0760 BEGIN 
** Typical use: 
**      : X ... BEGIN ... test UNTIL ;
** or 
**      : X ... BEGIN ... test WHILE ... REPEAT ;
**************************************************************************/
static void ficlPrimitiveBeginCoIm(ficlVm *vm)
{
    ficlDictionary *dictionary = ficlVmGetDictionary(vm);
    markBranch(dictionary, vm, destTag);
    return;
}

static void ficlPrimitiveUntilCoIm(ficlVm *vm)
{
    ficlDictionary *dictionary = ficlVmGetDictionary(vm);

    ficlDictionaryAppendUnsigned(dictionary, ficlInstructionBranch0ParenWithCheck);
    resolveBackBranch(dictionary, vm, destTag);
    return;
}

static void ficlPrimitiveWhileCoIm(ficlVm *vm)
{
    ficlDictionary *dictionary = ficlVmGetDictionary(vm);

    FICL_STACK_CHECK(vm->dataStack, 2, 5);

    ficlDictionaryAppendUnsigned(dictionary, ficlInstructionBranch0ParenWithCheck);
    markBranch(dictionary, vm, origTag);

	/* equivalent to 2swap */
    ficlStackRoll(vm->dataStack, 3);
    ficlStackRoll(vm->dataStack, 3);

    ficlDictionaryAppendUnsigned(dictionary, 1);
    return;
}

static void ficlPrimitiveRepeatCoIm(ficlVm *vm)
{
    ficlDictionary *dictionary = ficlVmGetDictionary(vm);

    ficlDictionaryAppendUnsigned(dictionary, ficlInstructionBranchParenWithCheck);
    /* expect "begin" branch marker */
    resolveBackBranch(dictionary, vm, destTag);
    /* expect "while" branch marker */
    resolveForwardBranch(dictionary, vm, origTag);
    return;
}


static void ficlPrimitiveAgainCoIm(ficlVm *vm)
{
    ficlDictionary *dictionary = ficlVmGetDictionary(vm);

    ficlDictionaryAppendUnsigned(dictionary, ficlInstructionBranchParenWithCheck);
    /* expect "begin" branch marker */
    resolveBackBranch(dictionary, vm, destTag);
    return;
}


/**************************************************************************
                        c h a r   &   f r i e n d s
** 6.1.0895 CHAR    CORE ( "<spaces>name" -- char )
** Skip leading space delimiters. Parse name delimited by a space.
** Put the value of its first character onto the stack. 
**
** bracket-char     CORE 
** Interpretation: Interpretation semantics for this word are undefined.
** Compilation: ( "<spaces>name" -- )
** Skip leading space delimiters. Parse name delimited by a space.
** Append the run-time semantics given below to the current definition. 
** Run-time: ( -- char )
** Place char, the value of the first character of name, on the stack. 
**************************************************************************/
static void ficlPrimitiveChar(ficlVm *vm)
{
    ficlString s;

    FICL_STACK_CHECK(vm->dataStack, 0, 1);

    s = ficlVmGetWord(vm);
    ficlStackPushUnsigned(vm->dataStack, (ficlUnsigned)(s.text[0]));
    return;
}

static void ficlPrimitiveCharCoIm(ficlVm *vm)
{
    ficlPrimitiveChar(vm);
    ficlPrimitiveLiteralIm(vm);
    return;
}

/**************************************************************************
                        c h a r P l u s
** char-plus        CORE ( c-addr1 -- c-addr2 )
** Add the size in address units of a character to c-addr1, giving c-addr2. 
**************************************************************************/
static void ficlPrimitiveCharPlus(ficlVm *vm)
{
    char *p;

    FICL_STACK_CHECK(vm->dataStack,1,1);


    p = ficlStackPopPointer(vm->dataStack);
    ficlStackPushPointer(vm->dataStack, p + 1);
    return;
}

/**************************************************************************
                        c h a r s
** chars        CORE ( n1 -- n2 )
** n2 is the size in address units of n1 characters. 
** For most processors, this function can be a no-op. To guarantee
** portability, we'll multiply by sizeof (char).
**************************************************************************/
#if defined (_M_IX86)
#pragma warning(disable: 4127)
#endif
static void ficlPrimitiveChars(ficlVm *vm)
{
    if (sizeof (char) > 1)
    {
        ficlInteger i;

        FICL_STACK_CHECK(vm->dataStack,1,1);

        i = ficlStackPopInteger(vm->dataStack);
        ficlStackPushInteger(vm->dataStack, i * sizeof (char));
    }
    /* otherwise no-op! */
    return;
}
#if defined (_M_IX86)
#pragma warning(default: 4127)
#endif
 

/**************************************************************************
                        c o u n t
** COUNT    CORE ( c-addr1 -- c-addr2 u )
** Return the character string specification for the counted string stored
** at c-addr1. c-addr2 is the address of the first character after c-addr1.
** u is the contents of the character at c-addr1, which is the length in
** characters of the string at c-addr2. 
**************************************************************************/
static void ficlPrimitiveCount(ficlVm *vm)
{
    ficlCountedString *counted;

    FICL_STACK_CHECK(vm->dataStack,1,2);


    counted = ficlStackPopPointer(vm->dataStack);
    ficlStackPushPointer(vm->dataStack, counted->text);
    ficlStackPushUnsigned(vm->dataStack, counted->length);
    return;
}

/**************************************************************************
                        e n v i r o n m e n t ?
** environment-query CORE ( c-addr u -- FICL_FALSE | i*x FICL_TRUE )
** c-addr is the address of a character string and u is the string's
** character count. u may have a value in the range from zero to an
** implementation-defined maximum which shall not be less than 31. The
** character string should contain a keyword from 3.2.6 Environmental
** queries or the optional word sets to be checked for correspondence
** with an attribute of the present environment. If the system treats the
** attribute as unknown, the returned flag is FICL_FALSE; otherwise, the flag
** is FICL_TRUE and the i*x returned is of the type specified in the table for
** the attribute queried. 
**************************************************************************/
static void ficlPrimitiveEnvironmentQ(ficlVm *vm)
{
    ficlDictionary *environment;
    ficlWord *word;
    ficlString name;

    FICL_STACK_CHECK(vm->dataStack, 2, 1);


    environment = vm->callback.system->environment;
    name.length = ficlStackPopUnsigned(vm->dataStack);
    name.text    = ficlStackPopPointer(vm->dataStack);

    word = ficlDictionaryLookup(environment, name);

    if (word != NULL)
    {
        ficlVmExecuteWord(vm, word);
        ficlStackPushInteger(vm->dataStack, FICL_TRUE);
    }
    else
    {
        ficlStackPushInteger(vm->dataStack, FICL_FALSE);
    }
    return;
}

/**************************************************************************
                        e v a l u a t e
** EVALUATE CORE ( i*x c-addr u -- j*x )
** Save the current input source specification. Store minus-one (-1) in
** SOURCE-ID if it is present. Make the string described by c-addr and u
** both the input source and input buffer, set >IN to zero, and FICL_VM_STATE_INTERPRET.
** When the parse area is empty, restore the prior input source
** specification. Other stack effects are due to the words EVALUATEd. 
**
**************************************************************************/
static void ficlPrimitiveEvaluate(ficlVm *vm)
{
    ficlCell id;
    int result;
	ficlString string;

    FICL_STACK_CHECK(vm->dataStack,2,0);


    FICL_STRING_SET_LENGTH(string, ficlStackPopUnsigned(vm->dataStack));
    FICL_STRING_SET_POINTER(string, ficlStackPopPointer(vm->dataStack));

    id = vm->sourceId;
    vm->sourceId.i = -1;
    result = ficlVmExecuteString(vm, string);
    vm->sourceId = id;
    if (result != FICL_VM_STATUS_OUT_OF_TEXT)
        ficlVmThrow(vm, result);

    return;
}


/**************************************************************************
                        s t r i n g   q u o t e
** Interpreting: get string delimited by a quote from the input stream,
** copy to a scratch area, and put its count and address on the stack.
** Compiling: FICL_VM_STATE_COMPILE code to push the address and count of a string
** literal, FICL_VM_STATE_COMPILE the string from the input stream, and align the dictionary
** pointer.
**************************************************************************/
static void ficlPrimitiveStringQuoteIm(ficlVm *vm)
{
    ficlDictionary *dictionary = ficlVmGetDictionary(vm);

    if (vm->state == FICL_VM_STATE_INTERPRET)
    {
        ficlCountedString *counted = (ficlCountedString *)dictionary->here;
        ficlVmGetString(vm, counted, '\"');
        ficlStackPushPointer(vm->dataStack, counted->text);
        ficlStackPushUnsigned(vm->dataStack, counted->length);
    }
    else    /* FICL_VM_STATE_COMPILE state */
    {
	    ficlDictionaryAppendUnsigned(dictionary, ficlInstructionStringLiteralParen);
        dictionary->here = FICL_POINTER_TO_CELL(ficlVmGetString(vm, (ficlCountedString *)dictionary->here, '\"'));
        ficlDictionaryAlign(dictionary);
    }

    return;
}


/**************************************************************************
                        t y p e
** Pop count and char address from stack and print the designated string.
**************************************************************************/
static void ficlPrimitiveType(ficlVm *vm)
{
    ficlUnsigned length;
    char *s;

    FICL_STACK_CHECK(vm->dataStack, 2, 0);


    length = ficlStackPopUnsigned(vm->dataStack);
    s = ficlStackPopPointer(vm->dataStack);
	
	if ((s == NULL) || (length == 0))
		return;

    /* 
    ** Since we don't have an output primitive for a counted string
    ** (oops), make sure the string is null terminated. If not, copy
    ** and terminate it.
    */
    if (s[length] != 0)
    {
        char *here = (char *)ficlVmGetDictionary(vm)->here;
        if (s != here)
            strncpy(here, s, length);

        here[length] = '\0';
        s = here;
    }

    ficlVmTextOut(vm, s);
    return;
}

/**************************************************************************
                        w o r d
** word CORE ( char "<chars>ccc<char>" -- c-addr )
** Skip leading delimiters. Parse characters ccc delimited by char. An
** ambiguous condition exists if the length of the parsed string is greater
** than the implementation-defined length of a counted string. 
** 
** c-addr is the address of a transient region containing the parsed word
** as a counted string. If the parse area was empty or contained no
** characters other than the delimiter, the resulting string has a zero
** length. A space, not included in the length, follows the string. A
** program may replace characters within the string. 
** NOTE! Ficl also NULL-terminates the dest string.
**************************************************************************/
static void ficlPrimitiveWord(ficlVm *vm)
{
    ficlCountedString *counted;
    char delim;
    ficlString name;

    FICL_STACK_CHECK(vm->dataStack, 1, 1);


    counted = (ficlCountedString *)vm->pad;
    delim = (char)ficlStackPopInteger(vm->dataStack);
    name = ficlVmParseStringEx(vm, delim, 1);

    if (FICL_STRING_GET_LENGTH(name) > FICL_PAD_SIZE - 1)
        FICL_STRING_SET_LENGTH(name, FICL_PAD_SIZE - 1);

    counted->length = (ficlUnsigned8)FICL_STRING_GET_LENGTH(name);
    strncpy(counted->text, FICL_STRING_GET_POINTER(name), FICL_STRING_GET_LENGTH(name));

	/* store an extra space at the end of the primitive... why? dunno yet.  Guy Carver did it. */
    counted->text[counted->length] = ' ';
    counted->text[counted->length + 1] = 0;

    ficlStackPushPointer(vm->dataStack, counted);
    return;
}


/**************************************************************************
                        p a r s e - w o r d
** Ficl   PARSE-WORD  ( <spaces>name -- c-addr u )
** Skip leading spaces and parse name delimited by a space. c-addr is the
** address within the input buffer and u is the length of the selected 
** string. If the parse area is empty, the resulting string has a zero length.
**************************************************************************/
static void ficlPrimitiveParseNoCopy(ficlVm *vm)
{
    ficlString s;

    FICL_STACK_CHECK(vm->dataStack, 0, 2);


    s = ficlVmGetWord0(vm);
    ficlStackPushPointer(vm->dataStack, FICL_STRING_GET_POINTER(s));
    ficlStackPushUnsigned(vm->dataStack, FICL_STRING_GET_LENGTH(s));
    return;
}


/**************************************************************************
                        p a r s e
** CORE EXT  ( char "ccc<char>" -- c-addr u )
** Parse ccc delimited by the delimiter char. 
** c-addr is the address (within the input buffer) and u is the length of 
** the parsed string. If the parse area was empty, the resulting string has
** a zero length. 
** NOTE! PARSE differs from WORD: it does not skip leading delimiters.
**************************************************************************/
static void ficlPrimitiveParse(ficlVm *vm)
{
    ficlString s;
    char delim;


    FICL_STACK_CHECK(vm->dataStack, 1, 2);


    delim = (char)ficlStackPopInteger(vm->dataStack);

    s = ficlVmParseStringEx(vm, delim, 0);
    ficlStackPushPointer(vm->dataStack, FICL_STRING_GET_POINTER(s));
    ficlStackPushUnsigned(vm->dataStack, FICL_STRING_GET_LENGTH(s));
    return;
}


/**************************************************************************
                        f i n d
** FIND CORE ( c-addr -- c-addr 0  |  xt 1  |  xt -1 )
** Find the definition named in the counted string at c-addr. If the
** definition is not found, return c-addr and zero. If the definition is
** found, return its execution token xt. If the definition is immediate,
** also return one (1), otherwise also return minus-one (-1). For a given
** string, the values returned by FIND while compiling may differ from
** those returned while not compiling. 
**************************************************************************/
static void do_find(ficlVm *vm, ficlString name, void *returnForFailure)
{
    ficlWord *word;

    word = ficlDictionaryLookup(ficlVmGetDictionary(vm), name);
    if (word)
    {
        ficlStackPushPointer(vm->dataStack, word);
        ficlStackPushInteger(vm->dataStack, (ficlWordIsImmediate(word) ? 1 : -1));
    }
    else
    {
        ficlStackPushPointer(vm->dataStack, returnForFailure);
        ficlStackPushUnsigned(vm->dataStack, 0);
    }
    return;
}



/**************************************************************************
                        f i n d
** FIND CORE ( c-addr -- c-addr 0  |  xt 1  |  xt -1 )
** Find the definition named in the counted string at c-addr. If the
** definition is not found, return c-addr and zero. If the definition is
** found, return its execution token xt. If the definition is immediate,
** also return one (1), otherwise also return minus-one (-1). For a given
** string, the values returned by FIND while compiling may differ from
** those returned while not compiling. 
**************************************************************************/
static void ficlPrimitiveCFind(ficlVm *vm)
{
    ficlCountedString *counted;
    ficlString name;


    FICL_STACK_CHECK(vm->dataStack, 1, 2);

    counted = ficlStackPopPointer(vm->dataStack);
    FICL_STRING_SET_FROM_COUNTED_STRING(name, *counted);
    do_find(vm, name, counted);
}



/**************************************************************************
                        s f i n d
** Ficl   ( c-addr u -- 0 0  |  xt 1  |  xt -1 )
** Like FIND, but takes "c-addr u" for the string.
**************************************************************************/
static void ficlPrimitiveSFind(ficlVm *vm)
{
    ficlString name;


    FICL_STACK_CHECK(vm->dataStack, 2, 2);


    name.length = ficlStackPopInteger(vm->dataStack);
    name.text = ficlStackPopPointer(vm->dataStack);

    do_find(vm, name, NULL);
}



/**************************************************************************
                        r e c u r s e
** 
**************************************************************************/
static void ficlPrimitiveRecurseCoIm(ficlVm *vm)
{
    ficlDictionary *dictionary = ficlVmGetDictionary(vm);

    FICL_IGNORE(vm);
    ficlDictionaryAppendCell(dictionary, FICL_LVALUE_TO_CELL(dictionary->smudge));
    return;
}


/**************************************************************************
                        s o u r c e
** CORE ( -- c-addr u )
** c-addr is the address of, and u is the number of characters in, the
** input buffer. 
**************************************************************************/
static void ficlPrimitiveSource(ficlVm *vm)
{

    FICL_STACK_CHECK(vm->dataStack,0,2);

    ficlStackPushPointer(vm->dataStack, vm->tib.text);
    ficlStackPushInteger(vm->dataStack, ficlVmGetInBufLen(vm));
    return;
}


/**************************************************************************
                        v e r s i o n
** non-standard...
**************************************************************************/
static void ficlPrimitiveVersion(ficlVm *vm)
{
    ficlVmTextOut(vm, "Ficl version " FICL_VERSION "\n");
    return;
}


/**************************************************************************
                        t o I n
** to-in CORE
**************************************************************************/
static void ficlPrimitiveToIn(ficlVm *vm)
{

    FICL_STACK_CHECK(vm->dataStack,0,1);

    ficlStackPushPointer(vm->dataStack, &vm->tib.index);
    return;
}


/**************************************************************************
                        c o l o n N o N a m e
** CORE EXT ( C:  -- colon-sys )  ( S:  -- xt )
** Create an unnamed colon definition and push its address.
** Change state to FICL_VM_STATE_COMPILE.
**************************************************************************/
static void ficlPrimitiveColonNoName(ficlVm *vm)
{
    ficlDictionary *dictionary = ficlVmGetDictionary(vm);
    ficlWord *word;
    ficlString name;

    FICL_STRING_SET_LENGTH(name, 0);
    FICL_STRING_SET_POINTER(name, NULL);

    vm->state = FICL_VM_STATE_COMPILE;
    word = ficlDictionaryAppendWord(dictionary, name, (ficlPrimitive)ficlInstructionColonParen, FICL_WORD_DEFAULT | FICL_WORD_SMUDGED);
    ficlStackPushPointer(vm->dataStack, word);
    markControlTag(vm, colonTag);
    return;
}


/**************************************************************************
                        u s e r   V a r i a b l e
** user  ( u -- )  "<spaces>name"  
** Get a name from the input stream and create a user variable
** with the name and the index supplied. The run-time effect
** of a user variable is to push the address of the indexed ficlCell
** in the running vm's user array. 
**
** User variables are vm local cells. Each vm has an array of
** FICL_USER_CELLS of them when FICL_WANT_USER is nonzero.
** Ficl's user facility is implemented with two primitives,
** "user" and "(user)", a variable ("nUser") (in softcore.c) that 
** holds the index of the next free user ficlCell, and a redefinition
** (also in softcore) of "user" that defines a user word and increments
** nUser.
**************************************************************************/
#if FICL_WANT_USER
static void ficlPrimitiveUser(ficlVm *vm)
{
    ficlDictionary *dictionary = ficlVmGetDictionary(vm);
    ficlString name = ficlVmGetWord(vm);
    ficlCell c;

    c = ficlStackPop(vm->dataStack);
    if (c.i >= FICL_USER_CELLS)
    {
        ficlVmThrowError(vm, "Error - out of user space");
    }

    ficlDictionaryAppendWord(dictionary, name, (ficlPrimitive)ficlInstructionUserParen, FICL_WORD_DEFAULT);
    ficlDictionaryAppendCell(dictionary, c);
    return;
}
#endif


#if FICL_WANT_LOCALS
/*
** Each local is recorded in a private locals dictionary as a 
** word that does doLocalIm at runtime. DoLocalIm compiles code
** into the client definition to fetch the value of the 
** corresponding local variable from the return stack.
** The private dictionary gets initialized at the end of each block
** that uses locals (in ; and does> for example).
*/
void ficlLocalParenIm(ficlVm *vm, int isDouble, int isFloat)
{
    ficlDictionary *dictionary = ficlVmGetDictionary(vm);
    ficlInteger nLocal = vm->runningWord->param[0].i;

#if !FICL_WANT_FLOAT
	FICL_VM_ASSERT(vm, !isFloat);
	/* get rid of unused parameter warning */
	isFloat = 0;
#endif /* FICL_WANT_FLOAT */

    if (vm->state == FICL_VM_STATE_INTERPRET)
    {
		ficlStack *stack;
#if FICL_WANT_FLOAT
		if (isFloat)
			stack = vm->floatStack;
		else
#endif /* FICL_WANT_FLOAT */
		stack = vm->dataStack;

        ficlStackPush(stack, vm->returnStack->frame[nLocal]);
		if (isDouble)
            ficlStackPush(stack, vm->returnStack->frame[nLocal+1]);
    }
    else
    {
		ficlInstruction instruction;
		ficlInteger appendLocalOffset;
#if FICL_WANT_FLOAT
        if (isFloat)
		{
			instruction = (isDouble) ? ficlInstructionGetF2LocalParen : ficlInstructionGetFLocalParen;
			appendLocalOffset = FICL_TRUE;
		}
		else
#endif /* FICL_WANT_FLOAT */
		if (nLocal == 0)
		{
			instruction = (isDouble) ? ficlInstructionGet2Local0 : ficlInstructionGetLocal0;
			appendLocalOffset = FICL_FALSE;
		}
		else if ((nLocal == 1) && !isDouble)
		{
			instruction = ficlInstructionGetLocal1;
			appendLocalOffset = FICL_FALSE;
		}
		else
		{
			instruction = (isDouble) ? ficlInstructionGet2LocalParen : ficlInstructionGetLocalParen;
			appendLocalOffset = FICL_TRUE;
		}

		ficlDictionaryAppendUnsigned(dictionary, instruction);
		if (appendLocalOffset)
			ficlDictionaryAppendCell(dictionary, FICL_LVALUE_TO_CELL(nLocal));
    }
    return;
}

static void ficlPrimitiveDoLocalIm(ficlVm *vm)
{
    ficlLocalParenIm(vm, 0, 0);
}

static void ficlPrimitiveDo2LocalIm(ficlVm *vm)
{
    ficlLocalParenIm(vm, 1, 0);
}

#if FICL_WANT_FLOAT
static void ficlPrimitiveDoFLocalIm(ficlVm *vm)
{
    ficlLocalParenIm(vm, 0, 1);
}

static void ficlPrimitiveDoF2LocalIm(ficlVm *vm)
{
    ficlLocalParenIm(vm, 1, 1);
}
#endif /* FICL_WANT_FLOAT */



/**************************************************************************
                        l o c a l P a r e n
** paren-local-paren LOCAL 
** Interpretation: Interpretation semantics for this word are undefined.
** Execution: ( c-addr u -- )
** When executed during compilation, (LOCAL) passes a message to the 
** system that has one of two meanings. If u is non-zero,
** the message identifies a new local whose definition name is given by
** the string of characters identified by c-addr u. If u is zero,
** the message is last local and c-addr has no significance. 
**
** The result of executing (LOCAL) during compilation of a definition is
** to create a set of named local identifiers, each of which is
** a definition name, that only have execution semantics within the scope
** of that definition's source. 
**
** local Execution: ( -- x )
**
** Push the local's value, x, onto the stack. The local's value is
** initialized as described in 13.3.3 Processing locals and may be
** changed by preceding the local's name with TO. An ambiguous condition
** exists when local is executed while in interpretation state. 
**************************************************************************/
void ficlLocalParen(ficlVm *vm, int isDouble, int isFloat)
{
    ficlDictionary *dictionary;
    ficlString name;

    FICL_STACK_CHECK(vm->dataStack,2,0);  


    dictionary = ficlVmGetDictionary(vm);
    FICL_STRING_SET_LENGTH(name, ficlStackPopUnsigned(vm->dataStack));
    FICL_STRING_SET_POINTER(name, (char *)ficlStackPopPointer(vm->dataStack));

    if (FICL_STRING_GET_LENGTH(name) > 0)
    {   /* add a local to the **locals** dictionary and update localsCount */
        ficlPrimitive code;
		ficlInstruction instruction;
        ficlDictionary *locals = ficlSystemGetLocals(vm->callback.system);
        if (vm->callback.system->localsCount >= FICL_MAX_LOCALS)
        {
            ficlVmThrowError(vm, "Error: out of local space");
        }

#if !FICL_WANT_FLOAT
		FICL_VM_ASSERT(vm, !isFloat);
		/* get rid of unused parameter warning */
		isFloat = 0;
#else /* FICL_WANT_FLOAT */
		if (isFloat)
		{
			if (isDouble)
			{
				code = ficlPrimitiveDoF2LocalIm;
				instruction = ficlInstructionToF2LocalParen;
			}
			else
			{
				code = ficlPrimitiveDoFLocalIm;
				instruction = ficlInstructionToFLocalParen;
			}
		}
		else
#endif /* FICL_WANT_FLOAT */
        if (isDouble)
		{
			code = ficlPrimitiveDo2LocalIm;
			instruction = ficlInstructionTo2LocalParen;
		}
		else
		{
			code = ficlPrimitiveDoLocalIm;
			instruction = ficlInstructionToLocalParen;
		}

        ficlDictionaryAppendWord(locals, name, code, FICL_WORD_COMPILE_ONLY_IMMEDIATE);
        ficlDictionaryAppendCell(locals,  FICL_LVALUE_TO_CELL(vm->callback.system->localsCount));

        if (vm->callback.system->localsCount == 0)
        {   /* FICL_VM_STATE_COMPILE code to create a local stack frame */
            ficlDictionaryAppendUnsigned(dictionary, ficlInstructionLinkParen);
            /* save location in dictionary for #locals */
            vm->callback.system->localsFixup = dictionary->here;
            ficlDictionaryAppendCell(dictionary, FICL_LVALUE_TO_CELL(vm->callback.system->localsCount));
        }

        ficlDictionaryAppendUnsigned(dictionary, instruction);
        ficlDictionaryAppendCell(dictionary, FICL_LVALUE_TO_CELL(vm->callback.system->localsCount));

        vm->callback.system->localsCount += (isDouble) ? 2 : 1;
    }
    else if (vm->callback.system->localsCount > 0)
    {
        /* write localsCount to (link) param area in dictionary */
        *(ficlInteger *)(vm->callback.system->localsFixup) = vm->callback.system->localsCount;
    }

    return;
}


static void ficlPrimitiveLocalParen(ficlVm *vm)
{
   ficlLocalParen(vm, 0, 0);
}

static void ficlPrimitive2LocalParen(ficlVm *vm)
{
   ficlLocalParen(vm, 1, 0);
}


#endif /* FICL_WANT_LOCALS */


/**************************************************************************
                        t o V a l u e
** CORE EXT 
** Interpretation: ( x "<spaces>name" -- )
** Skip leading spaces and parse name delimited by a space. Store x in 
** name. An ambiguous condition exists if name was not defined by VALUE. 
** NOTE: In Ficl, VALUE is an alias of CONSTANT
**************************************************************************/
static void ficlPrimitiveToValue(ficlVm *vm)
{
    ficlString name = ficlVmGetWord(vm);
    ficlDictionary *dictionary = ficlVmGetDictionary(vm);
    ficlWord *word;
	ficlInstruction instruction = 0;
	ficlStack *stack;
	ficlInteger isDouble;
#if FICL_WANT_LOCALS
	ficlInteger nLocal;
	ficlInteger appendLocalOffset;
	ficlInteger isFloat;
#endif /* FICL_WANT_LOCALS */

#if FICL_WANT_LOCALS
    if ((vm->callback.system->localsCount > 0) && (vm->state == FICL_VM_STATE_COMPILE))
    {
        ficlDictionary *locals;

        locals = ficlSystemGetLocals(vm->callback.system);
        word = ficlDictionaryLookup(locals, name);
		if (!word)
			goto TO_GLOBAL;

		if (word->code == ficlPrimitiveDoLocalIm)
		{
			instruction = ficlInstructionToLocalParen;
			isDouble = isFloat = FICL_FALSE;
		}
		else if (word->code == ficlPrimitiveDo2LocalIm)
		{
			instruction = ficlInstructionTo2LocalParen;
			isDouble = FICL_TRUE;
			isFloat = FICL_FALSE;
		}
#if FICL_WANT_FLOAT
		else if (word->code == ficlPrimitiveDoFLocalIm)
		{
			instruction = ficlInstructionToFLocalParen;
			isDouble = FICL_FALSE;
			isFloat = FICL_TRUE;
		}
		else if (word->code == ficlPrimitiveDoF2LocalIm)
		{
			instruction = ficlInstructionToF2LocalParen;
			isDouble = isFloat = FICL_TRUE;
		}
#endif /* FICL_WANT_FLOAT */
		else
		{
			ficlVmThrowError(vm, "to %.*s : local is of unknown type", FICL_STRING_GET_LENGTH(name), FICL_STRING_GET_POINTER(name));
			return;
		}

		nLocal = word->param[0].i;
		appendLocalOffset = FICL_TRUE;

#if FICL_WANT_FLOAT
		if (!isFloat)
		{
#endif /* FICL_WANT_FLOAT */
		if (nLocal == 0)
		{
			instruction = (isDouble) ? ficlInstructionTo2Local0 : ficlInstructionToLocal0;
			appendLocalOffset = FICL_FALSE;
		}
		else if ((nLocal == 1) && !isDouble)
		{
			instruction = ficlInstructionToLocal1;
			appendLocalOffset = FICL_FALSE;
		}
#if FICL_WANT_FLOAT
		}
#endif /* FICL_WANT_FLOAT */
		
        ficlDictionaryAppendUnsigned(dictionary, instruction);
		if (appendLocalOffset)
			ficlDictionaryAppendCell(dictionary, FICL_LVALUE_TO_CELL(nLocal));
        return;
    }
#endif

#if FICL_WANT_LOCALS
TO_GLOBAL:
#endif /* FICL_WANT_LOCALS */
    word = ficlDictionaryLookup(dictionary, name);
    if (!word)
        ficlVmThrowError(vm, "%.*s not found", FICL_STRING_GET_LENGTH(name), FICL_STRING_GET_POINTER(name));

	switch ((ficlInstruction)word->code)
	{
		case ficlInstructionConstantParen:
			instruction = ficlInstructionStore;
			stack = vm->dataStack;
			isDouble = FICL_FALSE;
			break;
		case ficlInstruction2ConstantParen:
			instruction = ficlInstruction2Store;
			stack = vm->dataStack;
			isDouble = FICL_TRUE;
			break;
#if FICL_WANT_FLOAT
		case ficlInstructionFConstantParen:
			instruction = ficlInstructionFStore;
			stack = vm->floatStack;
			isDouble = FICL_FALSE;
			break;
		case ficlInstructionF2ConstantParen:
			instruction = ficlInstructionF2Store;
			stack = vm->floatStack;
			isDouble = FICL_TRUE;
			break;
#endif /* FICL_WANT_FLOAT */
		default:
		{
		    ficlVmThrowError(vm, "to %.*s : value/constant is of unknown type", FICL_STRING_GET_LENGTH(name), FICL_STRING_GET_POINTER(name));
		    return;
		}
	}
	
    if (vm->state == FICL_VM_STATE_INTERPRET)
	{
        word->param[0] = ficlStackPop(stack);
		if (isDouble)
			word->param[1] = ficlStackPop(stack);
	}
    else        /* FICL_VM_STATE_COMPILE code to store to word's param */
    {
        ficlStackPushPointer(vm->dataStack, &word->param[0]);
        ficlPrimitiveLiteralIm(vm);
        ficlDictionaryAppendUnsigned(dictionary, instruction);
    }
    return;
}


/**************************************************************************
						f m S l a s h M o d
** f-m-slash-mod CORE ( d1 n1 -- n2 n3 )
** Divide d1 by n1, giving the floored quotient n3 and the remainder n2.
** Input and output stack arguments are signed. An ambiguous condition
** exists if n1 is zero or if the quotient lies outside the range of a
** single-ficlCell signed integer. 
**************************************************************************/
static void ficlPrimitiveFMSlashMod(ficlVm *vm)
{
	ficl2Integer d1;
	ficlInteger n1;
	ficl2IntegerQR qr;

	FICL_STACK_CHECK(vm->dataStack, 3, 2);

	n1 = ficlStackPopInteger(vm->dataStack);
	d1 = ficlStackPop2Integer(vm->dataStack);
	qr = ficl2IntegerDivideFloored(d1, n1);
	ficlStackPushInteger(vm->dataStack, qr.remainder);
	ficlStackPushInteger(vm->dataStack, FICL_2UNSIGNED_GET_LOW(qr.quotient));
	return;
}


/**************************************************************************
						s m S l a s h R e m
** s-m-slash-remainder CORE ( d1 n1 -- n2 n3 )
** Divide d1 by n1, giving the symmetric quotient n3 and the remainder n2.
** Input and output stack arguments are signed. An ambiguous condition
** exists if n1 is zero or if the quotient lies outside the range of a
** single-ficlCell signed integer. 
**************************************************************************/
static void ficlPrimitiveSMSlashRem(ficlVm *vm)
{
	ficl2Integer d1;
	ficlInteger n1;
	ficl2IntegerQR qr;
	
	FICL_STACK_CHECK(vm->dataStack, 3, 2);

	n1 = ficlStackPopInteger(vm->dataStack);
	d1 = ficlStackPop2Integer(vm->dataStack);
	qr = ficl2IntegerDivideSymmetric(d1, n1);
	ficlStackPushInteger(vm->dataStack, qr.remainder);
	ficlStackPushInteger(vm->dataStack, FICL_2UNSIGNED_GET_LOW(qr.quotient));
	return;
}


static void ficlPrimitiveMod(ficlVm *vm)
{
	ficl2Integer d1;
	ficlInteger n1;
	ficlInteger i;
	ficl2IntegerQR qr;
	FICL_STACK_CHECK(vm->dataStack, 2, 1);

	n1 = ficlStackPopInteger(vm->dataStack);
	i = ficlStackPopInteger(vm->dataStack);
	FICL_INTEGER_TO_2INTEGER(i, d1);
	qr = ficl2IntegerDivideSymmetric(d1, n1);
	ficlStackPushInteger(vm->dataStack, qr.remainder);
	return;
}


/**************************************************************************
						u m S l a s h M o d
** u-m-slash-mod CORE ( ud u1 -- u2 u3 )
** Divide ud by u1, giving the quotient u3 and the remainder u2.
** All values and arithmetic are unsigned. An ambiguous condition
** exists if u1 is zero or if the quotient lies outside the range of a
** single-ficlCell unsigned integer. 
*************************************************************************/
static void ficlPrimitiveUMSlashMod(ficlVm *vm)
{
	ficl2Unsigned ud;
	ficlUnsigned u1;
	ficl2UnsignedQR uqr;

	u1    = ficlStackPopUnsigned(vm->dataStack);
	ud    = ficlStackPop2Unsigned(vm->dataStack);
	uqr   = ficl2UnsignedDivide(ud, u1);
	ficlStackPushUnsigned(vm->dataStack, uqr.remainder);
	ficlStackPushUnsigned(vm->dataStack, FICL_2UNSIGNED_GET_LOW(uqr.quotient));
	return;
}



/**************************************************************************
						m S t a r
** m-star CORE ( n1 n2 -- d )
** d is the signed product of n1 times n2. 
**************************************************************************/
static void ficlPrimitiveMStar(ficlVm *vm)
{
	ficlInteger n2;
	ficlInteger n1;
	ficl2Integer d;
	FICL_STACK_CHECK(vm->dataStack, 2, 2);

	n2 = ficlStackPopInteger(vm->dataStack);
	n1 = ficlStackPopInteger(vm->dataStack);

	d = ficl2IntegerMultiply(n1, n2);
	ficlStackPush2Integer(vm->dataStack, d);
	return;
}


static void ficlPrimitiveUMStar(ficlVm *vm)
{
	ficlUnsigned u2;
	ficlUnsigned u1;
	ficl2Unsigned ud;
	FICL_STACK_CHECK(vm->dataStack, 2, 2);

	u2 = ficlStackPopUnsigned(vm->dataStack);
	u1 = ficlStackPopUnsigned(vm->dataStack);

	ud = ficl2UnsignedMultiply(u1, u2);
	ficlStackPush2Unsigned(vm->dataStack, ud);
	return;
}


/**************************************************************************
						d n e g a t e
** DOUBLE   ( d1 -- d2 )
** d2 is the negation of d1. 
**************************************************************************/
static void ficlPrimitiveDNegate(ficlVm *vm)
{
	ficl2Integer i = ficlStackPop2Integer(vm->dataStack);
	i = ficl2IntegerNegate(i);
	ficlStackPush2Integer(vm->dataStack, i);

	return;
}




/**************************************************************************
                        p a d
** CORE EXT  ( -- c-addr )
** c-addr is the address of a transient region that can be used to hold
** data for intermediate processing.
**************************************************************************/
static void ficlPrimitivePad(ficlVm *vm)
{
    ficlStackPushPointer(vm->dataStack, vm->pad);
}


/**************************************************************************
                        s o u r c e - i d
** CORE EXT, FILE   ( -- 0 | -1 | fileid )
**    Identifies the input source as follows:
**
** SOURCE-ID       Input source
** ---------       ------------
** fileid          Text file fileid
** -1              String (via EVALUATE)
** 0               User input device
**************************************************************************/
static void ficlPrimitiveSourceID(ficlVm *vm)
{
    ficlStackPushInteger(vm->dataStack, vm->sourceId.i);
    return;
}


/**************************************************************************
                        r e f i l l
** CORE EXT   ( -- flag )
** Attempt to fill the input buffer from the input source, returning a FICL_TRUE
** flag if successful. 
** When the input source is the user input device, attempt to receive input
** into the terminal input buffer. If successful, make the result the input
** buffer, set >IN to zero, and return FICL_TRUE. Receipt of a line containing no
** characters is considered successful. If there is no input available from
** the current input source, return FICL_FALSE. 
** When the input source is a string from EVALUATE, return FICL_FALSE and
** perform no other action. 
**************************************************************************/
static void ficlPrimitiveRefill(ficlVm *vm)
{
    ficlInteger ret = (vm->sourceId.i == -1) ? FICL_FALSE : FICL_TRUE;
    if (ret && (vm->restart == 0))
        ficlVmThrow(vm, FICL_VM_STATUS_RESTART);

    ficlStackPushInteger(vm->dataStack, ret);
    return;
}


/**************************************************************************
                        freebsd exception handling words
** Catch, from ANS Forth standard. Installs a safety net, then EXECUTE
** the word in ToS. If an exception happens, restore the state to what
** it was before, and pushes the exception value on the stack. If not,
** push zero.
**
** Notice that Catch implements an inner interpreter. This is ugly,
** but given how Ficl works, it cannot be helped. The problem is that
** colon definitions will be executed *after* the function returns,
** while "code" definitions will be executed immediately. I considered
** other solutions to this problem, but all of them shared the same
** basic problem (with added disadvantages): if Ficl ever changes it's
** inner thread modus operandi, one would have to fix this word.
**
** More comments can be found throughout catch's code.
**
** Daniel C. Sobral Jan 09/1999
** sadler may 2000 -- revised to follow ficl.c:ficlExecXT.
**************************************************************************/

static void ficlPrimitiveCatch(ficlVm *vm)
{
    int         except;
    jmp_buf     vmState;
    ficlVm     vmCopy;
    ficlStack  dataStackCopy;
    ficlStack  returnStackCopy;
    ficlWord   *word;

    FICL_VM_ASSERT(vm, vm);
    FICL_VM_ASSERT(vm, vm->callback.system->exitInnerWord);
    

    /*
    ** Get xt.
    ** We need this *before* we save the stack pointer, or
    ** we'll have to pop one element out of the stack after
    ** an exception. I prefer to get done with it up front. :-)
    */

    FICL_STACK_CHECK(vm->dataStack, 1, 0);

    word = ficlStackPopPointer(vm->dataStack);

    /* 
    ** Save vm's state -- a catch will not back out environmental
    ** changes.
    **
    ** We are *not* saving dictionary state, since it is
    ** global instead of per vm, and we are not saving
    ** stack contents, since we are not required to (and,
    ** thus, it would be useless). We save vm, and vm
    ** "stacks" (a structure containing general information
    ** about it, including the current stack pointer).
    */
    memcpy((void*)&vmCopy, (void*)vm, sizeof(ficlVm));
    memcpy((void*)&dataStackCopy, (void*)vm->dataStack, sizeof(ficlStack));
    memcpy((void*)&returnStackCopy, (void*)vm->returnStack, sizeof(ficlStack));

    /*
    ** Give vm a jmp_buf
    */
    vm->exceptionHandler = &vmState;

    /*
    ** Safety net
    */
    except = setjmp(vmState);

    switch (except)
    {
        /*
        ** Setup condition - push poison pill so that the VM throws
        ** VM_INNEREXIT if the XT terminates normally, then execute
        ** the XT
        */
    case 0:
        ficlVmPushIP(vm, &(vm->callback.system->exitInnerWord));          /* Open mouth, insert emetic */
        ficlVmExecuteWord(vm, word);
        ficlVmInnerLoop(vm, 0);
        break;

        /*
        ** Normal exit from XT - lose the poison pill, 
        ** restore old setjmp vector and push a zero. 
        */
    case FICL_VM_STATUS_INNER_EXIT:
        ficlVmPopIP(vm);                   /* Gack - hurl poison pill */
        vm->exceptionHandler = vmCopy.exceptionHandler;        /* Restore just the setjmp vector */
        ficlStackPushInteger(vm->dataStack, 0);   /* Push 0 -- everything is ok */
        break;

        /*
        ** Some other exception got thrown - restore pre-existing VM state
        ** and push the exception code
        */
    default:
        /* Restore vm's state */
        memcpy((void*)vm, (void*)&vmCopy, sizeof(ficlVm));
        memcpy((void*)vm->dataStack, (void*)&dataStackCopy, sizeof(ficlStack));
        memcpy((void*)vm->returnStack, (void*)&returnStackCopy, sizeof(ficlStack));

        ficlStackPushInteger(vm->dataStack, except);/* Push error */
        break;
    }
}

/**************************************************************************
**                     t h r o w
** EXCEPTION
** Throw --  From ANS Forth standard.
**
** Throw takes the ToS and, if that's different from zero,
** returns to the last executed catch context. Further throws will
** unstack previously executed "catches", in LIFO mode.
**
** Daniel C. Sobral Jan 09/1999
**************************************************************************/
static void ficlPrimitiveThrow(ficlVm *vm)
{
    int except;
    
    except = ficlStackPopInteger(vm->dataStack);

    if (except)
        ficlVmThrow(vm, except);
}


/**************************************************************************
**                     a l l o c a t e
** MEMORY
**************************************************************************/
static void ficlPrimitiveAllocate(ficlVm *vm)
{
    size_t size;
    void *p;

    size = ficlStackPopInteger(vm->dataStack);
    p = ficlMalloc(size);
    ficlStackPushPointer(vm->dataStack, p);
    if (p)
        ficlStackPushInteger(vm->dataStack, 0);
    else
        ficlStackPushInteger(vm->dataStack, 1);
}


/**************************************************************************
**                     f r e e 
** MEMORY
**************************************************************************/
static void ficlPrimitiveFree(ficlVm *vm)
{
    void *p;

    p = ficlStackPopPointer(vm->dataStack);
    ficlFree(p);
    ficlStackPushInteger(vm->dataStack, 0);
}


/**************************************************************************
**                     r e s i z e
** MEMORY
**************************************************************************/
static void ficlPrimitiveResize(ficlVm *vm)
{
    size_t size;
    void *new, *old;

    size = ficlStackPopInteger(vm->dataStack);
    old = ficlStackPopPointer(vm->dataStack);
    new = ficlRealloc(old, size);
    if (new) 
    {
        ficlStackPushPointer(vm->dataStack, new);
        ficlStackPushInteger(vm->dataStack, 0);
    } 
    else 
    {
        ficlStackPushPointer(vm->dataStack, old);
        ficlStackPushInteger(vm->dataStack, 1);
    }
}


/**************************************************************************
**                     e x i t - i n n e r 
** Signals execXT that an inner loop has completed
**************************************************************************/
static void ficlPrimitiveExitInner(ficlVm *vm)
{
    ficlVmThrow(vm, FICL_VM_STATUS_INNER_EXIT);
}


#if 0
/**************************************************************************
                        
** 
**************************************************************************/
static void ficlPrimitiveName(ficlVm *vm)
{
    FICL_IGNORE(vm);
    return;
}


#endif
/**************************************************************************
                        f i c l C o m p i l e C o r e
** Builds the primitive wordset and the environment-query namespace.
**************************************************************************/

void ficlSystemCompileCore(ficlSystem *system)
{
	ficlWord *interpret;
    ficlDictionary *dictionary = ficlSystemGetDictionary(system);
    ficlDictionary *environment = ficlSystemGetEnvironment(system);

    FICL_SYSTEM_ASSERT(system, dictionary);
    FICL_SYSTEM_ASSERT(system, environment);


	#define FICL_TOKEN(token, description) 
	#define FICL_INSTRUCTION_TOKEN(token, description, flags) ficlDictionarySetInstruction(dictionary, description, token, flags);
	#include "ficltokens.h"
	#undef FICL_TOKEN
	#undef FICL_INSTRUCTION_TOKEN

    /*
    ** The Core word set
    ** see softcore.c for definitions of: abs bl space spaces abort"
    */
    ficlDictionarySetPrimitive(dictionary, "#",         ficlPrimitiveNumberSign,     FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "#>",        ficlPrimitiveNumberSignGreater,FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "#s",        ficlPrimitiveNumberSignS,    FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "\'",        ficlPrimitiveTick,       FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "(",         ficlPrimitiveParenthesis,    FICL_WORD_IMMEDIATE);
    ficlDictionarySetPrimitive(dictionary, "+loop",     ficlPrimitivePlusLoopCoIm,   FICL_WORD_COMPILE_ONLY_IMMEDIATE);
    ficlDictionarySetPrimitive(dictionary, ".",         ficlPrimitiveDot,    FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, ".\"",       ficlPrimitiveDotQuoteCoIm,   FICL_WORD_COMPILE_ONLY_IMMEDIATE);
    ficlDictionarySetPrimitive(dictionary, ":",         ficlPrimitiveColon,          FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, ";",         ficlPrimitiveSemicolonCoIm,  FICL_WORD_COMPILE_ONLY_IMMEDIATE);
    ficlDictionarySetPrimitive(dictionary, "<#",        ficlPrimitiveLessNumberSign, FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, ">body",     ficlPrimitiveToBody,         FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, ">in",       ficlPrimitiveToIn,           FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, ">number",   ficlPrimitiveToNumber,       FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "abort",     ficlPrimitiveAbort,      FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "accept",    ficlPrimitiveAccept,         FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "align",     ficlPrimitiveAlign,          FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "aligned",   ficlPrimitiveAligned,        FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "allot",     ficlPrimitiveAllot,          FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "base",      ficlPrimitiveBase,           FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "begin",     ficlPrimitiveBeginCoIm,      FICL_WORD_COMPILE_ONLY_IMMEDIATE);
    ficlDictionarySetPrimitive(dictionary, "case",      ficlPrimitiveCaseCoIm,       FICL_WORD_COMPILE_ONLY_IMMEDIATE);
    ficlDictionarySetPrimitive(dictionary, "char",      ficlPrimitiveChar,       FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "char+",     ficlPrimitiveCharPlus,       FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "chars",     ficlPrimitiveChars,      FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "constant",  ficlPrimitiveConstant,       FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "count",     ficlPrimitiveCount,          FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "cr",        ficlPrimitiveCR,             FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "create",    ficlPrimitiveCreate,         FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "decimal",   ficlPrimitiveDecimal,        FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "depth",     ficlPrimitiveDepth,          FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "do",        ficlPrimitiveDoCoIm,         FICL_WORD_COMPILE_ONLY_IMMEDIATE);
    ficlDictionarySetPrimitive(dictionary, "does>",     ficlPrimitiveDoesCoIm,       FICL_WORD_COMPILE_ONLY_IMMEDIATE);
    ficlDictionarySetPrimitive(dictionary, "else",      ficlPrimitiveElseCoIm,       FICL_WORD_COMPILE_ONLY_IMMEDIATE);
    ficlDictionarySetPrimitive(dictionary, "emit",      ficlPrimitiveEmit,           FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "endcase",   ficlPrimitiveEndcaseCoIm,    FICL_WORD_COMPILE_ONLY_IMMEDIATE);
    ficlDictionarySetPrimitive(dictionary, "endof",     ficlPrimitiveEndofCoIm,      FICL_WORD_COMPILE_ONLY_IMMEDIATE);
    ficlDictionarySetPrimitive(dictionary, "environment?", ficlPrimitiveEnvironmentQ,FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "evaluate",  ficlPrimitiveEvaluate,       FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "execute",   ficlPrimitiveExecute,        FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "exit",      ficlPrimitiveExitCoIm,       FICL_WORD_COMPILE_ONLY_IMMEDIATE);
    ficlDictionarySetPrimitive(dictionary, "fallthrough",ficlPrimitiveFallthroughCoIm,FICL_WORD_COMPILE_ONLY_IMMEDIATE);
    ficlDictionarySetPrimitive(dictionary, "find",      ficlPrimitiveCFind,          FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "fm/mod",    ficlPrimitiveFMSlashMod, FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "here",      ficlPrimitiveHere,           FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "hold",      ficlPrimitiveHold,           FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "if",        ficlPrimitiveIfCoIm,         FICL_WORD_COMPILE_ONLY_IMMEDIATE);
    ficlDictionarySetPrimitive(dictionary, "immediate", ficlPrimitiveImmediate,      FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "literal",   ficlPrimitiveLiteralIm,      FICL_WORD_IMMEDIATE);
    ficlDictionarySetPrimitive(dictionary, "loop",      ficlPrimitiveLoopCoIm,       FICL_WORD_COMPILE_ONLY_IMMEDIATE);
    ficlDictionarySetPrimitive(dictionary, "m*",        ficlPrimitiveMStar,      FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "mod",       ficlPrimitiveMod,        FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "of",        ficlPrimitiveOfCoIm,         FICL_WORD_COMPILE_ONLY_IMMEDIATE);
    ficlDictionarySetPrimitive(dictionary, "postpone",  ficlPrimitivePostponeCoIm,   FICL_WORD_COMPILE_ONLY_IMMEDIATE);
    ficlDictionarySetPrimitive(dictionary, "quit",      ficlPrimitiveQuit,           FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "recurse",   ficlPrimitiveRecurseCoIm,    FICL_WORD_COMPILE_ONLY_IMMEDIATE);
    ficlDictionarySetPrimitive(dictionary, "repeat",    ficlPrimitiveRepeatCoIm,     FICL_WORD_COMPILE_ONLY_IMMEDIATE);
    ficlDictionarySetPrimitive(dictionary, "s\"",       ficlPrimitiveStringQuoteIm,  FICL_WORD_IMMEDIATE);
    ficlDictionarySetPrimitive(dictionary, "sign",      ficlPrimitiveSign,           FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "sm/rem",    ficlPrimitiveSMSlashRem, FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "source",    ficlPrimitiveSource,         FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "state",     ficlPrimitiveState,          FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "then",      ficlPrimitiveEndifCoIm,      FICL_WORD_COMPILE_ONLY_IMMEDIATE);
    ficlDictionarySetPrimitive(dictionary, "type",      ficlPrimitiveType,           FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "u.",        ficlPrimitiveUDot,           FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "um*",       ficlPrimitiveUMStar,     FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "um/mod",    ficlPrimitiveUMSlashMod, FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "until",     ficlPrimitiveUntilCoIm,      FICL_WORD_COMPILE_ONLY_IMMEDIATE);
    ficlDictionarySetPrimitive(dictionary, "variable",  ficlPrimitiveVariable,       FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "while",     ficlPrimitiveWhileCoIm,      FICL_WORD_COMPILE_ONLY_IMMEDIATE);
    ficlDictionarySetPrimitive(dictionary, "word",      ficlPrimitiveWord,   FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "[",         ficlPrimitiveLeftBracketCoIm,   FICL_WORD_COMPILE_ONLY_IMMEDIATE);
    ficlDictionarySetPrimitive(dictionary, "[\']",      ficlPrimitiveBracketTickCoIm,FICL_WORD_COMPILE_ONLY_IMMEDIATE);
    ficlDictionarySetPrimitive(dictionary, "[char]",    ficlPrimitiveCharCoIm,       FICL_WORD_COMPILE_ONLY_IMMEDIATE);
    ficlDictionarySetPrimitive(dictionary, "]",         ficlPrimitiveRightBracket,       FICL_WORD_DEFAULT);
    /* 
    ** The Core Extensions word set...
    ** see softcore.fr for other definitions
    */
    /* "#tib" */
    ficlDictionarySetPrimitive(dictionary, ".(",        ficlPrimitiveDotParen,       FICL_WORD_IMMEDIATE);
    /* ".r" */
    ficlDictionarySetPrimitive(dictionary, ":noname",   ficlPrimitiveColonNoName,    FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "?do",       ficlPrimitiveQDoCoIm,        FICL_WORD_COMPILE_ONLY_IMMEDIATE);
    ficlDictionarySetPrimitive(dictionary, "again",     ficlPrimitiveAgainCoIm,      FICL_WORD_COMPILE_ONLY_IMMEDIATE);
    ficlDictionarySetPrimitive(dictionary, "c\"",       ficlPrimitiveCountedStringQuoteIm, FICL_WORD_IMMEDIATE);
    ficlDictionarySetPrimitive(dictionary, "hex",       ficlPrimitiveHex,            FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "pad",       ficlPrimitivePad,            FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "parse",     ficlPrimitiveParse,          FICL_WORD_DEFAULT);
    /* query restore-input save-input tib u.r u> unused [FICL_VM_STATE_COMPILE] */
    ficlDictionarySetPrimitive(dictionary, "refill",    ficlPrimitiveRefill,         FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "source-id", ficlPrimitiveSourceID,       FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "to",        ficlPrimitiveToValue,        FICL_WORD_IMMEDIATE);
    ficlDictionarySetPrimitive(dictionary, "value",     ficlPrimitiveConstant,       FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "\\",        ficlPrimitiveBackslash,    FICL_WORD_IMMEDIATE);


    /*
    ** Environment query values for the Core word set
    */
    ficlDictionarySetConstant(environment, "/counted-string",   FICL_COUNTED_STRING_MAX);
    ficlDictionarySetConstant(environment, "/hold",             FICL_PAD_SIZE);
    ficlDictionarySetConstant(environment, "/pad",              FICL_PAD_SIZE);
    ficlDictionarySetConstant(environment, "address-unit-bits", 8);
    ficlDictionarySetConstant(environment, "core",              FICL_TRUE);
    ficlDictionarySetConstant(environment, "core-ext",          FICL_FALSE);
    ficlDictionarySetConstant(environment, "floored",           FICL_FALSE);
    ficlDictionarySetConstant(environment, "max-char",          UCHAR_MAX);
    ficlDictionarySetConstant(environment, "max-n",             0x7fffffff);
    ficlDictionarySetConstant(environment, "max-u",             0xffffffff);
	{
	ficl2Unsigned combined;
	FICL_2UNSIGNED_SET(INT_MAX, UINT_MAX, combined);
    ficlDictionarySet2Constant(environment,"max-d",             FICL_2UNSIGNED_TO_2INTEGER(combined));
	FICL_2UNSIGNED_SET(UINT_MAX, UINT_MAX, combined);
    ficlDictionarySet2Constant(environment,"max-ud",            FICL_2UNSIGNED_TO_2INTEGER(combined));
	}
    ficlDictionarySetConstant(environment, "return-stack-cells",FICL_DEFAULT_STACK_SIZE);
    ficlDictionarySetConstant(environment, "stack-cells",       FICL_DEFAULT_STACK_SIZE);

    /*
    ** The optional Double-Number word set (partial)
    */
    ficlDictionarySetPrimitive(dictionary, "2constant", ficlPrimitive2Constant,    FICL_WORD_IMMEDIATE);
    ficlDictionarySetPrimitive(dictionary, "2value",    ficlPrimitive2Constant,    FICL_WORD_IMMEDIATE);
    ficlDictionarySetPrimitive(dictionary, "2literal",  ficlPrimitive2LiteralIm,   FICL_WORD_IMMEDIATE);
    ficlDictionarySetPrimitive(dictionary, "2variable", ficlPrimitive2Variable,    FICL_WORD_IMMEDIATE);
    ficlDictionarySetPrimitive(dictionary, "dnegate",   ficlPrimitiveDNegate,    FICL_WORD_DEFAULT);


    /*
    ** The optional Exception and Exception Extensions word set
    */
    ficlDictionarySetPrimitive(dictionary, "catch",     ficlPrimitiveCatch,      FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "throw",     ficlPrimitiveThrow,      FICL_WORD_DEFAULT);

    ficlDictionarySetConstant(environment, "exception",         FICL_TRUE);
    ficlDictionarySetConstant(environment, "exception-ext",     FICL_TRUE);

    /*
    ** The optional Locals and Locals Extensions word set
    ** see softcore.c for implementation of locals|
    */
#if FICL_WANT_LOCALS
    ficlDictionarySetPrimitive(dictionary, "doLocal",   ficlPrimitiveDoLocalIm,      FICL_WORD_COMPILE_ONLY_IMMEDIATE);
    ficlDictionarySetPrimitive(dictionary, "(local)",   ficlPrimitiveLocalParen,     FICL_WORD_COMPILE_ONLY);
    ficlDictionarySetPrimitive(dictionary, "(2local)",  ficlPrimitive2LocalParen,  FICL_WORD_COMPILE_ONLY);

    ficlDictionarySetConstant(environment, "locals",            FICL_TRUE);
    ficlDictionarySetConstant(environment, "locals-ext",        FICL_TRUE);
    ficlDictionarySetConstant(environment, "#locals",           FICL_MAX_LOCALS);
#endif

    /*
    ** The optional Memory-Allocation word set
    */

    ficlDictionarySetPrimitive(dictionary, "allocate",  ficlPrimitiveAllocate,    FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "free",      ficlPrimitiveFree,        FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "resize",    ficlPrimitiveResize,      FICL_WORD_DEFAULT);
    
    ficlDictionarySetConstant(environment, "memory-alloc",      FICL_TRUE);

    /*
    ** The optional Search-Order word set 
    */
    ficlSystemCompileSearch(system);

    /*
    ** The optional Programming-Tools and Programming-Tools Extensions word set
    */
    ficlSystemCompileTools(system);

    /*
    ** The optional File-Access and File-Access Extensions word set
    */
#if FICL_WANT_FILE
    ficlSystemCompileFile(system);
#endif

    /*
    ** Ficl extras
    */
    ficlDictionarySetPrimitive(dictionary, ".ver",      ficlPrimitiveVersion,    FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, ">name",     ficlPrimitiveToName,         FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "add-parse-step",
                                    ficlPrimitiveAddParseStep,   FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "body>",     ficlPrimitiveFromBody,       FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "compile-only",
                                    ficlPrimitiveCompileOnly,    FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "endif",     ficlPrimitiveEndifCoIm,      FICL_WORD_COMPILE_ONLY_IMMEDIATE);
    ficlDictionarySetPrimitive(dictionary, "last-word", ficlPrimitiveLastWord,    FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "hash",      ficlPrimitiveHash,           FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "objectify", ficlPrimitiveSetObjectFlag,  FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "?object",   ficlPrimitiveIsObject,       FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "parse-word",ficlPrimitiveParseNoCopy,    FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "sfind",     ficlPrimitiveSFind,          FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "sliteral",  ficlPrimitiveSLiteralCoIm,   FICL_WORD_COMPILE_ONLY_IMMEDIATE);
    ficlDictionarySetPrimitive(dictionary, "sprintf",   ficlPrimitiveSprintf,    FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "strlen",    ficlPrimitiveStrlen,     FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "x.",        ficlPrimitiveHexDot,         FICL_WORD_DEFAULT);
#if FICL_WANT_USER
    ficlDictionarySetPrimitive(dictionary, "user",      ficlPrimitiveUser,   FICL_WORD_DEFAULT);
#endif

    /*
    ** internal support words
    */
    interpret =
    ficlDictionarySetPrimitive(dictionary, "interpret", ficlPrimitiveInterpret,      FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "lookup",    ficlPrimitiveLookup,         FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(dictionary, "(parse-step)", 
                                    ficlPrimitiveParseStepParen, FICL_WORD_DEFAULT);
	system->exitInnerWord =
    ficlDictionarySetPrimitive(dictionary, "exit-inner",ficlPrimitiveExitInner,  FICL_WORD_DEFAULT);

	/*
	** Set constants representing the internal instruction words
	** If you want all of 'em, turn that "#if 0" to "#if 1".
	** By default you only get the numbers (fi0, fiNeg1, etc).
	*/
	#define FICL_TOKEN(token, description) ficlDictionarySetConstant(dictionary, #token, token);
#if 0
	#define FICL_INSTRUCTION_TOKEN(token, description, flags) ficlDictionarySetConstant(dictionary, #token, token);
#else
	#define FICL_INSTRUCTION_TOKEN(token, description, flags) 
#endif /* 0 */
	#include "ficltokens.h"
	#undef FICL_TOKEN
	#undef FICL_INSTRUCTION_TOKEN


    /*
    ** Set up system's outer interpreter loop - maybe this should be in initSystem?
    */
	system->interpreterLoop[0] = interpret;
	system->interpreterLoop[1] = (ficlWord *)ficlInstructionBranchParen;
	system->interpreterLoop[2] = (ficlWord *)(void *)(-2);

    FICL_SYSTEM_ASSERT(system, ficlDictionaryCellsAvailable(dictionary) > 0);

    return;
}

