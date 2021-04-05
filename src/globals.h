#include <wums.h>
#include <coreinit/mutex.h>

typedef struct MetaInformation_t {
    char shortname[64];
    char longname[64];
    char author[64];
} MetaInformation;

typedef struct BundleMountInformation_t {
    bool isMounted;
    char toMountPath[255];
    char mountedPath[255];
} BundleMountInformation;

typedef struct RPXReplacementInfo_t {
    bool isRPXReplaced;
    MetaInformation metaInformation;
    char iconCache[65580];
} RPXReplacementInfo;

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