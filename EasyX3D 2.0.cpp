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
#define sw 1280
#define sh 720
using namespace std;

// 你的原始窗口大小变量
int screen_w = sw;
 int screen_h = sh;
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
	bool visible;
	float depth;
};
float M_vp[4][4];
float fov_deg = 60.0f;
float pitch_deg = 0.0f;
float yaw_deg = 0.0f;
const float n = 0.1f, f = 100.0f;
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
	float fov_rad = deg_to_rad(fov_deg);
	float plane_y = n * tan(0.5f * fov_rad);
	float aspect = (float)screen_w / screen_h;
	float plane_x = aspect * plane_y;
	float M_v[4][4] = { camera_vision_x.x, camera_vision_x.y, camera_vision_x.z,-camera_translate.x,
					   camera_vision_y.x, camera_vision_y.y, camera_vision_y.z,-camera_translate.y,
					   camera_vision_z.x, camera_vision_z.y, camera_vision_z.z,-camera_translate.z,
					   0 ,                0,                  0,                1 };
	float M_p[4][4] = { n * screen_w / (2 * plane_x),0,-screen_w / 2,                          0,
					   0,                     -(screen_h * n) / (2 * plane_y),-screen_h / 2 ,0,
					   0,                     0,n + f,                                    -n * f,
					   0,                     0, -1,                                           0, };
	for (int i = 0; i <= 3; i++)
	{
		for (int j = 0; j <= 3; j++)
		{
			M_vp[i][j] = 0;
			for (int k = 0; k <= 3; k++)
			{
				M_vp[i][j] += (M_p[i][k] * M_v[k][j]);
			}
		}
	}
}
bool turn(point& camera_world_position)
//看画面有没有变化
{
	static float prev_yaw = -361, prev_pitch = 91, x = -1, y = -1, z = -1,pre_fov=-1;
	if (prev_yaw != yaw_deg || prev_pitch != pitch_deg || x != camera_world_position.x || y != camera_world_position.y || z != camera_world_position.z||pre_fov!=fov_deg)
	{
		prev_yaw = yaw_deg, prev_pitch = pitch_deg, x = camera_world_position.x, y = camera_world_position.y, z = camera_world_position.z,pre_fov=fov_deg;
		return 1;
	}
	return 0;
}

int scale = 25;
string text_num(string text, float a)
{
	settextstyle(scale, 0, "微软雅黑");
	string s = text + to_string(a);
	return s;
}
void world_to_screen(point& point_world, point_2d& point_pixel)
{
	float M_point_world[4] = { point_world.x,point_world.y,point_world.z,1 };

	float M_point_screen[4];
	for (int i = 0; i <= 3; i++)
	{
		M_point_screen[i] = 0;
		for (int j = 0; j <= 3; j++)
		{
			M_point_screen[i] += (M_vp[i][j] * M_point_world[j]);
		}
	}
	point_pixel.visible = 1;
	point_pixel.x = M_point_screen[0] / M_point_screen[3];
	point_pixel.y = M_point_screen[1] / M_point_screen[3];
	point_pixel.depth = M_point_screen[2] / M_point_screen[3];
	if (M_point_screen[3] < n || M_point_screen[3] > f)point_pixel.visible = 0;
	/*if (point_pixel.x < 0)point_pixel.visible |= 1 << 1;
	else if (point_pixel.x > screen_w)point_pixel.visible |= 1 << 2;
	if (point_pixel.y < 0)point_pixel.visible |= 1 << 2;
	else if (point_pixel.y > screen_h)point_pixel.visible |= 1 << 4;*/



}
struct color
{
	int r;
	int g;
	int b;
};
struct screen
{
	color rgb;
	float depth;
};
screen screen_buffer[3000][3000];
struct triangle
{
	point point_world[3];
	color rgb;
};
float cross_2d(point_2d a, point_2d b)
{
	return a.x * b.y - a.y * b.x;
}
point_2d sub_2d(point_2d a, point_2d b)
{
	point_2d r;
	r.x = a.x - b.x;
	r.y = a.y - b.y;
	return r;
}

void draw_triangle(point_2d point_screen[3], color cl,float coso)
{
	color fog = { 255,255,255 };
	float alpha = 0, fogbegin = n, fogend = 20, fog_density = 0.6;
	float fov_rad = deg_to_rad(fov_deg);
	float plane_y = n * tan(0.5f * fov_rad);
	float aspect = (float)screen_w / screen_h;
	float plane_x = aspect * plane_y;
	int miny = screen_h, maxy = -1, minx = screen_w, maxx = -1;
	//扫描线算法
	for (int i = 0; i <= 2; i++)
	{
		miny = min(miny, point_screen[i].y);
		maxy = max(maxy, point_screen[i].y);
		minx = min(minx, point_screen[i].x);
		maxx = max(maxx, point_screen[i].x);
	}
	float Mx[2][2];
	{
		float a = point_screen[0].x - point_screen[2].x;
		float b = point_screen[1].x - point_screen[2].x;
		float c = point_screen[0].y - point_screen[2].y;
		float d = point_screen[1].y- point_screen[2].y;
		float D = a * d - b * c;

		if (fabs(D) < 1e-6) return;

		Mx[0][0] = d / D;
		Mx[0][1] = -b / D;
		Mx[1][0] = -c / D;
		Mx[1][1] = a / D;
	}
	float M_inv[4][4] = {
				2 * plane_x / (n * screen_w),  0,                        0,         -plane_x / n  ,
				0,                      -2 * plane_y / (n * screen_h),   0,          plane_y / n   ,
				 0,                       0,                        0,         -1           ,
				 0,                       0,                       -1.0f / (n * f), -(n + f) / (n * f)
	};
	float naddf = n + f,nf=n*f;
	miny = max(0, miny), maxy = min(screen_h - 1, maxy), minx = max(0, minx), maxx = min(screen_w - 1, maxx);
	for (int y= miny; y<=maxy; y++)
	{
		for (int x = minx; x <= maxx; x++)
		{
			float pixel_depth=1000;
			{
				float m, n, q;
				float dx = x - point_screen[2].x;
				float dy = y - point_screen[2].y;
				m = Mx[0][0] * dx + Mx[0][1] * dy;
				n= Mx[1][0] * dx + Mx[1][1] * dy;
				q = 1 - m - n;
				if (m < 0 || n < 0 || q < 0)continue;
				pixel_depth = m * point_screen[0].depth + n * point_screen[1].depth + q * point_screen[2].depth;
				if (pixel_depth < screen_buffer[y][x].depth)
				{
					float w = -(nf /( naddf +pixel_depth));

					float p_screen[4] = { w * x,w * y,w * pixel_depth,w };
	float p_camera[3];
					for (int i = 0; i <= 2; i++)
					{
						p_camera[i] = 0;
						for (int j = 0; j <= 3; j++)
						{
							p_camera[i] += (M_inv[i][j] * p_screen[j]);
						}
					}
					float l= sqrt(p_camera[0] * p_camera[0] + p_camera[1] * p_camera[1] + p_camera[2] * p_camera[2]);
					alpha =l / (fogend-fogbegin);
					if (alpha > 1)alpha = 1;
					if (alpha < 0)alpha = 0;
					alpha *= fog_density;
					float invalpha = 1 - alpha;
					color res = { round(fog.r * alpha + cl.r * invalpha),round(fog.g * alpha + cl.g * invalpha),round(fog.b * alpha + cl.b * invalpha) };
					screen_buffer[y][x].rgb = res;
					screen_buffer[y][x].depth = pixel_depth;
				}
			}
		}
	}
}
void triangle_to_screen(triangle triangle_world)
{
	point noraml_camera;
	// 单位法向量
	point p0 = triangle_world.point_world[0];
	point p1 = triangle_world.point_world[1];
	point p2 = triangle_world.point_world[2];

	// 边向量
	float e1x = p1.x - p0.x;
	float e1y = p1.y - p0.y;
	float e1z = p1.z - p0.z;

	float e2x = p2.x - p0.x;
	float e2y = p2.y - p0.y;
	float e2z = p2.z - p0.z;

	// 叉乘 → 法向量
	float nx = e1y * e2z - e1z * e2y;
	float ny = e1z * e2x - e1x * e2z;
	float nz = e1x * e2y - e1y * e2x;

	// 归一化 
	float len = sqrt(nx * nx + ny * ny + nz * nz);
	if (len < 1e-6f) len = 1.0f;
	nx /= len;
	ny /= len;
	nz /= len;
	//相机对物体的简单光照
	point l = { 1 / sqrt(3),1 / sqrt(3) ,1 / sqrt(3) };
	float coso = nx * l.x + ny*l.y +nz * l.z;
	coso = fabs(coso);
	color c;
	c.r = round(triangle_world.rgb.r * coso);
	c.g = round(triangle_world.rgb.g * coso);
	c.b = round(triangle_world.rgb.b * coso);
	if (c.r < 0)c.r = 0;
	if (c.g < 0)c.g = 0;
	if (c.b < 0)c.b = 0;
	point_2d point_screen[3];
	for (int i = 0; i <= 2; i++)world_to_screen(triangle_world.point_world[i], point_screen[i]);
	if (!point_screen[0].visible || !point_screen[1].visible|| !point_screen[2].visible)return;
	setlinecolor(BLACK);

	draw_triangle(point_screen, c,coso);
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
void sky(point& camera_world_position, point& camera_vision)
{
	static point horizon_world;
	static point_2d horizon_pixel;
	horizon_world = camera_vision;
	horizon_world.x += camera_world_position.x;
	horizon_world.y = camera_world_position.y;
	horizon_world.z += camera_world_position.z;
	world_to_screen(horizon_world, horizon_pixel);
	int y = horizon_pixel.y;
	horizon_pixel.depth = f;
	if (y > screen_h - 1)y = screen_h - 1;
	else if (y < 0)y = 0;
	float kr = sin(deg_to_rad(yaw_deg)) * 25 + 25;
	color skyfog = { 255,255,255 };
   float skyfog_density = 0.3;
	for (int i = 0; i <y; i++)
	{
		float kg = 100.0f / y;
		color c = { kr, 100 +kg * i, 255 };
		color res = { round(c.r * (1 - skyfog_density) + skyfog.r * skyfog_density),round(c.g * (1 - skyfog_density) + skyfog.g * skyfog_density),round(c.b * (1 - skyfog_density) + skyfog.b * skyfog_density) };
		for (int j = 0; j <= screen_w - 1; j++)
		{
			screen* p = &screen_buffer[i][j];
			p->rgb = res;
			p->depth = horizon_pixel.depth;
		}
	}
	for (int i = y; i <= screen_h-1; i++)
	{

		float kb = 51.0f / (screen_h - 1 - y);
		float kg = 4.0f/ (screen_h - 1 -y);
		color c={ kr, 200 + kg * (i - y), 255 - kb * (i - y) };
		color res = { round(c.r * (1 - skyfog_density) + skyfog.r * skyfog_density),round(c.g * (1 - skyfog_density) + skyfog.g * skyfog_density),round(c.b * (1 - skyfog_density) + skyfog.b* skyfog_density) };
		for (int j = 0; j <= screen_w - 1; j++)
		{
			screen* p = &screen_buffer[i][j];
			p->rgb = res;
			p->depth = horizon_pixel.depth;
		}
	}
}
void draw1()
{
	// 获取显存指针
	DWORD* pBuffer = GetImageBuffer();
	if (!pBuffer) return;
	int cnt=1;
	// 直接写显存，比 putpixel 快 50 倍
	for (int y = 0; y < screen_h; y++)
	{
		for (int x = 0; x < screen_w; x++)
		{
			color t;
			t.r = 0, t.g = 0, t.b = 0;
			int time=0;
			for (int i = -cnt; i <= cnt; i++)
			{
				for (int j = -cnt; j <= cnt; j++)
				{
					int tx = x + i;
					int ty = y + j;
					if (tx < screen_w && ty < screen_h&& tx >-1 && ty >-1)
					{
						time++;
						t.r += screen_buffer[ty][tx].rgb.r;
						t.g += screen_buffer[ty][tx].rgb.g;
						t.b += screen_buffer[ty][tx].rgb.b;
					}
				}
			}
			t.r = round((1.0f * t.r) / time);
			t.g= round((1.0f * t.g) / time);
			t.b = round((1.0f * t.b) / time);
			color& c = t;
			pBuffer[y * screen_w + x] = (c.r << 16) | (c.g << 8) | c.b;
		}
	}
}
void draw_edge()
{
	// 获取显存指针
	DWORD* pBuffer = GetImageBuffer();
	if (!pBuffer) return;
	int cnt = 1;
	// 直接写显存，比 putpixel 快 50 倍
	for (int y = 0; y < screen_h; y++)
	{
		for (int x = 0; x < screen_w; x++)
		{
			float t=0;
			int time = 0;
			for (int i = -cnt; i <= cnt; i++)
			{
				for (int j = -cnt; j <= cnt; j++)
				{
					int tx = x + i;
					int ty = y + j;
					if (tx < screen_w && ty < screen_h && tx >-1 && ty >-1)
					{
						time++;
						float dr=screen_buffer[ty][tx].rgb.r-screen_buffer[y][x].rgb.r;
						float dg = screen_buffer[ty][tx].rgb.g - screen_buffer[y][x].rgb.g;
						float db= screen_buffer[ty][tx].rgb.b - screen_buffer[y][x].rgb.b;
						float l = 255*sqrt(dr * dr + dg * dg + db * db) /sqrt(195075.0f);
						t +=l;
					}
				}
			}
			t= t/ time;
			color c;
				float alpha =t / 255;
				c.r =(1-alpha)* screen_buffer[y][x].rgb.r;
				c.g = (1 - alpha) * screen_buffer[y][x].rgb.g;
				c.b = (1 - alpha) * screen_buffer[y][x].rgb.b;
			pBuffer[y * screen_w + x] = (c.r << 16) | (c.g << 8) | c.b;
		}
	}

	

}
void clear_screen()
{
	
		// 遍历整个屏幕
		for (int y = 0; y < screen_h; y++)
		{
			for (int x = 0; x < screen_w; x++)
			{
				// 清空颜色：黑色（你也可以改成天空色）
				screen_buffer[y][x].rgb.r = 0;
				screen_buffer[y][x].rgb.g = 0;
				screen_buffer[y][x].rgb.b = 0;

				// 清空深度：设为最远距离（保证新像素能正常深度测试）
				screen_buffer[y][x].depth = f;
			}
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
	sky(camera_world_position,camera_vision);
triangle tri;
tri.point_world[0] = { 0,   0, 0 };
tri.point_world[1] = { 0, 10, 0 };
tri.point_world[2] = { 10,  0,0  };
tri.rgb = {250,250,250}; 
triangle tri2;
tri2.point_world[0] = { 10,   10, 10 };
tri2.point_world[1] = { 0, 10, 0 };
tri2.point_world[2] = { 0, 14, 0 };
tri2.rgb = { 250,0,250 };
triangle tri3;
tri3.point_world[0] = { 10,   10, 10 };
tri3.point_world[1] = { 10, 20, 0 };
tri3.point_world[2] = { 10,  0,-10 };
tri3.rgb = { 250,250,0 };
triangle_to_screen(tri);
triangle_to_screen(tri2);
triangle_to_screen(tri3);
draw1();
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