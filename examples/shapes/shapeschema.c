#include "../../jipg.h"

JIPG_PARSER(Shapes,
            JIPG_ARRAY(
                JIPG_OBJECT(
                    JIPG_OBJECT_KV("sides", JIPG_INT()),
                    JIPG_OBJECT_KV("radius", JIPG_FLOAT()),
                    JIPG_OBJECT_KV("coord",
                                   JIPG_OBJECT(
                                       JIPG_OBJECT_KV("x", JIPG_FLOAT()),
                                       JIPG_OBJECT_KV("y", JIPG_FLOAT()))))))

JIPG_MAIN()
