//----------------------------------------------------------------------------------
//		時間ジャンププラグイン
//----------------------------------------------------------------------------------
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include "filter.h"
//#define DEBUG
//#include "dbgtrace.h"


#ifndef lengthof
#define lengthof(arr)	(sizeof(arr) / sizeof((arr)[0]))
#endif


#define BUTTON_MARGIN	6
#define BUTTON_W		60
#define BUTTON_H		18

#define IDC_BCK_1SEC	101
#define IDC_FWD_1SEC	102
#define IDC_BCK_5SEC	103
#define IDC_FWD_5SEC	104
#define IDC_BCK_10SEC	105
#define IDC_FWD_10SEC	106
#define IDC_BCK_15SEC	107
#define IDC_FWD_15SEC	108
#define IDC_BCK_30SEC	109
#define IDC_FWD_30SEC	110
#define IDC_BCK_1MIN	111
#define IDC_FWD_1MIN	112
#define IDC_BCK_1_5MIN	113
#define IDC_FWD_1_5MIN	114
#define IDC_BCK_2MIN	115
#define IDC_FWD_2MIN	116
#define IDC_BCK_3MIN	117
#define IDC_FWD_3MIN	118
#define IDC_BCK_5MIN	119
#define IDC_FWD_5MIN	120
#define IDC_BCK_10MIN	121
#define IDC_FWD_10MIN	122
#define IDC_BCK_15MIN	123
#define IDC_FWD_15MIN	124
#define IDC_EDIT		131
#define IDC_JUMP		132

struct {
	char *title;
	int id;
} buttons[] = {
	{"-0:01", IDC_BCK_1SEC},
	{"+0:01", IDC_FWD_1SEC},
	{"-0:05", IDC_BCK_5SEC},
	{"+0:05", IDC_FWD_5SEC},
	{"-0:10", IDC_BCK_10SEC},
	{"+0:10", IDC_FWD_10SEC},
	{"-0:15", IDC_BCK_15SEC},
	{"+0:15", IDC_FWD_15SEC},
	{"-0:30", IDC_BCK_30SEC},
	{"+0:30", IDC_FWD_30SEC},
	{"-1:00", IDC_BCK_1MIN},
	{"+1:00", IDC_FWD_1MIN},
	{"-1:30", IDC_BCK_1_5MIN},
	{"+1:30", IDC_FWD_1_5MIN},
};

#define WINDOW_W	((BUTTON_W + BUTTON_MARGIN) * 2 + BUTTON_MARGIN)
#define WINDOW_H	((BUTTON_H + BUTTON_MARGIN) * (lengthof(buttons) / 2 + 2) + BUTTON_MARGIN + BUTTON_MARGIN/2)



#if 1
//---------------------------------------------------------------------
//		DllMain
//---------------------------------------------------------------------
extern "C" BOOL WINAPI _DllMainCRTStartup(HANDLE hDllHandle, DWORD dwReason, LPVOID lpreserved)
{
	return TRUE;
}
#endif


//---------------------------------------------------------------------
//		フィルタ構造体定義
//---------------------------------------------------------------------
#define	TRACK_N	0		//	トラックバーの数

#define	CHECK_N	1		//	チェックボックスの数
static TCHAR	*check_name[] =		{	"Sync JmpWnd"	};
static int		check_default[] =	{	1			};


FILTER_DLL filter = {
	FILTER_FLAG_ALWAYS_ACTIVE | FILTER_FLAG_MAIN_MESSAGE | FILTER_FLAG_WINDOW_SIZE
		| FILTER_FLAG_DISP_FILTER | FILTER_FLAG_EX_INFORMATION,
	WINDOW_W | FILTER_WINDOW_SIZE_CLIENT, WINDOW_H | FILTER_WINDOW_SIZE_CLIENT,
	"時間ジャンプ",
	TRACK_N, NULL, NULL,
	NULL, NULL,
	CHECK_N, check_name, check_default,
	NULL,
	/*func_init*/ NULL,
	/*func_exit*/ NULL,
	NULL,
	func_WndProc,
	NULL, NULL,
	NULL,
	NULL,
	"時間ジャンプ version 0.05 by K.Takata",
	NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL,
};


//---------------------------------------------------------------------
//		フィルタ構造体のポインタを渡す関数
//---------------------------------------------------------------------
EXTERN_C FILTER_DLL __declspec(dllexport) * __stdcall GetFilterTable(void)
{
	RECT rc = {0, 0, WINDOW_W, WINDOW_H};
	AdjustWindowRectEx(&rc, WS_CAPTION | WS_OVERLAPPED | WS_SYSMENU, FALSE,
			WS_EX_TOOLWINDOW | WS_EX_WINDOWEDGE);
	filter.x = rc.right - rc.left;
	filter.y = rc.bottom - rc.top;
	return &filter;
}


//---------------------------------------------------------------------
//		初期化
//---------------------------------------------------------------------
BOOL func_init(FILTER *fp)
{
	return TRUE;
}


//---------------------------------------------------------------------
//		終了
//---------------------------------------------------------------------
BOOL func_exit(FILTER *fp)
{
	return TRUE;
}



BOOL CALLBACK EnumThreadWndProc(HWND hwnd, LPARAM lParam);
//---------------------------------------------------------------------
//		ジャンプウィンドウ検索
//---------------------------------------------------------------------
HWND FindJumpWindow(void)
{
	HWND hwndJump = NULL;
	
	EnumThreadWindows(GetCurrentThreadId(), EnumThreadWndProc, (LPARAM) &hwndJump);
	return hwndJump;
}


#define JUMPWINDOWNAME	"ジャンプウィンドウ"
#define JUMPWINDOWCLASS	"AviUtl"

BOOL CALLBACK EnumThreadWndProc(HWND hwnd, LPARAM lParam)
{
	char buf[80];
	
	GetWindowText(hwnd, buf, lengthof(buf));
	buf[lengthof(JUMPWINDOWNAME)-1] = '\0';
	if (lstrcmp(buf, JUMPWINDOWNAME) != 0) {
		return TRUE;
	}
	GetClassName(hwnd, buf, lengthof(buf));
	if (lstrcmp(buf, JUMPWINDOWCLASS) != 0) {
		return TRUE;
	}
	*(HWND *) lParam = hwnd;
	return FALSE;
}


//---------------------------------------------------------------------
//		時間ジャンプ
//---------------------------------------------------------------------
#define IDC_JUMPWINDOW_MAINWNDFRAME	40043
BOOL timejump(void *editp, FILTER *fp, int sec, bool abs = false)
{
	FILE_INFO fi;
	int new_frame, cur_frame, frame_n;
	HWND hwndJump;
	
	fp->exfunc->get_file_info(editp, &fi);
	cur_frame = fp->exfunc->get_frame(editp);
	frame_n = fp->exfunc->get_frame_n(editp);
	
	if (abs) {
		cur_frame = 0;
	}
	new_frame = cur_frame
			+ (fi.video_rate * sec
					+ ((sec >= 0) ? fi.video_scale : -fi.video_scale) / 2)
			/ fi.video_scale;
	if (new_frame < 0) {
		new_frame = 0;
	} else if (new_frame >= frame_n) {
		new_frame = frame_n - 1;
	}
	fp->exfunc->set_frame(editp,  new_frame);
	
	if (fp->check[0]) {
		hwndJump = FindJumpWindow();
		if (hwndJump && IsWindowVisible(hwndJump)) {
			PostMessage(hwndJump, WM_COMMAND,
					MAKEWPARAM(IDC_JUMPWINDOW_MAINWNDFRAME, 0), 0);
		}
	}
	
	return TRUE;
}


int get_second(const char *str, bool *abs)
{
	int sec = 0;
	int sign = 1;
	
	while (*str == ' ')
		str++;
	if ((*str == '+') || (*str == '-')) {
		if (*str == '-') {
			sign = -1;
		}
		*abs = false;
		str++;
	} else {
		*abs = true;
	}
	
	while (1) {
		int i = 0;
		while ('0' <= *str && *str <= '9') {
			i = i * 10 + (*str++ - '0');
		}
		sec += i;
		if (*str == ':') {
			sec *= 60;
			str++;
		} else {
			break;
		}
	}
	return sec * sign;
}


//---------------------------------------------------------------------
//		全コントロール有効/無効
//---------------------------------------------------------------------
void EnableToolWindow(HWND hwnd, BOOL bEnable)
{
	HWND hwndChild = NULL;
	
	while ((hwndChild = FindWindowEx(hwnd, hwndChild, NULL, NULL)) != NULL) {
		EnableWindow(hwndChild, bEnable);
	}
}



WNDPROC g_EditProc = NULL;
//---------------------------------------------------------------------
//		EditProc
//---------------------------------------------------------------------
LRESULT CALLBACK EditProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if ((uMsg == WM_CHAR) && (wParam == '\r')) {
		PostMessage(GetParent(hwnd), WM_COMMAND, (WPARAM) IDC_JUMP, (LPARAM) hwnd);
		return 0;
	}
	return CallWindowProc(g_EditProc, hwnd, uMsg, wParam, lParam);
}


WNDPROC g_ButtonProc = NULL;
//---------------------------------------------------------------------
//		ButtonProc
//---------------------------------------------------------------------
LRESULT CALLBACK ButtonProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_KEYDOWN:
	case WM_KEYUP:
		if (wParam == VK_SPACE || wParam == VK_TAB) {
			break;
		}
		PostMessage(GetParent(hwnd), uMsg, wParam, lParam);
		return 0;
	}
	return CallWindowProc(g_ButtonProc, hwnd, uMsg, wParam, lParam);
}


//---------------------------------------------------------------------
//		WndProc
//---------------------------------------------------------------------
BOOL func_WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam,
		void *editp, FILTER *fp)
{
	int i;
	bool abs;
	static HFONT hFont;
	LOGFONT lf;
	char buf[64];
	static HWND hwndEdit;
	HWND hwndBtn;
	HWND hwndChild;
	//	TRUEを返すと全体が再描画される
	
	switch (message) {
	case WM_FILTER_INIT:
		hwndChild = FindWindowEx(hwnd, NULL, "BUTTON", check_name[0]);
		SetWindowPos(hwndChild, NULL,
				BUTTON_MARGIN,
				(BUTTON_H + BUTTON_MARGIN) * (lengthof(buttons) / 2 + 1) + BUTTON_MARGIN*2,
				0, 0, SWP_NOSIZE | SWP_NOZORDER);
		
		hFont = CreateFont(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
				DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
				DEFAULT_QUALITY, DEFAULT_PITCH, NULL);
	//	SetWindowFont(hwnd, hFont, FALSE);
		for (i = 0; i < lengthof(buttons); i++) {
			int x = i % 2;
			int y = i / 2;
			hwndBtn = CreateWindow("BUTTON", buttons[i].title, WS_VISIBLE | WS_CHILD | WS_TABSTOP,
					(BUTTON_W + BUTTON_MARGIN) * x + BUTTON_MARGIN,
					(BUTTON_H + BUTTON_MARGIN) * y + BUTTON_MARGIN,
					BUTTON_W, BUTTON_H, hwnd, (HMENU) buttons[i].id,
					GetWindowInstance(hwnd), NULL);
			SetWindowFont(hwndBtn, hFont, TRUE);
			g_ButtonProc = SubclassWindow(hwndBtn, ButtonProc);
		}
		hwndEdit = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_VISIBLE | WS_CHILD | WS_TABSTOP,
				BUTTON_MARGIN + 10,
				(BUTTON_H + BUTTON_MARGIN) * (lengthof(buttons) / 2) + BUTTON_MARGIN + BUTTON_MARGIN/2,
				BUTTON_W + 5, BUTTON_H, hwnd, (HMENU) IDC_EDIT,
				GetWindowInstance(hwnd), NULL);
		SetWindowFont(hwndEdit, hFont, TRUE);
		hwndBtn = CreateWindow("BUTTON", "Jump", WS_VISIBLE | WS_CHILD | WS_TABSTOP,
				BUTTON_MARGIN*2 + BUTTON_W + 10,
				(BUTTON_H + BUTTON_MARGIN) * (lengthof(buttons) / 2) + BUTTON_MARGIN + BUTTON_MARGIN/2,
				BUTTON_W - 20, BUTTON_H, hwnd, (HMENU) IDC_JUMP,
				GetWindowInstance(hwnd), NULL);
		SetWindowFont(hwndBtn, hFont, TRUE);
		SetWindowText(hwndEdit, "+00:00");
		g_ButtonProc = SubclassWindow(hwndBtn, ButtonProc);
		g_EditProc = SubclassWindow(hwndEdit, EditProc);
		EnableToolWindow(hwnd, FALSE);
		break;
		
	case WM_FILTER_FILE_OPEN:
		EnableToolWindow(hwnd, TRUE);
		break;
		
	case WM_FILTER_FILE_CLOSE:
		EnableToolWindow(hwnd, FALSE);
		break;
		
	case WM_FILTER_EXIT:
	//	SubclassWindow(hwndEdit, g_EditProc);
		while ((hwndChild = FindWindowEx(hwnd, NULL, NULL, NULL)) != NULL) {
			DestroyWindow(hwndChild);
		}
		DeleteObject(hFont);
		break;
		
	case WM_COMMAND:
		switch (wParam) {
		case IDC_BCK_1SEC:		timejump(editp, fp, -1);	break;
		case IDC_FWD_1SEC:		timejump(editp, fp, +1);	break;
		case IDC_BCK_5SEC:		timejump(editp, fp, -5);	break;
		case IDC_FWD_5SEC:		timejump(editp, fp, +5);	break;
		case IDC_BCK_10SEC:		timejump(editp, fp, -10);	break;
		case IDC_FWD_10SEC:		timejump(editp, fp, +10);	break;
		case IDC_BCK_15SEC:		timejump(editp, fp, -15);	break;
		case IDC_FWD_15SEC:		timejump(editp, fp, +15);	break;
		case IDC_BCK_30SEC:		timejump(editp, fp, -30);	break;
		case IDC_FWD_30SEC:		timejump(editp, fp, +30);	break;
		case IDC_BCK_1MIN:		timejump(editp, fp, -60);	break;
		case IDC_FWD_1MIN:		timejump(editp, fp, +60);	break;
		case IDC_BCK_1_5MIN:	timejump(editp, fp, -90);	break;
		case IDC_FWD_1_5MIN:	timejump(editp, fp, +90);	break;
		case IDC_BCK_2MIN:		timejump(editp, fp, -120);	break;
		case IDC_FWD_2MIN:		timejump(editp, fp, +120);	break;
		case IDC_BCK_3MIN:		timejump(editp, fp, -180);	break;
		case IDC_FWD_3MIN:		timejump(editp, fp, +180);	break;
		case IDC_BCK_5MIN:		timejump(editp, fp, -300);	break;
		case IDC_FWD_5MIN:		timejump(editp, fp, +300);	break;
		case IDC_BCK_10MIN:		timejump(editp, fp, -600);	break;
		case IDC_FWD_10MIN:		timejump(editp, fp, +600);	break;
		case IDC_BCK_15MIN:		timejump(editp, fp, -900);	break;
		case IDC_FWD_15MIN:		timejump(editp, fp, +900);	break;
			
		case IDC_JUMP:
			GetWindowText(hwndEdit, buf, lengthof(buf));
			i = get_second(buf, &abs);
			timejump(editp, fp, i, abs);
			break;
			
		default:
			return FALSE;
		}
		return TRUE;
		
	case WM_KEYDOWN:
	case WM_KEYUP:
		PostMessage(GetWindowOwner(hwnd), message, wParam, lParam);
		break;
		
	case WM_FILTER_SAVE_START:
		SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED);
		break;
	case WM_FILTER_SAVE_END:
		SetThreadExecutionState(ES_CONTINUOUS);
		break;
		
	default:
		break;
	}
	return FALSE;
}

/*
History

2008/08/17 version 0.01
・最初の公開バージョン

2008/09/07 version 0.02
・時間ジャンプウィンドウにフォーカスがある場合でも、←、→、[、] などの
  ショートカットキーが使えるようにした。 

2008/10/04 version 0.03
・ファイルを開いていないときは、ボタンを押せないように変更。
・ジャンプウィンドウプラグインの画面を同期して表示できるように変更。

2009/11/01 version 0.04
・エンコード中は、PC がスリープ状態にならないように変更。（Win2k 以降必須）

2011/01/06 version 0.05
・YUY2 フィルタモードに対応。

*/
