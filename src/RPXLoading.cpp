#include <string>
#include <coreinit/cache.h>
#include <coreinit/ios.h>
#include <romfs_dev.h>
#include "utils/logger.h"
#include "RPXLoading.h"
#include "globals.h"
#include "FileUtils.h"


bool loadRPXFromSDOnNextLaunch(const std::string &path) {
    LOAD_REQUEST request;
    memset(&request, 0, sizeof(request));

    request.command = 0xFC; // IPC_CUSTOM_LOAD_CUSTOM_RPX;
    request.target = 0;     // LOAD_FILE_TARGET_SD_CARD
    request.filesize = 0;   // unknown
    request.fileoffset = 0; //

    romfs_fileInfo info;

    std::string completePath = "/vol/external01/" + path;
    int res = getRPXInfoForPath(completePath, &info);
    bool isBundle = false;
    if (res >= 0) {
        isBundle = true;
        request.filesize = ((uint32_t * ) & info.length)[1];
        request.fileoffset = ((uint32_t * ) & info.offset)[1];
    } else {
        DEBUG_FUNCTION_LINE("not a bundle %s %d", completePath.c_str(), res);
    }

    strncpy(request.path, path.c_str(), 255);

    DEBUG_FUNCTION_LINE("Loading file %s size: %08X offset: %08X", request.path, request.filesize, request.fileoffset);

    DCFlushRange(&request, sizeof(LOAD_REQUEST));

    int mcpFd = IOS_Open("/dev/mcp", (IOSOpenMode) 0);
    if (mcpFd >= 0) {
        int out = 0;
        IOS_Ioctl(mcpFd, 100, &request, sizeof(request), &out, sizeof(out));
        IOS_Close(mcpFd);
    }

    if(isBundle){
        gTryToReplaceOnNextLaunch = true;
        memset(gLoadedBundlePath,0, sizeof(gLoadedBundlePath));
        strncpy(gLoadedBundlePath, completePath.c_str(), completePath.length());
    }else {
        if (!gIsMounted) {
            gTryToReplaceOnNextLaunch = false;
            memset(gLoadedBundlePath, 0, sizeof(gLoadedBundlePath));
        } else {
            // keep the old /vol/content mounted, this way you can reload just the rpx via wiiload
            gTryToReplaceOnNextLaunch = true;
        }
    }

    return true;
}

WUMS_EXPORT_FUNCTION(loadRPXFromSDOnNextLaunch);