#include <stdio.h>
#include <stdlib.h>

#include "person_parser.h"

int main(void) {
    const char *json =
        "{\n"
        "    \"name\": \"Sam\",\n"
        "    \"age\": 20,\n"
        "    \"friends\": [\"Alan\", \"Alex\", \"Dan\"],\n"
        "    \"parents\": [\"Homer\", \"Marge\"]\n"
        "}";

    Person p = {0};
    if (!parse_Person_cstr(json, &p)) {
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
