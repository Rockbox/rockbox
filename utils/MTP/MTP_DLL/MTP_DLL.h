#ifdef MTP_DLL_EXPORTS
#define MTP_DLL_API __declspec(dllexport)
#else
#define MTP_DLL_API __declspec(dllimport)
#endif

extern "C"
{
    __declspec(dllexport) bool send_fw(LPWSTR file, int filesize, void (*callback)(unsigned int progress, unsigned int max));
}
