#include "Driver.h"

#pragma INITCODE
extern "C" NTSTATUS DriverEntry(
    IN PDRIVER_OBJECT pDriverObject,
    IN PUNICODE_STRING pRegistryPath)
{
    UNREFERENCED_PARAMETER(pRegistryPath);
    NTSTATUS status;
    KdPrint(("Enter DriverEntry\n"));

    //����ж�غ���
    pDriverObject->DriverUnload = HelloDDKUnload;

    //������ǲ����
    pDriverObject->MajorFunction[IRP_MJ_CREATE] = HelloDDKDispatchRoutin;
    pDriverObject->MajorFunction[IRP_MJ_CLOSE] = HelloDDKDispatchRoutin;
    pDriverObject->MajorFunction[IRP_MJ_WRITE] = HelloDDKDispatchRoutin;
    pDriverObject->MajorFunction[IRP_MJ_READ] = HelloDDKRead;
    pDriverObject->MajorFunction[IRP_MJ_CLEANUP] = HelloDDKDispatchRoutin;
    pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = HelloDDKDispatchRoutin;
    pDriverObject->MajorFunction[IRP_MJ_SET_INFORMATION] = HelloDDKDispatchRoutin;
    pDriverObject->MajorFunction[IRP_MJ_SHUTDOWN] = HelloDDKDispatchRoutin;
    pDriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = HelloDDKDispatchRoutin;

    //���������豸����
    status = CreateDevice(pDriverObject);

    KdPrint(("Leave DriverEntry\n"));
    return status;
}

#pragma LOCKEDCODE
VOID OnTimerDpc(IN PKDPC pDpc,
    IN PVOID pContext,
    IN PVOID SysArg1,
    IN PVOID SysArg2)
{
    UNREFERENCED_PARAMETER(pDpc);
    UNREFERENCED_PARAMETER(SysArg1);
    UNREFERENCED_PARAMETER(SysArg2);
    PDEVICE_OBJECT pDevObj = (PDEVICE_OBJECT)pContext;
    PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;

    PIRP currentPendingIRP = pdx->currentPendingIRP;

    KdPrint(("Cancel the current pending irp!\n"));

    //�������״̬ΪSTATUS_CANCELLED
    currentPendingIRP->IoStatus.Status = STATUS_CANCELLED;
    currentPendingIRP->IoStatus.Information = 0;    // bytes xfered
    IoCompleteRequest(currentPendingIRP, IO_NO_INCREMENT);
}

#pragma INITCODE
NTSTATUS CreateDevice(
    IN PDRIVER_OBJECT	pDriverObject)
{
    NTSTATUS status;
    PDEVICE_OBJECT pDevObj;
    PDEVICE_EXTENSION pDevExt;

    //�����豸����
    UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Device\\MyDDKDevice");

    //�����豸
    status = IoCreateDevice(pDriverObject,
        sizeof(DEVICE_EXTENSION),
        &devName,
        FILE_DEVICE_UNKNOWN,
        0, TRUE,
        &pDevObj);
    if (!NT_SUCCESS(status))
        return status;

    pDevObj->Flags |= DO_BUFFERED_IO;
    pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
    pDevExt->pDevice = pDevObj;
    pDevExt->ustrDeviceName = devName;

    KeInitializeTimer(&pDevExt->pollingTimer);

    KeInitializeDpc(&pDevExt->pollingDPC,
        OnTimerDpc,
        (PVOID)pDevObj);

    //������������
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
VOID HelloDDKUnload(IN PDRIVER_OBJECT pDriverObject)
{
    PDEVICE_OBJECT	pNextObj;
    KdPrint(("Enter DriverUnload\n"));
    pNextObj = pDriverObject->DeviceObject;
    while (pNextObj != NULL)
    {
        PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)
            pNextObj->DeviceExtension;

        //ɾ����������
        UNICODE_STRING pLinkName = pDevExt->ustrSymLinkName;
        IoDeleteSymbolicLink(&pLinkName);

        pNextObj = pNextObj->NextDevice;
        IoDeleteDevice(pDevExt->pDevice);
    }
}

#pragma PAGEDCODE
NTSTATUS HelloDDKDispatchRoutin(IN PDEVICE_OBJECT pDevObj,
    IN PIRP pIrp)
{
    UNREFERENCED_PARAMETER(pDevObj);
    KdPrint(("Enter HelloDDKDispatchRoutin\n"));

    PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);
    //����һ���ַ���������IRP���Ͷ�Ӧ����
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
        KdPrint(("\t%s\n", irpname[type]));


    //��һ��IRP�ļ򵥲������������ܶ�IRP�����ӵĲ���
    NTSTATUS status = STATUS_SUCCESS;
    // ���IRP
    pIrp->IoStatus.Status = status;
    pIrp->IoStatus.Information = 0;	// bytes xfered
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    KdPrint(("Leave HelloDDKDispatchRoutin\n"));

    return status;
}

NTSTATUS HelloDDKRead(IN PDEVICE_OBJECT pDevObj,
    IN PIRP pIrp)
{
    KdPrint(("Enter HelloDDKRead\n"));

    PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)
        pDevObj->DeviceExtension;

    //��IRP����Ϊ����
    IoMarkIrpPending(pIrp);

    //�������IRP��¼����
    pDevExt->currentPendingIRP = pIrp;

    //����3��ĳ�ʱ
    ULONG ulMicroSecond = 3000000;

    //��32λ����ת����64λ����
    LARGE_INTEGER timeout; 
    timeout .QuadPart = (-10 * ulMicroSecond);

    KeSetTimer(
        &pDevExt->pollingTimer,
        timeout,
        &pDevExt->pollingDPC);

    KdPrint(("Leave HelloDDKRead\n"));

    //����pending״̬
    return STATUS_PENDING;
}