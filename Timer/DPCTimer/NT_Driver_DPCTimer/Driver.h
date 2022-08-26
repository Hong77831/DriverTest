#pragma once
#ifdef __cplusplus
extern "C"
{
#endif
#include <NTDDK.h>
#ifdef __cplusplus
}
#endif

#include "Ioctls.h"
#define PAGEDCODE code_seg("PAGE")
#define LOCKEDCODE code_seg()
#define INITCODE  code_seg("INIT")

#define PAGEDDATA data_seg("PAGE")
#define LOCKEDDATA data_seg()
#define INITDATA data_seg("INIT")

#define arraysize(p) (sizeof(p)/sizeof((p)[0]))

#define TIMER_OUT 3

typedef struct _DEVICE_EXTENSION {
    PDEVICE_OBJECT pDevice;
    UNICODE_STRING ustrDeviceName;
    UNICODE_STRING ustrSymLinkName;

    KDPC pollingDPC;
    KTIMER pollingTimer;
    LARGE_INTEGER pollingInterval;
}DEVICE_EXTENSION, * PDEVICE_EXTENSION;

NTSTATUS CreateDevice(IN PDRIVER_OBJECT pDriverObject);
VOID HelloDDKUnload(IN PDRIVER_OBJECT pDriverObject);
NTSTATUS HelloDDKDispatchRoutin(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp);
NTSTATUS HelloDDKDeviceIOControl(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp);
VOID PollingTimeDpc(IN PKDPC pDpc, IN PVOID pContext, IN PVOID SysArg1, IN PVOID SysArg2);
