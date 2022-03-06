FROM wiiuenv/devkitppc:20220303

COPY --from=wiiuenv/librpxloader:20220212 /artifacts $DEVKITPRO
COPY --from=wiiuenv/libfunctionpatcher:20220211 /artifacts $DEVKITPRO
COPY --from=wiiuenv/wiiumodulesystem:20220204 /artifacts $DEVKITPRO
COPY --from=wiiuenv/libromfs_wiiu:20220305 /artifacts $DEVKITPRO

WORKDIR project