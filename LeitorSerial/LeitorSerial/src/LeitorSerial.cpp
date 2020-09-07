#include "framework.h"
#include "LeitorSerial.h"

#define MAX_LOADSTRING 100

HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];

INT_PTR CALLBACK    DialogOP(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{

	InitCommonControls();
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_WINDOWSPROJECT1, szWindowClass, MAX_LOADSTRING);
	WNDCLASSEXW wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = static_cast<HICON>(::LoadImage(hInstance,MAKEINTRESOURCE(IDI_WINDOWSPROJECT1),
												IMAGE_ICON,
												0, 0,
												LR_DEFAULTCOLOR | LR_DEFAULTSIZE));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_WINDOWSPROJECT1);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = static_cast<HICON>(::LoadImage(hInstance,MAKEINTRESOURCE(IDI_WINDOWSPROJECT1),
												  IMAGE_ICON,
												  0, 0,
												  LR_DEFAULTCOLOR | LR_DEFAULTSIZE));
	RegisterClassExW(&wcex);
	HWND hWnd = CreateWindowW(szWindowClass, szTitle, 
							 WS_SYSMENU | WS_MINIMIZEBOX | WS_SIZEBOX,
							 CW_USEDEFAULT, CW_USEDEFAULT, 
							 650, 700, 
							 nullptr, nullptr, hInstance, nullptr);
	ShowWindow(hWnd, nCmdShow);
	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WINDOWSPROJECT1));
	MSG msg;
	BOOL bExit = TRUE;
	while (bExit)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT) {
				bExit = FALSE;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return (int)msg.wParam;
}

//Winapi vars
OPENFILENAMEA ofn;
RECT rect;
CHOOSEFONT lpc;
LOGFONT logF, logFC;
NONCLIENTMETRICS ncm;
HFONT hFont = (HFONT)NULL, lFont = (HFONT)NULL, sysFont;
COLORREF textColor;
HBRUSH hBrush;
FILE* record;

//Common controls
HWND cbPort, cbPrec, cbGanho;					//Comboboxes
HWND edt1,edt2;									//Edits
HWND rd, rd2, rd3, chk1, chk3, chk4;			//Buttons
HWND st, st1, st2, st3, st4, st5,st6;			//Statics
HWND lv;										//ListViews
HWND pgBar;										//Progress Bar

//Plot vars
Graficos graph;

//Communication vars
COMMPORTS cp;
DWORD idThread, readBytes, writeBytes, evt;
HANDLE hThread, commPort = INVALID_HANDLE_VALUE;
OVERLAPPED olr, olw;
DataConf dc;
DataProtocol dp;
WCHAR  wbuffer[50];

//State vars
atomic <bool> bRunning = FALSE;
int  tensaoType = INT_32BITS;

//Default sets
char gnufile[MAX_PATH];
string scripttoload = "gnuscript-example.gnu";

DWORD WINAPI Thread(LPVOID lp) {

	bRunning = TRUE;
	double fct = 1, tara = 0, calcVolts = 0;
	int nPrec = 0;
	WCHAR ptfb[10],wtime[50];
	char ptff[10];
	BOOL PlotEnable = FALSE;
	clock_t begin;
	unsigned long currenttime =0 ,timelimit;
	DWORD e = 0;
	LVITEM lvi;
	HWND hwnd = (HWND)lp;

	ZeroMemory(&olr, sizeof(OVERLAPPED));
	ZeroMemory(&olw, sizeof(OVERLAPPED));
	ZeroMemory(&dc, sizeof(DataConf));
	olr.hEvent = CreateEvent(0, TRUE, FALSE, L"Read");
	olw.hEvent = CreateEvent(0, TRUE, FALSE, L"Write");
	if (olr.hEvent == NULL || olw.hEvent == NULL) {
		MessageBox(hwnd, ErrorCreateEvent, 0, MB_OK | MB_ICONERROR);
		return -1;
	}
	cp.cc.dcb.fDtrControl = DTR_CONTROL_ENABLE;

	if (SendMessage(chk4, BM_GETCHECK, 0, 0) == BST_CHECKED) {
		dc.prec = SendMessage(cbPrec, CB_GETCURSEL, 0, 0);
	}

	dc.ganho = SendMessage(cbGanho, CB_GETCURSEL, 0, 0) + 1;

	SendMessage(edt2, WM_GETTEXT, sizeof(wbuffer), (LPARAM)wbuffer);
	dc.msegs = wcstoul(wbuffer, 0, 10) * 1000;
	if (dc.msegs == 0) {
		MessageBox(hwnd, InfoTimeLimit, InfoTitle, MB_OK | MB_ICONINFORMATION);
		dc.msegs = 60000;
	}

	SetCommState(commPort, &cp.cc.dcb);
	if (WriteFile(commPort, &dc, sizeof(dc), &writeBytes, &olw) == FALSE) {
		if (GetLastError() == ERROR_IO_PENDING) {
			if (GetOverlappedResult(commPort, &olw, &writeBytes, TRUE) == FALSE) {

				MessageBox(hwnd, ErrorGetOverlappedResult, ErrorTitle, MB_ICONERROR | MB_OK);
				goto label_exit;
			}
		}
		else {
			MessageBox(hwnd, ErrorWriteFile, ErrorTitle, MB_ICONERROR | MB_OK);
			goto label_exit;
		}
	}
	ZeroMemory(wbuffer, sizeof(wbuffer));
	
	switch (dc.ganho) {
	case 1:
		dc.ganho = 20;	//input offset voltage
		break;
	case 2:
		dc.ganho = 80; 
		break;
	case 3:
		dc.ganho = 40;  
		break;
	}
	
	SendMessage(edt1, WM_GETTEXT, sizeof(wbuffer), (LPARAM)wbuffer);
	tara = wcstod(wbuffer, 0);

	if (SendMessage(chk4, BM_GETCHECK, 0, 0) == BST_CHECKED) {
		tensaoType = FLOAT_32BITS;
		fct = (double)dc.ganho / pow(2 , 23);
		nPrec = SendMessage(cbPrec, CB_GETCURSEL, 0, 0);
	}
	
	switch (nPrec) {
	case 0:
		lstrcpyW(ptfb, L"%.0f");
		strcpy_s(ptff, "%d %.0f\n");
		break;
	case 1:
		lstrcpyW(ptfb, L"%.1f");
		strcpy_s(ptff, "%d %.1f\n");
		break;
	case 2:
		lstrcpyW(ptfb, L"%.2f");
		strcpy_s(ptff, "%d %.2f\n");
		break;
	case 3:
		lstrcpyW(ptfb, L"%.3f");
		strcpy_s(ptff, "%d %.3f\n");
		break;
	}

	if (SendMessage(chk3, BM_GETCHECK, 0, 0) == BST_CHECKED) {
		PlotEnable = TRUE;
		if (graph.IsGNUPlotRunning()) {
			MessageBox(hwnd, WarningGnuRunning, WarningTitle, MB_OK | MB_ICONWARNING);
			graph.FinishGNUPlotProgram();
		}
		if (!graph.StartGNUPlotProgram()) {
			MessageBox(hwnd, ErrorGnuPlotNotFound, ErrorTitle, MB_OK | MB_ICONERROR);
			PlotEnable = FALSE;
		}
		else {

			/******************Plot Configuration*********************/
			//Setting plot features		
			if (!graph.GNUScript(scripttoload)) {
				MessageBox(hwnd, WarningScriptNotLoad, WarningTitle, MB_OK | MB_ICONWARNING);
			}
			/*********************************************************/
			fclose(_fsopen("tmpplot", "w", SH_DENYNO));
		}
	}
	SetCommMask(commPort, EV_RXCHAR);
	timelimit = dc.msegs / 1000;
	begin = clock();

	for (DWORD status = 0, i = 0; currenttime < timelimit;) {
		currenttime = (clock() - begin) / CLOCKS_PER_SEC;
		SendMessage(pgBar, PBM_SETPOS, 1000*currenttime/timelimit, 0);

		if (!WaitCommEvent(commPort, &evt, &olr)) {					//Verify serial port events
			if (GetLastError() == ERROR_IO_PENDING) {
				status = WaitForSingleObject(olr.hEvent, 5000);		//Waits until 5 seconds
				if (status == WAIT_TIMEOUT) {
					MessageBox(hwnd, ErrorTimeOut, ErrorTitle, MB_OK | MB_ICONERROR);
					if (graph.IsGNUPlotRunning()) {
						graph.FinishGNUPlotProgram();
					}
					break;
				}
				if (status == WAIT_FAILED) {

					MessageBox(hwnd, ErrorWaitForSingleObject, ErrorTitle, MB_ICONERROR);
					break;
				}

			}
			else {

				MessageBox(hwnd, ErrorWaitCommEvent, ErrorTitle, MB_ICONERROR);
				COMSTAT cs;
				ClearCommError(commPort, &readBytes, &cs);
				break;
			}
		}

		if (evt & EV_RXCHAR) {

			if (!ReadFile(commPort, &dp, sizeof(dp), 0, &olr)) {
				GetOverlappedResult(commPort, &olr, &readBytes, FALSE);
				status = WaitForSingleObject(olr.hEvent, 3000);		 //Waits until 3 seconds
				if (status == WAIT_TIMEOUT) {
					MessageBox(hwnd, ErrorReadFileTimeOut, ErrorTitle, MB_OK | MB_ICONERROR);
					break;
				}
			}
			if (dp.signbegin[0] == 'B' && dp.signend[0] == 'E') {	 //Data signature

				calcVolts = fct * dp.dt - tara;
				
				//Add 'Sinal' column a new item
				swprintf_s(wbuffer, sizeof(wbuffer), ptfb, calcVolts);
				ZeroMemory(&lvi, sizeof(LVITEM));
				lvi.mask = LVIF_TEXT;
				lvi.pszText = wbuffer;
				lvi.iItem = e;
				lvi.cchTextMax = wcslen(wbuffer);
				ListView_InsertItem(lv, &lvi);

				//Add 'Tempo' column a new item
				wsprintf(wtime, L"%d", dp.mtime);
				ListView_SetItemText(lv, e, 1, wtime);


				/******Update plot file********/
				if (PlotEnable) {
					record = _fsopen("tmpplot", "a", SH_DENYNO);     //Open file in sharing mode
					if (record != NULL) {
						fprintf(record, ptff, dp.mtime, calcVolts);
						if (!fclose(record)) {
							if (e == 4) {							 //This number must match gnuplot inline command 'every'
								graph.CmdLine("plot \"tmpplot\" every 4 with lines lt 1 \n");
							}
							i++;									 //plot offset
							e++;									 //index order						
							if (i == 10) {
								graph.CmdLine("replot\n");			 //Update plot chart
								i = 0;
							}
						}
					}
				}
				/*******************************/
			}
			else {
				if (dp.signbegin[0] == 'E' && dp.signend[0] == 'D') { //End of communication signature
					MessageBox(hwnd, FinishedReading, InfoTitle, MB_ICONINFORMATION);
					break;
				}
				MessageBox(hwnd, ErrorSync, ErrorTitle, MB_OK | MB_ICONERROR);
				if (graph.IsGNUPlotRunning()) {
					graph.FinishGNUPlotProgram();
				}
				break;
			}
			memset(&dp, 0, sizeof(dp));
		}
		InvalidateRect(hwnd, &rect, FALSE);
	}
label_exit:

	PostMessage(hwnd, WM_COMMAND, DESCONECTAR_BUTTON, 0);
	
	return 0;
}

INT_PTR CALLBACK  DialogOP(HWND h, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_INITDIALOG:
		if (cp.portname.empty()) {
			EnableWindow(GetDlgItem(h, IDC_BUTTON3), FALSE);
		}
		SendMessageA(GetDlgItem(h, IDC_EDIT3), WM_SETTEXT, scripttoload.size(), (LPARAM)scripttoload.c_str());
		SendMessageA(GetDlgItem(h, IDC_EDIT1), WM_SETTEXT, graph.GetGnuFilePath().size(), (LPARAM)(graph.GetGnuFilePath().c_str()));
		SendMessage(GetDlgItem(h, IDC_EDIT1), EM_SETLIMITTEXT, 100, 0);
		ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
		ofn.lStructSize = sizeof(ofn);
		ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER | OFN_NOCHANGEDIR;
		ofn.nMaxFile = MAX_PATH;
		ofn.hwndOwner = h;
		break;
	case WM_COMMAND:
	{
		if (HIWORD(wParam) == 0) {
			if (LOWORD(wParam) == IDC_BUTTON3) {
				if (!cp.portname.empty()) {
					cp.cc.dwSize = sizeof(COMMCONFIG);
					if (CommConfigDialog(cp.portname.c_str(), h, &cp.cc)) {
					}
				}
			}
			if (LOWORD(wParam) == IDC_OPOK) {
				memset(gnufile, 0, sizeof(gnufile));
				SendMessageA(GetDlgItem(h, IDC_EDIT1), WM_GETTEXT, MAX_PATH, (LPARAM)gnufile);
				SendMessage(h, WM_CLOSE, 0, 0);
			}
			if (LOWORD(wParam) == IDC_SCRIPT) {	
				ofn.lpstrFile = (char*)calloc(MAX_PATH, sizeof(char));
				ofn.lpstrFilter = "All\0*.*\0";
				if (GetOpenFileNameA(&ofn)) {
					scripttoload = ofn.lpstrFile;
					SendMessageA(GetDlgItem(h, IDC_EDIT3), WM_SETTEXT, strlen(ofn.lpstrFile), (LPARAM)ofn.lpstrFile);
				}
				free(ofn.lpstrFile);
			}
			if (LOWORD(wParam) == IDC_GNUFILE) {
				ofn.lpstrFile = (char*)calloc(MAX_PATH, sizeof(char));
				ofn.lpstrFilter = "Exe\0*.exe\0";
				if (GetOpenFileNameA(&ofn)) {
					graph.SetGnuFilePath(ofn.lpstrFile);
					SendMessageA(GetDlgItem(h, IDC_EDIT1), WM_SETTEXT, graph.GetGnuFilePath().size(), (LPARAM)(graph.GetGnuFilePath().c_str()));
				}
				free(ofn.lpstrFile);
			}
		}
	}
	break;
	case WM_CLOSE:
	{
		EndDialog(h, 0);
		return 0;
	}

	}
	return 0;
}


void ComponentG(HWND& h, HINSTANCE& hi)
{
	//Absolute position and size

	rect.bottom = 100;
	rect.top = 10;
	rect.left = 600;
	rect.right = 300;

	//Default COMM config
	cp.cc.dcb.BaudRate = 9600;
	cp.cc.dcb.DCBlength = sizeof(DCB);
	cp.cc.dcb.ByteSize = 8;

	//All of interface's components
	cbPort = CreateWindowEx(0, WC_COMBOBOX, 0, CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_VISIBLE | WS_CHILD, 10, 10, 150, 100, h, 0, hi, 0);
	rd = CreateWindowEx(0, WC_BUTTON, L"Conectar", WS_VISIBLE | WS_CHILD | WS_DISABLED, 10, 50, 100, 25, h, (HMENU)CONECTAR_BUTTON, hi, 0);
	chk1 = CreateWindowEx(0, WC_BUTTON, L"Configurações", WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 10, 100, 250, 300, h, (HMENU)1407, hi, 0);
	st = CreateWindowEx(0, WC_STATIC, L"Casas decimais:", WS_VISIBLE | WS_CHILD | WS_DISABLED, 15, 130, 110, 20, h, 0, hi, 0);
	cbPrec = CreateWindowEx(0, WC_COMBOBOX, 0, CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_VISIBLE | WS_DISABLED | WS_CHILD, 15, 150, 150, 100, h, 0, hi, 0);
	chk4 = CreateWindowEx(0, WC_BUTTON, L"Tensão", WS_VISIBLE | WS_CHILD | BS_CHECKBOX | WS_DISABLED, 170, 150, 70, 20, h, (HMENU)TENSAO_CHECKBUTTON, hi, 0);	
	st3 = CreateWindowEx(0, WC_STATIC, L"Tempo de amostragem:", WS_VISIBLE | WS_CHILD | WS_DISABLED, 15, 190, 160, 20, h, 0, hi, 0);
	edt2 = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, 0, WS_VISIBLE | WS_CHILD | ES_NUMBER | WS_DISABLED, 15, 210, 150, 20, h, 0, hi, 0);
	st4 = CreateWindowEx(0, WC_STATIC, L"segundos", WS_VISIBLE | WS_CHILD | WS_DISABLED, 170, 210, 80, 20, h, 0, hi, 0);
	st6 = CreateWindowEx(0, WC_STATIC, L"Tara:", WS_VISIBLE | WS_CHILD | WS_DISABLED, 15, 240, 80, 20, h, 0, hi, 0);
	edt1 = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, 0, WS_VISIBLE | WS_CHILD | ES_NUMBER | WS_DISABLED, 15, 260, 150, 20, h, 0, hi, 0);
	chk3 = CreateWindowEx(0, WC_BUTTON, L"Plot", WS_VISIBLE | WS_CHILD | BS_CHECKBOX | BS_NOTIFY | WS_DISABLED, 15, 290, 100, 20, h, (HMENU)PLOT_CHECKBUTTON, hi, 0);
	st5 = CreateWindowEx(0, WC_STATIC, L"Ganho:", WS_VISIBLE | WS_CHILD | WS_DISABLED, 15, 320, 80, 20, h, 0, hi, 0);
	cbGanho = CreateWindowEx(0, WC_COMBOBOX, 0, CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_VISIBLE | WS_DISABLED | WS_CHILD, 15, 340, 150, 100, h, 0, hi, 0);
	pgBar = CreateWindowEx(0, PROGRESS_CLASS, 0, WS_VISIBLE | WS_CHILD , 10, 420, 250, 30, h, (HMENU)PROGRESS_BAR, hInst, 0);
	
	rd2 = CreateWindowEx(0, WC_BUTTON, L"Iniciar", WS_VISIBLE | WS_CHILD | WS_DISABLED, 10, 470, 100, 25, h, (HMENU)INICIAR_BUTTON, hi, 0);
	rd3 = CreateWindowEx(0, WC_BUTTON, L"Desconectar", WS_VISIBLE | WS_CHILD | WS_DISABLED, 160, 470, 100, 25, h, (HMENU)DESCONECTAR_BUTTON, hi, 0);
	
	lv = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEWW, 0, WS_VISIBLE | LVS_REPORT | WS_CHILD , 300, 110, 300, 500, h, (HMENU)2090, hi, 0);
	ListView_SetExtendedListViewStyle(lv, LVS_EX_FULLROWSELECT);
	LVCOLUMN lcsinal,lctempo;
	WCHAR lvsinal[] = L"Sinal";
	WCHAR lvtempo[] = L"Tempo (milisegundos)";
	ZeroMemory(&lcsinal, sizeof(LVCOLUMN));
	lcsinal.mask = LVCF_TEXT | LVCF_FMT | LVCF_WIDTH;
	lcsinal.cx = 150;
	lcsinal.fmt = LVCFMT_CENTER;
	lcsinal.cchTextMax = sizeof(lvsinal);
	lcsinal.pszText = lvsinal;
	ListView_InsertColumn(lv, 0, &lcsinal);
	lctempo.mask = LVCF_TEXT | LVCF_FMT | LVCF_WIDTH;
	lctempo.pszText = lvtempo;
	lctempo.fmt = LVCFMT_CENTER;
	lctempo.cx = 150;
	lctempo.cchTextMax = sizeof(lvtempo);
	ListView_InsertColumn(lv, 1, &lctempo);

	SendMessage(pgBar, PBM_SETSTEP, 1, 0);
	SendMessage(pgBar, PBM_SETRANGE, 0, 1000 << 16);

	SendMessage(h, WM_DEVICECHANGE, DBT_DEVNODES_CHANGED, 0);
	SendMessage(cbPrec, CB_ADDSTRING, 0, (LPARAM)L"1 mV");
	SendMessage(cbPrec, CB_ADDSTRING, 0, (LPARAM)L"0.1 mV");
	SendMessage(cbPrec, CB_ADDSTRING, 0, (LPARAM)L"0.01 mV");
	SendMessage(cbPrec, CB_ADDSTRING, 0, (LPARAM)L"0.001 mV");
	SendMessage(cbPrec, CB_SETCURSEL, 0, 0);
	SendMessage(cbGanho, CB_ADDSTRING, 0, (LPARAM)L"128");
	SendMessage(cbGanho, CB_ADDSTRING, 0, (LPARAM)L"32");
	SendMessage(cbGanho, CB_ADDSTRING, 0, (LPARAM)L"64");
	SendMessage(cbGanho, CB_SETCURSEL, 0, 0);
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);
	sysFont = (HFONT)CreateFontIndirect(&ncm.lfMessageFont);
	SendMessage(rd, WM_SETFONT, (WPARAM)sysFont, 0);
	SendMessage(rd2, WM_SETFONT, (WPARAM)sysFont, 0);
	SendMessage(rd3, WM_SETFONT, (WPARAM)sysFont, 0);
	SendMessage(st, WM_SETFONT, (WPARAM)sysFont, 0);
	SendMessage(st1, WM_SETFONT, (WPARAM)sysFont, 0);
	SendMessage(st2, WM_SETFONT, (WPARAM)sysFont, 0);
	SendMessage(st3, WM_SETFONT, (WPARAM)sysFont, 0);
	SendMessage(st4, WM_SETFONT, (WPARAM)sysFont, 0);
	SendMessage(st5, WM_SETFONT, (WPARAM)sysFont, 0);
	SendMessage(st6, WM_SETFONT, (WPARAM)sysFont, 0);
	SendMessage(edt1, WM_SETFONT, (WPARAM)sysFont, 0);
	SendMessage(edt2, WM_SETFONT, (WPARAM)sysFont, 0);
	SendMessage(cbPort, WM_SETFONT, (WPARAM)sysFont, 0);
	SendMessage(cbPrec, WM_SETFONT, (WPARAM)sysFont, 0);
	SendMessage(cbGanho, WM_SETFONT, (WPARAM)sysFont, 0);
	SendMessage(chk1, WM_SETFONT, (WPARAM)sysFont, 0);
	SendMessage(chk3, WM_SETFONT, (WPARAM)sysFont, 0);
	SendMessage(chk4, WM_SETFONT, (WPARAM)sysFont, 0);
	SendMessage(edt2, WM_SETFONT, (WPARAM)sysFont, 0);
	
}
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

	switch (message)
	{
	case WM_CREATE:
	{	
		ComponentG(hWnd, hInst);
	}
	break;
	case WM_DEVICECHANGE:
		if (wParam == DBT_DEVNODES_CHANGED) {
			if (IsWindowEnabled(cbPort)) {				//Verify COM ports available
				if (AddPortsNametoCB(cp, cbPort)) {
					EnableWindow(rd, TRUE);
				}
				else {
					EnableWindow(rd, FALSE);
				}
			}
		}
		break;

	case WM_COMMAND:
	{
		if (HIWORD(wParam) == 0) {
			if (wParam == ID_ARQUIVO_ABRIR) {
				ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
				ofn.lStructSize = sizeof(OPENFILENAMEA);
				ofn.Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
				ofn.hwndOwner = hWnd;
				ofn.nMaxFile = MAX_PATH;
				ofn.lpstrFile = (char*)calloc(MAX_PATH, sizeof(char));
				ofn.lpstrFilter = "LeitorSerial File (.lsu)\0*.lsu\0\0";
				if (GetOpenFileNameA(&ofn)) {
					PutDataInLV(ofn.lpstrFile, lv);
				}
				free(ofn.lpstrFile);

			}
			if (wParam == ID_ARQUIVO_SALVAR) {
				ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
				ofn.lStructSize = sizeof(OPENFILENAMEA);
				ofn.Flags = OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT;
				ofn.hwndOwner = hWnd;
				ofn.nMaxFile = MAX_PATH;
				ofn.lpstrFile = (char*)calloc(MAX_PATH, sizeof(char));
				ofn.lpstrFilter = "LeitorSerial File (.lsu)\0*.lsu\0\0";
				if (GetSaveFileNameA(&ofn)) {
					if (ofn.nFileExtension == 0) {
						sprintf_s(ofn.lpstrFile, MAX_PATH, "%s.lsu", ofn.lpstrFile);
					}
					SaveFile(ofn.lpstrFile, lv, tensaoType);
					
				}
			}
			if (wParam == ID_ARQUIVO_EXPORTAR) {
				ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
				ofn.lStructSize = sizeof(OPENFILENAMEA);
				ofn.Flags = OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT;
				ofn.hwndOwner = hWnd;
				ofn.nMaxFile = MAX_PATH;
				ofn.lpstrFile = (char*)calloc(MAX_PATH, sizeof(char));
				ofn.lpstrFilter = "Excel (.xlsx)\0*.*\0Text ANSI (.txt)\0*.*\0\0";
				if (GetSaveFileNameA(&ofn)) {
					if (ofn.nFileExtension == 0) {
						if (ofn.nFilterIndex == 1) {									//Excel extension selected
							sprintf_s(ofn.lpstrFile, MAX_PATH, "%s.xlsx", ofn.lpstrFile);
						}
						if (ofn.nFilterIndex == 2) {									//Plain text extension selected
							sprintf_s(ofn.lpstrFile, MAX_PATH, "%s.txt", ofn.lpstrFile);
						}
					}
					int k = tensaoType;
					ExportFile(ofn.lpstrFile, lv, ofn.nFilterIndex, tensaoType);
					
					//It occurs when attempt to connect without saving the list first, so the listview must be cleared after saving it.
					if (lParam == 1) { 
						memset(wbuffer, 0, sizeof(wbuffer));
						ListView_DeleteAllItems(lv);
						InvalidateRect(hWnd, 0, TRUE);
					}
				}
				free(ofn.lpstrFile);
			}
			if (wParam == PLOT_CHECKBUTTON) {
				if (SendMessage(chk3, BM_GETCHECK, 0, 0) == BST_CHECKED) {
					SendMessage(chk3, BM_SETCHECK, BST_UNCHECKED, 0);
				}
				else {
					SendMessage(chk3, BM_SETCHECK, BST_CHECKED, 0);
				}
			}
			if (wParam == TENSAO_CHECKBUTTON) {
				if (SendMessage(chk4, BM_GETCHECK, 0, 0) == BST_CHECKED) {
					SendMessage(chk4, BM_SETCHECK, BST_UNCHECKED, 0);
					EnableWindow(cbPrec, FALSE);
					tensaoType = INT_32BITS;
				}
				else {
					EnableWindow(cbPrec, TRUE);					
					SendMessage(chk4, BM_SETCHECK, BST_CHECKED, 0);
					tensaoType = FLOAT_32BITS;
				}
			}
			if (wParam == ID_OP) {
				DialogBox(hInst, MAKEINTRESOURCE(IDD_FORMVIEW), hWnd, DialogOP);
			}
			if (wParam == ID_FONTE) {
				ZeroMemory(&lpc, sizeof(lpc));
				lpc.lStructSize = sizeof(CHOOSEFONT);
				lpc.Flags = CF_EFFECTS | CF_SCREENFONTS;
				lpc.hwndOwner = 0;
				lpc.lpLogFont = &logF;
				if (ChooseFont(&lpc) == TRUE) {
					textColor = lpc.rgbColors;
					hFont = CreateFontIndirect(&logF);
					InvalidateRect(hWnd, 0, TRUE);
				}

			}
			if (wParam == DESCONECTAR_BUTTON) {

				CloseHandle(commPort);
				cp.portname.clear();
				EnableWindow(cbPort, TRUE);
				SendMessage(hWnd, WM_DEVICECHANGE, DBT_DEVNODES_CHANGED, 0);
				bRunning = FALSE;

				SendMessage(pgBar, PBM_SETPOS, 0, 0);
				EnableWindow(cbPrec, FALSE);
				EnableWindow(cbGanho, FALSE);
				EnableWindow(rd, TRUE);
				EnableWindow(rd2, FALSE);
				EnableWindow(rd3, FALSE);
				EnableWindow(edt1, FALSE);
				EnableWindow(edt2, FALSE);
				EnableWindow(st, FALSE);
				EnableWindow(st1, FALSE);
				EnableWindow(st2, FALSE);
				EnableWindow(st3, FALSE);
				EnableWindow(st4, FALSE);
				EnableWindow(st5, FALSE);
				EnableWindow(st6, FALSE);
				EnableWindow(chk3, FALSE);
				EnableWindow(chk3, FALSE);
				EnableWindow(chk4, FALSE);
			}
			if (wParam == CONECTAR_BUTTON) {
				if (ListView_GetItemCount(lv) > 0) {
					if (MessageBox(hWnd, WarningSaveFile, WarningTitle, MB_YESNO | MB_ICONWARNING) == IDNO) {
						memset(wbuffer, 0, sizeof(wbuffer));
						ListView_DeleteAllItems(lv);
						InvalidateRect(hWnd, 0, TRUE);
					}
					else {
						SendMessage(hWnd, WM_COMMAND, ID_ARQUIVO_SALVAR, (LPARAM)1);
						break;
					}

				}
				
				int nCursel = SendMessage(cbPort, CB_GETCURSEL, 0, 0);
				WCHAR selectedPort[20];
				SendMessage(cbPort, CB_GETLBTEXT, nCursel, (LPARAM)selectedPort);
				commPort = CreateFile(selectedPort, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
				if (commPort == INVALID_HANDLE_VALUE) {
					MessageBox(hWnd, ErrorSerialConnection, ErrorTitle, MB_OK | MB_ICONERROR);

				}
				else {
					cp.portname = selectedPort;
					EnableWindow(rd, FALSE);
					EnableWindow(rd2, TRUE);
					EnableWindow(rd3, TRUE);
					EnableWindow(cbPort, FALSE);
					EnableWindow(cbGanho, TRUE);
					EnableWindow(edt1, TRUE);
					EnableWindow(edt2, TRUE);
					EnableWindow(st, TRUE);
					EnableWindow(st1, TRUE);
					EnableWindow(st2, TRUE);
					EnableWindow(st3, TRUE);
					EnableWindow(st4, TRUE);
					EnableWindow(st5, TRUE);
					EnableWindow(st6, TRUE);
					EnableWindow(chk3, TRUE);
					EnableWindow(chk4, TRUE);

				}

			}
			if (wParam == INICIAR_BUTTON) {
				EnableWindow(cbPrec, FALSE);
				EnableWindow(cbGanho, FALSE);
				EnableWindow(rd2, FALSE);
				EnableWindow(rd3, FALSE);
				EnableWindow(edt1, FALSE);
				EnableWindow(edt2, FALSE);
				EnableWindow(st, FALSE);
				EnableWindow(st1, FALSE);
				EnableWindow(st2, FALSE);
				EnableWindow(st3, FALSE);
				EnableWindow(st4, FALSE);
				EnableWindow(st5, FALSE);
				EnableWindow(st6, FALSE);
				EnableWindow(chk3, FALSE);
				EnableWindow(chk3, FALSE);
				EnableWindow(chk4, FALSE);
				hThread = CreateThread(0, 0, Thread, hWnd, CREATE_ALWAYS, &idThread);
				if (hThread == NULL) {
					MessageBox(hWnd, ErrorThread, ErrorTitle, MB_ICONERROR);
				}
			}
		}
	}
	break;
	case WM_CTLCOLORSTATIC:
	{
		if (hBrush == NULL) {
			hBrush = CreateSolidBrush(RGB(255, 255, 255));			//Setting all component's background colors to white
		}
		return (INT_PTR)hBrush;
	}
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HFONT oldFont;
		HDC hdc = BeginPaint(hWnd, &ps);
		Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);
		if (hFont == NULL) {
			if (lFont == NULL) {
				ncm.lfMessageFont.lfWidth = 15;
				ncm.lfMessageFont.lfHeight = 35;
				ncm.lfMessageFont.lfItalic = 10;
				lFont = CreateFontIndirect(&ncm.lfMessageFont);

			}

			oldFont = (HFONT)SelectObject(hdc, lFont);
		}
		else {
			oldFont = (HFONT)SelectObject(hdc, hFont);
		}


		COLORREF oldC = SetTextColor(hdc, textColor);
		DrawText(hdc, wbuffer, lstrlenW(wbuffer), &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		SetTextColor(hdc, oldC);
		EndPaint(hWnd, &ps);
		DeleteObject(oldFont);
		DeleteDC(hdc);
		UpdateWindow(hWnd);
	}
	break;
	case WM_CLOSE:
	{
		if (bRunning) {
			if (MessageBox(hWnd, WarningProgramExit, WarningTitle, MB_YESNO | MB_ICONEXCLAMATION) == IDNO) {
				return 0;
			}
			bRunning = FALSE;
		}
		if (WaitForSingleObject(hThread, 0) == WAIT_OBJECT_0) {
			TerminateThread(hThread, 0);
		}
		graph.FinishGNUPlotProgram();
		
		DestroyWindow(hWnd);
	}
	break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

