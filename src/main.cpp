#include <wums.h>
#include <cstring>
#include <whb/log_udp.h>
#include <coreinit/debug.h>
#include <coreinit/title.h>
#include <sysapp/title.h>
#include <string>
#include "utils/logger.h"
#include "utils/StringTools.h"
#include "FSFileReplacements.h"
#include "globals.h"
#include "FileUtils.h"
#include "FSDirReplacements.h"
#include "RPXLoading.h"

#include <romfs_dev.h>
#include <coreinit/cache.h>
#include <nn/act.h>

WUMS_MODULE_EXPORT_NAME("homebrew_rpx_loader");

WUMS_INITIALIZE(args) {
    WHBLogUdpInit();
    DEBUG_FUNCTION_LINE("Patch functions");
    // we only patch static functions, we don't need re-patch them and every launch
    FunctionPatcherPatchFunction(fs_file_function_replacements, fs_file_function_replacements_size);
    FunctionPatcherPatchFunction(fs_dir_function_replacements, fs_dir_function_replacements_size);
    FunctionPatcherPatchFunction(rpx_utils_function_replacements, rpx_utils_function_replacements_size);
    DEBUG_FUNCTION_LINE("Patch functions finished");
    gIsMounted = false;
}


WUMS_APPLICATION_ENDS() {
    DEBUG_FUNCTION_LINE("bye bye from rpx loader");
    if (gIsMounted) {
        romfsUnmount("rom");
        gIsMounted = false;
        DCFlushRange(&gIsMounted, sizeof(gIsMounted));
    }
}

WUMS_APPLICATION_STARTS() {
    uint32_t upid = OSGetUPID();
    if (upid != 2 && upid != 15) {
        return;
    }
    WHBLogUdpInit();
    if (_SYSGetSystemApplicationTitleId(SYSTEM_APP_ID_HEALTH_AND_SAFETY) != OSGetTitleID()) {
        DEBUG_FUNCTION_LINE("gTryToReplaceOnNextLaunch, gReplacedRPX and gIsMounted to FALSE");
        gReplacedRPX = false;
        gTryToReplaceOnNextLaunch = false;
        gIsMounted = false;
        DCFlushRange(&gReplacedRPX, sizeof(gReplacedRPX));
        DCFlushRange(&gTryToReplaceOnNextLaunch, sizeof(gTryToReplaceOnNextLaunch));
        DCFlushRange(&gIsMounted, sizeof(gIsMounted));
    } else {
        if (gTryToReplaceOnNextLaunch) {
            gCurrentHash = StringTools::hash(gLoadedBundlePath);

            nn::act::Initialize();
            nn::act::SlotNo slot = nn::act::GetSlotNo();
            nn::act::Finalize();

            std::string basePath = StringTools::strfmt("/vol/external01/wiiu/apps/save/%08X", gCurrentHash);
            std::string common = StringTools::strfmt("fs:/vol/external01/wiiu/apps/save/%08X/common", gCurrentHash);
            std::string user = StringTools::strfmt("fs:/vol/external01/wiiu/apps/save/%08X/%08X", gCurrentHash, 0x80000000 | slot);

            strncpy(gSavePath,basePath.c_str(), 255);
            memset(gWorkingDir, 0, sizeof(gWorkingDir));
            DCFlushRange(gWorkingDir, sizeof(gWorkingDir));

            CreateSubfolder(common.c_str());
            CreateSubfolder(user.c_str());
            DEBUG_FUNCTION_LINE("Created %s and %s", common.c_str(), user.c_str());
            if (romfsMount("rom", gLoadedBundlePath, RomfsSource_FileDescriptor_CafeOS) == 0) {
                DEBUG_FUNCTION_LINE("MOUNTED!");
                gIsMounted = true;
            } else {
                DEBUG_FUNCTION_LINE("MOUNTED FAILED %s", gLoadedBundlePath);
                gIsMounted = false;
            }
            gReplacedRPX = true;
            DCFlushRange(&gReplacedRPX, sizeof(gReplacedRPX));
            DCFlushRange(&gIsMounted, sizeof(gIsMounted));
            DCFlushRange(&gTryToReplaceOnNextLaunch, sizeof(gTryToReplaceOnNextLaunch));
        }
    }
}

