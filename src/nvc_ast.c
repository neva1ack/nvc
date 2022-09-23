#ifdef __cplusplus
extern "C" {
#endif

#include <nvc_ast.h>

#include <nvc_output.h>
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

static void nvc_free_nodes(nvc_ast_node_t** nodes, size_t size);

static void nvc_free_nodes_recursive(nvc_ast_node_t* node) {
    if (node) {
        // TODO: dont forget to free anything added here
        switch (node->kind) {
            case NVC_AST_NODE_BINARY_OP:
                nvc_free_nodes_recursive(node->binary_op.lhs);
                nvc_free_nodes_recursive(node->binary_op.rhs);
                break;
            case NVC_AST_NODE_FUN_DECL:
                nvc_free_nodes(node->fun_decl.body, node->fun_decl.body_len);
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
            // TODO: print body pretty
            for (size_t i = 0; i < node->fun_decl.body_len; ++i) {
                nvc_print_ast_recursive(node->fun_decl.body[i]);
            }
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

    // parse single token nodes
    if (stream.size == 1) {
        switch (stream.tokens->kind) {
            case NVC_TOK_INT_LIT: {
                *eaten = 1;
                nvc_ast_node_t* node = malloc(sizeof(nvc_ast_node_t));
                node->kind = NVC_AST_NODE_INT_LIT;
                node->i = stream.tokens->int_lit;
                return node;
            }
            case NVC_TOK_FLOAT_LIT: {
                *eaten = 1;
                nvc_ast_node_t* node = malloc(sizeof(nvc_ast_node_t));
                node->kind = NVC_AST_NODE_FLOAT_LIT;
                node->d = stream.tokens->float_lit;
                return node;
            }
            case NVC_TOK_STR_LIT: {
                *eaten = 1;
                nvc_ast_node_t* node = malloc(sizeof(nvc_ast_node_t));
                node->kind = NVC_AST_NODE_STRING_LIT;
                node->str_lit = stream.tokens->str_lit;
                return node;
            }
            case NVC_TOK_SYMBOL:
                // TODO: probably generate a reference of some kind here
            case NVC_TOK_OP:
                // TODO: this is illegal there are no 0-operand operators
            case NVC_TOK_SPECIAL:
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
            // eat newlines
            nvc_tok_t* ptr = stream.tokens;
            while (++ptr && ptr->kind == NVC_TOK_SPECIAL &&
                   ptr->spec_kind == NVC_SPEC_TOK_NEWLINE)
                ;
            // check 1st token is a symbol (var name)
            if (!ptr || ptr->kind != NVC_TOK_SYMBOL) {
                nvc_print_unexpected_token(ptr);
                fprintf(stderr, "note: expected symbol(<var_name>).\n");
                goto error;
            }
            char* var_name = ptr->symbol;
            // eat newlines (nom nom nom)
            while (++ptr && ptr->kind == NVC_TOK_SPECIAL &&
                   ptr->spec_kind == NVC_SPEC_TOK_NEWLINE)
                ;
            // check 2nd token is '=' op
            if (!ptr || ptr->kind != NVC_TOK_OP || ptr->op_kind != NVC_OP_EQ) {
                nvc_print_unexpected_token(ptr);
                fprintf(stderr, "note: expected op(%s).\n",
                        nvc_op_to_str(NVC_OP_EQ));
                goto error;
            }
            // eat newlines (nom nom nom)
            while (++ptr && ptr->kind == NVC_TOK_SPECIAL &&
                   ptr->spec_kind == NVC_SPEC_TOK_NEWLINE)
                ;

            size_t subrem = ptr - stream.tokens;
            // remaining tokens stream
            nvc_token_stream_t let_rvalue_expr_stream = {
                .tokens = ptr, .size = stream.size - subrem};

            fprintf(stdout, "parsing let rvalue stream:\n");

            for (size_t i = 0; i < let_rvalue_expr_stream.size; ++i) {
                char* tokstr =
                    nvc_token_to_str(let_rvalue_expr_stream.tokens + i);
                fprintf(stdout, "\t%s\n", tokstr);
                free(tokstr);
            }

            size_t sub_eaten = 0;
            nvc_ast_node_t* rhs = nvc_parse_recursive(let_rvalue_expr_stream,
                                                      &sub_eaten, depth + 1);
            if (!rhs) {
                // recursive call will emit error
                goto error;
            }

            *eaten = subrem + sub_eaten;
            fprintf(stdout, "eaten: %ld\n", *eaten);

            nvc_ast_node_t* let_assign = calloc(1, sizeof(nvc_ast_node_t));
            if (!let_assign) {
                fprintf(stderr, "Out of memory!\n");
                nvc_free_nodes_recursive(rhs);
                goto error;
            }
            let_assign->kind = NVC_AST_NODE_LET_ASSIGN;
            // use token symbol note: this means it will be free when the
            // backing memory of the tokens is freed aka when the final
            // nvc_free_token_stream is called
            let_assign->let_assign.symbol = stream.tokens[1].symbol;
            let_assign->let_assign.rhs = rhs;

            return let_assign;

            //            // eat newlines
            //            nvc_tok_t* ptr = stream.tokens;
            //            while (++ptr && ptr->kind == NVC_TOK_SPECIAL &&
            //                   ptr->spec_kind == NVC_SPEC_TOK_NEWLINE)
            //                ;
            //            // must be at least 4 tokens in a let assign
            //            // e.g. symbol(let) symbol(var_name) op(4) int_lit(2)
            //            size_t rem_tokens = ptr - stream.tokens;
            //            if (rem_tokens < 4) {
            //                // TODO: syntax error
            //                fprintf(stderr,
            //                        "syntax error: invalid let decl only %ld"
            //                        "tokens need at least 4.\n",
            //                        rem_tokens);
            //                goto error;
            //            }
            //
            //            *eaten = rem_tokens;
            //            size_t sub_eaten = 0;
            //
            //            nvc_token_stream_t sub_stream = {.tokens =
            //            stream.tokens + 3,
            //                                             .size = rem_tokens -
            //                                             3};
            //
            //            nvc_ast_node_t* rhs =
            //                nvc_parse_recursive(sub_stream, &sub_eaten, depth
            //                + 1);
            //
            //            if (!rhs) {
            //                // TODO: syntax error
            //                // note: error is reported by recursive call so
            //                that will
            //                // fprintf
            //                goto error;
            //            }
            //
            //            if (sub_eaten != sub_stream.size) {
            //                // TODO: syntax error
            //                fprintf(stderr,
            //                        "syntax error: let rhs parse recursive
            //                        call must eat " "all %ld tokens.\n",
            //                        rem_tokens - 3);
            //                nvc_free_nodes_recursive(rhs);
            //                goto error;
            //            }
            //
            //            nvc_ast_node_t* let_assign = calloc(1,
            //            sizeof(nvc_ast_node_t)); if (!let_assign) {
            //                fprintf(stderr, "Out of memory!\n");
            //                nvc_free_nodes_recursive(rhs);
            //                goto error;
            //            }
            //            let_assign->kind = NVC_AST_NODE_LET_ASSIGN;
            //            // use token symbol note: this means it will be free
            //            when the
            //            // backing memory of the tokens is freed aka when the
            //            final
            //            // nvc_free_token_stream is called
            //            let_assign->let_assign.symbol =
            //            stream.tokens[1].symbol; let_assign->let_assign.rhs =
            //            rhs;
            //
            //            return let_assign;
        }
        // scan for fun keyword
        else if (strncmp(stream.tokens->symbol, "fun", 4) == 0) {
            fprintf(stdout, "parsing fun.\n");
            // eat newlines
            nvc_tok_t* ptr = stream.tokens;
            while (++ptr && ptr->kind == NVC_TOK_SPECIAL &&
                   ptr->spec_kind == NVC_SPEC_TOK_NEWLINE)
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
            while (++ptr && ptr->kind == NVC_TOK_SPECIAL &&
                   ptr->spec_kind == NVC_SPEC_TOK_NEWLINE)
                ;
            // check 2nd token is an (
            if (!ptr || ptr->kind != NVC_TOK_OP ||
                ptr->op_kind != NVC_OP_LPAREN) {
                fprintf(stderr,
                        "syntax error: malformed fun decl: missing '('.\n");
                goto error;
            }
            // eat more newlines (nom nom nom)
            while (++ptr && ptr->kind == NVC_TOK_SPECIAL &&
                   ptr->spec_kind == NVC_SPEC_TOK_NEWLINE)
                ;
            // parse param decl list
            // syntax: symbol(param_name) op(17) symbol(type_name) op(23)
            //                                                      ^^^
            //                                          optional for the last
            //                                          param
            size_t n_params = 0;
            // predict capacity by how many : before the ) note: this could
            // probably go horribly wrong if bad syntax
            size_t capacity = 0;
            // own scope to "hide" end_ptr
            {
                nvc_tok_t* end_ptr = ptr;
                while (++end_ptr) {
                    if (end_ptr->kind == NVC_TOK_OP &&
                        end_ptr->op_kind == NVC_OP_RPAREN)
                        break;
                    else if (end_ptr->kind == NVC_TOK_SPECIAL &&
                             end_ptr->spec_kind == NVC_SPEC_TOK_EOF) {
                        fprintf(stderr,
                                "syntax error: malformed fun decl: unclosed "
                                "param list decl.\n");
                        goto error;
                    } else if (end_ptr->kind == NVC_TOK_OP &&
                               end_ptr->op_kind == NVC_OP_TYPE_ANNOTATION)
                        ++capacity;
                }
            }

            nvc_fun_param_decl_t** params =
                calloc(capacity, sizeof(nvc_fun_param_decl_t*));
            if (!params) {
                fprintf(stderr, "Out of memory!\n");
                goto error;
            }
            while (ptr && ptr->kind != NVC_TOK_OP ||
                   ptr->op_kind != NVC_OP_RPAREN) {
                // check for param name
                if (ptr->kind != NVC_TOK_SYMBOL) {
                    fprintf(stderr,
                            "syntax error: malformed fun decl: missing param "
                            "name.\n");
                    nvc_free_params(params, n_params);
                    goto error;
                }
                char* param_name = ptr->symbol;
                // eat more newlines (nom nom nom)
                while (++ptr && ptr->kind == NVC_TOK_SPECIAL &&
                       ptr->spec_kind == NVC_SPEC_TOK_NEWLINE)
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
                while (++ptr && ptr->kind == NVC_TOK_SPECIAL &&
                       ptr->spec_kind == NVC_SPEC_TOK_NEWLINE)
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
                while (++ptr && ptr->kind == NVC_TOK_SPECIAL &&
                       ptr->spec_kind == NVC_SPEC_TOK_NEWLINE)
                    ;

                if (!ptr) {
                    fprintf(stderr, "syntax error: malformed fun decl.\n");
                    nvc_free_params(params, n_params);
                    goto error;
                }

                // create param
                nvc_fun_param_decl_t* param_decl =
                    calloc(1, sizeof(nvc_fun_param_decl_t));
                if (!param_decl) {
                    fprintf(stderr, "Out of memory!\n");
                    nvc_free_params(params, n_params);
                    goto error;
                }
                param_decl->param_name = param_name;
                param_decl->type_name = param_type_name;

                // dynamic allocation
                if (n_params >= capacity) {
                    // increase by 50%
                    capacity = (n_params / 2) * 3;
                    // TODO: remove me
                    fprintf(stdout, "Dynamic allocation (params) %ld -> %ld.\n",
                            n_params, capacity);
                    // reallocate with increased capacity
                    params = realloc(params,
                                     capacity * sizeof(nvc_fun_param_decl_t*));
                    if (!params) {
                        fprintf(stderr, "Out of memory!\n");
                        nvc_free_params(params, n_params);
                        goto error;
                    }
                }

                // insert param
                params[n_params] = param_decl;
                ++n_params;

                // if hits ')' end param list
                if (ptr->kind == NVC_TOK_OP && ptr->op_kind == NVC_OP_RPAREN) {
                    fprintf(stdout, "end param list, no optional ','.\n");
                    break;
                }
                // else make sure comma is there
                else if (ptr->kind != NVC_TOK_OP ||
                         ptr->op_kind != NVC_OP_COMMA) {
                    fprintf(stderr,
                            "syntax error: malformed fun decl: missing ',' "
                            "after param.\n");
                    nvc_free_params(params, n_params);
                    goto error;
                }

                // eat more newlines (nom nom nom)
                while (++ptr && ptr->kind == NVC_TOK_SPECIAL &&
                       ptr->spec_kind == NVC_SPEC_TOK_NEWLINE)
                    ;

                //                    fprintf(stderr,
                //                            "syntax error: malformed fun decl:
                //                            unknown token in param list.\n");
                //                    nvc_free_params(params, n_params);
                //                    goto error;
            }

            if (n_params == 0) {
                // nothing is allocated so just set ptr to NULL to avoid
                // confusion
                params = NULL;
            } else {
                // realloc params to minimal size
                params =
                    realloc(params, n_params * sizeof(nvc_fun_param_decl_t*));
                if (!params) {
                    fprintf(stderr, "Out of memory!\n");
                    nvc_free_params(params, n_params);
                    goto error;
                }
            }

            // eat more newlines (nom nom nom)
            while (++ptr && ptr->kind == NVC_TOK_SPECIAL &&
                   ptr->spec_kind == NVC_SPEC_TOK_NEWLINE)
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
            while (++ptr && ptr->kind == NVC_TOK_SPECIAL &&
                   ptr->spec_kind == NVC_SPEC_TOK_NEWLINE)
                ;

            if (!ptr) {
                fprintf(stderr,
                        "syntax error: malformed fun decl: missing token after "
                        "return decl.\n");
                nvc_free_params(params, n_params);
                goto error;
            }
            char* return_type_name = NULL;
            // check for return type name
            if (ptr->kind == NVC_TOK_SYMBOL) {
                return_type_name = ptr->symbol;
                // eat more newlines (nom nom nom)
                while (++ptr && ptr->kind == NVC_TOK_SPECIAL &&
                       ptr->spec_kind == NVC_SPEC_TOK_NEWLINE)
                    ;
                // check for ( after return type name
                if (!ptr || ptr->kind != NVC_TOK_OP ||
                    ptr->op_kind != NVC_OP_LPAREN) {
                    fprintf(stderr,
                            "syntax error: malformed fun decl: missing (.\n");
                    nvc_free_params(params, n_params);
                    goto error;
                }
            }
            // void return type
            else if (ptr->kind == NVC_TOK_OP && ptr->op_kind == NVC_OP_LPAREN) {
                return_type_name = NULL;
            } else {
                fprintf(stderr,
                        "syntax error: malformed fun decl: unknown token after "
                        "return decl, must be return type name or opening fun "
                        "body '('.\n");
                nvc_free_params(params, n_params);
                goto error;
            }
            // eat more newlines (nom nom nom)
            while (++ptr && ptr->kind == NVC_TOK_SPECIAL &&
                   ptr->spec_kind == NVC_SPEC_TOK_NEWLINE)
                ;
            if (!ptr) {
                fprintf(stderr,
                        "syntax error: malformed fun decl: unknown error in "
                        "fun body.\n");
                nvc_free_params(params, n_params);
                goto error;
            }

            nvc_ast_node_t** body_nodes = NULL;
            size_t body_len = 0;

            // check if closing ) aka empty function body
            if (ptr->kind == NVC_TOK_OP && ptr->op_kind == NVC_OP_RPAREN) {
                goto end_fun_body;
            }

            // TODO: fun bodies DO NOT work

            //            // TODO: find closing )
            //
            //            // find closing ) which determines how many tokens
            //            will be passed to
            //            // the ast
            //            nvc_tok_t* end_ptr = ptr;
            //            while (end_ptr) {
            //                if (end_ptr->kind == NVC_TOK_OP &&
            //                    end_ptr->op_kind == NVC_OP_RPAREN)
            //                    break;
            //
            //                if (end_ptr->kind == NVC_TOK_SPECIAL &&
            //                    end_ptr->spec_kind == NVC_SPEC_TOK_EOF) {
            //                    fprintf(stderr,
            //                            "syntax error: malformed fun decl:
            //                            unclosed fun " "body.\n");
            //                    nvc_free_params(params, n_params);
            //                    goto error;
            //                }
            //
            //                ++end_ptr;
            //            }
            //
            //            fprintf(stdout, "scanned %ld tokens.\n", end_ptr -
            //            ptr);
            //
            //            // generate ast for fun body
            //            nvc_token_stream_t body_stream = {
            //                .tokens = ptr,
            //                .size = stream.size - (end_ptr - ptr),
            //            };
            //
            //            nvc_ast_t* body_ast = nvc_parse(&body_stream);
            //
            //            body_nodes = body_ast->nodes;
            //            body_len = body_ast->size;

        end_fun_body : {
            nvc_ast_node_t* fun_decl = calloc(1, sizeof(nvc_ast_node_t));
            if (!fun_decl) {
                fprintf(stderr, "Out of memory!\n");
                nvc_free_params(params, n_params);
                nvc_free_nodes(body_nodes, body_len);
                goto error;
            }
            fun_decl->kind = NVC_AST_NODE_FUN_DECL;
            fun_decl->fun_decl.fun_name = fun_name;
            fun_decl->fun_decl.return_type_name = return_type_name;
            fun_decl->fun_decl.params = params;
            fun_decl->fun_decl.n_params = n_params;
            fun_decl->fun_decl.body = body_nodes;
            fun_decl->fun_decl.body_len = body_len;

            // TODO: this is wrong TOO!
            *eaten = (ptr - stream.tokens) + 1;
            fprintf(stdout, "done parsing fun: ate %ld tokens.\n", *eaten);
            return fun_decl;
        }
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
            fprintf(stdout, "Dynamic allocation (nodes) %ld -> %ld.\n", n_nodes,
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

#ifdef __cplusplus
}
#endif