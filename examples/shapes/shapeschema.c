#define JIPG_STRIP_PREFIX
#include "../../jipg.h"

PARSER(Shapes,
       ARRAY(
           OBJECT(
               KV("sides", INT()),
               KV("radius", FLOAT()),
               KV("coord",
                  OBJECT(
                      KV("x", FLOAT()),
                      KV("y", FLOAT()))))))

JIPG_MAIN()
