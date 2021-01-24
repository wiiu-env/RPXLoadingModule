#pragma once

#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

class FileReader {

public:
    FileReader(uint8_t *buffer, uint32_t size);
    explicit FileReader(std::string &path);

    virtual ~FileReader();

    virtual int read(uint8_t *buffer, uint32_t size) ;

private:
    bool isReadFromBuffer = false;
    uint8_t *input_buffer = nullptr;
    uint32_t input_size = 0;
    uint32_t input_pos = 0;

    bool isReadFromFile = false;
    int file_fd = 0;
};
