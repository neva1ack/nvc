#ifdef __cplusplus
extern "C" {
#endif

#include <nvc_output.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void nvc_print_unexpected_token(nvc_tok_t* token) {
    // print message with token
    char* tokstr = nvc_token_to_str(token);
    if (!tokstr) {
        return;
    }
    fprintf(stderr, "syntax error: unexpected token: %s at: %s:%d:%d.\n",
            tokstr, token->bufname, token->l + 1, token->c);
    free(tokstr);

    char* line_end_ptr = token->line;
    // TODO: is newline function here in the future
    while (++line_end_ptr && *line_end_ptr != '\0' && *line_end_ptr != '\n')
        ;
    if (!line_end_ptr || (*line_end_ptr != '\0' && *line_end_ptr != '\n')) {
        fprintf(stderr, "error: unable to find end of line.\n");
        return;
    }
    size_t line_len = line_end_ptr - token->line;
    char* linecpy = malloc(sizeof(char) * (line_len + 1));
    if (!linecpy) {
        fprintf(stderr, "Out of memory!\n");
        return;
    }
    // copy line
    strncpy(linecpy, token->line, line_len);
    linecpy[line_len] = '\0';
    fputs(linecpy, stderr);
    fputc('\n', stderr);
    // print here
    // note len = strlen("^--- here")
    // print on left
    uint32_t here_msg_len = 1 + 3 + 1 + 4;
    if (token->c >= here_msg_len) {
        fprintf(stderr, "%*chere ---^\n", token->c - here_msg_len, ' ');
    }
    // print on right
    else {
        fprintf(stderr, "%*c^--- here\n", token->c - 1, ' ');
    }
}

#ifdef __cplusplus
}
#endif