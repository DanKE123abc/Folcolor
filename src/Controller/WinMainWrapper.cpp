#include <windows.h>

extern int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // Convert ANSI command line to Unicode
    int len = MultiByteToWideChar(CP_ACP, 0, lpCmdLine, -1, NULL, 0);
    wchar_t* pCmdLineW = new wchar_t[len];
    MultiByteToWideChar(CP_ACP, 0, lpCmdLine, -1, pCmdLineW, len);
    
    // Call wWinMain
    int result = wWinMain(hInstance, hPrevInstance, pCmdLineW, nCmdShow);
    
    // Cleanup
    delete[] pCmdLineW;
    
    return result;
}
