#include "globals.h"

bool gIsMounted __attribute__((section(".data"))) = false;
uint32_t gCurrentHash __attribute__((section(".data"))) = 0;
bool gTryToReplaceOnNextLaunch __attribute__((section(".data"))) = false;
char gLoadedBundlePath[256] __attribute__((section(".data")));
char gSavePath[256] __attribute__((section(".data")));
char gWorkingDir[256] __attribute__((section(".data")));

bool gReplacedRPX  __attribute__((section(".data"))) = false;
BundleInformation gBundleInfo  __attribute__((section(".data")));