#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include "streamstreams.h"

int main(int argc, char* argv[]) {
    struct stream_header header;
    struct stream_marker marker;
    size_t name_size, block_size;
    int childstatus;
    int childpipe[2];
    pid_t child = 0;
    char *name = NULL;
    void *buffer = NULL;
    int ret = 0;

    childpipe[1] = -1;

    marker.marker = htonl(STREAMSTREAMS_MARKER);

    if(argc < 2) {
        fprintf(stderr, "please specify a command to pipe decapsulated streams to.\n");
        goto error;
    }

    if(fread(&header, sizeof(header), 1, stdin) < 1) {
        if(!feof(stdin))
            perror("reading stream header");
        goto error;
    }

    if(header.version != STREAMSTREAMS_VERSION) {
        fprintf(stderr, "unsupported version %d.\n", (int) header.version);
        goto error;
    }

    name_size = ntohs(header.name_size);
    if(name_size > 0) {
        name = malloc(name_size);
        if(name == NULL) {
            perror("allocating memory for stream name");
            goto error;
        }
        if(fread(name, name_size, 1, stdin) < 1) {
            fprintf(stderr, "error reading stream name.\n");
            goto error;
        }
    } else {
        name = NULL;
    }

    block_size = 1 << header.block_size;
    buffer = malloc(block_size);
    if(buffer == NULL) {
        perror("allocating memory for buffer");
        goto error;
    }

    if(pipe(childpipe) == -1) {
        perror("creating pipe for child");
        goto error;
    }
    
    child = fork();
    if(child == -1) {
        perror("forking subprocess");
        goto error;
    }
    if(child == 0) {
        // in child process
        if(close(childpipe[1]) < 0 || dup2(childpipe[0], 0) < 0) {
            perror("redirecting child's stdin");
            exit(1);
        }
        if(name != NULL) {
            setenv(STREAMSTREAMS_ENV_NAME, name, 1);
        }
        if(execvp(argv[1], &argv[1]) < 0) {
            perror("executing child process");
            exit(1);
        }
        exit(0); // should not ever execute
    }

    // parent process:

    while(1) {
        if(fread(buffer, sizeof(marker.marker), 1, stdin) < 1) {
            fprintf(stderr, "unexpected end of stream.\n");
            goto error;
        }
        if(memcmp(buffer, &marker.marker, sizeof(marker.marker)) == 0) {
            if(fread((void*)&marker + sizeof(marker.marker), sizeof(marker) - sizeof(marker.marker), 1, stdin) < 1) {
                fprintf(stderr, "unexpected end of stream when reading marker data.\n");
                goto error;
            }
            if(marker.type == STREAMSTREAMS_MARKERTYPE_NOMARKER) {
                if(fread(buffer + sizeof(marker.marker), block_size - sizeof(marker.marker), 1, stdin) < 1) {
                    fprintf(stderr, "unexpected read error.\n");
                    goto error;
                }
                write(childpipe[1], buffer, block_size);
            } else if(marker.type == STREAMSTREAMS_MARKERTYPE_EOS) {
                if(fread(buffer, ntohs(marker.data), 1, stdin) < 1) {
                    fprintf(stderr, "unexpected read error.\n");
                    goto error;
                }
                write(childpipe[1], buffer, ntohs(marker.data));
                goto quit;
            } else {
                fprintf(stderr, "unsupported marker type %d.\n", (int) marker.type);
                goto error;
            }
        } else {
            if(fread(buffer + sizeof(marker.marker), block_size - sizeof(marker.marker), 1, stdin) < 1) {
                fprintf(stderr, "unexpected read error.\n");
                goto error;
            }
            write(childpipe[1], buffer, block_size);
        }
    }

error:
    ret = -1;

quit:
    if(childpipe[1] != -1) {
        close(childpipe[1]);
    }

    wait(&childstatus);
    if(ret == 0) {
        // pass through child's return value
        if(WIFEXITED(childstatus)) ret = WEXITSTATUS(childstatus);
    }

    if(name != NULL) free(name);
    if(buffer != NULL) free(buffer);

    return ret;
}
