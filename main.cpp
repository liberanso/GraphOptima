/**********************************************************************************
Version: 1.0.1
Author: Mariya Zapletina
Noticies: 09.01.2015
***********************************************************************************/
#include "resource.h"
#include "CmnHdr.h"
#include "Func.h"

TCHAR szPathname[MAX_PATH]; // path to the file

/////////////////////plot wnd////////////////////////////////////////
TCHAR szGraphWndClass[] = _TEXT("GraphClass");
LRESULT CALLBACK WndGraph(HWND, UINT, WPARAM, LPARAM);

HINSTANCE hInst;
TCHAR szTitle[] = _TEXT("Win32");
TCHAR szMainWndClass[] = _TEXT("GraphOptima");

INT_PTR WINAPI MainDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL Dlg_OnInitDialog(HWND, HWND, LPARAM);
void Dlg_OnCommand(HWND, int, HWND, UINT);
void Dlg_OnClose(HWND);
/////////////////////////////////////////////////////////////////////
vector <dot> points; //vector for optimized dots from file
int quant = 0; // quantity of graphics
vector<string> names;
////////////////////////////////////////////////////////////////////

/////////////////enter point func////////////////////////////////////////////////
int WINAPI _tWinMain(HINSTANCE hInstance,	HINSTANCE,	LPTSTR lpCmdLine,	int) {
	InitCommonControls(); //this is to enable windows visual styles
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_GRAPHOPTIMA), NULL, MainDlgProc);
	return (0);
} 
////////////////////////////////////////////////////////////////////////////////

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
////////////////enter point func mainproc///////////////////////////////////////
INT_PTR WINAPI MainDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {

	case WM_COMMAND:  Dlg_OnCommand(hWnd, (int)LOWORD(wParam), (HWND)(lParam), (UINT)HIWORD(wParam)); return(TRUE); 
		break;
	case WM_INITDIALOG: Dlg_OnInitDialog(hWnd, hWnd, lParam); return (TRUE);
		break;
	case WM_CLOSE: Dlg_OnClose(hWnd); return(TRUE);
	case WM_DESTROY: PostQuitMessage(0); return(TRUE);
		break;
	}
	return(FALSE);
}
//////////////////////////////////////////////////////////////////////////////////
void Dlg_OnClose(HWND hwnd) {
	if (MessageBox(hwnd, _TEXT("Вы собираетесь завершить программу?"), _TEXT("Завершение работы"), MB_ICONQUESTION | MB_YESNO) == IDYES) {
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
		
	} break;
	case IDC_PLOT:
		GetDlgItemText(hwnd, IDC_FILENAME, szPathname, chDIMOF(szPathname));
		if (IsWindow(hGraph)) break;
		RegisterGraphClass();
		hGraph = CreateWindow(szGraphWndClass, _TEXT("Plot"), WS_SYSMENU | WS_POPUP | WS_VISIBLE | WS_THICKFRAME | WS_CAPTION, CW_USEDEFAULT, CW_USEDEFAULT, 700, 700, hwnd, 0, hInst, NULL);
		break;
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
		if (!points.empty())points.clear(); //чистим вектор с точками для построения
		if (!ReadFile(szPathname, points, quant, names)) {
			chMB("Cannot read the file. Please, try again. =(((");
			DestroyWindow(hWnd);
			return 12;
		}
		break;

	case WM_SIZE:
		sx = LOWORD(lParam);
		sy = HIWORD(lParam);
		break;

	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		DrawGraphics(hWnd, hdc, points, rect, quant, hline, hrect, sx, sy,names);
		EndPaint(hWnd, &ps);
		break;
	
	case WM_DESTROY:
		DeleteObject(hline);
		DeleteObject(hrect);
		if (!points.empty())points.clear(); //clear points for plotting
		break;
	default: return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

