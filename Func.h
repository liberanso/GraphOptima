/***********************************************************************************************************************
* 
* Считаем, что входной файл имеет кодировку ANSI (нет проверки на 
* Unicode/noUnicode)
* По сути, это основной функциональный код программы
*
************************************************************************************************************************/
#include <Windows.h> //API functions
#include <cstdio> //printf/scanf/fscanf
#include <cfloat> //DBL_EPSILON
#include <cstdlib> //atof
#include <vector>
#include <cmath>
#include <string.h>
#include <tchar.h>
#include <process.h> // to access to _beginthreadex
#include <CommCtrl.h> //this header and two next pragmas are to enable visual styles of windows for this app


#pragma comment(linker, \
  "\"/manifestdependency:type='Win32' "\
  "name='Microsoft.Windows.Common-Controls' "\
  "version='6.0.0.0' "\
  "processorArchitecture='*' "\
  "publicKeyToken='6595b64144ccf1df' "\
  "language='*'\"")
#pragma comment(lib, "ComCtl32.lib")

struct dot {	double x; 	double y; 	int num; 	}; // num - порядковый номер графика
using std::vector;
using std::string;
const double eps = DBL_EPSILON; //machine zero for double

/////////for graphics////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const int scaleX = 6; //число меток на оси Х
const int scaleY = 6; //чисто меток на оси У
const int indent = 25; //отступ
const int GRAPHSIZE = 1200; //размер окна в логических единицах
const int GRAPHWIDTH = 1000; //размер рабочей области окна в лог.единицах
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline BOOL AreInLine(struct dot, struct dot, struct dot ); //алгоритм определения, лежат ли 3 точки на одной прямой,
//основывается на уравнении прямой по двум точкам. Находится разность правой и левой частей 
//полученное значение сравнивается с машинным нулем.  
//Аргументы - три структуры с координатами х и у. 

void DrawPoint(HDC, struct dot, double, double, dot *, int *, double, double, double, double, double, double,vector<string>); // аргументы: окно, где рисуется, структура с координатами точки, коэффициенты перевода
//координаты точки из файла на размер окна (2), указатель на массив, хранящий последнюю построенную точку для каждого графика, массив флагов,
//показывающих, построена ли хотя бы первая точка этого графика или нет

BOOL DrawGraphics(HDC hdc, vector<dot>, int, int, int, vector<string>); // окно, окно, вектор с точками, структура из оконной процедуры дочернего окна, число графиков
//функция, вызываемая в main.cpp для рисования в дочернем pop-up окне

void ProceedNames(HDC, vector<string>, int, int, int);
//функция выводит имена графиков

void DrawGrid(HDC, int, int, double, double, double, double, double, double);
//рисует сетку
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline BOOL AreInLine(struct dot one, struct dot two, struct dot three) {
	
	double comp1 = (three.x - one.x)*(two.y - one.y), comp2=(three.y - one.y)*(two.x - one.x);
	double k = (one.y - two.y) / (one.x - two.x);
	double b = (one.y - ((one.y - two.y) / (one.x - two.x)*one.x));
	double comp3 = k*three.x + b;
	if (fabs(comp1 - comp2) <= pow(10.0,3.0)*DBL_EPSILON*fmax(comp1, comp2)) return (TRUE); 
	else if (fabs(three.y - comp3) <= pow(10.0,3.0)*DBL_EPSILON*fmax(three.y, comp3)) return (TRUE);
	//if (fabs((three.x - one.x)*(two.y - one.y) - (three.y - one.y)*(two.x - one.x)) <= DBL_EPSILON*fmax(comp1, comp2)) return (TRUE);
	//else if (fabs(three.y - (one.y - two.y) / (one.x - two.x)*three.x + (one.y - ((one.y - two.y) / (one.x - two.x)*one.x))) <= DBL_EPSILON*fmax(three.y, comp3))return (TRUE);
	else return (FALSE);
}

BOOL DrawGraphics(HDC hdc, vector<dot> vec, int quantity, int sx, int sy, vector<string> names) {
	double max_x, min_x, max_y, min_y, hx, hy;
	double k_x = 1.0, k_y = 1.0;
	dot *points = new dot[quantity];
	int *flag = new int[quantity];
	
	for (int i = 0; i != quantity; i++) { //нужно для определения заполненности массива с последней точкой/ 0 - ни одной точки графика не нарисовано
		flag[i] = 0;
	}

	max_x = vec[0].x; max_y = vec[0].y;
	min_x = vec[0].x; min_y = vec[0].x;

	for (vector <dot> :: iterator i = vec.begin(); i != vec.end(); i++) {
		if ((*i).x > max_x) max_x = (*i).x;
		if ((*i).x < min_x) min_x = (*i).x;
		if ((*i).y > max_y) max_y = (*i).y;
		if ((*i).y < min_y) min_y = (*i).y;
	}
		
	hx = max_x - min_x;
	hy = max_y - min_y;

	
	DrawGrid(hdc, sx, sy, max_x, min_x, max_y, min_y, hx, hy);
	

	//for (vector <dot> ::iterator i = vec.begin(); i != vec.end(); i++) {
	//DrawPoint(hdc, (*i), k_x, k_y, points, flag, max_x, min_x, max_y, min_y, hx, hy,names);
	//}

	//ProceedNames(hdc, names, quantity, sx, sy);

	delete[] flag;
	delete[] points;
	

	return (TRUE);
}

void DrawPoint(HDC hdc, struct dot point, double k_x, double k_y, dot *mas, int *flag, double max_x, double min_x, double max_y, double min_y,double hx, double hy, vector<string> names) {
	
	//рисуем точку
	//двигаем перо в неё, рисуем линию от нее до последней нарисованной точки этого графика цветом, зависящим от номера графика, если эта точка есть
	//пишем в массив эту точку
	//flag=0, если ни одной точки этого графика ещё не было нарисовано
	

///////////////////draw the rectangle for point/////////////////////////////
	HPEN pen;
	HBRUSH brush;
	pen = CreatePen(PS_DASHDOTDOT, 7, RGB(120, (11+40*point.num%243), 15));
	//brush = CreateSolidBrush(RGB(120, (11 + 40 * point.num % 243), 15));
	brush = CreateSolidBrush(RGB((170 * point.num % 235), (170 * point.num % 235), (170 * point.num % 235)));
	SelectObject(hdc, pen);
	SelectObject(hdc, brush);
	RECT r;
	SetRect(&r, (int)((point.x - min_x)*GRAPHWIDTH / hx + 0.5)-5, (int)((point.y - min_y)*GRAPHWIDTH / hy + 0.5)-5, (int)((point.x - min_x)*GRAPHWIDTH / hx + 0.5)+5, (int)((point.y - min_y)*GRAPHWIDTH / hy + 0.5) + 5);
	FillRect(hdc, &r, brush);
	//SetPixel(hdc, (int)((point.x-min_x)*GRAPHWIDTH/hx+0.5),(int) ((point.y-min_y)*GRAPHWIDTH/hy+0.5), RGB((200 * point.num %235 +20), (200 * point.num %255 +20), (200 * point.num %235+20)));
////////////////////////////////////////////////////////////////////////////
		
	if (flag[point.num]) { //if there are drawn points for this graphic
		pen = CreatePen(((point.num) % 4), 3, RGB((170 * point.num %235), (170 * point.num %235), (170 * point.num %235)));
		SelectObject(hdc, pen);
		SelectObject(hdc, brush);
		MoveToEx(hdc, (int)((mas[point.num].x - min_x)*GRAPHWIDTH/hx+0.5), (int) ((mas[point.num].y - min_y)*GRAPHWIDTH/hy+0.5), NULL);
		LineTo(hdc, (int) ((point.x-min_x)*GRAPHWIDTH/hx+0.5), (int) ((point.y-min_y)*GRAPHWIDTH/hy+0.5));
	}
	else {
		flag[point.num] = 1;
	}
		mas[point.num] = point;
		DeleteObject(brush);
		DeleteObject(pen);
};

void ProceedNames(HDC hdc, vector<string> names, int quantity, int sx, int sy) {
	
	HBRUSH brush=CreateSolidBrush(RGB(0,0,0));
	for (int i = 0; i != quantity; i++) {
		brush = CreateSolidBrush(RGB(200,10,11));
		SelectObject(hdc, brush);
		///////////////proceed the names////////////////////////////////////////////
		RECT namequad;
		SetRect(&namequad, i * 100 + 4, GRAPHSIZE - 92, i * 100 + 28, GRAPHSIZE - 118);
		FillRect(hdc, &namequad, brush);
		brush = CreateSolidBrush(RGB((170 * i % 235), (170 * i % 235), (170 * i % 235)));
		SelectObject(hdc, brush);
		SetRect(&namequad, i * 100 + 8, GRAPHSIZE - 98, i * 100 + 22, GRAPHSIZE - 112);
		FillRect(hdc, &namequad, brush);
		SetTextAlign(hdc, TA_LEFT | TA_TOP);
		
		wchar_t des[10];
		std::mbstowcs(des, names[i].c_str(), names[i].length());
		LPCWSTR lpcwstr = des;
		////////////////////////////////////////////////////////////////////////////
		TextOut(hdc, (int)i * 100 + 30, (int)(GRAPHSIZE - 105), lpcwstr, names[i].length());
	}
	DeleteObject(brush);
}

void DrawGrid(HDC hdc, int sx, int sy, double max_x, double min_x, double max_y, double min_y, double hx, double hy) {
	HPEN pengrid_one, pengrid_two, pengrid_dash,oldpen;
	SIZE textSize;
	pengrid_one= CreatePen(PS_SOLID, 1, RGB(120, 30, 50));
	pengrid_two = CreatePen(PS_SOLID, 3, RGB(0, 0, 0));
	pengrid_dash = CreatePen(PS_DASH, 1, RGB(120, 30, 50));
	oldpen = (HPEN)SelectObject(hdc,pengrid_one);

	SetMapMode(hdc, MM_ANISOTROPIC);
	SetWindowExtEx(hdc, GRAPHSIZE, -GRAPHSIZE, NULL); //window 1200x1200 logical units
	GetTextExtentPoint32(hdc, _TEXT("0.0e+007"), _tcslen(_TEXT("0.0e+007")), &textSize);
//	int width_intent = textSize.cx;
//	int height_intent = textSize.cy;
	SetViewportExtEx(hdc, sx, sy, NULL); //sx x sy client area in device units
	SetViewportOrgEx(hdc, 3*indent,  sy-indent , NULL); //function specifies which window point maps to the viewport origin (0,0) in device units
	
	SetTextAlign(hdc, TA_CENTER | TA_TOP);
	int x_gr, y_gr, i;
	TCHAR s[20];
	double grid_x, grid_y;
	
	for (grid_x = min_x, i = 0; i <= 5 * scaleX; grid_x += hx / (5 * scaleX), i++) {
		x_gr = (int)((grid_x - min_x)*GRAPHWIDTH / (max_x - min_x) + 0.5);
		MoveToEx(hdc, x_gr, 0, NULL);
		LineTo(hdc, x_gr, GRAPHWIDTH);
	}
	for (grid_y = min_y, i = 0; i <= 5 * scaleY; grid_y += hy / (5 * scaleY), i++) {
		y_gr = (int)((grid_y - min_y)*GRAPHWIDTH / (max_y - min_y) + 0.5);
		MoveToEx(hdc, 0, y_gr, NULL);
		LineTo(hdc, GRAPHWIDTH, y_gr);
	}
	SelectObject(hdc, oldpen);
	DeleteObject(pengrid_one);
	
	for (grid_x = min_x, i = 0; i <= scaleX; grid_x += hx / scaleX, i++) {
		x_gr = (int)((grid_x - min_x)*GRAPHWIDTH / (max_x - min_x) + 0.5);
		_stprintf(s, _TEXT("%.1le"), grid_x);
		TextOut(hdc, x_gr, -5, s, _tcsclen(s));
		oldpen = (HPEN)SelectObject(hdc, pengrid_one);
		MoveToEx(hdc, x_gr, -10, NULL);
		LineTo(hdc, x_gr, 10);
		oldpen = (HPEN)SelectObject(hdc, pengrid_dash);
		LineTo(hdc, x_gr, GRAPHWIDTH);
	}
	oldpen = (HPEN)SelectObject(hdc, pengrid_two);
	MoveToEx(hdc, 0, 0, NULL);
	LineTo(hdc, GRAPHWIDTH, 0);
	SetTextAlign(hdc, TA_RIGHT | TA_BOTTOM);
	
	for (grid_y = min_y+hy/scaleY, i = 0; i <= scaleY; grid_y += hy / scaleY, i++) {
		y_gr = (int)((grid_y - min_y)*GRAPHWIDTH / (max_y - min_y) + 0.5);
		_stprintf(s, _TEXT("%.1le"), grid_y);
		TextOut(hdc, -5, y_gr, s, _tcsclen(s));
		
		oldpen = (HPEN)SelectObject(hdc, pengrid_two);
		MoveToEx(hdc, -10, y_gr, NULL);
		LineTo(hdc, 10, y_gr);
		oldpen = (HPEN)SelectObject(hdc, pengrid_dash);
		LineTo(hdc, GRAPHWIDTH, y_gr);
	}
	oldpen = (HPEN)SelectObject(hdc, pengrid_two);
	MoveToEx(hdc, 0, 0, NULL);
	LineTo(hdc, 0, GRAPHWIDTH);
	SelectObject(hdc, oldpen);
	DeleteObject(pengrid_two);
	DeleteObject(pengrid_one);
	DeleteObject(pengrid_dash);

}


////////////END OF FILE/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
