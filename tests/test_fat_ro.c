/* SPDX-License-Identifier: AGPL-3.0-only */
#include "chaon_fat_ro.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
enum { TEST_SECTORS = 256 };
static uint8_t disk[TEST_SECTORS][512];
static uint8_t work[512];
static chaon_fat_catalog_t list;
static uint32_t reads, fail_at;
static bool fetch(void *ctx, uint32_t lba, uint8_t out[512])
{
    (void)ctx;
    ++reads;
    if (lba >= TEST_SECTORS || (fail_at && lba == fail_at)) return false;
    memcpy(out, disk[lba], 512);
    return true;
}
static void w16(uint8_t *p, unsigned v)
{ p[0] = (uint8_t)v; p[1] = (uint8_t)(v>>8); }
static void w32(uint8_t *p, uint32_t v)
{ for (unsigned i=0;i<4;++i) p[i] = (uint8_t)(v>>(8*i)); }
static void boot(unsigned lba, unsigned root_entries)
{
    uint8_t *s=disk[lba];s[0]=0xEB;s[510]=0x55;s[511]=0xAA;
    w16(s+11,512);s[13]=1;w16(s+14,1);s[16]=1;
    w16(s+17,root_entries);w16(s+19,200);w16(s+22,1);
}
static void item(unsigned lba, unsigned n, const char *name, uint8_t attr)
{
    uint8_t *p=disk[lba]+n*32;
    memcpy(p,name,11);p[11]=attr;
}
static void reset(void)
{memset(disk,0,sizeof disk);memset(&list,0,sizeof list);reads=0;fail_at=0;}
static chaon_fat_result_t scan(void)
{return chaon_fat_list_root(fetch,NULL,TEST_SECTORS,work,&list);}
static void fat12_root(void)
{
    reset();boot(0,16);
    item(2,0,"HELLO   TXT",0x20);
    item(2,1,"FOLDER     ",0x10);
    disk[2][64]=0; /* end marker */
    assert(scan()==CHAON_FAT_OK);
    assert(list.type==CHAON_FAT12 && list.count==2 && !list.more);
    assert(strcmp(list.entries[0].name,"HELLO.TXT")==0);
    assert(strcmp(list.entries[1].name,"FOLDER")==0);
    assert(list.entries[1].is_directory);
    assert(reads<=3);
}
static void mbr_partition(void)
{
    reset();boot(1,16);
    uint8_t *mbr=disk[0];mbr[510]=0x55;mbr[511]=0xAA;
    mbr[450]=0x06;w32(mbr+454,1);w32(mbr+458,200);
    item(3,0,"TEST    BIN",0x20);
    assert(scan()==CHAON_FAT_OK);
    assert(list.count==1 && strcmp(list.entries[0].name,"TEST.BIN")==0);
    assert(reads<=4);
}
static void bounded_and_invalid(void)
{
    reset();boot(0,32);
    for(unsigned i=0;i<9;i++)item(2,i,"SAMPLE  TXT",0x20);
    assert(scan()==CHAON_FAT_OK);
    assert(list.count==8 && list.more && reads<=3);
    reset();boot(0,16);w16(disk[0]+11,4096);
    assert(scan()!=CHAON_FAT_OK);
    reset();boot(0,16);disk[0][13]=0;
    assert(scan()==CHAON_FAT_INVALID);
    reset();boot(0,16);fail_at=2;
    assert(scan()==CHAON_FAT_IO_ERROR && list.count==0);
    reset();boot(0,16);w16(disk[0]+19,250);
    assert(scan()==CHAON_FAT_OK);
    reset();boot(0,16);w16(disk[0]+19,257);
    assert(scan()==CHAON_FAT_INVALID);
    reset();boot(0,16);disk[0][510]=0;
    assert(scan()==CHAON_FAT_UNSUPPORTED);
    reset();assert(chaon_fat_list_root(NULL,NULL,256,work,&list)==CHAON_FAT_INVALID);
}
int main(void)
{
    fat12_root();mbr_partition();bounded_and_invalid();
    puts("ChaoN FAT root read-only catalogue tests: PASS");
    return 0;
}
