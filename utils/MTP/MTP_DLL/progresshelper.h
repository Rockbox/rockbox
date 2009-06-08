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

class CProgressHelper : 
    public IWMDMProgress
{
    void          (*m_callback)(unsigned int progress, unsigned int max);
    DWORD          m_max_ticks;
    DWORD          m_cur_ticks;
    DWORD          m_counter;

public:
    CProgressHelper( void (*callback)(unsigned int progress, unsigned int max) );
    ~CProgressHelper();
    STDMETHOD(Begin)( DWORD dwEstimatedTicks );
    STDMETHOD(Progress)( DWORD dwTranspiredTicks );
    STDMETHOD(End)();

    STDMETHOD(QueryInterface) ( REFIID riid, void __RPC_FAR *__RPC_FAR *ppvObject );
    STDMETHOD_(ULONG, AddRef)( void );
    STDMETHOD_(ULONG, Release)( void );
};

