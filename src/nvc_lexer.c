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
    // TODO: optimize this with ascii table
    // special reserved operator characters
    return c == '<' || c == '=' || c == '>' || c == '+' || c == '-' ||
           c == '~' || c == '*' || c == '/' || c == ':' || c == '(' ||
           c == ')' || c == '[' || c == ']' || c == '^' || c == ',' || c == '.';
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
            case NVC_TOK_SYMBOL:
                // is never NULL
                free(tokens[i].symbol);
                break;
            case NVC_TOK_STR_LIT:
                // free if not NULL
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

nvc_token_stream_t* nvc_lexical_analysis(char* buf, long bufsz) {
    // maximum amount of tokens, resize later
    nvc_tok_t* toks = calloc(bufsz, sizeof(nvc_tok_t));
    // out of memory
    if (!toks) {
        fprintf(stderr, "Out of memory!\n");
        free(buf);
        return NULL;
    }

    char* buf_curr = buf;
    nvc_tok_t* toks_curr = toks;
    size_t toks_size = 0;

    const uint32_t COMMENT_LF = 1 << 1;
    const uint32_t STRING_LITERAL_LF = 1 << 2;
    uint32_t curr_flags = 0;

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
                    free(buf);
                    nvc_free_tokens(toks, toks_size);
                    return NULL;
                }
                // automatic newline folding at beginning of file and repeated
                // newlines
                if (toks_size > 0 && toks_curr[-1].kind != NVC_TOK_NEWLINE) {
                    // insert newline token and advance pointer
                    toks_curr->kind = NVC_TOK_NEWLINE;
                    ++toks_curr;
                    ++toks_size;
                }
                // eat curr and go to next
                ++buf_curr;
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
                        fprintf(stderr, "illegal state: cant find previous '.");
                        free(buf);
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
                        char* strlitcpy = calloc(strlitlen + 1, sizeof(char));
                        if (!strlitcpy) {
                            fprintf(stderr, "Out of memory!\n");
                            free(buf);
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
                free(buf);
                nvc_free_tokens(toks, toks_size);
                return NULL;
            }
            // allocate space note: this must be freed later
            char* symbolcpy = calloc(symbol_len + 1, sizeof(char));
            if (!symbolcpy) {
                fprintf(stderr, "Out of memory!\n");
                free(buf);
                nvc_free_tokens(toks, toks_size);
                return NULL;
            }
            // copy symbol
            strncpy(symbolcpy, buf_eat_start, symbol_len);
            // null terminate
            symbolcpy[symbol_len] = '\0';
            // insert symbol token and advance pointer
            toks_curr->kind = NVC_TOK_SYMBOL;
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
                free(buf);
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
                char* errorstr = calloc(operator_len + 1, sizeof(char));
                if (!errorstr) {
                    fprintf(stderr, "Out of memory!\n");
                    free(buf);
                    nvc_free_tokens(toks, toks_size);
                    return NULL;
                }
                strncpy(errorstr, buf_eat_start, operator_len);
                errorstr[operator_len] = '\0';
                fprintf(stderr, "unknown operator: '%s'.\n", errorstr);
                free(errorstr);
                free(buf);
                nvc_free_tokens(toks, toks_size);
                return NULL;
            }

            // insert operator token and advance pointer
            toks_curr->kind = NVC_TOK_OP;
            toks_curr->op_kind = op;
            ++toks_size;
            ++toks_curr;
            continue;
        }

        // parse number literals
        char* endptr;
        double dlit = strtod(buf_curr, &endptr);
        if (endptr != buf_curr) {
            // search for decimal point
            void* srch = memchr(buf_curr, '.', endptr - buf_curr);
            // could not find a decimal point
            if (!srch) {
                // parse as int
                endptr = NULL;
                int64_t ilit = strtoll(buf_curr, &endptr, 10);
                // parsed something (is int literal)
                if (endptr != buf_curr) {
                    // TODO: check for integer overflow
                    // insert token and advance token pointer
                    toks_curr->kind = NVC_TOK_INT_LIT;
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
    if (toks_size > 0 && toks_curr[-1].kind == NVC_TOK_NEWLINE) {
        toks_curr[-1].kind = NVC_TOK_EOF;
    } else {
        toks_curr->kind = NVC_TOK_EOF;
        ++toks_size;
    }

    // done reading buffer
    free(buf);

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
