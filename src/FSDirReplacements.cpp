#include "FSDirReplacements.h"
#include <coreinit/filesystem.h>
#include <coreinit/cache.h>
#include "utils/logger.h"
#include "globals.h"
#include "FSWrapper.h"
#include "FileUtils.h"

#define SYNC_RESULT_HANDLER [](FSStatus res) -> FSStatus { \
    return res; \
}

#define ASYNC_RESULT_HANDLER [client, block, asyncData](FSStatus res) -> FSStatus { \
   DEBUG_FUNCTION_LINE_VERBOSE("Result was %d", res); \
   return send_result_async(client, block, asyncData, res); \
}

DECL_FUNCTION(FSStatus, FSOpenDir, FSClient *client, FSCmdBlock *block, char *path, FSDirectoryHandle *handle, FSErrorFlag errorMask) {
    DEBUG_FUNCTION_LINE_VERBOSE("%s", path);
    FSStatus result = FSOpenDirWrapper(path, handle, errorMask,
                                       [client, block, handle, errorMask]
                                               (char *_path) -> FSStatus {
                                           return real_FSOpenDir(client, block, _path, handle, errorMask);
                                       },
                                       SYNC_RESULT_HANDLER);
    if (result != FS_STATUS_USE_REAL_OS) {
        DEBUG_FUNCTION_LINE_VERBOSE("Result was %d", result);
        return result;
    }

    return real_FSOpenDir(client, block, path, handle, errorMask);
}

DECL_FUNCTION(FSStatus, FSOpenDirAsync, FSClient *client, FSCmdBlock *block, char *path, FSDirectoryHandle *handle, FSErrorFlag errorMask, FSAsyncData *asyncData) {
    DEBUG_FUNCTION_LINE_VERBOSE("%s", path);
    FSErrorFlag realErrorMask = errorMask;

    // Even real_FSOpenDir is still calling our FSOpenDirAsync hook. To bypass our code we use "FORCE_REAL_FUNC_WITH_FULL_ERRORS" as an errorMask.
    if ((errorMask & ERROR_FLAG_MASK) != FORCE_REAL_FUNC_MAGIC) {
        FSStatus result = FSOpenDirWrapper(path, handle, errorMask,
                                           [client, block, handle, errorMask, asyncData]
                                                   (char *_path) -> FSStatus {
                                               return real_FSOpenDirAsync(client, block, _path, handle, errorMask, asyncData);
                                           },
                                           ASYNC_RESULT_HANDLER);
        if (result != FS_STATUS_USE_REAL_OS) {
            return result;
        }
    } else {
        realErrorMask = FS_ERROR_FLAG_ALL;
    }

    return real_FSOpenDirAsync(client, block, path, handle, realErrorMask, asyncData);
}

DECL_FUNCTION(FSStatus, FSReadDir, FSClient *client, FSCmdBlock *block, FSDirectoryHandle handle, FSDirectoryEntry *entry, FSErrorFlag errorMask) {
    DEBUG_FUNCTION_LINE_VERBOSE();
    FSStatus result = FSReadDirWrapper(handle, entry, errorMask, SYNC_RESULT_HANDLER);
    if (result != FS_STATUS_USE_REAL_OS) {
        return result;
    }

    return real_FSReadDir(client, block, handle, entry, errorMask);
}

DECL_FUNCTION(FSStatus, FSReadDirAsync, FSClient *client, FSCmdBlock *block, FSDirectoryHandle handle, FSDirectoryEntry *entry, FSErrorFlag errorMask, FSAsyncData *asyncData) {
    DEBUG_FUNCTION_LINE_VERBOSE();
    FSErrorFlag realErrorMask = errorMask;
    // Even real_FSReadDir is still calling our FSReadDirAsync hook. To bypass our code we use "FORCE_REAL_FUNC_WITH_FULL_ERRORS" as an errorMask.
    if ((errorMask & ERROR_FLAG_MASK) != FORCE_REAL_FUNC_MAGIC) {
        FSStatus result = FSReadDirWrapper(handle, entry, errorMask, ASYNC_RESULT_HANDLER);
        if (result != FS_STATUS_USE_REAL_OS) {
            return result;
        }
    } else {
        realErrorMask = FS_ERROR_FLAG_ALL;
    }

    return real_FSReadDirAsync(client, block, handle, entry, realErrorMask, asyncData);
}

DECL_FUNCTION(FSStatus, FSCloseDir, FSClient *client, FSCmdBlock *block, FSDirectoryHandle handle, FSErrorFlag errorMask) {
    DEBUG_FUNCTION_LINE_VERBOSE();
    FSStatus result = FSCloseDirWrapper(handle, errorMask, SYNC_RESULT_HANDLER);
    if (result != FS_STATUS_USE_REAL_OS) {
        return result;
    }

    return real_FSCloseDir(client, block, handle, errorMask);
}

DECL_FUNCTION(FSStatus, FSCloseDirAsync, FSClient *client, FSCmdBlock *block, FSDirectoryHandle handle, FSErrorFlag errorMask, FSAsyncData *asyncData) {
    DEBUG_FUNCTION_LINE_VERBOSE();
    FSErrorFlag realErrorMask = errorMask;
    // Even real_FSCloseDir is still calling our FSCloseDirAsync hook. To bypass our code we use "FORCE_REAL_FUNC_WITH_FULL_ERRORS" as an errorMask.
    if ((errorMask & ERROR_FLAG_MASK) != FORCE_REAL_FUNC_MAGIC) {
        FSStatus result = FSCloseDirWrapper(handle, errorMask, ASYNC_RESULT_HANDLER);
        if (result != FS_STATUS_USE_REAL_OS) {
            return result;
        }
    } else {
        realErrorMask = FS_ERROR_FLAG_ALL;
    }

    return real_FSCloseDirAsync(client, block, handle, realErrorMask, asyncData);
}

DECL_FUNCTION(FSStatus, FSRewindDir, FSClient *client, FSCmdBlock *block, FSDirectoryHandle handle, FSErrorFlag errorMask) {
    DEBUG_FUNCTION_LINE_VERBOSE();
    FSStatus result = FSRewindDirWrapper(handle, errorMask, SYNC_RESULT_HANDLER);
    if (result != FS_STATUS_USE_REAL_OS) {
        return result;
    }

    return real_FSRewindDir(client, block, handle, errorMask);
}

DECL_FUNCTION(FSStatus, FSRewindDirAsync, FSClient *client, FSCmdBlock *block, FSDirectoryHandle handle, FSErrorFlag errorMask, FSAsyncData *asyncData) {
    DEBUG_FUNCTION_LINE_VERBOSE();
    FSErrorFlag realErrorMask = errorMask;
    // Even real_FSRewindDir is still calling our FSRewindDirAsync hook. To bypass our code we use "FORCE_REAL_FUNC_WITH_FULL_ERRORS" as an errorMask.
    if ((errorMask & ERROR_FLAG_MASK) != FORCE_REAL_FUNC_MAGIC) {
        FSStatus result = FSRewindDirWrapper(handle, errorMask, ASYNC_RESULT_HANDLER);
        if (result != FS_STATUS_USE_REAL_OS) {
            return result;
        }
    } else {
        realErrorMask = FS_ERROR_FLAG_ALL;
    }

    return real_FSRewindDirAsync(client, block, handle, realErrorMask, asyncData);
}

DECL_FUNCTION(FSStatus, FSMakeDir, FSClient *client, FSCmdBlock *block, char *path, FSErrorFlag errorMask) {
    DEBUG_FUNCTION_LINE_VERBOSE("%s", path);
    FSStatus result = FSMakeDirWrapper(path, errorMask,
                                       [client, block, errorMask]
                                               (char *_path) -> FSStatus {
                                           return real_FSMakeDir(client, block, _path, errorMask);
                                       },
                                       SYNC_RESULT_HANDLER);
    if (result != FS_STATUS_USE_REAL_OS) {
        return result;
    }

    return real_FSMakeDir(client, block, path, errorMask);
}

DECL_FUNCTION(FSStatus, FSMakeDirAsync, FSClient *client, FSCmdBlock *block, char *path, FSErrorFlag errorMask, FSAsyncData *asyncData) {
    DEBUG_FUNCTION_LINE_VERBOSE("%s", path);
    FSStatus result = FSMakeDirWrapper(path, errorMask,
                                       [client, block, errorMask, asyncData]
                                               (char *_path) -> FSStatus {
                                           return real_FSMakeDirAsync(client, block, _path, errorMask, asyncData);
                                       },
                                       ASYNC_RESULT_HANDLER);
    if (result != FS_STATUS_USE_REAL_OS) {
        return result;
    }
    return real_FSMakeDirAsync(client, block, path, errorMask, asyncData);
}

DECL_FUNCTION(FSStatus, FSChangeDirAsync, FSClient *client, FSCmdBlock *block, const char *path, FSErrorFlag errorMask, FSAsyncData *asyncData) {
    DEBUG_FUNCTION_LINE_VERBOSE("FSChangeDirAsync %s", path);
    snprintf(gReplacementInfo.contentReplacementInfo.workingDir, sizeof(gReplacementInfo.contentReplacementInfo.workingDir), "%s", path);
    int len = strlen(gReplacementInfo.contentReplacementInfo.workingDir);
    if (len > 0 && gReplacementInfo.contentReplacementInfo.workingDir[len - 1] != '/') {
        gReplacementInfo.contentReplacementInfo.workingDir[len - 1] = '/';
        gReplacementInfo.contentReplacementInfo.workingDir[len] = 0;
    }
    DCFlushRange(gReplacementInfo.contentReplacementInfo.workingDir, sizeof(gReplacementInfo.contentReplacementInfo.workingDir));
    return real_FSChangeDirAsync(client, block, path, errorMask, asyncData);
}

function_replacement_data_t fs_dir_function_replacements[] = {
        REPLACE_FUNCTION(FSOpenDir, LIBRARY_COREINIT, FSOpenDir),
        REPLACE_FUNCTION(FSOpenDirAsync, LIBRARY_COREINIT, FSOpenDirAsync),

        REPLACE_FUNCTION(FSReadDir, LIBRARY_COREINIT, FSReadDir),
        REPLACE_FUNCTION(FSReadDirAsync, LIBRARY_COREINIT, FSReadDirAsync),

        REPLACE_FUNCTION(FSCloseDir, LIBRARY_COREINIT, FSCloseDir),
        REPLACE_FUNCTION(FSCloseDirAsync, LIBRARY_COREINIT, FSCloseDirAsync),

        REPLACE_FUNCTION(FSRewindDir, LIBRARY_COREINIT, FSRewindDir),
        REPLACE_FUNCTION(FSRewindDirAsync, LIBRARY_COREINIT, FSRewindDirAsync),

        REPLACE_FUNCTION(FSMakeDir, LIBRARY_COREINIT, FSMakeDir),
        REPLACE_FUNCTION(FSMakeDirAsync, LIBRARY_COREINIT, FSMakeDirAsync),

        REPLACE_FUNCTION(FSChangeDirAsync, LIBRARY_COREINIT, FSChangeDirAsync),
};

uint32_t fs_dir_function_replacements_size = sizeof(fs_dir_function_replacements) / sizeof(function_replacement_data_t);