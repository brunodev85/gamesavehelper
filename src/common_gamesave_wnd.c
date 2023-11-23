#include "main.h"
#include "common_gamesave_wnd.h"
#include "text_utils.h"

struct Gamedata {
	char name[64];
	char path[128];
};

enum Msg {
	MSG_CLOSE = WM_APP
};

HWND hwndStatic; 
HWND hwndList;
HWND hwndDlg;
bool backupWndClassRegistered = false;
bool restoreWndClassRegistered = false;
struct Gamedata* items = NULL;
int action;
int numItems = 0;
bool hasSelectedItems = false;

INT_PTR CALLBACK ProgressDialogProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
		case WM_INITDIALOG: {
			RECT rect, rect1;
            GetWindowRect(GetParent(hwndDlg), &rect);
            GetClientRect(hwndDlg, &rect1);
            SetWindowPos(hwndDlg, NULL, (rect.right + rect.left) / 2 - (rect1.right - rect1.left) / 2, (rect.bottom + rect.top) / 2 - (rect1.bottom - rect1.top) / 2, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

            HWND hwndProgressBar = GetDlgItem(hwndDlg, IDPBC_PROGRESS_BAR);

            SetWindowLongPtr(hwndProgressBar, GWL_STYLE, GetWindowLongPtr(hwndProgressBar, GWL_STYLE) | PBS_MARQUEE);
            SendMessage(hwndProgressBar, WM_USER+10, 1, 100);
            return (INT_PTR)TRUE;
		}
        case MSG_CLOSE: {
            DestroyWindow(hwndDlg);
            break;
        }		
	}

	return (INT_PTR)FALSE;	
}

void updateDisplayText() {
	int selectedItemCount = 0;
	for (int i = 0; i < numItems; i++) {
		if (SendMessage(hwndList, LB_GETSEL, i, 0) > 0) {
			selectedItemCount++;
		}
	}

	hasSelectedItems = selectedItemCount > 0;
	wchar_t text[128];
	swprintf(text, L"Selected items: %d/%d", selectedItemCount, numItems);
	SetWindowTextW(hwndStatic, text);
}

void addItem(char* name, char* path) {
	int index = numItems;
	numItems++;
	items = realloc(items, numItems * sizeof(struct Gamedata));
	strcpy(items[index].name, name);
	strcpy(items[index].path, path);

	wchar_t* wname = towchar(name);
	SendMessageW(hwndList, LB_ADDSTRING, 0, (LPARAM)wname);	
	free(wname);
}

void loadItems() {
	FILE* f = fopen("gamesave_locations.csv", "r");
	char line[1024];
	
	if (action == ACTION_BACKUP) {
		while (fgets(line, 1024, f)) {
			char* path = getcsvtext(line, 1);
			DIR* dir = opendir(path);
			if (dir) {
				char* name = getcsvtext(line, 0);
				addItem(name, path);			
				closedir(dir);
				free(name);
			}
			free(path);
		}
	}
	else if (action == ACTION_RESTORE) {
		char originPath[128];
		while (fgets(line, 1024, f)) {
			char* name = getcsvtext(line, 0);
			sprintf(originPath, "savedgames\\%s", name);
			DIR* dir = opendir(originPath);
			if (dir) {
				char* path = getcsvtext(line, 1);
				addItem(name, path);
				closedir(dir);
				free(path);
			}
			free(name);
		}		
	}
	
	fclose(f);	
}

void backupGamesavesTask(void *data) {
	mkdir("savedgames");
	bool error = false;
	char command[256];
	char targetPath[128];
	
	for (int i = 0; i < numItems; i++) {
		if (SendMessage(hwndList, LB_GETSEL, i, 0) > 0) {
			sprintf(targetPath, "savedgames\\%s", items[i].name);
		
			sprintf(command, "rmdir /s /q \"%s\"", targetPath);
			sysexec(command);
						
			sprintf(command, "xcopy /s /q \"%s\\\" \"%s\\\"", items[i].path, targetPath);
			sysexec(command);
			
			DIR* dir = opendir(targetPath);
			if (!dir) {
				error = true;
				break;
			}			
			else closedir(dir);
		}
	}	
	
	SendMessage(hwndList, LB_SELITEMRANGE, FALSE, MAKELPARAM(0, numItems-1));
	updateDisplayText();
	
	SendMessage(hwndDlg, MSG_CLOSE, 0, 0);
	if (!error) {
		MessageBox(NULL, (LPCWSTR)L"Files copied successfully.", (LPCWSTR)L"Information", MB_ICONINFORMATION);
	}
}

void restoreGamesavesTask(void *data) {
	bool error = false;
	char command[256];
	char originPath[128];
	
	for (int i = 0; i < numItems; i++) {
		if (SendMessage(hwndList, LB_GETSEL, i, 0) > 0) {
			sprintf(originPath, "savedgames\\%s", items[i].name);
						
			sprintf(command, "xcopy /s /y /q \"%s\\\" \"%s\\\"", originPath, items[i].path);
			sysexec(command);
			
			DIR* dir = opendir(items[i].path);
			if (!dir) {
				error = true;
				break;
			}			
			else closedir(dir);
		}
	}	
	
	SendMessage(hwndList, LB_SELITEMRANGE, FALSE, MAKELPARAM(0, numItems-1));
	updateDisplayText();
	
	SendMessage(hwndDlg, MSG_CLOSE, 0, 0);
	if (!error) {
		MessageBox(NULL, (LPCWSTR)L"Files restored successfully.", (LPCWSTR)L"Information", MB_ICONINFORMATION);
	}
}

LRESULT CALLBACK CommonGamesaveWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {	
	switch (msg) {  
		case WM_COMMAND: {
			switch (LOWORD(wParam)) {
				case IDC_LIST: {
					if (HIWORD(wParam) == LBN_SELCHANGE) {
						updateDisplayText();
				    }
					break;
				}
				case IDB_SELECT_ALL: {
					SendMessage(hwndList, LB_SELITEMRANGE, TRUE, MAKELPARAM(0, numItems-1));	
					updateDisplayText();
					break;	
				}
				case IDB_DESELECT_ALL: {
					SendMessage(hwndList, LB_SELITEMRANGE, FALSE, MAKELPARAM(0, numItems-1));
					updateDisplayText();
					break;	
				}
				case IDB_CANCEL: {
					DestroyWindow(hwnd);
					break;
				}
				case IDB_OK: {
					if (!hasSelectedItems) {
						MessageBox(NULL, (LPCWSTR)L"No items selected.", (LPCWSTR)L"Warning", MB_ICONWARNING);
						break;
					}
					
					if (action == ACTION_BACKUP) {
						hwndDlg = CreateDialogParam(globalHInstance, MAKEINTRESOURCE(IDD_PROGRESSDIALOG), hwnd, ProgressDialogProc, 0);
						ShowWindow(hwndDlg, SW_SHOW);
						_beginthread(&backupGamesavesTask, 0, NULL);
					}
					else if (action == ACTION_RESTORE) {
						hwndDlg = CreateDialogParam(globalHInstance, MAKEINTRESOURCE(IDD_PROGRESSDIALOG), hwnd, ProgressDialogProc, 0);
						ShowWindow(hwndDlg, SW_SHOW);
						_beginthread(&restoreGamesavesTask, 0, NULL);
					}
					break;
				}				
			}
			break;
		}
		case WM_CREATE: {		
			RECT rect;
			GetClientRect(hwnd, &rect);
			int hwndWidth = rect.right - rect.left;
			int hwndHeight = rect.bottom - rect.top;
			
			int listBoxWidth = hwndWidth - MARGIN * 2;
			int listBoxHeight = hwndHeight - BOTTOM_BAR_HEIGHT - MARGIN * 2;
			hwndList = CreateWindowW(WC_LISTBOXW, NULL, WS_CHILD | WS_VISIBLE | LBS_NOTIFY | WS_VSCROLL | WS_BORDER | LBS_MULTIPLESEL, 
									 MARGIN, MARGIN, listBoxWidth, listBoxHeight, hwnd, (HMENU)IDC_LIST, NULL, NULL);
			
			loadItems();
			
			int offsetX = MARGIN;
			int offsetY = hwndHeight - BOTTOM_BAR_HEIGHT - MARGIN * 2;
			int textBoxWidth = hwndWidth - (SMALL_BUTTON_WIDTH + MARGIN) * 4 - MARGIN * 2;
			hwndStatic = CreateWindowW(WC_STATICW, NULL, WS_CHILD | WS_VISIBLE,
									   offsetX, offsetY + 5, textBoxWidth, SMALL_BUTTON_HEIGHT - 10, hwnd, (HMENU)IDC_STATIC, NULL, NULL);
									   
			offsetX += textBoxWidth + MARGIN;
			CreateWindowW(L"BUTTON", L"Select All", WS_CHILD | WS_VISIBLE, offsetX, offsetY, SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT, 
						  hwnd, (HMENU)IDB_SELECT_ALL, globalHInstance, NULL);

			offsetX += SMALL_BUTTON_WIDTH + MARGIN;
			CreateWindowW(L"BUTTON", L"Deselect All", WS_CHILD | WS_VISIBLE, offsetX, offsetY, SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT, 
						  hwnd, (HMENU)IDB_DESELECT_ALL, globalHInstance, NULL);

			offsetX += SMALL_BUTTON_WIDTH + MARGIN;
			CreateWindowW(L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE, offsetX, offsetY, SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT, 
						  hwnd, (HMENU)IDB_CANCEL, globalHInstance, NULL);	

			offsetX += SMALL_BUTTON_WIDTH + MARGIN;
			CreateWindowW(L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE, offsetX, offsetY, SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT, 
						  hwnd, (HMENU)IDB_OK, globalHInstance, NULL);	

			updateDisplayText();
			break;
		}
		case WM_DESTROY: {
			hwndList = NULL;
			hwndStatic = NULL;
			free(items);
			items = NULL;
			numItems = 0;
			hasSelectedItems = false;
			break;
		}
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);	
}

void initCommonGamesaveWindow(int _action) {
	action = _action;
	HINSTANCE hInstance = globalHInstance;
	
	LPCTSTR wndClass = NULL;
	bool* wndClassRegistered = false;
	
	if (action == ACTION_BACKUP) {
		wndClass = L"Backup Gamesaves";
		wndClassRegistered = &backupWndClassRegistered;
	}
	else if (action == ACTION_RESTORE) {
		wndClass = L"Restore Gamesaves";
		wndClassRegistered = &restoreWndClassRegistered;
	}
	
	if (!(*wndClassRegistered)) {
		WNDCLASSEX wcx;
		wcx.cbSize = sizeof(wcx);
		wcx.style = CS_HREDRAW | CS_VREDRAW;
		wcx.lpfnWndProc = &CommonGamesaveWndProc;
		wcx.cbClsExtra = 0;
		wcx.cbWndExtra = 0;
		wcx.hInstance = hInstance;
		wcx.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAIN));
		wcx.hCursor = (HCURSOR)LoadImage(hInstance, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_SHARED);
		wcx.hbrBackground = (HBRUSH)COLOR_WINDOW;
		wcx.lpszClassName = wndClass;
		wcx.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAIN));

		if (!RegisterClassEx(&wcx)) return;
		*wndClassRegistered = true;
	}
	
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
	int hwndWidth = 600;
	int hwndHeight = 480;	
	HWND hwnd = CreateWindowEx(0, wndClass, wndClass, WS_VISIBLE | WS_OVERLAPPEDWINDOW, (screenWidth - hwndWidth) / 2, (screenHeight - hwndHeight) / 2, 600, 480, 
							   NULL, NULL, hInstance, NULL);
	if (!hwnd) return;
	
	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);	
}