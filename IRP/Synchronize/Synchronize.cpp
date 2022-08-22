// Synchronize.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <windows.h>
#include <stdio.h>

#define BUFFER_SIZE 512
int main()
{
    HANDLE hDevice = CreateFile(L"test.dat",
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, // There is no FILE_FLAG_OVERLAPPED
        NULL);
    if (hDevice == INVALID_HANDLE_VALUE)
    {
        printf("Read Error!\n");
        return 1;
    }

    WCHAR buffer[BUFFER_SIZE];
    DWORD dwRead;
    ReadFile(hDevice, buffer, BUFFER_SIZE, &dwRead, NULL); //There is no FILE_FLAG_OVERLAPPED
    CloseHandle(hDevice);
    return 0;

}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
