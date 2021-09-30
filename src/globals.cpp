#include <coreinit/filesystem.h>
#include "globals.h"

RPXLoader_ReplacementInformation gReplacementInfo __attribute__((section(".data")));

FSClient * gFSClient __attribute__((section(".data"))) = nullptr;
FSCmdBlock * gFSCmd __attribute__((section(".data"))) = nullptr;