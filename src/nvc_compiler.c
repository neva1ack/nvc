#ifdef __cplusplus
extern "C" {
#endif

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
    char* buf = malloc((*fs + 1) * sizeof(char));
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
        goto error;
    // open and read file to buffer
    FILE* fp = fopen(filename, "r");
    long fs;
    char* buf = nvc_read_file(fp, &fs);
    if (!buf) {
        fclose(fp);
        goto error;
    }
    fclose(fp);
    if (filesize)
        *filesize = fs;
    return buf;
error:
    if (filesize)
        *filesize = 0;
    return NULL;
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

    // TODO: remove me
    fputs("----- Tokens -----\n", stdout);

    nvc_token_stream_t* stream = nvc_lexical_analysis(filename, buf, bufsz);

    if (!stream) {
        free(buf);
        // don't bother nullify pointer as it goes out of scope here
        return 1;
    }

    // TODO: remove me
    // debug print tokens
    for (size_t i = 0; i < stream->size; ++i) {
        char* tokstr = nvc_token_to_str(stream->tokens + i);
        if (!tokstr) {
            nvc_free_token_stream(stream);
            free(buf);
            return 1;
        }
        fprintf(stdout, "%s ", tokstr);
        free(tokstr);
    }
    fputc('\n', stdout);

    // operate on token stream here

    // TODO: remove me
    fputs("------- AST ------\n", stdout);

    nvc_ast_t* ast = nvc_parse(stream);

    if (!ast) {
        nvc_free_token_stream(stream);
        // don't bother nullify stream pointer as it goes out of scope here
        free(buf);
        // don't bother nullify pointer as it goes out of scope here
        return 1;
    }

    // TODO: remove me
    // debug print ast
    nvc_print_ast(ast);

    // operate on ast here

    nvc_free_ast(ast);
    nvc_free_token_stream(stream);
    // don't bother nullify stream pointer as it goes out of scope here
    free(buf);
    // don't bother nullify pointer as it goes out of scope here

    return 0;
}

#ifdef __cplusplus
}
#endif