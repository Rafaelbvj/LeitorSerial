#include "Comm.h"
int AddPortsNametoCB(COMMPORTS *cm,HWND cb) {
    HKEY key;
    WCHAR bu[20], bu2[20];
    CHAR bu3[20];
    DWORD sizet = sizeof(bu); //size max 
    
    RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"HARDWARE\\DEVICEMAP\\SERIALCOMM", 0, KEY_READ, &key);
    cm->nCursel = SendMessage(cb, CB_GETCURSEL, 0, 0);
    DWORD status = ERROR_SUCCESS;
    SendMessage(cb, CB_RESETCONTENT, 0, 0);
    for (int i = 0; status == ERROR_SUCCESS; i++) {

        status = RegEnumValue(key, i, bu2, &sizet, 0, NULL, (LPBYTE)bu, &sizet);
        
        if (status == ERROR_SUCCESS) {
            WideCharToMultiByte(CP_UTF8, WC_COMPOSITECHECK, bu, sizet, bu3, sizeof(bu3), 0, 0);
            SendMessage(cb, CB_ADDSTRING, 0, (LPARAM)bu);
            vector <string>::iterator it = find(cm->cm.begin(), cm->cm.end(), string(bu3));
            if (it == cm->cm.end()) {
                cm->cm.push_back(string(bu3));
            }
            sizet = sizeof(bu);
            continue;

        }
        if (i == 0) {
            status = -1;
        }

    }
    SendMessage(cb, CB_SETCURSEL, cm->nCursel, 0);
    RegCloseKey(key);
    return status;
}