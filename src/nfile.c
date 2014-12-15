#include <stdio.h>
#include <stdlib.h>

#include "nfile.h"

char *nfile_read(const char* fname) {
    FILE *fp = fopen(fname, "rb");
    if (!fp) {
        perror(fname);
        exit(1);
    }

    fseek(fp, 0L, SEEK_END);
    long size = ftell(fp);
    rewind(fp);

    // Allocate memory for entire content
    char *buffer = calloc(1, size + 1);
    if (!buffer) {
        fclose(fp);
        fputs("ERR: nfile_read: failed to allocate memory.", stderr);
        exit(1);
    }

    // Copy the file into the buffer
    if (fread(buffer, size, 1, fp) != 1) {
        fclose(fp);
        free(buffer);
        fputs("ERR: nfile_read: failed to read file.", stderr);
        exit(1);
    }

    fclose(fp);
    return buffer;
}
