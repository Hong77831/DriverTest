#include "Driver.h"

#pragma INITCODE
extern "C" NTSTATUS DriverEntry(
    IN PDRIVER_OBJECT pDriverObject,
    IN PUNICODE_STRING pRegistryPath)
{
    UNREFERENCED_PARAMETER(pRegistryPath);
    NTSTATUS status;
    KdPrint(("Enter DriverEntry\n"));

    //设置卸载函数
    pDriverObject->DriverUnload = HelloDDKUnload;

    //设置派遣函数
    for (int i = 0; i < arraysize(pDriverObject->MajorFunction); ++i)
        pDriverObject->MajorFunction[i] = HelloDDKDispatchRoutin;

    pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = HelloDDKDeviceIOControl;

    //创建驱动设备对象
    status = CreateDevice(pDriverObject);

    KdPrint(("Leave DriverEntry\n"));
    return status;
}

#pragma INITCODE
NTSTATUS CreateDevice(
    IN PDRIVER_OBJECT	pDriverObject)
{
    NTSTATUS status;
    PDEVICE_OBJECT pDevObj;
    PDEVICE_EXTENSION pDevExt;

    //创建设备名称
    UNICODE_STRING devName;
    RtlInitUnicodeString(&devName, L"\\Device\\MyDDKDevice");

    //创建设备
    status = IoCreateDevice(pDriverObject,
        sizeof(DEVICE_EXTENSION),
        &devName,
        FILE_DEVICE_UNKNOWN,
        0, TRUE,
        &pDevObj);
    if (!NT_SUCCESS(status))
        return status;

    pDevObj->Flags |= DO_DIRECT_IO;
    pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
    pDevExt->pDevice = pDevObj;
    pDevExt->ustrDeviceName = devName;

    //创建符号链接
    UNICODE_STRING symLinkName;
    RtlInitUnicodeString(&symLinkName, L"\\??\\HelloDDK");
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
        //删除符号链接
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
        KdPrint(("\t%s\n", irpname[type]));

    NTSTATUS status = STATUS_SUCCESS;
    // 完成IRP
    pIrp->IoStatus.Status = status;
    pIrp->IoStatus.Information = 0;	// bytes xfered
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    KdPrint(("Leave HelloDDKDispatchRoutin\n"));

    return status;
}

#pragma PAGEDCODE
VOID Time_Test()
{
    LARGE_INTEGER current_system_time;
    //得到当前系统时间
    KeQuerySystemTime(&current_system_time);

    LARGE_INTEGER current_local_time;
    //从系统时间转换成当地时区时间
    ExSystemTimeToLocalTime(&current_system_time, &current_local_time);

    TIME_FIELDS current_time_info;
    //由当地时区时间得到月日年信息
    RtlTimeToTimeFields(&current_local_time, &current_time_info);

    //显示年月日等信息
    KdPrint(("Current year:%d\n", current_time_info.Year));
    KdPrint(("Current month:%d\n", current_time_info.Month));
    KdPrint(("Current day:%d\n", current_time_info.Day));
    KdPrint(("Current Hour:%d\n", current_time_info.Hour));
    KdPrint(("Current Minute:%d\n", current_time_info.Minute));
    KdPrint(("Current Second:%d\n", current_time_info.Second));
    KdPrint(("Current Milliseconds:%d\n", current_time_info.Milliseconds));
    KdPrint(("Current Weekday:%d\n", current_time_info.Weekday));
}

#pragma PAGEDCODE
NTSTATUS HelloDDKDeviceIOControl(IN PDEVICE_OBJECT pDevObj,
    IN PIRP pIrp)
{
    NTSTATUS status = STATUS_SUCCESS;
    KdPrint(("Enter HelloDDKDeviceIOControl\n"));

    //得到当前堆栈
    PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);
    //得到输入缓冲区大小
    ULONG cbin = stack->Parameters.DeviceIoControl.InputBufferLength;
    //得到输出缓冲区大小
    ULONG cbout = stack->Parameters.DeviceIoControl.OutputBufferLength;
    //得到IOCTL码
    ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;
    UNREFERENCED_PARAMETER(cbin);
    UNREFERENCED_PARAMETER(cbout);

    PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)
        pDevObj->DeviceExtension;
    UNREFERENCED_PARAMETER(pDevExt);

    ULONG info = 0;

    switch (code)
    {    // process request
    case IOCTL_TIME_TEST:
    {
        KdPrint(("IOCTL_TIME_TEST\n"));
        Time_Test();
        break;
    }
    default:
        status = STATUS_INVALID_VARIANT;
    }

    // 完成IRP
    pIrp->IoStatus.Status = status;
    pIrp->IoStatus.Information = info;    // bytes xfered
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    KdPrint(("Leave HelloDDKDeviceIOControl\n"));

    return status;
}

