/*
 * Windows MTP Firmware Uploading Implementation
 *
 * Based on http://opensource.creative.com/mtp_xfer.html
 * Edited by Maurus Cuelenaere for Rockbox
 *
 * Copyright (c) 2009, Maurus Cuelenaere
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY MAURUS CUELENAERE ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL MAURUS CUELENAERE BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <windows.h>
#include "mswmdm_i.c"
#include "mswmdm.h"
#include "sac.h"
#include "scclient.h"

#include "progresshelper.h"




CProgressHelper::CProgressHelper( void (*callback)(unsigned int progress, unsigned int max) )
{
    m_cur_ticks = 0;
    m_max_ticks = 0;
    m_counter = 0;

    m_callback = callback;
}

CProgressHelper::~CProgressHelper()
{
}

HRESULT CProgressHelper::Begin( DWORD dwEstimatedTicks )
{
    m_max_ticks = dwEstimatedTicks;

    return S_OK;
}

HRESULT CProgressHelper::Progress( DWORD dwTranspiredTicks )
{
    m_cur_ticks = dwTranspiredTicks;

    if(m_callback != NULL)
        m_callback(m_cur_ticks, max(m_max_ticks, m_cur_ticks));

    return S_OK;
}

HRESULT CProgressHelper::End()
{
    m_cur_ticks = m_max_ticks;

    return S_OK;
}

HRESULT CProgressHelper::QueryInterface( REFIID riid, void __RPC_FAR *__RPC_FAR *ppvObject )
{
    if(riid == IID_IWMDMProgress || riid == IID_IUnknown)
    {
        *ppvObject = this;
        return S_OK;
    }
    else
    {
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }
}

ULONG CProgressHelper::AddRef()
{
    return m_counter++;
}
ULONG CProgressHelper::Release()
{
    return m_counter--;
}
