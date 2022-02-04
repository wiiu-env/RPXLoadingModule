#pragma once

#include "FileReader.h"
#include "logger.h"
#include <zlib.h>

#define BUFFER_SIZE 0x20000

class FileReaderCompressed : public FileReader {
public:
    FileReaderCompressed(uint8_t *buffer, uint32_t size);

    explicit FileReaderCompressed(std::string &file);

    ~FileReaderCompressed() override = default;

    int read(uint8_t *buffer, uint32_t size) override;

private:
    bool initDone = false;
    uint8_t zlib_in_buf[BUFFER_SIZE]{};
    z_stream strm{};

    void initCompressedData();
};
