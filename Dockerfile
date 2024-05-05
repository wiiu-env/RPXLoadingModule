FROM ghcr.io/wiiu-env/devkitppc:20240505

COPY --from=ghcr.io/wiiu-env/librpxloader:20240425 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/libfunctionpatcher:20230621 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/wiiumodulesystem:20240424 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/libwuhbutils:20230621 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/libcontentredirection:20240424 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/libromfs_wiiu:20240505 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/libmocha:20231127 /artifacts $DEVKITPRO

WORKDIR project
