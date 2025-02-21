#include "stdafx.h"

#include <cfgmgr32.h>
#include <devpkey.h>
#include <ntddser.h>
#include "curvebug.h"

#pragma comment(lib, "setupapi.lib")

wchar_t* VidPids[] = {

    L"\\\\?\\usb#vid_0483&pid_5740",
    L"\\\\?\\usb#vid_0483&pid_5741"

};


HANDLE OpenSpecifiedDevice(
	IN       HDEVINFO                    HardwareDeviceInfo,
	IN       PSP_INTERFACE_DEVICE_DATA   DeviceInterfaceData,
	OUT		 wchar_t** devName,
	OUT      DWORD& devInst,
	rsize_t	 bufsize
)
{
	PSP_INTERFACE_DEVICE_DETAIL_DATA     functionClassDeviceData = NULL;
	ULONG                                requiredLength = 0;
	HANDLE								 hOut = INVALID_HANDLE_VALUE;
	SP_DEVINFO_DATA						 devInfoData;

	//
	// allocate a function class device data structure to receive the
	// goods about this particular device.
	//
	SetupDiGetInterfaceDeviceDetail(
		HardwareDeviceInfo,
		DeviceInterfaceData,
		NULL, // probing so no output buffer yet
		0, // probing so output buffer length of zero
		&requiredLength,
		NULL); // not interested in the specific dev-node



	functionClassDeviceData = (PSP_INTERFACE_DEVICE_DETAIL_DATA)malloc(requiredLength);
	if (functionClassDeviceData == NULL) Damnit(NULL);
	functionClassDeviceData->cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);
	devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

	//
	// Retrieve the information from Plug and Play.
	//
	if (!SetupDiGetInterfaceDeviceDetail(
		HardwareDeviceInfo,
		DeviceInterfaceData,
		functionClassDeviceData,
		requiredLength,
		&requiredLength,
		&devInfoData))
	{
		free(functionClassDeviceData);
		return INVALID_HANDLE_VALUE;
	}

	*devName = NULL;
	for (int i = 0; i < sizeof(VidPids) / sizeof(VidPids[0]); i++) {
		wchar_t* PidVidStr = VidPids[i];
		if (wcsncmp(functionClassDeviceData->DevicePath, PidVidStr, wcslen(PidVidStr)) == 0) {
			*devName = _wcsdup(functionClassDeviceData->DevicePath); // must be freed by caller not good form.
			hOut = (HANDLE)3;
			break;
		}
	}

	free(functionClassDeviceData);
	return hOut;
}


HANDLE OpenUsbDevice(LPGUID  pGuid, wchar_t** devpath, DWORD& outDevInst, rsize_t bufsize)
/*++
Routine Description:

Do the required PnP things in order to find the next available proper device in the system at this time.

Arguments:
pGuid:      ptr to GUID registered by the driver itself
devpath: the generated name for this device

Return Value:

return HANDLE if the open and initialization was successful,
else INVALID_HANDLE_VALUE.
--*/
{
	ULONG                    NumberDevices = 20;
	HANDLE                   hOut = INVALID_HANDLE_VALUE;
	HDEVINFO                 hDevInfo;
	SP_INTERFACE_DEVICE_DATA deviceInterfaceData;
	ULONG                    i;
	BOOLEAN                  done;

	//
	// Open a handle to the plug and play dev node.
	// SetupDiGetClassDevs() returns a device information set that contains info on all
	// installed devices of a specified class.
	//
	hDevInfo = SetupDiGetClassDevs(
		pGuid,
		NULL, // Define no enumerator (global)
		NULL, // Define no
		(DIGCF_PRESENT | // Only Devices present
			DIGCF_INTERFACEDEVICE)); // Function class devices.
	if (hDevInfo == INVALID_HANDLE_VALUE)
		Damnit(NULL);
	//
	// Take a wild guess at the number of devices we have;
	// Be prepared to realloc and retry if there are more than we guessed
	//
	deviceInterfaceData.cbSize = sizeof(SP_INTERFACE_DEVICE_DATA);

	for (i = 0, done = FALSE; !done;) {
		NumberDevices += 20;  // keep increasing the number of devices until we reach the limit
		for (; i < NumberDevices; i++) {

			// SetupDiEnumDeviceInterfaces() returns information about device interfaces
			// exposed by one or more devices. Each call returns information about one interface;
			// the routine can be called repeatedly to get information about several interfaces
			// exposed by one or more devices.
			if (SetupDiEnumDeviceInterfaces(
				hDevInfo,   // pointer to a device information set
				NULL,       // pointer to an SP_DEVINFO_DATA, We don't care about specific PDOs
				pGuid,      // pointer to a GUID
				i,          //zero-based index into the list of interfaces in the device information set
				&deviceInterfaceData)) // pointer to a caller-allocated buffer that contains a completed SP_DEVICE_INTERFACE_DATA structure
			{
				// open the device
				hOut = OpenSpecifiedDevice(hDevInfo, &deviceInterfaceData, devpath, outDevInst, bufsize);
				if (hOut != INVALID_HANDLE_VALUE)
				{
					done = TRUE;
					break;
				}
			}
			else {
				// EnumDeviceInterfaces error
				if (ERROR_NO_MORE_ITEMS == GetLastError())
					done = TRUE;
			}
		}  // end-for
	}

	SetupDiDestroyDeviceInfoList(hDevInfo);
	return hOut;
}



HANDLE FindCommPort()
{
	wchar_t* devpath = NULL;
	DWORD dwDevInst = 0;
	int err = ERROR_SUCCESS;
	HANDLE portHandle;

	//
	// Find and open a handle to the Comm driver object.
	//
	if (OpenUsbDevice((LPGUID)&GUID_DEVINTERFACE_COMPORT, &devpath, dwDevInst, MAX_PATH) == INVALID_HANDLE_VALUE)
		Damnit(L"Couldn't find device");

	portHandle = CreateFile(devpath,
		GENERIC_READ | GENERIC_WRITE,
		NULL,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);
	if (portHandle == INVALID_HANDLE_VALUE)
		Damnit(L"I/O Open failed");
	COMMTIMEOUTS timeouts;
	timeouts.ReadIntervalTimeout = 40;
	timeouts.ReadTotalTimeoutMultiplier = 1;
	timeouts.ReadTotalTimeoutConstant = 200;
	timeouts.WriteTotalTimeoutConstant = 200;
	timeouts.WriteTotalTimeoutMultiplier = 1;

	if (!SetCommTimeouts(portHandle, &timeouts))
		Damnit(NULL);
	PurgeComm(portHandle, PURGE_RXCLEAR);


	if (devpath) free(devpath);

	return portHandle;

}

