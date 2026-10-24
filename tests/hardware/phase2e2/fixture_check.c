/* SPDX-License-Identifier: AGPL-3.0-only */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "chaon_inspect.h"

typedef struct {
    const char *name;
    chaon_inspect_state_t state;
    uint8_t steps;
    uint16_t line;
} fixture_case_t;

static const fixture_case_t cases[] = {
    { "BENIGN.CHN", CHAON_INSPECT_VALID, 5, 0 },
    { "SLOWRUN.CHN", CHAON_INSPECT_VALID, 5, 0 },
    { "KEYREL.CHN", CHAON_INSPECT_VALID, 5, 0 },
    { "FRESHA.CHN", CHAON_INSPECT_VALID, 3, 0 },
    { "FRESHB.CHN", CHAON_INSPECT_VALID, 3, 0 },
    { "BADGRAM.CHN", CHAON_INSPECT_BAD_SYNTAX, 0, 2 },
    { "EMPTY.CHN", CHAON_INSPECT_BAD_SOURCE, 0, 0 },
    { "OVER2048.CHN", CHAON_INSPECT_BAD_SOURCE, 0, 0 },
    /* Parser-valid bytes; the FAT selection boundary rejects the extension. */
    { "CONTROL.TXT", CHAON_INSPECT_VALID, 3, 0 },
};

int main(int argc, char **argv)
{
    assert(argc == 2);
    uint8_t source[2048];
    chaon_inspect_workspace_t workspace;
    chaon_program_t program;

    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; ++i) {
        char path[1024];
        const int written = snprintf(path, sizeof path, "%s/%s",
                                     argv[1], cases[i].name);
        assert(written > 0 && (size_t)written < sizeof path);
        FILE *file = fopen(path, "rb");
        assert(file != NULL);
        const size_t length = fread(source, 1, sizeof source, file);
        assert(ferror(file) == 0);
        assert(fgetc(file) == EOF);
        fclose(file);

        const chaon_inspection_t result = chaon_inspect_compile_bytes(
            source, length, &workspace, &program);
        assert(result.state == cases[i].state);
        assert(result.step_count == cases[i].steps);
        assert(result.error_line == cases[i].line);
        if (result.state != CHAON_INSPECT_VALID)
            assert(program.count == 0);
        printf("%s\t%zu\tstate=%d\tsteps=%u\tline=%u\n",
               cases[i].name, length, (int)result.state,
               (unsigned)result.step_count, (unsigned)result.error_line);
    }

    assert(sizeof "DELAY 250\nTYPE \"FRESH A\"\nENTER\n" ==
           sizeof "DELAY 250\nTYPE \"FRESH B\"\nENTER\n");
    puts("Phase 2E.2 fixture validation: PASS");
    return 0;
}
