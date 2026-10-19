#include "chaon_status.h"

chaon_rgb_t chaon_status_led(chaon_status_t status, uint32_t now_ms)
{
    switch (status) {
        case CHAON_STATUS_WAIT_USB:
            return (chaon_rgb_t){24, 33, 12}; /* dim ChaoN green */
        case CHAON_STATUS_READY:
            return (chaon_rgb_t){180, 246, 107}; /* #B4F66B */
        case CHAON_STATUS_ARMED:
            return ((now_ms / 250U) % 2U == 0U)
                ? (chaon_rgb_t){255, 196, 0}
                : (chaon_rgb_t){0, 0, 0};
        case CHAON_STATUS_RUNNING:
            return (chaon_rgb_t){0, 210, 230}; /* distinct from ready */
        case CHAON_STATUS_CANCELLED:
            return (chaon_rgb_t){255, 0, 0};
        case CHAON_STATUS_FAULT:
            return ((now_ms / 125U) % 2U == 0U)
                ? (chaon_rgb_t){255, 0, 0}
                : (chaon_rgb_t){0, 0, 0};
        default:
            return (chaon_rgb_t){255, 0, 0};
    }
}

const char *chaon_status_name(chaon_status_t status)
{
    switch (status) {
        case CHAON_STATUS_WAIT_USB: return "USB OFFLINE";
        case CHAON_STATUS_READY: return "READY";
        case CHAON_STATUS_ARMED: return "ARMED";
        case CHAON_STATUS_RUNNING: return "RUNNING";
        case CHAON_STATUS_CANCELLED: return "CANCELLED";
        case CHAON_STATUS_FAULT: return "FAULT";
        default: return "FAULT";
    }
}

const char *chaon_status_hint(chaon_status_t status)
{
    switch (status) {
        case CHAON_STATUS_WAIT_USB: return "CONNECT USB";
        case CHAON_STATUS_READY: return "BOOT TO ARM";
        case CHAON_STATUS_ARMED: return "BOOT TO RUN";
        case CHAON_STATUS_RUNNING: return "BOOT TO STOP";
        case CHAON_STATUS_CANCELLED: return "KEYS RELEASED";
        case CHAON_STATUS_FAULT: return "CHECK LOGS";
        default: return "CHECK LOGS";
    }
}
