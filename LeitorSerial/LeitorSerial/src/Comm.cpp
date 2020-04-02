#include "Comm.h"
int AddDatatoLV(string str, HWND& lv) {
	FILE* file;
	FileHeader fh;
	LVITEM lvi;
	ZeroMemory(&lvi, sizeof(LVITEM));
	if (fopen_s(&file, str.c_str(), "rb")!=0) {
		MessageBox(0, ErrorOpenFile,L"Erro",MB_OK);
		return errno;
	}
	fread(&fh, sizeof(FileHeader), 1, file);
	if (fh.ID[0] != 'L' || fh.ID[1] != 'S' || fh.ID[2] != 'U') {
		MessageBox(0, ErrorBrokenFile, L"Erro", MB_OK);
		return -2;
	}
	size_t s = sizeof(FileData) * fh.nblocks, read =0;
	FileData* fd = (FileData*)malloc(s);
	WCHAR* wstr = (WCHAR*)malloc(sizeof(WCHAR) * 20);
	do {
		read+=fread(fd, sizeof(FileData), fh.nblocks, file);
	} while (!feof(file));
	if (read < fh.nblocks) {
		MessageBox(0, ErrorBrokenFile, L"Erro", MB_OK);
		return -1;			
	}
	for (long i = 0;i<fh.nblocks; i++) {
		wsprintf(wstr, L"%d", fd[i].dt);
		lvi.mask = LVIF_TEXT;
		lvi.iItem = i;
		lvi.pszText = wstr;
		lvi.cchTextMax = wcslen(wstr);
		ListView_InsertItem(lv, &lvi);
		wsprintf(wstr, L"%d", fd[i].mtime);
		ListView_SetItemText(lv, i,1,wstr);
	}
	free(wstr);
	free(fd);
	fclose(file);
	return 0;
}
int AddPortsNametoCB(COMMPORTS& cm, HWND& cb) {
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
		vector <wstring>::iterator it = find(cm.cm.begin(), cm.cm.end(), bu);
		if (it == cm.cm.end()) {
			cm.cm.push_back(bu);
		}
	}
	SendMessage(cb, CB_SETCURSEL, cm.nCursel, 0);
	RegCloseKey(key);
	return status;
}