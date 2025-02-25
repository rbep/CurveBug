// BugZapper.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <ntddser.h>
// Windows Header Files:
#include <windows.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <cfgmgr32.h>
#include <setupapi.h>
#include <initguid.h>


#pragma comment(lib, "setupapi.lib")

HANDLE FindCommPort(void);


int main()
{
	HANDLE port = FindCommPort();
	WriteFile(port, "E", 1, NULL, NULL);

    std::cout << "Zapped!\n";

}

void Damnit(const char *msg) {
	std::cout << msg;

	exit(1);
}


PTCHAR NameOfMyCommDevice(
	IN       HDEVINFO                    HardwareDeviceInfo,
	IN       PSP_INTERFACE_DEVICE_DATA   DeviceInterfaceData)
{
	PTCHAR devName = NULL;
	PSP_INTERFACE_DEVICE_DETAIL_DATA     functionClassDeviceData = NULL;
	ULONG                                requiredLength = 0;
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
	if (functionClassDeviceData == NULL) Damnit("Strange error");
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
		return NULL;
	}

	const wchar_t* PidVidStr = L"\\\\?\\usb#vid_16d0&pid_13f9";
	if (wcsncmp(functionClassDeviceData->DevicePath, PidVidStr, wcslen(PidVidStr)) == 0){
			devName = _wcsdup(functionClassDeviceData->DevicePath); // must be freed by caller not good form.
	}

	free(functionClassDeviceData);
	return devName;
}


HANDLE FindCommPort()
{
	PTCHAR devpath = NULL;
	HANDLE portHandle;
	HDEVINFO                 hDevInfo;
	SP_INTERFACE_DEVICE_DATA deviceInterfaceData;
	ULONG                    devIndex = 0;

	//
	// Open a handle to the plug and play dev node.
	// SetupDiGetClassDevs() returns a device information set that contains info on all
	// installed devices of a specified class.
	//
	hDevInfo = SetupDiGetClassDevs(
		(LPGUID)&GUID_DEVINTERFACE_COMPORT,
		NULL, // Define no enumerator (global)
		NULL, // Define no
		(DIGCF_PRESENT | // Only Devices present
			DIGCF_INTERFACEDEVICE)); // Function class devices.

	//
	// Take a wild guess at the number of devices we have;
	// Be prepared to realloc and retry if there are more than we guessed
	//
	deviceInterfaceData.cbSize = sizeof(SP_INTERFACE_DEVICE_DATA);

	// SetupDiEnumDeviceInterfaces() returns information about device interfaces
	// exposed by one or more devices. Each call returns information about one interface;
	// the routine can be called repeatedly to get information about several interfaces
	// exposed by one or more devices.
	while (NULL !=
		SetupDiEnumDeviceInterfaces(
			hDevInfo,   // pointer to a device information set
			NULL,       // pointer to an SP_DEVINFO_DATA, We don't care about specific PDOs
			(LPGUID)&GUID_DEVINTERFACE_COMPORT,      // pointer to a GUID
			devIndex,          //zero-based index into the list of interfaces in the device information set
			&deviceInterfaceData)) // pointer to a caller-allocated buffer that contains a completed SP_DEVICE_INTERFACE_DATA structure
	{
		// open the device
		devpath = NameOfMyCommDevice(hDevInfo, &deviceInterfaceData);
		if (devpath != NULL)
			break;
		devIndex++;
	}

	if (!devpath)
		Damnit("Couldn't Find Device");

	SetupDiDestroyDeviceInfoList(hDevInfo);
	portHandle = CreateFile(devpath,
		GENERIC_READ | GENERIC_WRITE,
		NULL,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);
	if (portHandle == INVALID_HANDLE_VALUE)
		Damnit("I/O Open failed");

	// setup some reasonable timeouts on the comms. Allows for semi-graceful error recovery
	COMMTIMEOUTS timeouts;
	timeouts.ReadIntervalTimeout = 40;
	timeouts.ReadTotalTimeoutMultiplier = 1;
	timeouts.ReadTotalTimeoutConstant = 200;
	timeouts.WriteTotalTimeoutConstant = 200;
	timeouts.WriteTotalTimeoutMultiplier = 1;

	if (!SetCommTimeouts(portHandle, &timeouts))
		Damnit("Timeout issue");
	PurgeComm(portHandle, PURGE_RXCLEAR);

	if (devpath) free(devpath);

	return portHandle;

}


