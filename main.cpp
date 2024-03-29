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

#include "config.h"
#include "resource.h"

//	ID constants
static const int ID_EDIT = 1;
static const int ID_HOTKEY = 2;
static const int ID_TRAYICON = 300;
static const int MSG_TRAYICON = WM_USER + 1;
static const int IDTRAY_EXIT_ITEM = 500;

static const TCHAR szAppName[] = TEXT("data-aggregator");
static const std::string timestampFormat("%A, %d %B %G, %H:%M");

static bool windowVisible = false;

HWND hwnd = nullptr;
HWND hwndEdit = nullptr;
HMENU hTrayMenu = nullptr;
NOTIFYICONDATA nid;

NotesConfig notesConfig = NotesConfig("config.config");

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void InitTrayIconData(HWND);
void ParseAndSave(LPTSTR);
void ToggleWindowState();
void SaveNotes(const std::string&, const std::string&);
std::string GetTimeStamp();

int WINAPI WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	PSTR szCmdLine,
	int iCmdShow) {

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

	if(!RegisterClassEx(&wndclass)) {
		MessageBox(
			nullptr,
			TEXT("WndClass Registration failed!"),
			szAppName,
			MB_ICONERROR);
		return 0;
	}

	//	Window should appear at the top right
	int xWindow = GetSystemMetrics(SM_CXSCREEN) - notesConfig.getWindowWidth();
	int yWindow = 0;

	hwnd = CreateWindowEx(
		WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_LAYERED,
		szAppName,
		TEXT("Data Aggregator (in-dev)"),
		WS_POPUP,
		xWindow,
		yWindow,
		notesConfig.getWindowWidth(),
		notesConfig.getWindowHeight(),
		nullptr,
		nullptr,
		hInstance,
		nullptr);

	//	Make window translucent
	SetLayeredWindowAttributes(hwnd, 0, BYTE(255 * 0.8), LWA_ALPHA);
	// Window is hidden on startup
	ShowWindow(hwnd, SW_HIDE);
	UpdateWindow(hwnd);
	RegisterHotKey(
		hwnd,
		ID_HOTKEY,
		notesConfig.getHotkeyMod(),
		TEXT(notesConfig.getHotkeyBase()));
	
	MSG msg;
	while(GetMessage(&msg, nullptr, 0, 0)) {
		// We handle VK_ESCAPE before translating the message so it isn't
		// handled by the edit control instead
		if(msg.message == WM_KEYDOWN && GetAsyncKeyState(VK_ESCAPE)) {
			SendMessage(hwnd, WM_CLOSE, 0, 0);
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return msg.wParam;
}

LRESULT CALLBACK WndProc(
	HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {

	HDC hdc;
	PAINTSTRUCT ps;
	switch(message) {
	case WM_CREATE:
		hwndEdit = CreateWindow(
			TEXT("edit"),
			nullptr,
			WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_MULTILINE |
			  ES_AUTOVSCROLL | ES_NOHIDESEL,
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
		if(LOWORD(wParam) == ID_EDIT) {
			if(HIWORD(wParam) == EN_ERRSPACE || HIWORD(wParam) == EN_MAXTEXT)
				MessageBox(
					hwnd,
					TEXT("Edit control out of space."),
					szAppName,
					MB_OK | MB_ICONSTOP);
		}

		return 0;

	case WM_SIZE:
		MoveWindow(hwndEdit, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
		return 0;

	case WM_HOTKEY:
		if(
			LOWORD(lParam) == notesConfig.getHotkeyMod() &&
			HIWORD(lParam) == TEXT(notesConfig.getHotkeyBase())) {
			ToggleWindowState();
		}
		return 0;

	case MSG_TRAYICON:
		if(lParam == WM_LBUTTONUP) {
			ToggleWindowState();
		}
		else if(lParam == WM_RBUTTONDOWN) {
			POINT pos = {0};
			GetCursorPos(&pos);
			SetForegroundWindow(hwnd);
			UINT clicked = TrackPopupMenu(
				hTrayMenu, TPM_RETURNCMD, pos.x, pos.y, 0, hwnd, nullptr);
			PostMessage(hwnd, WM_NULL, 0, 0);
			if(clicked == IDTRAY_EXIT_ITEM) {
				SendMessage(hwnd, WM_CLOSE, 0, 0);
			}
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

void InitTrayIconData(HWND hwnd) {
	HINSTANCE hInstance = (HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE);
	memset(&nid, 0, sizeof(NOTIFYICONDATA));
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = hwnd;
	nid.uID = ID_TRAYICON;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage = MSG_TRAYICON;
	nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON));
	lstrcpy(nid.szTip, TEXT("Data Aggregator"));
}

// For some odd reason, when the window is made visible by clicking on the tray
// icon, it doesn't capture keyboard input until I click inside the window.
void ToggleWindowState() {
	if(windowVisible) {
		ShowWindow(hwnd, SW_HIDE);

		//	get the length of text in the control
		int length = GetWindowTextLength(hwndEdit);

		// now get the text in the control
		LPTSTR text = new TCHAR[length + 1];
		GetWindowText(hwndEdit, text, length + 1);

		// parse and save the text
		ParseAndSave(text);
		delete[] text;

		// empty the control
		SetWindowText(hwndEdit, TEXT(""));
	}
	else {
		ShowWindow(hwnd, SW_SHOW);
		SetForegroundWindow(hwnd);
		assert(SetFocus(hwndEdit));
	}
	windowVisible = !windowVisible;
}

// Note entry format:
// options start with a colon and end with a semicolon,
// indicate their value inside parentheses
// e.g, :file(myfile.txt);Notes Notes Notes
// if there is no colon in the beginning of a note, there are no
// options, save notes to default
// Supported options:
//   - file(myfile.txt)

void ParseAndSave(LPTSTR text) {
	if(lstrlen(text) == 0) {
		return;
	}

	std::string str(text);
	std::smatch res;

	// First we get the text to be stored
	std::regex text_regex(";(.+)");
	// If there is no semicolon, then save the text as is
	// TODO: handle the case where the note has semicolons
	if(!std::regex_search(str, res, text_regex)) {
		SaveNotes(notesConfig.getNotesFileName(), str);
		return;
	}
	// res[1] gets the first capture
	std::string to_store = res[1].str();
	str = std::regex_replace(
		str,
		text_regex,
		std::string(""),
		// remove the text, leaving options only
		std::regex_constants::format_first_only);
	
	//	Now to handle the options
	std::regex option_regex(":([^:]+\\))|:([^:]+);");
	while(std::regex_search(str, res, option_regex)) {
		std::string option = res[1].str();
		str = std::regex_replace(
			str,
			option_regex,
			std::string(""),
			std::regex_constants::format_first_only);
		
		// good luck understanding this later on, no verbose regex support in std::regex
		// each option must have a param. use 'void' for empty params
		std::regex func_and_param_capture("([A-Za-z0-9_\\.]+)\\(([A-Za-z0-9_\\.]+)\\)");	
		std::smatch match;
		std::regex_match(option, match, func_and_param_capture);

		// if the option is 'file' then save the data to the filename given in param
		if(match[1].str() == std::string("file")) {
			SaveNotes(match[2].str(), to_store);
		}
	}
}

void SaveNotes(const std::string& filename, const std::string& text) {
	std::ofstream ofs(filename, std::ios::app);
	ofs << "\n\n" << GetTimeStamp() << '\n' << text;
}

std::string GetTimeStamp() {
	auto now = std::chrono::system_clock::now();
	auto rawtime = std::chrono::system_clock::to_time_t(now);
	auto timeinfo = std::localtime(&rawtime);
	char timeStr[100];
	std::strftime(timeStr, 100, timestampFormat.c_str(), timeinfo);
	return timeStr;
}