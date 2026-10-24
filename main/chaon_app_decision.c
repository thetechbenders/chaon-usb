/* SPDX-License-Identifier: AGPL-3.0-only */
#include "chaon_app_decision.h"

#include <string.h>

static void revoke(chaon_app_decision_t *decision)
{
    if (!decision) return;
    chaon_run_authority_cancel(&decision->authority);
    chaon_execution_grant_clear(&decision->grant);
    chaon_execution_slot_clear(&decision->slot);
    decision->verification_pending = false;
    decision->verification_started_ms = 0;
}

void chaon_app_decision_reset(chaon_app_decision_t *decision)
{
    if (!decision) return;
    memset(decision, 0, sizeof *decision);
    decision->source = CHAON_APP_SOURCE_DEMO;
}

void chaon_app_decision_pick_result(chaon_app_decision_t *decision,
                                    bool selected_program_loaded)
{
    if (!decision) return;
    revoke(decision);
    decision->source = selected_program_loaded ?
        CHAON_APP_SOURCE_SELECTED : CHAON_APP_SOURCE_BLOCKED;
}

void chaon_app_decision_invalidate_selected(chaon_app_decision_t *decision)
{
    if (!decision) return;
    revoke(decision);
    if (decision->source != CHAON_APP_SOURCE_DEMO)
        decision->source = CHAON_APP_SOURCE_BLOCKED;
}

void chaon_app_decision_cancel_transaction(chaon_app_decision_t *decision)
{
    revoke(decision);
}

chaon_app_action_t chaon_app_decision_event(
    chaon_app_decision_t *decision, chaon_control_event_t event,
    int64_t now_ms, bool usb_mounted, bool selected_source_available,
    const chaon_selection_t *selected)
{
    if (!decision) return CHAON_APP_ACTION_NONE;

    if (!usb_mounted) {
        chaon_app_decision_reset(decision);
        return CHAON_APP_ACTION_NONE;
    }
    if (event == CHAON_CONTROL_BROWSER_ENTER) {
        /* Deliberately entering the browser clears the prior selection. */
        chaon_app_decision_reset(decision);
        return CHAON_APP_ACTION_NONE;
    }
    if (event == CHAON_CONTROL_PICK_READY) {
        /* Fail closed until the caller reports a successful owned copy. */
        revoke(decision);
        decision->source = CHAON_APP_SOURCE_BLOCKED;
    }
    if (decision->source == CHAON_APP_SOURCE_SELECTED &&
        !selected_source_available) {
        chaon_app_decision_invalidate_selected(decision);
    }

    const bool selected_eligible =
        decision->source == CHAON_APP_SOURCE_SELECTED &&
        selected_source_available;
    (void)chaon_run_authority_step(&decision->authority, event, now_ms,
        usb_mounted, selected_eligible, selected, &decision->grant);

    if (event != CHAON_CONTROL_EXECUTE) return CHAON_APP_ACTION_NONE;

    if (decision->source == CHAON_APP_SOURCE_DEMO &&
        (!selected || !selected->valid)) {
        revoke(decision);
        return CHAON_APP_ACTION_RUN_DEMO;
    }
    if (decision->source == CHAON_APP_SOURCE_SELECTED &&
        selected_eligible && decision->grant.issued) {
        decision->verification_pending = true;
        decision->verification_started_ms = now_ms;
        return CHAON_APP_ACTION_VERIFY_SELECTED;
    }

    chaon_app_decision_invalidate_selected(decision);
    return CHAON_APP_ACTION_REJECT_SELECTED;
}

chaon_app_action_t chaon_app_decision_verification(
    chaon_app_decision_t *decision, chaon_app_verify_t verification,
    int64_t now_ms, bool usb_mounted, bool selected_source_available,
    bool cancellation_requested, const chaon_selection_t *selected,
    chaon_program_t *out_program)
{
    if (out_program) memset(out_program, 0, sizeof *out_program);
    if (!decision || !decision->verification_pending)
        return CHAON_APP_ACTION_NONE;

    const bool within_window = now_ms >= decision->verification_started_ms &&
        now_ms - decision->verification_started_ms <=
            CHAON_APP_VERIFY_TIMEOUT_MS;
    const bool eligible = usb_mounted && selected_source_available &&
        !cancellation_requested && within_window && selected &&
        selected->valid && decision->source == CHAON_APP_SOURCE_SELECTED;

    if (verification == CHAON_APP_VERIFY_WAIT && eligible)
        return CHAON_APP_ACTION_NONE;

    decision->verification_pending = false;
    const bool fresh = verification == CHAON_APP_VERIFY_PASS && eligible;
    const bool authorized = fresh &&
        chaon_execution_slot_prepare(&decision->slot, &decision->grant,
            selected, selected->staging_generation, selected->usb_epoch,
            selected->source_sha256, fresh) &&
        chaon_execution_slot_take(&decision->slot, selected,
            selected->staging_generation, selected->usb_epoch,
            selected->source_sha256, fresh, out_program);

    chaon_run_authority_cancel(&decision->authority);
    chaon_execution_grant_clear(&decision->grant);
    chaon_execution_slot_clear(&decision->slot);
    decision->verification_started_ms = 0;
    if (authorized) return CHAON_APP_ACTION_RUN_SELECTED;

    decision->source = CHAON_APP_SOURCE_BLOCKED;
    return CHAON_APP_ACTION_REJECT_SELECTED;
}

chaon_app_display_t chaon_app_decision_display(
    const chaon_app_decision_t *decision, chaon_status_t status,
    bool selected_valid, bool catalogue_available)
{
    if (decision && decision->source == CHAON_APP_SOURCE_SELECTED &&
        selected_valid &&
        (status == CHAON_STATUS_READY || status == CHAON_STATUS_ARMED))
        return CHAON_APP_DISPLAY_SELECTED;
    if (status == CHAON_STATUS_READY && !selected_valid &&
        catalogue_available)
        return CHAON_APP_DISPLAY_CATALOG;
    return CHAON_APP_DISPLAY_STATUS;
}
