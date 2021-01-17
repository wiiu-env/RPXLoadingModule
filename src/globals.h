#include <wums.h>
#include <coreinit/mutex.h>

typedef struct BundleInformation_t {
    char shortname[64];
    char longname[64];
    char author[64];
} BundleInformation;


extern bool gIsMounted;
extern bool gTryToReplaceOnNextLaunch;
extern char gLoadedBundlePath[256];
extern char gSavePath[256];
extern char gWorkingDir[256];
extern uint32_t gCurrentHash;
extern bool gReplacedRPX;
extern BundleInformation gBundleInfo;