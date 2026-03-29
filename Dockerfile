FROM ghcr.io/wiiu-env/devkitppc:20260225

COPY --from=ghcr.io/wiiu-env/librpxloader:20260329 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/libfunctionpatcher:20260208 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/wiiumodulesystem:20260225 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/libwuhbutils:20260329 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/libcontentredirection:20260329 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/libromfs_wiiu:20260329 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/libmocha:20260126  /artifacts $DEVKITPRO

WORKDIR project
