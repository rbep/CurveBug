/***************************************************************************
 *
 *  Project  CurveBug
 *
 * Copyright (C) Robert Puckette, 2024-2025
 *
 * This software is licensed as described in the file COPYING.txt, which
 * you should have received as part of this distribution.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 *
 ***************************************************************************/
#include "stdafx.h"

#include <ntddser.h>
#include "curvebug.h"
#include "UsbFind.h"

#pragma comment(lib, "setupapi.lib")

PTCHAR VidPids[] = {
	L"\\\\?\\usb#vid_16d0&pid_13f9",  // 5840-5113 in decimal
	//L"\\\\?\\usb#vid_0483&pid_5740"  // default ST Micro PID (should phase out)
};

void SetupComm(HANDLE hFile) {
	if (hFile == INVALID_HANDLE_VALUE)
		return;
	COMMTIMEOUTS timeouts;
	timeouts.ReadIntervalTimeout = 40;
	timeouts.ReadTotalTimeoutMultiplier = 1;
	timeouts.ReadTotalTimeoutConstant = 200;
	timeouts.WriteTotalTimeoutConstant = 200;
	timeouts.WriteTotalTimeoutMultiplier = 1;
	if (!SetCommTimeouts(hFile, &timeouts)) // set the timeouts
		Damnit(NULL);
	PurgeComm(hFile, PURGE_RXCLEAR); // clear out any garbage
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
		return NULL;
	}

	for (int i = 0; i < sizeof(VidPids) / sizeof(VidPids[0]); i++) {
		PTCHAR PidVidStr = VidPids[i];
		if (wcsncmp(functionClassDeviceData->DevicePath, PidVidStr, wcslen(PidVidStr)) == 0) {
			devName = _wcsdup(functionClassDeviceData->DevicePath); // must be freed by caller not good form.
			break;
		}
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

	SetupDiDestroyDeviceInfoList(hDevInfo); // clean up the device info list

	if (!devpath)
		return AlternateFindCommPort(); // try the alternate method

	// open the device
	portHandle = CreateFile(devpath,
		GENERIC_READ | GENERIC_WRITE,
		NULL,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);
	if (portHandle != INVALID_HANDLE_VALUE) {
		SetupComm(portHandle);
	}

	if (devpath) free(devpath); // free the device path string

	return portHandle;

}
HANDLE AlternateFindCommPort()
{
	HANDLE portHandle = INVALID_HANDLE_VALUE;
	TCHAR portName[100];
	int PortNum = 100;
	DWORD Received;
	// open the device
	while (portHandle == INVALID_HANDLE_VALUE && PortNum > 1) {
		swprintf(portName, sizeof(portName), L"\\\\.\\COM%d", --PortNum);
		portHandle = CreateFile(portName,
			GENERIC_READ | GENERIC_WRITE,
			NULL,
			NULL,
			OPEN_EXISTING,
			0,
			NULL);
		if (portHandle != INVALID_HANDLE_VALUE) {
			SetupComm(portHandle);
			WriteFile(portHandle, "?", 1, NULL, NULL);
			Received = 0;
			ReadFile(portHandle, portName, sizeof(portName), &Received, NULL);
			if (Received == 4) {
				
			}
			else {
				CloseHandle(portHandle);
				portHandle = INVALID_HANDLE_VALUE;
			}
		}
	}

	return portHandle;
}



