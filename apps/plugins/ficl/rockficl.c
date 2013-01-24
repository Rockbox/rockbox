/*
** stub main for testing Ficl
** $Id: main.c,v 1.2 2010/09/10 09:01:28 asau Exp $
*/
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

#include "plugin.h"
#include <tlsf.h>
#include "ficl.h"
#include "console.h"

enum plugin_status plugin_start(const void* parameter)
{
    char buf[256];
    void *memory;
    size_t memory_size;
    ficlVm *vm;
    ficlSystem *system;

    const char* filename = (char *)parameter;

    if (filename == NULL)
        return PLUGIN_ERROR;

    memory = rb->plugin_get_buffer(&memory_size);
    init_memory_pool(memory_size, memory);

    system = ficlSystemCreate(NULL);
    ficlSystemCompileExtras(system);
    vm = ficlSystemCreateVm(system);

    snprintf(buf, sizeof(buf), ".( loading %s ) cr load %s\n cr", filename, filename);
    ficlVmEvaluate(vm, buf);

    ficlSystemDestroy(system);

    show_console();

    return PLUGIN_OK;
}
