#ifdef __cplusplus
extern "C" {
#endif

#ifndef NVC_LEXER_H
#define NVC_LEXER_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <nvc_output.h>

typedef int64_t nvc_int;
typedef long double nvc_fp;

typedef enum {
    NVC_OP_UNKNOWN = -1,

    NVC_OP_LT = 0,  // <
    NVC_OP_LE = 1,  // <=
    NVC_OP_GT = 2,  // >
    NVC_OP_GE = 3,  // >=
    NVC_OP_EQ = 4,  // =

    NVC_OP_ADD = 5,   // +
    NVC_OP_SUB = 6,   // -
    NVC_OP_MUL = 7,   // *
    NVC_OP_DIV = 8,   // /
    NVC_OP_NEG = 9,   // ~
    NVC_OP_POW = 10,  // ^

    NVC_OP_ADD_EQ = 11,  // +=
    NVC_OP_SUB_EQ = 12,  // -=
    NVC_OP_MUL_EQ = 13,  // *=
    NVC_OP_DIV_EQ = 14,  // /=
    // TODO: dead operator
    // NVC_OP_NEG_EQ = 15, // ~=
    NVC_OP_POW_EQ = 16,  // ^=

    // special
    NVC_OP_TYPE_ANNOTATION = 17,  // :
    NVC_OP_LPAREN = 18,           // (
    NVC_OP_RPAREN = 19,           // )
    NVC_OP_LBRACKET = 20,         // [
    NVC_OP_RBRACKET = 21,         // ]
    NVC_OP_RET_DECL = 22,         // ->
    NVC_OP_COMMA = 23,            // ,
    NVC_OP_DOT = 24,              // .
} nvc_operator_kind_t;

typedef enum {
    // string literals
    NVC_TOK_STR_LIT = 0,
    // number literals
    NVC_TOK_INT_LIT = 1,
    NVC_TOK_FP_LIT = 2,
    // symbols
    NVC_TOK_SYMBOL = 3,
    // operators
    NVC_TOK_OP = 4,
} nvc_tok_kind_t;

typedef struct {
    nvc_tok_kind_t kind;
    nvc_buffer_location_t buf_loc;
    union {
        nvc_int int_lit;  // note: this cannot (and will not) be negative
        nvc_fp fp_lit;    // note: this cannot (and will not) be negative
        nvc_operator_kind_t op_kind;
        char* symbol;   // this must be freed after use and will never be NULL
        char* str_lit;  // this must be freed after use UNLESS NULL because
                        // of empty string lit
    };
} nvc_tok_t;

typedef struct {
    nvc_tok_t* tokens;  // this will be automatically freed when
                        // nvc_free_token_stream on this
    uint32_t size;
} nvc_token_stream_t;

char* nvc_op_to_str(nvc_operator_kind_t op);

void nvc_free_token_stream(nvc_token_stream_t* stream);

char* nvc_token_to_str(nvc_tok_t* token);

nvc_token_stream_t* nvc_lexical_analysis(char* bufname, char* buf, long bufsz);

#endif  // NVC_LEXER_H

#ifdef __cplusplus
}
#endif
