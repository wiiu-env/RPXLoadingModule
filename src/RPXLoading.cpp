#include <string>
#include <coreinit/cache.h>
#include <coreinit/ios.h>
#include <romfs_dev.h>
#include "utils/logger.h"
#include "RPXLoading.h"
#include "globals.h"
#include "FileUtils.h"
#include <nn/acp/title.h>
#include <memory>
#include <algorithm>
#include "utils/FileReader.h"
#include "utils/FileReaderCompressed.h"
#include "utils/StringTools.h"
#include "utils/ini.h"
#include <cstring>
#include <rpxloader.h>


/*
 * Patch the meta xml for the home menu
 */
DECL_FUNCTION(int32_t, HBM_NN_ACP_ACPGetTitleMetaXmlByDevice, uint32_t titleid_upper, uint32_t titleid_lower, ACPMetaXml *metaxml, uint32_t device) {
    if (gReplacementInfo.rpxReplacementInfo.isRPXReplaced) {
        memset(&metaxml->longname_ja, 0, 0x338C - 0x38C); // clear all names

        snprintf(metaxml->longname_en, sizeof(metaxml->longname_en), "%s", gReplacementInfo.rpxReplacementInfo.metaInformation.longname);
        snprintf(metaxml->shortname_en, sizeof(metaxml->shortname_en), "%s", gReplacementInfo.rpxReplacementInfo.metaInformation.longname);
        snprintf(metaxml->publisher_en, sizeof(metaxml->publisher_en), "%s", gReplacementInfo.rpxReplacementInfo.metaInformation.longname);

        // Disbale the emanual
        metaxml->e_manual = 0;

        return 0;
    }
    int result = real_HBM_NN_ACP_ACPGetTitleMetaXmlByDevice(titleid_upper, titleid_lower, metaxml, device);
    return result;
}

DECL_FUNCTION(int, RPX_FSOpenFile, FSClient *client, FSCmdBlock *block, char *path, const char *mode, int *handle, int error) {
    const char *iconTex = "iconTex.tga";
    if (StringTools::EndsWith(path, iconTex)) {
        if (gReplacementInfo.rpxReplacementInfo.isRPXReplaced) {
            if (StringTools::EndsWith(path, iconTex)) {
                auto *reader = new FileReader(reinterpret_cast<uint8_t *>(gReplacementInfo.rpxReplacementInfo.iconCache), sizeof(gReplacementInfo.rpxReplacementInfo.iconCache));
                *handle = (uint32_t) reader;
                return FS_STATUS_OK;
            }
        }
    }
    int result = real_RPX_FSOpenFile(client, block, path, mode, handle, error);
    return result;
}

DECL_FUNCTION(FSStatus, RPX_FSReadFile, FSClient *client, FSCmdBlock *block, uint8_t *buffer, uint32_t size, uint32_t count, FSFileHandle handle, uint32_t unk1, uint32_t flags) {
    // We check if the handle is part of our heap (the MemoryMapping Module allocates to 0x80000000)
    if ((handle & 0xF0000000) == 0x80000000) {
        auto reader = (FileReader *) handle;
        return (FSStatus) reader->read(buffer, size * count);

    }
    FSStatus result = real_RPX_FSReadFile(client, block, buffer, size, count, handle, unk1, flags);
    return result;
}

DECL_FUNCTION(FSStatus, RPX_FSCloseFile, FSClient *client, FSCmdBlock *block, FSFileHandle handle, uint32_t flags) {
    // We check if the handle is part of our heap (the MemoryMapping Module allocates to 0x80000000)
    if ((handle & 0xF0000000) == 0x80000000) {
        auto reader = (FileReader *) handle;
        delete reader;
        return FS_STATUS_OK;
    }

    return real_RPX_FSCloseFile(client, block, handle, flags);
}

DECL_FUNCTION(void, Loader_ReportWarn) {
   return;
}

function_replacement_data_t rpx_utils_function_replacements[] = {
        REPLACE_FUNCTION_VIA_ADDRESS(Loader_ReportWarn, 0x32002f74, 0x01002f74),
        REPLACE_FUNCTION_VIA_ADDRESS_FOR_PROCESS(HBM_NN_ACP_ACPGetTitleMetaXmlByDevice, 0x2E36CE44, 0x0E36CE44, FP_TARGET_PROCESS_HOME_MENU),
        REPLACE_FUNCTION_FOR_PROCESS(RPX_FSOpenFile, LIBRARY_COREINIT, FSOpenFile, FP_TARGET_PROCESS_HOME_MENU),
        REPLACE_FUNCTION_FOR_PROCESS(RPX_FSReadFile, LIBRARY_COREINIT, FSReadFile, FP_TARGET_PROCESS_HOME_MENU),
        REPLACE_FUNCTION_FOR_PROCESS(RPX_FSCloseFile, LIBRARY_COREINIT, FSCloseFile, FP_TARGET_PROCESS_HOME_MENU),
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
        return 0;  /* unknown section/name, error */
    }

    return 1;
}

bool RL_LoadFromSDOnNextLaunch(const char *bundle_path) {
    LOAD_REQUEST request;
    memset(&request, 0, sizeof(request));

    request.command = 0xFC; // IPC_CUSTOM_LOAD_CUSTOM_RPX;
    request.target = 0;     // LOAD_FILE_TARGET_SD_CARD
    request.filesize = 0;   // unknown filesize
    request.fileoffset = 0; //

    romfs_fileInfo info;

    bool metaLoaded = false;

    std::string completePath = std::string("/vol/external01/") + bundle_path;
    int res = getRPXInfoForPath(completePath, &info);
    bool isBundle = false;
    if (res >= 0) {
        isBundle = true;
        request.filesize = ((uint32_t *) &info.length)[1];
        request.fileoffset = ((uint32_t *) &info.offset)[1];

        if (romfsMount("rcc", completePath.c_str(), RomfsSource_FileDescriptor_CafeOS) == 0) {
            if (ini_parse("rcc:/meta/meta.ini", parseINIhandler, &gReplacementInfo.rpxReplacementInfo.metaInformation) < 0) {
                DEBUG_FUNCTION_LINE("Failed to load and parse meta.ini");
            } else {
                metaLoaded = true;
            }
            FileReader *reader = nullptr;

            if (CheckFile("rcc:/meta/iconTex.tga")) {
                std::string path = "rcc:/meta/iconTex.tga";
                reader = new FileReader(path);
            } else if (CheckFile("rcc:/meta/iconTex.tga.gz")) {
                std::string path = "rcc:/meta/iconTex.tga.gz";
                reader = new FileReaderCompressed(path);
            }
            if (reader) {
                uint32_t alreadyRead = 0;
                uint32_t toRead = sizeof(gReplacementInfo.rpxReplacementInfo.iconCache);
                do {
                    int read = reader->read(reinterpret_cast<uint8_t *>(&gReplacementInfo.rpxReplacementInfo.iconCache[alreadyRead]), toRead);
                    if (read <= 0) {
                        break;
                    }
                    alreadyRead += read;
                    toRead -= read;
                } while (alreadyRead < sizeof(gReplacementInfo.rpxReplacementInfo.iconCache));
                delete reader;
            } else {
                memset(gReplacementInfo.rpxReplacementInfo.iconCache, 0, sizeof(gReplacementInfo.rpxReplacementInfo.iconCache));
            }
            romfsUnmount("rcc");
        }
    } else {
        if (!(gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_FROM_WUHB_BUNDLE &&
              gReplacementInfo.contentReplacementInfo.bundleMountInformation.isMounted)) {
            memset(gReplacementInfo.rpxReplacementInfo.iconCache, 0, sizeof(gReplacementInfo.rpxReplacementInfo.iconCache));
        }
    }

    if (!metaLoaded) {
        gReplacementInfo.rpxReplacementInfo.metaInformation.author[0] = '\0';
        gReplacementInfo.rpxReplacementInfo.metaInformation.shortname[0] = '\0';
        gReplacementInfo.rpxReplacementInfo.metaInformation.longname[0] = '\0';

        strncat(gReplacementInfo.rpxReplacementInfo.metaInformation.author, bundle_path, sizeof(gReplacementInfo.rpxReplacementInfo.metaInformation.author) - 1);
        strncat(gReplacementInfo.rpxReplacementInfo.metaInformation.shortname, bundle_path, sizeof(gReplacementInfo.rpxReplacementInfo.metaInformation.shortname) - 1);
        strncat(gReplacementInfo.rpxReplacementInfo.metaInformation.longname, bundle_path, sizeof(gReplacementInfo.rpxReplacementInfo.metaInformation.longname) - 1);
    }

    request.path[0] = '\0';
    strncat(request.path, bundle_path, sizeof(request.path) - 1);

    DCFlushRange(&request, sizeof(LOAD_REQUEST));

    int success = false;
    int mcpFd = IOS_Open("/dev/mcp", (IOSOpenMode) 0);
    if (mcpFd >= 0) {
        int out = 0;
        IOS_Ioctl(mcpFd, 100, &request, sizeof(request), &out, sizeof(out));
        if (out > 0) {
            success = true;
        }

        IOS_Close(mcpFd);
    }

    DCFlushRange(&gReplacementInfo, sizeof(gReplacementInfo));

    if (!success) {
        gReplacementInfo.rpxReplacementInfo.willRPXBeReplaced = false;
        DEBUG_FUNCTION_LINE("Failed to load %s on next restart", request.path);
        return false;
    } else {
        gReplacementInfo.rpxReplacementInfo.willRPXBeReplaced = true;
    }

    DEBUG_FUNCTION_LINE("Launch %s on next restart [size: %08X offset: %08X]", request.path, request.filesize, request.fileoffset);

    if (isBundle) {
        DEBUG_FUNCTION_LINE("Loaded file is a .wuhb bundle");
        gReplacementInfo.contentReplacementInfo.bundleMountInformation.toMountPath[0] = '\0';
        strncat(gReplacementInfo.contentReplacementInfo.bundleMountInformation.toMountPath,
                completePath.c_str(),
                sizeof(gReplacementInfo.contentReplacementInfo.bundleMountInformation.toMountPath) - 1);
        gReplacementInfo.contentReplacementInfo.mode = CONTENTREDIRECT_FROM_WUHB_BUNDLE;
    } else {
        DEBUG_FUNCTION_LINE("Loaded file is no bundle");
        gReplacementInfo.rpxReplacementInfo.willRPXBeReplaced = true;

        if (gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_FROM_WUHB_BUNDLE &&
            gReplacementInfo.contentReplacementInfo.bundleMountInformation.isMounted) {
            // keep the old /vol/content mounted, this way you can reload just the rpx via wiiload
            gReplacementInfo.contentReplacementInfo.bundleMountInformation.toMountPath[0] = '\0';
            strncat(gReplacementInfo.contentReplacementInfo.bundleMountInformation.toMountPath,
                    gReplacementInfo.contentReplacementInfo.bundleMountInformation.mountedPath,
                    sizeof(gReplacementInfo.contentReplacementInfo.bundleMountInformation.toMountPath) - 1);
        }
    }

    DCFlushRange(&gReplacementInfo, sizeof(gReplacementInfo));

    return true;
}

int32_t RL_MountBundle(const char *name, const char *path, BundleSource source) {
    return romfsMount(name, path, (RomfsSource) source);
}

int32_t RL_UnmountBundle(const char *name) {
    return romfsUnmount(name);
}

int32_t RL_FileOpen(const char *name, uint32_t *handle) {
    if (handle == nullptr) {
        return -1;
    }

    FileReader *reader = nullptr;
    std::string path = std::string(name);
    std::string pathGZ = path + ".gz";

    if (CheckFile(path.c_str())) {
        reader = new FileReader(path);
    } else if (CheckFile(pathGZ.c_str())) {
        reader = new FileReaderCompressed(pathGZ);
    } else {
        return -2;
    }
    if (reader == nullptr) {
        return -3;
    }
    *handle = (uint32_t) reader;
    return 0;
}

int32_t RL_FileRead(uint32_t handle, uint8_t *buffer, uint32_t size) {
    auto reader = (FileReader *) handle;
    return reader->read(buffer, size);
}

int32_t RL_FileClose(uint32_t handle) {
    auto reader = (FileReader *) handle;
    delete reader;
    return 0;
}

bool RL_FileExists(const char *name) {
    std::string checkgz = std::string(name) + ".gz";
    return CheckFile(name) || CheckFile(checkgz.c_str());
}

bool RL_RedirectContentWithFallback(const char * newContentPath) {
    auto dirHandle = opendir(newContentPath);
    if (dirHandle == nullptr) {
        return false;
    }
    closedir(dirHandle);

    gReplacementInfo.contentReplacementInfo.replacementPath[0] = '\0';
    strncat(gReplacementInfo.contentReplacementInfo.replacementPath, newContentPath, sizeof(gReplacementInfo.contentReplacementInfo.replacementPath) - 1);
    gReplacementInfo.contentReplacementInfo.mode = CONTENTREDIRECT_FROM_PATH;
    gReplacementInfo.contentReplacementInfo.fallbackOnError = true;
    gReplacementInfo.contentReplacementInfo.replaceSave = false;
    return true;
}

bool RL_DisableContentRedirection() {
    if (gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_FROM_PATH) {
        gReplacementInfo.contentReplacementInfo.mode = CONTENTREDIRECT_NONE;
        return true;
    }
    return false;
}

WUMS_EXPORT_FUNCTION(RL_LoadFromSDOnNextLaunch);
WUMS_EXPORT_FUNCTION(RL_MountBundle);
WUMS_EXPORT_FUNCTION(RL_UnmountBundle);
WUMS_EXPORT_FUNCTION(RL_FileOpen);
WUMS_EXPORT_FUNCTION(RL_FileRead);
WUMS_EXPORT_FUNCTION(RL_FileClose);
WUMS_EXPORT_FUNCTION(RL_FileExists);
WUMS_EXPORT_FUNCTION(RL_RedirectContentWithFallback);
WUMS_EXPORT_FUNCTION(RL_DisableContentRedirection);