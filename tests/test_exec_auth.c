/* SPDX-License-Identifier: AGPL-3.0-only */
#include "../main/chaon_exec_auth.h"
#include <assert.h>
int main(void) {
    chaon_exec_auth_t a = {0};
    assert(!chaon_exec_auth_begin(&a, 1, 0, 1, 1, 1, 100, 5, 7));
    assert(chaon_exec_auth_begin(&a, 1, 1, 1, 1, 1, 100, 5, 7));
    assert(!chaon_exec_auth_confirm(&a, 1, 1, 1, 1, 1, 5, 7, 1, 600, 0));
    assert(a.state == CHAON_AUTH_CONFIRM_WAIT);
    assert(chaon_exec_auth_confirm(&a, 1, 1, 1, 1, 1, 5, 7, 1, 2200, 1600));
    assert(!chaon_exec_auth_confirm(&a, 1, 1, 1, 1, 1, 5, 7, 1, 2400, 1700));
    assert(chaon_exec_auth_begin(&a, 1, 1, 1, 1, 1, 100, 5, 7));
    assert(!chaon_exec_auth_confirm(&a, 1, 1, 1, 1, 1, 5, 8, 1, 650, 0));
    assert(a.state == CHAON_AUTH_IDLE);
    assert(chaon_exec_auth_begin(&a, 1, 1, 1, 1, 1, 100, 5, 7));
    assert(!chaon_exec_auth_confirm(&a, 1, 1, 1, 1, 1, 5, 7, 1, 10601, 0));
    assert(a.state == CHAON_AUTH_IDLE);
    return 0;
}
