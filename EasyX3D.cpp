//改入口点
#pragma comment(linker, "/subsystem:windows /entry:mainCRTStartup")

#include <vector>
#include <iostream>
#include <cmath>
#include <windows.h>
#include <EasyX.h>
#include<string>
#include<time.h>
#define PI 3.1415926f
using namespace std;

// 你的原始窗口大小变量
int screen_w = 1700;
int screen_h = 1200;
// 全屏需要的额外变量
RECT oldRect;
bool firstRun = true;

struct CameraDelta {
	int dx;
	int dy;
};

static WNDPROC g_oldWheelProc = NULL;
static float g_wheelVal = 0.0f;
static HWND g_wheelWnd = NULL;
static DWORD g_lastWheelTime = GetTickCount();

LRESULT CALLBACK WheelWndProc(HWND h, UINT m, WPARAM w, LPARAM l)
{
	if (m == WM_MOUSEWHEEL)
	{
		// 滚轮方向：上+120 / 下-120
		short wheelDelta = (short)(w >> 16);

		// ==============================================
		// 核心：每一格 永远固定大小！速度完全不影响！
		// ==============================================
		const float FIXED_STEP = 1.0f;  // 每一格固定值，你可以随便改

		// 每一格 = 固定值
		g_wheelVal += (wheelDelta / 120.0f) * FIXED_STEP*5.0f;

		// 下面这些跟速度有关的代码 全部删掉！！！
		// 因为你要 每一格永远固定！

		return 0;
	}
	return CallWindowProc(g_oldWheelProc, h, m, w, l);
}
void InitWheel() {
	HWND hwnd = GetHWnd();
	if (hwnd == g_wheelWnd) return;

	// 重新绑定时，先恢复旧窗口过程
	if (g_wheelWnd && g_oldWheelProc) {
		SetWindowLongPtr(g_wheelWnd, GWLP_WNDPROC, (LONG_PTR)g_oldWheelProc);
	}

	g_wheelWnd = hwnd;
	g_oldWheelProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)WheelWndProc);
}
float GetWheelv()
{
	float val = g_wheelVal;
	g_wheelVal = 0;
	return val;
}
//隐藏鼠标
void SetInvisibleCursor()
{
	HWND hwnd = GetHWnd();

	// 创建一个 1x1 像素的完全透明位图
	BITMAPV5HEADER bmi = { 0 };
	bmi.bV5Size = sizeof(BITMAPV5HEADER);
	bmi.bV5Width = 1;
	bmi.bV5Height = 1;
	bmi.bV5Planes = 1;
	bmi.bV5BitCount = 32;
	bmi.bV5Compression = BI_BITFIELDS;
	bmi.bV5RedMask = 0x00FF0000;
	bmi.bV5GreenMask = 0x0000FF00;
	bmi.bV5BlueMask = 0x000000FF;
	bmi.bV5AlphaMask = 0xFF000000;

	void* pBits = NULL;
	HDC hdc = GetDC(NULL);
	HBITMAP hBitmap = CreateDIBSection(hdc, (BITMAPINFO*)&bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
	ReleaseDC(NULL, hdc);

	// 填充透明像素
	if (pBits)
	{
		DWORD* pPixel = (DWORD*)pBits;
		*pPixel = 0x00000000; // Alpha = 0, 完全透明
	}

	// 创建图标光标
	ICONINFO ii = { 0 };
	ii.fIcon = FALSE;
	ii.xHotspot = 0;
	ii.yHotspot = 0;
	ii.hbmMask = hBitmap;
	ii.hbmColor = hBitmap;

	HCURSOR hInvCur = CreateIconIndirect(&ii);

	// 应用到窗口
	SetClassLongPtr(hwnd, GCLP_HCURSOR, (LONG_PTR)hInvCur);
	SetCursor(hInvCur);
	ShowCursor(FALSE);

	// 清理资源
	DeleteObject(hBitmap);
}
// 全屏切换函数 F11
void ToggleFullScreen()
{
	static bool isFullScreen = false;
	static int saved_w, saved_h;
	static int saved_x, saved_y;
	static LONG savedStyle;

	if (!isFullScreen)
	{
		// 1. 保存原窗口信息
		HWND hwnd = GetHWnd();
		savedStyle = GetWindowLongPtr(hwnd, GWL_STYLE);
		GetWindowRect(hwnd, &oldRect);
		saved_x = oldRect.left;
		saved_y = oldRect.top;
		saved_w = screen_w;
		saved_h = screen_h;

		// 2. 获取屏幕真实分辨率
		int cx = GetSystemMetrics(SM_CXSCREEN);
		int cy = GetSystemMetrics(SM_CYSCREEN);

		// 3. 关旧窗口，开新全屏窗口
		closegraph();
		initgraph(cx, cy, 0);
		setbkcolor(RGB(0, 204, 204));

		//隐藏鼠标
		SetInvisibleCursor();

		// 4. 强制设置为无边框置顶全屏
		hwnd = GetHWnd();
		SetWindowLongPtr(hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
		SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, cx, cy, SWP_SHOWWINDOW);

		// 5. 更新变量
		screen_w = cx;
		screen_h = cy;
		InitWheel();
		isFullScreen = true;
	}
	else
	{
		// 1. 关全屏窗口
		closegraph();

		// 2. 恢复原窗口大小
		initgraph(saved_w, saved_h, 0);
		setbkcolor(RGB(0, 204, 204));

		//隐藏鼠标
		SetInvisibleCursor();

		// 3. 恢复窗口样式和位置
		HWND hwnd = GetHWnd();
		SetWindowLongPtr(hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
		SetWindowPos(hwnd, HWND_TOP, saved_x, saved_y, saved_w, saved_h, SWP_SHOWWINDOW);

		// 4. 恢复你的变量
		screen_w = saved_w;
		screen_h = saved_h;
		InitWheel();
		isFullScreen = false;
	}

	firstRun = true;
	Sleep(200);
}

CameraDelta Get3DCameraDelta(HWND hwnd)
{
	static POINT centerScreen = { 0, 0 };

	if (firstRun) {
		RECT rc;
		GetClientRect(hwnd, &rc);
		POINT centerClient = { (rc.left + rc.right) / 2, (rc.top + rc.bottom) / 2 };
		ClientToScreen(hwnd, &centerClient);
		centerScreen = centerClient;
		firstRun = false;
	}

	POINT current;
	GetCursorPos(&current);

	CameraDelta delta;
	delta.dx = current.x - centerScreen.x;
	delta.dy = current.y - centerScreen.y;

	SetCursorPos(centerScreen.x, centerScreen.y);

	return delta;
}

float deg_to_rad(float x)
{
	return PI * x / 180.0f;
}
struct point
{
	float x;
	float y;
	float z;
	float l;
};
struct point_2d
{
	int x;
	int y;
	int visible;
};
float pitch_deg = 0.0f;
float yaw_deg = 0.0f;
void camera_init(CameraDelta& delta, point& camera_world_position, point& camera_translate, point& camera_vision, point& camera_vision_z, point& camera_vision_x, point& camera_vision_y)
{
	const float k = 0.1f;//转动灵敏度

	//仰角
	pitch_deg -= delta.dy * k;
	if (pitch_deg >89.9f)pitch_deg = 89.9f;
	if (pitch_deg < -89.9f) pitch_deg = -89.9f;
	float pitch_rad = deg_to_rad(pitch_deg);

	//偏航角
	yaw_deg += delta.dx * k;
	if (yaw_deg >= 360.0f)yaw_deg -= 360;
	if (yaw_deg < 0.0f)yaw_deg += 360;
	float yaw_rad = deg_to_rad(yaw_deg);

	//计算相机向量
	camera_vision.x = cos(pitch_rad) * sin(yaw_rad);
	camera_vision.y = sin(pitch_rad);
	camera_vision.z = cos(pitch_rad) * cos(yaw_rad);
	camera_vision.l = 1;

	//求相机坐标系z轴
	camera_vision_z.x = -camera_vision.x;
	camera_vision_z.y = -camera_vision.y;
	camera_vision_z.z = -camera_vision.z;
	camera_vision_z.l = sqrt(camera_vision_z.x * camera_vision_z.x + camera_vision_z.y * camera_vision_z.y + camera_vision_z.z * camera_vision_z.z);


	//归一化相机坐标系z轴
	camera_vision_z.x /= camera_vision_z.l;
	camera_vision_z.y /= camera_vision_z.l;
	camera_vision_z.z /= camera_vision_z.l;

	//求相机坐标系x轴
	camera_vision_x.x = camera_vision.z;
	camera_vision_x.y = 0;
	camera_vision_x.z = -camera_vision.x;
	camera_vision_x.l = sqrt(camera_vision_x.x * camera_vision_x.x + camera_vision_x.y * camera_vision_x.y + camera_vision_x.z * camera_vision_x.z);

	//归一化相机坐标系x轴
	camera_vision_x.x /= camera_vision_x.l;
	camera_vision_x.y /= camera_vision_x.l;
	camera_vision_x.z /= camera_vision_x.l;

	//求相机坐标系y轴
	camera_vision_y.x = -camera_vision.x * camera_vision.y;
	camera_vision_y.y = camera_vision.x * camera_vision.x + camera_vision.z * camera_vision.z;
	camera_vision_y.z = -camera_vision.y * camera_vision.z;
	camera_vision_y.l = sqrt(camera_vision_y.x * camera_vision_y.x + camera_vision_y.y * camera_vision_y.y + camera_vision_y.z * camera_vision_y.z);

	//归一化相机坐标系y轴
	camera_vision_y.x /= camera_vision_y.l;
	camera_vision_y.y /= camera_vision_y.l;
	camera_vision_y.z /= camera_vision_y.l;

	// 键盘移动功能
	const float u = 0.1f;
	float v = u; //键盘灵敏度
	//按CTRL加速
	if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
	{
		v = (2.0f) * u;
	}
	else v = u;
	float fl = sqrt(camera_vision.x * camera_vision.x + camera_vision.z * camera_vision.z);
	float f_move_x = camera_vision.x / fl;
	float f_move_z = camera_vision.z / fl;
	float rl = sqrt(camera_vision_x.x * camera_vision_x.x + camera_vision_x.z * camera_vision_x.z);
	float r_move_x = camera_vision_x.x / rl;
	float r_move_z = camera_vision_x.z / rl;
	//W:进
	if (GetAsyncKeyState('W') & 0x8000)
	{
		camera_world_position.x += (f_move_x * v);
		camera_world_position.z += (f_move_z * v);
	}

	// A:左移
	if (GetAsyncKeyState('A') & 0x8000)
	{
		camera_world_position.x -= (r_move_x * v);
		camera_world_position.z -= (r_move_z * v);
	}

	// S:后退
	if (GetAsyncKeyState('S') & 0x8000)
	{
		camera_world_position.x -= (f_move_x * v);
		camera_world_position.z -= (f_move_z * v);
	}
	// D:右移
	if (GetAsyncKeyState('D') & 0x8000)
	{
		camera_world_position.x += (r_move_x * v);
		camera_world_position.z += (r_move_z * v);
	}
	// 空格:上升
	if (GetAsyncKeyState(VK_SPACE) & 0x8000)
	{
		camera_world_position.y += v;
	}

	// Shift:下降
	if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
	{
		camera_world_position.y -= v;
	}
	// ESC:退出
	if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
	{
		exit(0); // 关闭程序
	}
	//计算平移量
	camera_translate.x = camera_vision_x.x * camera_world_position.x + camera_vision_x.y * camera_world_position.y + camera_vision_x.z * camera_world_position.z;
	camera_translate.y = camera_vision_y.x * camera_world_position.x + camera_vision_y.y * camera_world_position.y + camera_vision_y.z * camera_world_position.z;
	camera_translate.z = camera_vision_z.x * camera_world_position.x + camera_vision_z.y * camera_world_position.y + camera_vision_z.z * camera_world_position.z;
}
float fov_deg = 60.0f;
bool turn(point& camera_world_position)
{
	static float prev_yaw = -361, prev_pitch = 91, x = -1, y = -1, z = -1,pre_fov=-1;
	if (prev_yaw != yaw_deg || prev_pitch != pitch_deg || x != camera_world_position.x || y != camera_world_position.y || z != camera_world_position.z||pre_fov!=fov_deg)
	{
		prev_yaw = yaw_deg, prev_pitch = pitch_deg, x = camera_world_position.x, y = camera_world_position.y, z = camera_world_position.z,pre_fov=fov_deg;
		return 1;
	}
	return 0;
}

// 世界转相机
void world_to_camera(vector<point>& world, vector<point>& world_camera, point& camera_translate, point& camera_vision_z, point& camera_vision_x, point& camera_vision_y)
{
	world_camera.clear();
	for (int i = 0; i < world.size(); i++)
	{
		world_camera.push_back({});
		point translate_p;
		translate_p.x = camera_vision_x.x * world[i].x + camera_vision_x.y * world[i].y + camera_vision_x.z * world[i].z;
		translate_p.y = camera_vision_y.x * world[i].x + camera_vision_y.y * world[i].y + camera_vision_y.z * world[i].z;
		translate_p.z = camera_vision_z.x * world[i].x + camera_vision_z.y * world[i].y + camera_vision_z.z * world[i].z;

		world_camera[i].x = translate_p.x - camera_translate.x;
		world_camera[i].y = translate_p.y - camera_translate.y;
		world_camera[i].z = translate_p.z - camera_translate.z;
	}
}
int scale = 25;
string text_num(string text, float a)
{
	settextstyle(scale, 0, "微软雅黑");
	string s = text + to_string(a);
	return s;
}
//相机转屏幕
void camera_to_screen(vector<point>& world_camera, vector<point_2d>& pixel, vector<point>& p, point& camera_world_position)
{
	float aspect = (float)screen_w / screen_h;
	float fov_rad = deg_to_rad(fov_deg);
	float d = 1.0f;
	float plane_y = tan(0.5 * fov_rad) * d;
	float plane_x = aspect * plane_y;
	pixel.clear();
	for (int i = 0; i < world_camera.size(); i++)
	{
		pixel.push_back({});
		pixel[i].visible = 1;
		const float r = 40.0;
		if (world_camera[i].z >= 0 || ((p[i].x - camera_world_position.x) * (p[i].x - camera_world_position.x) + (p[i].z - camera_world_position.z) * (p[i].z - camera_world_position.z)) >= r * r)
		{
			pixel[i].visible = -1;
			continue;
		}
		float nx = world_camera[i].x / (-world_camera[i].z);
		float ny = world_camera[i].y / (-world_camera[i].z);
		if (nx > plane_x || nx<-plane_x || ny>plane_y || ny < -plane_y)
		{
			pixel[i].visible = 0;
		}
		pixel[i].x = round((nx + plane_x) / (2 * plane_x) * screen_w);
		pixel[i].y = round((1 - (ny / plane_y)) * screen_h * 0.5);
	}

}

void draw(vector<point_2d>& pixel)
{
	for (int i = 1; i < pixel.size(); i++)
	{
		for (int j = 0; j < i; j++)
		{
			if (pixel[j].visible || pixel[i].visible)
			{
				if (pixel[j].visible == -1 || pixel[i].visible == -1)continue;
				line(pixel[j].x, pixel[j].y, pixel[i].x, pixel[i].y);
			}
		}
	}
}
void draw_grid(int& n, vector < vector<point_2d>>& pixel)
{
	for (int i = 0; i <= 2 * n; i++)
	{
		for (int j = 0; j <= 2 * n; j++)
		{
			if (i + 1 <= 2 * n)
			{
				if (pixel[i][j].visible || pixel[i + 1][j].visible)
				{
					if (pixel[i][j].visible == -1 || pixel[i + 1][j].visible == -1)
					{
					}
					else line(pixel[i][j].x, pixel[i][j].y, pixel[i + 1][j].x, pixel[i + 1][j].y);
				}
			}

			if (j + 1 <= 2 * n)
			{
				if (pixel[i][j].visible || pixel[i][j + 1].visible)
				{
					if (pixel[i][j].visible == -1 || pixel[i][j + 1].visible == -1)
					{
					}
					else line(pixel[i][j].x, pixel[i][j].y, pixel[i][j + 1].x, pixel[i][j + 1].y);;
				}
			}
		}
	}
}
//网格
void grid(int n, point& camera_world_position, point& camera_translate, point& camera_vision_z, point& camera_vision_x, point& camera_vision_y)
{
	static int pre_camerax = -1, pre_cameraz = -1;
	int camerax = round(camera_world_position.x);
	int cameraz = round(camera_world_position.z);
	static vector< vector<point>> grid_world;
	static vector < vector<point_2d>>grid_pixel;
	static vector<vector<point>>grid_camera;
	if (pre_camerax != camerax || pre_cameraz != cameraz)
	{
		grid_world.resize(2 * n + 1, vector<point>(2 * n + 1));
		grid_camera.resize(2 * n + 1, vector<point>(2 * n + 1));
		grid_pixel.resize(2 * n + 1, vector<point_2d>(2 * n + 1));
		pre_camerax = camerax, pre_cameraz = cameraz;
		for (int i = 0; i <= 2 * n; i++)
		{
			for (int j = 0; j <= 2 * n; j++)
			{

				grid_world[i][j].x = -n + j + camerax;
				grid_world[i][j].y = 0;
				grid_world[i][j].z = -n + i + cameraz;
			}
		}
	}
	for (int i = 0; i <= 2 * n; i++)
	{
		world_to_camera(grid_world[i], grid_camera[i], camera_translate, camera_vision_z, camera_vision_x, camera_vision_y);
	}
	for (int i = 0; i <= 2 * n; i++)
	{
		camera_to_screen(grid_camera[i], grid_pixel[i], grid_world[i], camera_world_position);
	}
	setlinecolor(WHITE);
	draw_grid(n, grid_pixel);
}
//物体1
void object(point& camera_world_position, point& camera_translate, point& camera_vision_z, point& camera_vision_x, point& camera_vision_y)
{
	static vector<point> point_world, point_camera;
	static vector<point_2d> point_pixel;
	const float m = 1;
	static bool sign = 1;
	if (sign)
	{
		point_world.push_back({ m, 0, 0 });
		point_world.push_back({ m, 0, m });
		point_world.push_back({ 0, 0, m });
		point_world.push_back({ 0, 0, 0 });
		point_world.push_back({ m, m, 0 });
		point_world.push_back({ m, m, m });
		point_world.push_back({ 0, m, m });
		point_world.push_back({ 0, m, 0 });
		sign = 0;
	}
	world_to_camera(point_world, point_camera, camera_translate, camera_vision_z, camera_vision_x, camera_vision_y);
	camera_to_screen(point_camera, point_pixel, point_world, camera_world_position);
	setlinecolor(RED);
	draw(point_pixel);
}
//物体2
void object2(point& camera_world_position, point& camera_translate, point& camera_vision_z, point& camera_vision_x, point& camera_vision_y)
{
	static vector<point> point_world, point_camera;
	static vector<point_2d> point_pixel;
	const float m = 10;
	static bool sign = 1;
	if (sign)
	{
		point_world.push_back({ 0, 0, 0 });
		point_world.push_back({ m, 0, 0 });
		point_world.push_back({ 0, 0, m });
		point_world.push_back({ 0, m, 0 });
		point_world.push_back({ m, m, m });
		sign = 0;
	}
	world_to_camera(point_world, point_camera, camera_translate, camera_vision_z, camera_vision_x, camera_vision_y);
	camera_to_screen(point_camera, point_pixel, point_world, camera_world_position);
	setlinecolor(GREEN);
	draw(point_pixel);
}
//绘制光标
void draw_sign()
{
	setlinecolor(BLACK);
	int cy = screen_h / 2 - 1;
	int cx = screen_w / 2 - 1;
	const int l = 10;
	line(cx, cy, cx + l, cy);
	line(cx, cy, cx, cy + l);
	line(cx, cy, cx - l, cy);
	line(cx, cy, cx, cy - l);
}
//绘制天空
void sky(point& camera_world_position, point& camera_translate, point& camera_vision, point& camera_vision_z, point& camera_vision_x, point& camera_vision_y)
{
	static vector <point> horizon_world, horizon_camera;
	static vector<point_2d> horizon_pixel;
	horizon_world.clear();
	horizon_world.push_back({});
	horizon_world[0] = camera_vision;
	horizon_world[0].x += camera_world_position.x;
	horizon_world[0].y = camera_world_position.y;
	horizon_world[0].z += camera_world_position.z;
	world_to_camera(horizon_world, horizon_camera, camera_translate, camera_vision_z, camera_vision_x, camera_vision_y);
	camera_to_screen(horizon_camera, horizon_pixel, horizon_world, camera_world_position);
	setlinecolor(RGB(0, 94, 255));
	int y = horizon_pixel[0].y;
	if (y > screen_h - 1)y = screen_h - 1;
	else if (y < 0)y = 0;
	for (int i = 0; i <y; i++)
	{
		float kg = 100.0f / y;
		float kr = sin(deg_to_rad(yaw_deg)) * 25 + 25;
		setlinecolor(RGB(kr, 100 + kg * i, 255));
		line(0, i, screen_w, i);
	}
	for (int i = y; i <= screen_h-1; i++)
	{

		float kb = 51.0f / (screen_h - 1 - y);
		float kg = 4.0f/ (screen_h - 1 -y);
		float kr = sin(deg_to_rad(yaw_deg)) * 25 + 25;
		setlinecolor(RGB(kr, 200 + kg * (i - y), 255 - kb* (i - y)));
		line(0, i, screen_w, i);
	}
}
int main()
{
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	initgraph(screen_w, screen_h, 0);
	HWND hwnd = GetHWnd();
	SetWindowPos(hwnd, NULL, 800, 100, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	InitWheel();
	setbkcolor(RGB(0, 204, 204));
	SetInvisibleCursor();
	const float m = 10.0;
	CameraDelta delta;
	point camera_translate, camera_vision, camera_vision_z, camera_vision_x, camera_vision_y;
	point camera_world_position = { 0,0,0,0 };
	Get3DCameraDelta(GetHWnd());
	cleardevice();
	clock_t now=0, last=0;
	float fps;
	while (1)
	{
		// F11 全屏切换
		if (GetAsyncKeyState(VK_F11) & 1)
		{
			ToggleFullScreen();
			Sleep(200);
		}
		{
			fov_deg -= (GetWheelv());
			if (fov_deg > 175.0f)fov_deg = 175.0f;
			if (fov_deg <5.0f)fov_deg = 5.0f;
		}
		delta = Get3DCameraDelta(GetHWnd());
		camera_init(delta, camera_world_position, camera_translate, camera_vision, camera_vision_z, camera_vision_x, camera_vision_y);
		if (!turn(camera_world_position))continue;
		BeginBatchDraw();
		cleardevice();
	setbkcolor(RGB(0, 204, 204));
sky(camera_world_position, camera_translate, camera_vision, camera_vision_z, camera_vision_x, camera_vision_y);
		grid(40, camera_world_position, camera_translate, camera_vision_z, camera_vision_x, camera_vision_y);
		object(camera_world_position, camera_translate, camera_vision_z, camera_vision_x, camera_vision_y);
		object2(camera_world_position, camera_translate, camera_vision_z, camera_vision_x, camera_vision_y);
		//绘制光标 
		draw_sign();
		//fov
		{
			string s = text_num("fov:", fov_deg);
			setbkmode(TRANSPARENT);
			outtextxy(0, screen_h-100, s.c_str());
		}
		//仰角
		{
			string s = text_num("pitch:", pitch_deg);
			setbkmode(TRANSPARENT);
			outtextxy(0, 0, s.c_str());
		}
		//偏角
		{
			string s = text_num("yaw:", yaw_deg);
			setbkmode(TRANSPARENT);
			outtextxy(0, 25, s.c_str());
		}
		//x
		{
			string s = text_num("x:", camera_world_position.x);
			int text_num_width = textwidth(s.c_str());
			setbkmode(TRANSPARENT);
			outtextxy(screen_w - text_num_width, 0, s.c_str());
		}
		// y
		{
			string s = text_num("y:", camera_world_position.y);
			int w = textwidth(s.c_str());
			setbkmode(TRANSPARENT);
			outtextxy(screen_w - w, scale, s.c_str());
		}
		// z
		{
			string s = text_num("z:", camera_world_position.z);
			int w = textwidth(s.c_str());
			setbkmode(TRANSPARENT);
			outtextxy(screen_w - w, scale * 2, s.c_str());
		}
		now = clock();
		float deltaTime = now - last;
		fps = 1000.0f / deltaTime;
		last = now;
		{
			string s = text_num("FPS:", fps);
			int w = textwidth(s.c_str());
			setbkmode(TRANSPARENT);
			outtextxy(screen_w - w, screen_h - 100, s.c_str());
		}
		EndBatchDraw();
	}
	return 0;
}