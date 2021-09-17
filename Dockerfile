FROM wiiuenv/devkitppc:20210917

COPY --from=wiiuenv/librpxloader:20210406 /artifacts $DEVKITPRO
COPY --from=wiiuenv/libfunctionpatcher:20210109 /artifacts $DEVKITPRO
COPY --from=wiiuenv/wiiumodulesystem:20210917 /artifacts $DEVKITPRO
COPY --from=wiiuenv/libromfs_wiiu:20210110 /artifacts $DEVKITPRO

WORKDIR project