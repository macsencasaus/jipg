#define JIPG_STRIP_PREFIX
#include "../../jipg.h"

PARSER(Coordinates,
       OBJECT(
           KV("x", FLOAT()),
           KV("y", FLOAT())))

PARSER(Shape,
       OBJECT(
           KV("sides", INT()),
           KV("radius", FLOAT()),
           KV("coords", USE(Coordinates))))

PARSER(Shapes, ARRAY(USE(Shape)))

JIPG_DESC("JSON parser generator for shape object.")
JIPG_MAIN("--single-file", "--header=shapes_parser.h")
