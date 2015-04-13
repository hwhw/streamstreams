#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "streamstreams.h"

int main(int argc, char* argv[]) {
    struct stream_header *header;
    struct stream_marker marker;
    size_t name_len;
    size_t header_size;
    size_t block_size, read;
    void *buffer;

    if(argc < 2) {
        name_len = 0;
    } else {
        name_len = strlen(argv[1]);
        if(name_len > 0xFFFF) {
            fprintf(stderr, "given stream name is too long.\n");
            exit(1);
        }
    }

    header_size = sizeof(struct stream_header) + name_len;
    header = (struct stream_header *) malloc(header_size);
    if(header == NULL) {
        perror("allocating memory for header");
        exit(-1);
    }

    block_size = 1 << STREAMSTREAMS_BLOCKSIZE_EXP;
    buffer = malloc(block_size);
    if(buffer == NULL) {
        perror("allocating memory for buffer");
        exit(-1);
    }

    header->version = STREAMSTREAMS_VERSION;
    header->block_size = STREAMSTREAMS_BLOCKSIZE_EXP;
    header->marker = htonl(STREAMSTREAMS_MARKER);
    header->name_size = htons(name_len);
    if(name_len > 0) memcpy(header->name, argv[1], name_len);

    fwrite(header, header_size, 1, stdout);

    marker.marker = htonl(STREAMSTREAMS_MARKER);

    while(1) {
        read = fread(buffer, 1, block_size, stdin);

        if(read < block_size) {
                /* last block of data */
                /* write marker: */
                marker.type = STREAMSTREAMS_MARKERTYPE_EOS;
                marker.data = htons(read);
                marker.reserved = 0;
                fwrite(&marker, sizeof(marker), 1, stdout);
                fwrite(buffer, read, 1, stdout);
                goto quit;
        }

        if(memcmp(buffer, (void*) &marker.marker, sizeof(marker.marker)) == 0) {
            /* marker value in input stream */
            /* write marker: */
            marker.type = STREAMSTREAMS_MARKERTYPE_NOMARKER;
            marker.data = 0;
            marker.reserved = 0;
            fwrite(&marker, sizeof(marker), 1, stdout);
            /* write remainder: */
            fwrite(buffer + sizeof(marker.marker), read - sizeof(marker.marker), 1, stdout);
        } else {
            fwrite(buffer, read, 1, stdout);
        }
    }

quit:
    free(header);
    free(buffer);
    exit(0);
}
