#include "Comm.h"

int ExportFile(string pathfile, HWND& lv,int index, int type) {
	lxw_workbook* LWb = NULL;
	lxw_worksheet* LWs = NULL;
	lxw_format* LF = NULL;
	FILE* record = NULL;
	LVITEM lvi;
	ZeroMemory(&lvi, sizeof(LVITEM));
	int qtLb = SendMessage(lv, LVM_GETITEMCOUNT, 0, 0);

	if (qtLb > 0) {
		if (index == 1) {
			LWb = workbook_new(pathfile.c_str());
			if (LWb == NULL) {
				MessageBox(0, ErrorExportFile, ErrorTitle, MB_OK | MB_ICONERROR);
				return -1;
			}
			LWs = workbook_add_worksheet(LWb, "Leitor Serial");
			LF = workbook_add_format(LWb);
			format_set_font_name(LF, "Arial");
			lvi.mask = LVIF_TEXT;
			lvi.pszText = (wchar_t*)malloc(50);
			lvi.cchTextMax = 50;
			long col = 0;
			double c = 0;
			for (long i = qtLb-1; i >= 0; --i) {
				lvi.iSubItem = 0;
				lvi.iItem = i;
				ListView_GetItem(lv, &lvi);
				switch (type) {
				case INT_32BITS:
					c = wcstol(lvi.pszText, 0, 10);
					break;
				case FLOAT_32BITS:
					c = wcstold(lvi.pszText, 0);
					break;
				}
				worksheet_write_number(LWs, qtLb-i, col, c, LF);
				lvi.iItem = i;
				lvi.iSubItem = 1;
				ListView_GetItem(lv, &lvi);
				c = wcstol(lvi.pszText, 0, 10);
				worksheet_write_number(LWs, qtLb-i, col+1, c, LF);
			}
			workbook_close(LWb);
		}
		if (index == 2) {
			if (fopen_s(&record, pathfile.c_str(), "w") != 0) {
				MessageBox(0, ErrorExportFile, ErrorTitle, MB_OK | MB_ICONERROR);
				return errno; 
			}
			lvi.mask = LVIF_TEXT;
			lvi.pszText = (wchar_t*)calloc(20, sizeof(wchar_t));
			lvi.cchTextMax = 20;
			lvi.iItem = qtLb-1;
			float casttoint;
			for (int c, d; lvi.iItem >= 0; lvi.iItem--) {
				lvi.iSubItem = 0;
				ListView_GetItem(lv, &lvi);
				switch (type) {
				case INT_32BITS:
					c = wcstol(lvi.pszText, 0, 10);
					fprintf(record, "%d ", c);
					break;
				case FLOAT_32BITS:
					casttoint = (float)wcstold(lvi.pszText, 0);
					fprintf(record, "%f ", casttoint);
					break;
				}
				lvi.iSubItem = 1;
				ListView_GetItem(lv, &lvi);
				d = wcstol(lvi.pszText, 0, 10);
				fprintf(record, "%d\n", d);
			}
			fclose(record);
		}
		free(lvi.pszText);
		return 0;
	}
	return -2;
}
int SaveFile(string pathfile, HWND& lv, int type) {
	FileHeader fh;
	FileData fd;
	FILE* record;
	LVITEM lvi;
	ZeroMemory(&lvi, sizeof(LVITEM));
	int qtLb = SendMessage(lv, LVM_GETITEMCOUNT, 0, 0);
	fh.ID[0] = 'L'; fh.ID[1] = 'S'; fh.ID[2] = 'U';
	fh.type = (char)type;
	fh.nblocks = qtLb;
	if (qtLb > 0) {
		if (fopen_s(&record, pathfile.c_str(), "wb") != 0) {
			MessageBox(0, ErrorCreateFile, ErrorTitle, MB_OK);
			return errno;
		}
		fwrite(&fh, sizeof(FileHeader), 1, record);
		lvi.mask = LVIF_TEXT;
		lvi.pszText = (wchar_t*)calloc(20,sizeof(wchar_t));
		lvi.cchTextMax = 20;
		float casttoint;
		for (long i = 0; i < qtLb; i++) {
			lvi.iSubItem = 0;
			lvi.iItem = i;
			ListView_GetItem(lv, &lvi);
			switch (fh.type) {
			case INT_32BITS:
				fd.dt = wcstol(lvi.pszText, 0, 10);
				break;
			case FLOAT_32BITS:
				casttoint = (float) wcstold(lvi.pszText,0);
				memcpy(&fd.dt, &casttoint, sizeof(casttoint));		//float and int has the same size, so we can use the same struct
				break;
			}
			lvi.iSubItem = 1;
			ListView_GetItem(lv, &lvi);
			fd.mtime = wcstol(lvi.pszText, 0, 10);
			fwrite(&fd, sizeof(FileData), 1, record);
		}
		fclose(record);
		free(lvi.pszText);
		return 0;
	}
	return -1;
}
int PutDataInLV(string str, HWND& lv) {
	FILE* file;
	FileHeader fh;
	LVITEM lvi;
	ZeroMemory(&lvi, sizeof(LVITEM));
	if (fopen_s(&file, str.c_str(), "rb")!=0) {
		MessageBox(0, ErrorOpenFile,ErrorTitle,MB_OK|MB_ICONERROR);
		return errno;
	}

	fread(&fh, sizeof(FileHeader), 1, file);
	if (fh.ID[0] != 'L' || fh.ID[1] != 'S' || fh.ID[2] != 'U') {
		MessageBox(0, ErrorCorrupted, ErrorTitle, MB_OK|MB_ICONERROR);
		return -3;
	}  
	size_t read =0;
	FileData* fd = (FileData*)malloc(sizeof(FileData) * fh.nblocks);
	do {
		read+=fread(fd, sizeof(FileData), fh.nblocks, file);
	} while (!feof(file));
	if (read < fh.nblocks) {
		MessageBox(0, ErrorIncomplete, ErrorTitle, MB_OK);
		return -2;			
	}
	wchar_t ptfformat[5],wstr[20];
	switch (fh.type) {
	case FLOAT_32BITS:
		lstrcpyW(ptfformat, L"%f");
		break;
	case INT_32BITS:
		lstrcpyW(ptfformat, L"%d");
		break;
	default:
		MessageBox(0, ErrorFormat, ErrorTitle, MB_OK|MB_ICONERROR);
		return -1;			

	}
	for (size_t i = 0;i<fh.nblocks; i++) {
		wsprintf(wstr,ptfformat, fd[i].dt);
		lvi.mask = LVIF_TEXT;
		lvi.iItem = i;
		lvi.pszText = wstr;
		lvi.cchTextMax = wcslen(wstr);
		ListView_InsertItem(lv, &lvi);
		wsprintf(wstr, L"%d", fd[i].mtime);
		ListView_SetItemText(lv, i,1,wstr);
	}
	free(fd);
	fclose(file);
	return 0;
}
bool AddPortsNametoCB(COMMPORTS& cm, HWND& cb) {
	HKEY key;
	WCHAR bu[20], bu2[20];
	DWORD sizet = sizeof(bu), i;
	RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"HARDWARE\\DEVICEMAP\\SERIALCOMM", 0, KEY_READ, &key);
	cm.nCursel = SendMessage(cb, CB_GETCURSEL, 0, 0);
	if (cm.nCursel == CB_ERR) {
		cm.nCursel = 0;
	}
	DWORD status = ERROR_SUCCESS;
	SendMessage(cb, CB_RESETCONTENT, 0, 0);
	for (i = 0;; i++, sizet = sizeof(bu)) {
		status = RegEnumValue(key, i, bu2, &sizet, 0, NULL, (LPBYTE)bu, &sizet);
		if (status == ERROR_NO_MORE_ITEMS) {
			break;
		}
		if (status != ERROR_SUCCESS) {
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
	return cm.cm.size() > 0 && i > 0? true:false;
}