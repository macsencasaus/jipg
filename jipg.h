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
#define JIPG_OBJECT(...) JIPG_OBJECT_IMPL(__VA_ARGS__)

#define JIPG_KV_IMPL(KEY, VALUE) \
    new_jipg_value(JIPG_KIND_OBJECT_KV, KEY, VALUE)
#define JIPG_KV(KEY, VALUE) JIPG_KV_IMPL(KEY, VALUE)

#define JIPG_ARRAY_IMPL(CAP, INTERNAL) \
    new_jipg_value(JIPG_KIND_ARRAY, CAP, INTERNAL)
#define JIPG_ARRAY(INTERNAL) JIPG_ARRAY_IMPL(0, INTERNAL)
#define JIPG_ARRAY_CAP(INTERNAL, CAP) JIPG_ARRAY_IMPL(CAP, INTERNAL)

#define JIPG_STRING_IMPL() \
    new_jipg_value(JIPG_KIND_STRING)
#define JIPG_STRING() JIPG_STRING_IMPL()

#define JIPG_INT_IMPL() \
    new_jipg_value(JIPG_KIND_INT)
#define JIPG_INT() JIPG_INT_IMPL()

#define JIPG_FLOAT_IMPL() \
    new_jipg_value(JIPG_KIND_FLOAT)
#define JIPG_FLOAT() JIPG_FLOAT_IMPL()

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
#endif

static uint64_t sbox_lut[256] = {
    4319792334712819669llu,
    11788048577503494824llu,
    4192769064029443593llu,
    2513787319205155662llu,
    2134567472807851094llu,
    1735254072534978428llu,
    10598951352238613536llu,
    6878563960102566144llu,
    13600800192354800053llu,
    11046179386901516386llu,
    17980805327672283720llu,
    8985440510998354245llu,
    6183111496651652315llu,
    6452449828796159277llu,
    15247151809474287309llu,
    628930027092821547llu,
    4211374611712381237llu,
    8281039959893252210llu,
    17390767703283089698llu,
    16873412698214365220llu,
    5026511325615183432llu,
    2754439531571637074llu,
    10844283540583983130llu,
    16374963750459844554llu,
    2654745661507182789llu,
    16336963449664068247llu,
    9014946178090130474llu,
    9843414387061402607llu,
    14259680413192756630llu,
    2260059874028813140llu,
    13745877420346827559llu,
    5972285565579831383llu,
    13852939945820309002llu,
    14837314206658177788llu,
    17402143655000940783llu,
    1708604567626761540llu,
    4971097370218678035llu,
    7674742225009642846llu,
    13155675349293154075llu,
    18326272682222133568llu,
    5298809203106154964llu,
    10321967684452856755llu,
    9904615985190967285llu,
    9017917336912873721llu,
    14740931612662927227llu,
    11200519612596959265llu,
    9554698661026540077llu,
    11018450097963108256llu,
    12305895460640548825llu,
    10438989659195208381llu,
    8015438448961043528llu,
    17136452634472530012llu,
    16835094323228825799llu,
    2931420695595796890llu,
    9267616329121027312llu,
    10026743574507445845llu,
    18362789087893070577llu,
    10974710480031330365llu,
    11419262694613242363llu,
    7568953912120267979llu,
    15887571027615820885llu,
    8775471937118169068llu,
    15195899463999599281llu,
    9747648019597706196llu,
    8201586468252878983llu,
    5719011471609942381llu,
    13161946082177172418llu,
    9643816490275617805llu,
    6730682062675188990llu,
    14089583241851970809llu,
    5635197312546352421llu,
    12655462067761187474llu,
    13449440608602046021llu,
    6644636282855459766llu,
    3050467116415598703llu,
    3684536428532129555llu,
    12028965273189028973llu,
    14643909474857509060llu,
    11361797772764692215llu,
    17277487187655181768llu,
    6415102370970183924llu,
    5524091569279524857llu,
    5068062803131918708llu,
    7109967286375589665llu,
    11593927233023144500llu,
    2558289301845890764llu,
    11523058457548473969llu,
    7863147243172846153llu,
    9163551553006898156llu,
    11306257618139311415llu,
    1091573458739121057llu,
    6769139033989907147llu,
    13564598501354656307llu,
    11284935533708579323llu,
    11521065115140151397llu,
    1444550222897899060llu,
    13662499872620805028llu,
    4245035599922404824llu,
    9067794782920689701llu,
    16507826654309833394llu,
    3691186535829678736llu,
    14320171767955027621llu,
    1705144956923856113llu,
    1833347130189032787llu,
    9400322860645968606llu,
    16598016380617033567llu,
    12773246362538168504llu,
    2346890632840894497llu,
    13284161663367897779llu,
    11156056256940801702llu,
    6566438270435079793llu,
    14184426567501892882llu,
    1693157918753510477llu,
    18378075785519122098llu,
    7204465313303453675llu,
    11941213939911013851llu,
    15352752417848660924llu,
    4139201040660667366llu,
    5909127669011412127llu,
    1766864564625658187llu,
    16892202770840929564llu,
    12228148462877517818llu,
    5407581766005195250llu,
    10393550889445925046llu,
    15256151015114260953llu,
    2353447696693151925llu,
    9710412925974416421llu,
    12743525087182496020llu,
    15105473404528537499llu,
    10889357871637817063llu,
    1474346148541527469llu,
    1917876497304447223llu,
    14609042902951154655llu,
    2134607030903574787llu,
    15770058668595058971llu,
    491218943873080365llu,
    4900270299576406967llu,
    1292099914962507676llu,
    14875850326639549802llu,
    11622712433991736902llu,
    8168170408415742480llu,
    12651352620691196346llu,
    11670412535230076013llu,
    5689074268401770601llu,
    12395942360559016504llu,
    13698981439924777122llu,
    3022290795689847035llu,
    6652749801995951047llu,
    17180084454509305085llu,
    7387193433063974682llu,
    11628907454178133268llu,
    9532358457880247002llu,
    12388837363011885658llu,
    13314773417382220648llu,
    9255963573622641440llu,
    9644072662424383990llu,
    7328950323976250927llu,
    14604503762832768879llu,
    3460848478746553427llu,
    11756520680566867444llu,
    10228646956711648526llu,
    1498319470492480755llu,
    13912900895468874853llu,
    7977243703868154757llu,
    8730092574978919811llu,
    4015964408900624350llu,
    7171236051813229128llu,
    18276241805241254269llu,
    2140743450950054296llu,
    11233125218774969325llu,
    8895381570282353483llu,
    12737923117724585870llu,
    8347198033509902173llu,
    13101468282174137783llu,
    5331670031567708380llu,
    9941446125369432390llu,
    6811306828184863563llu,
    16907006339194758401llu,
    8655596796607110507llu,
    6483948371237476292llu,
    4167424907297969719llu,
    7920519602557499627llu,
    14531642921202149660llu,
    13258987673360713llu,
    6467609713031142714llu,
    5010049225410484655llu,
    15514283798534139378llu,
    2059836957055789216llu,
    14708287984452760943llu,
    7535258690393535422llu,
    16388904321402024647llu,
    6203261491251237737llu,
    975838946655649106llu,
    4998134504284488305llu,
    16837050324560415195llu,
    11666423454290084314llu,
    7516705248621185019llu,
    15508954692014489101llu,
    1862610367258030004llu,
    9838818531799331815llu,
    16204719013375924404llu,
    2999112553610745537llu,
    16789972537408997378llu,
    8396049629559291046llu,
    3143880835529166843llu,
    10402797171161811909llu,
    7904118188430433842llu,
    5967276514064706584llu,
    1898989867942402000llu,
    3584701738153559995llu,
    14354476367859229218llu,
    16218552616112798093llu,
    8193391465707397554llu,
    14966801352791404391llu,
    15217520260583216837llu,
    3195377995422721604llu,
    2925353982122579135llu,
    16559652876595655381llu,
    3888455958671764989llu,
    17558526478372576919llu,
    7858177113633778887llu,
    11386798032920516188llu,
    15630723010864551520llu,
    2382795051889734418llu,
    11312546213314097891llu,
    14415980522082086403llu,
    2985758652673726372llu,
    12638527984265041327llu,
    15598397787008652528llu,
    6629363358464932203llu,
    6010360761383048377llu,
    17085558173212054345llu,
    11034086578976379926llu,
    7894618991144381370llu,
    8692562567562402969llu,
    7790874974804249978llu,
    11664943172575271568llu,
    1579272764917527478llu,
    15516260931218054255llu,
    7839395787036447128llu,
    15249406189344775469llu,
    6791182396147619394llu,
    17166851829350597035llu,
    4346208168030650664llu,
    11671380077830021326llu,
    16764016049878832982llu,
    4253245639498694995llu,
    15981415790128254689llu,
    14831301216167549422llu,
    7570532686478098418llu,
    10433655379636705433llu,
    11586733253399237367llu,
    17043373472347412192llu,
    13357506643414941308llu,
    5258160458685301505llu,
    3346984812667980496llu,
};

static inline uint64_t sbox_hash(const char *key);

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
        default:
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
            "    static uint64_t sbox_lut[256] = {\n");

    for (size_t i = 0; i < ARRAY_SIZE(sbox_lut); ++i) {
        fprintf(source, "       %lullu,\n", sbox_lut[i]);
    }

    fprintf(source,
            "    };\n"
            "    uint64_t res = 0;\n"
            "    for (size_t i = 0; i < tok->len; ++i) {\n"
            "        res = (res + sbox_lut[tok->lit[i]]) * 3;\n"
            "    }\n"
            "    return res;\n"
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
        uint64_t key_hash = sbox_hash(key);
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
        default: {
        }
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

static int jipg_main(int argc, char *argv[]) {
    size_t value_count = jipg_global_context.parser_count;
    static Jipg_Value *values[JIPG_PARSER_CAP];
    for (size_t i = 0; i < value_count; ++i) {
        Jipg_Parser *parser = jipg_global_context.parsers + i;
        values[i] = parser->value_gen();
        values[i]->head = parser->head_struct_name;
        jipg_generate_struct_names(
            values[i],
            parser->head_struct_name);
    }

    char *header_name = "jsonparser.h";
    char *source_name = "jsonparser.c";
    bool single_file = false;

    for (size_t idx = 1; idx < (size_t)argc; ++idx) {
        const char help_str[] = "--help";
        const char header_str[] = "--header=";
        const char source_str[] = "--source=";
        const char single_file_str[] = "--single-file";

        if (strncmp(argv[idx], help_str, strlen(help_str)) == 0) {
            printf(
                "jipg.h - the single file JSON Parser Generator for C!\n"
                "Command line options:\n"
                "  --help                  Show this message and exit.\n"
                "  --header=<header-file>  Path of generated header file.\n"
                "  --source=<source-file>  Path of generated source file.\n"
                "  --single-file           Generates single STB style header file.\n");
            return 0;
        } else if (strncmp(argv[idx], header_str, strlen(header_str)) == 0) {
            header_name = argv[idx] + strlen(header_str);
        } else if (strncmp(argv[idx], source_str, strlen(source_str)) == 0) {
            source_name = argv[idx] + strlen(source_str);
        } else if (strncmp(argv[idx], single_file_str, strlen(single_file_str)) == 0) {
            single_file = true;
        }
    }

    FILE *header = fopen(header_name, "w");
    if (header == NULL) {
        fprintf(stderr, "Unable to open %s\n", header_name);
        return 1;
    }

    jipg_emit_header(header, values, value_count, header_name);

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

    jipg_emit_source(source, values, value_count, single_file ? NULL : header_name);

    if (single_file) {
        fprintf(source, "\n#endif  // ");
        jipg_emit_header_impl_macro(header, header_name);
    }

    fclose(header);
    return 0;
}

#define JIPG_MAIN()                    \
    int main(int argc, char *argv[]) { \
        return jipg_main(argc, argv);  \
    }


static inline uint64_t sbox_hash(const char *key) {
    uint64_t res = 0;
    for (; *key; ++key) {
        res = (res + sbox_lut[*key]) * 3;
    }
    return res;
}

#endif  // JIPG_H
