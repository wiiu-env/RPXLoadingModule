#include "utils/logger.h"
#include <string.h>
#include <string>
#include <whb/log.h>

std::string sanitizeName(const std::string &input) {
    if (input.empty() || input.starts_with(' ')) {
        return "";
    }
    std::string result       = input;
    std::string illegalChars = "\\/:?\"<>|@=;`_^][";
    for (auto it = result.begin(); it < result.end(); ++it) {
        if (*it < '0' || *it > 'z') {
            *it = ' ';
        }
    }
    for (auto it = result.begin(); it < result.end(); ++it) {
        bool found = illegalChars.find(*it) != std::string::npos;
        if (found) {
            *it = ' ';
        }
    }
    uint32_t length = result.length();
    for (uint32_t i = 1; i < length; ++i) {
        if (result[i - 1] == ' ' && result[i] == ' ') {
            result.erase(i, 1);
            i--;
            length--;
        }
    }
    if (result.size() == 1 && result[0] == ' ') {
        result.clear();
    }
    return result;
}


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
                    DEBUG_FUNCTION_LINE("0x%08X (0x%04X); ", ((uint32_t) data) + i + 1, i + 1);
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