#include <windows.h>
#include <stdio.h>
//使用CTL_CODE必须加入winioctl.h
#include <winioctl.h>
#include "..\NT_Driver_Time\Ioctls.h"

int main()
{
    HANDLE hDevice =
        CreateFile(L"\\\\.\\HelloDDK",
            GENERIC_READ | GENERIC_WRITE,
            0,         // share mode none
            NULL,      // no security
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);    // no template

    if (hDevice == INVALID_HANDLE_VALUE)
    {
        printf("Failed to obtain file handle to device: "
            "%S with Win32 error code: %d\n",
            L"MyWDMDevice", GetLastError());
        return 1;
    }

    DWORD dwOutput;

    DeviceIoControl(hDevice, IOCTL_TIME_TEST, NULL, 0, NULL, 0, &dwOutput, NULL);

    CloseHandle(hDevice);

    return 0;
}