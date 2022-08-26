#include "Driver.h"

#pragma INITCODE
extern "C" NTSTATUS DriverEntry(IN PDRIVER_OBJECT pDriverObject, IN PUNICODE_STRING pRegistryPath)
{
    NTSTATUS status;
    UNREFERENCED_PARAMETER(pRegistryPath);

    KdPrint(("Enter DriverEntry!\n"));
    pDriverObject->DriverUnload = HelloDDKUnload;

    for (int i = 0; i < arraysize(pDriverObject->MajorFunction); ++i)
    {
        pDriverObject->MajorFunction[i] = HelloDDKDispatchRoutin;
    }

    pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = HelloDDKDeviceIOControl;

    status = CreateDevice(pDriverObject);
    KdPrint(("Leave DriverEntry!\n"));

    return status;
}

#pragma INITCODE
NTSTATUS CreateDevice(PDRIVER_OBJECT pDriverObject)
{
    PDEVICE_OBJECT pDevObj;
    PDEVICE_EXTENSION pDevExt;
    UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\DEVICE\\MyDDKDevice");

    NTSTATUS status = IoCreateDevice(pDriverObject, sizeof(DEVICE_EXTENSION), &devName, FILE_DEVICE_UNKNOWN, 0, TRUE, &pDevObj);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    pDevObj->Flags |= DO_DIRECT_IO;
    pDevExt = (PDEVICE_EXTENSION)pDriverObject->DriverExtension;
    pDevExt->pDevice = pDevObj;

    KeInitializeTimer(&pDevExt->pollingTimer);
    KeInitializeDpc(&pDevExt->pollingDPC, PollingTimeDpc, (PVOID)pDevObj);

    UNICODE_STRING symLinkName = RTL_CONSTANT_STRING(L"\\??\\HelloDDK");
    pDevExt->ustrSymLinkName = symLinkName;
    status = IoCreateSymbolicLink(&symLinkName, &devName);
    if (!NT_SUCCESS(status))
    {
        IoDeleteDevice(pDevObj);
        return status;
    }

    return STATUS_SUCCESS;

}

#pragma PAGECODE
VOID HelloDDKUnload(PDRIVER_OBJECT pDriverObject)
{
    PDEVICE_OBJECT pNextObj;
    KdPrint(("Enter HelloDDKUnload!\n"));
    pNextObj = pDriverObject->DeviceObject;
    while (NULL != pNextObj)
    {
        PDEVICE_EXTENSION pDevExt;
        pDevExt = (PDEVICE_EXTENSION)pNextObj->DeviceExtension;
        IoDeleteSymbolicLink(&pDevExt->ustrSymLinkName);
        pNextObj = pNextObj->NextDevice;
        IoDeleteDevice(pDevExt->pDevice);
    }
}

#pragma PAGECODE
NTSTATUS HelloDDKDispatchRoutin(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp)
{
    UNREFERENCED_PARAMETER(pDevObj);
    KdPrint(("Enter HelloDDKDispatchRoutin!\n"));

    PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);

    static char* irpname[] =
    {
        "IRP_MJ_CREATE",
        "IRP_MJ_CREATE_NAMED_PIPE",
        "IRP_MJ_CLOSE",
        "IRP_MJ_READ",
        "IRP_MJ_WRITE",
        "IRP_MJ_QUERY_INFORMATION",
        "IRP_MJ_SET_INFORMATION",
        "IRP_MJ_QUERY_EA",
        "IRP_MJ_SET_EA",
        "IRP_MJ_FLUSH_BUFFERS",
        "IRP_MJ_QUERY_VOLUME_INFORMATION",
        "IRP_MJ_SET_VOLUME_INFORMATION",
        "IRP_MJ_DIRECTORY_CONTROL",
        "IRP_MJ_FILE_SYSTEM_CONTROL",
        "IRP_MJ_DEVICE_CONTROL",
        "IRP_MJ_INTERNAL_DEVICE_CONTROL",
        "IRP_MJ_SHUTDOWN",
        "IRP_MJ_LOCK_CONTROL",
        "IRP_MJ_CLEANUP",
        "IRP_MJ_CREATE_MAILSLOT",
        "IRP_MJ_QUERY_SECURITY",
        "IRP_MJ_SET_SECURITY",
        "IRP_MJ_POWER",
        "IRP_MJ_SYSTEM_CONTROL",
        "IRP_MJ_DEVICE_CHANGE",
        "IRP_MJ_QUERY_QUOTA",
        "IRP_MJ_SET_QUOTA",
        "IRP_MJ_PNP",
    };

    UCHAR type = stack->MajorFunction;
    if(type >= arraysize(irpname))
    {
        KdPrint(("-Unknown IRP, major type %X!\n", type));
    }
    else
    {
        KdPrint(("\t%s\n", irpname[type]));
    }

    NTSTATUS status = STATUS_SUCCESS;
    pIrp->IoStatus.Status = status;
    pIrp->IoStatus.Information = 0;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    KdPrint(("Leave HelloDDKDispatchRoutin!\n"));

    return status;
}

#pragma PAGECODE
NTSTATUS HelloDDKDeviceIOControl(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp)
{
    NTSTATUS status = STATUS_SUCCESS;
    KdPrint(("Enter HelloDDKDeviceIOContrl!\n"));

    PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);
    ULONG cbin = stack->Parameters.DeviceIoControl.InputBufferLength;
    ULONG cbout = stack->Parameters.DeviceIoControl.OutputBufferLength;
    ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;
    UNREFERENCED_PARAMETER(cbin);
    UNREFERENCED_PARAMETER(cbout);
    UNREFERENCED_PARAMETER(code);
    
    PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
    ULONG info = 0;

    switch (code)
    {
    case IOCTL_START_TIMER:
    {
        KdPrint(("IOCTL_START_TIMER!\n"));

        ULONG ulMicroSeconds = *(PULONG)pIrp->AssociatedIrp.SystemBuffer;
        pDevExt->pollingInterval.QuadPart = ulMicroSeconds * -10;

        KeSetTimer(&pDevExt->pollingTimer,
            pDevExt->pollingInterval,
            &pDevExt->pollingDPC);
        break;
    }
    case IOCTL_STOP_TIMER:
    {
        KdPrint(("IOCTL_STOP_TIMER!\n"));
        KeCancelTimer(&pDevExt->pollingTimer);
        break;
    }
    default:
        status = STATUS_INVALID_VARIANT;
    }

    pIrp->IoStatus.Status = 0;
    pIrp->IoStatus.Information = info;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    KdPrint(("Leave HelloDDKDeviceIOControl!\n"));

    return status;
}

#pragma LOCKEDCODE
VOID PollingTimeDpc(IN PKDPC pDpc, IN PVOID pContext, IN PVOID SysArg1, IN PVOID SysArg2)
{
    UNREFERENCED_PARAMETER(pDpc);
    UNREFERENCED_PARAMETER(SysArg1);
    UNREFERENCED_PARAMETER(SysArg2);
    PDEVICE_OBJECT pDevObj = (PDEVICE_OBJECT)pContext;
    PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
    KeSetTimer(&pdx->pollingTimer, pdx->pollingInterval, &pdx->pollingDPC);
    KdPrint(("PollingTimerDpc!\n"));
    
    PEPROCESS pEProcess = IoGetCurrentProcess();
    PTSTR ProcessName = (PTSTR)((ULONGLONG)pEProcess + 0x174);
    KdPrint(("%s\n", ProcessName));

}
