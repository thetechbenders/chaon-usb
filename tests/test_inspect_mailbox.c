/* SPDX-License-Identifier: AGPL-3.0-only */
#include "chaon_inspect_mailbox.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static chaon_fat_catalog_t catalog(void)
{
    chaon_fat_catalog_t c = {0};
    c.count = 2;
    strcpy(c.entries[0].name, "TEST.CHN");
    memcpy(c.entries[0].raw_name, "TEST    CHN", 11);
    c.entries[0].first_cluster = 3;
    c.entries[0].file_size = 25;
    strcpy(c.entries[1].name, "OTHER.TXT");
    memcpy(c.entries[1].raw_name, "OTHER   TXT", 11);
    c.entries[1].first_cluster = 5;
    c.entries[1].file_size = 20;
    return c;
}
static void test_request_complete_and_invalidate(void)
{
    chaon_inspect_mailbox_t box = {0};
    chaon_fat_catalog_t c = catalog();
    chaon_inspection_t result = {0};
    uint32_t gen = 0;
    uint8_t index = 255;
    chaon_fat_entry_t identity = {0};
    assert(!chaon_inspect_mailbox_snapshot(&box, 0, &c.entries[0], &result));
    assert(chaon_inspect_mailbox_request(&box, &c, 0, &c.entries[0]));
    assert(chaon_inspect_mailbox_snapshot(&box, 0, &c.entries[0], &result));
    assert(result.state == CHAON_INSPECT_PENDING);
    assert(!chaon_inspect_mailbox_snapshot(&box, 1, &c.entries[1], &result));
    assert(chaon_inspect_mailbox_take(&box, &gen, &index, &identity));
    assert(index == 0);
    assert(!chaon_inspect_mailbox_take(&box, &gen, &index, &identity));
    result = (chaon_inspection_t){
        .state=CHAON_INSPECT_VALID, .step_count=3,
        .first_op=CHAON_INSPECT_OP_DELAY };
    chaon_inspect_mailbox_publish(&box, gen, index, result);
    chaon_inspection_t saved = {0};
    assert(chaon_inspect_mailbox_snapshot(&box, 0, &identity, &saved));
    assert(saved.state == CHAON_INSPECT_VALID && saved.step_count == 3);
    assert(saved.first_op == CHAON_INSPECT_OP_DELAY);
    identity.file_size++;
    assert(!chaon_inspect_mailbox_snapshot(&box, 0, &identity, &saved));
    chaon_inspect_mailbox_clear(&box);
    assert(!chaon_inspect_mailbox_snapshot(&box, 0, &c.entries[0], &saved));
}
static void test_cancel_and_replace_race(void)
{
    chaon_inspect_mailbox_t box = {0};
    chaon_fat_catalog_t c = catalog();
    uint32_t old = 0, current = 0;
    uint8_t oi = 0, ni = 0;
    chaon_fat_entry_t a = {0}, b = {0};
    const chaon_inspection_t success =
        { .state=CHAON_INSPECT_VALID, .step_count=5 };
    assert(chaon_inspect_mailbox_request(&box, &c, 0, &c.entries[0]));
    assert(chaon_inspect_mailbox_take(&box, &old, &oi, &a));
    assert(chaon_inspect_mailbox_request(&box, &c, 1, &c.entries[1]));
    assert(chaon_inspect_mailbox_take(&box, &current, &ni, &b));
    assert(current != old && ni == 1);
    chaon_inspect_mailbox_publish(&box, old, oi, success);
    chaon_inspection_t out = {0};
    assert(!chaon_inspect_mailbox_snapshot(&box, 0, &a, &out));
    assert(chaon_inspect_mailbox_snapshot(&box, 1, &b, &out));
    assert(out.state == CHAON_INSPECT_PENDING);
    chaon_inspect_mailbox_publish(&box, current, ni, success);
    assert(chaon_inspect_mailbox_snapshot(&box, 1, &b, &out));
    assert(out.state == CHAON_INSPECT_VALID);
    assert(chaon_inspect_mailbox_request(&box, &c, 1, &c.entries[1]));
    assert(chaon_inspect_mailbox_take(&box, &current, &ni, &b));
    chaon_inspect_mailbox_clear(&box); /* card removed mid-read */
    chaon_inspect_mailbox_publish(&box, current, ni, success);
    assert(!chaon_inspect_mailbox_snapshot(&box, 1, &b, &out));
}
static void test_malformed_requests(void)
{
    chaon_inspect_mailbox_t box = {0};
    chaon_fat_catalog_t c = catalog();
    chaon_fat_entry_t wrong = c.entries[0];
    wrong.first_cluster++;
    assert(!chaon_inspect_mailbox_request(&box, &c, 0, &wrong));
    wrong=c.entries[0];
    wrong.raw_name[0]='X';
    assert(!chaon_inspect_mailbox_request(&box, &c, 0, &wrong));
    wrong=c.entries[0];
    wrong.is_directory=true;
    assert(!chaon_inspect_mailbox_request(&box, &c, 0, &wrong));
    assert(!chaon_inspect_mailbox_request(&box, &c, 2, &c.entries[0]));
    assert(!chaon_inspect_mailbox_request(&box, &c, 8, &c.entries[0]));
    c.count=9;
    assert(!chaon_inspect_mailbox_request(&box, &c, 0, &wrong));
    assert(!chaon_inspect_mailbox_request(NULL, &c, 0, &wrong));
    chaon_inspect_mailbox_clear(NULL);
    assert(!chaon_inspect_mailbox_take(NULL, NULL, NULL, NULL));
}
int main(void)
{
    test_request_complete_and_invalidate();
    test_cancel_and_replace_race();
    test_malformed_requests();
    puts("ChaoN read-only inspection mailbox state tests: PASS");
    return 0;
}
