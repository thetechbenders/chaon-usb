/* SPDX-License-Identifier: AGPL-3.0-only */
#include "chaon_run_authority.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static chaon_selection_t selected;
static chaon_program_t program;
static chaon_run_authority_t authority;
static chaon_execution_grant_t grant;
static void init(void)
{
    chaon_fat_entry_t entry={0};
    uint8_t digest[32]={1};
    memcpy(entry.raw_name,"VALID   CHN",11);
    program.count=1;
    program.steps[0].op=CHAON_OP_DELAY;
    program.steps[0].delay_ms=100;
    chaon_run_authority_cancel(&authority);
    chaon_execution_grant_clear(&grant);
    assert(chaon_selection_load(&selected,&entry,&program,2,7,digest));
}
static bool step(chaon_control_event_t e,int64_t t,bool usb,bool fresh)
{
    return chaon_run_authority_step(&authority,e,t,usb,fresh,&selected,&grant);
}
int main(void)
{
    init();
    assert(!step(CHAON_CONTROL_EXECUTE,100,true,true));
    assert(!step(CHAON_CONTROL_ARM,100,true,true));
    assert(authority.armed);
    assert(!step(CHAON_CONTROL_NONE,200,true,true));
    assert(step(CHAON_CONTROL_EXECUTE,300,true,true));
    assert(grant.issued);
    assert(!step(CHAON_CONTROL_EXECUTE,320,true,true));
    assert(!grant.issued);
    init();
    assert(!step(CHAON_CONTROL_ARM,100,true,true));
    assert(!step(CHAON_CONTROL_NONE,30100,true,true));
    assert(!step(CHAON_CONTROL_EXECUTE,30120,true,true));
    init();
    assert(!step(CHAON_CONTROL_ARM,100,true,true));
    assert(!step(CHAON_CONTROL_EXECUTE,200,false,true));
    init();
    assert(!step(CHAON_CONTROL_ARM,100,true,true));
    assert(!step(CHAON_CONTROL_EXECUTE,200,true,false));
    init();
    assert(!step(CHAON_CONTROL_ARM,100,true,true));
    assert(!step(CHAON_CONTROL_BROWSER_ENTER,120,true,true));
    assert(!step(CHAON_CONTROL_EXECUTE,200,true,true));
    init();
    assert(!step(CHAON_CONTROL_ARM,100,true,true));
    selected.staging_generation++;
    assert(!step(CHAON_CONTROL_EXECUTE,200,true,true));
    init();
    assert(!step(CHAON_CONTROL_ARM,100,true,true));
    selected.source_sha256[31]=1;
    assert(!step(CHAON_CONTROL_EXECUTE,200,true,true));
    init();
    assert(!step(CHAON_CONTROL_ARM,100,true,true));
    assert(!step(CHAON_CONTROL_NONE,90,true,true));
    assert(!step(CHAON_CONTROL_EXECUTE,200,true,true));
    puts("ChaoN controller event-bound run authority tests: PASS");
    return 0;
}
