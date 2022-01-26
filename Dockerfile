FROM wiiuenv/devkitppc:20211229

COPY --from=wiiuenv/librpxloader:20211002 /artifacts $DEVKITPRO
COPY --from=wiiuenv/libfunctionpatcher:20210924 /artifacts $DEVKITPRO
COPY --from=wiiuenv/wiiumodulesystem:20220123 /artifacts $DEVKITPRO
COPY --from=wiiuenv/libromfs_wiiu:20211227 /artifacts $DEVKITPRO

WORKDIR project