#include <windows.h>
#include <stdio.h>

int main()
{
    HANDLE hDevice = CreateFile(L"\\\\.\\HelloDDK", GENERIC_READ | GENERIC_WRITE, 0,
        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
    if (INVALID_HANDLE_VALUE != hDevice)
    {
        wprintf(L"Open Device failed!");
        return 1;
    }

    OVERLAPPED overlap1 = { 0 };
    OVERLAPPED overlap2 = { 0 };

    UCHAR buffer[10];
    ULONG ulRead;

    BOOL bRead = ReadFile(hDevice, buffer, 10, &ulRead, &overlap1);
    if (!bRead && GetLastError() == ERROR_IO_PENDING)
    {
        wprintf(L"The operation is pending\n");
    }
    bRead = ReadFile(hDevice, buffer, 10, &ulRead, &overlap2);
    if (!bRead && GetLastError() == ERROR_IO_PENDING)
    {
        wprintf(L"The operation is pending\n");
    }
    Sleep(2000);
    CancelIo(hDevice);
    CloseHandle(hDevice);

    return 0;
}