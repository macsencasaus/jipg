#include "jipg.h"

JIPG_PARSER(person_json_obj,
            JIPG_OBJECT(
                JIPG_OBJECT_KV("name", JIPG_STRING()),
                JIPG_OBJECT_KV("age", JIPG_INT()),
                JIPG_OBJECT_KV("friends", JIPG_ARRAY(JIPG_STRING()))
            ))

JIPG_MAIN()
