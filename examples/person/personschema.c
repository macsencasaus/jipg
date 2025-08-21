#include "../../jipg.h"

JIPG_PARSER(Person,
            JIPG_OBJECT(
                JIPG_OBJECT_KV("name", JIPG_STRING()),
                JIPG_OBJECT_KV("age", JIPG_INT()),
                JIPG_OBJECT_KV("friends", JIPG_ARRAY(JIPG_STRING())),
                JIPG_OBJECT_KV("parents", JIPG_ARRAY_CAP(JIPG_STRING(), 2))
            ))

JIPG_MAIN()
