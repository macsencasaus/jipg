#ifndef JIPG_H
#define JIPG_H

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

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
    bool head;

    union {
        struct {
            char *struct_name;
            Jipg_Value *kv_head;
        } as_object;

        struct {
            const char *key;
            Jipg_Value *value;
            Jipg_Value *next;
        } as_object_kv;

        struct {
            char *struct_name;
            int64_t cap;
            Jipg_Value *internal;
        } as_array;

        struct {
            int64_t cap;
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
    const char *head_struct_name;
    Jipg_Value *(*value_gen)(void);
} Jipg_Parser;

typedef struct {
    size_t arena_size;
    Jipg_Value arena[JIPG_VALUE_ARENA_CAP];

    size_t parser_count;
    Jipg_Parser parsers[JIPG_PARSER_CAP];

    size_t name_alloc;
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

            Jipg_Value **last_next = &value->as_object.kv_head;
            for (size_t i = 0; i < count; ++i) {
                Jipg_Value *kv = va_arg(args, Jipg_Value *);
                *last_next = kv;
                last_next = &kv->as_object_kv.next;
            }
        } break;

        case JIPG_KIND_OBJECT_KV: {
            value->as_object_kv.key = va_arg(args, char *);
            value->as_object_kv.value = va_arg(args, Jipg_Value *);
        } break;

        case JIPG_KIND_ARRAY: {
            value->as_array.cap = va_arg(args, int64_t);
            value->as_array.internal = va_arg(args, Jipg_Value *);
        } break;

        case JIPG_KIND_STRING: {
            value->as_string.cap = va_arg(args, int64_t);
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

#define JIPG_ARRAY_IMPL(CAP, INTERNAL) \
    new_jipg_value(JIPG_KIND_ARRAY, CAP, INTERNAL)
#define JIPG_ARRAY(INTERNAL) JIPG_ARRAY_IMPL(-1, INTERNAL)
#define JIPG_ARRAY_CAP(INTERNAL, CAP) JIPG_ARRAY_IMPL(CAP, INTERNAL)

#define JIPG_STRING_IMPL(CAP) \
    new_jipg_value(JIPG_KIND_STRING, CAP)
#define JIPG_STRING() JIPG_STRING_IMPL(-1)
#define JIPG_STRING_CAP(CAP) JIPG_STRING_IMPL(CAP)

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
        JIPG_ASSERT(jipg_global_context.parser_count < JIPG_PARSER_CAP);        \
        size_t idx = jipg_global_context.parser_count++;                        \
        jipg_global_context.parsers[idx] =                                      \
            (Jipg_Parser){.head_struct_name = #STRUCT_NAME,                     \
                          .value_gen = jipg_##STRUCT_NAME##_gen};               \
    }

static void jipg_generate_struct_names(Jipg_Value *value, const char *head_struct_name) {
    const char *fmt = NULL;
    char **name = NULL;
    size_t struct_num;

    switch (value->kind) {
        case JIPG_KIND_OBJECT: {
            struct_num = jipg_global_context.name_alloc++;
            fmt = "%s_object%zu";
            name = &value->as_object.struct_name;

            Jipg_Value *kv = value->as_object.kv_head;
            for (; kv; kv = kv->as_object_kv.next)
                jipg_generate_struct_names(kv->as_object_kv.value, head_struct_name);
        } break;
        case JIPG_KIND_ARRAY: {
            struct_num = jipg_global_context.name_alloc++;
            fmt = "%s_array%zu";
            name = &value->as_array.struct_name;

            jipg_generate_struct_names(value->as_array.internal, head_struct_name);
        } break;
        default: {
        }
    }

    if (value->head && name) {
        size_t n = strlen(head_struct_name);
        *name = JIPG_REALLOC(NULL, n + 1);
        JIPG_ASSERT(*name);
        strcpy(*name, head_struct_name);
        name[n] = 0;
        return;
    }

    if (name) {
        JIPG_ASSERT(fmt);
        int n = snprintf(NULL, 0, fmt, head_struct_name, struct_num);
        *name = JIPG_REALLOC(NULL, n + 1);
        JIPG_ASSERT(*name);
        snprintf(*name, n + 1, fmt, head_struct_name, struct_num);
    }
}

static void jipg_emit_field_type(FILE *header, Jipg_Value *value) {
    switch (value->kind) {
        case JIPG_KIND_OBJECT_KV:
        case JIPG_KIND_VALUE_COUNT:
            UNREACHABLE();

        case JIPG_KIND_OBJECT: {
            JIPG_ASSERT(value->as_object.struct_name);
            fprintf(header, "%s ", value->as_object.struct_name);
        } break;
        case JIPG_KIND_ARRAY: {
            JIPG_ASSERT(value->as_array.struct_name);
            fprintf(header, "%s ", value->as_array.struct_name);
        } break;
        case JIPG_KIND_STRING: {
            fprintf(header, "const char *");
        } break;
        case JIPG_KIND_INT: {
            fprintf(header, "%s ", value->as_int.type);
        } break;
        case JIPG_KIND_FLOAT: {
            fprintf(header, "%s ", value->as_float.type);
        } break;
        case JIPG_KIND_BOOL: {
            fprintf(header, "bool ");
        } break;
    }
}

static void jipg_emit_value_types(FILE *header, Jipg_Value *value) {
    switch (value->kind) {
        case JIPG_KIND_OBJECT: {
            Jipg_Value *kv = value->as_object.kv_head;
            for (; kv; kv = kv->as_object_kv.next)
                jipg_emit_value_types(header, kv->as_object_kv.value);

            const char *struct_name = value->as_object.struct_name;

            fprintf(header, "typedef struct {\n");
            kv = value->as_object.kv_head;
            for (; kv; kv = kv->as_object_kv.next) {
                fprintf(header, "    ");
                jipg_emit_field_type(header, kv->as_object_kv.value);
                fprintf(header, "%s;\n", kv->as_object_kv.key);
            }
            fprintf(header, "} %s;\n", struct_name);

            if (value->head)
                fprintf(header, "\nbool parse_%s(const char *json, %s *res);\n\n", struct_name, struct_name);
            else
                fprintf(header, "\n");
        } break;

        case JIPG_KIND_ARRAY: {
            const char *struct_name = value->as_array.struct_name;

            fprintf(header, "typedef struct {\n");
            fprintf(header, "    size_t len;\n");
            fprintf(header, "    ");
            jipg_emit_field_type(header, value->as_array.internal);
            fprintf(header, "*items;\n");
            fprintf(header, "} %s;\n", struct_name);

            if (value->head)
                fprintf(header, "\nbool parse_%s(const char *json, %s *res);\n\n", struct_name, struct_name);
            else
                fprintf(header, "\n");
        } break;

        // Does not generate types for primitives for now
        // This means primitives may not be only value
        default: {
        }
    }
}

static void jipg_emit_header(FILE *header, Jipg_Value **values, size_t value_count) {
    fprintf(header, "// NOTE: This file has been auto-generated by %s\n\n", __FILE__);
    fprintf(header, "#ifndef JSONPARSER_H\n#define JSONPARSER_H\n\n");

    fprintf(header, "#include <stdbool.h>\n#include <stddef.h>\n#include <stdint.h>\n\n");

    for (size_t i = 0; i < value_count; ++i) {
        Jipg_Value *value = values[i];
        jipg_emit_value_types(header, value);
    }

    fprintf(header, "#endif  // JSONPARSER_H");
}

static int jipg_main(int argc, char *argv[]) {
    size_t value_count = jipg_global_context.parser_count;
    static Jipg_Value *values[JIPG_PARSER_CAP];
    for (size_t i = 0; i < value_count; ++i) {
        values[i] = jipg_global_context.parsers[i].value_gen();
        values[i]->head = true;
        jipg_generate_struct_names(values[i], jipg_global_context.parsers[i].head_struct_name);
    }

    const char *header_name = "jsonparser.h";
    const char *src_name = "jsonparser.c";

    FILE *header = fopen(header_name, "w");
    if (header == NULL) {
        fprintf(stderr, "Unable to open %s\n", header_name);
        return 1;
    }

    jipg_emit_header(header, values, value_count);

    fclose(header);
    return 0;
}

#define JIPG_MAIN()                    \
    int main(int argc, char *argv[]) { \
        return jipg_main(argc, argv);  \
    }

#endif  // JIPG_H
