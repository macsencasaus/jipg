#include "../../jipg.h"

// clang-format off

JIPG_PARSER(Person,
            JIPG_OBJECT(
                JIPG_KV("name", JIPG_STRING()),
                JIPG_KV("age", JIPG_INT()),
                JIPG_KV("friends", JIPG_ARRAY(JIPG_STRING(), .alias = "friends_array")),
                JIPG_KV("parents", JIPG_ARRAY(JIPG_STRING(), .cap = 2, .nullable = true))
            ))

// clang-format on

JIPG_MAIN()
