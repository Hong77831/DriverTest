#include "Driver.h"

#pragma INITCODE
extern "C" NTSTATUS DriverEntry(IN PDRIVER_OBJECT pDriverObject,
    IN PUNICODE_STRING pRegistryPath)
{
    NTSTATUS status;
    UNREFERENCED_PARAMETER(pRegistryPath);
    KdPrint(("Entry DriverEntry"));
    pDriverObject->DriverUnload = HelloDDKUnload;

    pDriverObject->MajorFunction[IRP_MJ_CREATE] = HelloDDKDispatchRoutin;
    pDriverObject->MajorFunction[IRP_MJ_CLOSE] = HelloDDKDispatchRoutin;
    pDriverObject->MajorFunction[IRP_MJ_WRITE] = HelloDDKDispatchRoutin;
    pDriverObject->MajorFunction[IRP_MJ_READ] = HelloDDKRead;
    pDriverObject->MajorFunction[IRP_MJ_CLEANUP] = HelloDDKCleanUp;
    pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = HelloDDKDispatchRoutin;
    pDriverObject->MajorFunction[IRP_MJ_SET_INFORMATION] = HelloDDKDispatchRoutin;
    pDriverObject->MajorFunction[IRP_MJ_SHUTDOWN] = HelloDDKDispatchRoutin;
    pDriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = HelloDDKDispatchRoutin;
    
    status = CreateDevice(pDriverObject);

    KdPrint(("Leave DriverEntry\n"));
    return status;
}

#pragma INITCODE
NTSTATUS CreateDevice(IN PDRIVER_OBJECT pDriverObject)
{
    NTSTATUS status;
    PDEVICE_OBJECT pDevObj;
    PDEVICE_EXTENSION pDevExt;

    UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Device\\MyDDKDevice");
    status = IoCreateDevice(pDriverObject, sizeof(DEVICE_EXTENSION), &devName,
        FILE_DEVICE_UNKNOWN, 0, TRUE, &pDevObj);
    if (!NT_SUCCESS(status))
    {
        KdPrint(("IoCreateDevice failed!\n"));
        return status;
    }

    pDevObj->Flags |= DO_BUFFERED_IO;
    pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
    pDevExt->ustrDeviceName = devName;
    pDevExt->pIRPLinkListHead = (PLIST_ENTRY)ExAllocatePool2(PagedPool, sizeof(LIST_ENTRY), '1111');

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

#pragma PAGEDCODE
VOID HelloDDKUnload(IN PDRIVER_OBJECT pDriverObject) {
    PDEVICE_OBJECT pNextObj;
    pNextObj = pDriverObject->DeviceObject;
    PDEVICE_EXTENSION pDevExt;
    while (NULL != pNextObj)
    {
        pDevExt = (PDEVICE_EXTENSION)pNextObj->DeviceExtension;
        pNextObj = pNextObj->NextDevice;
        IoDeleteSymbolicLink(&pDevExt->ustrSymLinkName);
        ExFreePool(pDevExt->pIRPLinkListHead);
        IoDeleteDevice(pDevExt->pDevice);
    }

}

#pragma PAGECODE
NTSTATUS HelloDDKDispatchRoutin(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp)
{
    UNREFERENCED_PARAMETER(pDevObj);
    KdPrint(("Enter HelloDDKDispatchRoutin\n"));

    PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);
    //建立一个字符串数组与IRP类型对应起来
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
    if (type >= arraysize(irpname))
        KdPrint((" - Unknown IRP, major type %X\n", type));
    else
        KdPrint(("    %s\n", irpname[type]));


    //对一般IRP的简单操作，后面会介绍对IRP更复杂的操作
    NTSTATUS status = STATUS_SUCCESS;
    // 完成IRP
    pIrp->IoStatus.Status = status;
    pIrp->IoStatus.Information = 0;	// bytes xfered
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    KdPrint(("Leave HelloDDKDispatchRoutin\n"));

    return status;
}

VOID CancelReadIRP(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    KdPrint(("Enter CancelReadIRP\n"));

    Irp->IoStatus.Status = STATUS_CANCELLED;
    Irp->IoStatus.Information = 0;	// bytes xfered
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    IoReleaseCancelSpinLock(Irp->CancelIrql);
    KdPrint(("Leave CancelReadIRP\n"));

}

NTSTATUS HelloDDKRead(IN PDEVICE_OBJECT pDevObj,
    IN PIRP pIrp)
{
    KdPrint(("Enter HelloDDKRead!\n"));
    PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
    PMY_IRP_ENTRY pIrp_entry = (PMY_IRP_ENTRY)ExAllocatePool2(PagedPool, sizeof(MY_IRP_ENTRY), '2222');
    if (pIrp_entry)
    {
        pIrp_entry->pIRP = pIrp;
        InsertHeadList(pDevExt->pIRPLinkListHead, &pIrp_entry->ListEntry);
        IoSetCancelRoutine(pIrp, CancelReadIRP);
    }
    else
    {
        KdPrint(("ExAllocatePool2 failed!\n"));
    }

    IoMarkIrpPending(pIrp);
    KdPrint(("Leave HelloDDKRead\n"));

    return STATUS_PENDING;
}

NTSTATUS HelloDDKCleanUp(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp)
{
    KdPrint(("Enter HelloDDKCleanUp\n"));
    PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;

    PMY_IRP_ENTRY my_irp_entry;
    while (!IsListEmpty(pDevExt->pIRPLinkListHead))
    {
        PLIST_ENTRY pEntry = RemoveHeadList(pDevExt->pIRPLinkListHead);
        my_irp_entry = CONTAINING_RECORD(pEntry, MY_IRP_ENTRY, ListEntry);
        my_irp_entry->pIRP->IoStatus.Status = STATUS_SUCCESS;
        my_irp_entry->pIRP->IoStatus.Information = 0;
        IoCompleteRequest(my_irp_entry->pIRP, IO_NO_INCREMENT);
        ExFreePool(my_irp_entry);
    }

    NTSTATUS status = STATUS_SUCCESS;
    pIrp->IoStatus.Status = status;
    pIrp->IoStatus.Information = 0;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    KdPrint(("Leave HelloDDKCleanUp\n"));
    return STATUS_SUCCESS;

}
