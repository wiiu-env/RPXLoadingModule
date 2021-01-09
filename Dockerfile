FROM wiiuenv/devkitppc:20210101

COPY --from=wiiuenv/libfunctionpatcher:20210109 /artifacts $DEVKITPRO
COPY --from=wiiuenv/wiiumodulesystem:20210101 /artifacts $DEVKITPRO
COPY --from=wiiuenv/libromfs_wiiu:20210109 /artifacts $DEVKITPRO

WORKDIR project