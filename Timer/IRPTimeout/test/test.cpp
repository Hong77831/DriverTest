#include <windows.h>
#include <stdio.h>
#include <process.h>

int main()
{
    HANDLE hDevice =
        CreateFile(L"\\\\.\\HelloDDK",
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

    if (hDevice == INVALID_HANDLE_VALUE)
    {
        wprintf(L"Open Device failed!");
        return 1;
    }

    DWORD dwRead;

    //如果读IRP没有被完成，ReadFile一直都不会退出!
    ReadFile(hDevice, NULL, 0, &dwRead, NULL);

    ReadFile(hDevice, NULL, 0, &dwRead, NULL);

    CloseHandle(hDevice);

    return 0;
}