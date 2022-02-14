#include "utils/utils.h"
#include <coreinit/filesystem.h>
#include <coreinit/mutex.h>
#include <wums.h>
#include <wut.h>

typedef struct WUT_PACKED MetaInformation_t {
    char shortname[64];
    char longname[64];
    char author[64];
} MetaInformation;
WUT_CHECK_SIZE(MetaInformation_t, 0xC0);

typedef struct BundleMountInformation_t {
    bool isMounted;
    char toMountPath[255];
    char mountedPath[255];
} BundleMountInformation;

#define ICON_SIZE 65580

typedef struct WUT_PACKED RPXReplacementInfo_t {
    bool willRPXBeReplaced;
    bool isRPXReplaced;
    MetaInformation metaInformation;
    WUT_UNKNOWN_BYTES(0x3E);
    char iconCache[ROUNDUP(ICON_SIZE, 0x040)];
} RPXReplacementInfo;
// make sure the iconCache is aligned to 0x40
WUT_CHECK_OFFSET(RPXReplacementInfo, 0x100, iconCache);


typedef enum ContentRedirect_Mode {
    CONTENTREDIRECT_NONE,
    CONTENTREDIRECT_FROM_WUHB_BUNDLE,
    CONTENTREDIRECT_FROM_PATH,
} ContentRedirect_Mode;

typedef struct ContentReplacementInfo_t {
    ContentRedirect_Mode mode;

    BundleMountInformation bundleMountInformation;

    char workingDir[255];

    char replacementPath[255];

    bool replaceSave;

    char savePath[255];

    bool fallbackOnError;
} ContentReplacementInfo;

typedef struct RPXLoader_ReplacementInformation_t {
    RPXReplacementInfo rpxReplacementInfo;
    ContentReplacementInfo contentReplacementInfo;
} RPXLoader_ReplacementInformation;


extern RPXLoader_ReplacementInformation gReplacementInfo;
extern FSClient *gFSClient;
extern FSCmdBlock *gFSCmd;