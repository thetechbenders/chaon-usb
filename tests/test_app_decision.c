/* SPDX-License-Identifier: AGPL-3.0-only */
#include "chaon_app_decision.h"
#include "chaon_control.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static chaon_control_t control;
static chaon_app_decision_t decision;
static chaon_selection_t selected;
static chaon_program_t source_program;
static chaon_program_t output_program;
static int64_t tick;

static chaon_app_action_t drive(bool pressed, bool mounted,
                                bool catalogue, bool pick_ready,
                                bool selected_available)
{
    const chaon_control_event_t event = chaon_control_step_selection(&control,
        mounted, pressed, tick, catalogue, catalogue ? 1 : 0, pick_ready);
    const chaon_app_action_t action = chaon_app_decision_event(&decision,
        event, tick, mounted, selected_available, &selected);
    tick += CHAON_CONTROL_POLL_MS;
    return action;
}

static chaon_app_action_t gesture(unsigned duration_ms,
                                  bool selected_available)
{
    chaon_app_action_t result = CHAON_APP_ACTION_NONE;
    unsigned samples = duration_ms / CHAON_CONTROL_POLL_MS;
    if (samples < 4) samples = 4;
    for (unsigned sample = 0; sample < samples; ++sample) {
        const chaon_app_action_t action = drive(true, true, true, false,
                                                  selected_available);
        if (action != CHAON_APP_ACTION_NONE) result = action;
    }
    for (unsigned sample = 0; sample < 4; ++sample) {
        const chaon_app_action_t action = drive(false, true, true, false,
                                                  selected_available);
        if (action != CHAON_APP_ACTION_NONE) result = action;
    }
    return result;
}

static void reset_fixture(void)
{
    control = (chaon_control_t){0};
    chaon_app_decision_reset(&decision);
    chaon_selection_clear(&selected);
    source_program = (chaon_program_t){0};
    output_program = (chaon_program_t){0};
    tick = 0;
    for (unsigned sample = 0; sample < 4; ++sample)
        assert(drive(false, true, true, false, false) ==
               CHAON_APP_ACTION_NONE);
    assert(control.state == CHAON_STATUS_READY);
}

static void load_selected(void)
{
    chaon_fat_entry_t identity = {0};
    uint8_t digest[CHAON_SOURCE_SHA256_BYTES] = {2};
    memcpy(identity.raw_name, "VALID   CHN", 11);
    memcpy(identity.name, "VALID.CHN", 10);
    identity.first_cluster = 7;
    identity.file_size = 16;
    source_program.count = 1;
    source_program.steps[0].op = CHAON_OP_DELAY;
    source_program.steps[0].delay_ms = 50;
    assert(chaon_selection_load(&selected, &identity, &source_program,
                                17, 4, digest));
}

static void pick_selected(void)
{
    assert(gesture(1020, false) == CHAON_APP_ACTION_NONE);
    assert(control.browser_active);
    assert(gesture(1020, false) == CHAON_APP_ACTION_NONE);
    assert(control.browser_selected);

    chaon_control_event_t event = CHAON_CONTROL_NONE;
    for (unsigned sample = 0; sample < 4; ++sample) {
        event = chaon_control_step_selection(&control, true, false, tick,
            true, 1, true);
        const chaon_app_action_t action = chaon_app_decision_event(&decision,
            event, tick, true, false, &selected);
        assert(action == CHAON_APP_ACTION_NONE);
        tick += CHAON_CONTROL_POLL_MS;
        if (event == CHAON_CONTROL_PICK_READY) break;
    }
    assert(event == CHAON_CONTROL_PICK_READY);
    load_selected();
    chaon_app_decision_pick_result(&decision, true);
    assert(decision.source == CHAON_APP_SOURCE_SELECTED);
}

static void begin_selected_verification(void)
{
    assert(gesture(80, true) == CHAON_APP_ACTION_NONE);
    assert(control.state == CHAON_STATUS_ARMED);
    assert(decision.authority.armed);
    assert(gesture(80, true) == CHAON_APP_ACTION_VERIFY_SELECTED);
    assert(control.state == CHAON_STATUS_RUNNING);
    assert(decision.verification_pending && decision.grant.issued);
}

static void test_success_and_display_truth(void)
{
    reset_fixture();
    pick_selected();
    assert(chaon_app_decision_display(&decision, CHAON_STATUS_READY,
        true, true) == CHAON_APP_DISPLAY_SELECTED);
    assert(chaon_app_decision_display(&decision, CHAON_STATUS_ARMED,
        true, true) == CHAON_APP_DISPLAY_SELECTED);
    begin_selected_verification();
    assert(chaon_app_decision_verification(&decision, CHAON_APP_VERIFY_PASS,
        tick, true, true, false, &selected, &output_program) ==
        CHAON_APP_ACTION_RUN_SELECTED);
    assert(output_program.count == 1 &&
           output_program.steps[0].delay_ms == 50);
    assert(!decision.grant.issued && !decision.slot.ready &&
           !decision.verification_pending);
}

static void test_failed_verification_never_falls_back(void)
{
    reset_fixture();
    pick_selected();
    begin_selected_verification();
    assert(chaon_app_decision_verification(&decision, CHAON_APP_VERIFY_FAIL,
        tick, true, true, false, &selected, &output_program) ==
        CHAON_APP_ACTION_REJECT_SELECTED);
    assert(output_program.count == 0 &&
           decision.source == CHAON_APP_SOURCE_BLOCKED);
    chaon_selection_clear(&selected); /* production invalidates it on reject */
    assert(chaon_app_decision_event(&decision, CHAON_CONTROL_EXECUTE, tick,
        true, false, &selected) == CHAON_APP_ACTION_REJECT_SELECTED);
}

static void test_missing_grant_and_failed_pick(void)
{
    reset_fixture();
    load_selected();
    chaon_app_decision_pick_result(&decision, true);
    assert(chaon_app_decision_event(&decision, CHAON_CONTROL_EXECUTE, tick,
        true, true, &selected) == CHAON_APP_ACTION_REJECT_SELECTED);
    reset_fixture();
    chaon_app_decision_pick_result(&decision, false);
    assert(gesture(80, false) == CHAON_APP_ACTION_NONE);
    assert(gesture(80, false) == CHAON_APP_ACTION_REJECT_SELECTED);
}

static void test_usb_and_sd_revocation(void)
{
    reset_fixture();
    pick_selected();
    assert(gesture(80, true) == CHAON_APP_ACTION_NONE);
    assert(decision.authority.armed);
    assert(drive(false, false, true, false, false) ==
           CHAON_APP_ACTION_NONE);
    assert(!decision.authority.armed && !decision.grant.issued &&
           !decision.verification_pending);

    reset_fixture();
    pick_selected();
    assert(gesture(80, true) == CHAON_APP_ACTION_NONE);
    assert(chaon_app_decision_event(&decision, CHAON_CONTROL_NONE, tick,
        true, false, &selected) == CHAON_APP_ACTION_NONE);
    assert(decision.source == CHAON_APP_SOURCE_BLOCKED);
    assert(gesture(80, false) == CHAON_APP_ACTION_REJECT_SELECTED);
}

static void test_cancel_and_timeout(void)
{
    reset_fixture();
    pick_selected();
    begin_selected_verification();
    assert(chaon_app_decision_verification(&decision, CHAON_APP_VERIFY_WAIT,
        tick, true, true, true, &selected, &output_program) ==
        CHAON_APP_ACTION_REJECT_SELECTED);

    reset_fixture();
    pick_selected();
    begin_selected_verification();
    const int64_t expired = decision.verification_started_ms +
        CHAON_APP_VERIFY_TIMEOUT_MS + 1;
    assert(chaon_app_decision_verification(&decision, CHAON_APP_VERIFY_WAIT,
        expired, true, true, false, &selected, &output_program) ==
        CHAON_APP_ACTION_REJECT_SELECTED);
}

static void test_explicit_no_selection_demo(void)
{
    reset_fixture();
    assert(chaon_app_decision_display(&decision, CHAON_STATUS_READY,
        false, true) == CHAON_APP_DISPLAY_CATALOG);
    assert(chaon_app_decision_display(&decision, CHAON_STATUS_ARMED,
        false, true) == CHAON_APP_DISPLAY_STATUS);
    assert(gesture(80, false) == CHAON_APP_ACTION_NONE);
    assert(gesture(80, false) == CHAON_APP_ACTION_RUN_DEMO);
}

int main(void)
{
    test_success_and_display_truth();
    test_failed_verification_never_falls_back();
    test_missing_grant_and_failed_pick();
    test_usb_and_sd_revocation();
    test_cancel_and_timeout();
    test_explicit_no_selection_demo();
    puts("ChaoN production app authorization decision tests: PASS");
    return 0;
}
