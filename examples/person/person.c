#include <stdio.h>
#include <stdlib.h>

#include "person_parser.h"

static char *read_file_to_buffer(const char *filename, size_t *size_out);

int main(void) {
    size_t file_size;
    const char *json = read_file_to_buffer("person.json", &file_size);

    Person p = {0};
    if (!parse_Person(json, file_size, &p)) {
        return 1;
    }

    printf(
        "Name: %s\n"
        "Age: %ld\n",
        p.name, p.age);

    printf("Friends: ");
    for (size_t i = 0; i < p.friends.len; ++i) {
        printf("%s", p.friends.items[i]);

        if (i < p.friends.len - 1)
            printf(", ");
    }
    printf("\n");

    printf("Parents: ");
    for (size_t i = 0; i < p.parents.len; ++i) {
        printf("%s", p.parents.items[i]);

        if (i < p.parents.len - 1)
            printf(", ");
    }
    printf("\n");

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
