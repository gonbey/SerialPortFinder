// ConsoleApplication5.cpp : このファイルには 'main' 関数が含まれています。プログラム実行の開始と終了がそこで行われます。
//
#pragma comment(lib, "SetupAPI.lib")
#define _CRT_SECURE_NO_WARNINGS
#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>
#include <tchar.h>

#include <setupapi.h>
#include <initguid.h>

#include <stdio.h>

	int test(LPCTSTR vidpid);
	// This is the GUID for the USB device class
	DEFINE_GUID(GUID_DEVINTERFACE_USB_DEVICE,
		0xA5DCBF10L, 0x6530, 0x11D2, 0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED);
	// (A5DCBF10-6530-11D2-901F-00C04FB951ED)

	int main() {
		test(_T("vid_04d8&pid_000a"));
	}

	// "vid_04d8&pid_000a")
	int test(LPCTSTR vidpid)
	{
		HDEVINFO                         hDevInfo;
		SP_DEVICE_INTERFACE_DATA         DevIntfData;
		PSP_DEVICE_INTERFACE_DETAIL_DATA DevIntfDetailData;
		SP_DEVINFO_DATA                  DevData;

		DWORD dwSize, dwType, dwMemberIdx;
		HKEY hKey;
		BYTE lpData[1024];

		// We will try to get device information set for all USB devices that have a
		// device interface and are currently present on the system (plugged in).
		hDevInfo = SetupDiGetClassDevs(
			&GUID_DEVINTERFACE_USB_DEVICE, NULL, 0, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);

		if (hDevInfo != INVALID_HANDLE_VALUE)
		{
			// Prepare to enumerate all device interfaces for the device information
			// set that we retrieved with SetupDiGetClassDevs(..)
			DevIntfData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
			dwMemberIdx = 0;

			// Next, we will keep calling this SetupDiEnumDeviceInterfaces(..) until this
			// function causes GetLastError() to return  ERROR_NO_MORE_ITEMS. With each
			// call the dwMemberIdx value needs to be incremented to retrieve the next
			// device interface information.

			SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &GUID_DEVINTERFACE_USB_DEVICE,
				dwMemberIdx, &DevIntfData);

			while (GetLastError() != ERROR_NO_MORE_ITEMS)
			{
				// As a last step we will need to get some more details for each
				// of device interface information we are able to retrieve. This
				// device interface detail gives us the information we need to identify
				// the device (VID/PID), and decide if it's useful to us. It will also
				// provide a DEVINFO_DATA structure which we can use to know the serial
				// port name for a virtual com port.

				DevData.cbSize = sizeof(DevData);

				// Get the required buffer size. Call SetupDiGetDeviceInterfaceDetail with
				// a NULL DevIntfDetailData pointer, a DevIntfDetailDataSize
				// of zero, and a valid RequiredSize variable. In response to such a call,
				// this function returns the required buffer size at dwSize.

				SetupDiGetDeviceInterfaceDetail(
					hDevInfo, &DevIntfData, NULL, 0, &dwSize, NULL);

				// Allocate memory for the DeviceInterfaceDetail struct. Don't forget to
				// deallocate it later!
				DevIntfDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize);
				DevIntfDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

				if (SetupDiGetDeviceInterfaceDetail(hDevInfo, &DevIntfData,
					DevIntfDetailData, dwSize, &dwSize, &DevData))
				{
					// Finally we can start checking if we've found a useable device,
					// by inspecting the DevIntfDetailData->DevicePath variable.
					// The DevicePath looks something like this:
					//
					// \\?\usb#vid_04d8&pid_0033#5&19f2438f&0&2#{a5dcbf10-6530-11d2-901f-00c04fb951ed}
					//
					// The VID for Velleman Projects is always 10cf. The PID is variable
					// for each device:
					//
					//    -------------------
					//    | Device   | PID  |
					//    -------------------
					//    | K8090    | 8090 |
					//    | VMB1USB  | 0b1b |
					//    -------------------
					//
					// As you can see it contains the VID/PID for the device, so we can check
					// for the right VID/PID with string handling routines.

					if (NULL != _tcsstr((TCHAR*)DevIntfDetailData->DevicePath, vidpid))
					{
						// To find out the serial port for our K8090 device,
						// we'll need to check the registry:

						hKey = SetupDiOpenDevRegKey(
							hDevInfo,
							&DevData,
							DICS_FLAG_GLOBAL,
							0,
							DIREG_DEV,
							KEY_READ
						);

						dwType = REG_SZ;
						dwSize = sizeof(lpData) / sizeof(lpData[0]);
						RegQueryValueEx(hKey, _T("PortName"), NULL, &dwType, lpData, &dwSize);
						RegCloseKey(hKey);

						// Eureka!
						_tprintf(_T("Found a device on port '%s'\n"), lpData);
					}
				}

				HeapFree(GetProcessHeap(), 0, DevIntfDetailData);

				// Continue looping
				SetupDiEnumDeviceInterfaces(
					hDevInfo, NULL, &GUID_DEVINTERFACE_USB_DEVICE, ++dwMemberIdx, &DevIntfData);
			}

			SetupDiDestroyDeviceInfoList(hDevInfo);
		}

		return 0;
	}
}