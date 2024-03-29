#pragma once

#include <fcntl.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

class FileReader {

public:
    FileReader(uint8_t *buffer, uint32_t size);

    explicit FileReader(std::string &path);

    virtual ~FileReader();

    virtual int64_t read(uint8_t *buffer, uint32_t size);

    uint32_t getHandle() {
        return (uint32_t) this;
    }

private:
    bool isReadFromBuffer = false;
    uint8_t *input_buffer = nullptr;
    uint32_t input_size   = 0;
    uint32_t input_pos    = 0;

    bool isReadFromFile = false;
    int file_fd         = -1;
};
