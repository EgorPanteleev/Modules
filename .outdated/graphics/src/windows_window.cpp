
#include "window.h"
#include "containers.h"

#include "GraphicsLibApi.h"

#include <windows.h>
#include <WinUser.h>
#include <uxtheme.h>
#include <vssym32.h>
#include <windowsx.h>
#include <shobjidl_core.h>

#include <GL/glew.h>
#include <GL/wglew.h>

#include <stdio.h>

#define EASYTAB_IMPLEMENTATION
#include "easytab.h"

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "glu32.lib")

bool window_init();

static tp::ModuleManifest* sModuleDependencies[] = {
	&tp::gModuleStringt, 
	&tp::gModuleMath, 
	&tp::gModuleContainer,
	NULL
};

tp::ModuleManifest tp::gModuleGlw = ModuleManifest("Glw", window_init, NULL, sModuleDependencies);

namespace tp {
	namespace glw {
		struct PlatformContext {
			HDC dc;
			HGLRC gl_handle;
			HWND window_handle;

			// proc
			UINT message;
			WPARAM w_param;
			LPARAM l_param;
			LRESULT res;
		};
	};
};

HWND windowsPlatformContextGetNativeWindow(tp::glw::PlatformContext* ctx) { return ctx->window_handle; }
UINT windowsPlatformContextGetMessage(tp::glw::PlatformContext* ctx) { return ctx->message; }
WPARAM windowsPlatformContextGetWparam(tp::glw::PlatformContext* ctx) { return ctx->w_param; }
LRESULT windowsPlatformContextGetLparam(tp::glw::PlatformContext* ctx) { return ctx->l_param; }

static LRESULT WindowCallback(HWND handle, UINT message, WPARAM w_param, LPARAM l_param);

static void Win32HandlePassCustomData(HWND handle, void* data) {
	LONG_PTR prev = SetWindowLongPtr(handle, GWLP_USERDATA, LONG_PTR(data));
}

static void SupportHighDpiScreens() {
	if (!SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
		OutputDebugStringA("WARNING: could not set DPI awareness");
	}
}

static void RegisterWindowClass() {

	SupportHighDpiScreens();

	WNDCLASSEXW window_class = { 0 };
	{
		window_class.cbSize = sizeof(window_class);
		window_class.lpszClassName = L"window_class_name";
		window_class.lpfnWndProc = WindowCallback;
		window_class.style = CS_HREDRAW | CS_VREDRAW;
		window_class.cbWndExtra = 0;
		window_class.hIcon = NULL;
		window_class.hCursor = LoadCursor(NULL, IDC_ARROW);
		window_class.hbrBackground = NULL;
		window_class.lpszMenuName = NULL;
	}
	RegisterClassExW(&window_class);
}

static void moveWindowToCurrentDesktop(HWND win32_handle) {
	static struct VdesktopHandler {

		IServiceProvider* pServiceProvider = NULL;
		IVirtualDesktopManager* pDesktopManager = NULL;
		const CLSID CLSID_ImmersiveShell = { 0xC2F03A33, 0x21F5, 0x47FA, 0xB4, 0xBB, 0x15, 0x63, 0x62, 0xA2, 0xF2, 0x39 };

		VdesktopHandler() {
			HRESULT hr = CoInitialize(NULL);
			RelAssert(SUCCEEDED(hr));
			hr = ::CoCreateInstance(CLSID_ImmersiveShell, NULL, CLSCTX_LOCAL_SERVER, __uuidof(IServiceProvider), (PVOID*)&pServiceProvider);
			RelAssert(SUCCEEDED(hr));
			hr = pServiceProvider->QueryService(__uuidof(IVirtualDesktopManager), &pDesktopManager);
			RelAssert(SUCCEEDED(hr));
		}

		~VdesktopHandler() {
			pDesktopManager->Release();
			pServiceProvider->Release();
			CoUninitialize();
		}
	} desktoph;

	BOOL bIsOnCurrentDesktop = FALSE;
	HRESULT hr = desktoph.pDesktopManager->IsWindowOnCurrentVirtualDesktop(win32_handle, &bIsOnCurrentDesktop);
	if (SUCCEEDED(hr)) {
		if (!bIsOnCurrentDesktop) {
			HWND dummyHWND = ::CreateWindowA("STATIC", "dummy", WS_VISIBLE, -100, -100, 10, 10, NULL, NULL, NULL, NULL);

			// sync-ish time?
			tp::sleep(230);

			GUID desktopId = { 0 };
			hr = desktoph.pDesktopManager->GetWindowDesktopId(dummyHWND, &desktopId);
			if (SUCCEEDED(hr)) {
				HRESULT ok = desktoph.pDesktopManager->MoveWindowToDesktop(win32_handle, desktopId);
				PostMessage(win32_handle, WM_ACTIVATE, WA_CLICKACTIVE, 0);
			}

			DestroyWindow(dummyHWND);
		}
	}
}

static bool win32_window_is_maximized(HWND handle) {
	WINDOWPLACEMENT placement = { 0 };
	placement.length = sizeof(WINDOWPLACEMENT);
	if (GetWindowPlacement(handle, &placement)) {
		return placement.showCmd == SW_SHOWMAXIMIZED;
	}
	return false;
}

static void create_redraw_event(HWND handle) {
	RECT size_rect;
	GetClientRect(handle, &size_rect);
	SetWindowPos(handle, NULL,
		size_rect.left, size_rect.top,
		size_rect.right - size_rect.left, size_rect.bottom - size_rect.top,
		SWP_FRAMECHANGED | SWP_DRAWFRAME
	);
}

//  --------------  wgl --------------------

void glerr(GLenum type) {
	DBG_BREAK(1);
}

#define AssertGL(x) { x; GLenum __gle = glGetError(); if (__gle != GL_NO_ERROR) glerr(__gle); }

static int wglChoosePixelFormat(tp::glw::PlatformContext* ctx) {

}

static void create_wgl_context(tp::glw::PlatformContext* ctx) {

	auto dummyHWND = ::CreateWindowA("STATIC", "dummy", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
	auto dummyHDC = GetDC(dummyHWND);

	PIXELFORMATDESCRIPTOR pfd;
	memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.dwFlags = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.cDepthBits = 32;
	pfd.iLayerType = PFD_MAIN_PLANE;

	int nPixelFormat = ChoosePixelFormat(dummyHDC, &pfd);
	SetPixelFormat(dummyHDC, nPixelFormat, &pfd);
	auto hrc = wglCreateContext(dummyHDC);
	wglMakeCurrent(dummyHDC, hrc);

	glewInit();

	if (!wglewIsSupported("WGL_ARB_create_context")) {
		DBG_BREAK(1);
	}

	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(hrc);
	ReleaseDC(dummyHWND, dummyHDC);
	DestroyWindow(dummyHWND);

	const int iPixelFormatAttribList[] = {
			WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
			WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
			WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
			WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
			WGL_COLOR_BITS_ARB, 32,
			WGL_DEPTH_BITS_ARB, 24,
			WGL_STENCIL_BITS_ARB, 8,
			WGL_SAMPLES_ARB, 4,
			0
	};
	int attributes[] = {
			WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
			WGL_CONTEXT_MINOR_VERSION_ARB, 2,
			WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
			0
	};

	nPixelFormat = 0;
	UINT iNumFormats = 0;

	wglChoosePixelFormatARB(ctx->dc, iPixelFormatAttribList, NULL, 1, &nPixelFormat, (UINT*)&iNumFormats);

	SetPixelFormat(ctx->dc, nPixelFormat, &pfd);

	ctx->gl_handle = wglCreateContextAttribsARB(ctx->dc, 0, attributes);

	wglMakeCurrent(NULL, NULL);
	wglMakeCurrent(ctx->dc, ctx->gl_handle);
}

static void terminate_wgl_context(HWND handle) {
}

static void set_wgl_current_context(HWND handle) {
}

//  --------------  creation --------------------

static void CreateWindowWin32(const tp::glw::Window::Appearence& aApperace, tp::glw::Window* window) {
	int flags = WS_THICKFRAME | WS_SYSMENU | ((aApperace.mHiden) ? 0 : WS_VISIBLE) | WS_MAXIMIZEBOX | WS_MINIMIZEBOX;

	auto handle = CreateWindowExW(
		WS_EX_APPWINDOW,
		L"window_class_name",
		L"No Title",
		flags,
		(int)aApperace.mPos.x, (int)aApperace.mPos.y,
		(int)aApperace.mSize.x, (int)aApperace.mSize.y,
		0, 0, 0, 0
	);

	Win32HandlePassCustomData(handle, window);

	window->mPlatformCtx->window_handle = handle;
	window->mPlatformCtx->dc = GetDC(handle);

	window->mDevice.mDPMM = GetDpiForWindow(window->mPlatformCtx->window_handle) / 25.4f;

	create_wgl_context(window->mPlatformCtx);
}

// ------------- window ------------------ 

tp::glw::Window::Device tp::glw::Window::mDevice;

void error_callback(int error, const char* description) {
	fprintf(stderr, "GLFW Error: %s\n", description);
}

bool window_init() {
	RegisterWindowClass();

	tp::glw::Window::mDevice.Size.assign(2000, 1000);
	tp::glw::Window::mDevice.FrameRate = 60;
	return true;
}

void tp::glw::Window::initialize() {
	mPlatformCtx = new PlatformContext();
	CreateWindowWin32(mAppearence, this);

	applyAppearance();

	create_redraw_event(mPlatformCtx->window_handle);

	EasyTab_Load(mPlatformCtx->window_handle);
}

tp::glw::Window::Window() {
	MODULE_SANITY_CHECK(gModuleContainer);

	mAppearence.mSize = mDevice.Size / 1.5f;
	mAppearence.mPos = (mDevice.Size - mAppearence.mSize) / 2.f;
	mAppearence.mSizeMin = { 100, 100 };
	mAppearence.mSizeMax = mDevice.Size;
	initialize();
}

tp::glw::Window::Window(const Appearence& aAppearence) {
	mAppearence = aAppearence;
	initialize();
}

void tp::glw::Window::applyAppearance() {
	auto handle = mPlatformCtx->window_handle;

	if (mAppearence.mAllDesktops) {
		moveWindowToCurrentDesktop(handle);
	}

	if (mCache.mAppearencePrev.mSize != mAppearence.mSize ||
			mCache.mAppearencePrev.mPos  != mAppearence.mPos ||
			mCache.mAppearencePrev.mTopZ != mAppearence.mTopZ) {

		RECT rec;
		RECT recc;

		GetWindowRect(mPlatformCtx->window_handle, &rec);
		GetClientRect(mPlatformCtx->window_handle, &recc);

		auto valx = (rec.right - rec.left) - recc.right;
		auto valy = (rec.bottom - rec.top) - recc.bottom;

		SetWindowPos(handle, 
			mAppearence.mTopZ ? HWND_TOPMOST : HWND_NOTOPMOST, // TOP Z
			(int)mAppearence.mPos.x, (int)mAppearence.mPos.y, // POS
			(int)mAppearence.mSize.x + valx, (int)mAppearence.mSize.y + valy, // SIZE
			mAppearence.mHiden ? SWP_HIDEWINDOW : SWP_SHOWWINDOW // HIDEN
		);

		mEvents.mRedraw = true;
	}

	if (mAppearence.mHiden != mCache.mAppearencePrev.mHiden) {
		mAppearence.mHiden ? ShowWindow(handle, SWP_SHOWWINDOW) : ShowWindow(handle, SWP_HIDEWINDOW);
	}

	if (mAppearence.mMaximized != mCache.mAppearencePrev.mMaximized) {
		mAppearence.mMaximized ? ShowWindow(handle, SW_SHOWMAXIMIZED) : ShowWindow(handle, SW_NORMAL);
	}

	if (mAppearence.mMinimized != mCache.mAppearencePrev.mMinimized) {
		if (mAppearence.mMinimized) ShowWindow(handle, SW_SHOWMINIMIZED);
		mAppearence.mMinimized = false;
	}
}

typedef LRESULT (WndProcHandler)(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, void* cd);

void tp::glw::Window::pollEvents() {
	mEvents.mKeysQueue.free();

	mEvents.mCursorPrev = mEvents.mCursor;

	applyAppearance();
	
	for (auto& cb : mNativeEventListeners) {
		cb.iter->val.exec_begin(mPlatformCtx, cb.iter->val.cd);
	}

	MSG msg = { 0 };
	while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessageW(&msg);

		for (auto& cb : mNativeEventListeners) {
			cb.iter->val.exec(mPlatformCtx, cb.iter->val.cd);
		}
	}

	for (auto& cb : mNativeEventListeners) {
		cb.iter->val.exec_end(mPlatformCtx, cb.iter->val.cd);
	}

	mCache.mAppearencePrev = mAppearence;

	if (mEvents.mKeysQueue.length)
		mEvents.mRedraw = 2;
}

void tp::glw::Window::beginDraw() {
	wglMakeCurrent(mPlatformCtx->dc, mPlatformCtx->gl_handle);
}

void tp::glw::Window::endDraw() {
	SwapBuffers(mPlatformCtx->dc);
	if (mEvents.mRedraw-- < 0) mEvents.mRedraw = 0;
}

#define WINRET(val) mPlatformCtx->res = val; return;
#define MAP_EV(SYSKEY, code) case SYSKEY: { mEvents.mKeysQueue.push(tp::KeyEvent(code, state)); break; }

void* tp::glw::Window::platform_window() const {
	return mPlatformCtx->window_handle;
}

void tp::glw::Window::platformCallback() {
	HWND handle = { (HWND__*)mPlatformCtx->window_handle };
	UINT message = { mPlatformCtx->message };
	WPARAM w_param = { mPlatformCtx->w_param };
	LPARAM l_param = { mPlatformCtx->l_param };

	switch (message) {
		// --------- main routines ---------------- //

		// resizing-start
	case WM_SIZING: {
		mCache.mUserResizing = true;
		break;
	}
								// resizing
	case WM_SIZE: {
		RECT rect;
		GetClientRect(handle, &rect);

		mAppearence.mSize.x = (tp::halnf)(rect.right - rect.left);
		mAppearence.mSize.y = (tp::halnf)(rect.bottom - rect.top);
		break;
	}
							// resizing-end
	case WM_CAPTURECHANGED: {
		mCache.mUserResizing = false;
		mEvents.mRedraw = true;
		break;
	}
												// redraw
	case WM_PAINT: {
		if (mCache.mUserResizing) {
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(handle, &ps);

			auto const col = mAppearence.mResizeColor;
			COLORREF bg_color = RGB(col.r * 255, col.g * 255, col.b * 255);
			COLORREF tex_color = RGB(210, 210, 210);
			HBRUSH bg_brush = CreateSolidBrush(bg_color);

			FillRect(hdc, &ps.rcPaint, bg_brush);

			DeleteObject(bg_brush);

			RECT rect;
			GetClientRect(handle, &rect);
			rect.top = LONG(rect.top + (rect.bottom - rect.top) / 2.f);
			SetTextColor(hdc, tex_color);
			SetBkMode(hdc, TRANSPARENT);
			DrawText(hdc, L"Resizing", -1, &rect, DT_CENTER | DT_VCENTER | DT_EXPANDTABS);

			EndPaint(handle, &ps);
			break;
		}
	}
							 // change area to be redraw
	case WM_NCCALCSIZE: {

		if (!w_param) {
			break;
		}

		UINT dpi = GetDpiForWindow(handle);

		RECT rec;
		GetWindowRect(mPlatformCtx->window_handle, &rec);

		int frame_x = GetSystemMetricsForDpi(SM_CXFRAME, dpi);
		int frame_y = GetSystemMetricsForDpi(SM_CYFRAME, dpi);
		int padding = GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);

		NCCALCSIZE_PARAMS* params = (NCCALCSIZE_PARAMS*)l_param;
		RECT* requested_client_rect = params->rgrc;

		requested_client_rect->right -= frame_x + padding;
		requested_client_rect->left += frame_x + padding;
		requested_client_rect->bottom -= frame_y + padding;

		if (win32_window_is_maximized(handle)) {
			requested_client_rect->top += padding;
		}

		WINRET(NULL);
	}
										// fix hittest
	case WM_NCHITTEST: {

		// Let the default procedure handle resizing areas
		LRESULT hit = DefWindowProc(handle, message, w_param, l_param);
		switch (hit) {
		case HTRIGHT:
		case HTLEFT:
		case HTTOPLEFT:
		case HTTOP:
		case HTTOPRIGHT:
		case HTBOTTOMRIGHT:
		case HTBOTTOM:
		case HTBOTTOMLEFT:
		case HTNOWHERE:
		{
			WINRET(hit);
		}
		}

		// minmaxbox test
		POINT point = { GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param) };
		MapWindowPoints(nullptr, handle, &point, 1);

		RECT rect;
		tp::rectf minmaxbox = mAppearence.mMinMaxBox;
		rect = { (long)minmaxbox.x, (long)minmaxbox.y, (long)(minmaxbox.x + minmaxbox.z), (long)(minmaxbox.y + minmaxbox.w) };
		if (PtInRect(&rect, point)) {
			mEvents.mCursor = mAppearence.mMinMaxBox.pos + (mAppearence.mMinMaxBox.size / 2);
			PostMessage(handle, WM_MOUSEMOVE, 0, MAKELONG(mEvents.mCursor.x, mEvents.mCursor.y));
			mCache.mMinMaxHover = true;
			WINRET(HTMAXBUTTON);
		}

		mCache.mMinMaxHover = false;

		// Looks like adjustment happening in NCCALCSIZE is messing with the detection
		// of the top hit area so manually fixing that.
		UINT dpi = GetDpiForWindow(handle);
		int frame_y = GetSystemMetricsForDpi(SM_CYFRAME, dpi);
		int padding = GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
		POINT cursor_point = { 0 };
		cursor_point.x = LOWORD(l_param);
		cursor_point.y = HIWORD(l_param);
		ScreenToClient(handle, &cursor_point);
		if (cursor_point.y > 0 && cursor_point.y < frame_y + padding) {
			WINRET(HTTOP);
		}

		// captions test
		bool caption = false;
		for (auto rec : mAppearence.mCaptionsAddArea) {
			caption |= rec->inside({ (tp::halnf)cursor_point.x, (tp::halnf)cursor_point.y });
		}
		for (auto rec : mAppearence.mCaptionsRemoveArea) {
			caption &= !rec->inside({ (tp::halnf)cursor_point.x, (tp::halnf)cursor_point.y });
		}

		if (caption) {
			WINRET(HTCAPTION);
		}

		WINRET(HTCLIENT);
	}
									 // fix max button click ?
	case WM_NCLBUTTONDOWN: {
		if (mCache.mMinMaxHover) {
			//PostMessage(handle, WM_LBUTTONDOWN, 0, 0);
			mAppearence.mMaximized = !mAppearence.mMaximized;
			mAppearence.mMaximized ? ShowWindow(handle, SW_SHOWMAXIMIZED) : ShowWindow(handle, SW_NORMAL);
			WINRET(NULL);
		}
		break;
	}
											 // blocking nativ header utils - not working
	case WM_SYSCOMMAND:
	{
		switch (w_param) {
		case 0xF012:
		case 0xF032:
		case 0xF022:
			break;
			WINRET(NULL);
		}

		if (w_param == SC_KEYMENU) {
			WINRET(NULL);
		}

		break;
	}
	// Mouse Move
	case WM_MOUSEMOVE: {

		mEvents.mCursorPrev = mEvents.mCursor;
		//mEvents.mPressurePrev = mEvents.mPressure;
		mEvents.mCursor.assign(GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param));

		 // We need to call TrackMouseEvent in order to receive WM_MOUSELEAVE events
		if (!mCache.mMouseTracked) {
			TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, mPlatformCtx->window_handle, 0 };
			::TrackMouseEvent(&tme);
			mCache.mMouseTracked = true;
		}

		mEvents.mRedraw += 1;
		break;
	}
	case WM_MOUSELEAVE: {
		POINT p;
		GetCursorPos(&p);
		ScreenToClient(mPlatformCtx->window_handle, &p);

		mEvents.mCursorPrev = mEvents.mCursor;
		//mEvents.mPressurePrev = mEvents.mPressure;

		mEvents.mCursor = { tp::halnf(p.x), tp::halnf(p.y) };

		mCache.mMouseTracked = false;
		mEvents.mRedraw += 1;
		break;
	}
	// --------- EVENTS ---------------- //

	// KEYBOARD
	case WM_KEYUP:
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	{
		mEvents.mRedraw = 2;

		auto state = (message == WM_KEYUP || message == WM_SYSKEYUP) ? tp::KeyEvent::EventState::RELEASED : tp::KeyEvent::EventState::PRESSED;

		//* VK_0 - VK_9 are the same as ASCII '0' - '9' (0x30 - 0x39)
		if ((w_param >= 0x41 && w_param <= 0x5A) || w_param >= 0x30 && w_param <= 0x39) {
			mEvents.mKeysQueue.push(tp::KeyEvent(tp::Keycode(w_param), state));
		}
		//* VK_A - VK_Z are the same as ASCII 'A' - 'Z' (0x41 - 0x5A)
		else if (w_param >= 0x30 && w_param <= 0x39) {
			mEvents.mKeysQueue.push(tp::KeyEvent(tp::Keycode(w_param), state));
		}
		else {
			switch (w_param) {
				MAP_EV(VK_SHIFT, tp::Keycode::LEFT_SHIFT);
				MAP_EV(VK_LEFT, tp::Keycode::LEFT);
				MAP_EV(VK_RIGHT, tp::Keycode::RIGHT);
				MAP_EV(VK_UP, tp::Keycode::UP);
				MAP_EV(VK_DOWN, tp::Keycode::DOWN);
				MAP_EV(VK_BACK, tp::Keycode::BACKSPACE);
				MAP_EV(VK_CONTROL, tp::Keycode::LEFT_CONTROL);
				MAP_EV(VK_MENU, tp::Keycode::LEFT_ALT);
				MAP_EV(VK_RETURN, tp::Keycode::ENTER);
				MAP_EV(VK_SPACE, tp::Keycode::SPACE);
				MAP_EV(VK_OEM_COMMA, tp::Keycode::COMMA);
				MAP_EV(VK_OEM_PERIOD, tp::Keycode::PERIOD);
				MAP_EV(VK_TAB, tp::Keycode::TAB);
				MAP_EV(VK_OEM_PLUS, tp::Keycode::EQUAL);
				MAP_EV(VK_OEM_MINUS, tp::Keycode::MINUS);
				MAP_EV(VK_OEM_3, tp::Keycode::TILDA);
				MAP_EV(VK_OEM_4, tp::Keycode::BRA);
				MAP_EV(VK_OEM_5, tp::Keycode::SLASH);
				MAP_EV(VK_OEM_6, tp::Keycode::KET);
				MAP_EV(VK_OEM_1, tp::Keycode::DOUBLE_DOT);
				MAP_EV(VK_OEM_7, tp::Keycode::QUOTES);
				MAP_EV(VK_OEM_2, tp::Keycode::INV_SLASH);
				MAP_EV(VK_ESCAPE, tp::Keycode::ESCAPE);
			}
		}
		WINRET(NULL);

	}

	// MOUSE1
	case WM_LBUTTONUP: {
		mEvents.mKeysQueue.push(tp::KeyEvent(tp::Keycode::MOUSE1, tp::KeyEvent::EventState::RELEASED));
		WINRET(NULL);
	}
	case WM_LBUTTONDOWN: {
		mEvents.mKeysQueue.push(tp::KeyEvent(tp::Keycode::MOUSE1, tp::KeyEvent::EventState::PRESSED));
		mEvents.mPressurePrev = mEvents.mPressure;
		mEvents.mPressure = 1.f;
		WINRET(NULL);
	}

										 // MOUSE2
	case WM_RBUTTONUP: {
		mEvents.mKeysQueue.push(tp::KeyEvent(tp::Keycode::MOUSE2, tp::KeyEvent::EventState::RELEASED));
		WINRET(NULL);
	}
	case WM_RBUTTONDOWN: {
		mEvents.mKeysQueue.push(tp::KeyEvent(tp::Keycode::MOUSE2, tp::KeyEvent::EventState::PRESSED));
		WINRET(NULL);
	}

										 // MOUSE3
	case WM_MBUTTONUP: {
		mEvents.mKeysQueue.push(tp::KeyEvent(tp::Keycode::MOUSE3, tp::KeyEvent::EventState::RELEASED));
		WINRET(NULL);
	}
	case WM_MBUTTONDOWN: {
		mEvents.mKeysQueue.push(tp::KeyEvent(tp::Keycode::MOUSE3, tp::KeyEvent::EventState::PRESSED));
		WINRET(NULL);
	}

										 // MOUSE WHEEL
	case WM_MOUSEWHEEL: {
		break;
	}

	case WM_MBUTTONDBLCLK:
	case WM_RBUTTONDBLCLK:
	case WM_LBUTTONDBLCLK: {
		WINRET(NULL);
	}
	}

	if (EasyTab_HandleEvent(handle, message, l_param, w_param) == EasyTabResult::EASYTAB_OK) {
		mEvents.mPressurePrev = mEvents.mPressure;
		mEvents.mPressure = EasyTab->Pressure;
	}

	WINRET(DefWindowProc(handle, message, w_param, l_param));
}

static LRESULT WindowCallback(HWND handle, UINT message, WPARAM w_param, LPARAM l_param) {
	tp::glw::Window* self = (tp::glw::Window*)GetWindowLongPtrW(handle, GWLP_USERDATA);

	if (self) {
		self->mPlatformCtx->message = message;
		self->mPlatformCtx->w_param = w_param;
		self->mPlatformCtx->l_param = l_param;

		self->platformCallback();
		return self->mPlatformCtx->res;
	}

	return DefWindowProc(handle, message, w_param, l_param);
}

tp::alni tp::glw::Window::sizeAllocatedMem() { return NULL; }
tp::alni tp::glw::Window::sizeUsedMem() { return NULL; }

tp::glw::Window::~Window() {

	EasyTab_Unload();

	// make the rendering context not current  
	wglMakeCurrent(NULL, NULL);

	// release the device context  
	ReleaseDC(mPlatformCtx->window_handle, mPlatformCtx->dc);

	// delete the rendering context  
	wglDeleteContext(mPlatformCtx->gl_handle);

	delete mPlatformCtx;
}