#include "chaon_vm.h"
#include "chaon_hid.h"
#include <stddef.h>

enum { KEY_PRESS_MS = 24, KEY_RELEASE_MS = 16, SLEEP_QUANTUM_MS = 10 };

static bool is_cancelled(const chaon_transport_t *io)
{
    return io->cancelled && io->cancelled(io->context);
}

static bool wait_cancelable(uint32_t ms, const chaon_transport_t *io)
{
    while (ms) {
        if (is_cancelled(io)) return false;
        const uint32_t slice = ms > SLEEP_QUANTUM_MS ? SLEEP_QUANTUM_MS : ms;
        io->sleep_ms(slice, io->context);
        ms -= slice;
    }
    return !is_cancelled(io);
}

static chaon_run_result_t tap(uint8_t modifier, uint8_t usage,
                             const chaon_transport_t *io)
{
    if (is_cancelled(io)) return CHAON_RUN_CANCELLED;
    if (!io->report(modifier, usage, io->context)) return CHAON_RUN_IO_ERROR;
    if (!wait_cancelable(KEY_PRESS_MS, io)) return CHAON_RUN_CANCELLED;
    if (!io->report(0, 0, io->context)) return CHAON_RUN_IO_ERROR;
    if (!wait_cancelable(KEY_RELEASE_MS, io)) return CHAON_RUN_CANCELLED;
    return CHAON_RUN_OK;
}

chaon_run_result_t chaon_run(const chaon_program_t *program,
                            const chaon_transport_t *io)
{
    if (!program || !io || !io->report || !io->sleep_ms ||
        program->count == 0 || program->count > CHAON_MAX_STEPS) {
        return CHAON_RUN_INVALID;
    }
    chaon_run_result_t result = CHAON_RUN_OK;
    for (size_t i = 0; i < program->count; ++i) {
        if (is_cancelled(io)) { result = CHAON_RUN_CANCELLED; break; }
        const chaon_instruction_t *step = &program->steps[i];
        switch (step->op) {
            case CHAON_OP_DELAY:
                if (step->delay_ms == 0 || step->delay_ms > 10000) {
                    result = CHAON_RUN_INVALID;
                } else if (!wait_cancelable(step->delay_ms, io)) {
                    result = CHAON_RUN_CANCELLED;
                }
                break;
            case CHAON_OP_ENTER:
            case CHAON_OP_TAB:
                result = tap(0, step->op == CHAON_OP_ENTER ?
                             CHAON_KEY_ENTER : CHAON_KEY_TAB, io);
                break;
            case CHAON_OP_TYPE: {
                bool terminated = false;
                for (size_t j = 0; j <= CHAON_MAX_TEXT; ++j) {
                    if (step->text[j] == '\0') { terminated = true; break; }
                    if (j == CHAON_MAX_TEXT) { result = CHAON_RUN_INVALID; break; }
                    uint8_t modifier = 0, usage = 0;
                    if (!chaon_hid_us_ascii(step->text[j], &modifier, &usage)) {
                        result = CHAON_RUN_INVALID;
                        break;
                    }
                    result = tap(modifier, usage, io);
                    if (result != CHAON_RUN_OK) break;
                }
                if (!terminated && result == CHAON_RUN_OK) result = CHAON_RUN_INVALID;
                break;
            }
            default:
                result = CHAON_RUN_INVALID;
                break;
        }
        if (result != CHAON_RUN_OK) break;
    }
    if (!io->report(0, 0, io->context)) return CHAON_RUN_IO_ERROR;
    return result;
}
