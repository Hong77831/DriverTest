#pragma once
#ifndef CTL_CODE
#pragma message("CTL_CODE undefined. Include winioctl.h or wdm.h")
#endif

#define IOCTL_START_TIMER CTL_CODE(\
FILE_DEVICE_UNKNOWN,\
0x800,\
METHOD_BUFFERED,\
FILE_ANY_ACCESS)

#define IOCTL_STOP CTL_CODE(\
FILE_DEVICE_UNKNOWN,\
0x801,\
METHOD_IN_DIRECT,\
FILE_ANY_ACCESS)


