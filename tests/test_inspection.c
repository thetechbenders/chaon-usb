/* SPDX-License-Identifier: AGPL-3.0-only */
#include "chaon_fat_ro.h"
#include "chaon_inspect.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Sparse FAT32 layout with 1-sector clusters, no RAM-hungry disk image. */
static uint8_t boot[512], fat[512], root[512], file_a[512], file_b[512], temp[512];
static uint8_t output[2048];
static chaon_fat_catalog_t list;
static uint32_t reads, fail_lba;
static bool fetch(void *ctx, uint32_t lba, uint8_t out[512])
{
    (void)ctx;
    ++reads;
    if (lba == fail_lba) return false;
    const uint8_t *source = NULL;
    if (lba == 0) source = boot;
    else if (lba == 32) source = fat;
    else if (lba == 832) source = root;
    else if (lba == 833) source = file_a;
    else if (lba == 834) source = file_b;
    if (!source) return false;
    memcpy(out, source, 512);
    return true;
}
static void w16(uint8_t *p, uint32_t v)
{ p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8); }
static void w32(uint8_t *p, uint32_t v)
{ for (unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(i*8)); }
static void fixture(const char *text)
{
    memset(boot,0,sizeof boot);
    memset(fat,0,sizeof fat);
    memset(root,0,sizeof root);
    memset(file_a,0,sizeof file_a);
    memset(file_b,0,sizeof file_b);
    boot[0]=0xEB;boot[510]=0x55;boot[511]=0xAA;
    w16(boot+11,512);boot[13]=1;w16(boot+14,32);
    boot[16]=1;w32(boot+32,90000);w32(boot+36,800);
    w32(boot+44,2);
    memcpy(root,"DEMO    CHN",11);root[11]=0x20;
    w16(root+26,3);
    const size_t len=strlen(text);
    assert(len < 2048);
    w32(root+28,(uint32_t)len);
    memcpy(file_a,text,len<512?len:512);
    if (len>512) memcpy(file_b,text+512,len-512);
    w32(fat+8,0x0FFFFFFF); /* root EOC */
    w32(fat+12,4);          /* file cluster 3 -> 4 */
    w32(fat+16,0x0FFFFFFF);
    reads=0;fail_lba=UINT32_MAX;
    assert(chaon_fat_list_root(fetch,NULL,90000,temp,&list)==CHAON_FAT_OK);
    assert(list.type==CHAON_FAT32&&list.count==1);
    assert(list.entries[0].first_cluster==3);
    assert(list.entries[0].file_size==len);
    assert(!memcmp(list.entries[0].raw_name,"DEMO    CHN",11));
}
static chaon_fat_file_result_t open_selected(size_t *size)
{
    return chaon_fat_read_script(fetch,NULL,90000,temp,0,
                                  &list.entries[0],output,size);
}
static void basic_inspection(void)
{
    const char text[]="DELAY 50\nTYPE \"Hello\"\nTAB\n";
    fixture(text);
    size_t size=0;
    assert(open_selected(&size)==CHAON_FAT_FILE_OK);
    assert(size==strlen(text) && !memcmp(output,text,size));
    assert(output[size]==0);
    const chaon_inspection_t r=chaon_inspect_bytes(output,size);
    assert(r.state==CHAON_INSPECT_VALID && r.step_count==3);
    assert(r.first_op==CHAON_INSPECT_OP_DELAY && r.error_line==0);
    assert(reads<20);
}
static void chained_and_bad_chain(void)
{
    char long_source[700]={0};
    size_t at=0;
    for (unsigned line=0;line<9;++line) {
        long_source[at++]='#';
        for (unsigned x=0;x<57;++x) long_source[at++]='a';
        long_source[at++]='\n';
    }
    memcpy(long_source+at,"TYPE \"OK\"\n",10);at+=10;
    assert(at>512 && at<2048);
    fixture(long_source);
    size_t size=0;
    assert(open_selected(&size)==CHAON_FAT_FILE_OK);
    assert(size==at);
    assert(chaon_inspect_bytes(output,size).state==CHAON_INSPECT_VALID);
    w32(fat+12,0x0FFFFFFF); /* early EOC */
    assert(open_selected(&size)==CHAON_FAT_FILE_INVALID);
    assert(size==0 && output[0]==0);
    w32(fat+12,3); /* self cycle */
    assert(open_selected(&size)==CHAON_FAT_FILE_INVALID);
    assert(size==0);
    w32(fat+12,4);
    fail_lba=834;
    assert(open_selected(&size)==CHAON_FAT_FILE_IO_ERROR);
    assert(size==0 && output[0]==0);
}
static void stale_and_bounds(void)
{
    fixture("TYPE \"SAFE\"\n");
    size_t size=15;
    chaon_fat_entry_t expected=list.entries[0];
    expected.first_cluster=4;
    assert(chaon_fat_read_script(fetch,NULL,90000,temp,0,
        &expected,output,&size)==CHAON_FAT_FILE_STALE);
    assert(size==0);
    expected=list.entries[0];
    memcpy(expected.raw_name,"DIFF    CHN",11);
    assert(chaon_fat_read_script(fetch,NULL,90000,temp,0,
        &expected,output,&size)==CHAON_FAT_FILE_STALE);
    expected=list.entries[0];
    expected.is_directory=true;
    assert(chaon_fat_read_script(fetch,NULL,90000,temp,0,
        &expected,output,&size)==CHAON_FAT_FILE_UNSUPPORTED);
    expected=list.entries[0];
    memcpy(expected.raw_name+8,"TXT",3);
    assert(chaon_fat_read_script(fetch,NULL,90000,temp,0,
        &expected,output,&size)==CHAON_FAT_FILE_UNSUPPORTED);
    w32(root+28,2048);
    expected=list.entries[0];
    expected.file_size=2048;
    assert(chaon_fat_read_script(fetch,NULL,90000,temp,0,
        &expected,output,&size)==CHAON_FAT_FILE_TOO_LARGE);
    assert(chaon_fat_read_script(NULL,NULL,90000,temp,0,
        &expected,output,&size)==CHAON_FAT_FILE_INVALID);
    assert(chaon_fat_read_script(fetch,NULL,90000,temp,8,
        &expected,output,&size)==CHAON_FAT_FILE_INVALID);
}
static void syntax_and_ascii(void)
{
    static const uint8_t a[]="TYPE \"abc\"\r\nENTER\n";
    chaon_inspection_t r=chaon_inspect_bytes(a,sizeof(a)-1);
    assert(r.state==CHAON_INSPECT_VALID && r.step_count==2);
    static const uint8_t b[]="TYPE \"oops\n";
    r=chaon_inspect_bytes(b,sizeof(b)-1);
    assert(r.state==CHAON_INSPECT_BAD_SYNTAX&&r.error_line==1);
    static const uint8_t c[]={'T','A','B','\n',0,'E','N','T','E','R'};
    r=chaon_inspect_bytes(c,sizeof c);
    assert(r.state==CHAON_INSPECT_BAD_SOURCE);
    static const uint8_t d[]={'T','A','B','\n',0xFF};
    r=chaon_inspect_bytes(d,sizeof d);
    assert(r.state==CHAON_INSPECT_BAD_SOURCE);
    static const uint8_t e[]={'T','A','B','\n',0x1B};
    r=chaon_inspect_bytes(e,sizeof e);
    assert(r.state==CHAON_INSPECT_BAD_SOURCE);
    r=chaon_inspect_bytes(NULL,1);
    assert(r.state==CHAON_INSPECT_BAD_SOURCE);
    r=chaon_inspect_bytes(a,2048);
    assert(r.state==CHAON_INSPECT_BAD_SOURCE);
    r=chaon_inspect_bytes(a,0);
    assert(r.state==CHAON_INSPECT_BAD_SOURCE);
}
int main(void)
{
    basic_inspection();chained_and_bad_chain();stale_and_bounds();
    syntax_and_ascii();
    puts("ChaoN bounded read-only script inspection tests: PASS");
    return 0;
}
