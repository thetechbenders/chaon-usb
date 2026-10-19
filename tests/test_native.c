#include "chaon_hid.h"
#include "chaon_script.h"
#include "chaon_vm.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    int reports, presses, releases, fail_on_report;
    uint8_t last_usage, last_mod;
    uint32_t slept_ms;
    bool cancel_now;
    bool cancel_after_first_press;
} mock_t;

static bool send_report(uint8_t modifier, uint8_t usage, void *context)
{
    mock_t *m = context;
    ++m->reports;
    if (m->reports == m->fail_on_report) return false;
    if (usage) ++m->presses;
    else ++m->releases;
    if (usage && m->cancel_after_first_press) m->cancel_now = true;
    m->last_usage = usage;
    m->last_mod = modifier;
    return true;
}
static void sleep_hook(uint32_t ms, void *context)
{
    mock_t *m = context;
    assert(ms >= 1 && ms <= 10);
    m->slept_ms += ms;
}
static bool cancelled(void *context) { return ((mock_t *)context)->cancel_now; }
static chaon_transport_t io_for(mock_t *m)
{
    chaon_transport_t io = {
        .report = send_report, .sleep_ms = sleep_hook,
        .cancelled = cancelled, .context = m
    };
    return io;
}

static void test_parse(void)
{
    chaon_program_t p = {0};
    chaon_parse_error_t err = {0};
    assert(chaon_parse("# comment\r\n DELAY 12\r\nTYPE \"Ab!\"\nENTER\nTAB\n",
                       &p, &err) == CHAON_PARSE_OK);
    assert(p.count == 4 && p.steps[0].delay_ms == 12);
    assert(strcmp(p.steps[1].text, "Ab!") == 0);
    assert(p.steps[2].op == CHAON_OP_ENTER && p.steps[3].op == CHAON_OP_TAB);
    assert(chaon_parse("TYPE \"unterminated", &p, &err) == CHAON_PARSE_SYNTAX);
    assert(err.line == 1 && p.count == 0);
    assert(chaon_parse("TYPE \"bad\\z\"", &p, &err) == CHAON_PARSE_SYNTAX);
    assert(chaon_parse("TYPE \"x\\\"y\"", &p, &err) == CHAON_PARSE_OK);
    assert(strcmp(p.steps[0].text, "x\"y") == 0);
    assert(chaon_parse("DELAY 10001", &p, &err) == CHAON_PARSE_BOUNDS);
    assert(chaon_parse("DELAY 0", &p, &err) == CHAON_PARSE_BOUNDS);
    assert(chaon_parse("DELAY -1", &p, &err) == CHAON_PARSE_SYNTAX);
    assert(chaon_parse("RUN command", &p, &err) == CHAON_PARSE_SYNTAX);
    assert(chaon_parse("# comment\n", &p, &err) == CHAON_PARSE_EMPTY);
    assert(chaon_parse(NULL, &p, &err) == CHAON_PARSE_INVALID_ARGUMENT);
    assert(chaon_parse("", NULL, &err) == CHAON_PARSE_INVALID_ARGUMENT);
    char too_long[CHAON_MAX_SOURCE + 2];
    memset(too_long, 'x', sizeof(too_long));
    too_long[sizeof(too_long) - 1] = '\0';
    assert(chaon_parse(too_long, &p, &err) == CHAON_PARSE_BOUNDS);
}
static void test_ascii(void)
{
    uint8_t mod = 0, key = 0;
    assert(chaon_hid_us_ascii('a', &mod, &key) && key == 4 && mod == 0);
    assert(chaon_hid_us_ascii('Z', &mod, &key) && key == 29 && mod == CHAON_SHIFT);
    assert(chaon_hid_us_ascii('!', &mod, &key) && key == 30 && mod == CHAON_SHIFT);
    assert(chaon_hid_us_ascii(')', &mod, &key) && key == 39 && mod == CHAON_SHIFT);
    assert(!chaon_hid_us_ascii('\n', &mod, &key));
    assert(!chaon_hid_us_ascii('a', NULL, &key));
    for (int i = 32; i <= 126; ++i) {
        assert(chaon_hid_us_ascii((char)i, &mod, &key));
        assert(key != 0);
    }
}
static void test_vm(void)
{
    chaon_program_t p = {0};
    assert(chaon_parse("DELAY 21\nTYPE \"Ab\"\nENTER\n", &p, NULL) == CHAON_PARSE_OK);
    mock_t m = {0};
    chaon_transport_t io = io_for(&m);
    assert(chaon_run(&p, &io) == CHAON_RUN_OK);
    assert(m.presses == 3 && m.releases >= 4);
    assert(m.slept_ms >= 21 + 3 * (24 + 16));
    assert(m.last_mod == 0 && m.last_usage == 0);

    m = (mock_t){.fail_on_report = 1};
    io = io_for(&m);
    assert(chaon_run(&p, &io) == CHAON_RUN_IO_ERROR);
    assert(m.releases >= 1);

    m = (mock_t){.cancel_now = true};
    io = io_for(&m);
    assert(chaon_run(&p, &io) == CHAON_RUN_CANCELLED);
    assert(m.last_usage == 0);

    /* Cancel after a real key-down; final release succeeds. */
    m = (mock_t){.cancel_after_first_press = true};
    io = io_for(&m);
    assert(chaon_run(&p, &io) == CHAON_RUN_CANCELLED);
    assert(m.presses == 1 && m.last_usage == 0 && m.last_mod == 0);

    /* A failed all-keys release cannot be a successful cancellation. */
    m = (mock_t){.cancel_after_first_press = true, .fail_on_report = 2};
    io = io_for(&m);
    assert(chaon_run(&p, &io) == CHAON_RUN_IO_ERROR);
    assert(m.presses == 1 && m.last_usage != 0);
    assert(chaon_run(NULL, &io) == CHAON_RUN_INVALID);
}
int main(void)
{
    test_parse();
    test_ascii();
    test_vm();
    puts("ChaoN portable C tests: PASS");
    return 0;
}
