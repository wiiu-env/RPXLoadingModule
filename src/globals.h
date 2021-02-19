#include <wums.h>
#include <coreinit/mutex.h>

typedef struct MetaInformation_t {
    char shortname[64];
    char longname[64];
    char author[64];
} MetaInformation;

typedef struct BundleMountInformation_t {
    bool redirectionRequested;
    bool isMounted;
    char path[255];
    char workingDir[255];
} BundleMountInformation;

typedef enum RPXLoader_Type {
    RPXLoader_NONE,
    RPXLoader_RPX,
    RPXLoader_BUNDLE,
    RPXLoader_BUNDLE_OTHER_RPX,
} RPXLoader_Type;

typedef struct RPXLoader_ReplacementInformation_t {
    bool isRPXReplaced;
    MetaInformation metaInformation;
    RPXLoader_Type replacementType;
    BundleMountInformation bundleMountInformation;
    char iconCache[65580];
    uint32_t currentHash;
    char savePath[255];
} RPXLoader_ReplacementInformation;

extern RPXLoader_ReplacementInformation gReplacementInfo;