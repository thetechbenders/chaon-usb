/* SPDX-License-Identifier: AGPL-3.0-only */
#include "chaon_inspect.h"
#include <string.h>

/* Called exclusively by the low-priority SD task. Keep the ~2.9 KiB
 * temporary parsed program off the 4 KiB FreeRTOS task stack.
 * Its contents are never published; clear immediately after validation. */
static chaon_program_t scratch;

chaon_inspection_t chaon_inspect_bytes(const uint8_t *source, size_t length)
{
    chaon_inspection_t result = { .state = CHAON_INSPECT_BAD_SOURCE };
    if (!source || length == 0 || length >= CHAON_MAX_SOURCE)
        return result;
    if (source[length] != 0) return result;
    for (size_t i=0;i<length;++i) {
        const uint8_t b = source[i];
        if (b == 0 || b > 126 ||
            (b < 32 && b != '\n' && b != '\r' && b != '\t'))
            return result;
    }
    chaon_parse_error_t err = {0};
    const chaon_parse_result_t parsed =
        chaon_parse((const char *)source, &scratch, &err);
    if (parsed == CHAON_PARSE_OK) {
        result.state = CHAON_INSPECT_VALID;
        result.step_count = (uint8_t)scratch.count;
        switch (scratch.steps[0].op) {
            case CHAON_OP_TYPE: result.first_op = CHAON_INSPECT_OP_TYPE; break;
            case CHAON_OP_DELAY: result.first_op = CHAON_INSPECT_OP_DELAY; break;
            case CHAON_OP_ENTER: result.first_op = CHAON_INSPECT_OP_ENTER; break;
            case CHAON_OP_TAB: result.first_op = CHAON_INSPECT_OP_TAB; break;
            default: result.state = CHAON_INSPECT_BAD_SYNTAX; break;
        }
    } else {
        result.state = CHAON_INSPECT_BAD_SYNTAX;
        result.error_line = (uint16_t)(err.line > UINT16_MAX ?
                                       UINT16_MAX : err.line);
    }
    memset(&scratch, 0, sizeof scratch);
    return result;
}
