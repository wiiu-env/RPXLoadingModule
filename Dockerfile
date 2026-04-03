FROM ghcr.io/wiiu-env/devkitppc:20260225

COPY --from=ghcr.io/wiiu-env/librpxloader:20260329 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/libfunctionpatcher:20260331 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/wiiumodulesystem:reentfix-dev-20260403-5ca1144 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/libwuhbutils:20260329 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/libcontentredirection:abisafe-dev-20260403-502a496 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/libromfs_wiiu:20260329 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/libmocha:20260331  /artifacts $DEVKITPRO

WORKDIR project
