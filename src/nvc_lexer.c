#ifdef __cplusplus
extern "C" {
#endif

#include <nvc_lexer.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static inline bool nvc_is_number(char c) {
    // is number or decimal point
    return c == '.' || (c >= '0' && c <= '9');
}

static inline nvc_int nvc_parse_int(char* str, char** end) {
    // note: this assumes str is not NULL and is null terminated
    // note: I'm not sure how efficient this is maybe I could compile it with
    // -O4 and replace with inline asm
    nvc_int i = 0;
    while (*str >= '0' && *str <= '9') {
        i = i * 10 + (*str - '0');
        ++str;
    }
    *end = str;
    return i;
}

static inline nvc_fp nvc_parse_fp(char* str, char** end) {
    // adapted from stb for readability and simplicity and my specific
    // requirements see:
    // https://github.com/nothings/stb/blob/master/stb_c_lexer.h
    // note: this
    // assumes str is not NULL and is null terminated note: see nvc_parse_int
    // for optimization suggestion lhs of decimal point
    *end = str;
    nvc_fp fp = 0.0;
    while (*str >= '0' && *str <= '9') {
        fp = fp * 10 + (*str - '0');
        ++str;
    }

    // early return without making *end -> str aka discard the value
    if (*str != '.') return 0.0;

    // rhs of decimal point
    char* dp = str;
    ++str;
    nvc_fp pow = 1.0, dec = 0.0;
    for (; *str >= '0' && *str <= '9'; pow *= 10.0) {
        dec = dec * 10 + (*str - '0');
        ++str;
    }
    fp += dec / pow;

    if (*end == dp && dp + 1 == str) return 0.0;

    *end = str;

    return fp;
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
            //      should have about the same speed?
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
                if (tokens[i].str_lit) free(tokens[i].str_lit);
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
            //      guaranteed
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
        case NVC_TOK_STR_LIT: {
            if (token->str_lit) {
                // TODO: maybe use strnlen here although i think null terminate
                //      is guaranteed
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
        case NVC_TOK_FP_LIT: {
            // note: I'm sure there some way to find the maximum length of a
            // double but I don't know how to do that so I'm just gonna go with
            // some arbitrarily large number as a brief look at stackoverflow
            // reveals it is probably quite large
            uint32_t max_allocation_size = 512;
            char* dblstr = NULL;
            do {
                // note: calloc must be used here instead of malloc to act as
                // null terminators
                dblstr = calloc(max_allocation_size, sizeof(char));
                if (dblstr) break;
                // * (2/3)
                max_allocation_size = (max_allocation_size / 3) * 2;
                // note: have an at least reasonable min size
            } while (max_allocation_size > (5 + 1 + 18 + 1 + 1));
            // check out of memory
            if (!dblstr) {
                fprintf(stderr,
                        "Out of memory while allocating double %d "
                        "bytes.\n",
                        max_allocation_size);
                return NULL;
            }
            snprintf(dblstr, max_allocation_size, "fp(%.2Lf)", token->fp_lit);
            // make SURE it's null terminated
            dblstr[max_allocation_size - 1] = '\0';
            return dblstr;
        }
        case NVC_TOK_INT_LIT: {
            // note: carefully chosen value, last + 1 is for null terminator
            uint32_t max_allocation_size = 3 + 1 + 18 + 1 + 1;
            // note: calloc must be used here instead of malloc to act as null
            // terminators
            char* intstr = calloc(max_allocation_size, sizeof(char));
            if (!intstr) {
                fprintf(stderr, "Out of memory!\n");
                return NULL;
            }
            snprintf(intstr, max_allocation_size, "int(%ld)", token->int_lit);
            // make SURE it's null terminated
            intstr[max_allocation_size - 1] = '\0';
            return intstr;
        }
        case NVC_TOK_OP: {
            char* opstr = nvc_op_to_str(token->op_kind);
            // TODO: pretty sure opstr is guaranteed null terminated but maybe
            //      use strnlen just in case?
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
            case '\n': 
                ++line_num;
                // eat curr and go to next
                ++buf_curr;
                line_start_ptr = buf_curr;
                continue;
            case '#':
                // don't toggle commenting if # is inside string literal
                if (curr_flags & STRING_LITERAL_LF) break;
                // toggle commenting
                curr_flags ^= COMMENT_LF;
                // eat curr and go to next
                ++buf_curr;
                continue;
            case '\'':
                // don't toggle string literals if ' is inside comment
                if (curr_flags & COMMENT_LF) break;
                // enclosing '
                if (curr_flags & STRING_LITERAL_LF) {
                    // find starting ' note: must be on the same line if there
                    // is none, it is an error because we are in an illegal
                    // state
                    char* str_lit_begin = buf_curr;
                    while (--str_lit_begin && str_lit_begin >= buf &&
                           *str_lit_begin != '\'')
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
                    toks_curr->buf_loc.bufname = bufname;
                    toks_curr->buf_loc.l = line_num;
                    toks_curr->buf_loc.c = buf_curr - line_start_ptr;
                    toks_curr->buf_loc.line = line_start_ptr;
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
            toks_curr->buf_loc.bufname = bufname;
            toks_curr->buf_loc.l = line_num;
            toks_curr->buf_loc.c = buf_curr - line_start_ptr;
            toks_curr->buf_loc.line = line_start_ptr;
            toks_curr->symbol = symbolcpy;
            ++toks_curr;
            ++toks_size;
            continue;
        }

        // parsing numbers must be done before operators
        if (nvc_is_number(*buf_curr)) {
            // try parse floating point first
            char* end;
            nvc_fp fp = nvc_parse_fp(buf_curr, &end);
            // parsed decimal
            if (buf_curr != end) {
                toks_curr->kind = NVC_TOK_FP_LIT;
                toks_curr->buf_loc.bufname = bufname;
                toks_curr->buf_loc.l = line_num;
                toks_curr->buf_loc.c = buf_curr - line_start_ptr;
                toks_curr->buf_loc.line = line_start_ptr;
                toks_curr->fp_lit = fp;
                ++toks_curr;
                ++toks_size;
                // advance buf ptr
                buf_curr = end;
                continue;
            }
            // try parse int second
            nvc_int i = nvc_parse_int(buf_curr, &end);
            // parsed int
            if (buf_curr != end) {
                toks_curr->kind = NVC_TOK_INT_LIT;
                toks_curr->buf_loc.bufname = bufname;
                toks_curr->buf_loc.l = line_num;
                toks_curr->buf_loc.c = buf_curr - line_start_ptr;
                toks_curr->buf_loc.line = line_start_ptr;
                toks_curr->int_lit = i;
                ++toks_curr;
                ++toks_size;
                // advance buf ptr
                buf_curr = end;
                continue;
            }
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
            toks_curr->buf_loc.bufname = bufname;
            toks_curr->buf_loc.l = line_num;
            toks_curr->buf_loc.c = buf_curr - line_start_ptr;
            toks_curr->buf_loc.line = line_start_ptr;
            toks_curr->op_kind = op;
            ++toks_size;
            ++toks_curr;
            continue;
        }

        // advance pointer
        ++buf_curr;
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
