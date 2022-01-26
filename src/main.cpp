#include <wums.h>
#include <cstring>
#include <malloc.h>

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
WUMS_USE_WUT_DEVOPTAB();


WUMS_INITIALIZE() {
    initLogging();
    DEBUG_FUNCTION_LINE("Patch functions");
    // we only patch static functions, we don't need re-patch them and every launch
    FunctionPatcherPatchFunction(fs_file_function_replacements, fs_file_function_replacements_size);
    FunctionPatcherPatchFunction(fs_dir_function_replacements, fs_dir_function_replacements_size);
    FunctionPatcherPatchFunction(rpx_utils_function_replacements, rpx_utils_function_replacements_size);
    DEBUG_FUNCTION_LINE("Patch functions finished");
    gReplacementInfo = {};

    deinitLogging();
}


WUMS_APPLICATION_ENDS() {
    if (gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_FROM_PATH) {
        gReplacementInfo.contentReplacementInfo.mode = CONTENTREDIRECT_NONE;
    }
    if (gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_FROM_WUHB_BUNDLE) {
        if (gReplacementInfo.contentReplacementInfo.bundleMountInformation.isMounted) {
            DEBUG_FUNCTION_LINE("Unmount /vol/content");
            romfsUnmount("rom");
            gReplacementInfo.contentReplacementInfo.bundleMountInformation.isMounted = false;
            DCFlushRange(&gReplacementInfo, sizeof(gReplacementInfo));
        }
    }
    gReplacementInfo.rpxReplacementInfo.isRPXReplaced = false;
    if (gFSClient) {
        FSDelClient(gFSClient, FS_ERROR_FLAG_ALL);
        free(gFSClient);
    }
    free(gFSCmd);

    deinitLogging();
}

WUMS_APPLICATION_STARTS() {
    uint32_t upid = OSGetUPID();
    if (upid != 2 && upid != 15) {
        return;
    }
    initLogging();
    if (gReplacementInfo.rpxReplacementInfo.willRPXBeReplaced) {
        gReplacementInfo.rpxReplacementInfo.willRPXBeReplaced = false;
        gReplacementInfo.rpxReplacementInfo.isRPXReplaced = true;
    }

    if (gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_FROM_PATH) {
        auto fsClient = (FSClient *) memalign(0x20, sizeof(FSClient));
        auto fsCmd = (FSCmdBlock *) memalign(0x20, sizeof(FSCmdBlock));

        if (fsClient == nullptr || fsCmd == nullptr) {
            DEBUG_FUNCTION_LINE("Failed to alloc memory for fsclient or fsCmd");
            free(fsClient);
            free(fsCmd);
        } else {
            auto rc = FSAddClient(fsClient, FS_ERROR_FLAG_ALL);
            if (rc < 0) {
                DEBUG_FUNCTION_LINE("Failed to add FSClient");
            } else {
                FSInitCmdBlock(fsCmd);
                gFSClient = fsClient;
                gFSCmd = fsCmd;
            }
        }
        return;
    }

    if (_SYSGetSystemApplicationTitleId(SYSTEM_APP_ID_HEALTH_AND_SAFETY) != OSGetTitleID()) {
        DEBUG_FUNCTION_LINE("Set mode to CONTENTREDIRECT_NONE and replaceSave to false");
        gReplacementInfo.contentReplacementInfo.mode = CONTENTREDIRECT_NONE;
        gReplacementInfo.contentReplacementInfo.replaceSave = false;
        DCFlushRange(&gReplacementInfo, sizeof(gReplacementInfo));
    } else {
        if (gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_FROM_WUHB_BUNDLE) {
            uint32_t currentHash = StringTools::hash(gReplacementInfo.contentReplacementInfo.bundleMountInformation.toMountPath);

            nn::act::Initialize();
            nn::act::PersistentId slot = nn::act::GetPersistentId();
            nn::act::Finalize();

            std::string basePath = StringTools::strfmt("/vol/external01/wiiu/apps/save/%08X", currentHash);
            std::string common = StringTools::strfmt("fs:/vol/external01/wiiu/apps/save/%08X/common", currentHash);
            std::string user = StringTools::strfmt("fs:/vol/external01/wiiu/apps/save/%08X/%08X", currentHash, 0x80000000 | slot);

            gReplacementInfo.contentReplacementInfo.savePath[0] = '\0';
            strncat(gReplacementInfo.contentReplacementInfo.savePath,
                    basePath.c_str(),
                    sizeof(gReplacementInfo.contentReplacementInfo.savePath) - 1);

            memset(gReplacementInfo.contentReplacementInfo.workingDir, 0, sizeof(gReplacementInfo.contentReplacementInfo.workingDir));

            CreateSubfolder(common.c_str());
            CreateSubfolder(user.c_str());
            DEBUG_FUNCTION_LINE("Created %s and %s", common.c_str(), user.c_str());
            if (romfsMount("rom", gReplacementInfo.contentReplacementInfo.bundleMountInformation.toMountPath, RomfsSource_FileDescriptor_CafeOS) == 0) {
                gReplacementInfo.contentReplacementInfo.bundleMountInformation.mountedPath[0] = '\0';
                strncat(gReplacementInfo.contentReplacementInfo.bundleMountInformation.mountedPath,
                        gReplacementInfo.contentReplacementInfo.bundleMountInformation.toMountPath,
                        sizeof(gReplacementInfo.contentReplacementInfo.bundleMountInformation.mountedPath) - 1);


                gReplacementInfo.contentReplacementInfo.replacementPath[0] = '\0';
                strncat(gReplacementInfo.contentReplacementInfo.replacementPath,
                        "rom:/content",
                        sizeof(gReplacementInfo.contentReplacementInfo.replacementPath) - 1);

                DEBUG_FUNCTION_LINE("Mounted %s to /vol/content", gReplacementInfo.contentReplacementInfo.bundleMountInformation.mountedPath);
                gReplacementInfo.contentReplacementInfo.bundleMountInformation.isMounted = true;
            } else {
                DEBUG_FUNCTION_LINE("Failed to mount %s", gReplacementInfo.contentReplacementInfo.bundleMountInformation.toMountPath);
                gReplacementInfo.contentReplacementInfo.bundleMountInformation.isMounted = false;
            }
        }

        DCFlushRange(&gReplacementInfo, sizeof(gReplacementInfo));
    }
}

