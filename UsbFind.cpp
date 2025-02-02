#include "stdafx.h"

#include <cfgmgr32.h>
#include <initguid.h>
#include <devpropdef.h>
#include <devpkey.h>
#include <ntddser.h>
#include "curvebug.h"


WCHAR* MyPidVid = L"\\\\?\\USB#VID_0483&PID_5740";

HANDLE FindCommPort() {

    DWORD ret;
    ULONG deviceInterfaceListBufferLength;

    //
    // Determine the size (in characters) of buffer required for to fetch a list of
    // all GUID_DEVINTERFACE_COMPORT device interfaces present on the system.
    //
    ret = CM_Get_Device_Interface_List_SizeW(
        &deviceInterfaceListBufferLength,
        (LPGUID)&GUID_DEVINTERFACE_COMPORT,
        NULL,
        CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
    if (ret != CR_SUCCESS) Damnit(NULL);

    //
    // Allocate buffer of the determined size.
    //
    PWCHAR deviceInterfaceListBuffer = (PWCHAR)malloc(deviceInterfaceListBufferLength * sizeof(WCHAR));
    if (deviceInterfaceListBuffer == NULL) Damnit(NULL);

    //
    // Fetch the list of all GUID_DEVINTERFACE_COMPORT device interfaces present
    // on the system.
    //
    ret = CM_Get_Device_Interface_ListW(
        (LPGUID)&GUID_DEVINTERFACE_COMPORT,
        NULL,
        deviceInterfaceListBuffer,
        deviceInterfaceListBufferLength,
        CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
    if (ret != CR_SUCCESS) Damnit(NULL);

    //
    // Iterate through the list, examining one interface at a time
    //
    PWCHAR currentInterface = deviceInterfaceListBuffer;
    while (*currentInterface) {
        //
        // Fetch the DEVPKEY_DeviceInterface_Serial_PortName for this interface
        //
        CONFIGRET configRet;
        DEVPROPTYPE devPropType;
        PWCHAR devPropBuffer;
        ULONG devPropSize = 0;

        // First, get the size of buffer required
        configRet = CM_Get_Device_Interface_PropertyW(
            currentInterface,
            &DEVPKEY_DeviceInterface_Serial_PortName,
            &devPropType,
            NULL,
            &devPropSize,
            0);
        if (configRet != CR_BUFFER_SMALL) Damnit(NULL);

        // Allocate the buffer
        devPropBuffer = (PWCHAR)malloc(devPropSize);
        if (devPropBuffer == NULL) Damnit(NULL);

        configRet = CM_Get_Device_Interface_PropertyW(
            currentInterface,
            &DEVPKEY_DeviceInterface_Serial_PortName,
            &devPropType,
            (PBYTE)devPropBuffer,
            &devPropSize,
            0);
        if (configRet != CR_SUCCESS) Damnit(NULL);

        // Verify the value is the correct type and size
        if ((devPropType != DEVPROP_TYPE_STRING) || (devPropSize < sizeof(WCHAR))) 
            Damnit(NULL);

        // Now, check if the interface is the one we are interested in
        if (wcsncmp(currentInterface, MyPidVid, wcslen(MyPidVid)) == 0){
            free(devPropBuffer);
            break;
        }

        // Advance to the next string (past the terminating NULL)
        currentInterface += wcslen(currentInterface) + 1;
        free(devPropBuffer);
    }

    //
    // currentInterface now either points to NULL (there was no match and we iterated
    // over all interfaces without a match) - or, it points to the interface with
    // the friendly name UART0, in which case we can open it.
    //
    if (*currentInterface == L'\0') Damnit(L"Couldn't find device");

    //
    // Now open the device interface as we would a COMx style serial port.
    //
    HANDLE portHandle = CreateFileW(
        currentInterface,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);
    if (portHandle == INVALID_HANDLE_VALUE) Damnit(L"I/O Open failed");
    COMMTIMEOUTS timeouts;
    timeouts.ReadIntervalTimeout = 10;
    timeouts.ReadTotalTimeoutMultiplier = 1;
    timeouts.ReadTotalTimeoutConstant = 400;
    timeouts.WriteTotalTimeoutConstant = 200;
    timeouts.WriteTotalTimeoutMultiplier = 1;

    if (!SetCommTimeouts(portHandle, &timeouts)) Damnit(NULL);

    free(deviceInterfaceListBuffer);

    return portHandle;

}
