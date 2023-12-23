#include "RPXLoading.h"
#include "data/defaultIcon.h"
#include "globals.h"
#include "utils/FileReader.h"
#include "utils/StringTools.h"
#include "utils/ini.h"
#include "utils/logger.h"
#include <coreinit/cache.h>
#include <coreinit/debug.h>
#include <coreinit/ios.h>
#include <coreinit/title.h>
#include <cstring>
#include <mocha/mocha.h>
#include <mutex>
#include <nn/acp/title.h>
#include <nn/save.h>
#include <romfs_dev.h>
#include <rpxloader/rpxloader.h>
#include <string>
#include <sysapp/title.h>
#include <wuhb_utils/utils.h>

std::mutex fileReaderListMutex;
std::forward_list<std::unique_ptr<FileReader>> openFileReaders;

void RPXLoadingCleanUp() {
    const std::lock_guard<std::mutex> lock(fileReaderListMutex);
    openFileReaders.clear();
}

/*
 * Patch the meta xml for the home menu
 */
DECL_FUNCTION(int32_t, HBM_NN_ACP_ACPGetTitleMetaXmlByDevice, uint32_t titleid_upper, uint32_t titleid_lower, ACPMetaXml *metaxml, uint32_t device) {
    if (gReplacementInfo.rpxReplacementInfo.isRPXReplaced) {
        memset(&metaxml->longname_ja, 0, 0x338C - 0x38C); // clear all names

        snprintf(metaxml->longname_en, sizeof(metaxml->longname_en), "%s", gReplacementInfo.rpxReplacementInfo.metaInformation.longname);
        snprintf(metaxml->shortname_en, sizeof(metaxml->shortname_en), "%s", gReplacementInfo.rpxReplacementInfo.metaInformation.shortname);
        snprintf(metaxml->publisher_en, sizeof(metaxml->publisher_en), "%s", gReplacementInfo.rpxReplacementInfo.metaInformation.author);

        // Disable the emanual
        metaxml->e_manual    = 0;
        metaxml->closing_msg = 0;

        return 0;
    }
    int result = real_HBM_NN_ACP_ACPGetTitleMetaXmlByDevice(titleid_upper, titleid_lower, metaxml, device);
    return result;
}

DECL_FUNCTION(int, RPX_FSOpenFile, FSClient *client, FSCmdBlock *block, char *path, const char *mode, uint32_t *handle, int error) {
    const char *iconTex       = "iconTex.tga";
    std::string_view pathView = path;
    if (gReplacementInfo.rpxReplacementInfo.isRPXReplaced && pathView.ends_with(iconTex)) {
        const std::lock_guard<std::mutex> lock(fileReaderListMutex);
        auto reader = make_unique_nothrow<FileReader>(reinterpret_cast<uint8_t *>(gReplacementInfo.rpxReplacementInfo.iconCache), ICON_SIZE);
        if (!reader) {
            DEBUG_FUNCTION_LINE_ERR("Failed to allocate memory for the FileReader");
            return FS_STATUS_FATAL_ERROR;
        }
        *handle = reader->getHandle();
        openFileReaders.push_front(std::move(reader));
        return FS_STATUS_OK;
    }
    int result = real_RPX_FSOpenFile(client, block, path, mode, handle, error);
    return result;
}

DECL_FUNCTION(FSStatus, RPX_FSReadFile, FSClient *client, FSCmdBlock *block, uint8_t *buffer, uint32_t size, uint32_t count, FSFileHandle handle, uint32_t unk1, uint32_t flags) {
    if (gReplacementInfo.rpxReplacementInfo.isRPXReplaced) {
        const std::lock_guard<std::mutex> lock(fileReaderListMutex);
        for (auto &reader : openFileReaders) {
            if ((uint32_t) reader->getHandle() == (uint32_t) handle) {
                return (FSStatus) (reader->read(buffer, size * count) / size);
            }
        }
    }
    return real_RPX_FSReadFile(client, block, buffer, size, count, handle, unk1, flags);
}

DECL_FUNCTION(FSStatus, RPX_FSCloseFile, FSClient *client, FSCmdBlock *block, FSFileHandle handle, uint32_t flags) {
    if (gReplacementInfo.rpxReplacementInfo.isRPXReplaced) {
        if (remove_locked_first_if(fileReaderListMutex, openFileReaders, [handle](auto &cur) { return cur->getHandle() == (uint32_t) handle; })) {
            return FS_STATUS_OK;
        }
    }

    return real_RPX_FSCloseFile(client, block, handle, flags);
}

RPXLoaderStatus RL_PrepareLaunchFromSD(const char *bundle_path);

DECL_FUNCTION(void, OSRestartGame, int argc, char *argv[]) {
    if (OSGetTitleID() == _SYSGetSystemApplicationTitleId(SYSTEM_APP_ID_HEALTH_AND_SAFETY) &&
        strlen(gReplacementInfo.contentReplacementInfo.bundleMountInformation.toMountPath) == 0) {
        RL_PrepareLaunchFromSD(gReplacementInfo.lastFileLoaded);
    }
    real_OSRestartGame(argc, argv);
}

DECL_FUNCTION(void, _SYSLaunchTitleWithStdArgsInNoSplash, uint64_t titleId, void *u1) {
    if (titleId == _SYSGetSystemApplicationTitleId(SYSTEM_APP_ID_HEALTH_AND_SAFETY) &&
        titleId == OSGetTitleID() &&
        strlen(gReplacementInfo.contentReplacementInfo.bundleMountInformation.toMountPath) == 0) {
        RL_PrepareLaunchFromSD(gReplacementInfo.lastFileLoaded);
    }
    real__SYSLaunchTitleWithStdArgsInNoSplash(titleId, u1);
}

DECL_FUNCTION(SAVEStatus, SAVEInitSaveDir, uint8_t slotNo) {
    if (saveLayerHandle != 0) {
        return SAVE_STATUS_OK;
    }
    return real_SAVEInitSaveDir(slotNo);
}

DECL_FUNCTION(SAVEStatus, SAVEInitCommonSaveDir) {
    if (saveLayerHandle != 0) {
        return SAVE_STATUS_OK;
    }
    return real_SAVEInitCommonSaveDir();
}

DECL_FUNCTION(SAVEStatus, SAVEInitAccountSaveDir, uint8_t slotNo) {
    if (saveLayerHandle != 0) {
        return SAVE_STATUS_OK;
    }
    return real_SAVEInitAccountSaveDir(slotNo);
}


DECL_FUNCTION(SAVEStatus, SAVEInitNoDeleteGroupSaveDir) {
    if (saveLayerHandle != 0) {
        return SAVE_STATUS_OK;
    }
    return real_SAVEInitNoDeleteGroupSaveDir();
}

DECL_FUNCTION(SAVEStatus, SAVEUpdateSaveDir) {
    if (saveLayerHandle != 0) {
        return SAVE_STATUS_OK;
    }
    return real_SAVEUpdateSaveDir();
}

function_replacement_data_t rpx_utils_function_replacements[] = {
        REPLACE_FUNCTION_VIA_ADDRESS_FOR_PROCESS(HBM_NN_ACP_ACPGetTitleMetaXmlByDevice, 0x2E36CE44, 0x0E36CE44, FP_TARGET_PROCESS_HOME_MENU),
        REPLACE_FUNCTION_FOR_PROCESS(RPX_FSOpenFile, LIBRARY_COREINIT, FSOpenFile, FP_TARGET_PROCESS_HOME_MENU),
        REPLACE_FUNCTION_FOR_PROCESS(RPX_FSReadFile, LIBRARY_COREINIT, FSReadFile, FP_TARGET_PROCESS_HOME_MENU),
        REPLACE_FUNCTION_FOR_PROCESS(RPX_FSCloseFile, LIBRARY_COREINIT, FSCloseFile, FP_TARGET_PROCESS_HOME_MENU),
        REPLACE_FUNCTION(OSRestartGame, LIBRARY_COREINIT, OSRestartGame),
        REPLACE_FUNCTION(_SYSLaunchTitleWithStdArgsInNoSplash, LIBRARY_SYSAPP, _SYSLaunchTitleWithStdArgsInNoSplash),

        REPLACE_FUNCTION(SAVEInitSaveDir, LIBRARY_NN_SAVE, SAVEInitSaveDir),
        REPLACE_FUNCTION(SAVEInitCommonSaveDir, LIBRARY_NN_SAVE, SAVEInitCommonSaveDir),
        REPLACE_FUNCTION(SAVEInitAccountSaveDir, LIBRARY_NN_SAVE, SAVEInitAccountSaveDir),
        REPLACE_FUNCTION(SAVEInitNoDeleteGroupSaveDir, LIBRARY_NN_SAVE, SAVEInitNoDeleteGroupSaveDir),
        REPLACE_FUNCTION(SAVEUpdateSaveDir, LIBRARY_NN_SAVE, SAVEUpdateSaveDir),
};

uint32_t rpx_utils_function_replacements_size = sizeof(rpx_utils_function_replacements) / sizeof(function_replacement_data_t);

static int parseINIhandler(void *user, const char *section, const char *name,
                           const char *value) {
    auto *fInfo = (MetaInformation *) user;
#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
    if (MATCH("menu", "longname")) {
        fInfo->longname[0] = '\0';
        strncat(fInfo->longname, value, sizeof(fInfo->longname) - 1);
    } else if (MATCH("menu", "shortname")) {
        fInfo->shortname[0] = '\0';
        strncat(fInfo->shortname, value, sizeof(fInfo->shortname) - 1);
    } else if (MATCH("menu", "author")) {
        fInfo->author[0] = '\0';
        strncat(fInfo->author, value, sizeof(fInfo->author) - 1);
    } else {
        return 0; /* unknown section/name, error */
    }

    return 1;
}

RPXLoaderStatus RL_PrepareLaunchFromSD(const char *bundle_path) {
    MochaRPXLoadInfo request;
    memset(&request, 0, sizeof(request));

    request.target     = LOAD_RPX_TARGET_SD_CARD; // LOAD_FILE_TARGET_SD_CARD
    request.filesize   = 0;                       // unknown filesize
    request.fileoffset = 0;                       //

    WUHBRPXInfo fileInfo;

    bool metaLoaded = false;

    std::string completePath       = std::string("/vol/external01/") + bundle_path;
    std::string completeNewLibPath = std::string("fs:/vol/external01/") + bundle_path;

    struct stat st {};
    if (stat(completeNewLibPath.c_str(), &st) < 0) {
        DEBUG_FUNCTION_LINE_ERR("Failed to prepare launch from SD, file (%s) not found.", bundle_path);
        return RPX_LOADER_RESULT_NOT_FOUND;
    }

    bool isBundle = false;
    auto rpxInfo  = WUHBUtils_GetRPXInfo(completePath.c_str(), BundleSource_FileDescriptor_CafeOS, &fileInfo);
    if (rpxInfo == WUHB_UTILS_RESULT_SUCCESS) {
        isBundle           = true;
        request.filesize   = ((uint32_t *) &fileInfo.length)[1];
        request.fileoffset = ((uint32_t *) &fileInfo.offset)[1];
        auto res           = -1;

        if (WUHBUtils_MountBundle("rcc", completePath.c_str(), BundleSource_FileDescriptor_CafeOS, &res) == WUHB_UTILS_RESULT_SUCCESS && res == 0) {
            uint8_t *buffer;
            uint32_t bufferSize;
            if (WUHBUtils_ReadWholeFile("rcc:/meta/meta.ini", &buffer, &bufferSize) == WUHB_UTILS_RESULT_SUCCESS) {
                buffer[bufferSize - 1] = '\0';
                if (ini_parse_string((const char *) buffer, parseINIhandler, &gReplacementInfo.rpxReplacementInfo.metaInformation) < 0) {
                    DEBUG_FUNCTION_LINE_ERR("Failed to load and parse meta.ini");
                } else {
                    metaLoaded = true;
                }
                free(buffer);
            } else {
                DEBUG_FUNCTION_LINE_ERR("Failed to read whole file meta.ini");
            }
            buffer     = nullptr;
            bufferSize = 0;

            if (WUHBUtils_ReadWholeFile("rcc:/meta/iconTex.tga", &buffer, &bufferSize) == WUHB_UTILS_RESULT_SUCCESS) {
                uint32_t cpySize = ICON_SIZE;
                if (bufferSize < cpySize) {
                    cpySize = bufferSize;
                    memset(gReplacementInfo.rpxReplacementInfo.iconCache, 0, ICON_SIZE);
                }
                memcpy(gReplacementInfo.rpxReplacementInfo.iconCache, buffer, cpySize);
                free(buffer);
            } else {
                memcpy(gReplacementInfo.rpxReplacementInfo.iconCache, defaultIconTexTGA, ICON_SIZE);
                DEBUG_FUNCTION_LINE_ERR("Failed to read iconTex.tga");
            }

            auto outRes = 0;
            if (WUHBUtils_UnmountBundle("rcc", &outRes) != WUHB_UTILS_RESULT_SUCCESS || outRes != WUHB_UTILS_RESULT_SUCCESS) {
                DEBUG_FUNCTION_LINE_ERR("Failed to unmount bundle");
            }
        }
    } else {
        if (!gReplacementInfo.contentReplacementInfo.bundleMountInformation.isMounted) {
            memcpy(gReplacementInfo.rpxReplacementInfo.iconCache, defaultIconTexTGA, ICON_SIZE);
        }
    }

    if (!metaLoaded) {
        gReplacementInfo.rpxReplacementInfo.metaInformation.author[0]    = '\0';
        gReplacementInfo.rpxReplacementInfo.metaInformation.shortname[0] = '\0';
        gReplacementInfo.rpxReplacementInfo.metaInformation.longname[0]  = '\0';

        strncat(gReplacementInfo.rpxReplacementInfo.metaInformation.author, bundle_path, sizeof(gReplacementInfo.rpxReplacementInfo.metaInformation.author) - 1);
        strncat(gReplacementInfo.rpxReplacementInfo.metaInformation.shortname, bundle_path, sizeof(gReplacementInfo.rpxReplacementInfo.metaInformation.shortname) - 1);
        strncat(gReplacementInfo.rpxReplacementInfo.metaInformation.longname, bundle_path, sizeof(gReplacementInfo.rpxReplacementInfo.metaInformation.longname) - 1);
    }

    request.path[0] = '\0';
    strncat(request.path, bundle_path, sizeof(request.path) - 1);

    OSMemoryBarrier();
    bool success = false;
    MochaUtilsStatus res;
    if ((res = Mocha_PrepareRPXLaunch(&request)) == MOCHA_RESULT_SUCCESS) {
        success = true;
    } else {
        DEBUG_FUNCTION_LINE_ERR("Failed to prepare rpx launch: %s", Mocha_GetStatusStr(res));
    }

    if (!success) {
        request.target = LOAD_RPX_TARGET_EXTRA_REVERT_PREPARE;
        Mocha_PrepareRPXLaunch(&request);

        gReplacementInfo.rpxReplacementInfo.willRPXBeReplaced = false;
        DEBUG_FUNCTION_LINE_ERR("Failed to load %s on next restart", request.path);
        return RPX_LOADER_RESULT_UNKNOWN_ERROR;
    } else {
        gReplacementInfo.rpxReplacementInfo.willRPXBeReplaced = true;
    }

    DEBUG_FUNCTION_LINE("Launch %s on next restart [size: %08X offset: %08X]", request.path, request.filesize, request.fileoffset);

    gReplacementInfo.contentReplacementInfo.bundleMountInformation.toMountPath[0] = '\0';
    if (isBundle) {
        DEBUG_FUNCTION_LINE_VERBOSE("Loaded file is a .wuhb bundle");
        strncat(gReplacementInfo.contentReplacementInfo.bundleMountInformation.toMountPath,
                completePath.c_str(),
                sizeof(gReplacementInfo.contentReplacementInfo.bundleMountInformation.toMountPath) - 1);
    } else {
        DEBUG_FUNCTION_LINE_VERBOSE("Loaded file is no bundle");
        gReplacementInfo.rpxReplacementInfo.willRPXBeReplaced = true;

        if (gReplacementInfo.contentReplacementInfo.bundleMountInformation.isMounted) {
            // keep the old /vol/content mounted, this way you can reload just the rpx via wiiload
            strncat(gReplacementInfo.contentReplacementInfo.bundleMountInformation.toMountPath,
                    gReplacementInfo.contentReplacementInfo.bundleMountInformation.mountedPath,
                    sizeof(gReplacementInfo.contentReplacementInfo.bundleMountInformation.toMountPath) - 2);
        }
    }

    strncpy(gReplacementInfo.lastFileLoaded, bundle_path, sizeof(gReplacementInfo.lastFileLoaded) - 2);

    OSMemoryBarrier();

    return RPX_LOADER_RESULT_SUCCESS;
}

RPXLoaderStatus RL_LaunchPreparedHomebrew() {
    // Request to launch the homebrew wrapper (H&S or Daily Log)
    auto res = Mocha_LaunchHomebrewWrapper();
    if (res != MOCHA_RESULT_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("Launch failed: %s", Mocha_GetStatusStr(res));
    }
    if (res == MOCHA_RESULT_NOT_FOUND) {
        return RPX_LOADER_RESULT_NOT_FOUND;
    } else if (res != MOCHA_RESULT_SUCCESS) {
        return RPX_LOADER_RESULT_UNKNOWN_ERROR;
    }
    return RPX_LOADER_RESULT_SUCCESS;
}

RPXLoaderStatus RL_LaunchHomebrew(const char *bundle_path) {
    RPXLoaderStatus res = RPX_LOADER_RESULT_SUCCESS;
    // Tell iosu which .rpx to launch and prepare the content redirection.
    if ((res = RL_PrepareLaunchFromSD(bundle_path)) != RPX_LOADER_RESULT_SUCCESS) {
        return res;
    }
    // Request to launch the homebrew wrapper (H&S or Daily Log)
    if ((res = RL_LaunchPreparedHomebrew()) != RPX_LOADER_RESULT_SUCCESS) {
        // If we fail to launch the correct application, we revert the .rpx redirection.
        MochaRPXLoadInfo loadInfoRevert;
        loadInfoRevert.target = LOAD_RPX_TARGET_EXTRA_REVERT_PREPARE;

        if (MochaUtilsStatus mochaRes = Mocha_PrepareRPXLaunch(&loadInfoRevert); mochaRes != MOCHA_RESULT_SUCCESS) {
            DEBUG_FUNCTION_LINE_WARN("Revert Mocha_PrepareRPXLaunch failed: %s [%d]", Mocha_GetStatusStr(mochaRes), mochaRes);
        }
        return res;
    }

    return res;
}

std::mutex mutex;

RPXLoaderStatus RL_DisableContentRedirection() {
    std::lock_guard<std::mutex> lock(mutex);
    if (contentLayerHandle != 0) {
        if (ContentRedirection_SetActive(contentLayerHandle, false) != CONTENT_REDIRECTION_RESULT_SUCCESS) {
            return RPX_LOADER_RESULT_UNKNOWN_ERROR;
        }
    }
    if (saveLayerHandle != 0) {
        if (ContentRedirection_SetActive(saveLayerHandle, false) != CONTENT_REDIRECTION_RESULT_SUCCESS) {
            ContentRedirection_SetActive(contentLayerHandle, true);
            return RPX_LOADER_RESULT_UNKNOWN_ERROR;
        }
    }
    return RPX_LOADER_RESULT_SUCCESS;
}

RPXLoaderStatus RL_EnableContentRedirection() {
    std::lock_guard<std::mutex> lock(mutex);
    if (contentLayerHandle != 0) {
        if (ContentRedirection_SetActive(contentLayerHandle, true) != CONTENT_REDIRECTION_RESULT_SUCCESS) {
            return RPX_LOADER_RESULT_UNKNOWN_ERROR;
        }
    }
    if (saveLayerHandle != 0) {
        if (ContentRedirection_SetActive(saveLayerHandle, true) != CONTENT_REDIRECTION_RESULT_SUCCESS) {
            ContentRedirection_SetActive(contentLayerHandle, false);
            return RPX_LOADER_RESULT_UNKNOWN_ERROR;
        }
    }
    return RPX_LOADER_RESULT_SUCCESS;
}

RPXLoaderStatus RL_UnmountCurrentRunningBundle() {
    std::lock_guard<std::mutex> lock(mutex);
    if (gReplacementInfo.contentReplacementInfo.bundleMountInformation.isMounted == false) {
        return RPX_LOADER_RESULT_SUCCESS;
    }

    int outRes = -1;
    if (ContentRedirection_RemoveDevice(WUHB_ROMFS_PATH, &outRes) == CONTENT_REDIRECTION_RESULT_SUCCESS) {
        if (outRes < 0) {
            DEBUG_FUNCTION_LINE_ERR("RemoveDevice \"%s\" failed for ContentRedirection Module", WUHB_ROMFS_NAME);
            OSFatal("RL_UnmountCurrentRunningBundle: RemoveDevice \"" WUHB_ROMFS_NAME "\" failed for ContentRedirection Module");
        }
    } else {
        DEBUG_FUNCTION_LINE_ERR("ContentRedirection_RemoveDevice failed");
        OSFatal("RL_UnmountCurrentRunningBundle: ContentRedirection_RemoveDevice failed");
    }

    RPXLoaderStatus res = RPX_LOADER_RESULT_SUCCESS;

    if (contentLayerHandle != 0) {
        if (ContentRedirection_RemoveFSLayer(contentLayerHandle) != CONTENT_REDIRECTION_RESULT_SUCCESS) {
            res = RPX_LOADER_RESULT_UNKNOWN_ERROR;
        }
        contentLayerHandle = 0;
    }

    if (saveLayerHandle != 0) {
        if (ContentRedirection_RemoveFSLayer(saveLayerHandle) != CONTENT_REDIRECTION_RESULT_SUCCESS) {
            res = RPX_LOADER_RESULT_UNKNOWN_ERROR;
        }
        saveLayerHandle = 0;
    }

    if (romfsUnmount(WUHB_ROMFS_NAME) < 0) {
        DEBUG_FUNCTION_LINE_WARN("Failed to unmount romfs \"%s\"", WUHB_ROMFS_NAME);
    }
    gReplacementInfo.contentReplacementInfo.bundleMountInformation.isMounted = false;
    OSMemoryBarrier();
    return res;
}

RPXLoaderStatus RL_GetVersion(RPXLoaderVersion *outVersion) {
    if (!outVersion) {
        return RPX_LOADER_RESULT_INVALID_ARGUMENT;
    }
    *outVersion = 2;
    return RPX_LOADER_RESULT_SUCCESS;
}

RPXLoaderStatus RL_GetPathOfRunningExecutable(char *outBuffer, uint32_t outSize) {
    if (outBuffer == nullptr || outSize == 0) {
        return RPX_LOADER_RESULT_INVALID_ARGUMENT;
    }
    if (strlen(gReplacementInfo.lastFileLoaded) > 0) {
        strncpy(outBuffer, gReplacementInfo.lastFileLoaded, outSize - 1);
        return RPX_LOADER_RESULT_SUCCESS;
    }

    return RPX_LOADER_RESULT_NOT_AVAILABLE;
}

WUMS_EXPORT_FUNCTION(RL_PrepareLaunchFromSD);
WUMS_EXPORT_FUNCTION(RL_LaunchPreparedHomebrew);
WUMS_EXPORT_FUNCTION(RL_LaunchHomebrew);

WUMS_EXPORT_FUNCTION(RL_GetVersion);
WUMS_EXPORT_FUNCTION(RL_EnableContentRedirection);
WUMS_EXPORT_FUNCTION(RL_DisableContentRedirection);
WUMS_EXPORT_FUNCTION(RL_UnmountCurrentRunningBundle);
WUMS_EXPORT_FUNCTION(RL_GetPathOfRunningExecutable);
