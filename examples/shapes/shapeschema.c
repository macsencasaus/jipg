#define JIPG_STRIP_PREFIX
#include "../../jipg.h"

// clang-format off

PARSER(Shapes,
       ARRAY(
           OBJECT(
               KV("sides", INT()),
               KV("radius", FLOAT()),
               KV("coord",
                  OBJECT(
                      KV("x", FLOAT()),
                      KV("y", FLOAT())
                  ))
            )
       ))

// clang-format on

JIPG_MAIN()
