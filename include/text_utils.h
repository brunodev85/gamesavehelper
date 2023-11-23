#ifndef TEXT_UTILS_H
#define TEXT_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <string.h>

char* getcsvtext(char* line, int num);
wchar_t* towchar(char* value);

#endif