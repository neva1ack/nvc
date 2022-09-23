#ifdef __cplusplus
extern "C" {
#endif

#include <nvc_compiler.h>

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Invalid syntax. %s <filename>", argv[0]);
        return 1;
    }

    return nvc_compile(argv[1]);
}

#ifdef __cplusplus
}
#endif