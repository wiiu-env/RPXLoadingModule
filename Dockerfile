FROM wiiuenv/devkitppc:20210920

COPY --from=wiiuenv/librpxloader:20210924 /artifacts $DEVKITPRO
COPY --from=wiiuenv/libfunctionpatcher:20210924 /artifacts $DEVKITPRO
COPY --from=wiiuenv/wiiumodulesystem:20210924 /artifacts $DEVKITPRO
COPY --from=wiiuenv/libromfs_wiiu:20210924 /artifacts $DEVKITPRO

WORKDIR project