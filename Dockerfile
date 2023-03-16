FROM ghcr.io/wiiu-env/devkitppc:20221228

COPY --from=ghcr.io/wiiu-env/librpxloader:20230218 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/libfunctionpatcher:20230108 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/wiiumodulesystem:20230106 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/libwuhbutils:20220904 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/libcontentredirection:20221010 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/libromfs_wiiu:20220904 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/libmocha:20220919 /artifacts $DEVKITPRO

WORKDIR project
