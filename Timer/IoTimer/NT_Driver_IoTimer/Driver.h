#pragma once
#ifdef __cplusplus
extern "C"
{
#endif
#include <NTDDK.h>
#ifdef __cplusplus
}
#endif

#include "ioctls.h"

#define PAGEDCODE code_seg("PAGE")
#define LOCKEDCODE code_seg()
#define INITCODE code_seg("INIT")

#define PAGEDATA data_seg("PAGE")
#define LOCKEDDATA data_seg()
#define INITDATA code_seg("INIT")

#define arraysize(p) (sizeof(p)/sizeof((p)[0]))

#define TIMER_OUT 3

typedef struct _DEVICE_EXTENSION {
    PDEVICE_OBJECT pDevice;
    UNICODE_STRING ustrDeviceName;
    UNICODE_STRING ustrSymLinkName;
    LONG lTimeCount;
}DEVICE_EXTENSION, *PDEVICE_EXTENSION;

NTSTATUS CreateDevice(IN PDRIVER_OBJECT pDriverObject);
VOID HelloDDKUnload(IN PDRIVER_OBJECT pDriverObject);
NTSTATUS HelloDDKDispatchRoutin(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp);
NTSTATUS HelloDDKDeviceIOControl(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp);
VOID OnTimer(IN PDEVICE_OBJECT pDevObj, IN PVOID Context);