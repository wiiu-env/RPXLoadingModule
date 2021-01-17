#include "FileReader.h"
#include "logger.h"
#include <string.h>
#include <errno.h>

int FileReader::read(uint8_t *buffer, int size) {
    if (isReadFromBuffer) {
        if (input_buffer == nullptr) {
            return -1;
        }
        uint32_t toRead = size;
        if (toRead > input_size - input_pos) {
            toRead = input_size - input_pos;
        }
        if (toRead == 0) {
            return 0;
        }
        memcpy(buffer, input_buffer, toRead);
        input_pos += toRead;
        return toRead;
    } else if (isReadFromFile) {
        int res = ::read(file_fd, buffer, size);
        return res;
    }
    return -2;
}

FileReader::FileReader(std::string &path) {
    int fd = -1;
    if ((fd = open(path.c_str(), O_RDONLY)) >= 0) {
        this->isReadFromFile = true;
        this->isReadFromBuffer = false;
        this->file_fd = fd;
    } else {
        DEBUG_FUNCTION_LINE("Failed to open file %s", path.c_str());
    }
}

FileReader::~FileReader() {
    if (isReadFromFile) {
        ::close(this->file_fd);
    }
}

FileReader::FileReader(uint8_t *buffer, uint32_t size) {
    this->input_buffer = buffer;
    this->input_size = size;
    this->input_pos = 0;
    this->isReadFromBuffer = false;
    this->isReadFromFile = false;
}