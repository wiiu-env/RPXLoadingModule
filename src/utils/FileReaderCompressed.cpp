#include "FileReaderCompressed.h"

int FileReaderCompressed::read(uint8_t *buffer, int size) {
    int startValue = this->strm.total_out;
    uint32_t newSize = 0;
    int ret = 0;
    do {
        uint32_t nextOut = BUFFER_SIZE;
        if (nextOut > size) {
            nextOut = size;
        }
        if (this->strm.avail_in == 0) {
            this->strm.avail_in = FileReader::read(this->zlib_in_buf, BUFFER_SIZE);
            if (this->strm.avail_in == 0 || this->strm.avail_in == -1) {
                break;
            }
            this->strm.next_in = this->zlib_in_buf;
        }
        /* run inflate() on input until output buffer not full */
        do {
            if (nextOut > size - newSize) {
                nextOut = size - newSize;
            }

            this->strm.avail_out = nextOut;
            this->strm.next_out = buffer + newSize;
            ret = inflate(&this->strm, Z_NO_FLUSH);

            if (ret == Z_STREAM_ERROR) {
                DEBUG_FUNCTION_LINE("Z_STREAM_ERROR");
                return 0;
            }

            switch (ret) {
                case Z_NEED_DICT:
                    DEBUG_FUNCTION_LINE("Z_NEED_DICT");
                    ret = Z_DATA_ERROR;     /* and fall through */
                case Z_DATA_ERROR:
                case Z_MEM_ERROR:
                    DEBUG_FUNCTION_LINE("Z_MEM_ERROR or Z_DATA_ERROR");
                    (void) inflateEnd(&this->strm);
                    return ret;
            }

            newSize = this->strm.total_out - startValue;
            if (newSize == size) {
                break;
            }
            nextOut = BUFFER_SIZE;
            if (newSize + nextOut >= (size)) {
                nextOut = (size) - newSize;
            }
        } while (this->strm.avail_out == 0 && newSize < (size));

        /* done when inflate() says it's done */
    } while (ret != Z_STREAM_END && newSize < size);

    return newSize;
}

FileReaderCompressed::FileReaderCompressed(std::string &file) : FileReader(file) {
    this->initCompressedData();
}

void FileReaderCompressed::initCompressedData() {
    /* allocate inflate state */
    this->strm.zalloc = Z_NULL;
    this->strm.zfree = Z_NULL;
    this->strm.opaque = Z_NULL;
    this->strm.avail_in = 0;
    this->strm.next_in = Z_NULL;
    int ret = inflateInit2(&this->strm, MAX_WBITS | 16); //gzip
    if (ret != Z_OK) {
        DEBUG_FUNCTION_LINE("inflateInit2 failed");
        return;
    }
    initDone = true;
}

FileReaderCompressed::FileReaderCompressed(uint8_t *buffer, uint32_t size) : FileReader(buffer, size) {
    this->initCompressedData();
}