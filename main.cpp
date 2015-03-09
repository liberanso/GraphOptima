/*************************************************************************************************************************************************************
Version: 3.0.1
Author: Mariya Zapletina
Noticies: 09/03/15
**************************************************************************************************************************************************************/
#include "resource.h"
#include "CmnHdr.h"
#include "Func.h"
//#include "macros.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TCHAR szGraphWndClass[] = _TEXT("GraphClass");
HINSTANCE hInst;
TCHAR szTitle[] = _TEXT("Win32");
TCHAR szMainWndClass[] = _TEXT("GraphOptima");

LRESULT CALLBACK WndGraph(HWND, UINT, WPARAM, LPARAM);
INT_PTR WINAPI MainDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL Dlg_OnInitDialog(HWND, HWND, LPARAM);
void Dlg_OnCommand(HWND, int, HWND, UINT);
void Dlg_OnClose(HWND);
DWORD WINAPI ParseThread(PVOID);
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
vector <dot> points; //vector for optimized dots from file
int quant = 0; // quantity of graphics
vector <string> names;
HANDLE parseEvent;
HANDLE readEvent;
HANDLE parseEnd;
HANDLE parseReady;

TCHAR szPathname[MAX_PATH]; // path to the file
vector <string> buf; //pointer to vector of strings read from file by parent thread that should be copied to child thread
int quantOfread = 0; //количество считанных из файла точек родительским потоком
BOOL ShutUp = FALSE; //флаг, устанавливаемый родительским процессом
//свидетельствует о том, что чайлд поток должен прекратиться
dot **dots;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////enter point func////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int WINAPI _tWinMain(HINSTANCE hInstance,	HINSTANCE,	LPTSTR lpCmdLine,	int) {
	InitCommonControls(); //this is to enable windows visual styles

	parseEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	parseReady = CreateEvent(NULL, FALSE, TRUE, NULL); //means initially parse is ready to copy read array
	parseEnd = CreateEvent(NULL, FALSE, FALSE, NULL);
	readEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	DWORD dwThreadID;
	HANDLE parseThread = _mbeginthreadex(NULL, 0, ParseThread, NULL, 0, &dwThreadID);
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_GRAPHOPTIMA), NULL, MainDlgProc);
	
	ShutUp = TRUE; //если родительский поток закрывается
	SetEvent(readEvent); //отсылаем сообщение дочернему

	//ждем от дочернего потока подтверждения и его завершения
	WaitForSingleObject(parseEnd, INFINITE); //checked
	CloseHandle(parseEvent);
	CloseHandle(readEvent);
	CloseHandle(parseEnd);
	CloseHandle(parseReady);
	CloseHandle(parseThread);
	return (0);
} 
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ATOM RegisterGraphClass() {
	WNDCLASSEX wgex = { 0 }; //Wnd Graph EX
	wgex.cbSize = sizeof(WNDCLASSEX);
	wgex.style = CS_HREDRAW | CS_VREDRAW;
	wgex.lpfnWndProc = WndGraph;
	wgex.hInstance = hInst;
	wgex.hCursor = LoadCursor(NULL, IDC_CROSS);
	wgex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wgex.lpszClassName = szGraphWndClass;
	wgex.hIconSm = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON2));
	return RegisterClassEx(&wgex);
}

////////////////enter point func mainproc//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT_PTR WINAPI MainDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {

	case WM_COMMAND:  Dlg_OnCommand(hWnd, (int)LOWORD(wParam), (HWND)(lParam), (UINT)HIWORD(wParam)); return(TRUE); //chHANDLE_DLGMSG
		break;
	case WM_INITDIALOG: Dlg_OnInitDialog(hWnd, hWnd, lParam); return (TRUE);
		break;
	case WM_CLOSE: Dlg_OnClose(hWnd); //return(TRUE);
	case WM_DESTROY: PostQuitMessage(0); return(FALSE); //changed
		break;
	}
	return(FALSE);
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Dlg_OnClose(HWND hwnd) {
	if (MessageBox(hwnd, _TEXT("Вы хотите завершить программу?"), _TEXT("Завершение работы"), MB_ICONQUESTION | MB_YESNO) == IDYES) {
		SetEvent(parseEnd);
		DestroyWindow(hwnd);
	}
}
BOOL Dlg_OnInitDialog(HWND hWnd, HWND hwndFocus, LPARAM lParam) {

	SendMessage(hWnd, WM_SETICON, TRUE, (LPARAM)LoadIcon((HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), MAKEINTRESOURCE(IDI_GRAPHOPTIMA)));
	SendMessage(hWnd, WM_SETICON, FALSE, (LPARAM)LoadIcon((HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), MAKEINTRESOURCE(IDI_GRAPHOPTIMA)));

	// Initialize the dialog box by disabling the Plot button
	EnableWindow(GetDlgItem(hWnd, IDC_PLOT), FALSE);
	return(TRUE);
}

void Dlg_OnCommand(HWND hwnd, int id, HWND hwndCtrl, UINT CodeNotify) { //главное окно
	static HWND hGraph;

	switch (id) {
	case IDCANCEL:
		EndDialog(hwnd, id);
		break;

	case IDC_FILENAME:
		EnableWindow(GetDlgItem(hwnd, IDC_PLOT), Edit_GetTextLength(hwndCtrl) > 0); //если в поле пути к файлу у нас ненулевая строка, то активируем кнопку Построить
		break;

	case IDC_FILESELECT: {
		OPENFILENAME ofn;
		ofn.lStructSize = sizeof(OPENFILENAME); //
		ofn.hwndOwner = hwnd;
		ofn.lpstrFilter = _TEXT("Текстовые файлы\0*.txt\0\0"); // .txt files needed
		ofn.lpstrCustomFilter = NULL;
		ofn.lpstrFile = szPathname;
		ofn.lpstrFile[0] = 0;
		ofn.nMaxFile = sizeof(szPathname) / sizeof(szPathname[0]);
		ofn.lpstrFileTitle = NULL; //
		ofn.lpstrInitialDir = NULL;
		ofn.lpstrTitle = _TEXT("Выберите файл для построения");
		ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
		ofn.lpstrDefExt = NULL;
		GetOpenFileName(&ofn);
		SetDlgItemText(hwnd, IDC_FILENAME, ofn.lpstrFile);
		SetFocus(GetDlgItem(hwnd, IDC_PLOT)); //выделяем кнопку "Построить"

	}
		break;

	case IDC_PLOT:
	{
		GetDlgItemText(hwnd, IDC_FILENAME, szPathname, chDIMOF(szPathname));
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		char str[20];
		FILE *f_in = _tfopen((const TCHAR *)szPathname, _TEXT("r"));
		if (f_in == NULL) {
			chMB("Невозможно открыть файл!");
			return;
		}

		if (fscanf(f_in, "%d", &quant) == 0) { //quantity of graphics
			chMB("Файл нарушен. Проверьте содержимое файла.");
			return;
		}

		for (int i = 0; i != quant; i++) {
			if (fscanf(f_in, "%s", str) != 0) {
				names.push_back(str);
			}
			else {
				chMB("Файл нарушен. Проверьте содержимое файла.");
				return;
			}
		}
		dots = new dot*[quant]; //array to save three last dots of each graphic
		for (int i = 0; i != quant; i++) {
			dots[i] = new dot[3];
		}

		while (!feof(f_in)) {
			int ch = 0;
			buf.resize(0); //затираем содержимое вектора, которое было считано из файла на прошлой итерации
			while ((!feof(f_in)) && (ch < (10 * (quant + 1)))) { //читаем массивом по 50 точек

				if ((fscanf(f_in, "%s", str) != 0)&&(!feof(f_in)&&(!ferror(f_in)))) {
					buf.push_back(str);
					ch++;
					quantOfread = ch;
				}
			}
			WaitForSingleObject(parseReady, INFINITE);
			SetEvent(parseEvent);
			WaitForSingleObject(readEvent, INFINITE);
		}
		if (feof(f_in)) ShutUp = TRUE;
		SetEvent(parseEvent);
		WaitForSingleObject(parseEnd, INFINITE);
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		if (IsWindow(hGraph)) break;
		RegisterGraphClass();
		hGraph = CreateWindow(szGraphWndClass, _TEXT("Plot"), WS_SYSMENU | WS_POPUP | WS_VISIBLE | WS_THICKFRAME | WS_CAPTION, CW_USEDEFAULT, CW_USEDEFAULT, 700, 700, hwnd, 0, hInst, NULL);
		
		break;
	}
	}
}

LRESULT CALLBACK WndGraph(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	PAINTSTRUCT ps;
	HDC hdc;
	RECT rect;
	static HPEN hline;
	static HBRUSH hrect;
	static int sx, sy;

	GetClientRect(hWnd, &rect);

	switch (message) {
	case WM_CREATE:
		break;

	case WM_SIZE:
		sx = LOWORD(lParam);
		sy = HIWORD(lParam);
		InvalidateRect(hWnd, NULL, TRUE);
		//UpdateWindow(hWnd);
		//RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE);
		return 0;
		break;

	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		DrawGraphics(hWnd, hdc, points, rect, quant, hline, hrect, sx, sy, names);
		EndPaint(hWnd, &ps);
		break;
	
	case WM_DESTROY:
		DeleteObject(hline);
		DeleteObject(hrect);
		if (!points.empty())points.resize(0); //clear points for plotting
		break;
	default: return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

DWORD WINAPI ParseThread(PVOID pvParam) {
	BOOL ShutDown = FALSE; //предполагаем, что поток будет работать вечно
	double tmp_x = 0.0, tmp_y = 0.0;
	int first = 1, tempSize=0;
	char *pEnd;
	
	//dots were here
	
	while (!ShutDown) {
		WaitForSingleObject(parseEvent, INFINITE);
		ShutDown = (ShutUp == TRUE); //проверяем, не установил ли родительский процесс флаг окончания
		if (!ShutDown) {
			
			//dots were here
			//копируем блок во временный char array[][]
			 char **temp = new char*[quantOfread];
			for (int i = 0; i != quantOfread; i++) {
				temp[i] = new char[20];
			}
			for (int i = 0; i != quantOfread; i++) {
				strncpy(temp[i], buf[i].c_str(), 20);
			}
			tempSize = quantOfread;
			//освобождаем блок памяти для родительского потока
			SetEvent(readEvent); //этого ивента ждет родительский поток, чтобы снова считать блок из файла.
			//родительский поток ставит ивент parseEvent, который ждет ЭТОТ поток, когда родительский читает блок

			//обрабатываем скопированный блок
			int k = 0, j = 0;
			if (first) { //читаем по 2 первые точки в каждый график
				if ((tempSize - 3 * (1 + quant)) >= 0) { //проверяем, что хотя бы по 3 точки каждого графика в файле присутствуют
					for (j = 0, k = 0; k != 2 * (1 + quant), j != 2; j++) {
						tmp_x = strtod(temp[k], &pEnd);
						k++;
						for (int q = 0; q != quant; q++) {
							tmp_y = strtod(temp[k], &pEnd);
							k++;
							dots[q][j].x = tmp_x;
							dots[q][j].y = tmp_y;
							dots[q][j].num = q;
						}
					}
					first = 0;
					k = 2 * (1 + quant);
					for (int q = 0; q != quant; q++) points.push_back(dots[q][0]);
				}
				else chMB("Файл слишком короткий! Проверьте содержимое файла.");
			}
			if (!first) {
				for (k; k != tempSize;) {
					tmp_x = strtod(temp[k], &pEnd);
					k++;
					for (int q = 0; q != quant; q++) {
						tmp_y = strtod(temp[k], &pEnd);
						k++;
						dots[q][2].x = tmp_x;
						dots[q][2].y = tmp_y;
						dots[q][2].num = q;
						if (AreInLine(dots[q][0], dots[q][1], dots[q][2])) {
							dots[q][1] = dots[q][2];
						}
						else {
							points.push_back(dots[q][1]);
						//	if (k == tempSize) points.push_back(dots[q][2]);
							dots[q][0] = dots[q][1];
							dots[q][1] = dots[q][2];
						}
					}
				}
			}
			for (int l = 0; l != tempSize; l++) {
				delete []temp[l];
			}
			//delete dots were here
			SetEvent(parseReady);
		}
		
	}
	//delete dots here
	for (int m = 0; m != quant; m++) {
		delete[] dots[m];
	}
		
	SetEvent(parseEnd);
	WaitForSingleObject(parseEvent, INFINITE);
	return(0);
}

