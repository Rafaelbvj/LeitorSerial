#include "Comm.h"
int AddPortsNametoCB(COMMPORTS& cm, HWND cb) {
	HKEY key;
	WCHAR bu[20], bu2[20];
	DWORD sizet = sizeof(bu); 
	RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"HARDWARE\\DEVICEMAP\\SERIALCOMM", 0, KEY_READ, &key);
	cm.nCursel = SendMessage(cb, CB_GETCURSEL, 0, 0);
	DWORD status = ERROR_SUCCESS;
	SendMessage(cb, CB_RESETCONTENT, 0, 0);
	for (int i = 0;; i++, sizet = sizeof(bu)) {
		status = RegEnumValue(key, i, bu2, &sizet, 0, NULL, (LPBYTE)bu, &sizet);
		if (status == ERROR_NO_MORE_ITEMS) {
			break;
		}
		if (status != ERROR_SUCCESS) {
			status = -1;
			break;
		}
		
		SendMessage(cb, CB_ADDSTRING, 0, (LPARAM)bu);
		vector <wstring>::iterator it = find(cm.cm.begin(), cm.cm.end(),bu);
		if (it == cm.cm.end()) {
			cm.cm.push_back(bu);
		}
	}
	SendMessage(cb, CB_SETCURSEL, cm.nCursel, 0);
	RegCloseKey(key);
	return status;
}