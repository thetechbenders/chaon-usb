/* SPDX-License-Identifier: AGPL-3.0-only */
#pragma once

static inline void vTaskDelay(int ticks) { (void)ticks; }
static inline int xTaskCreate(void (*function)(void *), const char *name,
    int stack_size, void *parameter, int priority, void *handle)
{
    (void)function;
    (void)name;
    (void)stack_size;
    (void)parameter;
    (void)priority;
    (void)handle;
    return 1;
}
