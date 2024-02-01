#include "main.h"
#include "common_gamesave_wnd.h"

struct ListItem {
	wchar_t* name;
	wchar_t* path;
};

enum Msg {
	MSG_CLOSE = WM_APP
};

static HWND hwndStatic; 
static HWND hwndList;
static HWND hwndDlg;
static bool backupWndClassRegistered = false;
static bool restoreWndClassRegistered = false;
static struct ListItem* items = NULL;
static int action;
static int numItems = 0;
static bool hasSelectedItems = false;

extern HINSTANCE globalHInstance;

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

static void updateDisplayText() {
	int selectedItemCount = 0;
	for (int i = 0; i < numItems; i++) {
		if (SendMessage(hwndList, LB_GETSEL, i, 0) > 0) {
			selectedItemCount++;
		}
	}

	hasSelectedItems = selectedItemCount > 0;
	wchar_t text[128];
	swprintf_s(text, 128, L"Selected items: %d/%d", selectedItemCount, numItems);
	SetWindowTextW(hwndStatic, text);
}

static void addItem(wchar_t* name, wchar_t* path) {
	int index = numItems;
	numItems++;
	items = realloc(items, numItems * sizeof(struct ListItem));
	
	int nameLen = wcslen(name);
	int pathLen = wcslen(path);
	items[index].name = malloc((nameLen + 1) * sizeof(wchar_t));
	items[index].path = malloc((pathLen + 1) * sizeof(wchar_t));
	
	wcscpy_s(items[index].name, nameLen + 1, name);
	wcscpy_s(items[index].path, pathLen + 1, path);	
}

static void addItemFromLine(wchar_t* line) {
	int textLen = wcslen(line);
	wchar_t name[128];
	wchar_t path[MAX_PATH];
	wmemset(name, '\0', 128);
	wmemset(path, '\0', MAX_PATH);
	int i, j;
	
	for (i = 0, j = 0; i < textLen && line[i] != ','; i++) name[j++] = line[i];
	for (i++, j = 0; i < textLen; i++) path[j++] = line[i];
	
	if (action == ACTION_BACKUP) {
		DWORD dwAttrib = GetFileAttributes(path);
		if (dwAttrib != INVALID_FILE_ATTRIBUTES) addItem(name, path);
	}
	else if (action == ACTION_RESTORE) {
		wchar_t originPath[MAX_PATH];
		swprintf_s(originPath, MAX_PATH, L"savedgames\\%ls", name);
		DWORD dwAttrib = GetFileAttributes(originPath);
		if (dwAttrib != INVALID_FILE_ATTRIBUTES) addItem(name, path);		
	}
}

static int compare(const void* a, const void* b) {
	struct ListItem* ia = (struct ListItem*)a;
	struct ListItem* ib = (struct ListItem*)b;
	return wcscoll(ia->name, ib->name);
}

static void loadItems() {
	HANDLE hFile = CreateFileW(L"gamesave_locations.csv", GENERIC_READ,FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	
	char data[16384];
	DWORD bytesRead;
	ReadFile(hFile, data, 16384, &bytesRead, NULL);
	
	wchar_t line[1024];
	char tmp[512];

	for (int i = 0, j = 0; i < bytesRead; i++) {
		char chr = data[i];
		if (chr == '\n' || i == bytesRead-1) {
			if (i == bytesRead-1) tmp[j++] = chr;
			if (j > 0) {
				wmemset(line, '\0', 1024);
				MultiByteToWideChar(CP_UTF8, 0, tmp, j, line, 1024);
				addItemFromLine(line);
			}
			j = 0;
		}
		else if (chr != '\r') {
			tmp[j++] = chr;
		}
	}	
	
	CloseHandle(hFile);
	
	qsort(items, numItems, sizeof(struct ListItem), compare);
	for (int i = 0; i < numItems; i++) SendMessageW(hwndList, LB_ADDSTRING, 0, (LPARAM)items[i].name);	
}

static DWORD WINAPI backupGamesavesTask(void *param) {
	wchar_t srcPath[MAX_PATH];
	wchar_t dstPath[MAX_PATH];
	bool error = false;
	CreateDirectory(L"savedgames", NULL);
	
	for (int i = 0; i < numItems; i++) {
		if (SendMessage(hwndList, LB_GETSEL, i, 0) > 0) {
			struct ListItem* item = &items[i];
			swprintf_s(dstPath, MAX_PATH, L"savedgames\\%ls\\", item->name);
			CreateDirectory(dstPath, NULL);
			dstPath[wcslen(dstPath)+1] = '\0';
			
			swprintf_s(srcPath, MAX_PATH, L"%ls\\*", item->path);
			srcPath[wcslen(srcPath)+1] = '\0';
			
			SHFILEOPSTRUCT sfo;
			memset(&sfo, 0, sizeof(sfo));
			sfo.hwnd = hwndDlg;
			sfo.wFunc = FO_COPY;
			sfo.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_NOCONFIRMMKDIR;
			sfo.pTo = dstPath;
			sfo.pFrom = srcPath;

			int res = SHFileOperation(&sfo);	
			if (res != 0) {
				error = true;
				break;
			}
		}
	}	
	
	SendMessage(hwndList, LB_SELITEMRANGE, FALSE, MAKELPARAM(0, numItems-1));
	updateDisplayText();
	
	SendMessage(hwndDlg, MSG_CLOSE, 0, 0);
	if (!error) {
		MessageBox(NULL, (LPCWSTR)L"Files copied successfully.", (LPCWSTR)L"Information", MB_ICONINFORMATION);
	}
	
	return 0;
}

static DWORD WINAPI restoreGamesavesTask(void *param) {
	wchar_t srcPath[MAX_PATH];
	wchar_t dstPath[MAX_PATH];	
	bool error = false;
	
	for (int i = 0; i < numItems; i++) {
		if (SendMessage(hwndList, LB_GETSEL, i, 0) > 0) {
			struct ListItem* item = &items[i];
			swprintf_s(srcPath, MAX_PATH, L"savedgames\\%ls\\*", item->name);
			srcPath[wcslen(srcPath)+1] = '\0';
			
			swprintf_s(dstPath, MAX_PATH, L"%ls\\", item->path);
			dstPath[wcslen(dstPath)+1] = '\0';
			
			SHFILEOPSTRUCT sfo;
			memset(&sfo, 0, sizeof(sfo));
			sfo.hwnd = hwndDlg;
			sfo.wFunc = FO_COPY;
			sfo.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_NOCONFIRMMKDIR;
			sfo.pTo = dstPath;
			sfo.pFrom = srcPath;

			int res = SHFileOperation(&sfo);	
			if (res != 0) {
				error = true;
				break;
			}
		}
	}	
	
	SendMessage(hwndList, LB_SELITEMRANGE, FALSE, MAKELPARAM(0, numItems-1));
	updateDisplayText();
	
	SendMessage(hwndDlg, MSG_CLOSE, 0, 0);
	if (!error) {
		MessageBox(NULL, (LPCWSTR)L"Files restored successfully.", (LPCWSTR)L"Information", MB_ICONINFORMATION);
	}
	
	return 0;
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
						CreateThread(NULL, 0, backupGamesavesTask, NULL, 0, NULL);
					}
					else if (action == ACTION_RESTORE) {
						hwndDlg = CreateDialogParam(globalHInstance, MAKEINTRESOURCE(IDD_PROGRESSDIALOG), hwnd, ProgressDialogProc, 0);
						ShowWindow(hwndDlg, SW_SHOW);
						CreateThread(NULL, 0, restoreGamesavesTask, NULL, 0, NULL);
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
			hwndList = CreateWindowEx(0, WC_LISTBOXW, NULL, WS_CHILD | WS_VISIBLE | LBS_NOTIFY | WS_VSCROLL | WS_BORDER | LBS_MULTIPLESEL, 
									 MARGIN, MARGIN, listBoxWidth, listBoxHeight, hwnd, (HMENU)IDC_LIST, globalHInstance, NULL);
			
			loadItems();
			
			int offsetX = MARGIN;
			int offsetY = hwndHeight - BOTTOM_BAR_HEIGHT - MARGIN * 2;
			int textBoxWidth = hwndWidth - (SMALL_BUTTON_WIDTH + MARGIN) * 4 - MARGIN * 2;
			hwndStatic = CreateWindowEx(0, WC_STATICW, NULL, WS_CHILD | WS_VISIBLE,
									   offsetX, offsetY + 5, textBoxWidth, SMALL_BUTTON_HEIGHT - 10, hwnd, (HMENU)IDC_STATIC, globalHInstance, NULL);
									   
			offsetX += textBoxWidth + MARGIN;
			CreateWindowEx(0, WC_BUTTON, L"Select All", WS_CHILD | WS_VISIBLE, offsetX, offsetY, SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT, 
						  hwnd, (HMENU)IDB_SELECT_ALL, globalHInstance, NULL);

			offsetX += SMALL_BUTTON_WIDTH + MARGIN;
			CreateWindowEx(0, WC_BUTTON, L"Deselect All", WS_CHILD | WS_VISIBLE, offsetX, offsetY, SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT, 
						  hwnd, (HMENU)IDB_DESELECT_ALL, globalHInstance, NULL);

			offsetX += SMALL_BUTTON_WIDTH + MARGIN;
			CreateWindowEx(0, WC_BUTTON, L"Cancel", WS_CHILD | WS_VISIBLE, offsetX, offsetY, SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT, 
						  hwnd, (HMENU)IDB_CANCEL, globalHInstance, NULL);	

			offsetX += SMALL_BUTTON_WIDTH + MARGIN;
			CreateWindowEx(0, WC_BUTTON, L"OK", WS_CHILD | WS_VISIBLE, offsetX, offsetY, SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT, 
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
	
	LPCTSTR wndClassName = NULL;
	bool* wndClassRegistered = false;
	
	if (action == ACTION_BACKUP) {
		wndClassName = L"Backup Gamesaves";
		wndClassRegistered = &backupWndClassRegistered;
	}
	else if (action == ACTION_RESTORE) {
		wndClassName = L"Restore Gamesaves";
		wndClassRegistered = &restoreWndClassRegistered;
	}
	
	if (!(*wndClassRegistered)) {
		WNDCLASSEX wcx;
		memset(&wcx, 0, sizeof(wcx));
		wcx.cbSize = sizeof(wcx);
		wcx.style = CS_HREDRAW | CS_VREDRAW;
		wcx.lpfnWndProc = &CommonGamesaveWndProc;
		wcx.cbClsExtra = 0;
		wcx.cbWndExtra = 0;
		wcx.hInstance = hInstance;
		wcx.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAIN));
		wcx.hCursor = LoadCursor(hInstance, IDC_ARROW);
		wcx.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wcx.lpszClassName = wndClassName;
		wcx.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAIN));

		if (!RegisterClassEx(&wcx)) return;
		*wndClassRegistered = true;
	}
	
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
	int hwndWidth = 600;
	int hwndHeight = 480;
	int centerX = (screenWidth - hwndWidth) / 2;
	int centerY = (screenHeight - hwndHeight) / 2;
	
	HWND hwnd = CreateWindowEx(0, wndClassName, wndClassName, WS_VISIBLE | WS_OVERLAPPEDWINDOW, 
							  centerX, centerY, 600, 480, NULL, NULL, hInstance, NULL);
	if (!hwnd) return;
	
	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);	
}