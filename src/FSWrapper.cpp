#include "FSWrapper.h"
#include "FileUtils.h"
#include "globals.h"
#include "utils/logger.h"
#include <coreinit/cache.h>
#include <coreinit/debug.h>
#include <cstring>
#include <fcntl.h>
#include <mutex>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

dirMagic_t dir_handles[DIR_HANDLES_LENGTH];
fileMagic_t file_handles[FILE_HANDLES_LENGTH];

std::mutex dir_handle_mutex;
std::mutex file_handle_mutex;

static inline void replaceContentPath(char *pathForCheck, size_t pathForCheckSize, int replaceLen, char *replaceWith);

inline void getFullPath(char *pathForCheck, int pathSize, char *path) {
    if (path[0] != '/' && path[0] != '\\') {
        snprintf(pathForCheck, pathSize, "%s%s", gReplacementInfo.contentReplacementInfo.workingDir, path);
        DEBUG_FUNCTION_LINE_VERBOSE("Real path is %s", pathForCheck);
    } else {
        pathForCheck[0] = '\0';
        strncat(pathForCheck, path, pathSize - 1);
    }

    for (size_t i = 0; i < strlen(pathForCheck); i++) {
        if (pathForCheck[i] == '\\') {
            pathForCheck[i] = '/';
        }
    }
}

inline bool checkForSave(char *pathForCheck, int pathSize, char *path) {
    if (!gReplacementInfo.contentReplacementInfo.replaceSave) {
        return false;
    }
    if (strncmp(path, "/vol/save", 9) == 0) {
        int copyLen = strlen(path);
        char copy[copyLen + 1];
        memcpy(copy, path, copyLen);
        copy[copyLen] = 0;
        memset(pathForCheck, 0, pathSize);
        snprintf(pathForCheck, pathSize, "%s%s", gReplacementInfo.contentReplacementInfo.savePath, &copy[9]);
        return true;
    }
    return false;
}

int getNewFileHandleIndex() {
    file_handle_mutex.lock();
    int32_t handle_id = -1;
    for (int i = 0; i < FILE_HANDLES_LENGTH; i++) {
        if (!file_handles[i].in_use) {
            handle_id = i;
            if (!file_handles[i].mutex) {
                file_handles[i].mutex = (OSMutex *) malloc(sizeof(OSMutex));
                if (!file_handles[i].mutex) {
                    OSFatal("Failed to alloc memory for mutex");
                }
                OSInitMutex(file_handles[i].mutex);
                DCFlushRange(file_handles[i].mutex, sizeof(OSMutex));
                DCFlushRange(&file_handles[i], sizeof(fileMagic_t));
            }
            break;
        }
    }
    file_handle_mutex.unlock();
    return handle_id;
}

bool isValidDirHandle(uint32_t handle) {
    return (handle & HANDLE_INDICATOR_MASK) == DIR_HANDLE_MAGIC;
}

bool isValidFileHandle(uint32_t handle) {
    return (handle & HANDLE_INDICATOR_MASK) == FILE_HANDLE_MAGIC;
}

int32_t getNewDirHandleIndex() {
    dir_handle_mutex.lock();
    int32_t handle_id = -1;
    for (int i = 0; i < DIR_HANDLES_LENGTH; i++) {
        if (!dir_handles[i].in_use) {
            handle_id = i;
            if (!dir_handles[i].mutex) {
                dir_handles[i].mutex = (OSMutex *) malloc(sizeof(OSMutex));
                OSInitMutex(dir_handles[i].mutex);
                if (!dir_handles[i].mutex) {
                    OSFatal("Failed to alloc memory for mutex");
                }
                DCFlushRange(dir_handles[i].mutex, sizeof(OSMutex));
                DCFlushRange(&dir_handles[i], sizeof(dirMagic_t));
            }
            break;
        }
    }
    dir_handle_mutex.unlock();
    return handle_id;
}

void freeFileHandle(uint32_t handle) {
    if (handle >= FILE_HANDLES_LENGTH) {
        DEBUG_FUNCTION_LINE("Invalid handle");
        return;
    }
    file_handle_mutex.lock();
    file_handles[handle].in_use = false;
    if (file_handles[handle].mutex) {
        free(file_handles[handle].mutex);
        file_handles[handle].mutex = nullptr;
    }
    DCFlushRange(&file_handles[handle], sizeof(fileMagic_t));
    file_handle_mutex.unlock();
}

void freeDirHandle(uint32_t handle) {
    if (handle >= DIR_HANDLES_LENGTH) {
        DEBUG_FUNCTION_LINE("Invalid handle");
        return;
    }
    dir_handle_mutex.lock();
    dir_handles[handle].in_use = false;
    if (dir_handles[handle].mutex) {
        free(dir_handles[handle].mutex);
        dir_handles[handle].mutex = nullptr;
    }
    dir_handles[handle] = {};
    DCFlushRange(&dir_handles[handle], sizeof(dirMagic_t));
    dir_handle_mutex.unlock();
}

extern "C" FSStatus (*real_FSOpenDir)(FSClient *, FSCmdBlock *, char *, FSDirectoryHandle *, FSErrorFlag);

FSStatus FSOpenDirWrapper(char *path,
                          FSDirectoryHandle *handle,
                          FSErrorFlag errorMask,
                          const std::function<FSStatus(char *)> &fallback_function,
                          const std::function<FSStatus(FSStatus)> &result_handler) {
    if ((gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_NONE) ||
        (gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_FROM_WUHB_BUNDLE && !gReplacementInfo.contentReplacementInfo.bundleMountInformation.isMounted)) {
        return FS_STATUS_USE_REAL_OS;
    }

    char pathForCheck[256];

    getFullPath(pathForCheck, sizeof(pathForCheck), path);

    if (checkForSave(pathForCheck, sizeof(pathForCheck), pathForCheck)) {
        DEBUG_FUNCTION_LINE("Redirect save to %s", pathForCheck);
        return fallback_function(pathForCheck);
    }
    if (handle == nullptr) {
        DEBUG_FUNCTION_LINE("Invalid args");
        return FS_STATUS_FATAL_ERROR;
    }

    if (strncmp(pathForCheck, "/vol/content", 12) == 0) {
        replaceContentPath(pathForCheck, sizeof(pathForCheck), 12, gReplacementInfo.contentReplacementInfo.replacementPath);

        DEBUG_FUNCTION_LINE_VERBOSE("%s -> %s", path, pathForCheck);
        FSStatus result = FS_STATUS_OK;

        int handle_index = getNewDirHandleIndex();

        if (handle_index >= 0) {
            DIR *dir;
            if ((dir = opendir(pathForCheck))) {
                OSLockMutex(dir_handles[handle_index].mutex);
                dir_handles[handle_index].handle = DIR_HANDLE_MAGIC | handle_index;
                *handle                          = dir_handles[handle_index].handle;
                dir_handles[handle_index].dir    = dir;
                dir_handles[handle_index].in_use = true;

                dir_handles[handle_index].path[0] = '\0';
                strncat(dir_handles[handle_index].path, pathForCheck, sizeof(dir_handles[handle_index].path) - 1);

                if (gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_FROM_PATH) {
                    auto dir_info = &dir_handles[handle_index];

                    dir_info->readResult                = nullptr;
                    dir_info->readResultCapacity        = 0;
                    dir_info->readResultNumberOfEntries = 0;

                    dir_info->realDirHandle = 0;

                    if (gFSClient && gFSCmd) {
                        FSDirectoryHandle realHandle = 0;
                        if (real_FSOpenDir(gFSClient, gFSCmd, path, &realHandle, (FSErrorFlag) FORCE_REAL_FUNC_WITH_FULL_ERRORS) == FS_STATUS_OK) {
                            dir_info->realDirHandle = realHandle;
                        } else {
                            DEBUG_FUNCTION_LINE_VERBOSE("Failed to open real dir %s", path);
                        }
                    } else {
                        DEBUG_FUNCTION_LINE("Global FSClient or FSCmdBlock were null");
                    }
                    DCFlushRange(dir_info, sizeof(dirMagic_t));
                }

                OSUnlockMutex(dir_handles[handle_index].mutex);
            } else {
                if (gReplacementInfo.contentReplacementInfo.fallbackOnError) {
                    return FS_STATUS_USE_REAL_OS;
                }
                if (errorMask & FS_ERROR_FLAG_NOT_FOUND) {
                    result = FS_STATUS_NOT_FOUND;
                }
            }
        } else {
            if (errorMask & FS_ERROR_FLAG_MAX) {
                result = FS_STATUS_MAX;
            }
        }
        return result_handler(result);
    }
    return FS_STATUS_USE_REAL_OS;
}

extern "C" FSStatus (*real_FSReadDir)(FSClient *client, FSCmdBlock *block, FSDirectoryHandle handle, FSDirectoryEntry *entry, FSErrorFlag errorMask);

FSStatus FSReadDirWrapper(FSDirectoryHandle handle,
                          FSDirectoryEntry *entry,
                          [[maybe_unused]] FSErrorFlag errorMask,
                          const std::function<FSStatus(FSStatus)> &result_handler) {

    if ((gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_NONE) ||
        (gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_FROM_WUHB_BUNDLE && !gReplacementInfo.contentReplacementInfo.bundleMountInformation.isMounted) ||
        !isValidDirHandle(handle)) {
        return FS_STATUS_USE_REAL_OS;
    }

    uint32_t handle_index = handle & HANDLE_MASK;
    if (handle_index >= DIR_HANDLES_LENGTH) {
        DEBUG_FUNCTION_LINE("Invalid handle");
        return result_handler(FS_STATUS_FATAL_ERROR);
    }

    OSLockMutex(dir_handles[handle_index].mutex);
    DIR *dir = dir_handles[handle_index].dir;

    if (gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_FROM_PATH) {
        auto dir_info = &dir_handles[handle_index];

        // Init list if needed
        if (dir_info->readResultCapacity == 0) {
            dir_info->readResult = (FSDirectoryEntry *) malloc(sizeof(FSDirectoryEntry));
            if (dir_info->readResult != nullptr) {
                dir_info->readResultCapacity = 1;
            }
        }
        DCFlushRange(dir_info, sizeof(dirMagic_t));
    }

    struct dirent *entry_ = readdir(dir);
    FSStatus result       = FS_STATUS_END;
    if (entry_) {
        entry->name[0] = '\0';
        strncat(entry->name, entry_->d_name, sizeof(entry->name) - 1);
        entry->info.mode = (FSMode) FS_MODE_READ_OWNER;
        if (entry_->d_type == DT_DIR) {
            entry->info.flags = (FSStatFlags) ((uint32_t) FS_STAT_DIRECTORY);
            entry->info.size  = 0;
        } else {
            entry->info.flags = (FSStatFlags) 0;
            if (strcmp(entry_->d_name, ".") == 0 || strcmp(entry_->d_name, "..") == 0) {
                entry->info.size = 0;
            } else {
                struct stat sb {};
                int strLen = strlen(dir_handles[handle_index].path) + 1 + strlen(entry_->d_name) + 1;
                char path[strLen];
                snprintf(path, sizeof(path), "%s/%s", dir_handles[handle_index].path, entry_->d_name);
                if (stat(path, &sb) >= 0) {
                    entry->info.size  = sb.st_size;
                    entry->info.flags = (FSStatFlags) 0;
                    entry->info.owner = sb.st_uid;
                    entry->info.group = sb.st_gid;
                }
            }
        }

        if (gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_FROM_PATH) {
            auto dir_info = &dir_handles[handle_index];
            if (dir_info->readResultNumberOfEntries >= dir_info->readResultCapacity) {
                auto newCapacity             = dir_info->readResultCapacity * 2;
                dir_info->readResult         = (FSDirectoryEntry *) realloc(dir_info->readResult, newCapacity * sizeof(FSDirectoryEntry));
                dir_info->readResultCapacity = newCapacity;
                if (dir_info->readResult == nullptr) {
                    OSFatal("Failed to alloc memory for dir entry list");
                }
            }

            memcpy(&dir_info->readResult[dir_info->readResultNumberOfEntries], entry, sizeof(FSDirectoryEntry));
            dir_info->readResultNumberOfEntries++;

            DCFlushRange(dir_info->readResult, sizeof(FSDirectoryEntry) * dir_info->readResultNumberOfEntries);
            DCFlushRange(dir_info, sizeof(dirMagic_t));
        }

        result = FS_STATUS_OK;
    } else if (gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_FROM_PATH) {
        auto dir_info = &dir_handles[handle_index];
        // Read the real directory.
        if (dir_info->realDirHandle != 0) {
            if (gFSClient && gFSCmd) {
                FSDirectoryEntry realDirEntry;
                FSStatus readDirResult = FS_STATUS_OK;
                result                 = FS_STATUS_END;
                while (readDirResult == FS_STATUS_OK) {
                    readDirResult = real_FSReadDir(gFSClient, gFSCmd, dir_info->realDirHandle, &realDirEntry, (FSErrorFlag) FORCE_REAL_FUNC_WITH_FULL_ERRORS);
                    if (readDirResult == FS_STATUS_OK) {
                        bool found = false;
                        for (int i = 0; i < dir_info->readResultNumberOfEntries; i++) {
                            auto curResult = &dir_info->readResult[i];
                            // Check if this is a new result
                            if (strncmp(curResult->name, realDirEntry.name, sizeof(realDirEntry.name) - 1) == 0) {
                                found = true;
                                break;
                            }
                        }
                        // If it's new we can use it :)
                        if (!found) {
                            memcpy(entry, &realDirEntry, sizeof(FSDirectoryEntry));
                            result = FS_STATUS_OK;
                            break;
                        }
                    } else if (readDirResult == FS_STATUS_END) {
                        result = FS_STATUS_END;
                        break;
                    } else {
                        DEBUG_FUNCTION_LINE("real_FSReadDir returned an unexpected error: %08X", readDirResult);
                        result = FS_STATUS_END;
                        break;
                    }
                }
            } else {
                DEBUG_FUNCTION_LINE("Global FSClient or FSCmdBlock were null");
            }
        }
    }

    OSUnlockMutex(dir_handles[handle_index].mutex);
    return result_handler(result);
}

extern "C" FSStatus (*real_FSCloseDir)(FSClient *client, FSCmdBlock *block, FSDirectoryHandle handle, FSErrorFlag errorMask);

FSStatus FSCloseDirWrapper(FSDirectoryHandle handle,
                           [[maybe_unused]] FSErrorFlag errorMask,
                           const std::function<FSStatus(FSStatus)> &result_handler) {
    if ((gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_NONE) ||
        (gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_FROM_WUHB_BUNDLE && !gReplacementInfo.contentReplacementInfo.bundleMountInformation.isMounted) ||
        !isValidDirHandle(handle)) {
        return FS_STATUS_USE_REAL_OS;
    }
    uint32_t handle_index = handle & HANDLE_MASK;
    if (handle_index >= DIR_HANDLES_LENGTH) {
        DEBUG_FUNCTION_LINE("Invalid handle");
        return FS_STATUS_FATAL_ERROR;
    }

    OSLockMutex(dir_handles[handle_index].mutex);

    DIR *dir = dir_handles[handle_index].dir;

    FSStatus result = FS_STATUS_OK;
    if (closedir(dir) != 0) {
        result = FS_STATUS_MEDIA_ERROR;
    }

    if (gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_FROM_PATH) {
        auto dir_info = &dir_handles[handle_index];

        if (dir_info->realDirHandle != 0) {
            if (gFSClient && gFSCmd) {
                auto realResult = real_FSCloseDir(gFSClient, gFSCmd, dir_info->realDirHandle, (FSErrorFlag) FORCE_REAL_FUNC_WITH_FULL_ERRORS);
                if (realResult == FS_STATUS_OK) {
                    dir_info->realDirHandle = 0;
                } else {
                    DEBUG_FUNCTION_LINE("Failed to closed dir %d", realResult);
                }
            } else {
                DEBUG_FUNCTION_LINE("Global FSClient or FSCmdBlock were null");
            }
        }

        if (dir_info->readResult != nullptr) {
            free(dir_info->readResult);
            dir_info->readResult                = nullptr;
            dir_info->readResultCapacity        = 0;
            dir_info->readResultNumberOfEntries = 0;
        }
        DCFlushRange(dir_info, sizeof(dirMagic_t));
    }

    OSUnlockMutex(dir_handles[handle_index].mutex);
    freeDirHandle(handle_index);
    return result_handler(result);
}

extern "C" FSStatus (*real_FSRewindDir)(FSClient *client, FSCmdBlock *block, FSDirectoryHandle handle, FSErrorFlag errorMask);

FSStatus FSRewindDirWrapper(FSDirectoryHandle handle,
                            [[maybe_unused]] FSErrorFlag errorMask,
                            const std::function<FSStatus(FSStatus)> &result_handler) {
    if ((gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_NONE) ||
        (gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_FROM_WUHB_BUNDLE && !gReplacementInfo.contentReplacementInfo.bundleMountInformation.isMounted) ||
        !isValidDirHandle(handle)) {
        return FS_STATUS_USE_REAL_OS;
    }
    uint32_t handle_index = handle & HANDLE_MASK;
    if (handle_index >= DIR_HANDLES_LENGTH) {
        DEBUG_FUNCTION_LINE("Invalid handle");
        return FS_STATUS_FATAL_ERROR;
    }
    uint32_t real_handle = handle & HANDLE_MASK;
    OSLockMutex(dir_handles[handle_index].mutex);

    DIR *dir = dir_handles[real_handle].dir;
    rewinddir(dir);

    if (gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_FROM_PATH) {
        auto dir_info = &dir_handles[handle_index];

        if (dir_info->readResult != nullptr) {
            dir_info->readResultNumberOfEntries = 0;
            memset(dir_info->readResult, 0, sizeof(FSDirectoryEntry) * dir_info->readResultCapacity);
        }

        if (dir_info->realDirHandle != 0) {
            if (gFSClient && gFSCmd) {
                if (real_FSRewindDir(gFSClient, gFSCmd, dir_info->realDirHandle, (FSErrorFlag) FORCE_REAL_FUNC_WITH_FULL_ERRORS) == FS_STATUS_OK) {
                    dir_info->realDirHandle = 0;
                } else {
                    DEBUG_FUNCTION_LINE("Failed to rewind dir");
                }
            } else {
                DEBUG_FUNCTION_LINE("Global FSClient or FSCmdBlock were null");
            }
        }
        DCFlushRange(dir_info, sizeof(dirMagic_t));
    }

    OSUnlockMutex(dir_handles[handle_index].mutex);
    return result_handler(FS_STATUS_OK);
}

FSStatus FSMakeDirWrapper(char *path,
                          FSErrorFlag errorMask,
                          const std::function<FSStatus(char *)> &fallback_function,
                          const std::function<FSStatus(FSStatus)> &result_handler) {
    if ((gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_NONE) ||
        (gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_FROM_PATH) ||
        (gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_FROM_WUHB_BUNDLE && !gReplacementInfo.contentReplacementInfo.bundleMountInformation.isMounted)) {
        return FS_STATUS_USE_REAL_OS;
    }

    char pathForCheck[256];

    getFullPath(pathForCheck, sizeof(pathForCheck), path);

    if (checkForSave(pathForCheck, sizeof(pathForCheck), pathForCheck)) {
        DEBUG_FUNCTION_LINE("Redirect save to %s", pathForCheck);
        return fallback_function(pathForCheck);
    }

    if (strncmp(pathForCheck, "/vol/content", 12) == 0) {
        FSStatus result = FS_STATUS_OK;
        if (errorMask & FS_ERROR_FLAG_ACCESS_ERROR) {
            result = FS_STATUS_ACCESS_ERROR;
        }
        return result_handler(result);
    }
    return FS_STATUS_USE_REAL_OS;
}

FSStatus FSOpenFileWrapper(char *path,
                           const char *mode,
                           FSFileHandle *handle,
                           FSErrorFlag errorMask,
                           const std::function<FSStatus(char *)> &fallback_function,
                           const std::function<FSStatus(FSStatus)> &result_handler) {
    if ((gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_NONE) ||
        (gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_FROM_WUHB_BUNDLE && !gReplacementInfo.contentReplacementInfo.bundleMountInformation.isMounted)) {
        return FS_STATUS_USE_REAL_OS;
    }
    if (path == nullptr) {
        OSFatal("Invalid args");
        return FS_STATUS_FATAL_ERROR;
    }

    char pathForCheck[256];

    getFullPath(pathForCheck, sizeof(pathForCheck), path);
    DEBUG_FUNCTION_LINE_VERBOSE("%s -> %s", path, pathForCheck);

    if (checkForSave(pathForCheck, sizeof(pathForCheck), pathForCheck)) {
        DEBUG_FUNCTION_LINE("Redirect save to %s", pathForCheck);
        return fallback_function(pathForCheck);
    }
    if (mode == nullptr || handle == nullptr) {
        DEBUG_FUNCTION_LINE("Invalid args");
        return FS_STATUS_FATAL_ERROR;
    }

    if (strncmp(pathForCheck, "/vol/content", 12) == 0) {
        replaceContentPath(pathForCheck, sizeof(pathForCheck), 12, gReplacementInfo.contentReplacementInfo.replacementPath);

        DEBUG_FUNCTION_LINE_VERBOSE("%s -> %s", path, pathForCheck);
        int handle_index = getNewFileHandleIndex();
        FSStatus result  = FS_STATUS_OK;
        if (handle_index >= 0) {
            OSLockMutex(file_handles[handle_index].mutex);
            int _mode = 0;
            // Map flags to open modes
            if (strcmp(mode, "r") == 0 || strcmp(mode, "rb") == 0) {
                _mode = 0x000;
            } else if (strcmp(mode, "r+") == 0) {
                _mode = 0x002;
            } else if (strcmp(mode, "w") == 0) {
                _mode = 0x601;
            } else if (strcmp(mode, "w+") == 0) {
                _mode = 0x602;
            } else if (strcmp(mode, "a") == 0) {
                _mode = 0x209;
            } else if (strcmp(mode, "a+") == 0) {
                _mode = 0x20A;
            } else {
                //DEBUG_FUNCTION_LINE("%s", mode);
                if (errorMask & FS_ERROR_FLAG_ACCESS_ERROR) {
                    result = FS_STATUS_ACCESS_ERROR;
                }
            }

            int32_t fd = open(pathForCheck, _mode);
            if (fd >= 0) {
                DEBUG_FUNCTION_LINE_VERBOSE("Opened %s as  %d", pathForCheck, fd);

                file_handles[handle_index].handle = FILE_HANDLE_MAGIC + handle_index;
                //DEBUG_FUNCTION_LINE("handle %08X", file_handles[handle_index].handle);
                *handle                           = file_handles[handle_index].handle;
                file_handles[handle_index].fd     = fd;
                file_handles[handle_index].in_use = true;
                DCFlushRange(&file_handles[handle_index], sizeof(fileMagic_t));
            } else {
                DEBUG_FUNCTION_LINE("File not found %s", pathForCheck);
                if (gReplacementInfo.contentReplacementInfo.fallbackOnError) {
                    result = FS_STATUS_USE_REAL_OS;
                } else if (errorMask & FS_ERROR_FLAG_NOT_FOUND) {
                    result = FS_STATUS_NOT_FOUND;
                }
            }
            OSUnlockMutex(file_handles[handle_index].mutex);
        } else {
            if (gReplacementInfo.contentReplacementInfo.fallbackOnError) {
                result = FS_STATUS_USE_REAL_OS;
            } else if (errorMask & FS_ERROR_FLAG_MAX) {
                result = FS_STATUS_MAX;
            }
        }
        if (result != FS_STATUS_USE_REAL_OS) {
            DEBUG_FUNCTION_LINE_VERBOSE("return error %d", result);
            return result_handler(result);
        }
    }
    return FS_STATUS_USE_REAL_OS;
}

FSStatus FSCloseFileWrapper(FSFileHandle handle,
                            [[maybe_unused]] FSErrorFlag errorMask,
                            const std::function<FSStatus(FSStatus)> &result_handler) {
    if ((gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_NONE) ||
        (gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_FROM_WUHB_BUNDLE && !gReplacementInfo.contentReplacementInfo.bundleMountInformation.isMounted) ||
        !isValidFileHandle(handle)) {
        return FS_STATUS_USE_REAL_OS;
    }

    uint32_t handle_index = handle & HANDLE_MASK;

    if (handle_index >= FILE_HANDLES_LENGTH) {
        DEBUG_FUNCTION_LINE("Invalid handle");
        return FS_STATUS_FATAL_ERROR;
    }

    OSLockMutex(file_handles[handle_index].mutex);

    int real_fd                       = file_handles[handle_index].fd;
    file_handles[handle_index].in_use = false;

    DEBUG_FUNCTION_LINE_VERBOSE("closing %d", real_fd);

    FSStatus result = FS_STATUS_OK;
    if (close(real_fd) != 0) {
        result = FS_STATUS_MEDIA_ERROR;
    }
    OSUnlockMutex(file_handles[handle_index].mutex);

    freeFileHandle(handle_index);
    return result_handler(result);
}

FSStatus FSGetStatWrapper(char *path, FSStat *stats, FSErrorFlag errorMask,
                          const std::function<FSStatus(char *)> &fallback_function,
                          const std::function<FSStatus(FSStatus)> &result_handler) {
    if ((gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_NONE) ||
        (gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_FROM_WUHB_BUNDLE && !gReplacementInfo.contentReplacementInfo.bundleMountInformation.isMounted)) {
        return FS_STATUS_USE_REAL_OS;
    }
    if (path == nullptr) {
        OSFatal("Invalid args");
        return FS_STATUS_FATAL_ERROR;
    }

    char pathForCheck[256];

    getFullPath(pathForCheck, sizeof(pathForCheck), path);

    if (checkForSave(pathForCheck, sizeof(pathForCheck), pathForCheck)) {
        DEBUG_FUNCTION_LINE("Redirect save to %s", pathForCheck);
        return fallback_function(pathForCheck);
    }
    if (strncmp(pathForCheck, "/vol/content", 12) == 0) {
        replaceContentPath(pathForCheck, sizeof(pathForCheck), 12, gReplacementInfo.contentReplacementInfo.replacementPath);
        DEBUG_FUNCTION_LINE("%s -> %s", path, pathForCheck);

        FSStatus result = FS_STATUS_OK;
        if (stats == nullptr) {
            if (gReplacementInfo.contentReplacementInfo.fallbackOnError) {
                return FS_STATUS_USE_REAL_OS;
            }
            DEBUG_FUNCTION_LINE("Invalid args");
            return FS_STATUS_FATAL_ERROR;
        } else {
            struct stat path_stat {};
            memset(&path_stat, 0, sizeof(path_stat));
            if (stat(pathForCheck, &path_stat) < 0) {
                if (gReplacementInfo.contentReplacementInfo.fallbackOnError) {
                    return FS_STATUS_USE_REAL_OS;
                } else if (errorMask & FS_ERROR_FLAG_NOT_FOUND) {
                    DEBUG_FUNCTION_LINE("Path not found %s", pathForCheck);
                    result = FS_STATUS_NOT_FOUND;
                }
            } else {
                memset(&(stats->flags), 0, sizeof(stats->flags));
                if (S_ISDIR(path_stat.st_mode)) {
                    stats->flags = (FSStatFlags) ((uint32_t) FS_STAT_DIRECTORY);
                } else {
                    stats->size  = path_stat.st_size;
                    stats->mode  = (FSMode) FS_MODE_READ_OWNER;
                    stats->flags = (FSStatFlags) 0;
                    stats->owner = path_stat.st_uid;
                    stats->group = path_stat.st_gid;
                }
                DEBUG_FUNCTION_LINE_VERBOSE("stats file for %s, size %016lLX", path, stats->size);
            }
        }
        return result_handler(result);
    }
    return FS_STATUS_USE_REAL_OS;
}

static inline void replaceContentPath(char *pathForCheck, size_t pathForCheckSize, int skipLen, char *replaceWith) {
    size_t subStrLen = strlen(pathForCheck) - skipLen;
    if (subStrLen <= 0) {
        pathForCheck[0] = '\0';
        if (strlen(replaceWith) + 1 <= pathForCheckSize) {
            memcpy(pathForCheck, replaceWith, strlen(replaceWith) + 1);
        }
    } else {
        char pathCopy[subStrLen + 1];
        pathCopy[0] = '\0';
        strncat(pathCopy, &pathForCheck[skipLen], sizeof(pathCopy) - 1);
        snprintf(pathForCheck, pathForCheckSize, "%s%s", replaceWith, pathCopy);
    }
}

FSStatus FSGetStatFileWrapper(FSFileHandle handle,
                              FSStat *stats,
                              [[maybe_unused]] FSErrorFlag errorMask,
                              const std::function<FSStatus(FSStatus)> &result_handler) {
    if ((gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_NONE) ||
        (gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_FROM_WUHB_BUNDLE && !gReplacementInfo.contentReplacementInfo.bundleMountInformation.isMounted) ||
        !isValidFileHandle(handle)) {
        return FS_STATUS_USE_REAL_OS;
    }
    uint32_t handle_index = handle & HANDLE_MASK;

    if (handle_index >= FILE_HANDLES_LENGTH) {
        DEBUG_FUNCTION_LINE("Invalid handle");
        return FS_STATUS_FATAL_ERROR;
    }

    OSLockMutex(file_handles[handle_index].mutex);

    int real_fd = file_handles[handle_index].fd;
    //DEBUG_FUNCTION_LINE("FSGetStatFileAsync real_fd %d", real_fd);

    struct stat path_stat {};

    FSStatus result = FS_STATUS_OK;
    if (fstat(real_fd, &path_stat) < 0) {
        result = FS_STATUS_MEDIA_ERROR;
    } else {
        memset(&(stats->flags), 0, sizeof(stats->flags));

        stats->size  = path_stat.st_size;
        stats->mode  = (FSMode) FS_MODE_READ_OWNER;
        stats->flags = (FSStatFlags) 0;
        stats->owner = path_stat.st_uid;
        stats->group = path_stat.st_gid;
    }

    OSUnlockMutex(file_handles[handle_index].mutex);
    return result_handler(result);
}

FSStatus FSReadFileWrapper(void *buffer,
                           uint32_t size,
                           uint32_t count,
                           FSFileHandle handle,
                           [[maybe_unused]] uint32_t unk1,
                           [[maybe_unused]] FSErrorFlag errorMask,
                           const std::function<FSStatus(FSStatus)> &result_handler) {
    if ((gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_NONE) ||
        (gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_FROM_WUHB_BUNDLE && !gReplacementInfo.contentReplacementInfo.bundleMountInformation.isMounted) ||
        !isValidFileHandle(handle)) {
        return FS_STATUS_USE_REAL_OS;
    }
    uint32_t handle_index = handle & HANDLE_MASK;

    if (handle_index >= FILE_HANDLES_LENGTH) {
        DEBUG_FUNCTION_LINE("Invalid handle");
        return FS_STATUS_FATAL_ERROR;
    }

    if (size * count == 0) {
        return result_handler(FS_STATUS_OK);
    }
    if (buffer == nullptr && (size * count != 0)) {
        DEBUG_FUNCTION_LINE("Fatal read FSErrorFlag errorMask, buffer is null but size*count is not");
        return FS_STATUS_FATAL_ERROR;
    }

    FSStatus result;

    OSLockMutex(file_handles[handle_index].mutex);

    int real_fd = file_handles[handle_index].fd;

    int32_t read = readIntoBuffer(real_fd, buffer, size, count);

    if (read < 0) {
        DEBUG_FUNCTION_LINE("Failed to read from handle");
        result = FS_STATUS_MEDIA_ERROR;
    } else {
        result = (FSStatus) (read / size);
    }

    OSUnlockMutex(file_handles[handle_index].mutex);
    return result_handler(result);
}

FSStatus FSReadFileWithPosWrapper(void *buffer,
                                  uint32_t size,
                                  uint32_t count,
                                  uint32_t pos,
                                  FSFileHandle handle,
                                  int32_t unk1,
                                  FSErrorFlag errorMask,
                                  const std::function<FSStatus(FSStatus)> &result_handler) {
    if ((gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_NONE) ||
        (gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_FROM_WUHB_BUNDLE && !gReplacementInfo.contentReplacementInfo.bundleMountInformation.isMounted) ||
        !isValidFileHandle(handle)) {
        return FS_STATUS_USE_REAL_OS;
    }

    FSStatus result;
    if ((result = FSSetPosFileWrapper(handle, pos, errorMask,
                                      [](FSStatus res) -> FSStatus { return res; })) != FS_STATUS_OK) {
        return result;
    }

    result = FSReadFileWrapper(buffer, size, count, handle, unk1, errorMask,
                               [](FSStatus res) -> FSStatus { return res; });

    if (result != FS_STATUS_USE_REAL_OS && result != FS_STATUS_FATAL_ERROR) {
        return result_handler(result);
    }
    return result;
}

FSStatus FSSetPosFileWrapper(FSFileHandle handle,
                             uint32_t pos,
                             [[maybe_unused]] FSErrorFlag errorMask,
                             const std::function<FSStatus(FSStatus)> &result_handler) {
    if ((gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_NONE) ||
        (gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_FROM_WUHB_BUNDLE && !gReplacementInfo.contentReplacementInfo.bundleMountInformation.isMounted) ||
        !isValidFileHandle(handle)) {
        return FS_STATUS_USE_REAL_OS;
    }

    uint32_t handle_index = handle & HANDLE_MASK;

    if (handle_index >= FILE_HANDLES_LENGTH) {
        DEBUG_FUNCTION_LINE("Invalid handle");
        return FS_STATUS_FATAL_ERROR;
    }

    FSStatus result = FS_STATUS_OK;
    OSLockMutex(file_handles[handle_index].mutex);

    int real_fd = file_handles[handle_index].fd;

    if (lseek(real_fd, (off_t) pos, SEEK_SET) != pos) {
        DEBUG_FUNCTION_LINE("Seek failed");
        result = FS_STATUS_MEDIA_ERROR;
    }
    DEBUG_FUNCTION_LINE_VERBOSE("pos set to %d for %d", pos, real_fd);

    OSUnlockMutex(file_handles[handle_index].mutex);
    return result_handler(result);
}

FSStatus FSGetPosFileWrapper(FSFileHandle handle,
                             uint32_t *pos,
                             [[maybe_unused]] FSErrorFlag errorMask,
                             const std::function<FSStatus(FSStatus)> &result_handler) {
    if ((gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_NONE) ||
        (gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_FROM_WUHB_BUNDLE && !gReplacementInfo.contentReplacementInfo.bundleMountInformation.isMounted) ||
        !isValidFileHandle(handle)) {
        return FS_STATUS_USE_REAL_OS;
    }
    uint32_t handle_index = handle & HANDLE_MASK;

    if (handle_index >= FILE_HANDLES_LENGTH) {
        DEBUG_FUNCTION_LINE("Invalid handle");
        return FS_STATUS_FATAL_ERROR;
    }

    FSStatus result = FS_STATUS_OK;
    OSLockMutex(file_handles[handle_index].mutex);

    int real_fd = file_handles[handle_index].fd;

    off_t currentPos = lseek(real_fd, (off_t) 0, SEEK_CUR);
    if (currentPos == -1) {
        DEBUG_FUNCTION_LINE("Failed to get current pos for %d", real_fd);
        result = FS_STATUS_MEDIA_ERROR;
    } else {
        *pos = currentPos;
        DEBUG_FUNCTION_LINE_VERBOSE("result was %d for %d", *pos, real_fd);
    }

    OSUnlockMutex(file_handles[handle_index].mutex);
    return result_handler(result);
}

FSStatus FSIsEofWrapper(FSFileHandle handle,
                        [[maybe_unused]] FSErrorFlag errorMask,
                        const std::function<FSStatus(FSStatus)> &result_handler) {
    if ((gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_NONE) ||
        (gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_FROM_WUHB_BUNDLE && !gReplacementInfo.contentReplacementInfo.bundleMountInformation.isMounted) ||
        !isValidFileHandle(handle)) {
        return FS_STATUS_USE_REAL_OS;
    }
    uint32_t handle_index = handle & HANDLE_MASK;

    if (handle_index >= FILE_HANDLES_LENGTH) {
        DEBUG_FUNCTION_LINE("Invalid handle");
        return FS_STATUS_FATAL_ERROR;
    }

    FSStatus result = FS_STATUS_OK;
    OSLockMutex(file_handles[handle_index].mutex);

    int real_fd = file_handles[handle_index].fd;

    off_t currentPos = lseek(real_fd, (off_t) 0, SEEK_CUR);
    off_t endPos     = lseek(real_fd, (off_t) 0, SEEK_END);

    if (currentPos == endPos) {
        DEBUG_FUNCTION_LINE_VERBOSE("FSIsEof END for %d\n", real_fd);
        result = FS_STATUS_END;
    } else {
        lseek(real_fd, currentPos, SEEK_CUR);
        DEBUG_FUNCTION_LINE_VERBOSE("FSIsEof OK for %d\n", real_fd);
        result = FS_STATUS_OK;
    }

    OSUnlockMutex(file_handles[handle_index].mutex);
    return result_handler(result);
}

FSStatus FSTruncateFileWrapper(FSFileHandle handle,
                               FSErrorFlag errorMask,
                               const std::function<FSStatus(FSStatus)> &result_handler) {
    if ((gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_NONE) ||
        (gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_FROM_PATH) ||
        (gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_FROM_WUHB_BUNDLE && !gReplacementInfo.contentReplacementInfo.bundleMountInformation.isMounted) ||
        !isValidFileHandle(handle)) {
        return FS_STATUS_USE_REAL_OS;
    }
    uint32_t handle_index = handle & HANDLE_MASK;

    if (handle_index >= FILE_HANDLES_LENGTH) {
        DEBUG_FUNCTION_LINE("Invalid handle");
        return FS_STATUS_FATAL_ERROR;
    }
    FSStatus result = FS_STATUS_OK;
    if (errorMask & FS_ERROR_FLAG_ACCESS_ERROR) {
        result = FS_STATUS_ACCESS_ERROR;
    }
    return result_handler(result);
}

FSStatus FSWriteFileWrapper([[maybe_unused]] uint8_t *buffer,
                            [[maybe_unused]] uint32_t size,
                            [[maybe_unused]] uint32_t count,
                            FSFileHandle handle,
                            [[maybe_unused]] uint32_t unk1,
                            FSErrorFlag errorMask,
                            const std::function<FSStatus(FSStatus)> &result_handler) {
    if ((gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_NONE) ||
        (gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_FROM_PATH) ||
        (gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_FROM_WUHB_BUNDLE && !gReplacementInfo.contentReplacementInfo.bundleMountInformation.isMounted) ||
        !isValidFileHandle(handle)) {
        return FS_STATUS_USE_REAL_OS;
    }
    FSStatus result = FS_STATUS_OK;
    if (errorMask & FS_ERROR_FLAG_ACCESS_ERROR) {
        result = FS_STATUS_ACCESS_ERROR;
    }
    return result_handler(result);
}

FSStatus FSRemoveWrapper(char *path,
                         FSErrorFlag errorMask,
                         const std::function<FSStatus(char *)> &fallback_function,
                         const std::function<FSStatus(FSStatus)> &result_handler) {
    if ((gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_NONE) ||
        (gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_FROM_PATH) ||
        (gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_FROM_WUHB_BUNDLE && !gReplacementInfo.contentReplacementInfo.bundleMountInformation.isMounted)) {
        return FS_STATUS_USE_REAL_OS;
    }

    if (path == nullptr) {
        OSFatal("Invalid args");
        return FS_STATUS_USE_REAL_OS;
    }

    char pathForCheck[256];

    getFullPath(pathForCheck, sizeof(pathForCheck), path);

    if (checkForSave(pathForCheck, sizeof(pathForCheck), pathForCheck)) {
        DEBUG_FUNCTION_LINE("Redirect save to %s", pathForCheck);
        return fallback_function(pathForCheck);
    }
    FSStatus result = FS_STATUS_OK;
    if (errorMask & FS_ERROR_FLAG_ACCESS_ERROR) {
        result = FS_STATUS_ACCESS_ERROR;
    }
    return result_handler(result);
}

FSStatus FSRenameWrapper(char *oldPath,
                         char *newPath,
                         FSErrorFlag errorMask,
                         const std::function<FSStatus(char *, char *)> &fallback_function,
                         const std::function<FSStatus(FSStatus)> &result_handler) {
    if ((gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_NONE) ||
        (gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_FROM_PATH) ||
        (gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_FROM_WUHB_BUNDLE && !gReplacementInfo.contentReplacementInfo.bundleMountInformation.isMounted)) {
        return FS_STATUS_USE_REAL_OS;
    }

    char pathForCheck1[256];
    char pathForCheck2[256];

    getFullPath(pathForCheck1, sizeof(pathForCheck1), oldPath);
    getFullPath(pathForCheck2, sizeof(pathForCheck2), newPath);

    if (checkForSave(pathForCheck1, sizeof(pathForCheck1), pathForCheck1)) {
        if (checkForSave(pathForCheck2, sizeof(pathForCheck2), pathForCheck2)) {
            DEBUG_FUNCTION_LINE("Redirect save to %s/%s", pathForCheck1, pathForCheck2);
            return fallback_function(pathForCheck1, pathForCheck2);
        }
    }

    FSStatus result = FS_STATUS_OK;

    if (errorMask & FS_ERROR_FLAG_ACCESS_ERROR) {
        result = FS_STATUS_ACCESS_ERROR;
    }

    return result_handler(result);
}

FSStatus FSFlushFileWrapper([[maybe_unused]] FSFileHandle handle, FSErrorFlag errorMask, const std::function<FSStatus(FSStatus)> &result_handler) {
    if ((gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_NONE) ||
        (gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_FROM_PATH) ||
        (gReplacementInfo.contentReplacementInfo.mode == CONTENTREDIRECT_FROM_WUHB_BUNDLE && !gReplacementInfo.contentReplacementInfo.bundleMountInformation.isMounted)) {
        return FS_STATUS_USE_REAL_OS;
    }
    FSStatus result = FS_STATUS_OK;

    if (errorMask & FS_ERROR_FLAG_ACCESS_ERROR) {
        result = FS_STATUS_ACCESS_ERROR;
    }

    return result_handler(result);
}