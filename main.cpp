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
vector <string> names;
vector <string> buf; //vector of strings read from file by parent thread that should be copied to child thread

HANDLE parseEvent;
HANDLE readEvent;
HANDLE parseEnd;
HANDLE parseReady;
BOOL STOPREADPARSE = TRUE;

int quant = 0; // quantity of graphics
TCHAR szPathname[MAX_PATH]; // path to the file

int quantOfread = 0; //количество точек, считанных из файла родительским потоком
BOOL FileEnd = FALSE; //флаг, устанавливаемый родительским процессом, свидетельствует о том, что parseThread должен приостановить свою работу
dot **dots; // dots array at each moment containing three dots of each graphic to analize by AreInLine function 
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////enter point func////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int WINAPI _tWinMain(HINSTANCE hInstance,	HINSTANCE,	LPTSTR lpCmdLine,	int) {
	InitCommonControls(); //this is to enable windows visual styles
	MSG msg;

	parseEvent = CreateEvent(NULL, FALSE, FALSE, NULL); // means initially MainThread had not read anything from file to buf and there`s nothing for parseThread to copy
	parseReady = CreateEvent(NULL, FALSE, FALSE, NULL); //means initially parseThread is NOT ready to copy array read by main thread, 
	// is set after parseThread analyzed the hole temp array and is ready to stop or get another one
	//for the first time is set automatically at the beginning of parseThread
	parseEnd = CreateEvent(NULL, FALSE, FALSE, NULL); // means initially parseThread is working, is set by parseThread before it returns(0)
	readEvent = CreateEvent(NULL, FALSE, FALSE, NULL); // means that parseThread copied the block and main thread can read another one
	

	DWORD dwThreadID;
	HANDLE parseThread = _mbeginthreadex(NULL, 0, ParseThread, NULL, 0, &dwThreadID);

	DialogBox(hInstance, MAKEINTRESOURCE(IDD_GRAPHOPTIMA), NULL, MainDlgProc);
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	STOPREADPARSE = TRUE; //если родительский поток закрывается 
	SetEvent(parseEvent); //посылаем наш флажок parseThreadу

	//ждем от дочернего потока подтверждения его завершения
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

	case WM_COMMAND:  return (SetDlgMsgResult(hWnd, uMsg, HANDLE_WM_COMMAND(hWnd, wParam, lParam, Dlg_OnCommand)));
		break;
	case WM_INITDIALOG: return (SetDlgMsgResult(hWnd, uMsg, HANDLE_WM_INITDIALOG(hWnd, wParam, lParam, Dlg_OnInitDialog)));
		break;
	case WM_CLOSE: //Dlg_OnClose(hWnd);
		return (SetDlgMsgResult(hWnd, uMsg, HANDLE_WM_CLOSE(hWnd, wParam, lParam, Dlg_OnClose)));
	case WM_DESTROY: PostQuitMessage(0); return(FALSE); 
		break;
	//default: return (FALSE);
	}
	return(FALSE);
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Dlg_OnClose(HWND hwnd) {
	if (MessageBox(hwnd, _TEXT("Вы хотите завершить программу?"), _TEXT("Завершение работы"), MB_ICONQUESTION | MB_YESNO) == IDYES) {
		DestroyWindow(hwnd); //sends WM_DESTROY (before its children are destroyed) and WM_NCDESTROY(after) messages to hwnd window to deactivate it and remove the keyboard focus from it
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
		EndDialog(hwnd, id); //Destroys a modal dialog box, causing the system to end any processing for the dialog box.
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
		GetDlgItemText(hwnd, IDC_FILENAME, szPathname, (sizeof(szPathname)/sizeof(szPathname[0])));
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		char str[20];
		FileEnd = FALSE; //как только мы нажали на кнопку Построить, конец файла не достигнут

		FILE *f_in = _tfopen((const TCHAR *)szPathname, _TEXT("r")); //пытаемся открыть файл
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

		STOPREADPARSE = FALSE; //прочитали число графиков из файла, их названия, создали структуру под точки - говорим дочернему потоку, мол, можно готовиться к чтению
		SetEvent(parseReady);// тут же говорим, что parseThread может принимать массив (это самое начало, файл только загрузили, parseThread должен быть свободен всё равно

		
		while (!feof(f_in)) {//пока не конец файла, читаем его в массив
			int ch = 0;
			buf.resize(0); //затираем содержимое вектора, которое было считано из файла на прошлой итерации
			while ((!feof(f_in)) && (ch < (50 * (quant + 1)))) { //читаем массивом по 50 точек

				if ((fscanf(f_in, "%s", str) != 0)&&(!feof(f_in)&&(!ferror(f_in)))) {
					buf.push_back(str);
					ch++;
					quantOfread = ch;
				}
			}

			WaitForSingleObject(parseReady, INFINITE); //если parseThread готов к принятию блока (либо на первой итерации главный поток сам себе это сказал,
			//либо на последующих итерациях это ему сообщает parseThread)
			SetEvent(parseEvent); //поднимает флажок, мол, мэйн поток считал кусок файла, parseThread, принимай
			WaitForSingleObject(readEvent, INFINITE); //ждем, пока дочерний поток скопирует блок
		}
		
		if (feof(f_in)) FileEnd = TRUE; //если вылетели из вайла, достигнув конца файла, то поднимаем флажок конца 
		WaitForSingleObject(parseReady, INFINITE); //ждем, пока parseThread обработает наш блок
	/////////////////////////////////////////////////////////////////
		//а если ferror(f_in)??????????????
	/////////////////////////////////////////////////////////////////

		SetEvent(parseEvent); //посылаем parseThread наш флажок конца файла
		WaitForSingleObject(parseReady, INFINITE); //ждем, пока он на флажок посмотрит и уснет до следующего файла
		//вообще говоря, наверное, это необязательно, но пусть пока будет
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		if (IsWindow(hGraph)) break; //если окошко с графиком уже существует в системе, то не пересоздаем его
		RegisterGraphClass(); //регистрируем класс окошка с графиком
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
	
	WaitForSingleObject(parseEvent, INFINITE); //ждем, пока мэйн поток считает кусок файла и разрешит нам его копировать
	while (!STOPREADPARSE) { //если не поднят флажок конца работы
		BOOL ShutDown = (FileEnd == TRUE); //смотрим, не достигнут ли конец файла

		double tmp_x = 0.0, tmp_y = 0.0;
		int first = 1, tempSize = 0;
		char *pEnd;

		while (!ShutDown) { //если не конец файла
			
				//создаем временный char array[][] для блока, пришедшего из мэйн потока
				char **temp = new char*[quantOfread];
				for (int i = 0; i != quantOfread; i++) {
					temp[i] = new char[20];
				}

				//копируем блок во временный char array[][]
				for (int i = 0; i != quantOfread; i++) {
					strncpy(temp[i], buf[i].c_str(), 20);
				}

				tempSize = quantOfread; //пишем размер считанного блока во внутренню переменную, потому что quantOfread может измениться в мэйн потоке,
				//пока тут будет предыдущий блок файла обрабатываться здесь
								
				SetEvent(readEvent); //говорим мэйн потоку, что он может читать следующий кусок файла				

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
						first = 0; //считали первые точки, сбросили флажок
						k = 2 * (1 + quant);
						for (int q = 0; q != quant; q++) points.push_back(dots[q][0]); //забили первую точку каждого графика в результирующий массив
					} 
					else chMB("Файл слишком короткий! Проверьте содержимое файла."); //если в файле было меньше 3 точек на каждый график
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
								dots[q][0] = dots[q][1];
								dots[q][1] = dots[q][2];
							}
						}
					}
				}
				
				//считали блок, чистим наш временный char array[][], чтобы на следующей итерации в нем не было мусора
				for (int l = 0; l != tempSize; l++) {
					delete[]temp[l];
				}

				SetEvent(parseReady); //обработали весь блок, говорим родительскому потоку, что ждем от него очередную порцию
			}
				
		//достигнув конца файла, чистим содержимое временных структур с точками и одновременно пушим последнюю точку в результирующий файл
		for (int m = 0; m != quant; m++) {
			points.push_back(dots[m][2]);
			delete[] dots[m];
		}

		SetEvent(parseReady);
		// в этом месте у нас дотс почищены, ShutDown = TRUE,
		//parseThread говорит, что готов к труду и обороне
		// и засыпает в ожидании дальнейших указаний от мэйн потока
		WaitForSingleObject(parseEvent, INFINITE);

	} //stopreadparse = true

	SetEvent(parseEnd); //этого ивента от нас при закрытии ждет мэйн поток
	return(0);
}
