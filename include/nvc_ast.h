#ifndef NVC_AST_H
#define NVC_AST_H

#include <nvc_lexer.h>

typedef enum {
    NVC_AST_NODE_UNKNOWN = -1,

    NVC_AST_NODE_INT_LIT = 0,
    NVC_AST_NODE_FLOAT_LIT = 1,
    NVC_AST_NODE_BINARY_OP = 2,
    NVC_AST_NODE_UNARY_OP = 3,
    NVC_AST_NODE_LET_ASSIGN = 4,
    NVC_AST_NODE_STRING_LIT = 5,
    NVC_AST_NODE_FUN_DECL = 6,
} nvc_ast_node_kind_t;

typedef struct nvc_ast_node_s nvc_ast_node_t;

typedef struct {
    nvc_operator_kind_t op;
    nvc_ast_node_t* lhs;  // this must be freed after use
    nvc_ast_node_t* rhs;  // this must be freed after use
} nvc_ast_binary_op_t;

typedef struct {
    nvc_operator_kind_t op;
    nvc_ast_node_t* expr;  // this must be freed after use
} nvc_ast_unary_op_t;

typedef struct {
    char* symbol;  // this will be freed when nvc_free_token_stream is called on
                   // the nvc_token_stream_t used as a parameter to create this
                   // nvc_ast_t
    nvc_ast_node_t* rhs;  // this must be freed after use
} nvc_ast_let_assign_t;

typedef struct {
    char* param_name;  // both of these fields are freed when
                       // nvc_free_token_stream is called on
    char* type_name;   // the nvc_token_stream_t used as a parameter to create
                       // this nvc_ast_t
} nvc_fun_param_decl_t;

typedef struct {
    char*
        fun_name;  // both of these fields are freed when nvc_free_token_stream
    char* return_type_name;  // is called on the nvc_token_stream_t used as a
                             // parameter to create this nvc_ast_t with
                             // nvc_parse_t
    nvc_fun_param_decl_t** params;  // this and the pointers pointed to must be freed after use
    size_t n_params;
    // TODO: fun body
} nvc_ast_fun_decl_t;

struct nvc_ast_node_s {
    nvc_ast_node_kind_t kind;
    union {
        int64_t i;
        double d;
        char* str_lit;  // this will be freed when nvc_free_token_stream is
                        // called on the nvc_token_stream_t used as a parameter
                        // to create this nvc_ast_t
        nvc_ast_binary_op_t binary_op;
        nvc_ast_unary_op_t unary_op;
        nvc_ast_let_assign_t let_assign;
        nvc_ast_fun_decl_t fun_decl;
    };
};

typedef struct {
    nvc_ast_node_t**
        nodes;  // this and the pointers it points to must be freed after use
    size_t size;
} nvc_ast_t;

void nvc_print_ast(nvc_ast_t* ast);

void nvc_free_ast(nvc_ast_t* ast);

nvc_ast_t* nvc_parse(nvc_token_stream_t* stream);

#endif  // NVC_AST_H
