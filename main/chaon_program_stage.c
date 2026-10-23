/* SPDX-License-Identifier: AGPL-3.0-only */
#include "chaon_program_stage.h"

#include <string.h>

enum { CHAON_PROGRAM_STAGE_MAX_BYTES = 3200 };
_Static_assert(sizeof(chaon_program_t) <= 3072,
               "bounded program unexpectedly grew");
_Static_assert(sizeof(chaon_program_stage_t) <= CHAON_PROGRAM_STAGE_MAX_BYTES,
               "program stage unexpectedly grew");

static bool same_file(const chaon_fat_entry_t *a,
                      const chaon_fat_entry_t *b)
{
    return a && b && a->is_directory == b->is_directory &&
        a->first_cluster == b->first_cluster &&
        a->file_size == b->file_size &&
        memcmp(a->raw_name, b->raw_name, sizeof a->raw_name) == 0;
}

static bool valid_program(const chaon_program_t *program)
{
    if (!program || program->count == 0 ||
        program->count > CHAON_MAX_STEPS)
        return false;
    for (size_t i = 0; i < program->count; ++i) {
        const chaon_instruction_t *step = &program->steps[i];
        switch (step->op) {
            case CHAON_OP_TYPE: {
                bool terminated = false;
                for (size_t j = 0; j <= CHAON_MAX_TEXT; ++j) {
                    const unsigned char ch = (unsigned char)step->text[j];
                    if (ch == 0) {
                        terminated = true;
                        break;
                    }
                    if (ch < 32 || ch > 126) return false;
                }
                if (!terminated) return false;
                break;
            }
            case CHAON_OP_DELAY:
                if (step->delay_ms == 0 || step->delay_ms > 10000)
                    return false;
                break;
            case CHAON_OP_ENTER:
            case CHAON_OP_TAB:
                break;
            default:
                return false;
        }
    }
    return true;
}

void chaon_program_stage_clear(chaon_program_stage_t *stage)
{
    if (!stage) return;
    ++stage->generation;
    stage->state = CHAON_PROGRAM_STAGE_EMPTY;
    stage->queued = false;
    stage->index = 0;
    memset(&stage->identity, 0, sizeof stage->identity);
    memset(&stage->program, 0, sizeof stage->program);
}

bool chaon_program_stage_request(chaon_program_stage_t *stage,
    const chaon_fat_catalog_t *catalog, uint8_t index,
    const chaon_fat_entry_t *expected)
{
    if (!stage) return false;
    chaon_program_stage_clear(stage);
    if (!catalog || !expected ||
        catalog->count > CHAON_FAT_MAX_FILES || index >= catalog->count ||
        expected->is_directory ||
        !same_file(&catalog->entries[index], expected))
        return false;
    stage->state = CHAON_PROGRAM_STAGE_PENDING;
    stage->queued = true;
    stage->index = index;
    stage->identity = *expected;
    return true;
}

bool chaon_program_stage_take(chaon_program_stage_t *stage,
    uint32_t *generation, uint8_t *index, chaon_fat_entry_t *identity)
{
    if (!stage || !generation || !index || !identity ||
        stage->state != CHAON_PROGRAM_STAGE_PENDING || !stage->queued)
        return false;
    *generation = stage->generation;
    *index = stage->index;
    *identity = stage->identity;
    stage->queued = false;
    return true;
}

bool chaon_program_stage_publish(chaon_program_stage_t *stage,
    uint32_t generation, uint8_t index,
    const chaon_fat_entry_t *identity,
    const chaon_program_t *validated_program)
{
    if (!stage || stage->generation != generation ||
        stage->state != CHAON_PROGRAM_STAGE_PENDING || stage->queued ||
        stage->index != index || !same_file(&stage->identity, identity))
        return false;
    if (stage->identity.is_directory ||
        memcmp(stage->identity.raw_name + 8, "CHN", 3) != 0 ||
        !valid_program(validated_program)) {
        stage->state = CHAON_PROGRAM_STAGE_EMPTY;
        stage->index = 0;
        memset(&stage->identity, 0, sizeof stage->identity);
        memset(&stage->program, 0, sizeof stage->program);
        return false;
    }
    stage->program = *validated_program;
    stage->state = CHAON_PROGRAM_STAGE_READY;
    return true;
}

bool chaon_program_stage_copy(const chaon_program_stage_t *stage,
    uint8_t index, const chaon_fat_entry_t *identity,
    uint32_t *generation, chaon_program_t *out)
{
    if (generation) *generation = 0;
    if (!generation || !out) return false;
    memset(out, 0, sizeof *out);
    if (!stage || stage->state != CHAON_PROGRAM_STAGE_READY ||
        stage->index != index || !same_file(&stage->identity, identity))
        return false;
    *out = stage->program;
    *generation = stage->generation;
    return true;
}
