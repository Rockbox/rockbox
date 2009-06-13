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
#include "MTP_DLL.h"

/*
 * Compilation requirements:
 *
 * Download the Windows Media Format 9.5 SDK
 * Add "c:\wmsdk\wmfsdk95\include,c:\wmsdk\wmfsdk95\wmdm\inc" to your inclusion path
 * Add "c:\wmsdk\wmfsdk95\lib,c:\wmsdk\wmfsdk95\wmdm\lib" to your library inclusion path
 * Link to "mssachlp.lib"
 *
 */

struct mtp_if {
	IComponentAuthenticate* pICompAuth;
	CSecureChannelClient *pSacClient;
	IWMDeviceManager3* pIdvMgr;
	bool initialized;
};


extern "C" {
static int mtp_init(struct mtp_if* mtp)
{
	HRESULT hr;
	mtp->pSacClient = new CSecureChannelClient;
	mtp->pIdvMgr = NULL;
	mtp->initialized = false;

	/* these are generic keys */
	BYTE abPVK[] = {0x00};
	BYTE abCert[] = {0x00};

	CoInitialize(NULL);

	/* get an authentication interface */
	hr = CoCreateInstance(CLSID_MediaDevMgr, NULL, CLSCTX_ALL,
						IID_IComponentAuthenticate, (void **)&mtp->pICompAuth);
	if SUCCEEDED(hr)
	{
		/* create a secure channel client certificate */
		hr = mtp->pSacClient->SetCertificate(SAC_CERT_V1, (BYTE*) abCert,
								sizeof(abCert), (BYTE*) abPVK, sizeof(abPVK));
		if SUCCEEDED(hr)
		{
			/* bind the authentication interface to the secure channel client */
			mtp->pSacClient->SetInterface(mtp->pICompAuth);

			/* trigger communication */
			hr = mtp->pSacClient->Authenticate(SAC_PROTOCOL_V1);                    
			if SUCCEEDED(hr)
			{
				/* get main interface to media device manager */
				hr = mtp->pICompAuth->QueryInterface(IID_IWMDeviceManager2, 
										(void**)&mtp->pIdvMgr);
				if SUCCEEDED(hr)
				{
 					mtp->initialized = true;
				}
			}
		}
	}
	else {
		CoUninitialize();
	}
	return mtp->initialized;
}


static int mtp_close(struct mtp_if* mtp)
{
	if(mtp->initialized)
	{
		mtp->pIdvMgr->Release();
		mtp->pICompAuth->Release();
		CoUninitialize();
		mtp->initialized = false;
	}
	return 0;
}

MTP_DLL_API int mtp_description(wchar_t* name, wchar_t* manufacturer, DWORD* version)
{
	HRESULT hr;
	int num = 0;
	struct mtp_if mtp;
	/* zero mtp structure */
	memset(&mtp, 0, sizeof(struct mtp_if));

	/* initialize interface */
	mtp_init(&mtp);
	if(mtp.initialized == false) {
		return -1;
	}

	/* we now have a media device manager interface... */
	/* enumerate devices... */
	IWMDMEnumDevice *pIEnumDev;
	wchar_t pwsString[256];
	hr = mtp.pIdvMgr->EnumDevices2(&pIEnumDev);
	if SUCCEEDED(hr) {
		hr = pIEnumDev->Reset(); /* Next will now return the first device */
		if SUCCEEDED(hr) {
			IWMDMDevice3* pIDevice;
			unsigned long ulNumFetched;
			hr = pIEnumDev->Next(1, (IWMDMDevice **)&pIDevice, &ulNumFetched);
			while (SUCCEEDED(hr) && (hr != S_FALSE)) {
				/* output device name */
				hr = pIDevice->GetName(pwsString, 256);
				if SUCCEEDED(hr) {
					wcsncpy_s(name, 256, pwsString, _TRUNCATE);
					num++;
				}
				/* device manufacturer */
				hr = pIDevice->GetManufacturer(pwsString, 256);
				if SUCCEEDED(hr) {
					wcsncpy_s(manufacturer, 256, pwsString, _TRUNCATE);
				}
				/* device version -- optional interface so might fail. */
				DWORD ver;
				hr = pIDevice->GetVersion(&ver);
				if SUCCEEDED(hr) {
					*version = ver;	
				}
				else {
					*version = 0;
				}
				
				/* move to next device */
				hr = pIEnumDev->Next(1, (IWMDMDevice **)&pIDevice, &ulNumFetched);
			}
			pIEnumDev->Release();
		}
		mtp_close(&mtp);
	}
	return (num > 0) ? num : -1;
}

MTP_DLL_API int mtp_sendnk(LPWSTR file, int filesize, void (*callback)(unsigned int progress, unsigned int max))
{
	HRESULT hr;
	bool return_value = false;
	struct mtp_if mtp;
	/* zero mtp structure */
	memset(&mtp, 0, sizeof(struct mtp_if));

	/* initialize interface */
	mtp_init(&mtp);
	if(mtp.initialized == false) {
		return false;
	}
    /* enumerate devices... */
    IWMDMEnumDevice *pIEnumDev;
    hr = mtp.pIdvMgr->EnumDevices2(&pIEnumDev);
    if SUCCEEDED(hr)
    {
        hr = pIEnumDev->Reset(); /* Next will now return the first device */
        if SUCCEEDED(hr)
        {
            IWMDMDevice3* pIDevice;
            unsigned long ulNumFetched;
            hr = pIEnumDev->Next(1, (IWMDMDevice **)&pIDevice, &ulNumFetched);
            while (SUCCEEDED(hr) && (hr != S_FALSE))
            {
                /* get storage info */
                DWORD tempDW;
                pIDevice->GetType(&tempDW);
                if (tempDW & WMDM_DEVICE_TYPE_STORAGE)
                {
                    IWMDMEnumStorage *pIEnumStorage = NULL;
                    IWMDMStorage *pIStorage = NULL;
                    IWMDMStorage3 *pIFileStorage = NULL; 
                    hr = pIDevice->EnumStorage(&pIEnumStorage);
                    if SUCCEEDED(hr)
                    {
                        pIEnumStorage->Reset();
                        hr = pIEnumStorage->Next(1, (IWMDMStorage **)&pIStorage, &ulNumFetched);
                        while (SUCCEEDED(hr) && (hr != S_FALSE))
                        {
                            IWMDMStorage3 *pNewStorage;
                            hr = pIStorage->QueryInterface(IID_IWMDMStorage3, (void **)&pNewStorage);
                            if SUCCEEDED(hr)
                            {
                                IWMDMStorageControl3 *pIWMDMStorageControl;
                                hr = pNewStorage->QueryInterface(IID_IWMDMStorageControl3, 
                                                               (void**)&pIWMDMStorageControl);
                                if SUCCEEDED(hr)
                                {
                                    IWMDMMetaData *pIWMDMMetaData = NULL;
                                    hr = pNewStorage->CreateEmptyMetadataObject(&pIWMDMMetaData);
                                    if (SUCCEEDED(hr))
                                    {
                                        DWORD dw = WMDM_FORMATCODE_UNDEFINEDFIRMWARE;
                                        hr = pIWMDMMetaData->AddItem(WMDM_TYPE_DWORD, g_wszWMDMFormatCode, (BYTE *)&dw, sizeof(dw));
                                        hr = pIWMDMMetaData->AddItem(WMDM_TYPE_STRING, g_wszWMDMFileName, (BYTE *)L"nk.bin", 32);
                                        DWORD ow[2];
                                        ow[0] = filesize;
                                        ow[1] = 0;
                                        hr = pIWMDMMetaData->AddItem(WMDM_TYPE_QWORD, g_wszWMDMFileSize, (BYTE *)ow, 2 * sizeof(dw));
                                        if (SUCCEEDED(hr))
                                        {
                                            IWMDMStorage *pNewObject = NULL;
                                            CProgressHelper *progress = new CProgressHelper(callback);

                                            hr = pIWMDMStorageControl->Insert3(
                                                     WMDM_MODE_BLOCK | WMDM_CONTENT_FILE | WMDM_MODE_PROGRESS,
                                                     0,
                                                     file,
                                                     NULL,
                                                     NULL,
                                                     (callback == NULL ? NULL : (IWMDMProgress*)progress),
                                                     pIWMDMMetaData,
                                                     NULL,
                                                     (IWMDMStorage **)&pNewObject);

                                            if(SUCCEEDED(hr)
												|| hr == WMDM_S_NOT_ALL_PROPERTIES_APPLIED
                                                || hr == WMDM_S_NOT_ALL_PROPERTIES_RETRIEVED)
                                            {
                                                return_value = true;
                                                hr = S_FALSE;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    pIEnumStorage->Release();
                }

                /* move to next device */
                if(!return_value)
                    hr = pIEnumDev->Next(1, (IWMDMDevice **)&pIDevice, &ulNumFetched);
            }
            pIEnumDev->Release();
        }
		mtp_close(&mtp);
	}
	return return_value ? 1 : 0;
}

}
