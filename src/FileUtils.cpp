#include "FileUtils.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <mutex>

#include <coreinit/debug.h>
#include <map>
#include <coreinit/thread.h>
#include <dirent.h>
#include <coreinit/cache.h>
#include "utils/StringTools.h"
#include "utils/utils.h"
#include "utils/logger.h"

extern "C" OSMessageQueue *OSGetDefaultAppIOQueue();

FSCmdBlockBody *fsCmdBlockGetBody(FSCmdBlock *cmdBlock) {
    if (!cmdBlock) {
        return nullptr;
    }

    auto body = (FSCmdBlockBody *) (ROUNDUP((uint32_t) cmdBlock, 0x40));
    return body;
}

std::mutex sendMutex;

FSStatus send_result_async(FSClient *client, FSCmdBlock *block, FSAsyncData *asyncData, FSStatus status) {
    if (asyncData->callback != nullptr) {
        if (asyncData->ioMsgQueue != nullptr) {
            //OSFatal("ERROR: callback and ioMsgQueue both set.");
            return FS_STATUS_FATAL_ERROR;
        }
        // userCallbacks are called in the DefaultAppIOQueue.
        asyncData->ioMsgQueue = OSGetDefaultAppIOQueue();
        //DEBUG_FUNCTION_LINE("Force to OSGetDefaultAppIOQueue (%08X)", asyncData->ioMsgQueue);
    }

    if (asyncData->ioMsgQueue != nullptr) {
#pragma GCC diagnostic ignored "-Waddress-of-packed-member"
        FSAsyncResult *result = &(fsCmdBlockGetBody(block)->asyncResult);
        //DEBUG_FUNCTION_LINE("Send result %d to ioMsgQueue (%08X)", status, asyncData->ioMsgQueue);
        result->asyncData.callback = asyncData->callback;
        result->asyncData.param = asyncData->param;
        result->asyncData.ioMsgQueue = asyncData->ioMsgQueue;
        memset(&result->ioMsg, 0, sizeof(result->ioMsg));
        result->ioMsg.data = result;
        result->ioMsg.type = OS_FUNCTION_TYPE_FS_CMD_ASYNC;
        result->client = client;
        result->block = block;
        result->status = status;

        while (!OSSendMessage(asyncData->ioMsgQueue, (OSMessage * ) & (result->ioMsg), OS_MESSAGE_FLAGS_NONE)) {
            DEBUG_FUNCTION_LINE("Failed to send message");
        }
    }
    return FS_STATUS_OK;
}

int32_t readIntoBuffer(int32_t handle, void *buffer, size_t size, size_t count) {
    int32_t sizeToRead = size * count;
    /*
    // https://github.com/decaf-emu/decaf-emu/blob/131aeb14fccff8461a5fd9f2aa5c040ba3880ef5/src/libdecaf/src/cafe/libraries/coreinit/coreinit_fs_cmd.cpp#L2346
    if (sizeToRead > 0x100000) {
        sizeToRead = 0x100000;
    }*/
    void *newBuffer = buffer;
    int32_t curResult = -1;
    int32_t totalSize = 0;
    // int32_t toRead = 0;
    while (sizeToRead > 0) {
        curResult = read(handle, newBuffer, sizeToRead);
        if (curResult < 0) {
            DEBUG_FUNCTION_LINE("Error: Reading %08X bytes from handle %08X. result %08X errno: %d ", size * count, handle, curResult, errno);
            return -1;
        }
        if (curResult == 0) {
            //DEBUG_FUNCTION_LINE("EOF");
            break;
        }
        newBuffer = (void *) (((uint32_t) newBuffer) + curResult);
        totalSize += curResult;
        sizeToRead -= curResult;
        if (sizeToRead > 0) {
            //DEBUG_FUNCTION_LINE("Reading. missing %08X bytes\n", sizeToRead);
        }
    }
    //DEBUG_FUNCTION_LINE("Success: Read %08X bytes from handle %08X. result %08X \n", size * count, handle, totalSize);
    return totalSize;
}

int32_t CheckFile(const char *filepath) {
    if (!filepath) {
        return 0;
    }

    struct stat filestat{};

    char dirnoslash[strlen(filepath) + 2];
    snprintf(dirnoslash, sizeof(dirnoslash), "%s", filepath);

    while (dirnoslash[strlen(dirnoslash) - 1] == '/') {
        dirnoslash[strlen(dirnoslash) - 1] = '\0';
    }

    char *notRoot = strrchr(dirnoslash, '/');
    if (!notRoot) {
        strcat(dirnoslash, "/");
    }

    if (stat(dirnoslash, &filestat) == 0) {
        return 1;
    }

    return 0;
}

int32_t CreateSubfolder(const char *fullpath) {
    if (!fullpath)
        return 0;

    int32_t result = 0;

    char dirnoslash[strlen(fullpath) + 1];
    strcpy(dirnoslash, fullpath);

    int32_t pos = strlen(dirnoslash) - 1;
    while (dirnoslash[pos] == '/') {
        dirnoslash[pos] = '\0';
        pos--;
    }

    if (CheckFile(dirnoslash)) {
        return 1;
    } else {
        char parentpath[strlen(dirnoslash) + 2];
        strcpy(parentpath, dirnoslash);
        char *ptr = strrchr(parentpath, '/');

        if (!ptr) {
            //!Device root directory (must be with '/')
            strcat(parentpath, "/");
            struct stat filestat{};
            if (stat(parentpath, &filestat) == 0) {
                return 1;
            }

            return 0;
        }

        ptr++;
        ptr[0] = '\0';

        result = CreateSubfolder(parentpath);
    }

    if (!result) {
        return 0;
    }

    if (mkdir(dirnoslash, 0777) == -1) {
        return 0;
    }

    return 1;
}


int32_t getRPXInfoForPath(const std::string &path, romfs_fileInfo *info) {
    if (romfsMount("rcc", path.c_str(), RomfsSource_FileDescriptor_CafeOS) < 0) {
        return -1;
    }
    DIR *dir;
    struct dirent *entry;

    if (!(dir = opendir("rcc:/code/"))) {
        romfsUnmount("rcc");
        return -2;
    }
    bool found = false;
    int res = -3;
    while ((entry = readdir(dir)) != nullptr) {
        if (StringTools::EndsWith(entry->d_name, ".rpx")) {
            if (romfsGetFileInfoPerPath("rcc", (std::string("code/") + entry->d_name).c_str(), info) >= 0) {
                found = true;
                res = 0;
            } else {
                DEBUG_FUNCTION_LINE("Failed to get info for %s", entry->d_name);
            }
            break;
        }
    }

    closedir(dir);

    romfsUnmount("rcc");

    if (!found) {
        return -4;
    }
    return res;
}