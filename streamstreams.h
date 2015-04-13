#include <stdint.h>

#define STREAMSTREAMS_ENV_NAME "STREAMNAME"

#define STREAMSTREAMS_VERSION 1
#define STREAMSTREAMS_BLOCKSIZE_EXP 12

struct __attribute__ ((__packed__)) stream_header {
    uint8_t version;
    uint8_t block_size;
    uint32_t marker;
    uint16_t name_size;
    char name[0];
};

/* from proper random data */
#define STREAMSTREAMS_MARKER 0x62dbbe1c

/* no marker intended: marker value is to be sent to output */
#define STREAMSTREAMS_MARKERTYPE_NOMARKER 0

/* end of stream */
#define STREAMSTREAMS_MARKERTYPE_EOS 1

struct __attribute__ ((__packed__)) stream_marker {
    uint32_t marker;
    uint8_t type;
    uint16_t data;
    uint8_t reserved;
};
