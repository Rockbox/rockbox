
#ifndef MTP_DLL_H
#define MTP_DLL_H

#ifndef MTP_NODLL
#ifdef MTP_DLL_EXPORTS
#define MTP_DLL_API __declspec(dllexport)
#else
#define MTP_DLL_API __declspec(dllimport)
#endif
#else
#define MTP_DLL_API
#endif

#ifdef __cplusplus
extern "C"
{
#endif
MTP_DLL_API int mtp_sendnk(LPWSTR file, int filesize, void (*callback)(unsigned int progress, unsigned int max));
MTP_DLL_API int mtp_description(wchar_t* name, wchar_t* manufacturer, DWORD* version);
#ifdef __cplusplus
}
#endif

#endif
