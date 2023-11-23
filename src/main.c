#include "main.h"
#include "common_gamesave_wnd.h"

void debug_printf(const char *fmt, ...) {
	FILE *f = fopen("debug.txt", "a");
	if (f == NULL) return;
	va_list args;
    va_start(args, fmt);
    vfprintf(f, fmt, args);
    va_end(args);
	fclose(f);
}

DWORD sysexec(char* command) {
	char* cmdPath = "c:\\windows\\system32\\cmd.exe";
    char cmdArgs[strlen(command) + 4];
    sprintf(cmdArgs, "/c %s", command);
    
	PROCESS_INFORMATION pi = {0};
    STARTUPINFOA si = {0};
    si.cb = sizeof(STARTUPINFOA);
    
	DWORD status;
    if (CreateProcessA(cmdPath, cmdArgs, NULL, NULL, 0, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        GetExitCodeProcess(pi.hProcess, &status);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    
    return status;
}

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {  
		case WM_COMMAND: {
			switch (LOWORD(wParam)) {
				case IDB_BACKUP_GAMESAVE:
					initCommonGamesaveWindow(ACTION_BACKUP);
					break;
				case IDB_RESTORE_GAMESAVE:
					initCommonGamesaveWindow(ACTION_RESTORE);
					break;
				case ID_HELP_ABOUT:
					DialogBox(globalHInstance, MAKEINTRESOURCE(IDD_ABOUTDIALOG), hwnd, &AboutDialogProc);
					return 0;
				case ID_FILE_EXIT:
					DestroyWindow(hwnd);
					return 0;
			}
			break;
		}
		case WM_SYSCOMMAND: {
			switch (LOWORD(wParam)) {
				case ID_HELP_ABOUT: {
					DialogBox(globalHInstance, MAKEINTRESOURCE(IDD_ABOUTDIALOG), hwnd, &AboutDialogProc);
					return 0;
				}
			}
			break;
		}
		case WM_CREATE: {
			RECT rect;
			GetClientRect(hwnd, &rect);
			int hwndWidth = rect.right - rect.left;
			int hwndHeight = rect.bottom - rect.top;
			
			int centerX = (hwndWidth - LARGE_BUTTON_WIDTH) / 2;
			int centerY = (hwndHeight - LARGE_BUTTON_HEIGHT) / 2;
			int halfHeight = LARGE_BUTTON_HEIGHT / 2;
			
			HWND button;
			HICON hicon;
			
			button = CreateWindowW(L"BUTTON", L" Backup Gamesaves", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON , centerX, centerY - (halfHeight + MARGIN), LARGE_BUTTON_WIDTH, LARGE_BUTTON_HEIGHT, 
							       hwnd, (HMENU)IDB_BACKUP_GAMESAVE, globalHInstance, NULL);
			hicon = LoadIcon(globalHInstance, MAKEINTRESOURCE(IDI_BACKUP));
			SendMessageW(button, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hicon);								   
						  
			button = CreateWindowW(L"BUTTON", L" Restore Gamesaves", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON , centerX, centerY + (halfHeight + MARGIN), LARGE_BUTTON_WIDTH, LARGE_BUTTON_HEIGHT, 
					  			   hwnd, (HMENU)IDB_RESTORE_GAMESAVE, globalHInstance, NULL);
			hicon = LoadIcon(globalHInstance, MAKEINTRESOURCE(IDI_RESTORE));
			SendMessageW(button, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hicon);
			return 0;
		}
		case WM_DESTROY: {
			PostQuitMessage(0);
			return 0;
		}
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

INT_PTR CALLBACK AboutDialogProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
		case WM_COMMAND: {
			switch (LOWORD(wParam)) {
				case IDOK:
				case IDCANCEL: {
				  EndDialog(hwndDlg, (INT_PTR) LOWORD(wParam));
				  return (INT_PTR) TRUE;
				}
			}
			break;
		}
		case WM_INITDIALOG: {
			RECT rect, rect1;
            GetWindowRect(GetParent(hwndDlg), &rect);
            GetClientRect(hwndDlg, &rect1);
            SetWindowPos(hwndDlg, NULL, (rect.right + rect.left) / 2 - (rect1.right - rect1.left) / 2, (rect.bottom + rect.top) / 2 - (rect1.bottom - rect1.top) / 2, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
			return (INT_PTR)TRUE;
		}
	}

	return (INT_PTR)FALSE;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	globalHInstance = hInstance;

	WNDCLASSEX wcx;
	wcx.cbSize = sizeof(wcx);
	wcx.style = CS_HREDRAW | CS_VREDRAW;
	wcx.lpfnWndProc = &MainWndProc;
	wcx.cbClsExtra = 0;
	wcx.cbWndExtra = 0;
	wcx.hInstance = hInstance;
	wcx.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAIN));
	wcx.hCursor = LoadCursor(hInstance, IDC_ARROW);
	wcx.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wcx.lpszMenuName = MAKEINTRESOURCE(IDR_MAINMENU);
	wcx.lpszClassName = APP_NAME;
	wcx.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAIN));

	if (!RegisterClassEx(&wcx)) return 0;

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
	int hwndWidth = 350;
	int hwndHeight = 250;
	HWND hwnd = CreateWindowEx(0, APP_NAME, APP_NAME, WS_OVERLAPPEDWINDOW, (screenWidth - hwndWidth) / 2, (screenHeight - hwndHeight) / 2, hwndWidth, hwndHeight, 
							   NULL, NULL, hInstance, NULL);
	if (!hwnd) return 0;

	HMENU hSysMenu = GetSystemMenu(hwnd, FALSE);
	InsertMenu(hSysMenu, 5, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
	InsertMenu(hSysMenu, 6, MF_BYPOSITION, ID_HELP_ABOUT, L"About");

	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);

	MSG msg;
	while(GetMessage(&msg, NULL, 0, 0) > 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}	

	return (int)msg.wParam;
}
