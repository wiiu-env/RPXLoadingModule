#pragma once

#include <cstdint>
#include <function_patcher/function_patching.h>
#include <rpxloader/rpxloader.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct __attribute((packed)) {
    uint32_t command;
    uint32_t target;
    uint32_t filesize;
    uint32_t fileoffset;
    char path[256];
} LOAD_REQUEST;

extern function_replacement_data_t rpx_utils_function_replacements[];
extern uint32_t rpx_utils_function_replacements_size;

void RPXLoadingCleanUp();
RPXLoaderStatus RL_UnmountCurrentRunningBundle();

#ifdef __cplusplus
}
#endif
