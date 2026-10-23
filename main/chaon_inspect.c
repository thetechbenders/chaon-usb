/* SPDX-License-Identifier: AGPL-3.0-only */
#include "chaon_inspect.h"
#include <string.h>

/* Summary-only compatibility storage. The firmware SD task uses its own
 * explicit workspace and candidate program instead of these objects. */
static chaon_program_t summary_scratch;
static chaon_inspect_workspace_t summary_workspace;

chaon_inspection_t chaon_inspect_compile_bytes(const uint8_t *source,
    size_t length, chaon_inspect_workspace_t *workspace,
    chaon_program_t *out)
{
    chaon_inspection_t result = { .state = CHAON_INSPECT_BAD_SOURCE };
    if (out) memset(out, 0, sizeof *out);
    if (workspace) memset(workspace, 0, sizeof *workspace);
    if (!source || !workspace || !out || length == 0 ||
        length >= CHAON_MAX_SOURCE)
        return result;
    for (size_t i=0;i<length;++i) {
        const uint8_t b = source[i];
        if (b == 0 || b > 126 ||
            (b < 32 && b != '\n' && b != '\r' && b != '\t'))
            return result;
    }
    memcpy(workspace->source, source, length);
    workspace->source[length] = '\0';
    chaon_parse_error_t err = {0};
    const chaon_parse_result_t parsed =
        chaon_parse(workspace->source, out, &err);
    if (parsed == CHAON_PARSE_OK) {
        result.state = CHAON_INSPECT_VALID;
        result.step_count = (uint8_t)out->count;
        switch (out->steps[0].op) {
            case CHAON_OP_TYPE: result.first_op = CHAON_INSPECT_OP_TYPE; break;
            case CHAON_OP_DELAY: result.first_op = CHAON_INSPECT_OP_DELAY; break;
            case CHAON_OP_ENTER: result.first_op = CHAON_INSPECT_OP_ENTER; break;
            case CHAON_OP_TAB: result.first_op = CHAON_INSPECT_OP_TAB; break;
            default:
                result.state = CHAON_INSPECT_BAD_SYNTAX;
                memset(out, 0, sizeof *out);
                break;
        }
    } else {
        result.state = CHAON_INSPECT_BAD_SYNTAX;
        result.error_line = (uint16_t)(err.line > UINT16_MAX ?
                                       UINT16_MAX : err.line);
    }
    memset(workspace, 0, sizeof *workspace);
    return result;
}

chaon_inspection_t chaon_inspect_bytes(const uint8_t *source, size_t length)
{
    const chaon_inspection_t result = chaon_inspect_compile_bytes(source,
        length, &summary_workspace, &summary_scratch);
    memset(&summary_scratch, 0, sizeof summary_scratch);
    return result;
}
