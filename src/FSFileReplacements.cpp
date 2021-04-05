#include "FSFileReplacements.h"
#include "globals.h"

#include <coreinit/dynload.h>
#include <coreinit/cache.h>
#include <wums.h>
#include <cstring>
#include <string>

#include <coreinit/filesystem.h>
#include "utils/logger.h"
#include "FileUtils.h"
#include "utils/utils.h"
#include "utils/StringTools.h"
#include "FSWrapper.h"

#define SYNC_RESULT_HANDLER [](FSStatus res) -> FSStatus { \
    return res; \
}

#define ASYNC_RESULT_HANDLER [client, block, asyncData](FSStatus res) -> FSStatus { \
    return send_result_async(client, block, asyncData, res);\
}

DECL_FUNCTION(FSStatus, FSOpenFile, FSClient *client, FSCmdBlock *block, char *path, const char *mode, FSFileHandle *handle, FSErrorFlag errorMask) {
    DEBUG_FUNCTION_LINE_VERBOSE("%s", path);
    FSStatus result = FSOpenFileWrapper(path, mode, handle, errorMask,
                                        [client, block, mode, handle, errorMask]
                                                (char *_path) -> FSStatus {
                                            return real_FSOpenFile(client, block, _path, mode, handle, errorMask);
                                        },
                                        SYNC_RESULT_HANDLER);
    if (result != FS_STATUS_USE_REAL_OS) {
        return result;
    }

    FSStatus res = real_FSOpenFile(client, block, path, mode, handle, errorMask);
    return res;
}

DECL_FUNCTION(FSStatus, FSOpenFileAsync, FSClient *client, FSCmdBlock *block, char *path, const char *mode, FSFileHandle *handle, FSErrorFlag errorMask, FSAsyncData *asyncData) {
    DEBUG_FUNCTION_LINE_VERBOSE("%s", path);
    FSStatus result = FSOpenFileWrapper(path, mode, handle, errorMask,
                                        [client, block, mode, handle, errorMask, asyncData]
                                                (char *_path) -> FSStatus {
                                            return real_FSOpenFileAsync(client, block, _path, mode, handle, errorMask, asyncData);
                                        },
                                        ASYNC_RESULT_HANDLER);
    if (result != FS_STATUS_USE_REAL_OS) {
        return result;
    }

    return real_FSOpenFileAsync(client, block, path, mode, handle, errorMask, asyncData);
}

DECL_FUNCTION(FSStatus, FSCloseFile, FSClient *client, FSCmdBlock *block, FSFileHandle handle, FSErrorFlag errorMask) {
    DEBUG_FUNCTION_LINE_VERBOSE("");
    FSStatus result = FSCloseFileWrapper(handle, errorMask, SYNC_RESULT_HANDLER);
    if (result != FS_STATUS_USE_REAL_OS) {
        return result;
    }

    FSStatus res = real_FSCloseFile(client, block, handle, errorMask);
    return res;
}

DECL_FUNCTION(FSStatus, FSCloseFileAsync, FSClient *client, FSCmdBlock *block, FSFileHandle handle, FSErrorFlag errorMask, FSAsyncData *asyncData) {
    DEBUG_FUNCTION_LINE_VERBOSE("");
    FSStatus result = FSCloseFileWrapper(handle, errorMask, ASYNC_RESULT_HANDLER);
    if (result != FS_STATUS_USE_REAL_OS) {
        return result;
    }

    return real_FSCloseFileAsync(client, block, handle, errorMask, asyncData);
}

DECL_FUNCTION(FSStatus, FSGetStat, FSClient *client, FSCmdBlock *block, char *path, FSStat *stats, FSErrorFlag errorMask) {
    DEBUG_FUNCTION_LINE_VERBOSE("%s", path);
    FSStatus result = FSGetStatWrapper(path, stats, errorMask,
                                       [client, block, stats, errorMask](char *_path) -> FSStatus {
                                           return real_FSGetStat(client, block, _path, stats, errorMask);
                                       },
                                       SYNC_RESULT_HANDLER);
    if (result != FS_STATUS_USE_REAL_OS) {
        return result;
    }
    return real_FSGetStat(client, block, path, stats, errorMask);
}

DECL_FUNCTION(FSStatus, FSGetStatAsync, FSClient *client, FSCmdBlock *block, char *path, FSStat *stats, FSErrorFlag errorMask, FSAsyncData *asyncData) {
    DEBUG_FUNCTION_LINE_VERBOSE("%s", path);
    FSStatus result = FSGetStatWrapper(path, stats, errorMask,
                                       [client, block, stats, errorMask, asyncData](char *_path) -> FSStatus {
                                           return real_FSGetStatAsync(client, block, _path, stats, errorMask, asyncData);
                                       },
                                       ASYNC_RESULT_HANDLER);
    if (result != FS_STATUS_USE_REAL_OS) {
        return result;
    }
    return real_FSGetStatAsync(client, block, path, stats, errorMask, asyncData);
}

DECL_FUNCTION(FSStatus, FSGetStatFile, FSClient *client, FSCmdBlock *block, FSFileHandle handle, FSStat *stats, FSErrorFlag errorMask) {
    DEBUG_FUNCTION_LINE_VERBOSE("");
    FSStatus result = FSGetStatFileWrapper(handle, stats, errorMask, SYNC_RESULT_HANDLER);
    if (result != FS_STATUS_USE_REAL_OS) {
        return result;
    }

    return real_FSGetStatFile(client, block, handle, stats, errorMask);
}

DECL_FUNCTION(FSStatus, FSGetStatFileAsync, FSClient *client, FSCmdBlock *block, FSFileHandle handle, FSStat *stats, FSErrorFlag errorMask, FSAsyncData *asyncData) {
    DEBUG_FUNCTION_LINE_VERBOSE("");
    FSStatus result = FSGetStatFileWrapper(handle, stats, errorMask, ASYNC_RESULT_HANDLER);
    if (result != FS_STATUS_USE_REAL_OS) {
        return result;
    }

    return real_FSGetStatFileAsync(client, block, handle, stats, errorMask, asyncData);
}

DECL_FUNCTION(int32_t, FSReadFile, FSClient *client, FSCmdBlock *block, void *buffer, uint32_t size, uint32_t count, FSFileHandle handle, uint32_t unk1, FSErrorFlag errorMask) {
    DEBUG_FUNCTION_LINE_VERBOSE("");
    FSStatus result = FSReadFileWrapper(buffer, size, count, handle, unk1, errorMask, SYNC_RESULT_HANDLER);
    if (result != FS_STATUS_USE_REAL_OS) {
        return result;
    }

    return real_FSReadFile(client, block, buffer, size, count, handle, unk1, errorMask);
}

DECL_FUNCTION(int32_t, FSReadFileAsync, FSClient *client, FSCmdBlock *block, void *buffer, uint32_t size, uint32_t count, FSFileHandle handle, uint32_t unk1, FSErrorFlag errorMask, FSAsyncData *asyncData) {
    DEBUG_FUNCTION_LINE_VERBOSE("");
    FSStatus result = FSReadFileWrapper(buffer, size, count, handle, unk1, errorMask, ASYNC_RESULT_HANDLER);
    if (result != FS_STATUS_USE_REAL_OS) {
        return send_result_async(client, block, asyncData, result);
    }

    return real_FSReadFileAsync(client, block, buffer, size, count, handle, unk1, errorMask, asyncData);
}

DECL_FUNCTION(FSStatus, FSReadFileWithPos, FSClient *client, FSCmdBlock *block, void *buffer, uint32_t size, uint32_t count, uint32_t pos, FSFileHandle handle, uint32_t unk1, FSErrorFlag errorMask) {
    DEBUG_FUNCTION_LINE_VERBOSE("");
    FSStatus result = FSReadFileWithPosWrapper(buffer, size, count, pos, handle, unk1, errorMask, SYNC_RESULT_HANDLER);
    if (result != FS_STATUS_USE_REAL_OS) {
        return result;
    }
    DEBUG_FUNCTION_LINE_VERBOSE("%d", size * count);

    FSStatus res = real_FSReadFileWithPos(client, block, buffer, size, count, pos, handle, unk1, errorMask);
    return res;
}

DECL_FUNCTION(int32_t, FSReadFileWithPosAsync, FSClient *client, FSCmdBlock *block, void *buffer, uint32_t size, uint32_t count, uint32_t pos, FSFileHandle handle, int32_t unk1, FSErrorFlag errorMask, FSAsyncData *asyncData) {
    DEBUG_FUNCTION_LINE_VERBOSE("");
    FSStatus result = FSReadFileWithPosWrapper(buffer, size, count, pos, handle, unk1, errorMask, ASYNC_RESULT_HANDLER);
    if (result != FS_STATUS_USE_REAL_OS) {
        return result;
    }

    return real_FSReadFileWithPosAsync(client, block, buffer, size, count, pos, handle, unk1, errorMask, asyncData);
}

DECL_FUNCTION(FSStatus, FSSetPosFile, FSClient *client, FSCmdBlock *block, FSFileHandle handle, uint32_t pos, FSErrorFlag errorMask) {
    DEBUG_FUNCTION_LINE_VERBOSE("");
    FSStatus result = FSSetPosFileWrapper(handle, pos, errorMask, SYNC_RESULT_HANDLER);
    if (result != FS_STATUS_USE_REAL_OS) {
        return result;
    }
    return real_FSSetPosFile(client, block, handle, pos, errorMask);
}

DECL_FUNCTION(FSStatus, FSSetPosFileAsync, FSClient *client, FSCmdBlock *block, FSFileHandle handle, uint32_t pos, FSErrorFlag errorMask, FSAsyncData *asyncData) {
    DEBUG_FUNCTION_LINE_VERBOSE("");
    FSStatus result = FSSetPosFileWrapper(handle, pos, errorMask, ASYNC_RESULT_HANDLER);
    if (result != FS_STATUS_USE_REAL_OS) {
        return result;
    }

    return real_FSSetPosFileAsync(client, block, handle, pos, errorMask, asyncData);
}

DECL_FUNCTION(FSStatus, FSGetPosFile, FSClient *client, FSCmdBlock *block, FSFileHandle handle, uint32_t *pos, FSErrorFlag errorMask) {
    DEBUG_FUNCTION_LINE_VERBOSE("");
    FSStatus result = FSGetPosFileWrapper(handle, pos, errorMask, SYNC_RESULT_HANDLER);
    if (result != FS_STATUS_USE_REAL_OS) {
        return result;
    }
    return real_FSGetPosFile(client, block, handle, pos, errorMask);
}

DECL_FUNCTION(FSStatus, FSGetPosFileAsync, FSClient *client, FSCmdBlock *block, FSFileHandle handle, uint32_t *pos, FSErrorFlag errorMask, FSAsyncData *asyncData) {
    DEBUG_FUNCTION_LINE_VERBOSE("");
    FSStatus result = FSGetPosFileWrapper(handle, pos, errorMask, ASYNC_RESULT_HANDLER);
    if (result != FS_STATUS_USE_REAL_OS) {
        return result;
    }

    return real_FSGetPosFileAsync(client, block, handle, pos, errorMask, asyncData);
}

DECL_FUNCTION(FSStatus, FSIsEof, FSClient *client, FSCmdBlock *block, FSFileHandle handle, FSErrorFlag errorMask) {
    DEBUG_FUNCTION_LINE_VERBOSE("");
    FSStatus result = FSIsEofWrapper(handle, errorMask, SYNC_RESULT_HANDLER);
    if (result != FS_STATUS_USE_REAL_OS) {
        return result;
    }
    return real_FSIsEof(client, block, handle, errorMask);
}

DECL_FUNCTION(FSStatus, FSIsEofAsync, FSClient *client, FSCmdBlock *block, FSFileHandle handle, FSErrorFlag errorMask, FSAsyncData *asyncData) {
    DEBUG_FUNCTION_LINE_VERBOSE("");
    FSStatus result = FSIsEofWrapper(handle, errorMask, ASYNC_RESULT_HANDLER);
    if (result != FS_STATUS_USE_REAL_OS) {
        return result;
    }
    return real_FSIsEofAsync(client, block, handle, errorMask, asyncData);
}

DECL_FUNCTION(FSStatus, FSWriteFile, FSClient *client, FSCmdBlock *block, uint8_t *buffer, uint32_t size, uint32_t count, FSFileHandle handle, uint32_t unk1, FSErrorFlag errorMask) {
    DEBUG_FUNCTION_LINE_VERBOSE("");
    FSStatus result = FSWriteFileWrapper(buffer, size, count, handle, unk1, errorMask, SYNC_RESULT_HANDLER);
    if (result != FS_STATUS_USE_REAL_OS) {
        return result;
    }

    return real_FSWriteFile(client, block, buffer, size, count, handle, unk1, errorMask);
}

DECL_FUNCTION(FSStatus, FSWriteFileAsync, FSClient *client, FSCmdBlock *block, uint8_t *buffer, uint32_t size, uint32_t count, FSFileHandle handle, uint32_t unk1, FSErrorFlag errorMask, FSAsyncData *asyncData) {
    DEBUG_FUNCTION_LINE_VERBOSE("");
    FSStatus result = FSWriteFileWrapper(buffer, size, count, handle, unk1, errorMask, ASYNC_RESULT_HANDLER);
    if (result != FS_STATUS_USE_REAL_OS) {
        return result;
    }

    return real_FSWriteFileAsync(client, block, buffer, size, count, handle, unk1, errorMask, asyncData);
}

DECL_FUNCTION(FSStatus, FSTruncateFile, FSClient *client, FSCmdBlock *block, FSFileHandle handle, FSErrorFlag errorMask) {
    DEBUG_FUNCTION_LINE_VERBOSE("");
    FSStatus result = FSTruncateFileWrapper(handle, errorMask, SYNC_RESULT_HANDLER);
    if (result != FS_STATUS_USE_REAL_OS) {
        return result;
    }

    return real_FSTruncateFile(client, block, handle, errorMask);
}

DECL_FUNCTION(FSStatus, FSTruncateFileAsync, FSClient *client, FSCmdBlock *block, FSFileHandle handle, FSErrorFlag errorMask, FSAsyncData *asyncData) {
    DEBUG_FUNCTION_LINE_VERBOSE("");
    FSStatus result = FSTruncateFileWrapper(handle, errorMask, ASYNC_RESULT_HANDLER);
    if (result != FS_STATUS_USE_REAL_OS) {
        return result;
    }

    return real_FSTruncateFileAsync(client, block, handle, errorMask, asyncData);
}

DECL_FUNCTION(FSStatus, FSRemove, FSClient *client, FSCmdBlock *block, char *path, FSErrorFlag errorMask) {
    DEBUG_FUNCTION_LINE_VERBOSE("%s", path);
    FSStatus result = FSRemoveWrapper(path, errorMask,
                                      [client, block, errorMask](char *_path) -> FSStatus {
                                          return real_FSRemove(client, block, _path, errorMask);
                                      },
                                      SYNC_RESULT_HANDLER);
    if (result != FS_STATUS_USE_REAL_OS) {
        return result;
    }

    return real_FSRemove(client, block, path, errorMask);
}

DECL_FUNCTION(FSStatus, FSRemoveAsync, FSClient *client, FSCmdBlock *block, char *path, FSErrorFlag errorMask, FSAsyncData *asyncData) {
    DEBUG_FUNCTION_LINE_VERBOSE("%s", path);
    FSStatus result = FSRemoveWrapper(path, errorMask,
                                      [client, block, errorMask, asyncData](char *_path) -> FSStatus {
                                          return real_FSRemoveAsync(client, block, _path, errorMask, asyncData);
                                      },
                                      ASYNC_RESULT_HANDLER);
    if (result != FS_STATUS_USE_REAL_OS) {
        return result;
    }

    return real_FSRemoveAsync(client, block, path, errorMask, asyncData);
}

DECL_FUNCTION(FSStatus, FSRename, FSClient *client, FSCmdBlock *block, char *oldPath, char *newPath, FSErrorFlag errorMask) {
    DEBUG_FUNCTION_LINE_VERBOSE("%s %s", oldPath, newPath);
    FSStatus result = FSRenameWrapper(oldPath, newPath, errorMask,
                                      [client, block, errorMask](char *_oldOath, char *_newPath) -> FSStatus {
                                          return real_FSRename(client, block, _oldOath, _newPath, errorMask);
                                      },
                                      SYNC_RESULT_HANDLER);
    if (result != FS_STATUS_USE_REAL_OS) {
        return result;
    }

    return real_FSRename(client, block, oldPath, newPath, errorMask);
}

DECL_FUNCTION(FSStatus, FSRenameAsync, FSClient *client, FSCmdBlock *block, char *oldPath, char *newPath, FSErrorFlag errorMask, FSAsyncData *asyncData) {
    DEBUG_FUNCTION_LINE_VERBOSE("%s %s", oldPath, newPath);
    FSStatus result = FSRenameWrapper(oldPath, newPath, errorMask,
                                      [client, block, errorMask, asyncData](char *_oldOath, char *_newPath) -> FSStatus {
                                          return real_FSRenameAsync(client, block, _oldOath, _newPath, errorMask, asyncData);
                                      },
                                      ASYNC_RESULT_HANDLER);
    if (result != FS_STATUS_USE_REAL_OS) {
        return result;
    }

    return real_FSRenameAsync(client, block, oldPath, newPath, errorMask, asyncData);
}

DECL_FUNCTION(FSStatus, FSFlushFile, FSClient *client, FSCmdBlock *block, FSFileHandle handle, FSErrorFlag errorMask) {
    DEBUG_FUNCTION_LINE_VERBOSE("");
    FSStatus result = FSFlushFileWrapper(handle, errorMask, SYNC_RESULT_HANDLER);
    if (result != FS_STATUS_USE_REAL_OS) {
        return result;
    }

    return real_FSFlushFile(client, block, handle, errorMask);
}

DECL_FUNCTION(FSStatus, FSFlushFileAsync, FSClient *client, FSCmdBlock *block, FSFileHandle handle, FSErrorFlag errorMask, FSAsyncData *asyncData) {
    DEBUG_FUNCTION_LINE_VERBOSE("");
    FSStatus result = FSFlushFileWrapper(handle, errorMask, ASYNC_RESULT_HANDLER);
    if (result != FS_STATUS_USE_REAL_OS) {
        return result;
    }

    return real_FSFlushFileAsync(client, block, handle, errorMask, asyncData);
}

DECL_FUNCTION(FSStatus, FSChangeModeAsync, FSClient *client, FSCmdBlock *block, char *path, FSMode mode, FSErrorFlag errorMask, FSAsyncData *asyncData) {
    DEBUG_FUNCTION_LINE_VERBOSE("FSChangeModeAsync %s", path);
    return real_FSChangeModeAsync(client, block, path, mode, errorMask, asyncData);
}

DECL_FUNCTION(FSStatus, FSGetFreeSpaceSizeAsync, FSClient *client, FSCmdBlock *block, char *path, uint64_t *outSize, FSErrorFlag errorMask, FSAsyncData *asyncData) {
    DEBUG_FUNCTION_LINE_VERBOSE("FSGetFreeSpaceSizeAsync %s", path);
    return real_FSGetFreeSpaceSizeAsync(client, block, path, outSize, errorMask, asyncData);
}


function_replacement_data_t fs_file_function_replacements[] = {
        REPLACE_FUNCTION(FSOpenFile, LIBRARY_COREINIT, FSOpenFile),
        REPLACE_FUNCTION(FSOpenFileAsync, LIBRARY_COREINIT, FSOpenFileAsync),

        REPLACE_FUNCTION(FSCloseFile, LIBRARY_COREINIT, FSCloseFile),
        REPLACE_FUNCTION(FSCloseFileAsync, LIBRARY_COREINIT, FSCloseFileAsync),

        REPLACE_FUNCTION(FSGetStat, LIBRARY_COREINIT, FSGetStat),
        REPLACE_FUNCTION(FSGetStatAsync, LIBRARY_COREINIT, FSGetStatAsync),

        REPLACE_FUNCTION(FSGetStatFile, LIBRARY_COREINIT, FSGetStatFile),
        REPLACE_FUNCTION(FSGetStatFileAsync, LIBRARY_COREINIT, FSGetStatFileAsync),

        REPLACE_FUNCTION(FSReadFile, LIBRARY_COREINIT, FSReadFile),
        REPLACE_FUNCTION(FSReadFileAsync, LIBRARY_COREINIT, FSReadFileAsync),

        REPLACE_FUNCTION(FSReadFileWithPos, LIBRARY_COREINIT, FSReadFileWithPos),
        REPLACE_FUNCTION(FSReadFileWithPosAsync, LIBRARY_COREINIT, FSReadFileWithPosAsync),

        REPLACE_FUNCTION(FSSetPosFile, LIBRARY_COREINIT, FSSetPosFile),
        REPLACE_FUNCTION(FSSetPosFileAsync, LIBRARY_COREINIT, FSSetPosFileAsync),

        REPLACE_FUNCTION(FSGetPosFile, LIBRARY_COREINIT, FSGetPosFile),
        REPLACE_FUNCTION(FSGetPosFileAsync, LIBRARY_COREINIT, FSGetPosFileAsync),

        REPLACE_FUNCTION(FSIsEof, LIBRARY_COREINIT, FSIsEof),
        REPLACE_FUNCTION(FSIsEofAsync, LIBRARY_COREINIT, FSIsEofAsync),

        REPLACE_FUNCTION(FSWriteFile, LIBRARY_COREINIT, FSWriteFile),
        REPLACE_FUNCTION(FSWriteFileAsync, LIBRARY_COREINIT, FSWriteFileAsync),

        REPLACE_FUNCTION(FSTruncateFile, LIBRARY_COREINIT, FSTruncateFile),
        REPLACE_FUNCTION(FSTruncateFileAsync, LIBRARY_COREINIT, FSTruncateFileAsync),

        REPLACE_FUNCTION(FSRemove, LIBRARY_COREINIT, FSRemove),
        REPLACE_FUNCTION(FSRemoveAsync, LIBRARY_COREINIT, FSRemoveAsync),

        REPLACE_FUNCTION(FSRename, LIBRARY_COREINIT, FSRename),
        REPLACE_FUNCTION(FSRenameAsync, LIBRARY_COREINIT, FSRenameAsync),

        REPLACE_FUNCTION(FSFlushFile, LIBRARY_COREINIT, FSFlushFile),
        REPLACE_FUNCTION(FSFlushFileAsync, LIBRARY_COREINIT, FSFlushFileAsync),

        //REPLACE_FUNCTION(FSChangeModeAsync, LIBRARY_COREINIT, FSChangeModeAsync),

        //REPLACE_FUNCTION(FSGetFreeSpaceSizeAsync, LIBRARY_COREINIT, FSGetFreeSpaceSizeAsync),
};

uint32_t fs_file_function_replacements_size = sizeof(fs_file_function_replacements) / sizeof(function_replacement_data_t);