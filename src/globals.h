#include "utils/utils.h"
#include <content_redirection/redirection.h>
#include <coreinit/filesystem.h>
#include <coreinit/mutex.h>
#include <wums.h>
#include <wut.h>

#define WUHB_ROMFS_NAME         "wuhbrom"
#define WUHB_ROMFS_PATH         WUHB_ROMFS_NAME ":"
#define WUHB_ROMFS_CONTENT_PATH WUHB_ROMFS_PATH "/content"

typedef struct WUT_PACKED MetaInformation_t {
    char shortname[64];
    char longname[64];
    char author[64];
} MetaInformation;
WUT_CHECK_SIZE(MetaInformation_t, 0xC0);

typedef struct ContentRedirectionInformation_t {
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

typedef struct ContentReplacementInfo_t {
    BundleMountInformation bundleMountInformation;
    char replacementPath[0x280];
} ContentReplacementInfo;

typedef struct ContentReplacementWithFallback_t {
    char replacementPath[0x280];
} ContentReplacementWithFallback;

typedef struct RPXLoader_ReplacementInformation_t {
    RPXReplacementInfo rpxReplacementInfo;
    ContentReplacementInfo contentReplacementInfo;
    ContentReplacementWithFallback contentReplacementWithFallbackInfo;
} RPXLoader_ReplacementInformation;

extern RPXLoader_ReplacementInformation gReplacementInfo;

extern CRLayerHandle contentLayerHandle;
extern CRLayerHandle saveLayerHandle;