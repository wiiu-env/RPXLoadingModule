#pragma once
#include <coreinit/filesystem.h>
#include <functional>
#include <romfs_dev.h>
#include <string>

int32_t CreateSubfolder(const char *fullpath);

int32_t getRPXInfoForPath(const std::string &path, romfs_fileInfo *info);

int32_t CheckFile(const char *filepath);

int32_t LoadFileToMem(const char *filepath, uint8_t **inbuffer, uint32_t *size);