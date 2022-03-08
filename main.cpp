//	Time to chew steel.

// later
//  if explorer crashes, will the icon disappear
//  get a proper icon
// ini loading

//next steps:
//  remove ghost icon
// change text, bg color


// Include Windows.h before all others.
#ifdef UNICODE
#undef UNICODE
#endif

#ifdef _UNICODE
#undef _UNICODE
#endif

#include <Windows.h>
#include <ShellAPI.h>
#include <tchar.h>
#include <stdio.h>
#include <winuser.h>

#include <chrono>
#include <ctime>
#include <fstream>
#include <regex>
#include <string>
#include <sstream>
#include <cassert>

#include "resource.h"

//	Window dimensions (make loadable from config)
static const int WINDOW_WIDTH = 340;
static const int WINDOW_HEIGHT = 200;

// Default file (make loadable from config)
static std::string saveFile("notes.txt");
static std::string timestampFormat("%A, %d %B %G, %H:%M");

//	ID constants
static const int ID_EDIT = 1;
static const int ID_HOTKEY = 2;
static const int ID_TRAYICON = 300;
static const int MSG_TRAYICON = WM_USER + 1;

//	Hotkey constants (make loadable from config)
static const TCHAR hotkey_char = TEXT(' ');
static const UINT hotkey_modifiers = MOD_CONTROL;

//	Tray menu item IDs
static const int IDTRAY_EXIT_ITEM = 500;

static const TCHAR szAppName[] = TEXT("data-aggregator");

static bool windowVisible = false;

HWND hwnd = nullptr;
HWND hwndEdit = nullptr;
HMENU hTrayMenu = nullptr;
NOTIFYICONDATA nid;

int CDECL MessageBoxPrintf (TCHAR * szCaption, TCHAR * szFormat, ...)
{
     TCHAR   szBuffer [1024] ;
     va_list pArgList ;

          // The va_start macro (defined in STDARG.H) is usually equivalent to:
          // pArgList = (char *) &szFormat + sizeof (szFormat) ;

     va_start (pArgList, szFormat) ;

          // The last argument to wvsprintf points to the arguments

     _vsntprintf_s (szBuffer, sizeof (szBuffer) / sizeof (TCHAR), 
                  szFormat, pArgList) ;

          // The va_end macro just zeroes out pArgList for no good reason

     va_end (pArgList) ;

     return MessageBox (nullptr, szBuffer, szCaption, 0) ;
}
void debugBox(LPCTSTR debugText)
{
	MessageBox(nullptr, debugText, TEXT("Debug Text"), MB_OK);
}

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void InitTrayIconData(HWND);
void ParseAndSave(LPTSTR);
void ToggleWindowState();
void SaveNotes(const std::string&, const std::string&);
std::string GetTimeStamp();

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	MSG msg;
	WNDCLASSEX wndclass;

	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = hInstance;
	wndclass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON));
	wndclass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
	wndclass.lpszMenuName = nullptr;
	wndclass.lpszClassName = szAppName;
	wndclass.cbSize = sizeof(WNDCLASSEX);
	wndclass.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON_SM));

	if(!RegisterClassEx(&wndclass))
	{
		MessageBox(nullptr, TEXT("WndClass Registration failed!"), szAppName, MB_ICONERROR);
		return 0;
	}

	//	Get the screen's dimensions
	int cx = GetSystemMetrics(SM_CXSCREEN);
	//int cy = GetSystemMetrics(SM_CYSCREEN);

	//	Window should appear at the top right
	int xWindow = cx - WINDOW_WIDTH;
	int yWindow = 0;

	hwnd = CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_LAYERED,
						szAppName,
						TEXT("Data Aggregator (in-dev)"),
						WS_POPUP,
						xWindow,
						yWindow,
						WINDOW_WIDTH,
						WINDOW_HEIGHT,
						nullptr,
						nullptr,
						hInstance,
						nullptr);
	SetLayeredWindowAttributes(hwnd, 0, BYTE(255 * 0.8), LWA_ALPHA);	//	Make window translucent
	ShowWindow(hwnd, SW_HIDE);	// Window is hidden on startup
	UpdateWindow(hwnd);
	RegisterHotKey(hwnd, ID_HOTKEY, hotkey_modifiers, hotkey_char);
	
	while(GetMessage(&msg, nullptr, 0, 0))
	{
		//	We process VK_ESCAPE before Translating the message so it isn't handled the Edit control instead
		if(msg.message == WM_KEYDOWN)
			if(GetAsyncKeyState(VK_ESCAPE))
				SendMessage(hwnd, WM_CLOSE, 0, 0);
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	PAINTSTRUCT ps;
	switch(message)
	{
	case WM_CREATE:
		hwndEdit = CreateWindow(TEXT("edit"),
								nullptr,
								WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_NOHIDESEL,
								0,
								0,
								0,
								0,
								hwnd,
								(HMENU)ID_EDIT,
								((LPCREATESTRUCT)lParam)->hInstance,
								nullptr);

		InitTrayIconData(hwnd);
		Shell_NotifyIcon(NIM_ADD, &nid);

		hTrayMenu = CreatePopupMenu();
		AppendMenu(hTrayMenu, MF_STRING, IDTRAY_EXIT_ITEM, TEXT("Exit"));

		return 0;

	case WM_SETFOCUS:
		SetFocus(hwndEdit);
		return 0;

	case WM_COMMAND:
		if(LOWORD(wParam) == ID_EDIT)
		{
			if(HIWORD(wParam) == EN_ERRSPACE || HIWORD(wParam) == EN_MAXTEXT)
				MessageBox(hwnd, TEXT("Edit control out of space."), szAppName, MB_OK | MB_ICONSTOP);
		}

		return 0;

	case WM_SIZE:
		MoveWindow(hwndEdit, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
		return 0;

	case WM_HOTKEY:
		if(LOWORD(lParam) == hotkey_modifiers && HIWORD(lParam) == hotkey_char)
			ToggleWindowState();
		return 0;

	case MSG_TRAYICON:	// We're not checking the id because we've only got one icon
		if(lParam == WM_LBUTTONUP)
			ToggleWindowState();
		else if(lParam == WM_RBUTTONDOWN)
		{
			POINT pos = {0};
			GetCursorPos(&pos);
			SetForegroundWindow(hwnd);
			UINT clicked = TrackPopupMenu(hTrayMenu, TPM_RETURNCMD, pos.x, pos.y, 0, hwnd, nullptr);
			PostMessage(hwnd, WM_NULL, 0, 0);
			if(clicked == IDTRAY_EXIT_ITEM)
				SendMessage(hwnd, WM_CLOSE, 0, 0);
		}

		return 0;

	case WM_PAINT:
		hdc = BeginPaint(hwnd, &ps);
		EndPaint(hwnd, &ps);
		return 0;

	case WM_CLOSE:
		DestroyWindow(hwnd);
		return 0;

	case WM_DESTROY:
		Shell_NotifyIcon(NIM_DELETE, &nid);
		PostQuitMessage(0);
		return 0;

	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}

void InitTrayIconData(HWND hwnd)
{
	HINSTANCE hInstance = (HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE);
	memset(&nid, 0, sizeof(NOTIFYICONDATA));
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = hwnd;
	nid.uID = ID_TRAYICON;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage = MSG_TRAYICON;
	nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON));
	//nid.uVersion = NOTIFYICON_VERSION_4;
	lstrcpy(nid.szTip, TEXT("Data Aggregator in-dev version"));
}

// For some odd reason, when the window is made visible by clicking on the tray icon,
// it doesn't capture keyboard input until I click inside the window.
void ToggleWindowState()
{
	if(windowVisible)
	{
		ShowWindow(hwnd, SW_HIDE);

		//	Get the length of text in the control
		int length = GetWindowTextLength(hwndEdit);

		//Now get the text in the control
		LPTSTR text = new TCHAR[length + 1];
		GetWindowText(hwndEdit, text, length + 1);

		//pase and save the text
		ParseAndSave(text);

		//empty the control (or not), depending on the settings chosen by the user
		SetWindowText(hwndEdit, TEXT(""));
	}
	else
	{
		ShowWindow(hwnd, SW_SHOW);
		SetActiveWindow(hwnd);
		assert(SetFocus(hwndEdit));
	}
	windowVisible = !windowVisible;
}

// Note entry format:
// options start with a colon and end with a semicolon,
// indicate their value inside parentheses, e.g, :append("myfile.txt");Notes Notes Notes
// if there is no colon in the beginning of a note, there are no options, save notes to default.txt
// Supported options:
// file(myfile.txt)
// asis() - everything after this option is taken to be plain text (no more parsing)
// possibly the ability to save path specified by the arg passed to file(), save in program folder for now, ini file folder later on
// Save with timestamp

void ParseAndSave(LPTSTR text)
{
	// one possiblity is to handle each options separately, like so:
	// :file\(.+\) (no grabbing yet)
	// ;.+ (text, no grabbing yet)

	// this matches one option
	// :.+\)|:.+;
	// this does it properly (apparently)
	// :[^:]+\)|:.+;
	// this matches all the options and the text (no, it doesn't)
	// :[^:]+\)|:[^:]+;.+
	// with grabbing
	// :([^:]+\))|:([^:]+);
	// grab text separately
	// ;.+

	//	If there is no text, return
	if(lstrlen(text) == 0)
		return;

	std::string str(text);
	std::smatch res;

	// First we get the text to be stored
	std::regex text_regex(";(.+)");
	//	If there is no semicolon, then save the text as is (wont work if text has a semicolon)
	if(!std::regex_search(str, res, text_regex))
	{
		SaveNotes(saveFile, str);
		return;
	}
	std::string to_store = res[1].str();		// res[1] gets the first capture
	str = std::regex_replace(str, text_regex, std::string(""), std::regex_constants::format_first_only);	// remove the text, leaving options only
	
	//	Now to handle the options
	std::regex option_regex(":([^:]+\\))|:([^:]+);");
	while(std::regex_search(str, res, option_regex))
	{
		std::string option = res[1].str();
		str = std::regex_replace(str, option_regex, std::string(""), std::regex_constants::format_first_only);
		
		// good luck understanding this later on, no verbose regex support in std::regex
		// each option must have a param. use 'void' for empty params
		std::regex func_and_param_capture("([A-Za-z0-9_\\.]+)\\(([A-Za-z0-9_\\.]+)\\)");	
		std::smatch match;
		std::regex_match(option, match, func_and_param_capture);

		// if the option is 'file' then save the data to the filename given in param
		if(match[1].str() == std::string("file"))
		{
			SaveNotes(match[2].str(), to_store);
		}
	}

	delete[] text;
	text = nullptr;
}

void SaveNotes(const std::string& filename, const std::string& text)
{
	std::ofstream ofs(filename, std::ios::app);
	ofs << "\n\n" << GetTimeStamp() << '\n' << text;
}

std::string GetTimeStamp()
{
	auto now = std::chrono::system_clock::now();
	auto rawtime = std::chrono::system_clock::to_time_t(now);
	auto timeinfo = std::localtime(&rawtime);
	char timeStr[100];
	std::strftime(timeStr, 100, timestampFormat.c_str(), timeinfo);
	return timeStr;
}