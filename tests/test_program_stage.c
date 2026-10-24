/* SPDX-License-Identifier: AGPL-3.0-only */
#include "chaon_inspect.h"
#include "chaon_program_stage.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static chaon_inspect_workspace_t workspace;
static chaon_program_t candidate;
static chaon_program_t copied;
static unsigned hid_reports;
static struct {
    uint8_t before[16];
    chaon_program_t program;
    uint8_t after[16];
} guarded_copy;

static bool all_zero(const void *data, size_t size)
{
    const uint8_t *bytes = data;
    for (size_t i=0;i<size;++i) if (bytes[i]) return false;
    return true;
}

static chaon_fat_catalog_t catalog(void)
{
    chaon_fat_catalog_t c = {0};
    c.count = 2;
    memcpy(c.entries[0].raw_name,"FIRST   CHN",11);
    strcpy(c.entries[0].name,"FIRST.CHN");
    c.entries[0].first_cluster=3;
    c.entries[0].file_size=25;
    memcpy(c.entries[1].raw_name,"SECOND  CHN",11);
    strcpy(c.entries[1].name,"SECOND.CHN");
    c.entries[1].first_cluster=7;
    c.entries[1].file_size=12;
    return c;
}

static chaon_inspection_t compile(const char *source)
{
    return chaon_inspect_compile_bytes((const uint8_t *)source,
        strlen(source),&workspace,&candidate);
}

static void take(chaon_program_stage_t *stage, uint32_t *generation,
                 uint8_t *index, chaon_fat_entry_t *identity)
{
    assert(chaon_program_stage_take(stage,generation,index,identity));
    assert(!chaon_program_stage_take(stage,generation,index,identity));
}

static void test_valid_stage_and_clear(void)
{
    chaon_program_stage_t stage={0};
    const chaon_fat_catalog_t c=catalog();
    uint32_t generation=0;uint8_t index=255;chaon_fat_entry_t identity={0};
    assert(sizeof(chaon_program_t)<=3072);
    assert(sizeof(chaon_program_stage_t)<=3200);
    assert(chaon_program_stage_request(&stage,&c,0,&c.entries[0]));
    take(&stage,&generation,&index,&identity);
    const chaon_inspection_t result=compile("DELAY 7\nTYPE \"OK\"\nTAB\n");
    assert(result.state==CHAON_INSPECT_VALID&&result.step_count==3);
    assert(chaon_program_stage_publish(&stage,generation,index,&identity,
                                       &candidate));
    memset(&copied,0xA5,sizeof copied);
    uint32_t copied_generation=0;
    assert(chaon_program_stage_copy(&stage,0,&c.entries[0],
                                    &copied_generation,&copied));
    assert(copied_generation==generation);
    assert(copied.count==3&&copied.steps[0].delay_ms==7);
    assert(!strcmp(copied.steps[1].text,"OK"));
    memset(&guarded_copy,0x5A,sizeof guarded_copy);
    assert(chaon_program_stage_copy(&stage,0,&c.entries[0],
                                    &copied_generation,
                                    &guarded_copy.program));
    for (size_t i=0;i<sizeof guarded_copy.before;++i)
        assert(guarded_copy.before[i]==0x5A);
    for (size_t i=0;i<sizeof guarded_copy.after;++i)
        assert(guarded_copy.after[i]==0x5A);
    assert(hid_reports==0); /* This binary does not link VM or HID code. */
    chaon_program_stage_clear(&stage);
    assert(stage.state==CHAON_PROGRAM_STAGE_EMPTY);
    assert(all_zero(&stage.program,sizeof stage.program));
    assert(!chaon_program_stage_copy(&stage,0,&c.entries[0],
                                     &copied_generation,&copied));
    assert(copied_generation==0);
    assert(all_zero(&copied,sizeof copied));
}

static void test_cancellation_before_and_after_validation(void)
{
    chaon_program_stage_t stage={0};
    const chaon_fat_catalog_t c=catalog();
    uint32_t generation=0;uint8_t index=0;chaon_fat_entry_t identity={0};
    assert(chaon_program_stage_request(&stage,&c,0,&c.entries[0]));
    const uint32_t requested=stage.generation;
    chaon_program_stage_clear(&stage); /* cancelled before SD task takes it */
    assert(stage.generation!=requested);
    assert(!chaon_program_stage_take(&stage,&generation,&index,&identity));

    assert(chaon_program_stage_request(&stage,&c,0,&c.entries[0]));
    take(&stage,&generation,&index,&identity);
    assert(compile("TYPE \"SAFE\"\n").state==CHAON_INSPECT_VALID);
    chaon_program_stage_clear(&stage); /* USB loss after validation */
    assert(!chaon_program_stage_publish(&stage,generation,index,&identity,
                                        &candidate));
    assert(stage.state==CHAON_PROGRAM_STAGE_EMPTY);
    assert(all_zero(&stage.program,sizeof stage.program));
    assert(hid_reports==0);
}

static void test_generation_replacement_and_identity(void)
{
    chaon_program_stage_t stage={0};
    const chaon_fat_catalog_t c=catalog();
    uint32_t old_generation=0,new_generation=0;
    uint8_t old_index=0,new_index=0;
    chaon_fat_entry_t old_identity={0},new_identity={0};
    assert(chaon_program_stage_request(&stage,&c,0,&c.entries[0]));
    take(&stage,&old_generation,&old_index,&old_identity);
    assert(compile("TYPE \"OLD\"\n").state==CHAON_INSPECT_VALID);
    const chaon_program_t old_candidate=candidate;
    assert(chaon_program_stage_request(&stage,&c,1,&c.entries[1]));
    take(&stage,&new_generation,&new_index,&new_identity);
    assert(new_generation!=old_generation&&new_index==1);
    assert(!chaon_program_stage_publish(&stage,old_generation,old_index,
                                        &old_identity,&old_candidate));
    assert(stage.state==CHAON_PROGRAM_STAGE_PENDING);
    assert(compile("ENTER\n").state==CHAON_INSPECT_VALID);
    chaon_fat_entry_t wrong_publish=new_identity;
    wrong_publish.raw_name[0]='X';
    assert(!chaon_program_stage_publish(&stage,new_generation,new_index,
                                        &wrong_publish,&candidate));
    assert(stage.state==CHAON_PROGRAM_STAGE_PENDING);
    assert(chaon_program_stage_publish(&stage,new_generation,new_index,
                                       &new_identity,&candidate));
    /* Even a late failure from the superseded request cannot erase READY. */
    assert(!chaon_program_stage_publish(&stage,old_generation,old_index,
                                        &old_identity,NULL));
    assert(stage.state==CHAON_PROGRAM_STAGE_READY);
    uint32_t copied_generation=99;
    assert(!chaon_program_stage_copy(&stage,0,&old_identity,
                                     &copied_generation,&copied));
    assert(copied_generation==0);
    assert(all_zero(&copied,sizeof copied));
    chaon_fat_entry_t changed=new_identity;
    changed.first_cluster++;
    assert(!chaon_program_stage_copy(&stage,1,&changed,
                                     &copied_generation,&copied));
    assert(all_zero(&copied,sizeof copied));
    assert(chaon_program_stage_copy(&stage,1,&new_identity,
                                    &copied_generation,&copied));
    assert(copied_generation==new_generation);
    assert(copied.count==1&&copied.steps[0].op==CHAON_OP_ENTER);
}

static void test_failed_and_partial_program_rejection(void)
{
    chaon_program_stage_t stage={0};
    const chaon_fat_catalog_t c=catalog();
    uint32_t generation=0;uint8_t index=0;chaon_fat_entry_t identity={0};
    assert(chaon_program_stage_request(&stage,&c,0,&c.entries[0]));
    take(&stage,&generation,&index,&identity);
    assert(compile("TYPE \"OK\"\nBROKEN\n").state==
           CHAON_INSPECT_BAD_SYNTAX);
    assert(all_zero(&candidate,sizeof candidate));
    assert(!chaon_program_stage_publish(&stage,generation,index,&identity,
                                        &candidate));
    assert(stage.state==CHAON_PROGRAM_STAGE_EMPTY);
    assert(all_zero(&stage.program,sizeof stage.program));

    assert(chaon_program_stage_request(&stage,&c,0,&c.entries[0]));
    take(&stage,&generation,&index,&identity);
    candidate=(chaon_program_t){.count=1};
    candidate.steps[0].op=CHAON_OP_DELAY; /* zero delay is invalid */
    assert(!chaon_program_stage_publish(&stage,generation,index,&identity,
                                        &candidate));
    assert(all_zero(&stage.program,sizeof stage.program));

    /* SD read failure uses the same rejection path: NULL cannot publish. */
    assert(chaon_program_stage_request(&stage,&c,0,&c.entries[0]));
    take(&stage,&generation,&index,&identity);
    assert(!chaon_program_stage_publish(&stage,generation,index,&identity,NULL));
    assert(stage.state==CHAON_PROGRAM_STAGE_EMPTY);
    assert(all_zero(&stage.program,sizeof stage.program));

    /* A valid grammar result from a non-.CHN identity is never stageable. */
    chaon_fat_catalog_t not_script=c;
    memcpy(not_script.entries[0].raw_name+8,"TXT",3);
    assert(chaon_program_stage_request(&stage,&not_script,0,
                                       &not_script.entries[0]));
    take(&stage,&generation,&index,&identity);
    assert(compile("TAB\n").state==CHAON_INSPECT_VALID);
    assert(!chaon_program_stage_publish(&stage,generation,index,&identity,
                                        &candidate));
    assert(stage.state==CHAON_PROGRAM_STAGE_EMPTY);
    assert(all_zero(&stage.program,sizeof stage.program));
}

static void test_malformed_requests(void)
{
    chaon_program_stage_t stage={0};
    chaon_fat_catalog_t c=catalog();
    uint32_t generation=0;uint8_t index=0;chaon_fat_entry_t identity={0};
    assert(chaon_program_stage_request(&stage,&c,0,&c.entries[0]));
    take(&stage,&generation,&index,&identity);
    assert(compile("TAB\n").state==CHAON_INSPECT_VALID);
    assert(chaon_program_stage_publish(&stage,generation,index,&identity,
                                       &candidate));
    chaon_fat_entry_t changed=c.entries[0];
    changed.raw_name[0]='X';
    assert(!chaon_program_stage_request(&stage,&c,0,&changed));
    assert(stage.state==CHAON_PROGRAM_STAGE_EMPTY);
    assert(all_zero(&stage.program,sizeof stage.program));
    changed=c.entries[0];changed.is_directory=true;
    assert(!chaon_program_stage_request(&stage,&c,0,&changed));
    assert(!chaon_program_stage_request(&stage,&c,2,&c.entries[0]));
    c.count=CHAON_FAT_MAX_FILES+1;
    assert(!chaon_program_stage_request(&stage,&c,0,&c.entries[0]));
    chaon_program_stage_clear(NULL);
    uint32_t copied_generation=99;
    assert(!chaon_program_stage_copy(NULL,0,NULL,&copied_generation,&copied));
    assert(copied_generation==0);
    assert(all_zero(&copied,sizeof copied));
}

static void test_hashed_stage(void)
{
    chaon_program_stage_t stage={0};
    chaon_fat_catalog_t c=catalog();
    uint32_t generation=0, copied_generation=0;
    uint8_t index=0, digest[CHAON_SHA256_SIZE]={0}, copied_digest[CHAON_SHA256_SIZE]={0};
    chaon_fat_entry_t identity={0};
    assert(chaon_program_stage_request(&stage,&c,0,&c.entries[0]));
    take(&stage,&generation,&index,&identity);
    assert(compile("TAB\n").state==CHAON_INSPECT_VALID);
    assert(!chaon_program_stage_publish_hashed(&stage,generation,index,
                                                &identity,&candidate,digest));
    assert(stage.state==CHAON_PROGRAM_STAGE_PENDING);
    chaon_source_sha256((const uint8_t *)"TAB\n",4,digest);
    assert(chaon_program_stage_publish_hashed(&stage,generation,index,
                                               &identity,&candidate,digest));
    assert(chaon_program_stage_copy_hashed(&stage,index,&identity,
               &copied_generation,&copied,copied_digest));
    assert(copied_generation==generation);
    assert(memcmp(copied_digest,digest,sizeof digest)==0);
    chaon_program_stage_clear(&stage);
    assert(!chaon_program_stage_copy_hashed(&stage,index,&identity,
               &copied_generation,&copied,copied_digest));
    assert(all_zero(copied_digest,sizeof copied_digest));
}

int main(void)
{
    test_valid_stage_and_clear();
    test_cancellation_before_and_after_validation();
    test_generation_replacement_and_identity();
    test_failed_and_partial_program_rejection();
    test_malformed_requests();
    test_hashed_stage();
    printf("ChaoN bounded program staging tests: PASS (program=%zu stage=%zu)\n",
           sizeof(chaon_program_t),sizeof(chaon_program_stage_t));
    return 0;
}
