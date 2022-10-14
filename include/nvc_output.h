#ifdef __cplusplus
extern "C" {
#endif

#ifndef NVC_OUTPUT_H
#define NVC_OUTPUT_H

#include <stdint.h>

// note: this structure is purely used for reporting errors/warnings
typedef struct {
    char* bufname;  // this does NOT need to be freed, it is not owned
    char* line;     // this does NOT need to be freed, it is just a reference to
                    // the start of the error line
    uint32_t l, c;  // line number and char number in source of token (for
                    // errors/warnings)
} nvc_buffer_location_t;

void nvc_print_buffer_message(const char* msg, nvc_buffer_location_t loc);

#endif  // NVC_OUTPUT_H

#ifdef __cplusplus
}
#endif