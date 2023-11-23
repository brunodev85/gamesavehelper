#include "text_utils.h"

char* getcsvtext(char* line, int index) {
	char tmp[1024];
	strcpy(tmp, line);

	const char delim[3] = ",\n";
	const char* token = strtok(tmp, delim);
	int i = 0;
	while (token != NULL) {
		if (i++ == index) {
			char* result = malloc(128);
			strcpy(result, token);
			return result;
		}
		token = strtok(NULL, delim);
	}
	
    return NULL;
}

wchar_t* towchar(char* value) {
	int len = strlen(value);
	wchar_t* result = malloc(sizeof(wchar_t) * len + sizeof(wchar_t));
	result[len] = '\0';
	mbstowcs(result, value, len);
	return result;
}