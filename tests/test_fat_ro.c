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
    /* Bytes at MBR offset 11 may coincidentally equal 512. */
    w16(mbr+11,512);
    item(3,0,"TEST    BIN",0x20);
    assert(scan()==CHAON_FAT_OK);
    assert(list.count==1 && strcmp(list.entries[0].name,"TEST.BIN")==0);
    assert(reads<=4);
    /* An untrusted partition record must never trigger out-of-media I/O. */
    w32(mbr+454,UINT32_MAX);
    assert(scan()==CHAON_FAT_INVALID);
    w32(mbr+454,1);w32(mbr+458,UINT32_MAX);
    assert(scan()==CHAON_FAT_INVALID);
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

/* Sparse synthetic media exercise true FAT16/FAT32 geometry without
 * allocating a 100,000-sector host test image. */
static uint8_t sparse_boot[512], sparse_fat[512], sparse_fat2[512];
static uint8_t sparse_root[512], sparse_next[512];
static uint32_t sparse_root_lba, sparse_next_lba;
static bool sparse_read(void *ctx, uint32_t lba, uint8_t out[512])
{
    (void)ctx;
    const uint8_t *src = NULL;
    if (lba == 0) src = sparse_boot;
    else if (lba == sparse_root_lba) src = sparse_root;
    else if (lba == sparse_next_lba) src = sparse_next;
    else if (lba == 32) src = sparse_fat;
    else if (lba == 832) src = sparse_fat2;
    if (!src) return false;
    memcpy(out, src, 512);
    return true;
}
static void test_fat32_chain(void)
{
    memset(sparse_boot,0,512);memset(sparse_fat,0,512);
    memset(sparse_root,0xE5,512);memset(sparse_next,0,512);
    uint8_t *b=sparse_boot;
    b[0]=0xEB;b[510]=0x55;b[511]=0xAA;
    w16(b+11,512);b[13]=1;w16(b+14,32);b[16]=1;
    w32(b+32,90000);w32(b+36,800);w32(b+44,2);
    sparse_root_lba=832;sparse_next_lba=833;
    w32(sparse_fat+8,3);w32(sparse_fat+12,0x0FFFFFFF);
    memcpy(sparse_next,"SECOND  TXT",11);
    sparse_next[11]=0x20;
    assert(chaon_fat_list_root(sparse_read,NULL,100000,work,&list)==CHAON_FAT_OK);
    assert(list.type==CHAON_FAT32 && list.count==1 && !list.more);
    assert(strcmp(list.entries[0].name,"SECOND.TXT")==0);

    /* A malformed cycle must reach a fixed scan ceiling, not loop. */
    w32(sparse_fat+8,2);
    assert(chaon_fat_list_root(sparse_read,NULL,100000,work,&list)==CHAON_FAT_LIMIT);
    assert(list.more && list.count==0);
    w32(sparse_fat+8,0x0FFFFFF7); /* bad cluster */
    assert(chaon_fat_list_root(sparse_read,NULL,100000,work,&list)==CHAON_FAT_INVALID);
}
static void test_fat32_active_fat(void)
{
    memset(sparse_boot,0,512);memset(sparse_fat,0,512);
    memset(sparse_fat2,0,512);memset(sparse_root,0xE5,512);
    memset(sparse_next,0,512);
    uint8_t *b=sparse_boot;
    b[0]=0xEB;b[510]=0x55;b[511]=0xAA;
    w16(b+11,512);b[13]=1;w16(b+14,32);b[16]=2;
    w32(b+32,90000);w32(b+36,800);w32(b+44,2);
    w16(b+40,0x0081); /* mirroring disabled: use FAT #1 */
    sparse_root_lba=1632;sparse_next_lba=1633;
    w32(sparse_fat+8,0x0FFFFFF7); /* inactive FAT #0 is bad */
    w32(sparse_fat2+8,3);w32(sparse_fat2+12,0x0FFFFFFF);
    memcpy(sparse_next,"ACTIVE  TXT",11);sparse_next[11]=0x20;
    assert(chaon_fat_list_root(sparse_read,NULL,100000,work,&list)==CHAON_FAT_OK);
    assert(list.type==CHAON_FAT32 && list.count==1);
    assert(strcmp(list.entries[0].name,"ACTIVE.TXT")==0);
    w16(b+40,0x0082); /* nonexistent FAT #2: reject */
    assert(chaon_fat_list_root(sparse_read,NULL,100000,work,&list)==CHAON_FAT_INVALID);
}

static void test_fat16_sparse(void)
{
    memset(sparse_boot,0,512);memset(sparse_root,0,512);
    uint8_t *b=sparse_boot;
    b[0]=0xEB;b[510]=0x55;b[511]=0xAA;
    w16(b+11,512);b[13]=1;w16(b+14,1);b[16]=1;
    w16(b+17,32);w16(b+19,12000);w16(b+22,50);
    sparse_root_lba=51;sparse_next_lba=52;
    memcpy(sparse_root,"A       TXT",11);
    sparse_root[11]=0x20;
    assert(chaon_fat_list_root(sparse_read,NULL,20000,work,&list)==CHAON_FAT_OK);
    assert(list.type==CHAON_FAT16 && list.count==1);
    assert(strcmp(list.entries[0].name,"A.TXT")==0);
}

int main(void)
{
    fat12_root();mbr_partition();bounded_and_invalid();
    test_fat16_sparse();test_fat32_chain();test_fat32_active_fat();
    puts("ChaoN FAT root read-only catalogue tests: PASS");
    return 0;
}
