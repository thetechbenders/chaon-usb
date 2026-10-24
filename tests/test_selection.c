/* SPDX-License-Identifier: AGPL-3.0-only */
#include "chaon_selection.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
static chaon_selection_t selected;
static chaon_program_t program;
int main(void)
{
    chaon_fat_entry_t file={0};
    uint8_t sha[CHAON_SOURCE_SHA256_BYTES]={1};
    uint8_t altered[CHAON_SOURCE_SHA256_BYTES]={1};
    altered[31]=1;
    memcpy(file.raw_name,"VALID   CHN",11);
    program.count=1;
    program.steps[0].op=CHAON_OP_DELAY;
    program.steps[0].delay_ms=500;
    assert(!chaon_selection_load(&selected,&file,&program,0,1,sha));
    assert(!selected.valid);
    assert(chaon_selection_load(&selected,&file,&program,2,1,sha));
    assert(chaon_selection_matches(&selected,2,1,sha));
    assert(!chaon_selection_matches(&selected,3,1,sha));
    assert(!chaon_selection_matches(&selected,2,2,sha));
    assert(!chaon_selection_matches(&selected,2,1,altered));
    chaon_selection_refresh(&selected,1,true);
    assert(selected.valid);
    chaon_selection_refresh(&selected,1,false);
    assert(!selected.valid);
    assert(!chaon_selection_matches(&selected,2,1,sha));
    assert(chaon_selection_load(&selected,&file,&program,2,1,sha));
    chaon_selection_refresh(&selected,2,true);
    assert(!selected.valid);
    assert(chaon_selection_load(&selected,&file,&program,2,1,sha));
    program.steps[0].delay_ms=0;
    assert(!chaon_selection_load(&selected,&file,&program,2,1,sha));
    program.steps[0].delay_ms=500;
    assert(chaon_selection_load(&selected,&file,&program,2,1,sha));
    file.is_directory=true;
    assert(!chaon_selection_load(&selected,&file,&program,2,1,sha));
    assert(!selected.valid);
    puts("ChaoN selected-program ownership tests: PASS");
    return 0;
}
