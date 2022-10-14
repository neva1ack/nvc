#ifdef __cplusplus
extern "C" {
#endif

#ifndef NVC_AST_H
#define NVC_AST_H

#include <nvc_lexer.h>

typedef enum {
    // literals
    NVC_AST_NODE_INT_LIT = 0,
    NVC_AST_NODE_FP_LIT = 1,
    NVC_AST_NODE_STRING_LIT = 2,
    // decls
    NVC_AST_NODE_LET_DECL = 10,
    NVC_AST_NODE_FUN_DECL = 11,
    NVC_AST_NODE_TYPE_DECL = 12,
    // operator chain
    NVC_AST_NODE_OP_CHAIN = 20,
} nvc_ast_node_kind_t;

typedef enum {
    NVC_BIN_OP_UNKNOWN = -1,

    // note: the indices match the nvc_operator_kind_t so it can be printed
    NVC_BIN_OP_LT = 0,  // <
    NVC_BIN_OP_LE = 1,  // <=
    NVC_BIN_OP_GT = 2,  // >
    NVC_BIN_OP_GE = 3,  // >=

    NVC_BIN_OP_ADD = 5,   // +
    NVC_BIN_OP_SUB = 6,   // -
    NVC_BIN_OP_MUL = 7,   // *
    NVC_BIN_OP_DIV = 8,   // /
    NVC_BIN_OP_POW = 10,  // ^

    // TODO: assignment operators?
} nvc_binary_op_kind_t;

typedef enum {
    NVC_UN_OP_UNKNOWN = -1,

    // note: the indices match the nvc_operator_kind_t so it can be printed
    NVC_UN_OP_ADD = 5,  // + note: this does not do anything
    NVC_UN_OP_SUB = 6,  // -
    NVC_UN_OP_NEG = 9,  // ~
} nvc_unary_op_kind_t;

typedef struct nvc_ast_node_s nvc_ast_node_t;

typedef struct {
    char* symbol;  // this will be freed when the owning nvc_token_stream_t is
                   // freed
    nvc_ast_node_t* rhs;  // this must be freed after use
} nvc_ast_let_decl_t;

typedef struct {
    char* param_name;  // both of these fields are freed when the owning
    char* type_name;   // nvc_token_stream_t is freed
} nvc_fun_param_decl_t;

typedef struct {
    char*
        fun_name;  // both of these fields are freed when nvc_free_token_stream
    char* return_type_name;  // is called on the nvc_token_stream_t used as a
                             // parameter to create this nvc_ast_t with
                             // nvc_parse_t note: return_type_name may be NULL
    nvc_fun_param_decl_t** params;  // this and the pointers pointed to MUST be
                                    // freed after use UNLESS it is NULL
    uint32_t n_params;
    nvc_ast_node_t** body;  // this and the pointers pointed to must be freed
                            // after use UNLESS it is NULL
    uint32_t body_size;
} nvc_ast_fun_decl_t;

typedef struct {
    char* member_name;  // both of these fields are freed when the owning
    char* type_name;    // nvc_token_stream_t is freed
} nvc_type_member_decl_t;

typedef struct {
    char* type_name;
    nvc_type_member_decl_t** members;
    uint32_t n_members;
} nvc_ast_type_decl_t;

typedef enum {
    NVC_CHAIN_ELEM_UNARY_OP = 0,
    NVC_CHAIN_ELEM_BINARY_OP = 1,
    NVC_CHAIN_ELEM_NODE = 2,
} nvc_chain_elem_kind_t;

typedef struct {
    nvc_chain_elem_kind_t kind;
    union {
        nvc_unary_op_kind_t unary_op_kind;
        nvc_binary_op_kind_t binary_op_kind;
        nvc_ast_node_t* node;  // this MUST be freed
    };
} nvc_ast_chain_elem_t;

typedef struct {
    nvc_ast_chain_elem_t**
        elems;  // this pointer and pointers it points to MUST be freed
    uint32_t n_elems;
} nvc_ast_op_chain_t;

struct nvc_ast_node_s {
    nvc_ast_node_kind_t kind;
    union {
        // literals
        nvc_int i;      // note: this cannot (and will not) be negative
        nvc_fp fp;      // note: this cannot (and will not) be negative
        char* str_lit;  // this will be freed when the owning
                        // nvc_free_token_stream freed

        // operator chain
        nvc_ast_op_chain_t op_chain;

        // declarations
        nvc_ast_let_decl_t let_decl;
        nvc_ast_fun_decl_t fun_decl;
        nvc_ast_type_decl_t type_decl;
    };
};

typedef struct {
    nvc_ast_node_t**
        nodes;  // this and the pointers it points to must be freed after use
    uint32_t size;
} nvc_ast_t;

void nvc_print_ast(nvc_ast_t* ast);

void nvc_free_ast(nvc_ast_t* ast);

nvc_ast_t* nvc_parse(nvc_token_stream_t* stream);

#endif  // NVC_AST_H

#ifdef __cplusplus
}
#endif
