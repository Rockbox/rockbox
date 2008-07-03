/* Windows MTP Firmware Uploading Implementation
 *
 * Based on http://opensource.creative.com/mtp_xfer.html
 *
 * Edited by Maurus Cuelenaere for Rockbox
 */

#include <windows.h>
#include "mswmdm_i.c"
#include "mswmdm.h"
#include "sac.h"
#include "scclient.h"

/*
 * Compilation requirements:
 *
 * Download the Windows Media Format 9.5 SDK
 * Add "c:\wmsdk\wmfsdk95\include,c:\wmsdk\wmfsdk95\wmdm\inc" to your inclusion path
 * Add "c:\wmsdk\wmfsdk95\lib,c:\wmsdk\wmfsdk95\wmdm\lib" to your library inclusion path
 * Link to "mssachlp.lib"
 *
 */

__declspec(dllexport) bool send_fw(LPWSTR file, int filesize)
{
    bool return_value = false;
	HRESULT hr;
	IComponentAuthenticate* pICompAuth;
	CSecureChannelClient *m_pSacClient = new CSecureChannelClient;
	IWMDeviceManager3* m_pIdvMgr = NULL;

	/* these are generic keys */
	BYTE abPVK[] = {0x00};
	BYTE abCert[] = {0x00};

	CoInitialize(NULL);

	/* get an authentication interface */
	hr = CoCreateInstance(CLSID_MediaDevMgr, NULL, CLSCTX_ALL ,IID_IComponentAuthenticate, (void **)&pICompAuth);
	if SUCCEEDED(hr)
	{
		/* create a secure channel client certificate */
		hr = m_pSacClient->SetCertificate(SAC_CERT_V1, (BYTE*) abCert, sizeof(abCert), (BYTE*) abPVK, sizeof(abPVK));
		if SUCCEEDED(hr)
		{
			/* bind the authentication interface to the secure channel client */
			m_pSacClient->SetInterface(pICompAuth);

			/* trigger communication */
			hr = m_pSacClient->Authenticate(SAC_PROTOCOL_V1);                    
			if SUCCEEDED(hr)
			{
				/* get main interface to media device manager */
				hr = pICompAuth->QueryInterface(IID_IWMDeviceManager2, (void**)&m_pIdvMgr);
				if SUCCEEDED(hr)
				{
					/* enumerate devices... */
					IWMDMEnumDevice *pIEnumDev;
					hr = m_pIdvMgr->EnumDevices2(&pIEnumDev);
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
#if 0
								/* output device name */
								wchar_t pwsString[256];
								hr = pIDevice->GetName(pwsString, 256);
								if SUCCEEDED(hr)
									wprintf(L"Found device %s\n", pwsString);
#endif

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

                                                            hr = pIWMDMStorageControl->Insert3(
                                                                     WMDM_MODE_BLOCK | WMDM_CONTENT_FILE,
                                                                     0,
                                                                     file,
                                                                     NULL,
                                                                     NULL,
                                                                     NULL,
                                                                     pIWMDMMetaData,
                                                                     NULL,
                                                                     (IWMDMStorage **)&pNewObject);

                                                            if(SUCCEEDED(hr) || hr == WMDM_S_NOT_ALL_PROPERTIES_APPLIED
                                                                || hr == WMDM_S_NOT_ALL_PROPERTIES_RETRIEVED)
                                                                return_value = true;
                                                        }
                                                    }
                                                }
                                            }
										}
										/* move to next Storage */
										if(!return_value)
											hr = pIEnumStorage->Next(1, (IWMDMStorage **)&pIStorage, &ulNumFetched);
									}
									pIEnumStorage->Release();
								}

								/* move to next device */
								if(!return_value)
									hr = pIEnumDev->Next(1, (IWMDMDevice **)&pIDevice, &ulNumFetched);
							}
							pIEnumDev->Release();
						}
						m_pIdvMgr->Release();
					}
					pICompAuth->Release();
				}
			}
		}
	}

	CoUninitialize();

	return return_value;
}

/*
 * Example code:

#include <windows.h>
int filesize(char* filename)
{
	FILE* fd;
    int tmp;
	fopen_s(&fd, filename, "r");
    fseek(fd, 0, SEEK_END);
    tmp = ftell(fd);
	fclose(fd);
    return tmp;
}

__declspec(dllimport) bool send_fw(LPWSTR file, int filesize);

int _tmain(int argc, char* argv[])
{
	printf("Sending %s [%d]...\n", "Zlant.bin", filesize("Zlant.bin"));
	if(send_fw(L"Zlant.bin", filesize("Zlant.bin")))
		printf("Transmitting succesfull!\n");
	else
		printf("Transmitting failed!\n");
	return 0;
}

 */
