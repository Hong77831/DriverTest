#pragma once
#ifndef CTL_CODE
#pragma message("CTL_CODE undefined. Include winioctl.h or wdm.h")
#endif

#define IOCTL_TIME_TEST CTL_CODE(\
            FILE_DEVICE_UNKNOWN, \
            0x800, \
            METHOD_BUFFERED, \
            FILE_ANY_ACCESS)