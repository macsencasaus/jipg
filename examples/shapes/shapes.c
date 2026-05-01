#include <stdio.h>
#include <stdlib.h>

#define SHAPES_PARSER_IMPLEMENTATION
#include "shapes_parser.h"

static char *read_file_to_buffer(const char *filename, size_t *size_out);

int main(void) {
    size_t file_size;
    const char *json = read_file_to_buffer("shapes.json", &file_size);

    Shapes shapes = {0};
    if (!parse_Shapes(json, file_size, &shapes)) {
        fprintf(stderr, "Error\n");
        return 1;
    }

    printf("Shapes(%zu):\n", shapes.len);
    for (size_t i = 0; i < shapes.len; ++i) {
        Shape shape = shapes.items[i];
        Coordinates coords = shape.coords;
        printf(
            "  side: %ld\n"
            "  radius: %g\n"
            "  x: %g\n"
            "  y: %g\n\n",
            shape.sides,
            shape.radius,
            coords.x,
            coords.y);
    }

    return 0;
}

static char *read_file_to_buffer(const char *filename, size_t *size_out) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("fopen");
        return NULL;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        perror("fseek");
        fclose(fp);
        return NULL;
    }

    long filesize = ftell(fp);
    if (filesize < 0) {
        perror("ftell");
        fclose(fp);
        return NULL;
    }
    rewind(fp);

    char *buffer = (char *)malloc(filesize + 1);
    if (!buffer) {
        perror("malloc");
        fclose(fp);
        return NULL;
    }

    size_t read_size = fread(buffer, 1, filesize, fp);
    if (read_size != (size_t)filesize) {
        perror("fread");
        free(buffer);
        fclose(fp);
        return NULL;
    }

    buffer[filesize] = '\0';

    fclose(fp);
    if (size_out) {
        *size_out = (size_t)filesize;
    }
    return buffer;
}
