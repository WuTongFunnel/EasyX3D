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
#define Texturesize 2048
using namespace std;

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
vector<vector<screen>> screen_buffer;
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
		const float FIXED_STEP = 1.0f;  // 每一格固定值

		// 每一格 = 固定值
		g_wheelVal += (wheelDelta / 120.0f) * FIXED_STEP*5.0f;


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
		screen_buffer.clear();
		screen_buffer.resize(screen_h, std::vector<screen>(screen_w));
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
		screen_buffer.clear();
		screen_buffer.resize(screen_h, vector<screen>(screen_w));
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
	float u;
	float v;
};

struct pointl
{
	float x;
	float y;
	float z;
	float l;
};
struct point_2d
{
	float x;
	float y;
	int visible;
	float depth;
	float u;
	float v;
	float w;
};
float M_vp[4][4];
float fov_deg = 60.0f;
float pitch_deg = 0.0f;
float yaw_deg = 0.0f;
const float near_place = 0.01f, far_place = 100.0f;
void camera_init(CameraDelta& delta, point& camera_world_position, point& camera_translate, point& camera_vision, point& camera_vision_z, pointl& camera_vision_x, pointl& camera_vision_y)
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

	//求相机坐标系z轴
	camera_vision_z.x = -camera_vision.x;
	camera_vision_z.y = -camera_vision.y;
	camera_vision_z.z = -camera_vision.z;


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
	float plane_y = near_place * tan(0.5f * fov_rad);
	float aspect = (float)screen_w / screen_h;
	float plane_x = aspect * plane_y;
	float M_v[4][4] = { camera_vision_x.x, camera_vision_x.y, camera_vision_x.z,-camera_translate.x,
					   camera_vision_y.x, camera_vision_y.y, camera_vision_y.z,-camera_translate.y,
					   camera_vision_z.x, camera_vision_z.y, camera_vision_z.z,-camera_translate.z,
					   0 ,                0,                  0,                1 };
	float M_p[4][4] = { near_place * screen_w / (2 * plane_x),0,-screen_w / 2,                          0,
					   0,                     -(screen_h * near_place) / (2 * plane_y),-screen_h / 2 ,0,
					   0,                     0,-(near_place + far_place),                                    -near_place * far_place,
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
	point_pixel.x = M_point_screen[0];
	point_pixel.y = M_point_screen[1];
	point_pixel.depth = M_point_screen[2] ;
	point_pixel.w = M_point_screen[3];
	point_pixel.v = point_world.v;
	point_pixel.u = point_world.u;
	if (point_pixel.w < near_place )point_pixel.visible = -1;
	else if (point_pixel.w >far_place)point_pixel.visible = 0;
}
struct triangle
{
	point point_world[3];
	int texture_num;
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
int sky_y;
color sky_color[3000];
struct texture_struct
{
	color texture[Texturesize][Texturesize];
};
void draw_a_triangle(const point_2d (&a_triangle_screen)[3], color(&cl)[Texturesize][Texturesize])
{
	int miny = screen_h, maxy = -1, minx = screen_w, maxx = -1;
	//扫描线算法
	for (int i = 0; i <= 2; i++)
	{
		miny = min(miny, round(a_triangle_screen[i].y));
		maxy = max(maxy, round(a_triangle_screen[i].y));
		minx = min(minx, round(a_triangle_screen[i].x));
		maxx = max(maxx, round( a_triangle_screen[i].x));
	}
	float Mx[2][2];
	{
		float a = a_triangle_screen[0].x - a_triangle_screen[2].x;
		float b = a_triangle_screen[1].x - a_triangle_screen[2].x;
		float c = a_triangle_screen[0].y - a_triangle_screen[2].y;
		float d = a_triangle_screen[1].y- a_triangle_screen[2].y;
		float D = a * d - b * c;

		if (fabs(D) <1e-6) return;

		Mx[0][0] = d / D;
		Mx[0][1] = -b / D;
		Mx[1][0] = -c / D;
		Mx[1][1] = a / D;
	}
	miny = max(0, miny), maxy = min(screen_h - 1, maxy), minx = max(0, minx), maxx = min(screen_w - 1, maxx);
	for (int y= miny; y<=maxy; y++)
	{
		for (int x = minx; x <= maxx; x++)
		{
			
				float m, n, q;
				float dx = x+0.5 - a_triangle_screen[2].x;
				float dy = y+0.5 - a_triangle_screen[2].y;
				m = Mx[0][0] * dx + Mx[0][1] * dy;
				n= Mx[1][0] * dx + Mx[1][1] * dy;
				q = 1 - m - n;
				if (m < -(1e-6) || n < -(1e-6) || q < -(1e-6))continue;

				float pixel_depth = m * a_triangle_screen[0].depth + n * a_triangle_screen[1].depth + q * a_triangle_screen[2].depth;
				float pixel_w = near_place * far_place / (near_place + far_place -pixel_depth);
				float w_m = (pixel_w * m) / a_triangle_screen[0].w;
				float w_n = (pixel_w * n) / a_triangle_screen[1].w;
				float w_q = (pixel_w * q) / a_triangle_screen[2].w;
				float pixel_u =w_m* a_triangle_screen[0].u+ w_n * a_triangle_screen[1].u+ w_q * a_triangle_screen[2].u;
				float pixel_v = w_m * a_triangle_screen[0].v + w_n * a_triangle_screen[1].v + w_q * a_triangle_screen[2].v;
				color res;
				{
					// 新代码（稳定不崩）
					int texture_x = round(((Texturesize - 1) * pixel_u));
					int texture_y = round(((Texturesize - 1) * pixel_v));

					// 强制钳位 0~3，绝对不越界
					texture_x = max(0, min(Texturesize - 1, texture_x));
					texture_y = max(0, min(Texturesize - 1, texture_y));
				 res = cl[texture_y][texture_x];
				}
				if (pixel_depth <screen_buffer[y][x].depth )
				{
					screen_buffer[y][x].rgb = res;
					screen_buffer[y][x].depth = pixel_depth;
				}
			
		}
	}
}
void draw_triangle(const point_2d (&triangle_screen)[3], color(&cl)[Texturesize][Texturesize])
{
	point_2d a_triangle_screen[3];//构造合法三角形
	vector<point_2d>cut_point;//剔除掉在近裁面的点
	int cnt = 0;
	for (int i = 0; i <= 2; i++)
	{
		if (triangle_screen[i].visible == 0)//在远裁面后面
		{
			return;
		}
		else if (triangle_screen[i].visible == -1)//在近裁面前面
		{
			cut_point.push_back(triangle_screen[i]);
		}
		else
		{
			a_triangle_screen[cnt] = triangle_screen[i];
			cnt++;
		}
	}
	if (cnt == 3)
	{
		for (int i = 0; i <= 2; i++)
		{
			a_triangle_screen[i].y/= a_triangle_screen[i].w;
			a_triangle_screen[i].x /= a_triangle_screen[i].w;
			a_triangle_screen[i].depth /= a_triangle_screen[i].w;
		}
            		draw_a_triangle(a_triangle_screen, cl);
	}
	else if (!cnt)
	{
		return;
	}
    	else if (cnt == 1)
	{
	
		float w_a = a_triangle_screen[0].w;
		float x_a = a_triangle_screen[0].x;
		float y_a = a_triangle_screen[0].y;
		float u_a = a_triangle_screen[0].u;
		float v_a = a_triangle_screen[0].v;
			point_2d crossn;//求与近裁面的交点
			for (int j = 0; j <= 1; j++)
			{
               float w_b= cut_point[j].w;
			   float x_b = cut_point[j].x;
			   float y_b = cut_point[j].y;
			   float u_b = cut_point[j].u;
			   float v_b = cut_point[j].v;
			   float t = (w_a - near_place) / (w_a - w_b);
			   crossn.x = (1 - t) * x_a + t * x_b;
			   crossn.y =  (1 - t) * y_a + t  * y_b;
			   crossn.u = (1 - t)  * u_a +  t  * u_b;
			   crossn.v = (1 - t)* v_a + t * v_b;
				crossn.depth = (near_place+far_place)*near_place-near_place*far_place;
				crossn.w= near_place;

				a_triangle_screen[j+1] = crossn;
			}
			for (int i = 0; i <= 2; i++)
			{
				a_triangle_screen[i].y /= a_triangle_screen[i].w;
				a_triangle_screen[i].x /= a_triangle_screen[i].w;
				a_triangle_screen[i].depth /= a_triangle_screen[i].w;
			}
			draw_a_triangle(a_triangle_screen, cl);
	}
	else if(cnt==2)
	{
		point_2d crossn[2];//求与近裁面的两个交点，因为是四边形，需要作为两个三角形渲染
		for (int i = 0; i <= 1; i++)
		{
			float w_a = a_triangle_screen[i].w;
			float x_a = a_triangle_screen[i].x;
			float y_a = a_triangle_screen[i].y;
			float u_a = a_triangle_screen[i].u;
			float v_a = a_triangle_screen[i].v;
			float w_b = cut_point[0].w;
			float x_b = cut_point[0].x;
			float y_b = cut_point[0].y;
			float u_b = cut_point[0].u;
			float v_b = cut_point[0].v;
			float t = (w_a - near_place) / (w_a - w_b);
			crossn[i].x = (1 - t)  * x_a + t  * x_b;
			crossn[i].y = (1 - t)  * y_a + t* y_b;
			crossn[i].u = (1 - t) * u_a + t * u_b;
			crossn[i].v = (1 - t) * v_a + t * v_b;
			crossn[i].depth = (near_place + far_place) * near_place - near_place * far_place;
			crossn[i].w = near_place;
		}
		for (int i = 0; i <= 1; i++)
		{
			a_triangle_screen[i].y /= a_triangle_screen[i].w;
			a_triangle_screen[i].x /= a_triangle_screen[i].w;
			a_triangle_screen[i].depth /= a_triangle_screen[i].w;
		}
		for (int i = 0; i <= 1; i++)
		{
			crossn[i].y /= crossn[i].w;
			crossn[i].x /= crossn[i].w;
			crossn[i].depth /= crossn[i].w;
		}
		{
			// 三角2：左上 → 右下交点 → 右上  ✅ 健康三角
			point_2d a_new_triangle[3] = { a_triangle_screen[0], crossn[1] ,a_triangle_screen[1] };
			draw_a_triangle(a_new_triangle, cl);
		}
		{
			// 三角1：左上 → 右下交点 → 左下交点
			point_2d a_new_triangle[3] = { crossn[1], crossn[0], a_triangle_screen[0]};
			draw_a_triangle(a_new_triangle, cl);
		}
	}
}
void object_fog(int y,int x)
{
float 	pixel_depth = screen_buffer[y][x].depth;
if (pixel_depth > far_place)return;
color cl= screen_buffer[y][x].rgb;
color c = sky_color[y];
float alpha = 0, fogbegin =1, fogend = 30, fog_density =1;
color fog = { 255,255,255 };
	float fov_rad = deg_to_rad(fov_deg);
	float plane_y = near_place * tan(0.5f * fov_rad);
	float aspect = (float)screen_w / screen_h;
	float plane_x = aspect * plane_y;
	float w = (near_place*far_place / (near_place+far_place- pixel_depth));
	float M_inv[4][4] = {
		2 * plane_x / (near_place * screen_w),  0,                        0,         -plane_x / near_place  ,
		0,                      -2 * plane_y / (near_place * screen_h),   0,          plane_y / near_place   ,
		0,                       0,                        0,         -1           ,
		0,                       0,                       -1.0f / (near_place * far_place), (near_place + far_place) / (near_place * far_place)
	};
	float p_screen[4] = { w * (x+0.5f),w * (y-0.5f),w * pixel_depth,w };
	float p_camera[3];
	for (int i = 0; i <= 2; i++)
	{
		p_camera[i] = 0;
		for (int j = 0; j <= 3; j++)
		{
			p_camera[i] += (M_inv[i][j] * p_screen[j]);
		}
	}
	float l = sqrt(p_camera[0] * p_camera[0] + p_camera[1] * p_camera[1] + p_camera[2] * p_camera[2]);
	alpha = (l - fogbegin) / (fogend - fogbegin);
	bool overend = 0;
	if (alpha > 1)
	{
		alpha = 1;
		overend = 1;
	}
	else if (alpha < 0)alpha = 0;
	alpha *= fog_density;
	color res = {
				(int)round(fog.r * alpha + cl.r * (1 - alpha)* (1 - alpha)*(1 - alpha)),
				(int)round(fog.g * alpha + cl.g * (1 - alpha) * (1 - alpha) * (1 - alpha)),
				(int)round(fog.b * alpha + cl.b * (1 - alpha)* (1 - alpha)*(1 - alpha))
	};

	if (overend)
	{
		float kr = sin(deg_to_rad(yaw_deg)) * 25 + 25;
		float k = (l - fogend) / 20;
		if (k > 1) k = 1;
		else if (k < 0) k = 0;
		res.r = (int)(res.r * (1 - k) * (1 - k) * (1 - k) + k * c.r);
		res.g = (int)(res.g * (1 - k) * (1 - k) * (1 - k) + k * c.g);
		res.b = (int)(res.b * (1 - k) * (1 - k) * (1 - k) + k * c.b);
	}
	screen_buffer[y][x].rgb = res;
}
void a_triangle_to_screen(triangle &triangle_world, color (&cl)[Texturesize][Texturesize])
{
	point_2d point_screen[3];
	for (int i = 0; i <= 2; i++)world_to_screen(triangle_world.point_world[i], point_screen[i]);
	draw_triangle(point_screen, cl);
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
	int y = horizon_pixel.y/ horizon_pixel.w;
	horizon_pixel.depth = far_place+1;
	if (y > screen_h - 1)y = screen_h - 1;
	else if (y < 0)y = 0;
	float kr = sin(deg_to_rad(yaw_deg)) * 25 + 25;
	color skyfog = { 255,255,255 };
   float skyfog_density = 0.0;
   sky_y = y;
	for (int i = 0; i <y; i++)
	{
		float kg = 100.0f / y;
		color c = { kr, 100 +kg * i, 255 };
		sky_color[i] = c;
		color res = { round(c.r * (1 - skyfog_density) * (1 - skyfog_density) * (1 - skyfog_density) + skyfog.r * skyfog_density),round(c.g * (1 - skyfog_density) * (1 - skyfog_density) * (1 - skyfog_density) + skyfog.g * skyfog_density),round(c.b * (1 - skyfog_density) * (1 - skyfog_density) * (1 - skyfog_density) + skyfog.b * skyfog_density) };
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
		sky_color[i] = c;
		color res = { round(c.r * (1 - skyfog_density)* (1 - skyfog_density)* (1 - skyfog_density) + skyfog.r * skyfog_density),round(c.g * (1 - skyfog_density) *(1 - skyfog_density)* (1 - skyfog_density) + skyfog.g * skyfog_density),round(c.b * (1 - skyfog_density)* (1 - skyfog_density)* (1 - skyfog_density) + skyfog.b* skyfog_density) };
		for (int j = 0; j <= screen_w - 1; j++)
		{
			screen* p = &screen_buffer[i][j];
			p->rgb = res;
			p->depth = horizon_pixel.depth;
		}
	}
}
void sun(point& camera_world_position)
{
 point sun_world;
	 point_2d sun_pixel;
	sun_world = {2,2,2};
	sun_world.x += camera_world_position.x;
	sun_world.y += camera_world_position.y;
	sun_world.z += camera_world_position.z;
	world_to_screen(sun_world, sun_pixel);
	sun_pixel.x /= sun_pixel.w;
	sun_pixel.y /= sun_pixel.w;
	if (sun_pixel.visible==-1 )return;
	int cnt = 25;
	for (int i = -cnt; i <= cnt; i++)
	{
		for (int j = -cnt; j <= cnt; j++)
		{
			int nx = sun_pixel.x + i;
			int ny = sun_pixel.y + j;
			if (nx > -1 && ny > -1&&nx<screen_w&&ny<screen_h)screen_buffer[ny][nx].rgb = { 255,255,255 };
		}
	}
}
void draw()
{
	// 获取显存指针
	DWORD* pBuffer = GetImageBuffer();
	if (!pBuffer) return;
	for (int y = 0; y < screen_h; y++)for (int x = 0; x < screen_w; x++)object_fog(y, x);
	for(int y=0;y<screen_h;y++)
	{
		for (int x = 0; x<screen_w; x++)
		{
			color& c = screen_buffer[y][x].rgb;
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
	for (int y = 0; y < screen_h; y++)for (int x = 0; x < screen_w; x++)object_fog(y, x);
	for (int y = 0; y < screen_h; y++)
	{
		for (int x = 0; x < screen_w; x++)
		{
			float dt = 0;
			if (screen_buffer[y][x].depth < far_place + 1)
			{
				for (int i = -cnt; i <= cnt; i++)
				{
					for (int j = -cnt; j <= cnt; j++)
					{
						int tx = x + i;
						int ty = y + j;
						if (tx < screen_w && ty < screen_h && tx >-1 && ty >-1)
						{
							float dr = screen_buffer[ty][tx].rgb.r - screen_buffer[y][x].rgb.r;
							float dg = screen_buffer[ty][tx].rgb.g - screen_buffer[y][x].rgb.g;
							float db = screen_buffer[ty][tx].rgb.b - screen_buffer[y][x].rgb.b;
							float l =abs(dr) + abs(dg) + abs(db);
							dt += l;
						}
					}
				}
			}
			color c;
			if (dt >256)
			{
				c.r = 0;
				c.g = 0;
				c.b = 0;
			}
			else c = screen_buffer[y][x].rgb;
			pBuffer[y * screen_w + x] = (c.r << 16) | (c.g << 8) | c.b;
		}
	}

}
texture_struct texture_object[3];
vector<triangle>render_triangle;
void render()
{
	for (int i = 0; i < render_triangle.size(); i++)
	{
		a_triangle_to_screen(render_triangle[i], texture_object[render_triangle[i].texture_num].texture);
	}
}
class block
{
private:
	struct face_struct
	{
		point face[4];
		int num;
	};
public:
	void cube_face(face_struct face)
	{
		// 给面的4个顶点设置UV
		face.face[0].u = 0; face.face[0].v = 0;
		face.face[1].u = 1; face.face[1].v = 0;
		face.face[2].u = 1; face.face[2].v = 1;
		face.face[3].u = 0; face.face[3].v = 1;

		// 生成两个三角形
		render_triangle.push_back({ face.face[0], face.face[1], face.face[2],face.num });
		render_triangle.push_back({ face.face[0], face.face[2], face.face[3],face.num });
	}
	void draw_cube(point t, float m)
	{
		point p[8];
		p[0] = { t.x,     t.y,     t.z };
		p[1] = { t.x + m, t.y,     t.z };
		p[2] = { t.x + m, t.y - m, t.z };
		p[3] = { t.x,     t.y - m, t.z };

		p[4] = { t.x,     t.y,     t.z + m };
		p[5] = { t.x + m, t.y,     t.z + m };
		p[6] = { t.x + m, t.y - m, t.z + m };
		p[7] = { t.x,     t.y - m, t.z + m };
		// 正面
		cube_face({ { p[0], p[1], p[2], p[3] } ,1});

		// 背面
		cube_face({ { p[4], p[5], p[6], p[7] },1 });

		// 左面
		cube_face({ { p[0], p[4], p[7], p[3] } ,1});

		// 右面
		cube_face({ { p[1], p[5], p[6], p[2] } ,1});

		// 上面
		cube_face({ { p[0], p[1], p[5], p[4] },0 });

		// 下面
		cube_face({ { p[3], p[2], p[6], p[7] },2 });
	}
}cube;
void init_object()
{
	for (int i = 1; i <= 5; i++)
	{
		for (int j = 1; j <= 5; j++)cube.draw_cube({ 1.0f*i,0,1.0f * j }, 1);
	}
}
void render_draw()
{
	render();
	draw();
}
void load_texture(color texture[Texturesize][Texturesize],string s,float kr=1.0f, float kg = 1.0f, float kb = 1.0f)
{
	IMAGE img;
	if (loadimage(&img, s.c_str()))
	{
		for (int y = 0; y <= (Texturesize- 1)/2; y++)
		{
			
				for (int x = 0; x <= Texturesize - 1 ;x++)
				{
					if (x < (Texturesize - 1) / 2)
					{
						texture[y][x] = { 0,0,0 };
					}
					else texture[y][x] = { 255,255,255 };
				}
		}
		for (int y = (Texturesize - 1) / 2; y <= (Texturesize - 1); y++)
		{

			for (int x = 0; x <= Texturesize - 1; x++)
			{
				if (x < (Texturesize - 1) / 2)
				{
					texture[y][x] = { 255,255,255 };
				}
				else texture[y][x] = { 0,0,0 };
			}
		}
		return;
	}
	int img_w = img.getwidth();
	int img_h= img.getheight();
	float sample_lenth = min(img_w, img_h);
	float sampling_size= (sample_lenth-1)/(Texturesize-1);
	DWORD* buf = GetImageBuffer(&img);
	float y = 0;
	for (int y_step = 0; y_step <= Texturesize - 1; y_step++)
	{
		float x = 0;
		for (int x_step=0; x_step <= Texturesize - 1; x_step++)
		{
			DWORD color = buf[(int)round(y) * img_w + (int)round(x)];
			int r = GetBValue(color)*kr;
			int g = GetGValue(color)*kg;
			int b = GetRValue(color)*kb;
			texture[y_step][x_step] = {r,g,b };
			x += sampling_size;
		}
		y += sampling_size;
	}
}
int main()
{
	load_texture(texture_object[2].texture, "bottom.png");
	load_texture(texture_object[0].texture, "top.png");
	load_texture(texture_object[1].texture, "side.png");
	screen_buffer.resize(screen_h, std::vector<screen>(screen_w));
	srand(time(NULL));
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	initgraph(screen_w, screen_h, 0);
	HWND hwnd = GetHWnd();
	SetWindowPos(hwnd, NULL, 800, 100, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	InitWheel();
	setbkcolor(RGB(0, 204, 204));
	SetInvisibleCursor();
	const float m = 10.0;
	CameraDelta delta;
	point camera_translate, camera_vision, camera_vision_z;
		pointl camera_vision_x, camera_vision_y;
	point camera_world_position = { 0,0,0 };
	Get3DCameraDelta(GetHWnd());
	cleardevice();
	clock_t now=0, last=0;
	float fps;
	init_object();
	while (1)
	{
		if (GetAsyncKeyState('P') & 0x8000)
		{
			while (1)
			{
				Sleep(1);
				if (GetAsyncKeyState('P') & 0x8000)
				{
					break;
				}
			}
		}
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
	sun(camera_world_position);
	   render_draw();
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