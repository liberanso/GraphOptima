/*********************************************************************
* 
* Считаем, что входной файл имеет кодировку ANSI (нет проверки на 
* Unicode/noUnicode)
* По сути, это основной функциональный код программы
*
*********************************************************************/
#include <Windows.h> //API functions
#include <cstdio> //printf/scanf/fscanf
#include <cfloat> //DBL_EPSILON
#include <cstdlib> //atof
#include <vector>
#include <cmath>
#include <string.h>
#include <tchar.h>
#include <CommCtrl.h> //this and two next pragmas are to enable visual styles of windows for this app

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

/////////for graphics//////////////////////////////////////////////////
const int scaleX = 6;
const int scaleY = 6;
const int indent = 25; //отступ
const int GRAPHSIZE = 1200; //размеры окна
const int GRAPHWIDTH = 1000; //в логических единицах
//////////////////////////////////////////////////////////////////////


inline BOOL AreInLine(struct dot, struct dot, struct dot ); //алгоритм определения, лежат ли 3 точки на одной прямой,
//основывается на уравнении прямой по двум точкам. Находится разность правой и левой частей 
//полученное значение сравнивается с машинным нулем.  
//Аргументы - три структуры с координатами х и у. 

void DrawPoint(HDC, struct dot, double, double, dot *, int *, HPEN, HBRUSH, double, double, double, double, double, double,vector<string>); // аргументы: окно, где рисуется, структура с координатами точки, коэффициенты перевода
//координаты точки из файла на размер окна (2), указатель на массив, хранящий последнюю построенную точку для каждого графика, массив флагов,
//показывающих, построена ли хотя бы первая точка этого графика или нет
int ReadFile(TCHAR *, vector <dot>&, int&, vector<string>&); // путь к файлу, вектор для хранения точек, количество графиков
BOOL DrawGraphics(HWND hWnd, HDC hdc, vector<dot>, RECT, int, HPEN, HBRUSH, int, int, vector<string>); // окно, окно, вектор с точками, структура из оконной процедуры дочернего окна, число графиков
void ProceedNames(HDC, HPEN, HBRUSH,vector<string>, int);

int ReadFile(TCHAR *PathName, vector <dot>& points_vec, int& quant, vector<string>&names) {

	char str[20];
	double tmp_x = 0.0, tmp_y = 0.0;
	int first = 1;
	quant = 0;

	FILE *f_in = _tfopen((const TCHAR *)PathName, _TEXT("r"));
	if (f_in == NULL) {
		chMB("Невозможно открыть файл!");
		return 0;
	}

	fscanf(f_in, "%d", &quant); //quantity of graphics

	dot **dots = new dot*[quant];
	for (int i = 0; i != quant; i++) {
		dots[i] = new dot[3];
	}

	for (int i = 0; i != quant; i++) {
		fscanf(f_in, "%s", str);
		names.push_back(str);
	}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	while (!feof(f_in)) {
		if (first) {
			for (int i = 0; i != 2; i++) { //reading of first two points

				if (fscanf(f_in, "%s", str) != 0) {
					tmp_x = atof(str);
					for (int q = 0; q != quant; q++) {
						dots[q][i].x = tmp_x;
					}
				}
				else if (feof(f_in)) {
					chMB("End of file is reached!");
					for (int q = 0; q != quant; q++) delete[] dots[3]; //free the memory
					fclose(f_in);
					return quant;
				}
				else {
					chMB("some trouble. maybe file corrupted");
					for (int q = 0; q != quant; q++) delete[] dots[3]; //free the memory
					fclose(f_in);
					return(0);
				}
				for (int q = 0; q != quant; q++) {
					if (fscanf(f_in, "%s", str) != 0) {
						tmp_y = atof(str);
						dots[q][i].y = tmp_y;
						dots[q][i].num = q;
					}
					else if (feof(f_in)) {
						chMB("End of file is reached!");
						for (int q = 0; q != quant; q++) delete[] dots[3]; //free the memory
						fclose(f_in);
						return quant;
					}
					else {
						chMB("some trouble. maybe file corrupted");
						for (int q = 0; q != quant; q++) delete[] dots[3]; //free the memory
						fclose(f_in);
						return 0;
					}
				}
			}

			for (int q = 0; q != quant; q++) {
				points_vec.push_back(dots[q][0]);
			}
			first = 0;
		}
		if (!first) {

			if (fscanf(f_in, "%s", str) != 0) {
				tmp_x = atof(str);
			}
			else if (feof(f_in)) {
				chMB("End of file is reached!");
				for (int q = 0; q != quant; q++) delete[] dots[3]; //free the memory
				fclose(f_in);
				return quant;
			}
			else {
				chMB("some trouble. maybe file corrupted");
				for (int q = 0; q != quant; q++) delete[] dots[3]; //free the memory
				fclose(f_in);
				return 0;
			}
			for (int q = 0; q != quant; q++) {
				dots[q][2].x = tmp_x;
				if (fscanf(f_in, "%s", str) != 0) {
					tmp_y = atof(str);
					dots[q][2].y = tmp_y;
					dots[q][2].num = q;
				}
				else if (feof(f_in)) {
					chMB("End of file is reached!");
					for (int q = 0; q != quant; q++) delete[] dots[3]; //free the memory
					fclose(f_in);
					return quant;
				}
				else {
					chMB("some trouble. maybe file corrupted");
					for (int q = 0; q != quant; q++) delete[] dots[3]; //free the memory
					fclose(f_in);
					return 0;
				}
			}
			for (int q = 0; q != quant; q++) {
				if (AreInLine(dots[q][0], dots[q][1], dots[q][2])) {
					dots[q][1] = dots[q][2];
				}
				else {
					//write the non-linear dot to vector <dot> 
					points_vec.push_back(dots[q][1]);
					dots[q][0] = dots[q][1];
					dots[q][1] = dots[q][2];
				}
			}
		}
	}
}

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

BOOL DrawGraphics(HWND hWnd, HDC hdc, vector<dot> vec, RECT rect, int quantity, HPEN pen, HBRUSH brush, int sx, int sy, vector<string> names) {
	
	dot *points = new dot[quantity];
	int *flag = new int[quantity];
	for (int i = 0; i != quantity; i++) { //нужно для определения заполненности массива с последней точкой/ 0 - ни одной точки графика не нарисовано
		flag[i] = 0;
	}

	double max_x, min_x, max_y, min_y, hx, hy;
	double k_x = 1.0, k_y = 1.0;
	
	max_x = vec[0].x;
	max_y = vec[0].y;
	min_x = vec[0].x;
	min_y = vec[0].x;

	for (vector <dot> :: iterator i = vec.begin(); i != vec.end(); i++) {
		if ((*i).x > max_x) max_x = (*i).x;
		if ((*i).x < min_x) min_x = (*i).x;
		if ((*i).y > max_y) max_y = (*i).y;
		if ((*i).y < min_y) min_y = (*i).y;
	}
		
	hx = max_x - min_x;
	hy = max_y - min_y;

///////////////////draw the grid////////////////////////////////////////////
	SetMapMode(hdc, MM_ANISOTROPIC);
	SetWindowExtEx(hdc, GRAPHSIZE, -GRAPHSIZE, NULL);
	SetViewportExtEx(hdc, sx, sy, NULL);
	SetViewportOrgEx(hdc, 2 * indent, sy - indent, NULL);
	SetTextAlign(hdc, TA_RIGHT | TA_TOP);
	int x_gr, y_gr, i;
	TCHAR s[20];
	double grid_x, grid_y;
	for ( grid_x= min_x, i = 0; i <= scaleX; grid_x += hx / scaleX, i++) {
		x_gr = (int)((grid_x - min_x)*GRAPHWIDTH / (max_x - min_x) + 0.5);
		_stprintf(s, _TEXT("%.1lf"), grid_x);
		TextOut(hdc, x_gr, 0, s, _tcsclen(s));
		MoveToEx(hdc, x_gr, -10, NULL);
		LineTo(hdc, x_gr, 10);
	}
	MoveToEx(hdc, 0, 0, NULL);
	LineTo(hdc, GRAPHWIDTH, 0);
	SetTextAlign(hdc, TA_RIGHT | TA_BOTTOM);
	for (grid_y = min_y, i = 0; i <= scaleY; grid_y += hy / scaleY, i++) {
		y_gr = (int)((grid_y - min_y)*GRAPHWIDTH / (max_y - min_y) + 0.5);
		_stprintf(s, _TEXT("%.1lf"), grid_y);
		TextOut(hdc, 0, y_gr, s, _tcsclen(s));
		MoveToEx(hdc, -10, y_gr, NULL);
		LineTo(hdc, 10, y_gr);
	}
	MoveToEx(hdc, 0, 0, NULL);
	LineTo(hdc, 0, GRAPHWIDTH);

	for (vector <dot> ::iterator i = vec.begin(); i != vec.end(); i++) {
		DrawPoint(hdc, (*i), k_x, k_y, points, flag, pen, brush, max_x, min_x, max_y, min_y, hx, hy,names);
	}
	ProceedNames(hdc, pen, brush, names, quantity);
	delete[] flag;
	delete[] points;
	//delete[] lpnames;
	return (TRUE);
}

void DrawPoint(HDC hdc, struct dot point, double k_x, double k_y, dot *mas, int *flag, HPEN pen, HBRUSH brush, double max_x, double min_x, double max_y, double min_y,double hx, double hy, vector<string> names) {
	
	//рисуем точку
	//двигаем перо в неё, рисуем линию от нее до последней нарисованной точки этого графика цветом, зависящим от номера графика, если эта точка есть
	//пишем в массив эту точку
	//flag=0, если ни одной точки этого графика ещё не было нарисовано
	

///////////////////draw the rectangle for point/////////////////////////////
	pen = CreatePen(PS_DASHDOTDOT, 7, RGB(120, (11+40*point.num%243), 15));
	//brush = CreateSolidBrush(RGB(120, (11 + 40 * point.num % 243), 15));
	brush = CreateSolidBrush(RGB((170 * point.num % 235), (170 * point.num % 235), (170 * point.num % 235)));
	SelectObject(hdc, pen);
	SelectObject(hdc, brush);
	RECT r;
	SetRect(&r, (int)((point.x - min_x)*GRAPHWIDTH / hx + 0.5)-5, (int)((point.y - min_y)*GRAPHWIDTH / hy + 0.5)-5, (int)((point.x - min_x)*GRAPHWIDTH / hx + 0.5)+5, (int)((point.y - min_y)*GRAPHWIDTH / hy + 0.5) + 5);
	FillRect(hdc, &r, brush);
	SetPixel(hdc, (int)((point.x-min_x)*GRAPHWIDTH/hx+0.5),(int) ((point.y-min_y)*GRAPHWIDTH/hy+0.5), RGB((200 * point.num %235 +20), (200 * point.num %255 +20), (200 * point.num %235+20)));
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
	};

void ProceedNames(HDC hdc, HPEN pen, HBRUSH brush,vector<string> names, int quantity) {
	for (int i = 0; i != quantity; i++) {
		brush = CreateSolidBrush(RGB(200,10,11));
		SelectObject(hdc, brush);
		///////////////proceed the names////////////////////////////////////////////
		RECT namequad;
		SetRect(&namequad, i * 100 + 4, GRAPHWIDTH - 22, i * 100 + 28, GRAPHWIDTH - 48);
		FillRect(hdc, &namequad, brush);
		brush = CreateSolidBrush(RGB((170 * i % 235), (170 * i % 235), (170 * i % 235)));
		SelectObject(hdc, brush);
		SetRect(&namequad, i * 100 + 8, GRAPHWIDTH - 28, i * 100 + 22, GRAPHWIDTH - 42);
		FillRect(hdc, &namequad, brush);
		SetTextAlign(hdc, TA_LEFT | TA_TOP);
		
		wchar_t des[10];
		std::mbstowcs(des, names[i].c_str(), names[i].length());
		LPCWSTR lpcwstr = des;
		////////////////////////////////////////////////////////////////////////////
		TextOut(hdc, (int)i * 100 + 25, (int)(GRAPHWIDTH - 35), lpcwstr, names[i].length());
	}
}
