#ifndef JIPG_H
#define JIPG_H

#include <ctype.h>
#include <libgen.h>
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

#ifndef JIPG_INIT_LIST_CAP
#define JIPG_INIT_LIST_CAP 8
#endif

static_assert((JIPG_INIT_LIST_CAP & (JIPG_INIT_LIST_CAP - 1)) == 0, "JIPG_INIT_LIST_CAP must be power of two");

#define UNREACHABLE()                                                                               \
    do {                                                                                            \
        fprintf(stderr, "UNREACHABLE CODE REACHED: %s:%d in %s()\n", __FILE__, __LINE__, __func__); \
        abort();                                                                                    \
    } while (0)

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(*arr))

#define STR2(x) #x
#define STR(x) STR2(x)

typedef enum {
    JIPG_KIND_OBJECT,
    JIPG_KIND_OBJECT_KV,
    JIPG_KIND_ARRAY,
    JIPG_KIND_STRING,
    JIPG_KIND_INT,
    JIPG_KIND_FLOAT,
    JIPG_KIND_BOOL,

    JIPG_KIND_PARSER_REF,

    JIPG_KIND_VALUE_COUNT,
} Jipg_Value_Kind;

typedef struct Jipg_Value Jipg_Value;

struct Jipg_Value {
    Jipg_Value_Kind kind;
    const char *head;

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
            size_t cap;
            Jipg_Value *internal;
        } as_array;

        struct {
            const char *struct_name;
        } as_parser_ref;
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

    Jipg_Value *values[JIPG_PARSER_CAP];

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

        case JIPG_KIND_PARSER_REF: {
            value->as_parser_ref.struct_name = va_arg(args, char *);
        } break;

        case JIPG_KIND_STRING:
        case JIPG_KIND_INT:
        case JIPG_KIND_FLOAT:
        case JIPG_KIND_BOOL:
            break;
    }

    va_end(args);

    return value;
}

#define JIPG_OBJECT_IMPL(...) \
    new_jipg_value(JIPG_KIND_OBJECT, sizeof((Jipg_Value *[]){__VA_ARGS__}) / sizeof(Jipg_Value *), __VA_ARGS__)

#define JIPG_KV_IMPL(KEY, VALUE) \
    new_jipg_value(JIPG_KIND_OBJECT_KV, KEY, VALUE)

#define JIPG_ARRAY_IMPL(CAP, INTERNAL) \
    new_jipg_value(JIPG_KIND_ARRAY, CAP, INTERNAL)

#define JIPG_STRING_IMPL() \
    new_jipg_value(JIPG_KIND_STRING)

#define JIPG_INT_IMPL() \
    new_jipg_value(JIPG_KIND_INT)

#define JIPG_FLOAT_IMPL() \
    new_jipg_value(JIPG_KIND_FLOAT)

#define JIPG_BOOL_IMPL() \
    new_jipg_value(JIPG_KIND_BOOL)

#define JIPG_USE_IMPL(NAME) \
    new_jipg_value(JIPG_KIND_PARSER_REF, STR(NAME))

#define JIPG_BOOL() JIPG_BOOL_IMPL()
#define JIPG_OBJECT(...) JIPG_OBJECT_IMPL(__VA_ARGS__)
#define JIPG_KV(KEY, VALUE) JIPG_KV_IMPL(KEY, VALUE)
#define JIPG_ARRAY(INTERNAL) JIPG_ARRAY_IMPL(0, INTERNAL)
#define JIPG_ARRAY_CAP(INTERNAL, CAP) JIPG_ARRAY_IMPL(CAP, INTERNAL)
#define JIPG_STRING() JIPG_STRING_IMPL()
#define JIPG_INT() JIPG_INT_IMPL()
#define JIPG_FLOAT() JIPG_FLOAT_IMPL()
#define JIPG_USE(NAME) JIPG_USE_IMPL(NAME)

#define JIPG_PARSER(STRUCT_NAME, VALUE)                                                             \
    static Jipg_Value *jipg_##STRUCT_NAME##_gen(void) {                                             \
        return VALUE;                                                                               \
    }                                                                                               \
                                                                                                    \
    static void jipg_register_##STRUCT_NAME(void) __attribute__((constructor));                     \
    static void jipg_register_##STRUCT_NAME(void) {                                                 \
        static_assert(__COUNTER__ < JIPG_PARSER_CAP, "Too many parsers, increase JIPG_PARSER_CAP"); \
        size_t idx = jipg_global_context.parser_count++;                                            \
        jipg_global_context.parsers[idx] =                                                          \
            (Jipg_Parser){.head_struct_name = #STRUCT_NAME,                                         \
                          .value_gen = jipg_##STRUCT_NAME##_gen};                                   \
    }

#ifdef JIPG_STRIP_PREFIX
#define OBJECT JIPG_OBJECT
#define KV JIPG_KV
#define ARRAY JIPG_ARRAY
#define ARRAY_CAP JIPG_ARRAY_CAP
#define STRING JIPG_STRING
#define INT JIPG_INT
#define FLOAT JIPG_FLOAT
#define BOOL JIPG_BOOL
#define PARSER JIPG_PARSER
#define USE JIPG_USE
#endif

static inline uint64_t fnv1a_64(const char *s) {
    uint64_t hash = 14695981039346656037ULL;
    while (*s) {
        hash ^= (uint8_t)(*s++);
        hash *= 1099511628211ULL;
    }
    return hash;
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

    if (name) {
        JIPG_ASSERT(fmt);
        int n = snprintf(NULL, 0, fmt, head_struct_name, struct_num);
        *name = JIPG_REALLOC(NULL, n + 1);
        JIPG_ASSERT(*name);
        snprintf(*name, n + 1, fmt, head_struct_name, struct_num);
    }
}

static const char *jipg_value_struct_name(const Jipg_Value *value) {
    switch (value->kind) {
        case JIPG_KIND_OBJECT:
            return value->as_object.struct_name;
        case JIPG_KIND_ARRAY:
            return value->as_array.struct_name;
        case JIPG_KIND_PARSER_REF: {
            const char *head_name = value->as_parser_ref.struct_name;
            for (size_t i = 0; i < jipg_global_context.parser_count; ++i) {
                Jipg_Value *value = jipg_global_context.values[i];
                if (!value->head)
                    continue;
                if (strcmp(head_name, value->head) == 0)
                    return jipg_value_struct_name(value);
            }
            fprintf(stderr, "Use of undefined parser: %s\n", head_name);
            exit(1);
        } break;

        case JIPG_KIND_OBJECT_KV:
        case JIPG_KIND_STRING:
        case JIPG_KIND_INT:
        case JIPG_KIND_FLOAT:
        case JIPG_KIND_BOOL:
        case JIPG_KIND_VALUE_COUNT:
            return NULL;
    }
}

static const char *jipg_value_name(const Jipg_Value *value) {
    const char *st = jipg_value_struct_name(value);
    if (st) return st;
    switch (value->kind) {
        case JIPG_KIND_STRING:
            return "str";
        case JIPG_KIND_INT:
            return "int";
        case JIPG_KIND_FLOAT:
            return "float";
        case JIPG_KIND_BOOL:
            return "bool";

        case JIPG_KIND_OBJECT:
        case JIPG_KIND_OBJECT_KV:
        case JIPG_KIND_ARRAY:
        case JIPG_KIND_VALUE_COUNT:
        case JIPG_KIND_PARSER_REF:
            UNREACHABLE();
    };
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
            fprintf(header, "char *");
        } break;
        case JIPG_KIND_INT: {
            fprintf(header, "%s ", JIPG_DEFAULT_INT_TYPE);
        } break;
        case JIPG_KIND_FLOAT: {
            fprintf(header, "%s ", JIPG_DEFAULT_FLOAT_TYPE);
        } break;
        case JIPG_KIND_BOOL: {
            fprintf(header, "bool ");
        } break;
        case JIPG_KIND_PARSER_REF: {
            fprintf(header, "%s ", value->as_parser_ref.struct_name);
        } break;
    }
}

static void jipg_emit_value_types(FILE *header, Jipg_Value *value) {
    const char *name;
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
            name = struct_name;

            if (!value->head) fprintf(header, "\n");
        } break;

        case JIPG_KIND_ARRAY: {
            Jipg_Value *internal = value->as_array.internal;
            jipg_emit_value_types(header, internal);
            const char *struct_name = value->as_array.struct_name;

            fprintf(header,
                    "typedef struct {\n"
                    "    size_t len;\n"
                    "    ");
            jipg_emit_field_type(header, internal);
            fprintf(header,
                    "*items;\n"
                    "} %s;\n",
                    struct_name);
            name = struct_name;

            if (!value->head) fprintf(header, "\n");
        } break;

        // Does not generate types for primitives for now
        // This means primitives may not be the only value
        default: {
        }
    }

    if (value->head) {
        fprintf(header, "\ntypedef %s %s;\n\n", name, value->head);
        name = value->head;

        fprintf(header, "bool parse_%s(const char *json, size_t json_length, %s *res);\n\n", name, name);
        fprintf(header,
                "static inline bool parse_%s_cstr(const char *json, %s *res) {\n"
                "    return parse_%s(json, strlen(json), res);\n"
                "}\n\n",
                name, name, name);
    }
}

static void jipg_emit_file_name_all_caps(FILE *header, char *file_name) {
    char *base = basename(file_name);
    for (char c = *base; c && c != '.'; c = *(++base)) {
        if (isalpha(c)) {
            fputc(toupper(c), header);
        } else {
            fputc('_', header);
        }
    }
}

static void jipg_emit_header_macro(FILE *header, char *header_name) {
    jipg_emit_file_name_all_caps(header, header_name);
    fprintf(header, "_H");
}
static void jipg_emit_header_impl_macro(FILE *header, char *header_name) {
    jipg_emit_file_name_all_caps(header, header_name);
    fprintf(header, "_IMPLEMENTATION");
}

static void jipg_emit_header(FILE *header, Jipg_Value **values, size_t value_count, char *header_name) {
    static const char *header_includes[] = {
        "<stdbool.h>",
        "<stddef.h>",
        "<stdint.h>",
        "<string.h>",
    };

    fprintf(header, "// NOTE: This file has been auto-generated by %s\n\n", __FILE__);
    fprintf(header, "#ifndef ");
    jipg_emit_header_macro(header, header_name);
    fprintf(header, "\n#define ");
    jipg_emit_header_macro(header, header_name);
    fprintf(header, "\n\n");

    for (size_t i = 0; i < ARRAY_SIZE(header_includes); ++i)
        fprintf(header, "#include %s\n", header_includes[i]);
    fprintf(header, "\n");

    for (size_t i = 0; i < value_count; ++i) {
        Jipg_Value *value = values[i];
        jipg_emit_value_types(header, value);
    }

    fprintf(header, "#endif  // ");
    jipg_emit_header_macro(header, header_name);
}

static void jipg_emit_lexer_impl(FILE *source) {
    fprintf(source,
            "typedef enum {\n"
            "   TOKEN_TYPE_NONE,\n"
            "   TOKEN_TYPE_ILLEGAL,\n"
            "   TOKEN_TYPE_EOF,\n"
            "\n"
            "   TOKEN_TYPE_LBRACE,\n"
            "   TOKEN_TYPE_RBRACE,\n"
            "   TOKEN_TYPE_LBRACKET,\n"
            "   TOKEN_TYPE_RBRACKET,\n"
            "   TOKEN_TYPE_COLON,\n"
            "   TOKEN_TYPE_COMMA,\n"
            "\n"
            "   TOKEN_TYPE_STRING,\n"
            "   TOKEN_TYPE_NUMBER,\n"
            "   TOKEN_TYPE_TRUE,\n"
            "   TOKEN_TYPE_FALSE,\n"
            "   TOKEN_TYPE_NULL,\n"
            "} Token_Type;\n");

    fprintf(source,
            "typedef struct {\n"
            "   const char *lit;\n"
            "   uint32_t len;\n"
            "   Token_Type type;\n"
            "} Token;\n");

    fprintf(source,
            "typedef struct {\n"
            "   const char *input;\n"
            "   size_t len;\n"
            "   size_t pos;\n"
            "   size_t read_pos;\n"
            "   char ch;\n"
            "} Lexer;\n");

    fprintf(source,
            "static inline void read_char(Lexer *l) {\n"
            "   if (l->read_pos >= l->len) {\n"
            "      l->ch = 0;\n"
            "   } else {\n"
            "      l->ch = l->input[l->read_pos];\n"
            "   }\n"
            "   l->pos = l->read_pos;\n"
            "   ++l->read_pos;\n"
            "}\n");

    fprintf(source,
            "static inline void read_chars(Lexer *l, size_t n) {\n"
            "    for (size_t i = 0; i < n; ++i)\n"
            "        read_char(l);\n"
            "}\n");

    fprintf(source,
            "static inline size_t read_digits(Lexer *l) {\n"
            "    size_t size = 0;"
            "    while (isdigit(l->ch)) {\n"
            "        ++size;\n"
            "        read_char(l);\n"
            "    }\n"
            "    return size;\n"
            "}\n");

    fprintf(source,
            "static inline size_t read_integer(Lexer *l) {\n"
            "    size_t size = 0;\n"
            "    if (l->ch == '-') {\n"
            "        ++size;\n"
            "        read_char(l);\n"
            "    }\n"
            "    size += read_digits(l);\n"
            "    return size;\n"
            "}\n");

    fprintf(source,
            "static inline size_t read_fraction(Lexer *l) {\n"
            "    size_t size = 0;\n"
            "    if (l->ch == '.') {\n"
            "        ++size;\n"
            "        read_char(l);\n"
            "        size += read_digits(l);\n"
            "    }\n"
            "    return size;\n"
            "}\n");

    fprintf(source,
            "static inline size_t read_exponent(Lexer *l) {\n"
            "    size_t size = 0;\n"
            "    if (l->ch == 'e') {\n"
            "        ++size;\n"
            "        read_char(l);\n"
            "        size += read_digits(l);\n"
            "    }\n"
            "    return size;\n"
            "}\n");

    fprintf(source,
            "static inline size_t read_number(Lexer *l) {\n"
            "    size_t size = 0;\n"
            "    size += read_integer(l);\n"
            "    size += read_fraction(l);\n"
            "    size += read_exponent(l);\n"
            "    return size;\n"
            "}\n");

    fprintf(source,
            "static inline bool is_whitespace(char ch) {\n"
            "    return ch == ' ' || ch == '\\t' || ch == '\\n' || ch == '\\r';\n"
            "}\n"
            "static inline void skip_whitespace(Lexer *l) {\n"
            "    while(is_whitespace(l->ch)) {\n"
            "        read_char(l);\n"
            "    }\n"
            "}\n");

    fprintf(source,
            "static inline Token next_token(Lexer *l) {\n"
            "    skip_whitespace(l);\n"
            "    Token tok = {.lit = l->input + l->pos, .len = 1};\n"
            "    switch (l->ch) {\n"
            "        case '{': {\n"
            "            tok.type = TOKEN_TYPE_LBRACE;\n"
            "        } break;\n"
            "        case '}': {\n"
            "            tok.type = TOKEN_TYPE_RBRACE;\n"
            "        } break;\n"
            "        case '[': {\n"
            "            tok.type = TOKEN_TYPE_LBRACKET;\n"
            "        } break;\n"
            "        case ']': {\n"
            "            tok.type = TOKEN_TYPE_RBRACKET;\n"
            "        } break;\n"
            "        case ':': {\n"
            "            tok.type = TOKEN_TYPE_COLON;\n"
            "        } break;\n"
            "        case ',': {\n"
            "            tok.type = TOKEN_TYPE_COMMA;\n"
            "        } break;\n"
            "        case '\"': {\n"
            "            tok.type = TOKEN_TYPE_STRING;\n"
            "            read_char(l);\n"
            "            tok.len = 0;\n"
            "            tok.lit = l->input + l->pos;\n"
            "            for (; l->ch != '\"'; read_char(l), ++tok.len) {\n"
            "                if (l->ch == '\\\\') {\n"
            "                    read_char(l);\n"
            "                    ++tok.len;\n"
            "                }\n"
            "            }\n"
            "            // read_char(l);\n"
            "        } break;\n"
            "        default: {\n"
            "            if (isdigit(l->ch) || l->ch == '.' || l->ch == '-') {\n"
            "                tok.type = TOKEN_TYPE_NUMBER;"
            "                tok.len = read_number(l);\n"
            "                return tok;"
            "            } else {\n"
            "                if (strncmp(l->input, \"true\", 4) == 0) {\n"
            "                    tok.type = TOKEN_TYPE_TRUE;\n"
            "                    tok.len = 4;\n"
            "                    read_chars(l, 4);\n"
            "                } else if (strncmp(l->input, \"false\", 5) == 0) {\n"
            "                    tok.type = TOKEN_TYPE_FALSE;\n"
            "                    tok.len = 5;\n"
            "                    read_chars(l, 5);\n"
            "                } else if (strncmp(l->input, \"null\", 4) == 0) {\n"
            "                    tok.type = TOKEN_TYPE_NULL;\n"
            "                    tok.len = 4;\n"
            "                    read_chars(l, 4);\n"
            "                } else {\n"
            "                    tok.type = TOKEN_TYPE_ILLEGAL;\n"
            "                }\n"
            "                return tok;\n"
            "            }\n"
            "        }\n"
            "    }\n"
            "    read_char(l);\n"
            "    return tok;\n"
            "}\n");

    fprintf(source,
            "static inline uint64_t hash(Token *tok) {\n"
            "    uint64_t hash = 14695981039346656037ULL;\n"
            "    for (size_t i = 0; i < tok->len; ++i) {\n"
            "        hash ^= (uint8_t)tok->lit[i];\n"
            "        hash *= 1099511628211ULL;\n"
            "    }\n"
            "    return hash;\n"
            "}\n");
}

static void jipg_emit_helpers(FILE *source) {
    fprintf(source,
            "static inline bool parse_bool(Lexer *l, bool *res) {\n"
            "    Token tok = next_token(l);\n"
            "    switch (tok.type) {\n"
            "        case TOKEN_TYPE_TRUE: {\n"
            "            *res = true;\n"
            "            return true;\n"
            "        } break;\n"
            "        case TOKEN_TYPE_FALSE: {\n"
            "            *res = false;\n"
            "            return true;\n"
            "        } break;\n"
            "        default:\n"
            "            return false;\n"
            "    }\n"
            "}\n");

    fprintf(source,
            "static inline bool parse_int(Lexer *l, int64_t *res) {\n"
            "    Token tok = next_token(l);\n"
            "    *res = (int64_t)atof(tok.lit);\n"
            "    return true;\n"
            "}\n");

    fprintf(source,
            "static inline bool parse_float(Lexer *l, double *res) {\n"
            "    Token tok = next_token(l);\n"
            "    *res = atof(tok.lit);\n"
            "    return true;\n"
            "}\n");

    fprintf(source,
            "static inline bool parse_str(Lexer *l, char **res) {\n"
            "    Token tok = next_token(l);\n"
            "    if (tok.type != TOKEN_TYPE_STRING)\n"
            "        return false;\n"
            "    *res = (char *)%s(NULL, tok.len + 1);\n"
            "    if (!*res) return false;\n"
            "    memcpy(*res, tok.lit, tok.len);\n"
            "    (*res)[tok.len] = 0;\n"
            "    return true;\n"
            "}\n",
            STR(JIPG_REALLOC));
}

static void jipg_emit_value_parser(FILE *source, Jipg_Value *value);

static void jipg_emit_object_parser(FILE *source, Jipg_Value *object) {
    Jipg_Value *kv = object->as_object.kv_head;

    for (; kv; kv = kv->as_object_kv.next) {
        Jipg_Value *value = kv->as_object_kv.value;
        jipg_emit_value_parser(source, value);
    }

    const char *struct_name = object->as_object.struct_name;

    fprintf(source,
            "static inline bool parse_%s(Lexer *l, %s *res) {\n"
            "    Token lbrace = next_token(l);\n"
            "    if (lbrace.type != TOKEN_TYPE_LBRACE) return false;\n"
            "    Token tok = next_token(l);\n"
            "    while (tok.type != TOKEN_TYPE_RBRACE) {\n"
            "        if (tok.type != TOKEN_TYPE_STRING) return false;\n"
            "        uint64_t key_hash = hash(&tok);\n"
            "        tok = next_token(l);\n"
            "        if (tok.type != TOKEN_TYPE_COLON) return false;\n"
            "        switch (key_hash) {\n",
            struct_name, struct_name);

    kv = object->as_object.kv_head;
    for (; kv; kv = kv->as_object_kv.next) {
        const char *key = kv->as_object_kv.key;
        uint64_t key_hash = fnv1a_64(key);
        fprintf(source,
                "            case %lullu: {  // %s\n",
                key_hash, key);

        const Jipg_Value *value = kv->as_object_kv.value;

        fprintf(source,
                "                if (!parse_%s(l, &res->%s))\n"
                "                    return false;\n",
                jipg_value_name(value), key);

        fprintf(source,
                "            } break;\n");
    }

    fprintf(source,
            "        }\n"
            "        tok = next_token(l);\n"
            "        if (tok.type == TOKEN_TYPE_COMMA)\n"
            "            tok = next_token(l);\n"
            "    }\n"
            "    return true;\n"
            "}\n");
}

static void jipg_emit_array_parser(FILE *source, Jipg_Value *array) {
    const char *struct_name = array->as_array.struct_name;
    Jipg_Value *internal = array->as_array.internal;
    jipg_emit_value_parser(source, internal);
    fprintf(source,
            "static inline bool parse_%s(Lexer *l, %s *res) {\n"
            "    Token lbracket = next_token(l);\n"
            "    if (lbracket.type != TOKEN_TYPE_LBRACKET) return false;\n"
            "    for (;;) {\n"
            "        Lexer save = *l;\n"
            "        Token tok = next_token(l);\n"
            "        if (tok.type == TOKEN_TYPE_RBRACKET) break;\n"
            "        if (tok.type != TOKEN_TYPE_COMMA) *l = save;\n",
            struct_name, struct_name);

    if (array->as_array.cap) {
        size_t cap = array->as_array.cap;
        fprintf(source,
                "        if (res->len == 0) {\n"
                "            res->items = %s(NULL, %zu * sizeof(*res->items));\n"
                "            if (res->items == NULL) return false;\n"
                "        }\n"
                "        if (res->len == %zu) return false;\n",
                STR(JIPG_REALLOC), cap, cap);
    } else {
        fprintf(source,
                "        if (res->len == 0 || (res->len > %d && (res->len & (res->len - 1)) == 0)) {\n"
                "            size_t new_cap = res->len ? res->len * 2 : %d;\n"
                "            res->items = %s(res->items, new_cap * sizeof(*res->items));\n"
                "            if (res->items == NULL) return false;\n"
                "        }\n",
                JIPG_INIT_LIST_CAP, JIPG_INIT_LIST_CAP, STR(JIPG_REALLOC));
    }

    fprintf(source,
            "        if (!parse_%s(l, res->items + res->len++))\n"
            "            return false;\n"
            "    }\n"
            "    return true;\n"
            "}\n",
            jipg_value_name(internal));
}

static void jipg_emit_value_parser(FILE *source, Jipg_Value *value) {
    switch (value->kind) {
        case JIPG_KIND_OBJECT: {
            jipg_emit_object_parser(source, value);
        } break;
        case JIPG_KIND_ARRAY: {
            jipg_emit_array_parser(source, value);
        } break;

        case JIPG_KIND_OBJECT_KV:
        case JIPG_KIND_STRING:
        case JIPG_KIND_INT:
        case JIPG_KIND_FLOAT:
        case JIPG_KIND_BOOL:
        case JIPG_KIND_PARSER_REF:
        case JIPG_KIND_VALUE_COUNT:
            break;
    }
}

static void jipg_emit_head_value_parser(FILE *source, Jipg_Value *value) {
    jipg_emit_value_parser(source, value);

    const char *struct_name = jipg_value_struct_name(value);
    JIPG_ASSERT(struct_name);

    int n = strlen(struct_name) - 1;

    fprintf(source,
            "bool parse_%s(const char *json, size_t json_length, %s *res) {\n"
            "    Lexer l = {\n"
            "        .input = json,\n"
            "        .len = json_length,\n"
            "    };\n"
            "    read_char(&l);\n"
            "    return parse_%s(&l, res);\n"
            "}\n",
            value->head, value->head, struct_name);
}

static void jipg_emit_source(FILE *source, Jipg_Value **values, size_t value_count, const char *header_name) {
    static const char *source_includes[] = {
        "<stdbool.h>",
        "<stdint.h>",
        "<stdlib.h>",
        "<string.h>",
        "<ctype.h>",
    };

    if (header_name)
        fprintf(source, "#include \"%s\"\n", header_name);

    for (size_t i = 0; i < ARRAY_SIZE(source_includes); ++i)
        fprintf(source, "#include %s\n", source_includes[i]);
    fprintf(source, "\n");

    jipg_emit_lexer_impl(source);
    jipg_emit_helpers(source);

    for (size_t i = 0; i < value_count; ++i) {
        Jipg_Value *value = values[i];
        jipg_emit_head_value_parser(source, value);
    }
}

static void jipg_parse_arg(char *arg, char **header_name, char **source_name, bool *single_file) {
    static const char *help_str = "--help";
    static const char *header_str = "--header=";
    static const char *source_str = "--source=";
    static const char *single_file_str = "--single-file";

    if (strncmp(arg, help_str, strlen(help_str)) == 0) {
        printf(
            "jipg.h - the single file JSON Parser Generator for C!\n"
            "Command line options:\n"
            "  --help                  Show this message and exit.\n"
            "  --header=<header-file>  Path of generated header file.\n"
            "  --source=<source-file>  Path of generated source file.\n"
            "  --single-file           Generates single STB style header file.\n");
        exit(0);
    } else if (strncmp(arg, header_str, strlen(header_str)) == 0) {
        *header_name = arg + strlen(header_str);
    } else if (strncmp(arg, source_str, strlen(source_str)) == 0) {
        *source_name = arg + strlen(source_str);
    } else if (strncmp(arg, single_file_str, strlen(single_file_str)) == 0) {
        *single_file = true;
    }
}

static int jipg_main(/* cli args */ int argc, char *argv[],
                     /* config args */ int cfg_argc, char *cfg_argv[]) {
    size_t value_count = jipg_global_context.parser_count;
    for (size_t i = 0; i < value_count; ++i) {
        Jipg_Parser *parser = jipg_global_context.parsers + i;
        Jipg_Value *value = parser->value_gen();
        value->head = parser->head_struct_name;
        jipg_generate_struct_names(value, parser->head_struct_name);
        jipg_global_context.values[i] = value;
    }

    char *header_name = "jsonparser.h";
    char *source_name = "jsonparser.c";
    bool single_file = false;

    for (int i = 0; i < cfg_argc; ++i) {
        if (!cfg_argv[i]) continue;
        jipg_parse_arg(cfg_argv[i], &header_name, &source_name, &single_file);
    }

    for (int i = 1; i < argc; ++i) {
        jipg_parse_arg(argv[i], &header_name, &source_name, &single_file);
    }

    FILE *header = fopen(header_name, "w");
    if (header == NULL) {
        fprintf(stderr, "Unable to open %s\n", header_name);
        return 1;
    }

    jipg_emit_header(header, jipg_global_context.values, value_count, header_name);

    FILE *source;
    if (single_file) {
        source = header;

        fprintf(source, "\n\n#ifdef ");
        jipg_emit_header_impl_macro(header, header_name);
        fprintf(source, "\n\n");
    } else {
        source = fopen(source_name, "w");
        if (source == NULL) {
            fprintf(stderr, "Unable to open %s\n", header_name);
            return 1;
        }
    }

    jipg_emit_source(source, jipg_global_context.values, value_count,
                     single_file ? NULL : header_name);

    if (single_file) {
        fprintf(source, "\n#endif  // ");
        jipg_emit_header_impl_macro(header, header_name);
    }

    fclose(header);
    return 0;
}

#define JIPG_MAIN(...)                                                      \
    int main(int argc, char *argv[]) {                                      \
        char *args[] = {/* Stop GCC from complaining */ NULL, __VA_ARGS__}; \
        return jipg_main(argc, argv, ARRAY_SIZE(args), args);               \
    }

#endif  // JIPG_H
