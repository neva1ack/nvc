#ifdef __cplusplus
extern "C" {
#endif

#include <nvc_output.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void nvc_print_buffer_message(const char* msg, nvc_buffer_location_t loc) {
    // print location
    fprintf(stderr, "%s: at: %s:%d:%d.\n", msg, loc.bufname, loc.l + 1, loc.c);

    char* line_end_ptr = loc.line;
    // TODO: is newline function here in the future
    while (++line_end_ptr && *line_end_ptr != '\0' && *line_end_ptr != '\n')
        ;
    if (!line_end_ptr || (*line_end_ptr != '\0' && *line_end_ptr != '\n')) {
        fprintf(stderr, "error: unable to find end of line.\n");
        return;
    }
    size_t line_len = line_end_ptr - loc.line;
    char* linecpy = malloc(sizeof(char) * (line_len + 1));
    if (!linecpy) {
        fprintf(stderr, "Out of memory!\n");
        return;
    }
    // copy line
    strncpy(linecpy, loc.line, line_len);
    linecpy[line_len] = '\0';
    fputs(linecpy, stderr);
    fputc('\n', stderr);
    // print here
    // note len = strlen("^--- here")
    // print on left
    uint32_t here_msg_len = 1 + 3 + 1 + 4;
    if (loc.c >= here_msg_len) {
        fprintf(stderr, "%*chere ---^\n", loc.c - here_msg_len, ' ');
    }
    // print on right
    else {
        fprintf(stderr, "%*c^--- here\n", loc.c - 1, ' ');
    }
}

#ifdef __cplusplus
}
#endif