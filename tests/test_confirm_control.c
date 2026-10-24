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
    /* Inspection finishes after a deliberate PICK. */
    assert(poll(&c,&t,false,true,true,true)==CHAON_CONTROL_PICK_READY);
    assert(!c.browser_active && !c.browser_selected &&
           c.state==CHAON_STATUS_READY);
    assert(poll(&c,&t,false,true,true,true)==CHAON_CONTROL_NONE);
    assert(gesture(&c,&t,80,false)==CHAON_CONTROL_ARM);
    assert(gesture(&c,&t,80,false)==CHAON_CONTROL_EXECUTE);
    c=(chaon_control_t){0}; t=0; prepare(&c,&t);
    assert(poll(&c,&t,false,false,true,true)==CHAON_CONTROL_NONE);
    assert(c.browser_selected && c.browser_active);
    assert(gesture(&c,&t,80,false)==CHAON_CONTROL_BROWSER_SELECT);
    assert(!c.browser_selected);
    assert(gesture(&c,&t,3100,false)==CHAON_CONTROL_BROWSER_CLOSE);
    c=(chaon_control_t){0}; t=0; prepare(&c,&t);
    assert(poll(&c,&t,true,true,true,true)==CHAON_CONTROL_NONE);
    assert(c.browser_active && c.browser_selected);
    /* USB loss must never complete PICK. */
    assert(poll(&c,&t,false,true,false,true)!=CHAON_CONTROL_PICK_READY);
    assert(!c.browser_active);
    puts("ChaoN PICK READY gesture tests: PASS");
    return 0;
}
