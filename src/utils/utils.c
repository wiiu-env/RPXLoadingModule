#include <string.h>
#include <stddef.h>
#include <whb/log.h>
#include <stdarg.h>
#include <stdio.h>
#include <coreinit/debug.h>
#include "utils/logger.h"

#define PRINTF_BUFFER_LENGTH 2048

// https://gist.github.com/ccbrown/9722406
void dumpHex(const void *data, size_t size) {
    char ascii[17];
    size_t i, j;
    ascii[16] = '\0';
    DEBUG_FUNCTION_LINE("0x%08X (0x0000): ", data);
    for (i = 0; i < size; ++i) {
        WHBLogWritef("%02X ", ((unsigned char *) data)[i]);
        if (((unsigned char *) data)[i] >= ' ' && ((unsigned char *) data)[i] <= '~') {
            ascii[i % 16] = ((unsigned char *) data)[i];
        } else {
            ascii[i % 16] = '.';
        }
        if ((i + 1) % 8 == 0 || i + 1 == size) {
            WHBLogWritef(" ");
            if ((i + 1) % 16 == 0) {
                WHBLogPrintf("|  %s ", ascii);
                if (i + 1 < size) {
                    DEBUG_FUNCTION_LINE("0x%08X (0x%04X); ", data + i + 1, i + 1);
                }
            } else if (i + 1 == size) {
                ascii[(i + 1) % 16] = '\0';
                if ((i + 1) % 16 <= 8) {
                    WHBLogWritef(" ");
                }
                for (j = (i + 1) % 16; j < 16; ++j) {
                    WHBLogWritef("   ");
                }
                WHBLogPrintf("|  %s ", ascii);
            }
        }
    }
}

BOOL OSFatal_printf(const char *fmt, ...) {
    char *buf1 = memalign(4,PRINTF_BUFFER_LENGTH);
    char *buf2 = memalign(4,PRINTF_BUFFER_LENGTH);
    va_list va;

    if (!buf1) {
        return FALSE;
    }

    if (!buf2) {
        free(buf1);
        return FALSE;
    }

    va_start(va, fmt);

    vsnprintf(buf1, PRINTF_BUFFER_LENGTH, fmt, va);
    snprintf(buf2, PRINTF_BUFFER_LENGTH, "%s\n", buf1);
    OSFatal(buf2);

    free(buf1);
    free(buf2);
    va_end(va);
    return TRUE;
}