/* SPDX-License-Identifier: AGPL-3.0-only */
#ifndef CHAON_APP_DECISION_H
#define CHAON_APP_DECISION_H

#include <stdbool.h>
#include <stdint.h>

#include "chaon_run_authority.h"

enum { CHAON_APP_VERIFY_TIMEOUT_MS = 5000 };

typedef enum {
    CHAON_APP_SOURCE_DEMO,
    CHAON_APP_SOURCE_SELECTED,
    CHAON_APP_SOURCE_BLOCKED
} chaon_app_source_t;

typedef enum {
    CHAON_APP_ACTION_NONE,
    CHAON_APP_ACTION_RUN_DEMO,
    CHAON_APP_ACTION_VERIFY_SELECTED,
    CHAON_APP_ACTION_RUN_SELECTED,
    CHAON_APP_ACTION_REJECT_SELECTED
} chaon_app_action_t;

typedef enum {
    CHAON_APP_VERIFY_WAIT,
    CHAON_APP_VERIFY_PASS,
    CHAON_APP_VERIFY_FAIL
} chaon_app_verify_t;

typedef enum {
    CHAON_APP_DISPLAY_STATUS,
    CHAON_APP_DISPLAY_CATALOG,
    CHAON_APP_DISPLAY_SELECTED
} chaon_app_display_t;

/* Static application-control ownership. Keep this object outside the
 * app_main task stack: it contains the bounded execution program slot. */
typedef struct {
    chaon_app_source_t source;
    chaon_run_authority_t authority;
    chaon_execution_grant_t grant;
    chaon_execution_slot_t slot;
    bool verification_pending;
    int64_t verification_started_ms;
} chaon_app_decision_t;

void chaon_app_decision_reset(chaon_app_decision_t *decision);
void chaon_app_decision_pick_result(chaon_app_decision_t *decision,
                                    bool selected_program_loaded);
void chaon_app_decision_invalidate_selected(chaon_app_decision_t *decision);
void chaon_app_decision_cancel_transaction(chaon_app_decision_t *decision);

/* Consume the real debounced controller event. PICK loading is reported
 * separately with chaon_app_decision_pick_result after the owned copy is made. */
chaon_app_action_t chaon_app_decision_event(
    chaon_app_decision_t *decision, chaon_control_event_t event,
    int64_t now_ms, bool usb_mounted, bool selected_source_available,
    const chaon_selection_t *selected);

/* Complete a pending SD reread. PASS is the actual SD worker freshness result.
 * The same result is supplied to both slot preparation and single-use take. */
chaon_app_action_t chaon_app_decision_verification(
    chaon_app_decision_t *decision, chaon_app_verify_t verification,
    int64_t now_ms, bool usb_mounted, bool selected_source_available,
    bool cancellation_requested, const chaon_selection_t *selected,
    chaon_program_t *out_program);

/* UI policy used by production: a selected READY/ARMED identity has priority
 * over catalogue rotation. With no selection, the existing READY preview stays. */
chaon_app_display_t chaon_app_decision_display(
    const chaon_app_decision_t *decision, chaon_status_t status,
    bool selected_valid, bool catalogue_available);

#endif
