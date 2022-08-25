FROM wiiuenv/devkitppc:20220806

COPY --from=wiiuenv/librpxloader:20220825 /artifacts $DEVKITPRO
COPY --from=wiiuenv/libfunctionpatcher:20220724 /artifacts $DEVKITPRO
COPY --from=wiiuenv/wiiumodulesystem:20220724 /artifacts $DEVKITPRO
COPY --from=wiiuenv/libwuhbutils:20220724 /artifacts $DEVKITPRO
COPY --from=wiiuenv/libcontentredirection:20220724 /artifacts $DEVKITPRO
COPY --from=wiiuenv/libromfs_wiiu:20220724 /artifacts $DEVKITPRO
COPY --from=wiiuenv/libmocha:20220728 /artifacts $DEVKITPRO

WORKDIR project
