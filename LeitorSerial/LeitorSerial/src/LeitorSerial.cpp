// WindowsProject1.cpp : Define o ponto de entrada para o aplicativo.
//

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
	
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	
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
	wcex.hIcon = static_cast<HICON>(::LoadImage(hInstance,
		MAKEINTRESOURCE(IDI_WINDOWSPROJECT1),
		IMAGE_ICON,
		0, 0,
		LR_DEFAULTCOLOR | LR_DEFAULTSIZE));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_WINDOWSPROJECT1);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = static_cast<HICON>(::LoadImage(hInstance,
		MAKEINTRESOURCE(IDI_WINDOWSPROJECT1),
		IMAGE_ICON,
		0, 0,
		LR_DEFAULTCOLOR | LR_DEFAULTSIZE));
	RegisterClassExW(&wcex);

	HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, 700, 700, nullptr, nullptr, hInstance, nullptr);



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
lxw_workbook* LWb;
lxw_worksheet* LWs;
lxw_format* LF;
FILE* record;

//Common controls

HWND cbPort, cbPrec, cbGanho;				//Combobox's
HWND edt, edt2;								//Edit
HWND rd, rd2, lb, chk1, chk2, chk3, chk4;	//Buttons
HWND st, st1, st2, st3, st4, st5;			//Statics

//Plot vars
auto& graph = Graficos::GetInstanceGNUPlot();

//Communication vars
COMMPORTS cp;
int nCursel;
DWORD idThread, readBytes, writeBytes, evt;
WCHAR  wbuffer[50], selectedPort[50];
char buffer[50];
HANDLE hThread, commPort;
OVERLAPPED olr, olw;
DataConf dc;
DataProtocol dp;
BOOL bRunning;

//Default configuration DialogOP
char localfile[100] = { "teste.txt" };
int baudrate = 9600;
char pathfile[MAX_PATH];

DWORD WINAPI Thread(LPVOID lp) {
	bRunning = TRUE;
	ZeroMemory(&olr, sizeof(OVERLAPPED));
	ZeroMemory(&olw, sizeof(OVERLAPPED));
	ZeroMemory(&cp, sizeof(COMMPORTS));
	ZeroMemory(&dc, sizeof(DataConf));
	olr.hEvent = CreateEvent(0, TRUE, FALSE, L"Read");
	olw.hEvent = CreateEvent(0, TRUE, FALSE, L"Write");
	if (olr.hEvent == NULL || olw.hEvent == NULL) {
		MessageBoxA((HWND)lp, "Erro CreateEVent", 0, MB_OK | MB_ICONERROR);
		return -1;
	}
	cp.dcb.BaudRate = baudrate;
	cp.dcb.DCBlength = sizeof(DCB);
	cp.dcb.fDtrControl = DTR_CONTROL_ENABLE;
	cp.dcb.ByteSize = 8;


	SendMessage(edt, WM_GETTEXT, sizeof(wbuffer), (LPARAM)wbuffer);
	if (SendMessage(chk4, BM_GETCHECK, 0, 0) == BST_CHECKED) {
		dc.prec = SendMessage(cbPrec, CB_GETCURSEL, 0, 0);
	}
	dc.ganho = SendMessage(cbGanho, CB_GETCURSEL, 0, 0) + 1;
	SendMessage(edt2, WM_GETTEXT, sizeof(wbuffer), (LPARAM)wbuffer);
	int u = dc.segs = wcstol(wbuffer, 0, 10);
	if (SendMessage(chk2, BM_GETCHECK, 0, 0) == BST_CHECKED) {
		memcpy(dc.localfile, localfile, strlen(localfile));
	}
	if (dc.segs == 0) {
		MessageBox((HWND)lp, L"Tempo de amostragem não definido ou invalido, por padrao a duraçao é de 60 segundos", L"Info", MB_OK | MB_ICONINFORMATION);
		dc.segs = 60;
	}

	SetCommState(commPort, &cp.dcb);
	if (WriteFile(commPort, &dc, sizeof(dc), &writeBytes, &olw) == FALSE) {
		if (GetLastError() == ERROR_IO_PENDING) {
			if (GetOverlappedResult(commPort, &olw, &writeBytes, TRUE) == FALSE) {
				sprintf_s(buffer, "Erro GetOverlappedResult:%d", GetLastError());
			}
		}
		else {
			sprintf_s(buffer, "Erro WriteFile:%d", GetLastError());
			MessageBoxA(0, buffer, 0, MB_ICONERROR | MB_OK);
			return -2;
		}
	}
	ZeroMemory(wbuffer, sizeof(wbuffer));
	BOOL PlotEnable = FALSE;


	if (SendMessage(chk3, BM_GETCHECK, 0, 0) == BST_CHECKED) {
		PlotEnable = TRUE;
		if (graph.IsGNUPlotRunning()) {
			MessageBox((HWND)lp, L"A ultima instância do GNUPlot sera fechada.", L"Aviso", MB_OK | MB_ICONWARNING);
			graph.FinishGNUPlotProgram();
		}
		if (!graph.StartGNUPlotProgram()) {
			MessageBox((HWND)lp, L"Erro ao iniciar GNUPlot. Verifique o caminho do executável ou se o programa foi instalado", L"Erro", MB_OK | MB_ICONERROR);
			PlotEnable = FALSE;
		}
		else {
			/******************Plot Configuration*********************/
			//Window's Title
			graph.CmdLine("set term wxt title 'GNUPlot - Leitor Serial'\n");
			graph.CmdLine("set xlabel 'Ordem/Numero de leitura'\n");
			graph.CmdLine("set ylabel 'Sinal Digital - 24 bits'\n");
			//Create Plot File
			fopen_s(&record, "tmpplot", "w");
			fprintf(record, "0 0\n");
			fclose(record);

			//Open gnuplot window
			graph.CmdLine("plot \"tmpplot\" every 2 with lines lt rgb 'red' title 'linha' \n");

			/*********************************************************/
		}
	}
	SetCommMask(commPort, EV_RXCHAR);
	clock_t begin = clock(), end = 0;
	DWORD e = 0;
	for (DWORD status = 0, i = 0; (end - begin) / CLOCKS_PER_SEC < dc.segs;) {
		end = clock();
		if (!WaitCommEvent(commPort, &evt, &olr)) {					//Verify serial port events
			if (GetLastError() == ERROR_IO_PENDING) {
				status = WaitForSingleObject(olr.hEvent, 5000);		//Waits until 5 seconds
				if (status == WAIT_TIMEOUT) {
					MessageBox((HWND)lp, L"Conexao expirou durante a leitura!", L"Erro", MB_OK | MB_ICONERROR);
					if (graph.IsGNUPlotRunning()) {
						graph.FinishGNUPlotProgram();
					}
					break;
				}
				if (status == WAIT_FAILED) {

					sprintf_s(buffer, "Erro WaitForSingleObject:%d", GetLastError());
					MessageBoxA(0, buffer, "Erro", MB_ICONERROR);
					break;
				}

			}
			else {
				sprintf_s(buffer, "Erro WaitCommEvent:%d", GetLastError());
				MessageBoxA(0, buffer, "Erro", MB_ICONERROR);
				COMSTAT cs;
				ClearCommError(commPort, &readBytes, &cs);
				break;
			}
		}

		if (evt & EV_RXCHAR) {

			ReadFile(commPort, &dp, sizeof(dp), 0, &olr);			  //Retorna FALSE ==> leitura nao concluida.
			GetOverlappedResult(commPort, &olr, &readBytes, TRUE);	  //Espera a leitura ser feita
			if (dp.signbegin[0] == 'B' && dp.signend[0] == 'E') {	  //Verifica integridade dos dados

				wsprintf(wbuffer, L"%d", dp.dt);
				/******Update plot file********/
				if (PlotEnable) {
					record = _fsopen("tmpplot", "a", SH_DENYNO);     //Open file on sharing mode
					if (record != NULL) {

						fprintf(record, "%d %d\n", e, dp.dt);
						if (!fclose(record)) {
							i++;		//plot offset
							e++;		//index order
							//Update plot chart
							if (i == 10) {
								graph.CmdLine("replot\n");
								i = 0;
							}
						}
					}

				}
				/*******************************/
				SendMessage(lb, LB_ADDSTRING, 0, (LPARAM)wbuffer);
				ZeroMemory(buffer, 50);
			}
			else {
				MessageBox((HWND)lp, L"Erro de sincronização, tente novamente.", L"Error", MB_OK | MB_ICONERROR);
				if (graph.IsGNUPlotRunning()) {
					graph.FinishGNUPlotProgram();
				}
				break;
			}
			memset(&dp, 0, sizeof(dp));
		}
		InvalidateRect((HWND)lp, &rect, FALSE);
	}
	bRunning = FALSE;
	CloseHandle(commPort);
	EnableWindow(cbPort, TRUE);
	EnableWindow(rd, TRUE);
	return 0;
}

INT_PTR CALLBACK  DialogOP(HWND h, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_INITDIALOG:
		SendMessageA(GetDlgItem(h, IDC_EDIT1), WM_SETTEXT, strlen(localfile), (LPARAM)localfile);
		SendMessage(GetDlgItem(h, IDC_BAUDRATE), WM_SETTEXT, 4, (LPARAM)L"9600");
		SendMessage(GetDlgItem(h, IDC_EDIT1), EM_SETLIMITTEXT, 100, 0);
		SendMessage(GetDlgItem(h, IDC_BAUDRATE), EM_SETLIMITTEXT, 6, 0);

		break;
	case WM_COMMAND:
	{
		if (LOWORD(wParam) == IDC_OPOK) {
			memset(localfile, 0, sizeof(localfile));
			SendMessageA(GetDlgItem(h, IDC_EDIT1), WM_GETTEXT, 100, (LPARAM)localfile);
			SendMessageA(GetDlgItem(h, IDC_BAUDRATE), WM_GETTEXT, 50, (LPARAM)buffer);
			baudrate = strtol(buffer, 0, 10);
			SendMessage(h, WM_CLOSE, 0, 0);
		}

	}
	break;
	case WM_CLOSE:
	{

		EndDialog(h, 0);

		return 0;
	}
	default:
		return DefWindowProc(h, msg, wParam, lParam);

	}
	return 0;
}


void ComponentG(HWND h, UINT m, WPARAM w, LPARAM l, HINSTANCE hi)
{

	rect.bottom = 100;
	rect.top = 10;
	rect.left = 600;
	rect.right = 300;
	cbPort = CreateWindowEx(0, WC_COMBOBOX, 0, CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_VISIBLE | WS_CHILD, 10, 10, 150, 100, h, 0, hi, 0);
	rd = CreateWindowEx(0, WC_BUTTON, L"Conectar", WS_VISIBLE | WS_CHILD, 10, 50, 100, 25, h, (HMENU)1404, hi, 0);
	rd2 = CreateWindowEx(0, WC_BUTTON, L"Iniciar", WS_VISIBLE | WS_CHILD | WS_DISABLED, 10, 520, 100, 25, h, (HMENU)1405, hi, 0);
	lb = CreateWindowEx(0, WC_LISTBOX, 0, WS_VISIBLE | WS_CHILD | LBS_STANDARD | LBS_HASSTRINGS | LBS_NOTIFY, 300, 110, 300, 500, h, 0, hi, 0);
	chk1 = CreateWindowEx(0, WC_BUTTON, L"Configurações", WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 10, 100, 250, 400, h, (HMENU)1407, hi, 0);
	st = CreateWindowEx(0, WC_STATIC, L"Casas decimais:", WS_VISIBLE | WS_CHILD | WS_DISABLED, 15, 130, 110, 20, h, 0, hi, 0);
	cbPrec = CreateWindowEx(0, WC_COMBOBOX, 0, CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_VISIBLE | WS_DISABLED | WS_CHILD, 15, 150, 150, 100, h, 0, hi, 0);
	chk4 = CreateWindowEx(0, WC_BUTTON, L"Tensão", WS_VISIBLE | WS_CHILD | BS_CHECKBOX | WS_DISABLED, 170, 150, 70, 20, h, (HMENU)1478, hi, 0);
	edt2 = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, 0, WS_VISIBLE | WS_CHILD | ES_NUMBER | WS_DISABLED, 15, 210, 150, 20, h, 0, hi, 0);
	st3 = CreateWindowEx(0, WC_STATIC, L"Tempo de amostragem:", WS_VISIBLE | WS_CHILD | WS_DISABLED, 15, 190, 160, 20, h, 0, hi, 0);
	st4 = CreateWindowEx(0, WC_STATIC, L"segundos", WS_VISIBLE | WS_CHILD | WS_DISABLED, 170, 210, 80, 20, h, 0, hi, 0);
	chk2 = CreateWindowEx(0, WC_BUTTON, L"Registro MicroSD", WS_VISIBLE | WS_CHILD | BS_CHECKBOX | BS_NOTIFY | WS_DISABLED, 15, 230, 150, 20, h, (HMENU)1444, hi, 0);
	chk3 = CreateWindowEx(0, WC_BUTTON, L"Plot", WS_VISIBLE | WS_CHILD | BS_CHECKBOX | BS_NOTIFY | WS_DISABLED, 15, 250, 100, 20, h, (HMENU)1445, hi, 0);
	st5 = CreateWindowEx(0, WC_STATIC, L"Ganho:", WS_VISIBLE | WS_CHILD | WS_DISABLED, 15, 280, 80, 20, h, 0, hi, 0);
	cbGanho = CreateWindowEx(0, WC_COMBOBOX, 0, CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_VISIBLE | WS_DISABLED | WS_CHILD, 15, 300, 150, 100, h, 0, hi, 0);


	SendMessage(cbPrec, CB_ADDSTRING, 0, (LPARAM)L"1");
	SendMessage(cbPrec, CB_ADDSTRING, 0, (LPARAM)L"0.1");
	SendMessage(cbPrec, CB_ADDSTRING, 0, (LPARAM)L"0.01");
	SendMessage(cbPrec, CB_ADDSTRING, 0, (LPARAM)L"0.001");
	SendMessage(cbPrec, CB_SETCURSEL, 0, 0);
	SendMessage(cbGanho, CB_ADDSTRING, 0, (LPARAM)L"1");
	SendMessage(cbGanho, CB_ADDSTRING, 0, (LPARAM)L"2");
	SendMessage(cbGanho, CB_ADDSTRING, 0, (LPARAM)L"3");
	SendMessage(cbGanho, CB_SETCURSEL, 0, 0);
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);
	sysFont = (HFONT)CreateFontIndirect(&ncm.lfMessageFont);
	SendMessage(rd, WM_SETFONT, (WPARAM)sysFont, 0);
	SendMessage(rd2, WM_SETFONT, (WPARAM)sysFont, 0);
	SendMessage(st, WM_SETFONT, (WPARAM)sysFont, 0);
	SendMessage(st1, WM_SETFONT, (WPARAM)sysFont, 0);
	SendMessage(st2, WM_SETFONT, (WPARAM)sysFont, 0);
	SendMessage(st3, WM_SETFONT, (WPARAM)sysFont, 0);
	SendMessage(st4, WM_SETFONT, (WPARAM)sysFont, 0);
	SendMessage(st5, WM_SETFONT, (WPARAM)sysFont, 0);
	SendMessage(cbPort, WM_SETFONT, (WPARAM)sysFont, 0);
	SendMessage(cbPrec, WM_SETFONT, (WPARAM)sysFont, 0);
	SendMessage(cbGanho, WM_SETFONT, (WPARAM)sysFont, 0);
	SendMessage(chk1, WM_SETFONT, (WPARAM)sysFont, 0);
	SendMessage(chk2, WM_SETFONT, (WPARAM)sysFont, 0);
	SendMessage(chk3, WM_SETFONT, (WPARAM)sysFont, 0);
	SendMessage(chk4, WM_SETFONT, (WPARAM)sysFont, 0);
	SendMessage(edt, WM_SETFONT, (WPARAM)sysFont, 0);
	SendMessage(edt2, WM_SETFONT, (WPARAM)sysFont, 0);
	SendMessage(lb, WM_SETFONT, (WPARAM)sysFont, 0);
}
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

	switch (message)
	{
	case WM_CREATE:
	{
		ComponentG(hWnd, message, wParam, lParam, hInst);


	}
	break;

	case WM_COMMAND:
	{
		if (HIWORD(wParam) == CBN_DROPDOWN) {
			if (IsWindowEnabled(cbPort)) {
				AddPortsNametoCB(&cp, cbPort);
			}
		}

		if (wParam == ID_ARQUIVO_SALVAR) {
			ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
			ofn.lStructSize = sizeof(OPENFILENAMEA);
			ofn.Flags = OFN_PATHMUSTEXIST | OFN_EXPLORER;
			ofn.hwndOwner = hWnd;
			ofn.nMaxFile = MAX_PATH;
			ofn.lpstrFile = pathfile;
			ofn.lpstrFilter = "Excel (.xlsx)\0*.*\0Text ANSI (.txt)\0*.*\0\0";
			if (GetSaveFileNameA(&ofn)) {
				if (ofn.nFilterIndex == 1) {			//Excel extension selected
					if (ofn.nFileExtension == 0) {
						sprintf_s(pathfile, "%s.xlsx", pathfile);
					}
					LWb = workbook_new(pathfile);
					LWs = workbook_add_worksheet(LWb, "Leitor Serial");
					LF = workbook_add_format(LWb);
					int qtLb = SendMessage(lb, LB_GETCOUNT, 0, 0);
					if (qtLb > 0) {
						for (long i = 0,c ; i < qtLb; i++) {
							SendMessageA(lb, LB_GETTEXT, i, (LPARAM)buffer);
							c = strtol(buffer, 0, 10);
							worksheet_write_number(LWs, i, 0, i, LF);
							worksheet_write_number(LWs, i, 1, c, LF);
						}
					}
					
					workbook_close(LWb);
				}
				if (ofn.nFilterIndex == 2) {			//txt extension selected
					if (ofn.nFileExtension == 0) {
						sprintf_s(pathfile, "%s.txt", pathfile);
					}
					int qtLb = SendMessage(lb, LB_GETCOUNT, 0, 0);
					fopen_s(&record, pathfile, "w");
					if (record == NULL) {
						MessageBox(hWnd, L"Erro ao salvar o arquivo", L"Erro", MB_OK | MB_ICONERROR);
						return -1;
					}
					if (qtLb > 0) {
						for (int i = 0; i < qtLb; i++) {
							SendMessageA(lb, WM_GETTEXT, i, (LPARAM)buffer);
							fprintf(record, "%s\n", buffer);

						}						
					}
					fclose(record);
				}


			}


		}
		if (wParam == 1444) {
			if (SendMessage(chk2, BM_GETCHECK, 0, 0) == BST_CHECKED) {
				SendMessage(chk2, BM_SETCHECK, BST_UNCHECKED, 0);
			}
			else {
				SendMessage(chk2, BM_SETCHECK, BST_CHECKED, 0);
			}
		}
		if (wParam == 1445) {
			if (SendMessage(chk3, BM_GETCHECK, 0, 0) == BST_CHECKED) {
				SendMessage(chk3, BM_SETCHECK, BST_UNCHECKED, 0);
			}
			else {
				SendMessage(chk3, BM_SETCHECK, BST_CHECKED, 0);
			}
		}
		if (wParam == 1478) {
			if (SendMessage(chk4, BM_GETCHECK, 0, 0) == BST_CHECKED) {
				SendMessage(chk4, BM_SETCHECK, BST_UNCHECKED, 0);
				EnableWindow(cbPrec, FALSE);
			}
			else {
				EnableWindow(cbPrec, TRUE);
				SendMessage(chk4, BM_SETCHECK, BST_CHECKED, 0);
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
		if (wParam == 1404) {

			SendMessage(lb, LB_RESETCONTENT, 0, 0);
			nCursel = SendMessage(cbPort, CB_GETCURSEL, 0, 0);
			SendMessage(cbPort, CB_GETLBTEXT, nCursel, (LPARAM)selectedPort);
			commPort = CreateFile(selectedPort, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
			if (commPort == INVALID_HANDLE_VALUE) {
				MessageBox(0, L"Erro ao conectar a porta", L"Erro", MB_OK | MB_ICONERROR);

			}
			else {
				EnableWindow(rd, FALSE);
				EnableWindow(rd2, TRUE);
				EnableWindow(cbPort, FALSE);
				EnableWindow(cbGanho, TRUE);
				EnableWindow(edt, TRUE);
				EnableWindow(edt2, TRUE);
				EnableWindow(st, TRUE);
				EnableWindow(st1, TRUE);
				EnableWindow(st2, TRUE);
				EnableWindow(st3, TRUE);
				EnableWindow(st4, TRUE);
				EnableWindow(st5, TRUE);
				EnableWindow(chk2, TRUE);
				EnableWindow(chk3, TRUE);
				EnableWindow(chk4, TRUE);

			}

		}
		if (wParam == 1405) {
			EnableWindow(cbPrec, FALSE);
			EnableWindow(cbGanho, FALSE);
			EnableWindow(rd2, FALSE);
			EnableWindow(edt, FALSE);
			EnableWindow(edt2, FALSE);
			EnableWindow(st, FALSE);
			EnableWindow(st1, FALSE);
			EnableWindow(st2, FALSE);
			EnableWindow(st3, FALSE);
			EnableWindow(st4, FALSE);
			EnableWindow(st5, FALSE);
			EnableWindow(chk2, FALSE);
			EnableWindow(chk3, FALSE);
			EnableWindow(chk3, FALSE);
			EnableWindow(chk4, FALSE);
			hThread = CreateThread(0, 0, Thread, (LPVOID)hWnd, CREATE_ALWAYS, &idThread);
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
		DrawText(hdc, wbuffer, 7, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
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
			if (MessageBox(hWnd, L"O programa está em execução. Tem certeza que deseja sair?", L"Aviso", MB_YESNO | MB_ICONEXCLAMATION) == IDNO) {
				return 0;
			}

		}
		if (graph.IsGNUPlotRunning()) {
			graph.FinishGNUPlotProgram();
		}
		TerminateThread(hThread, 0);
		CloseHandle(hThread);
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

