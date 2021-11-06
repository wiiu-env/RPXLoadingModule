FROM wiiuenv/devkitppc:20211106

COPY --from=wiiuenv/librpxloader:20211002 /artifacts $DEVKITPRO
COPY --from=wiiuenv/libfunctionpatcher:20210924 /artifacts $DEVKITPRO
COPY --from=wiiuenv/wiiumodulesystem:20211031 /artifacts $DEVKITPRO
COPY --from=wiiuenv/libromfs_wiiu:20210924 /artifacts $DEVKITPRO

WORKDIR project