#ifdef __cplusplus
extern "C" {
#endif

#include <nvc_ast.h>

#include <nvc_output.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void nvc_free_ptr_array(void** ptr_array, uint32_t len) {
    if (ptr_array) {
        for (uint32_t i = 0; i < len; ++i) {
            free(ptr_array[i]);
        }
        free(ptr_array);
    }
}

static void nvc_free_nodes_recursive(nvc_ast_node_t* node) {
    if (node) {
        // TODO: dont forget to free anything added here
        switch (node->kind) {
            case NVC_AST_NODE_FUN_DECL:
                nvc_free_ptr_array(node->fun_decl.body,
                                   node->fun_decl.body_size);
                nvc_free_ptr_array(node->fun_decl.params,
                                   node->fun_decl.n_params);
                break;
            case NVC_AST_NODE_LET_DECL:
                nvc_free_nodes_recursive(node->let_decl.rhs);
                break;
            case NVC_AST_NODE_TYPE_DECL:
                nvc_free_ptr_array(node->type_decl.members,
                                   node->type_decl.n_members);
                break;
            case NVC_AST_NODE_OP_CHAIN:
                for (uint32_t i = 0; i < node->op_chain.n_elems; ++i) {
                    nvc_ast_chain_elem_t* elem = node->op_chain.elems[i];
                    if (elem->kind == NVC_CHAIN_ELEM_NODE)
                        nvc_free_nodes_recursive(elem->node);
                }
                nvc_free_ptr_array(node->op_chain.elems,
                                   node->op_chain.n_elems);
                break;
            default: break;
        }

        free(node);
    }
}

static void nvc_free_nodes(nvc_ast_node_t** nodes, size_t size) {
    if (nodes) {
        for (size_t i = 0; i < size; ++i) {
            nvc_free_nodes_recursive(nodes[i]);
        }

        free(nodes);
    }
}

static void nvc_print_ast_recursive(nvc_ast_node_t* node) {
    switch (node->kind) {
        case NVC_AST_NODE_LET_DECL:
            fprintf(stdout, "let(%s, ", node->let_decl.symbol);
            nvc_print_ast_recursive(node->let_decl.rhs);
            fputc(')', stdout);
            break;
        case NVC_AST_NODE_FUN_DECL:
            fprintf(stdout, "fun %s(", node->fun_decl.fun_name);
            for (uint32_t i = 0; i < node->fun_decl.n_params; ++i) {
                fprintf(stdout, "%s: %s,", node->fun_decl.params[i]->param_name,
                        node->fun_decl.params[i]->type_name);
            }
            fprintf(stdout, ") -> %s(", node->fun_decl.return_type_name);
            // TODO: print body pretty
            for (size_t i = 0; i < node->fun_decl.body_size; ++i) {
                nvc_print_ast_recursive(node->fun_decl.body[i]);
            }
            fputc(')', stdout);
            break;
        case NVC_AST_NODE_TYPE_DECL:
            fprintf(stdout, "type %s(", node->type_decl.type_name);
            // TODO: print type nodes
            for (uint32_t i = 0; i < node->type_decl.n_members; ++i) {
            }
            break;
        case NVC_AST_NODE_STRING_LIT:
            fprintf(stdout, "'%s'", node->str_lit);
            break;
        case NVC_AST_NODE_OP_CHAIN:
            // TODO: print op chains
            break;
        case NVC_AST_NODE_INT_LIT: fprintf(stdout, "%ld", node->i); break;
        case NVC_AST_NODE_FP_LIT: fprintf(stdout, "%.2Lf", node->fp); break;
    }
}

void nvc_print_ast(nvc_ast_t* ast) {
    for (size_t i = 0; i < ast->size; ++i) {
        nvc_print_ast_recursive(ast->nodes[i]);
        fputc('\n', stdout);
    }
}

void nvc_free_ast(nvc_ast_t* ast) {
    if (ast) {
        nvc_free_nodes(ast->nodes, ast->size);
        free(ast);
    }
}

static nvc_binary_op_kind_t nvc_map_binary_op(nvc_operator_kind_t op) {
    switch (op) {
        case NVC_OP_LE: return NVC_BIN_OP_LE;
        case NVC_OP_GE: return NVC_BIN_OP_GE;
        case NVC_OP_LT: return NVC_BIN_OP_LT;
        case NVC_OP_GT: return NVC_BIN_OP_GT;
        case NVC_OP_POW: return NVC_BIN_OP_POW;
        case NVC_OP_DIV: return NVC_BIN_OP_DIV;
        case NVC_OP_MUL: return NVC_BIN_OP_MUL;
        case NVC_OP_SUB: return NVC_BIN_OP_SUB;
        case NVC_OP_ADD: return NVC_BIN_OP_ADD;
        default: return NVC_BIN_OP_UNKNOWN;
    }
}

static nvc_unary_op_kind_t nvc_map_unary_op(nvc_operator_kind_t op) {
    switch (op) {
        case NVC_OP_NEG: return NVC_UN_OP_NEG;
        case NVC_OP_SUB: return NVC_UN_OP_SUB;
        case NVC_OP_ADD: return NVC_UN_OP_ADD;
        default: return NVC_UN_OP_UNKNOWN;
    }
}

// note: this is just SHIT
static bool nvc_chainable_tokens(nvc_tok_t* lhs, nvc_tok_t* rhs) {
    // TODO: this is hardcoded asf :sadge: this will make it difficult to have
    // operator overloads in future
    switch (lhs->kind) {
        case NVC_TOK_SYMBOL:
        case NVC_TOK_FP_LIT:
        case NVC_TOK_INT_LIT:
            // rhs must be a binary operator when the lhs is an int literal,
            // float literal, or symbol
            return rhs->kind == NVC_TOK_OP &&
                   nvc_map_binary_op(rhs->op_kind) != NVC_BIN_OP_UNKNOWN;
        case NVC_TOK_STR_LIT: break;  // TODO: string operators?
        case NVC_TOK_OP:
            // lhs must be a binary operator (or unary operator) and the rhs
            // must be an int literal, float literal, or symbol
            return (nvc_map_binary_op(lhs->op_kind) != NVC_BIN_OP_UNKNOWN ||
                    nvc_map_unary_op(lhs->op_kind) != NVC_UN_OP_UNKNOWN) &&
                   (rhs->kind == NVC_TOK_FP_LIT ||
                    rhs->kind == NVC_TOK_INT_LIT ||
                    rhs->kind == NVC_TOK_SYMBOL ||
                    // note: unary op can be rhs
                    (rhs->kind == NVC_TOK_OP &&
                     nvc_map_unary_op(rhs->op_kind) != NVC_UN_OP_UNKNOWN));
    }

    return false;
}

// static nvc_ast_op_chain_t nvc_scan_op_chain(nvc_token_stream_t stream,
//                                             uint32_t* eaten) {}
//
// static nvc_ast_op_chain_t* nvc_scan_chain(nvc_token_stream_t stream) {
//     // create chain from token stream
//
//     nvc_tok_t* ptr = stream.tokens;
//     while (ptr && ++ptr && nvc_chainable_tokens(ptr - 1, ptr))
//         ;
//     return ptr - stream.tokens;
// }

// static nvc_ast_node_t* nvc_int_lit_node(nvc_tok_t* token) {
//     nvc_ast_node_t* node = malloc(sizeof(nvc_ast_node_t));
//     node->kind = NVC_AST_NODE_INT_LIT;
//     node->i = token->int_lit;
//     return node;
// }
//
// static nvc_ast_node_t* nvc_fp_lit_node(nvc_tok_t* token) {
//     nvc_ast_node_t* node = malloc(sizeof(nvc_ast_node_t));
//     node->kind = NVC_AST_NODE_FP_LIT;
//     node->fp = token->fp_lit;
//     return node;
// }
//
// static nvc_ast_node_t* nvc_str_lit_node(nvc_tok_t* token) {
//     nvc_ast_node_t* node = malloc(sizeof(nvc_ast_node_t));
//     node->kind = NVC_AST_NODE_STRING_LIT;
//     node->str_lit = token->str_lit;
//     return node;
// }

static nvc_ast_node_t* nvc_parse_recursive(nvc_token_stream_t stream,
                                           uint32_t* eaten,
                                           int depth) {
    fprintf(stdout, "recursively parsing stream: %d %d\n", stream.size, depth);

    // empty stream fail case
    if (stream.size <= 0) {
        fprintf(stderr, "recursive call on empty stream: depth=%d\n", depth);
        goto error;
    }

    // parse single elem token stream but only for literals and fail first
    if (stream.size == 1) {
        switch (stream.tokens->kind) {
            case NVC_TOK_INT_LIT: goto parse_int_token;
            case NVC_TOK_FP_LIT: goto parse_fp_token;
            case NVC_TOK_STR_LIT: goto parse_str_token;
            case NVC_TOK_OP: {
                // this is illegal, there are no 0-ary operations
                char* tokstr = nvc_token_to_str(stream.tokens);
                fprintf(stderr,
                        "syntax error: single token stream with only operator "
                        "token: %s.\n",
                        tokstr);
                free(tokstr);
                goto error;
            }
        }
    }

    // resolve keywords first
    if (stream.tokens->kind == NVC_TOK_SYMBOL) {
        // TODO: maybe use strncmp but this should always be null terminated
        if (strncmp(stream.tokens->symbol, "let\0", 4) == 0) {
            // check 1st token is a symbol (var name)
            if (stream.size <= 1) goto kw_expect_var_name;
            nvc_tok_t* ptr = stream.tokens;
            if (!(++ptr) || ptr->kind != NVC_TOK_SYMBOL) {
                nvc_print_buffer_message("unexpected token", ptr->buf_loc);
            kw_expect_var_name:
                fprintf(stderr, "note: expected symbol(<var_name>).\n");
                goto error;
            }
            char* var_name = ptr->symbol;
            // check 2nd token is '=' op
            if (stream.size <= 2) goto kw_expect_op;
            if (!(++ptr) || ptr->kind != NVC_TOK_OP ||
                ptr->op_kind != NVC_OP_EQ) {
                nvc_print_buffer_message("unexpected token", ptr->buf_loc);
            kw_expect_op:
                fprintf(stderr, "note: expected op(%s).\n",
                        nvc_op_to_str(NVC_OP_EQ));
                goto error;
            }

            ++ptr;

            // create token stream of remaining tokens
            nvc_token_stream_t rem_tokens_stream = {
                .tokens = ptr,
                .size = stream.size - (ptr - stream.tokens),
            };

            if (rem_tokens_stream.size <= 0) {
                fprintf(stderr, "note: expected rhs token.\n");
                goto error;
            }

            uint32_t recursive_eaten = 0;
            nvc_ast_node_t* rhs = nvc_parse_recursive(
                rem_tokens_stream, &recursive_eaten, depth + 1);
            if (!rhs)
                // recursive call will emit error
                goto error;

            *eaten = (ptr - stream.tokens) + recursive_eaten;

            nvc_ast_node_t* let_decl = malloc(sizeof(nvc_ast_node_t));
            if (!let_decl) {
                fprintf(stderr, "Out of memory!\n");
                nvc_free_nodes_recursive(rhs);
                goto error;
            }
            let_decl->kind = NVC_AST_NODE_LET_DECL;
            let_decl->let_decl.symbol = var_name;
            let_decl->let_decl.rhs = rhs;

            return let_decl;
        } else if (strncmp(stream.tokens->symbol, "fun\0", 4) == 0) {
            // TODO: fun keyword
        } else if (strncmp(stream.tokens->symbol, "type\0", 5) == 0) {
            // TODO: type keyword
        }
    }

    // single token stream parsing for symbols
    // note: this is done after keyword parsing and separate to other single
    // token stream parsing to so that keywords are reserved and will error when
    // cannot be parsed
    if (stream.size == 1 && stream.tokens->kind == NVC_TOK_SYMBOL) {
        fprintf(stderr, "not implemented token!\n");
        // TODO: probably generate a reference of some kind here
        goto error;
    }

    // parse operator chain
    switch (stream.tokens->kind) {
        case NVC_TOK_SYMBOL:
            // note: this is a reference
            break;
        case NVC_TOK_OP: {
            // note: this must be a unary op
            nvc_unary_op_kind_t unary_op =
                nvc_map_unary_op(stream.tokens->op_kind);
            if (unary_op != NVC_UN_OP_UNKNOWN) {
                //                break;
            }

            nvc_print_buffer_message("illegal operator", stream.tokens->buf_loc);
            fprintf(stderr, "syntax error: illegal operator: %s\n",
                    nvc_op_to_str(stream.tokens->op_kind));
            goto error;
        }
        case NVC_TOK_INT_LIT:
            // note: this must be followed by a binary op
            break;
        case NVC_TOK_FP_LIT:
            // note: this must be followed by a binary op
            break;
        case NVC_TOK_STR_LIT:
            // note: this must be followed by a binary op
            break;
    }

    fprintf(stderr, "Unreachable code!\n");
error:
    *eaten = 0;
    return NULL;
parse_int_token : {
    *eaten = 1;
    nvc_ast_node_t* node = malloc(sizeof(nvc_ast_node_t));
    node->kind = NVC_AST_NODE_INT_LIT;
    node->i = stream.tokens->int_lit;
    return node;
}
parse_fp_token : {
    *eaten = 1;
    nvc_ast_node_t* node = malloc(sizeof(nvc_ast_node_t));
    node->kind = NVC_AST_NODE_FP_LIT;
    node->fp = stream.tokens->fp_lit;
    return node;
};
parse_str_token : {
    *eaten = 1;
    nvc_ast_node_t* node = malloc(sizeof(nvc_ast_node_t));
    node->kind = NVC_AST_NODE_STRING_LIT;
    node->str_lit = stream.tokens->str_lit;
    return node;
}
}

nvc_ast_t* nvc_parse(nvc_token_stream_t* stream) {
    // initially allocate spaces for 2^3 nodes
    uint32_t capacity = 1 << 3;
    nvc_ast_node_t** nodes = calloc(capacity, sizeof(nvc_ast_node_t*));
    if (!nodes) {
        fprintf(stderr, "Out of memory!\n");
        return NULL;
    }
    uint32_t n_nodes = 0;
    uint32_t ate = 0;

    for (; ate < stream->size;) {
        uint32_t eaten = 0;
        nvc_ast_node_t* node = NULL;

        if (ate == 0) {
            node = nvc_parse_recursive(*stream, &eaten, 0);
        } else {
            nvc_token_stream_t sub_stream = {
                .tokens = stream->tokens + ate,
                .size = stream->size - ate,
            };

            node = nvc_parse_recursive(sub_stream, &eaten, 0);
        }

        if (!node) {
            nvc_free_nodes(nodes, n_nodes);
            return NULL;
        }

        nodes[n_nodes] = node;
        ++n_nodes;
        ate += eaten;

        // dynamic allocation
        if (n_nodes >= capacity) {
            // increase by 50%
            capacity = (n_nodes / 2) * 3;
            // TODO: remove me
            fprintf(stdout, "Dynamic allocation (nodes) %d -> %d.\n", n_nodes,
                    capacity);
            // reallocate with increased capacity
            nodes = realloc(nodes, capacity * sizeof(nvc_ast_node_t*));
            if (!nodes) {
                fprintf(stderr, "Out of memory!\n");
                nvc_free_nodes(nodes, n_nodes);
                return NULL;
            }
        }
    }

    // shrink nodes to save memory
    nodes = realloc(nodes, n_nodes * sizeof(nvc_ast_node_t*));
    // allocate abstract syntax tree
    nvc_ast_t* ast = calloc(1, sizeof(nvc_ast_t));
    if (!ast) {
        nvc_free_nodes(nodes, n_nodes);
        fprintf(stderr, "Out of memory!\n");
        return NULL;
    }
    ast->nodes = nodes;
    ast->size = n_nodes;
    return ast;
}

#ifdef __cplusplus
}
#endif
