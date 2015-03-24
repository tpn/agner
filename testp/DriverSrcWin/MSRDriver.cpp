/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;
; MSRDriver.cpp                                          © 2012-03-02 Agner Fog
;
; Device driver for access to Model-specific registers and control registers
; in Windows 2000 and later, 32 bit and 64 bit x86 platforms.
;
; Acknowledgments:
; The first version of this driver was written in 2004 by my clever students 
; Søren Stentoft Hansen and Kian Karas using the Four-F Kernel Mode Driver Kit
; found at www.website.masmforum.com/tutorials/kmdtute/
;
; © 2005 - 2012 GNU General Public License www.gnu.org/licenses
;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

#if defined(_WIN64)          // Definitions added
#define _AMD64_  
#else
#define _X86_
#endif

#include <ntddk.h>           // Windows driver kit
#include "intrin1.h"         // Intrinsic functions
#include "MSRDriver.h"       // Structures shared with calling program

// Define 32/64 bit integer
#ifndef _SIZE_T_DEFINED
#ifdef  _WIN64
typedef unsigned long long size_t;
#else
typedef unsigned int     size_t;
#endif
#define _SIZE_T_DEFINED
#endif

// Functions defined below
extern "C" size_t ReadCR(int r);                 // read control register
extern "C" void WriteCR(int r, size_t value);    // write control register



UNICODE_STRING g_usDeviceName = {
    40, 42, L"\\Device\\devMSRDriver"};

UNICODE_STRING  g_usSymbolicLinkName = {
    30, 32, L"\\??\\slMSRDriver"};


NTSTATUS DispatchCreateClose(IN PDEVICE_OBJECT  DeviceObject, IN PIRP  Irp) {
    // CreateFile was called, to get device handle or
    // CloseHandle was called, to close device handle
    // In both cases we are in user process context here

    Irp->IoStatus.Status = STATUS_SUCCESS; 
    Irp->IoStatus.Information = 0; 
    IoCompleteRequest(Irp, IO_NO_INCREMENT); 
    return STATUS_SUCCESS; 
} 


NTSTATUS DispatchControl(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp) {
    // DeviceIoControl was called
    // We are in user process context here

    NTSTATUS status = STATUS_SUCCESS;
    ULONG dwInArrSize;
    ULONG dwOutArrSize;
    int i, n1, n2, reg;
    EMSR_COMMAND command;
    long long InValue, OutValue;
    union {
        size_t val;  // value of cr4 register, 32 or 64 bits
        int lo;      // low 32 bits of value
    } cr4val;

    PIO_STACK_LOCATION pIOStack = IoGetCurrentIrpStackLocation(pIrp);

#define IOCTL_MSR_DRIVER  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_READ_ACCESS + FILE_WRITE_ACCESS)

    if (pIOStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_MSR_DRIVER) {
        // the I/O control code is ours

        // get input array size       
        dwInArrSize = pIOStack->Parameters.DeviceIoControl.InputBufferLength;

        // get output array size
        dwOutArrSize = pIOStack->Parameters.DeviceIoControl.OutputBufferLength;

        // get pointer to input/output array
        SMSRInOut * pInOut = (SMSRInOut*)pIrp->AssociatedIrp.SystemBuffer;

        // number of input and output records
        n1 = dwInArrSize  / sizeof(SMSRInOut);
        n2 = dwOutArrSize / sizeof(SMSRInOut);

        // command loop
        for (i = 0; i < n1; i++, pInOut++) {

            // get command
            command = pInOut->msr_command;
            // get register number
            reg = pInOut->register_number;
            // get data
            InValue = pInOut->value;
            OutValue = 0;

            // dispatch command
            switch (command) {

            case MSR_IGNORE:    // do nothing
                break;

            case MSR_STOP:      // stop loop
                i = n1; break;

            case MSR_READ:      // read model-specific register
                OutValue = __readmsr(reg);
                break;

            case MSR_WRITE:     // write model-specific register
                __writemsr(reg, InValue);
                break;

            case CR_READ:       // read control register
                OutValue = (long long)ReadCR(reg);
                break;

            case CR_WRITE:      // write control register
                WriteCR(reg, (size_t)InValue);
                break;

            case PMC_ENABLE:    // Enable RDPMC and RDTSC instructions
                cr4val.val = __readcr4();  // Read CR4
                cr4val.lo |= 0x100;        // Enable RDPMC
                cr4val.lo &= ~4;           // Enable RDTSC
                __writecr4(cr4val.val);    // Write CR4
                break;

            case PMC_DISABLE:   // Disable RDPMC instruction (RDTSC remains enabled)
                cr4val.val = __readcr4();  // Read CR4
                cr4val.lo &= ~0x100;       // Disable RDPMC
                //cr4val.lo |= 4;          // Disable RDTSC
                __writecr4(cr4val.val);    // Write CR4
                break;

            case PROC_GET:      // Which processor number am I running on (in multiprocessor system)
                OutValue = KeGetCurrentProcessorNumber();
                break;

            case PROC_SET: {    // Fix to certain processor number (in multiprocessor system)
                size_t affinity = (size_t)1 << InValue;
                OutValue = ZwSetInformationThread((HANDLE)(-2), ThreadAffinityMask, &affinity, sizeof(affinity));
                break;}

            default: // unknown command
                status = STATUS_INVALID_DEVICE_REQUEST;
                break;
            }

            // save data
            if (i < n2) {
                pInOut->value = OutValue;
            }
            else {
                if (command == MSR_READ || command == CR_READ) {
                    status = STATUS_BUFFER_TOO_SMALL;
                }
            }
        }
    }
    else {
        status = STATUS_INVALID_DEVICE_REQUEST;
    }

    pIrp->IoStatus.Status = status;

    // number of bytes returned
    if (i > n2) i = n2;
    pIrp->IoStatus.Information = (ULONG_PTR)(i * sizeof(SMSRInOut));

    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    return status;
}


void DriverUnload (PDRIVER_OBJECT pDriverObject) {
    // ControlService,,SERVICE_CONTROL_STOP was called
    // We are in System process (pid = 8) context here
    IoDeleteSymbolicLink (&g_usSymbolicLinkName);
    IoDeleteDevice (pDriverObject->DeviceObject);
}

extern "C"
NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pusRegistryPath) {
    // StartService was called
    // We are in System process (pid = 8) context here

    NTSTATUS status, s2;
    PDEVICE_OBJECT pDeviceObject;

    status = STATUS_DEVICE_CONFIGURATION_ERROR;

    // Register device (in this case it's virtual)
    s2 = IoCreateDevice (pDriverObject, 0, &g_usDeviceName, FILE_DEVICE_UNKNOWN, 0, false, &pDeviceObject);

    if (s2 == STATUS_SUCCESS) {

        s2 = IoCreateSymbolicLink (&g_usSymbolicLinkName, &g_usDeviceName);

        if (s2 == STATUS_SUCCESS) {

            // Announce dispatch routines
            pDriverObject->DriverUnload = DriverUnload;
            pDriverObject->MajorFunction[IRP_MJ_CREATE] = DispatchCreateClose;
            pDriverObject->MajorFunction[IRP_MJ_CLOSE] = DispatchCreateClose;
            pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchControl;

            status = STATUS_SUCCESS;
        }
        else {
            IoDeleteDevice (pDeviceObject);
        }
    }
    return status;
}


// read control register
size_t ReadCR(int r){
    switch (r) {
    case 0:
        return __readcr0();
    case 2:
        return __readcr2();
    case 3:
        return __readcr3();
    case 4:
        return __readcr4();
#ifdef _M_X64 // cr8 only in 64 bit mode
    case 8:
        return __readcr8();
#endif
    }
    // default = 0   
    return 0;
};

void WriteCR(int r, size_t value) {
    // write control register
    switch (r) {
    case 0:
        __writecr0(value);
        break;
    case 3:
        __writecr3(value);
        break;
    case 4:
        __writecr4(value);
        break;
#ifdef _M_X64 // cr8 only in 64 bit mode
    case 8:
        __writecr8(value);
        break;
#endif
    default:
        ; // wrong value. ignore
    }
}
