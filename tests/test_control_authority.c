/* SPDX-License-Identifier: AGPL-3.0-only */
#include "chaon_control.h"
#include "chaon_selection.h"
#include "chaon_run_authority.h"
#include "chaon_execution_slot.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static chaon_control_t control;
static chaon_selection_t selected;
static chaon_run_authority_t authority;
static chaon_execution_grant_t grant;
static chaon_execution_slot_t slot;
static chaon_program_t source_program, output;
static int64_t tick;
static unsigned issued;

static void drive(bool down, bool mounted)
{
    const chaon_control_event_t event=chaon_control_step(
        &control,mounted,down,tick);
    if (chaon_run_authority_step(&authority,event,tick,mounted,mounted,
        &selected,&grant)) issued++;
    tick+=CHAON_CONTROL_POLL_MS;
}
static void gesture(unsigned hold_ms)
{
    for(unsigned ms=0;ms<hold_ms;ms+=CHAON_CONTROL_POLL_MS)
        drive(true,true);
    for(int i=0;i<5;i++) drive(false,true);
}
static void reset(void)
{
    chaon_fat_entry_t entry={0};
    uint8_t digest[CHAON_SOURCE_SHA256_BYTES]={2};
    control=(chaon_control_t){0};
    chaon_selection_clear(&selected);
    chaon_execution_slot_clear(&slot);
    chaon_execution_grant_clear(&grant);
    chaon_run_authority_cancel(&authority);
    tick=0;issued=0;
    memcpy(entry.raw_name,"VALID   CHN",11);
    source_program=(chaon_program_t){0};
    source_program.count=1;
    source_program.steps[0].op=CHAON_OP_DELAY;
    source_program.steps[0].delay_ms=50;
    assert(chaon_selection_load(&selected,&entry,&source_program,17,4,digest));
    for(int i=0;i<5;i++) drive(false,true);
}
int main(void)
{
    reset();
    assert(control.state==CHAON_STATUS_READY);
    assert(!grant.issued);
    gesture(100);
    assert(control.state==CHAON_STATUS_ARMED && authority.armed);
    assert(!grant.issued);
    gesture(100);
    assert(control.state==CHAON_STATUS_RUNNING);
    assert(issued==1 && grant.issued);
    assert(chaon_execution_slot_prepare(&slot,&grant,&selected,17,4,
        selected.source_sha256,true));
    assert(!grant.issued);
    assert(chaon_execution_slot_take(&slot,&selected,17,4,
        selected.source_sha256,true,&output));
    assert(output.steps[0].delay_ms==50);
    assert(!chaon_execution_slot_take(&slot,&selected,17,4,
        selected.source_sha256,true,&output));
    reset();
    gesture(100);
    assert(authority.armed);
    /* Expired controller ARM cannot mint an execution grant. */
    tick+=CHAON_CONTROL_ARM_TTL_MS;
    drive(false,true);
    gesture(100);
    assert(issued==0 && !grant.issued);
    reset();
    gesture(100);
    chaon_selection_clear(&selected);
    gesture(100);
    assert(issued==0 && !grant.issued);
    reset();
    gesture(100);
    drive(false,false);
    drive(false,true);
    gesture(100);
    assert(issued==0 && !grant.issued);
    puts("ChaoN controller-to-grant-to-slot integrated tests: PASS");
    return 0;
}
