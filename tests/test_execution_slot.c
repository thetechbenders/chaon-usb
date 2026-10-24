/* SPDX-License-Identifier: AGPL-3.0-only */
#include "chaon_execution_slot.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
static chaon_selection_t selected;
static chaon_execution_slot_t slot;
static chaon_execution_grant_t grant;
static chaon_program_t original, taken;
int main(void)
{
    chaon_fat_entry_t id={0};
    uint8_t sha[32]={1}, bad[32]={1};
    bad[31]=1;
    memcpy(id.raw_name,"VALID   CHN",11);
    original.count=1;
    original.steps[0].op=CHAON_OP_DELAY;
    original.steps[0].delay_ms=500;
    assert(chaon_selection_load(&selected,&id,&original,7,3,sha));
    assert(!chaon_execution_grant_issue(&grant,&selected,false));
    assert(!chaon_execution_slot_prepare(&slot,&grant,&selected,7,3,sha,true));
    assert(chaon_execution_grant_issue(&grant,&selected,true));
    assert(!chaon_execution_slot_prepare(&slot,&grant,&selected,7,3,bad,true));
    assert(!grant.issued && !slot.ready);
    assert(chaon_execution_grant_issue(&grant,&selected,true));
    assert(!chaon_execution_slot_prepare(&slot,&grant,&selected,7,3,sha,false));
    assert(chaon_execution_grant_issue(&grant,&selected,true));
    assert(chaon_execution_slot_prepare(&slot,&grant,&selected,7,3,sha,true));
    assert(!grant.issued);
    assert(!chaon_execution_slot_prepare(&slot,&grant,&selected,7,3,sha,true));
    assert(!slot.ready);
    assert(chaon_execution_grant_issue(&grant,&selected,true));
    assert(chaon_execution_slot_prepare(&slot,&grant,&selected,7,3,sha,true));
    original.steps[0].delay_ms=999;
    chaon_selection_clear(&selected);
    assert(!chaon_execution_slot_take(&slot,&selected,7,3,sha,true,&taken));
    assert(!slot.ready && taken.count==0);
    assert(chaon_selection_load(&selected,&id,&original,8,3,sha));
    assert(chaon_execution_grant_issue(&grant,&selected,true));
    assert(chaon_execution_slot_prepare(&slot,&grant,&selected,8,3,sha,true));
    assert(!chaon_execution_slot_take(&slot,&selected,8,3,sha,false,&taken));
    assert(!slot.ready);
    assert(chaon_execution_grant_issue(&grant,&selected,true));
    assert(chaon_execution_slot_prepare(&slot,&grant,&selected,8,3,sha,true));
    assert(!chaon_execution_slot_take(&slot,&selected,8,4,sha,true,&taken));
    assert(chaon_execution_grant_issue(&grant,&selected,true));
    assert(chaon_execution_slot_prepare(&slot,&grant,&selected,8,3,sha,true));
    assert(chaon_execution_slot_take(&slot,&selected,8,3,sha,true,&taken));
    assert(taken.steps[0].delay_ms==999);
    assert(!chaon_execution_slot_take(&slot,&selected,8,3,sha,true,&taken));
    puts("ChaoN revoked one-shot execution ownership tests: PASS");
    return 0;
}
