#include <nvc_compiler.h>

#include <nvc_ast.h>

static char* nvc_read_file(FILE* fp, long* fs) {
    // TODO: this method of reading a file might be inefficient/unsafe
    if (!fp)
        goto error;
    if (fseek(fp, 0, SEEK_END) != 0)
        goto error;
    *fs = ftell(fp);
    if (*fs == -1)
        goto error;
    if (fseek(fp, 0, SEEK_SET) != 0)
        goto error;
    // allocate filesize + null terminate
    char* buf = malloc(*fs + 1);
    if (!buf)
        // out of memory
        goto error;
    if (fread(buf, *fs, 1, fp) != 1)
        goto error;
    // null terminate
    buf[*fs] = '\0';
    return buf;
error:
    return NULL;
}

static char* nvc_open_and_read_file(char* filename, long* filesize) {
    if (!filename)
        return NULL;
    // open and read file to buffer
    FILE* fp = fopen(filename, "r");
    long fs;
    char* buf = nvc_read_file(fp, &fs);
    if (!buf) {
        fclose(fp);
        return NULL;
    }
    fclose(fp);
    if (filesize)
        *filesize = fs;
    return buf;
}

int nvc_compile(char* filename) {
    // open and read file
    long bufsz;
    char* buf = nvc_open_and_read_file(filename, &bufsz);
    if (!buf) {
        fprintf(stderr, "Unable to read file: %s.\n", filename);
        return 1;
    }

    // TODO: remove me
    // debug print buf
    fputs("----- Source -----\n", stdout);
    fprintf(stdout, "%s\n", buf);

    nvc_token_stream_t* stream = nvc_lexical_analysis(buf, bufsz);

    if (!stream) {
        return 1;
    }

    // TODO: remove me
    // debug print tokens
    fputs("----- Tokens -----\n", stdout);
    for (size_t i = 0; i < stream->size; ++i) {
        switch (stream->tokens[i].kind) {
            case NVC_TOK_SYMBOL:
                fprintf(stdout, "symbol(%s)", stream->tokens[i].symbol);
                break;
            case NVC_TOK_EOF: fputs("eof", stdout); break;
            case NVC_TOK_NEWLINE: fputs("newline", stdout); break;
            case NVC_TOK_STR_LIT:
                fprintf(stdout, "str_lit(%s)", stream->tokens[i].str_lit);
                break;
            case NVC_TOK_FLOAT_LIT:
                fprintf(stdout, "float_lit(%.2f)", stream->tokens[i].float_lit);
                break;
            case NVC_TOK_INT_LIT:
                fprintf(stdout, "int_lit(%ld)", stream->tokens[i].int_lit);
                break;
            case NVC_TOK_OP:
                fprintf(stdout, "op(%d)", stream->tokens[i].op_kind);
                break;
            case NVC_TOK_UNKNOWN:
            default: fputs("unknown", stdout); break;
        }
        fputc(' ', stdout);
    }
    fputc('\n', stdout);

    // operate on token stream here

    nvc_ast_t* ast = nvc_parse(stream);

    if (!ast) {
        nvc_free_token_stream(stream);
        return 1;
    }

    // TODO: remove me
    // debug print ast
    fputs("----- AST -----\n", stdout);
    nvc_print_ast(ast);

    // operate on ast here

    nvc_free_ast(ast);
    nvc_free_token_stream(stream);

    return 0;
}
