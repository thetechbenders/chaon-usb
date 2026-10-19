#ifndef CHAON_SCRIPT_H
#define CHAON_SCRIPT_H
#include <stddef.h>
#include <stdint.h>

enum { CHAON_MAX_SOURCE = 2048, CHAON_MAX_STEPS = 32, CHAON_MAX_TEXT = 80 };
typedef enum { CHAON_OP_TYPE, CHAON_OP_DELAY, CHAON_OP_ENTER, CHAON_OP_TAB } chaon_op_t;
typedef struct {
    chaon_op_t op;
    uint32_t delay_ms;
    char text[CHAON_MAX_TEXT + 1];
} chaon_instruction_t;
typedef struct {
    size_t count;
    chaon_instruction_t steps[CHAON_MAX_STEPS];
} chaon_program_t;
typedef enum {
    CHAON_PARSE_OK, CHAON_PARSE_INVALID_ARGUMENT, CHAON_PARSE_EMPTY,
    CHAON_PARSE_SYNTAX, CHAON_PARSE_BOUNDS
} chaon_parse_result_t;
typedef struct {
    chaon_parse_result_t code;
    size_t line;
} chaon_parse_error_t;

/** Compile bounded text source into a fixed-capacity program.
 * Source must be NUL terminated. On failure output is reset to empty.
 * This is an original experimental grammar, not DuckyScript.
 */
chaon_parse_result_t chaon_parse(const char *source, chaon_program_t *out,
                                chaon_parse_error_t *error);
#endif
