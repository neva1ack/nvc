#include <nvc_ast.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void nvc_free_params(nvc_fun_param_decl_t** params, size_t size) {
    if (params) {
        for (size_t i = 0; i < size; ++i) {
            free(params[i]);
        }
        free(params);
    }
}

static void nvc_free_nodes_recursive(nvc_ast_node_t* node) {
    if (node) {
        // TODO: dont forget to free anything added here
        switch (node->kind) {
            case NVC_AST_NODE_BINARY_OP:
                nvc_free_nodes_recursive(node->binary_op.lhs);
                nvc_free_nodes_recursive(node->binary_op.rhs);
                break;
            case NVC_AST_NODE_FUN_DECL:
                nvc_free_params(node->fun_decl.params, node->fun_decl.n_params);
                break;
            case NVC_AST_NODE_LET_ASSIGN:
                nvc_free_nodes_recursive(node->let_assign.rhs);
                break;
            case NVC_AST_NODE_UNARY_OP:
                nvc_free_nodes_recursive(node->unary_op.expr);
                break;
            case NVC_AST_NODE_UNKNOWN:
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
        case NVC_AST_NODE_UNKNOWN:
        default: fputs("unknown", stdout); break;
        case NVC_AST_NODE_LET_ASSIGN:
            fprintf(stdout, "let(%s, ", node->let_assign.symbol);
            nvc_print_ast_recursive(node->let_assign.rhs);
            fputc(')', stdout);
            break;
        case NVC_AST_NODE_FUN_DECL:
            fprintf(stdout, "fun %s(", node->fun_decl.fun_name);
            for (size_t i = 0; i < node->fun_decl.n_params; ++i) {
                fprintf(stdout, "%s: %s,", node->fun_decl.params[i]->param_name,
                        node->fun_decl.params[i]->type_name);
            }
            fprintf(stdout, ") -> %s(", node->fun_decl.return_type_name);
            // TODO: print body
            // nvc_print_ast_recursive();
            fputc(')', stdout);
            break;
        case NVC_AST_NODE_STRING_LIT:
            fprintf(stdout, "'%s'", node->let_assign.rhs->str_lit);
            break;
        case NVC_AST_NODE_UNARY_OP: fputs("unary", stdout); break;
        case NVC_AST_NODE_INT_LIT:
            fprintf(stdout, "%ld", node->let_assign.rhs->i);
            break;
        case NVC_AST_NODE_FLOAT_LIT:
            fprintf(stdout, "%.2f", node->let_assign.rhs->d);
            break;
        case NVC_AST_NODE_BINARY_OP: fputs("binary", stdout); break;
    }
}

void nvc_print_ast(nvc_ast_t* ast) {
    for (size_t i = 0; i < ast->size; ++i) {
        nvc_print_ast_recursive(ast->nodes[i]);
    }

    fputc('\n', stdout);
}

void nvc_free_ast(nvc_ast_t* ast) {
    if (ast) {
        nvc_free_nodes(ast->nodes, ast->size);
        free(ast);
    }
}

static nvc_ast_node_t* nvc_parse_recursive(nvc_token_stream_t stream,
                                           size_t* eaten,
                                           int depth) {
    // error case
    if (stream.size <= 0) {
        fprintf(stderr, "parsing error: recursive call on empty stream.\n");
        goto error;
    }
    // base case
    if (stream.size == 1) {
        switch (stream.tokens->kind) {
            case NVC_TOK_INT_LIT: {
                *eaten = 1;
                nvc_ast_node_t* node = calloc(1, sizeof(nvc_ast_node_t));
                node->kind = NVC_AST_NODE_INT_LIT;
                node->i = stream.tokens->int_lit;
                return node;
            }
            case NVC_TOK_FLOAT_LIT: {
                *eaten = 1;
                nvc_ast_node_t* node = calloc(1, sizeof(nvc_ast_node_t));
                node->kind = NVC_AST_NODE_FLOAT_LIT;
                node->d = stream.tokens->float_lit;
                return node;
            }
            case NVC_TOK_STR_LIT: {
                *eaten = 1;
                nvc_ast_node_t* node = calloc(1, sizeof(nvc_ast_node_t));
                node->kind = NVC_AST_NODE_STRING_LIT;
                node->str_lit = stream.tokens->str_lit;
                return node;
            }
            case NVC_TOK_OP:
                // TODO: this is most likely illegal
            case NVC_TOK_SYMBOL:
                // TODO: probably generate a reference of some kind here
            case NVC_TOK_NEWLINE:
            case NVC_TOK_EOF:
            case NVC_TOK_UNKNOWN:
                fprintf(stderr,
                        "parsing error: recursive call on illegal or unknown "
                        "token.\n");
                goto error;
        }
    }

    if (stream.tokens->kind == NVC_TOK_SYMBOL) {
        // scan for let keyword
        if (strncmp(stream.tokens->symbol, "let", 4) == 0) {
            // find next eof/newline
            nvc_tok_t* ptr = stream.tokens;
            while (++ptr &&
                   (ptr->kind != NVC_TOK_NEWLINE && ptr->kind != NVC_TOK_EOF))
                ;
            // must be at least 4 tokens in a let assign
            // e.g. symbol(let) symbol(var_name) op(4) int_lit(2)
            size_t rem_tokens = ptr - stream.tokens;
            if (rem_tokens < 4) {
                // TODO: syntax error
                fprintf(stderr,
                        "syntax error: invalid let decl only %ld"
                        "tokens need at least 4.\n",
                        rem_tokens);
                goto error;
            }

            *eaten = rem_tokens;
            size_t sub_eaten = 0;

            nvc_token_stream_t sub_stream = {.tokens = stream.tokens + 3,
                                             .size = rem_tokens - 3};

            nvc_ast_node_t* rhs =
                nvc_parse_recursive(sub_stream, &sub_eaten, depth + 1);

            if (!rhs) {
                // TODO: syntax error
                // note: error is reported by recursive call so that will
                // fprintf
                goto error;
            }

            if (sub_eaten != sub_stream.size) {
                // TODO: syntax error
                fprintf(stderr,
                        "syntax error: let rhs parse recursive call must eat "
                        "all %ld tokens.\n",
                        rem_tokens - 3);
                free(rhs);
                goto error;
            }

            nvc_ast_node_t* let_assign = calloc(1, sizeof(nvc_ast_node_t));
            if (!let_assign) {
                fprintf(stderr, "Out of memory!\n");
                free(rhs);
                goto error;
            }
            let_assign->kind = NVC_AST_NODE_LET_ASSIGN;
            // use token symbol note: this means it will be free when the
            // backing memory of the tokens is freed aka when the final
            // nvc_free_token_stream is called
            let_assign->let_assign.symbol = stream.tokens[1].symbol;
            let_assign->let_assign.rhs = rhs;

            return let_assign;
        }
        // scan for fun keyword
        else if (strncmp(stream.tokens->symbol, "fun", 4) == 0) {
            fprintf(stdout, "parsing fun.\n");
            // eat newlines
            nvc_tok_t* ptr = stream.tokens;
            while (++ptr && ptr->kind == NVC_TOK_NEWLINE)
                ;
            // check 1st token is a symbol (function name)
            if (!ptr || ptr->kind != NVC_TOK_SYMBOL) {
                fprintf(stderr,
                        "syntax error: malformed fun decl: missing function "
                        "name.\n");
                goto error;
            }
            char* fun_name = ptr->symbol;
            // eat more newlines (nom nom nom)
            while (++ptr && ptr->kind == NVC_TOK_NEWLINE)
                ;
            // check 2nd token is an (
            if (!ptr || ptr->kind != NVC_TOK_OP ||
                ptr->op_kind != NVC_OP_LPAREN) {
                fprintf(stderr,
                        "syntax error: malformed fun decl: missing '('.\n");
                goto error;
            }
            // eat more newlines (nom nom nom)
            while (++ptr && ptr->kind == NVC_TOK_NEWLINE)
                ;
            // parse param decl list
            // syntax: symbol(param_name) op(17) symbol(type_name) op(23)
            //                                                      ^^^
            //                                          optional for the last
            //                                          param
            size_t n_params = 0;
            size_t capacity = 1 << 3;
            nvc_fun_param_decl_t** params =
                calloc(capacity, sizeof(nvc_fun_param_decl_t*));
            if (!params) {
                fprintf(stderr, "Out of memory!\n");
                goto error;
            }
            while (1) {
                // check for param name
                if (!ptr || ptr->kind != NVC_TOK_SYMBOL) {
                    fprintf(stderr,
                            "syntax error: malformed fun decl: missing param "
                            "name.\n");
                    nvc_free_params(params, n_params);
                    goto error;
                }
                char* param_name = ptr->symbol;
                // eat more newlines (nom nom nom)
                while (++ptr && ptr->kind == NVC_TOK_NEWLINE)
                    ;
                // check for type annotation
                if (!ptr || ptr->kind != NVC_TOK_OP ||
                    ptr->op_kind != NVC_OP_TYPE_ANNOTATION) {
                    fprintf(stderr,
                            "syntax error: malformed fun decl: missing param "
                            "type annotation operator ':'.\n");
                    nvc_free_params(params, n_params);
                    goto error;
                }
                // eat more newlines (nom nom nom)
                while (++ptr && ptr->kind == NVC_TOK_NEWLINE)
                    ;
                // check for type name
                if (!ptr || ptr->kind != NVC_TOK_SYMBOL) {
                    fprintf(stderr,
                            "syntax error: malformed fun decl: missing param "
                            "type name.\n");
                    nvc_free_params(params, n_params);
                    goto error;
                }
                char* param_type_name = ptr->symbol;
                // eat more newlines (nom nom nom)
                while (++ptr && ptr->kind == NVC_TOK_NEWLINE)
                    ;

                if (!ptr) {
                    fprintf(stderr, "syntax error: malformed fun decl.\n");
                    nvc_free_params(params, n_params);
                    goto error;
                }

                // append param
                nvc_fun_param_decl_t* param_decl =
                    calloc(1, sizeof(nvc_fun_param_decl_t));
                if (!param_decl) {
                    fprintf(stderr, "Out of memory!\n");
                    nvc_free_params(params, n_params);
                    goto error;
                }
                param_decl->param_name = param_name;
                param_decl->type_name = param_type_name;

                params[n_params] = param_decl;
                ++n_params;

                // dynamic allocation
                if (n_params >= capacity) {
                    // increase by 50%
                    capacity = (n_params / 2) * 3;
                    // TODO: remove me
                    fprintf(stdout, "Dynamic allocation %ld -> %ld.\n",
                            n_params, capacity);
                    // reallocate with increased capacity
                    params = calloc(capacity, sizeof(nvc_fun_param_decl_t*));
                    if (!params) {
                        fprintf(stderr, "Out of memory!\n");
                        nvc_free_params(params, n_params);
                        goto error;
                    }
                }

                // if hits ')' end param list
                if (ptr->kind == NVC_TOK_OP && ptr->op_kind == NVC_OP_RPAREN) {
                    break;
                }
                // else check for ','
                else if (ptr->kind != NVC_TOK_OP ||
                         ptr->op_kind != NVC_OP_COMMA) {
                    fprintf(stderr,
                            "syntax error: malformed fun decl: missing ',' "
                            "after param.\n");
                    nvc_free_params(params, n_params);
                    goto error;
                }
                // check enclosing ')' in case of optional ','
                if (++ptr && ptr->kind == NVC_TOK_OP &&
                    ptr->op_kind == NVC_OP_RPAREN) {
                    break;
                }
            }
            // eat more newlines (nom nom nom)
            while (++ptr && ptr->kind == NVC_TOK_NEWLINE)
                ;
            // check for return decl
            if (!ptr || ptr->kind != NVC_TOK_OP ||
                ptr->op_kind != NVC_OP_RET_DECL) {
                fprintf(stderr,
                        "syntax error: malformed fun decl: missing return "
                        "declaration: '->'.\n");
                nvc_free_params(params, n_params);
                goto error;
            }
            // eat more newlines (nom nom nom)
            while (++ptr && ptr->kind == NVC_TOK_NEWLINE)
                ;
            // check for return type name
            if (!ptr || ptr->kind != NVC_TOK_SYMBOL) {
                fprintf(stderr,
                        "syntax error: malformed fun decl: missing return type "
                        "name.\n");
                nvc_free_params(params, n_params);
                goto error;
            }
            char* return_type_name = ptr->symbol;

            // TODO: body

            nvc_ast_node_t* fun_decl = calloc(1, sizeof(nvc_ast_node_t));
            if (!fun_decl) {
                fprintf(stderr, "Out of memory!\n");
                // TODO: going to have to free body here once it's implemented!
                nvc_free_params(params, n_params);
                goto error;
            }
            fun_decl->kind = NVC_AST_NODE_FUN_DECL;
            fun_decl->fun_decl.fun_name = fun_name;
            fun_decl->fun_decl.return_type_name = return_type_name;
            fun_decl->fun_decl.params = params;
            fun_decl->fun_decl.n_params = n_params;

            fprintf(stdout, "done parsing fun: %ld.\n",
                    (ptr - stream.tokens) + 1);
            *eaten = (ptr - stream.tokens) + 1;
            return fun_decl;
        }
    }

    fprintf(stderr, "Unreachable code!\n");
error:
    *eaten = 0;
    return NULL;
}

nvc_ast_t* nvc_parse(nvc_token_stream_t* stream) {
    // initially allocate spaces for 2^3 nodes
    size_t capacity = 1 << 3;
    nvc_ast_node_t** nodes = calloc(capacity, sizeof(nvc_ast_node_t*));
    if (!nodes) {
        fprintf(stderr, "Out of memory!\n");
        return NULL;
    }
    size_t n_nodes = 0;

    for (size_t i = 0; i < stream->size; ++i) {
        size_t eaten = 0;
        nvc_ast_node_t* node = NULL;

        if (i == 0) {
            node = nvc_parse_recursive(*stream, &eaten, 0);
        } else {
            nvc_token_stream_t sub_stream = {
                .tokens = stream->tokens + i,
                .size = stream->size - i,
            };

            node = nvc_parse_recursive(sub_stream, &eaten, 0);
        }

        if (!node) {
            nvc_free_nodes(nodes, n_nodes);
            return NULL;
        }

        nodes[n_nodes] = node;
        ++n_nodes;
        i += eaten;

        // dynamic allocation
        if (n_nodes >= capacity) {
            // increase by 50%
            capacity = (n_nodes / 2) * 3;
            // TODO: remove me
            fprintf(stdout, "Dynamic allocation %ld -> %ld.\n", n_nodes,
                    capacity);
            // reallocate with increased capacity
            nodes = calloc(capacity, sizeof(nvc_ast_node_t*));
            if (!nodes) {
                fprintf(stderr, "Out of memory!\n");
                nvc_free_nodes(nodes, n_nodes);
                return NULL;
            }
        }
    }

    // shrink nodes to save memory
    // nodes = realloc(nodes, n_nodes * sizeof(nvc_ast_node_t*));
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
