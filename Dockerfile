FROM wiiuenv/devkitppc:20220507

COPY --from=wiiuenv/librpxloader:20220417 /artifacts $DEVKITPRO
COPY --from=wiiuenv/libfunctionpatcher:20220507 /artifacts $DEVKITPRO
COPY --from=wiiuenv/wiiumodulesystem:20220507 /artifacts $DEVKITPRO
COPY --from=wiiuenv/libwuhbutils:20220415 /artifacts $DEVKITPRO
COPY --from=wiiuenv/libcontentredirection:20220414 /artifacts $DEVKITPRO
COPY --from=wiiuenv/libromfs_wiiu:20220414 /artifacts $DEVKITPRO

WORKDIR project