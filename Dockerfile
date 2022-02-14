FROM wiiuenv/devkitppc:20220213

COPY --from=wiiuenv/librpxloader:20220212 /artifacts $DEVKITPRO
COPY --from=wiiuenv/libfunctionpatcher:20210924 /artifacts $DEVKITPRO
COPY --from=wiiuenv/wiiumodulesystem:20220123 /artifacts $DEVKITPRO
COPY --from=wiiuenv/libromfs_wiiu:20220213 /artifacts $DEVKITPRO

WORKDIR project