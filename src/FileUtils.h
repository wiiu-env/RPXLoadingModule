#pragma once

#include <coreinit/filesystem.h>
#include <romfs_dev.h>
#include <string>

struct WUT_PACKED FSCmdBlockBody { //! FSAsyncResult object used for this command.

    WUT_UNKNOWN_BYTES(0x96C);
    FSAsyncResult asyncResult;
};
WUT_CHECK_OFFSET(FSCmdBlockBody, 0x96C, asyncResult);
WUT_CHECK_SIZE(FSCmdBlockBody, 0x96C + 0x28);

FSCmdBlockBody *fsCmdBlockGetBody(FSCmdBlock *cmdBlock);

FSStatus send_result_async(FSClient *client, FSCmdBlock *block, FSAsyncData *asyncData, FSStatus result);

int32_t readIntoBuffer(int32_t handle, void *buffer, size_t size, size_t count);

int32_t CreateSubfolder(const char *fullpath);

int32_t getRPXInfoForPath(const std::string &path, romfs_fileInfo *info);

int32_t CheckFile(const char *filepath);