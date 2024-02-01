#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <windows.h>
#include <commctrl.h>
#include <wingdi.h>
#include <wchar.h>

#include "resource.h"

#define SMALL_BUTTON_WIDTH 100
#define SMALL_BUTTON_HEIGHT 30
#define LARGE_BUTTON_WIDTH 195
#define LARGE_BUTTON_HEIGHT 45
#define BOTTOM_BAR_HEIGHT 30
#define MARGIN 5

#define ACTION_BACKUP 1
#define ACTION_RESTORE 2

//#define DEBUG

#ifdef DEBUG
void debug_printf(const char *fmt, ...);
#endif

DWORD sysexec(char* command);

INT_PTR CALLBACK AboutDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);

#endif