/* SPDX-License-Identifier: AGPL-3.0-only */
#include "chaon_run_authority.h"
#include <string.h>
void chaon_run_authority_cancel(chaon_run_authority_t *a)
{
    if(a) memset(a,0,sizeof *a);
}
bool chaon_run_authority_step(chaon_run_authority_t *a,
    chaon_control_event_t event, int64_t now_ms, bool usb_mounted,
    bool source_available, const chaon_selection_t *s,
    chaon_execution_grant_t *out)
{
    chaon_execution_grant_clear(out);
    if(!a) return false;
    const bool eligible=usb_mounted && source_available && s && s->valid &&
        s->staging_generation && s->usb_epoch && now_ms>=0;
    if(!eligible) { chaon_run_authority_cancel(a); return false; }
    if(a->armed && (now_ms<a->armed_ms ||
        now_ms-a->armed_ms>=CHAON_CONTROL_ARM_TTL_MS ||
        a->generation!=s->staging_generation || a->usb_epoch!=s->usb_epoch ||
        memcmp(a->digest,s->source_sha256,sizeof a->digest)!=0))
        chaon_run_authority_cancel(a);
    if(event==CHAON_CONTROL_ARM) {
        chaon_run_authority_cancel(a);
        a->armed=true;
        a->armed_ms=now_ms;
        a->generation=s->staging_generation;
        a->usb_epoch=s->usb_epoch;
        memcpy(a->digest,s->source_sha256,sizeof a->digest);
        return false;
    }
    if(event==CHAON_CONTROL_EXECUTE && a->armed) {
        /* Must be the distinct, debounced second physical controller event.
         * Consume arm regardless of grant issuance outcome. */
        chaon_run_authority_cancel(a);
        return chaon_execution_grant_issue(out,s,true);
    }
    if(event!=CHAON_CONTROL_NONE)
        chaon_run_authority_cancel(a);
    return false;
}
