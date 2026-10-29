#include "common.h"
#include <stdio.h>

void print_error(const char *prefix) {
    DWORD err = GetLastError();
    LPSTR msg = NULL;

    FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPSTR)&msg, 0, NULL);

    fprintf(stderr, "%s failed (code %lu): %s",
            prefix, err, msg ? msg : "unknown error\n");

    if (msg) LocalFree(msg);
}
