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

WUMS_INITIALIZE() {
    WHBLogUdpInit();
    DEBUG_FUNCTION_LINE("Patch functions");
    // we only patch static functions, we don't need re-patch them and every launch
    FunctionPatcherPatchFunction(fs_file_function_replacements, fs_file_function_replacements_size);
    FunctionPatcherPatchFunction(fs_dir_function_replacements, fs_dir_function_replacements_size);
    FunctionPatcherPatchFunction(rpx_utils_function_replacements, rpx_utils_function_replacements_size);
    DEBUG_FUNCTION_LINE("Patch functions finished");
    gReplacementInfo = {};
}


WUMS_APPLICATION_ENDS() {
    if (gReplacementInfo.bundleMountInformation.isMounted) {
        DEBUG_FUNCTION_LINE("Unmount /vol/content");
        romfsUnmount("rom");
        gReplacementInfo.bundleMountInformation.isMounted = false;
        DCFlushRange(&gReplacementInfo, sizeof(gReplacementInfo));
    }
}

WUMS_APPLICATION_STARTS() {
    uint32_t upid = OSGetUPID();
    if (upid != 2 && upid != 15) {
        return;
    }
    WHBLogUdpInit();
    if (_SYSGetSystemApplicationTitleId(SYSTEM_APP_ID_HEALTH_AND_SAFETY) != OSGetTitleID()) {
        DEBUG_FUNCTION_LINE("Set gTryToReplaceOnNextLaunch, gReplacedRPX and gIsMounted to FALSE");
        gReplacementInfo.replacementType = RPXLoader_NONE;
        gReplacementInfo.isRPXReplaced = false;
        gReplacementInfo.bundleMountInformation.isMounted = false;
        gReplacementInfo.bundleMountInformation.redirectionRequested = false;
        DCFlushRange(&gReplacementInfo, sizeof(gReplacementInfo));
    } else {
        if (gReplacementInfo.replacementType != RPXLoader_NONE) {
            gReplacementInfo.isRPXReplaced =  true;
        }
        if (gReplacementInfo.bundleMountInformation.redirectionRequested) {
            if (gReplacementInfo.replacementType == RPXLoader_BUNDLE || gReplacementInfo.replacementType == RPXLoader_BUNDLE_OTHER_RPX) {
                gReplacementInfo.currentHash = StringTools::hash(gReplacementInfo.bundleMountInformation.path);

                nn::act::Initialize();
                nn::act::SlotNo slot = nn::act::GetSlotNo();
                nn::act::Finalize();

                std::string basePath = StringTools::strfmt("/vol/external01/wiiu/apps/save/%08X", gReplacementInfo.currentHash);
                std::string common = StringTools::strfmt("fs:/vol/external01/wiiu/apps/save/%08X/common", gReplacementInfo.currentHash);
                std::string user = StringTools::strfmt("fs:/vol/external01/wiiu/apps/save/%08X/%08X", gReplacementInfo.currentHash, 0x80000000 | slot);

                strncpy(gReplacementInfo.savePath, basePath.c_str(), sizeof(gReplacementInfo.savePath));
                memset(gReplacementInfo.bundleMountInformation.workingDir, 0, sizeof(gReplacementInfo.bundleMountInformation.workingDir));

                CreateSubfolder(common.c_str());
                CreateSubfolder(user.c_str());
                DEBUG_FUNCTION_LINE("Created %s and %s", common.c_str(), user.c_str());
                if (romfsMount("rom", gReplacementInfo.bundleMountInformation.path, RomfsSource_FileDescriptor_CafeOS) == 0) {
                    DEBUG_FUNCTION_LINE("Mounted %s to /vol/content", gReplacementInfo.bundleMountInformation.path);
                    gReplacementInfo.bundleMountInformation.isMounted = true;
                } else {
                    DEBUG_FUNCTION_LINE("Failed to mount %s", gReplacementInfo.bundleMountInformation.path);
                    gReplacementInfo.bundleMountInformation.isMounted = false;
                }
            } else if (gReplacementInfo.replacementType == RPXLoader_RPX) {
                //
            }
        }
        DCFlushRange(&gReplacementInfo, sizeof(gReplacementInfo));
    }
}

