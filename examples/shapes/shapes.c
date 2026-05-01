#include <stdio.h>
#include <stdlib.h>

#define SHAPES_PARSER_IMPLEMENTATION
#include "shapes_parser.h"

int main(void) {
    const char *json =
        "[\n"
        "    {\n"
        "        \"sides\": 5,\n"
        "        \"radius\": 14.2,\n"
        "        \"coords\": {\n"
        "            \"x\": -3.4,\n"
        "            \"y\": 7.8\n"
        "        }\n"
        "    },\n"
        "    {\n"
        "        \"sides\": 8,\n"
        "        \"radius\": 22.5,\n"
        "        \"coords\": {\n"
        "            \"x\": 12.1,\n"
        "            \"y\": -4.6\n"
        "        }\n"
        "    },\n"
        "    {\n"
        "        \"sides\": 4,\n"
        "        \"radius\": 9.7,\n"
        "        \"coords\": {\n"
        "            \"x\": 0.0,\n"
        "            \"y\": 15.3\n"
        "        }\n"
        "    },\n"
        "    {\n"
        "        \"sides\": 6,\n"
        "        \"radius\": 30.0,\n"
        "        \"coords\": {\n"
        "            \"x\": -10.2,\n"
        "            \"y\": -10.2\n"
        "        }\n"
        "    },\n"
        "    {\n"
        "        \"sides\": 7,\n"
        "        \"radius\": 18.9,\n"
        "        \"coords\": {\n"
        "            \"x\": 5.5,\n"
        "            \"y\": -2.7\n"
        "        }\n"
        "    }\n"
        "]";

    Shapes shapes = {0};
    if (!parse_Shapes_cstr(json, &shapes)) {
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
