/* SPDX-License-Identifier: AGPL-3.0-only */
#include "chaon_source_verify.h"
#include "chaon_script.h"
#include <string.h>
static void wipe(void *p,size_t n)
{
    volatile uint8_t *v=p;
    while(n--) *v++=0;
}
bool chaon_source_verify(chaon_fat_read_t reader, void *ctx,
    uint32_t sectors, uint8_t sector[CHAON_FAT_SECTOR_BYTES],
    uint8_t index, const chaon_fat_entry_t *expected,
    const uint8_t digest[CHAON_SHA256_SIZE],
    uint8_t source[2048])
{
    if (!sector || !source) return false;
    bool ok=false;
    if (reader && expected && digest && sectors) {
        size_t length=0;
        const chaon_fat_file_result_t result=chaon_fat_read_script(reader,
            ctx,sectors,sector,index,expected,source,CHAON_MAX_SOURCE,&length);
        if (result==CHAON_FAT_FILE_OK) {
            uint8_t observed[CHAON_SHA256_SIZE];
            chaon_source_sha256(source,length,observed);
            unsigned diff=0;
            for(unsigned i=0;i<CHAON_SHA256_SIZE;i++)
                diff|=(unsigned)(observed[i]^digest[i]);
            ok=diff==0;
            wipe(observed,sizeof observed);
        }
    }
    wipe(source,CHAON_MAX_SOURCE);
    wipe(sector,CHAON_FAT_SECTOR_BYTES);
    return ok;
}
