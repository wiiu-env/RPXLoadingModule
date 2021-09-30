#pragma once

#include <cstdint>
#include <dirent.h>
#include <functional>
#include <coreinit/filesystem.h>
#include <coreinit/mutex.h>

typedef struct dirMagic {
    uint32_t handle{};
    DIR *dir{};
    bool in_use{};
    char path[256]{};

    OSMutex *mutex{};

    FSDirectoryEntry * readResult = nullptr;
    int readResultCapacity = 0;
    int readResultNumberOfEntries = 0;

    FSDirectoryHandle realDirHandle = 0;
} dirMagic_t;

typedef struct fileMagic {
    uint32_t handle;
    bool in_use;
    int fd;
    OSMutex *mutex;
} fileMagic_t;


#define ERROR_FLAG_MASK                     (0xFFFF0000)
#define FORCE_REAL_FUNC_MAGIC               (0x42420000)
#define FORCE_REAL_FUNC_WITH_FULL_ERRORS    (FORCE_REAL_FUNC_MAGIC | 0x0000FFFF)

#define HANDLE_INDICATOR_MASK   0xFFFFFF00
#define HANDLE_INDICATOR_MASK   0xFFFFFF00
#define HANDLE_MASK             (0x000000FF)
#define DIR_HANDLE_MAGIC        0x30000000
#define FILE_HANDLE_MAGIC       0x30000100

#define FILE_HANDLES_LENGTH     64
#define DIR_HANDLES_LENGTH      64

#define FS_STATUS_USE_REAL_OS   (FSStatus) 0xFFFF0000

extern dirMagic_t dir_handles[DIR_HANDLES_LENGTH];
extern fileMagic_t file_handles[FILE_HANDLES_LENGTH];

FSStatus FSOpenDirWrapper(char *path,
                          FSDirectoryHandle *handle,
                          FSErrorFlag errorMask,
                          const std::function<FSStatus(char *)> &fallback_function,
                          const std::function<FSStatus(FSStatus)> &result_handler);

FSStatus FSReadDirWrapper(FSDirectoryHandle handle,
                          FSDirectoryEntry *entry,
                          FSErrorFlag errorMask,
                          const std::function<FSStatus(FSStatus)> &result_handler);

FSStatus FSCloseDirWrapper(FSDirectoryHandle handle,
                           FSErrorFlag errorMask,
                           const std::function<FSStatus(FSStatus)> &result_handler);

FSStatus FSRewindDirWrapper(FSDirectoryHandle handle,
                            FSErrorFlag errorMask,
                            const std::function<FSStatus(FSStatus)> &result_handler);

FSStatus FSMakeDirWrapper(char *path,
                          FSErrorFlag errorMask,
                          const std::function<FSStatus(char *)> &fallback_function,
                          const std::function<FSStatus(FSStatus)> &result_handler);

FSStatus FSOpenFileWrapper(char *path,
                           const char *mode,
                           FSFileHandle *handle,
                           FSErrorFlag errorMask,
                           const std::function<FSStatus(char *)> &fallback_function,
                           const std::function<FSStatus(FSStatus)> &result_handler);

FSStatus FSCloseFileWrapper(FSFileHandle handle,
                            FSErrorFlag errorMask,
                            const std::function<FSStatus(FSStatus)> &result_handler);

FSStatus FSGetStatWrapper(char *path,
                          FSStat *stats,
                          FSErrorFlag errorMask,
                          const std::function<FSStatus(char *)> &fallback_function,
                          const std::function<FSStatus(FSStatus)> &result_handler);

FSStatus FSGetStatFileWrapper(FSFileHandle handle,
                              FSStat *stats,
                              FSErrorFlag errorMask,
                              const std::function<FSStatus(FSStatus)> &result_handler);

FSStatus FSReadFileWrapper(void *buffer,
                           uint32_t size,
                           uint32_t count,
                           FSFileHandle handle,
                           uint32_t unk1,
                           FSErrorFlag errorMask,
                           const std::function<FSStatus(FSStatus)> &result_handler);

FSStatus FSReadFileWithPosWrapper(void *buffer,
                                  uint32_t size,
                                  uint32_t count,
                                  uint32_t pos,
                                  FSFileHandle handle,
                                  int32_t unk1,
                                  FSErrorFlag errorMask,
                                  const std::function<FSStatus(FSStatus)> &result_handler);

FSStatus FSSetPosFileWrapper(FSFileHandle handle,
                             uint32_t pos,
                             FSErrorFlag errorMask,
                             const std::function<FSStatus(FSStatus)> &result_handler);

FSStatus FSGetPosFileWrapper(FSFileHandle handle,
                             uint32_t *pos,
                             FSErrorFlag errorMask,
                             const std::function<FSStatus(FSStatus)> &result_handler);

FSStatus FSIsEofWrapper(FSFileHandle handle,
                        FSErrorFlag errorMask,
                        const std::function<FSStatus(FSStatus)> &result_handler);

FSStatus FSTruncateFileWrapper(FSFileHandle handle,
                               FSErrorFlag errorMask,
                               const std::function<FSStatus(FSStatus)> &result_handler);

FSStatus FSWriteFileWrapper(uint8_t *buffer,
                            uint32_t size,
                            uint32_t count,
                            FSFileHandle handle,
                            uint32_t unk1,
                            FSErrorFlag errorMask,
                            const std::function<FSStatus(FSStatus)> &result_handler);

FSStatus FSRemoveWrapper(char *path,
                         FSErrorFlag errorMask,
                         const std::function<FSStatus(char *)> &fallback_function,
                         const std::function<FSStatus(FSStatus)> &result_handler);

FSStatus FSRenameWrapper(char *oldPath,
                         char *newPath,
                         FSErrorFlag errorMask,
                         const std::function<FSStatus(char *, char *)> &fallback_function,
                         const std::function<FSStatus(FSStatus)> &result_handler);

FSStatus FSFlushFileWrapper(FSFileHandle handle,
                            FSErrorFlag errorMask,
                            const std::function<FSStatus(FSStatus)> &result_handler);

int32_t getNewDirHandleIndex();

int32_t getNewFileHandleIndex();

bool isValidDirHandle(int32_t handle);

bool isValidFileHandle(int32_t handle);