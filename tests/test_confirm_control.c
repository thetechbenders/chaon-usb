/* SPDX-License-Identifier: AGPL-3.0-only */
#include "chaon_control.h"
#include <assert.h>
#include <stdio.h>
static chaon_control_event_t poll(chaon_control_t *c, int64_t *t,
                                 bool pressed, bool eligible,
                                 bool connected, bool catalog)
{
    const chaon_control_event_t e=chaon_control_step_confirmation(c,
        connected,pressed,*t,catalog,catalog?1:0,eligible);
    *t+=CHAON_CONTROL_POLL_MS;
    return e;
}
static chaon_control_event_t gesture(chaon_control_t *c,int64_t *t,
                                     unsigned ms,bool eligible)
{
    chaon_control_event_t result=CHAON_CONTROL_NONE;
    for(unsigned i=0;i<ms/20;i++){
        const chaon_control_event_t e=poll(c,t,true,eligible,true,true);
        if(e!=CHAON_CONTROL_NONE)result=e;
    }
    for(unsigned i=0;i<4;i++){
        const chaon_control_event_t e=poll(c,t,false,eligible,true,true);
        if(e!=CHAON_CONTROL_NONE)result=e;
    }
    return result;
}
static void prepare(chaon_control_t *c,int64_t *t)
{
    for(int i=0;i<4;i++)(void)poll(c,t,false,false,true,true);
    assert(gesture(c,t,1020,false)==CHAON_CONTROL_BROWSER_ENTER);
    assert(gesture(c,t,1020,false)==CHAON_CONTROL_BROWSER_SELECT);
    assert(c->browser_selected);
}
int main(void)
{
    chaon_control_t c={0};
    int64_t t=0;
    prepare(&c,&t);
    assert(gesture(&c,&t,4800,true)==CHAON_CONTROL_NONE);
    assert(c.browser_selected && !c.confirm_active);
    assert(gesture(&c,&t,5100,true)==CHAON_CONTROL_CONFIRM_ENTER);
    assert(c.confirm_active);
    for(int i=0;i<30;i++)assert(poll(&c,&t,false,true,true,true)==CHAON_CONTROL_NONE);
    assert(c.confirm_release_ready);
    assert(gesture(&c,&t,1600,true)==CHAON_CONTROL_CONFIRM_ACK);
    assert(!c.confirm_active && !c.browser_active);
    assert(gesture(&c,&t,80,false)==CHAON_CONTROL_ARM);
    c=(chaon_control_t){0};t=0;prepare(&c,&t);
    assert(gesture(&c,&t,5200,false)==CHAON_CONTROL_NONE);
    assert(!c.confirm_active && c.browser_selected);
    assert(gesture(&c,&t,80,false)==CHAON_CONTROL_BROWSER_SELECT);
    assert(!c.browser_selected);
    assert(gesture(&c,&t,3100,false)==CHAON_CONTROL_BROWSER_CLOSE);
    c=(chaon_control_t){0};t=0;prepare(&c,&t);
    assert(gesture(&c,&t,5100,true)==CHAON_CONTROL_CONFIRM_ENTER);
    /* Source validity may disappear any moment. No old gesture can arm. */
    assert(poll(&c,&t,false,false,true,true)==CHAON_CONTROL_CONFIRM_CANCEL);
    assert(!c.confirm_active && !c.browser_active);
    c=(chaon_control_t){0};t=0;prepare(&c,&t);
    assert(gesture(&c,&t,5100,true)==CHAON_CONTROL_CONFIRM_ENTER);
    assert(poll(&c,&t,false,true,false,true)==CHAON_CONTROL_CONFIRM_CANCEL);
    c=(chaon_control_t){0};t=0;prepare(&c,&t);
    assert(gesture(&c,&t,5100,true)==CHAON_CONTROL_CONFIRM_ENTER);
    t+=CHAON_CONTROL_CONFIRM_TTL_MS;
    assert(poll(&c,&t,false,true,true,true)==CHAON_CONTROL_CONFIRM_CANCEL);
    c=(chaon_control_t){0};t=0;prepare(&c,&t);
    assert(gesture(&c,&t,5100,true)==CHAON_CONTROL_CONFIRM_ENTER);
    for(int i=0;i<30;i++)(void)poll(&c,&t,false,true,true,true);
    assert(gesture(&c,&t,80,true)==CHAON_CONTROL_CONFIRM_CANCEL);
    assert(!c.confirm_active);
    puts("ChaoN inert confirmation gesture tests: PASS");
    return 0;
}
