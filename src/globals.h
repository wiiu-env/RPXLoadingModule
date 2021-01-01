#include <wums.h>
#include <coreinit/mutex.h>

extern bool gIsMounted;
extern bool gTryToReplaceOnNextLaunch;
extern char gLoadedBundlePath[256];
extern char gSavePath[256];
extern char gWorkingDir[256];
extern char gTempPath[256];
extern uint32_t gCurrentHash;