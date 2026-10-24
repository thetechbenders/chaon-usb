/* SPDX-License-Identifier: AGPL-3.0-only */
#include "chaon_selection.h"
#include <string.h>
static bool valid_program(const chaon_program_t *p)
{
    if (!p || !p->count || p->count > CHAON_MAX_STEPS) return false;
    for (size_t i=0; i<p->count; ++i) {
        const chaon_instruction_t *v=&p->steps[i];
        switch(v->op) {
        case CHAON_OP_TYPE: {
            bool term=false;
            for(size_t j=0;j<=CHAON_MAX_TEXT;++j) {
                unsigned char ch=(unsigned char)v->text[j];
                if(!ch) { term=true; break; }
                if(ch<32 || ch>126) return false;
            }
            if(!term) return false;
            break;
        }
        case CHAON_OP_DELAY:
            if(!v->delay_ms || v->delay_ms>10000) return false;
            break;
        case CHAON_OP_ENTER:
        case CHAON_OP_TAB: break;
        default: return false;
        }
    }
    return true;
}
static bool digest_present(const uint8_t *hash)
{
    if(!hash) return false;
    uint8_t any=0;
    for(unsigned i=0;i<CHAON_SOURCE_SHA256_BYTES;++i) any|=hash[i];
    return any!=0;
}
void chaon_selection_clear(chaon_selection_t *s)
{
    if(s) memset(s,0,sizeof *s);
}
bool chaon_selection_load(chaon_selection_t *s,
    const chaon_fat_entry_t *identity, const chaon_program_t *validated,
    uint32_t generation, uint32_t usb_epoch,
    const uint8_t sha256[CHAON_SOURCE_SHA256_BYTES])
{
    if(!s) return false;
    chaon_selection_clear(s);
    if(!identity || identity->is_directory ||
       memcmp(identity->raw_name+8,"CHN",3)!=0 ||
       !valid_program(validated) || !generation || !usb_epoch ||
       !digest_present(sha256)) return false;
    s->identity=*identity;
    s->program=*validated;
    s->staging_generation=generation;
    s->usb_epoch=usb_epoch;
    memcpy(s->source_sha256,sha256,CHAON_SOURCE_SHA256_BYTES);
    s->valid=true;
    return true;
}
void chaon_selection_refresh(chaon_selection_t *s,
                              uint32_t usb_epoch, bool source_still_fresh)
{
    if(s && s->valid && (!source_still_fresh ||
                         !usb_epoch || s->usb_epoch!=usb_epoch))
        chaon_selection_clear(s);
}
bool chaon_selection_matches(const chaon_selection_t *s,
    uint32_t generation, uint32_t usb_epoch,
    const uint8_t sha256[CHAON_SOURCE_SHA256_BYTES])
{
    return s && s->valid && generation && usb_epoch &&
        digest_present(sha256) &&
        s->staging_generation==generation && s->usb_epoch==usb_epoch &&
        memcmp(s->source_sha256,sha256,CHAON_SOURCE_SHA256_BYTES)==0;
}
