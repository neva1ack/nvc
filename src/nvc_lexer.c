#ifdef __cplusplus
extern "C" {
#endif

#include <nvc_lexer.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline bool nvc_is_whitespace(char c) {
    // special whitespace conditions due to no semicolon parsing
    return c == '\t'    // horizontal tabs
           || c == ' '  // spaces
        ;
}

static inline bool nvc_is_word(char c) {
    // is alpha or underscore
    return (c >= 'A' && c <= 'Z')     // alpha upper
           || (c >= 'a' && c <= 'z')  // alpha lower
           || c == '_'                // underscore
        ;
}

static inline bool nvc_is_operator(char c) {
    // special reserved operator characters
    return (c > '&' && c < '0')     // ' ( ) * + , - . /
           || c == ':'              // :
           || (c > ';' && c < '?')  // < = >
           || c == '[' || c == ']' || c == '^' || c == '~';  // [ ] ^ ~

    //    return c == '<' || c == '=' || c == '>' || c == '+' || c == '-' ||
    //           c == '~' || c == '*' || c == '/' || c == ':' || c == '(' ||
    //           c == ')' || c == '[' || c == ']' || c == '^' || c == ',' || c
    //           == '.';
}

char* nvc_op_to_str(nvc_operator_kind_t op) {
    // TODO: this is probably a bad way of doing this
    switch (op) {
        case NVC_OP_UNKNOWN:
        default: return "unknown";
        case NVC_OP_EQ: return "=";
        case NVC_OP_RPAREN: return ")";
        case NVC_OP_TYPE_ANNOTATION: return ":";
        case NVC_OP_LPAREN: return "(";
        case NVC_OP_COMMA: return ",";
        case NVC_OP_RET_DECL: return "->";
        case NVC_OP_DOT: return ".";
        case NVC_OP_RBRACKET: return "]";
        case NVC_OP_LBRACKET: return "[";
        case NVC_OP_ADD_EQ: return "+=";
        case NVC_OP_SUB_EQ: return "-=";
        case NVC_OP_MUL_EQ: return "*=";
        case NVC_OP_DIV_EQ: return "/=";
        case NVC_OP_ADD: return "+";
        case NVC_OP_SUB: return "-";
        case NVC_OP_MUL: return "*";
        case NVC_OP_DIV: return "/";
        case NVC_OP_POW: return "^";
        case NVC_OP_GT: return ">";
        case NVC_OP_LT: return "<";
        case NVC_OP_GE: return ">=";
        case NVC_OP_LE: return "<=";
        case NVC_OP_NEG: return "~";
        case NVC_OP_POW_EQ: return "^=";
    }
}

static nvc_operator_kind_t nvc_str_to_op(char* str, size_t* operator_len) {
    do {
        // single char operator
        if (*operator_len == 1) {
            // TODO: could use array but this is more memory efficient and
            // should have about the same speed?
            switch (str[0]) {
                case '<': return NVC_OP_LT;
                case '>': return NVC_OP_GT;
                case '=': return NVC_OP_EQ;
                case '+': return NVC_OP_ADD;
                case '-': return NVC_OP_SUB;
                case '*': return NVC_OP_MUL;
                case '/': return NVC_OP_DIV;
                case '~': return NVC_OP_NEG;
                case '^': return NVC_OP_POW;
                case ':': return NVC_OP_TYPE_ANNOTATION;
                case '(': return NVC_OP_LPAREN;
                case ')': return NVC_OP_RPAREN;
                case '[': return NVC_OP_LBRACKET;
                case ']': return NVC_OP_RBRACKET;
                case ',': return NVC_OP_COMMA;
                case '.': return NVC_OP_DOT;
            }
        }
        // two char operator
        else {
            // TODO: probably a way more efficient way to compute this
            if (strncmp(str, "<=", 2) == 0) {
                return NVC_OP_LE;
            } else if (strncmp(str, ">=", 2) == 0) {
                return NVC_OP_GE;
            } else if (strncmp(str, "+=", 2) == 0) {
                return NVC_OP_ADD_EQ;
            } else if (strncmp(str, "-=", 2) == 0) {
                return NVC_OP_SUB_EQ;
            } else if (strncmp(str, "*=", 2) == 0) {
                return NVC_OP_MUL_EQ;
            } else if (strncmp(str, "/=", 2) == 0) {
                return NVC_OP_DIV_EQ;
            } else if (strncmp(str, "^=", 2) == 0) {
                return NVC_OP_POW_EQ;
            } else if (strncmp(str, "->", 2) == 0) {
                return NVC_OP_RET_DECL;
            }
        }

        --*operator_len;
    } while (*operator_len);

    return NVC_OP_UNKNOWN;
}

static void nvc_free_tokens(nvc_tok_t* tokens, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        switch (tokens[i].kind) {
            case NVC_TOK_SYMBOL: free(tokens[i].symbol); break;
            case NVC_TOK_STR_LIT:
                // free if not NULL note: this can actually be NULL in the case
                // of an empty literal
                if (tokens[i].str_lit)
                    free(tokens[i].str_lit);
                break;
            default: break;
        }
    }
    // free buffer
    free(tokens);
}

void nvc_free_token_stream(nvc_token_stream_t* stream) {
    if (stream) {
        nvc_free_tokens(stream->tokens, stream->size);
        free(stream);
    }
}

// note: return value of this MUST be freed
char* nvc_token_to_str(nvc_tok_t* token) {
    switch (token->kind) {
        case NVC_TOK_SYMBOL: {
            // note: symbol is guaranteed non NULL unlike strlit
            // TODO: maybe use strnlen here although i think null terminate is
            // guaranteed
            size_t len = 6 + 1 + strlen(token->symbol) + 1;
            // can use malloc here because we know the exact size
            char* symbolbuf = malloc(sizeof(char) * (len + 1));
            if (!symbolbuf) {
                fprintf(stderr, "Out of memory!\n");
                return NULL;
            }
            snprintf(symbolbuf, len + 1, "symbol(%s)", token->symbol);
            symbolbuf[len] = '\0';
            return symbolbuf;
        }
        case NVC_TOK_SPECIAL:
            // TODO: this whole branch seems shit
            switch (token->spec_kind) {
                case NVC_SPEC_TOK_EOF: {
                    const char* value = "eof";
                    // TODO: maybe use strndup although this should be totally
                    // safe
                    char* cpy = strdup(value);
                    if (!cpy) {
                        fprintf(stderr, "Out of memory!\n");
                        return NULL;
                    }
                    return cpy;
                }
                case NVC_SPEC_TOK_NEWLINE: {
                    const char* value = "newline";
                    // TODO: maybe use strndup although this should be totally
                    // safe
                    char* cpy = strdup(value);
                    if (!cpy) {
                        fprintf(stderr, "Out of memory!\n");
                        return NULL;
                    }
                    return cpy;
                }
            }
        case NVC_TOK_STR_LIT: {
            if (token->str_lit) {
                // TODO: maybe use strnlen here although i think null terminate
                // is guaranteed
                size_t len = 3 + 1 + strlen(token->str_lit) + 1;
                // can use malloc here because we know the exact size
                char* strstrbuf = malloc(sizeof(char) * (len + 1));
                if (!strstrbuf) {
                    fprintf(stderr, "Out of memory!\n");
                    return NULL;
                }
                snprintf(strstrbuf, len + 1, "str(%s)", token->str_lit);
                strstrbuf[len] = '\0';
                return strstrbuf;
            }
            // str lit is NULL
            else {
                // TODO: This seems shit (same as NVC_TOK_SPECIAL branch)
                // note: still allocate on heap to avoid confusion when freeing
                // the return value of this function
                const char* value = "str(NULL)";
                // TODO: maybe use strndup although this should be totally safe
                char* cpy = strdup(value);
                if (!cpy) {
                    fprintf(stderr, "Out of memory!\n");
                    return NULL;
                }
                return cpy;
            }
        }
        case NVC_TOK_FLOAT_LIT: {
            // note: I'm sure there some way to find the maximum length of a
            // double but I don't know how to do that so I'm just gonna go with
            // some arbitrarily large number as a brief look at stackoverflow
            // reveals it is probably quite large
            uint32_t max_allocation_size = 1024;
            char* dblstr = NULL;
            do {
                // note: calloc must be used here instead of malloc to act as
                // null terminators
                char* dblstr = calloc(max_allocation_size, sizeof(char));
                if (dblstr)
                    break;
                // * (2/3)
                max_allocation_size = (max_allocation_size / 3) * 2;
                // note: have an at least reasonable min size
            } while (max_allocation_size > (5 + 1 + 18 + 1 + 1));
            // check out of memory
            if (!dblstr) {
                fprintf(stderr,
                        "Out of memory while allocating double %d"
                        "bytes.\n",
                        max_allocation_size);
                return NULL;
            }
            snprintf(dblstr, max_allocation_size, "float(%.2f)",
                     token->float_lit);
            // make SURE it's null terminated
            dblstr[max_allocation_size - 1] = '\0';
            return dblstr;
        }
        case NVC_TOK_INT_LIT: {
            // note: carefully chosen value, last + 1 is for null terminator
            uint32_t max_allocation_size = 3 + 1 + 19 + 1 + 1;
            // note: calloc must be used here instead of malloc to act as null
            // terminators
            char* intstr = calloc(max_allocation_size, sizeof(char));
            if (!intstr) {
                fprintf(stderr, "Out of memory!\n");
                return NULL;
            }
            snprintf(intstr, max_allocation_size, "int(%ld)",
                     token->int_lit);
            // make SURE it's null terminated
            intstr[max_allocation_size - 1] = '\0';
            return intstr;
        }
        case NVC_TOK_OP: {
            char* opstr = nvc_op_to_str(token->op_kind);
            // TODO: pretty sure opstr is guaranteed null terminated but maybe
            // use strnlen just in case?
            size_t len = 2 + 1 + strlen(opstr) + 1;
            // can use malloc here because exact size is known
            char* opstrcpy = malloc(sizeof(char) * (len + 1));
            if (!opstrcpy) {
                fprintf(stderr, "Out of memory!\n");
                return NULL;
            }
            snprintf(opstrcpy, len + 1, "op(%s)", opstr);
            opstrcpy[len] = '\0';
            return opstrcpy;
        }
        case NVC_TOK_UNKNOWN:
        default:
            return NULL;
    }

    // unreachable code
    return NULL;
}

nvc_token_stream_t* nvc_lexical_analysis(char* bufname, char* buf, long bufsz) {
    // maximum amount of tokens, resize later
    nvc_tok_t* toks = calloc(bufsz, sizeof(nvc_tok_t));
    // out of memory
    if (!toks) {
        fprintf(stderr, "Out of memory!\n");
        return NULL;
    }

    char* line_start_ptr = buf;
    char* buf_curr = buf;
    nvc_tok_t* toks_curr = toks;
    size_t toks_size = 0;

    const uint32_t COMMENT_LF = 1 << 1;
    const uint32_t STRING_LITERAL_LF = 1 << 2;
    uint32_t curr_flags = 0;

    // note: here line is 0-indexed but will be 1-indexed when pretty
    // printed
    uint32_t line_num = 0;

    while (*buf_curr != '\0') {
        // handle special chars
        switch (*buf_curr) {
            case '\n':  // TODO: may use an is newline function in future here?
                // cant end line while inside string literal
                if (curr_flags & STRING_LITERAL_LF) {
                    // TODO: syntax error
                    fprintf(stderr,
                            "syntax error: multiline string literals not "
                            "supported.");
                    nvc_free_tokens(toks, toks_size);
                    return NULL;
                }
                // automatic newline folding at beginning of file and repeated
                // newlines
                if (toks_size > 0 && toks_curr[-1].kind != NVC_TOK_SPECIAL) {
                    // insert newline token and advance pointer
                    toks_curr->kind = NVC_TOK_SPECIAL;
                    toks_curr->bufname = bufname;
                    toks_curr->l = line_num;
                    toks_curr->c = buf_curr - line_start_ptr;
                    toks_curr->line = line_start_ptr;
                    toks_curr->spec_kind = NVC_SPEC_TOK_NEWLINE;
                    ++toks_curr;
                    ++toks_size;
                    ++line_num;
                }
                // eat curr and go to next
                ++buf_curr;
                line_start_ptr = buf_curr;
                continue;
            case '#':
                // don't toggle commenting if # is inside string literal
                if (curr_flags & STRING_LITERAL_LF)
                    break;
                // toggle commenting
                curr_flags ^= COMMENT_LF;
                // eat curr and go to next
                ++buf_curr;
                continue;
            case '\'':
                // don't toggle string literals if ' is inside comment
                if (curr_flags & COMMENT_LF)
                    break;
                // enclosing '
                if (curr_flags & STRING_LITERAL_LF) {
                    // find starting ' note: must be on the same line if there
                    // is none, it is an error because we are in an illegal
                    // state
                    char* str_lit_begin = buf_curr;
                    while (--str_lit_begin && str_lit_begin >= buf &&
                           // TODO: may use some is newline function here in
                           // future?
                           *str_lit_begin != '\n' && *str_lit_begin != '\'')
                        ;
                    // unable to find starting '
                    if (*str_lit_begin != '\'') {
                        // TODO: report error
                        fprintf(stderr, "syntax error: cant find previous '.");
                        nvc_free_tokens(toks, toks_size);
                        return NULL;
                    }
                    // advance + 1 so the starting ' is not included in the
                    // string literal
                    ++str_lit_begin;
                    // get string lit length
                    size_t strlitlen = buf_curr - str_lit_begin;
                    // non-empty string lit
                    if (strlitlen != 0) {
                        // allocate space
                        char* strlitcpy =
                            malloc((strlitlen + 1) * sizeof(char));
                        if (!strlitcpy) {
                            fprintf(stderr, "Out of memory!\n");
                            nvc_free_tokens(toks, toks_size);
                            return NULL;
                        }
                        // copy string from buffer
                        strncpy(strlitcpy, str_lit_begin, strlitlen);
                        // insert null term
                        strlitcpy[strlitlen] = '\0';
                        toks_curr->str_lit = strlitcpy;
                    }
                    // empty string lit
                    else {
                        toks_curr->str_lit = NULL;
                    }
                    // insert token and advance pointer
                    toks_curr->kind = NVC_TOK_STR_LIT;
                    toks_curr->bufname = bufname;
                    toks_curr->l = line_num;
                    toks_curr->c = buf_curr - line_start_ptr;
                    toks_curr->line = line_start_ptr;
                    ++toks_curr;
                    ++toks_size;
                }
                // toggle string literal
                curr_flags ^= STRING_LITERAL_LF;
                // eat curr and go to next
                ++buf_curr;
                continue;
        }

        // while inside string literal or comment eat curr and
        // advance pointer
        if (curr_flags & STRING_LITERAL_LF || curr_flags & COMMENT_LF) {
            ++buf_curr;
            continue;
        }

        // eat word characters and parse symbols
        char* buf_eat_start = buf_curr;
        while (nvc_is_word(*buf_curr)) {
            ++buf_curr;
        }
        // a symbol was eaten
        if (buf_eat_start != buf_curr) {
            // calculate str len
            size_t symbol_len = buf_curr - buf_eat_start;
            if (symbol_len < 1) {
                // TODO: error reporting
                fprintf(stderr,
                        "illegal symbol: something went wrong while parsing "
                        "symbol.\n");
                nvc_free_tokens(toks, toks_size);
                return NULL;
            }
            // allocate space note: this must be freed later
            char* symbolcpy = malloc((symbol_len + 1) * sizeof(char));
            if (!symbolcpy) {
                fprintf(stderr, "Out of memory!\n");
                nvc_free_tokens(toks, toks_size);
                return NULL;
            }
            // copy symbol
            strncpy(symbolcpy, buf_eat_start, symbol_len);
            // null terminate
            symbolcpy[symbol_len] = '\0';
            // insert symbol token and advance pointer
            toks_curr->kind = NVC_TOK_SYMBOL;
            toks_curr->bufname = bufname;
            toks_curr->l = line_num;
            toks_curr->c = buf_curr - line_start_ptr;
            toks_curr->line = line_start_ptr;
            toks_curr->symbol = symbolcpy;
            ++toks_curr;
            ++toks_size;
            continue;
        }

        // eat operator chars and parse operators
        while (nvc_is_operator(*buf_curr)) {
            ++buf_curr;
        }
        // an operator was eaten
        if (buf_eat_start != buf_curr) {
            // calculate operator len
            size_t operator_len = buf_curr - buf_eat_start;
            // something went wrong here
            if (operator_len < 1) {
                // TODO: error reporting
                fprintf(stderr,
                        "illegal operator: something went wrong while parsing "
                        "operators.\n");
                nvc_free_tokens(toks, toks_size);
                return NULL;
            }
            // chained operator parsing
            else if (operator_len > 2) {
                operator_len = 2;
            }

            // parse operator
            nvc_operator_kind_t op =
                nvc_str_to_op(buf_eat_start, &operator_len);
            // fix buf pos
            buf_curr = buf_eat_start + operator_len;

            if (op == NVC_OP_UNKNOWN) {
                // TODO: error reporting
                char* errorstr = malloc((operator_len + 1) * sizeof(char));
                if (!errorstr) {
                    fprintf(stderr, "Out of memory!\n");
                    nvc_free_tokens(toks, toks_size);
                    return NULL;
                }
                strncpy(errorstr, buf_eat_start, operator_len);
                errorstr[operator_len] = '\0';
                fprintf(stderr, "unknown operator: '%s'.\n", errorstr);
                free(errorstr);
                // don't bother setting this to NULL as it immediately goes out
                // of scope
                //                errorstr = NULL;
                nvc_free_tokens(toks, toks_size);
                return NULL;
            }

            // insert operator token and advance pointer
            toks_curr->kind = NVC_TOK_OP;
            toks_curr->bufname = bufname;
            toks_curr->l = line_num;
            toks_curr->c = buf_curr - line_start_ptr;
            toks_curr->line = line_start_ptr;
            toks_curr->op_kind = op;
            ++toks_size;
            ++toks_curr;
            continue;
        }

        // parse number literals
        char* endptr = NULL;
        double dlit = strtod(buf_curr, &endptr);
        if (endptr && endptr != buf_curr) {
            // search for decimal point
            void* srch = memchr(buf_curr, '.', endptr - buf_curr);
            // could not find a decimal point
            if (!srch) {
                // parse as int
                endptr = NULL;
                int64_t ilit = strtoll(buf_curr, &endptr, 10);
                // parsed something (is int literal)
                if (endptr && endptr != buf_curr) {
                    // TODO: check for integer overflow
                    // insert token and advance token pointer
                    toks_curr->kind = NVC_TOK_INT_LIT;
                    toks_curr->bufname = bufname;
                    toks_curr->l = line_num;
                    toks_curr->c = buf_curr - line_start_ptr;
                    toks_curr->line = line_start_ptr;
                    toks_curr->int_lit = ilit;
                    ++toks_size;
                    ++toks_curr;
                    // advance buf pointer
                    buf_curr = endptr;
                    continue;
                }
            }
            // if could not be parsed as int or contains a decimal
            // point the double value will be used

            // TODO: check for double losing precision

            // insert toke and advance token pointer
            toks_curr->kind = NVC_TOK_FLOAT_LIT;
            toks_curr->bufname = bufname;
            toks_curr->l = line_num;
            toks_curr->c = buf_curr - line_start_ptr;
            toks_curr->line = line_start_ptr;
            toks_curr->float_lit = dlit;
            ++toks_size;
            ++toks_curr;
            // advance buf pointer
            buf_curr = endptr;
            continue;
        }

        // advance pointer
        ++buf_curr;
    }

    // insert eof token, note: does NOT advance pointer
    // note: eof acts as newline so can substitute last newline
    if (toks_size > 0 && toks_curr[-1].kind == NVC_TOK_SPECIAL) {
        toks_curr[-1].kind = NVC_TOK_SPECIAL;
        toks_curr[-1].bufname = bufname;
        toks_curr[-1].l = line_num;
        toks_curr[-1].c = buf_curr - line_start_ptr;
        toks_curr[-1].line = line_start_ptr;
        toks_curr[-1].spec_kind = NVC_SPEC_TOK_EOF;
    } else {
        toks_curr->kind = NVC_TOK_SPECIAL;
        toks_curr->bufname = bufname;
        toks_curr->l = line_num;
        toks_curr->c = buf_curr - line_start_ptr;
        toks_curr->line = line_start_ptr;
        toks_curr->spec_kind = NVC_SPEC_TOK_EOF;
        ++toks_size;
    }

    // resize toks to save memory
    toks = realloc(toks, sizeof(nvc_tok_t) * toks_size);

    nvc_token_stream_t* token_stream = calloc(1, sizeof(nvc_token_stream_t));

    if (!token_stream) {
        fprintf(stderr, "Out of memory!\n");
        nvc_free_tokens(toks, toks_size);
        return NULL;
    }

    token_stream->tokens = toks;
    token_stream->size = toks_size;

    return token_stream;
}

#ifdef __cplusplus
}
#endif