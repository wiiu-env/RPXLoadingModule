#include "globals.h"

RPXLoader_ReplacementInformation gReplacementInfo __attribute__((section(".data")));
CRLayerHandle contentLayerHandle __attribute__((section(".data"))) = 0;
CRLayerHandle saveLayerHandle __attribute__((section(".data")))    = 0;