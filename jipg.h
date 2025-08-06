#ifndef JIPG_H
#define JIPG_H

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifndef JIPG_DEFAULT_INT_TYPE
#define JIPG_DEFAULT_INT_TYPE "int64_t"
#endif

#ifndef JIPG_DEFAULT_FLOAT_TYPE
#define JIPG_DEFAULT_FLOAT_TYPE "double"
#endif

#ifndef JIPG_PARSER_CAP
#define JIPG_PARSER_CAP 8
#endif

#ifndef JIPG_REALLOC
#include <stdlib.h>
#define JIPG_REALLOC realloc
#endif

#ifndef JIPG_FREE
#include <stdlib.h>
#define JIPG_FREE free
#endif

#ifndef JIPG_ASSERT
#include <assert.h>
#define JIPG_ASSERT assert
#endif

#ifndef JIPG_VALUE_ARENA_CAP
#define JIPG_VALUE_ARENA_CAP 1024
#endif

#define UNREACHABLE()                                                                               \
    do {                                                                                            \
        fprintf(stderr, "UNREACHABLE CODE REACHED: %s:%d in %s()\n", __FILE__, __LINE__, __func__); \
        abort();                                                                                    \
    } while (0)

typedef enum {
    JIPG_KIND_OBJECT,
    JIPG_KIND_OBJECT_KV,
    JIPG_KIND_ARRAY,
    JIPG_KIND_STRING,
    JIPG_KIND_INT,
    JIPG_KIND_FLOAT,
    JIPG_KIND_BOOL,
    JIPG_KIND_VALUE_COUNT,
} Jipg_Value_Kind;

typedef struct Jipg_Value Jipg_Value;

struct Jipg_Value {
    Jipg_Value_Kind kind;

    union {
        struct {
            Jipg_Value *kv_head;
        } as_object;

        struct {
            const char *key;
            Jipg_Value *value;
            Jipg_Value *next;
        } as_object_kv;

        struct {
            bool has_cap;
            size_t cap;
            Jipg_Value *internal;
        } as_array;

        struct {
            bool has_cap;
            size_t cap;
        } as_string;

        struct {
            const char *type;
        } as_int;

        struct {
            const char *type;
        } as_float;
    };
};

typedef struct {
    const char *struct_name;
    Jipg_Value *(*value_gen)(void);
} Jipg_Parser;

typedef struct {
    size_t arena_size;
    Jipg_Value arena[JIPG_VALUE_ARENA_CAP];

    size_t value_count;
    Jipg_Parser values[JIPG_PARSER_CAP];
} Jipg_Context;

static Jipg_Context jipg_global_context = {0};

static inline Jipg_Value *new_jipg_value(Jipg_Value_Kind kind, ...) {
    JIPG_ASSERT(jipg_global_context.arena_size < JIPG_VALUE_ARENA_CAP);
    Jipg_Value *value = jipg_global_context.arena + jipg_global_context.arena_size++;
    *value = (Jipg_Value){.kind = kind};

    va_list args;
    va_start(args, kind);

    switch (kind) {
        case JIPG_KIND_VALUE_COUNT:
            UNREACHABLE();

        case JIPG_KIND_OBJECT: {
            size_t count = va_arg(args, size_t);

            Jipg_Value **last_next;
            for (size_t i = 0; i < count; ++i) {
                Jipg_Value *kv = va_arg(args, Jipg_Value *);
                if (i == 0) {
                    value->as_object.kv_head = kv;
                } else {
                    *last_next = kv;
                }
                last_next = &kv->as_object_kv.next;
            }
            value->as_object.kv_head = va_arg(args, Jipg_Value *);
        } break;

        case JIPG_KIND_OBJECT_KV: {
            value->as_object_kv.key = va_arg(args, char *);
            value->as_object_kv.value = va_arg(args, Jipg_Value *);
        } break;

        case JIPG_KIND_ARRAY: {
            value->as_array.has_cap = va_arg(args, bool);
            value->as_array.cap = va_arg(args, size_t);
            value->as_array.internal = va_arg(args, Jipg_Value *);
        } break;

        case JIPG_KIND_STRING: {
            value->as_string.has_cap = va_arg(args, bool);
            value->as_string.cap = va_arg(args, size_t);
        } break;

        case JIPG_KIND_INT: {
            value->as_int.type = va_arg(args, char *);
        } break;

        case JIPG_KIND_FLOAT: {
            value->as_float.type = va_arg(args, char *);
        } break;

        case JIPG_KIND_BOOL: {
        } break;
    }

    va_end(args);

    return value;
}

#define JIPG_OBJECT_IMPL(...) \
    new_jipg_value(JIPG_KIND_OBJECT, sizeof((Jipg_Value *[]){__VA_ARGS__}) / sizeof(Jipg_Value *), __VA_ARGS__)
#define JIPG_OBJECT(...) JIPG_OBJECT_IMPL(__VA_ARGS__)

#define JIPG_OBJECT_KV_IMPL(KEY, VALUE) \
    new_jipg_value(JIPG_KIND_OBJECT_KV, KEY, VALUE)
#define JIPG_OBJECT_KV(KEY, VALUE) JIPG_OBJECT_KV_IMPL(KEY, VALUE)

#define JIPG_ARRAY_IMPL(HAS_CAP, CAP, INTERNAL) \
    new_jipg_value(JIPG_KIND_ARRAY, HAS_CAP, CAP, INTERNAL)
#define JIPG_ARRAY(INTERNAL) JIPG_ARRAY_IMPL(false, 0, INTERNAL)
#define JIPG_ARRAY_CAP(INTERNAL, CAP) JIPG_ARRAY_IMPL(true, CAP, INTERNAL)

#define JIPG_STRING_IMPL(HAS_CAP, CAP) \
    new_jipg_value(JIPG_KIND_STRING, HAS_CAP, CAP)
#define JIPG_STRING() JIPG_STRING_IMPL(false, 0)
#define JIPG_STRING_CAP(CAP) JIPG_STRING_IMPL(true, CAP)

#define JIPG_INT_IMPL(INT_TYPE) \
    new_jipg_value(JIPG_KIND_INT, INT_TYPE)
#define JIPG_INT() JIPG_INT_IMPL(JIPG_DEFAULT_INT_TYPE)
#define JIPG_INT_T(INT_TYPE) JIPG_INT_IMPL(INT_TYPE)

#define JIPG_FLOAT_IMPL(FLOAT_TYPE) \
    new_jipg_value(JIPG_KIND_FLOAT, FLOAT_TYPE)
#define JIPG_FLOAT() JIPG_FLOAT_IMPL(JIPG_DEFAULT_FLOAT_TYPE)
#define JIPG_FLOAT_T(FLOAT_TYPE) JIPG_FLOAT_IMPL(FLOAT_TYPE)

#define JIPG_BOOL_IMPL() \
    new_jipg_value(JIPG_KIND_BOOL)
#define JIPG_BOOL() JIPG_BOOL_IMPL()

#define JIPG_PARSER(STRUCT_NAME, VALUE)                                         \
    static Jipg_Value *jipg_##STRUCT_NAME##_gen(void) {                         \
        return VALUE;                                                           \
    }                                                                           \
                                                                                \
    static void jipg_register_##STRUCT_NAME(void) __attribute__((constructor)); \
    static void jipg_register_##STRUCT_NAME(void) {                             \
        JIPG_ASSERT(jipg_global_context.value_count < JIPG_PARSER_CAP);         \
        size_t idx = jipg_global_context.value_count++;                         \
        jipg_global_context.values[idx] =                                       \
            (Jipg_Parser){.struct_name = #STRUCT_NAME,                          \
                          .value_gen = jipg_##STRUCT_NAME##_gen};               \
    }

static void jipg_emit_struct(FILE *out, Jipg_Value *value, const char *struct_name) {
    switch (value->kind) {
        default:
            UNREACHABLE();
        case JIPG_KIND_OBJECT: {
            fprintf(stderr, "typedef struct {\n");
            Jipg_Value *kv = value->as_object.kv_head;
            for (; kv; kv = kv->as_object_kv.next) {
                Jipg_Value *value = kv->as_object_kv.value;
                switch (value->kind) {
                    case JIPG_KIND_INT: {
                        fprintf(stderr, "    %s %s;\n", 
                                value->as_int.type, kv->as_object_kv.key);
                    } break;
                    case JIPG_KIND_FLOAT: {
                        fprintf(stderr, "    %s %s;\n",
                                value->as_float.type, kv->as_object_kv.key);
                    } break;
                }
            }
            fprintf(stderr, "}\n");
        } break;
    }
}

static int jipg_main(int argc, char *argv[]) {
    const char *header_name = "jsonparser.h";
    const char *src_name = "jsonparser.c";

    FILE *header = fopen(header_name, "w");
    if (header == NULL) {
        fprintf(stderr, "Unable to open %s\n", header_name);
        return 1;
    }

    fprintf(header, "// This file is auto-generated by %s\n", __FILE__);
    fprintf(header,
            "\n/*\n"
            " This file contains generated type definition\n"
            " and function declarations\n"
            "*/\n");

    fclose(header);
    return 0;
}

#define JIPG_MAIN()                    \
    int main(int argc, char *argv[]) { \
        return jipg_main(argc, argv);  \
    }

#endif  // JIPG_H
