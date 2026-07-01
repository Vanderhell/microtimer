/*
 * microtimer - Software timer manager for embedded systems.
 *
 * SPDX-License-Identifier: MIT
 * https://github.com/Vanderhell/microtimer
 */

#ifndef MTIMER_H
#define MTIMER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mtimer_config.h"

#include <stddef.h>
#include <stdint.h>

typedef enum {
    MTIMER_OK = 0,
    MTIMER_ERR_NULL = -1,
    MTIMER_ERR_FULL = -2,
    MTIMER_ERR_NOT_FOUND = -3,
    MTIMER_ERR_INVALID = -4,
    MTIMER_ERR_BUSY = -5,
    MTIMER_ERR_ABI = -6
} mtimer_err_t;

typedef enum {
    MTIMER_ONESHOT = 0,
    MTIMER_PERIODIC = 1
} mtimer_mode_t;

typedef enum {
    MTIMER_STOPPED = 0,
    MTIMER_RUNNING = 1,
    MTIMER_PAUSED = 2
} mtimer_state_t;

typedef uint32_t (*mtimer_clock_fn)(void);
typedef void (*mtimer_cb_fn)(uint8_t timer_id, void *ctx);

typedef struct {
    const char *name;
    uint32_t interval_ms;
    mtimer_cb_fn callback;
    void *ctx;
    uint32_t start_ms;
    uint32_t remaining_ms;
    uint32_t fire_count;
    uint8_t mode_storage;
    uint8_t state_storage;
    uint8_t allocated;
    uint8_t reserved;
} mtimer_entry_t;

typedef struct {
    mtimer_entry_t timers[MTIMER_MAX_TIMERS];
    mtimer_clock_fn clock;
    uint32_t tick_count;
    uint32_t fire_count;
    uint8_t active_tick;
    uint8_t reserved0;
    uint8_t reserved1;
    uint8_t reserved2;
} mtimer_t;

const char *mtimer_err_str(mtimer_err_t err);
const char *mtimer_state_str(mtimer_state_t state);

mtimer_err_t mtimer_init_sized(mtimer_t *tm, size_t tm_size, mtimer_clock_fn clock);

static inline mtimer_err_t mtimer_init(mtimer_t *tm, mtimer_clock_fn clock)
{
    mtimer_t *tm_once = tm;
    return mtimer_init_sized(tm_once, sizeof(*tm_once), clock);
}

int mtimer_create(
    mtimer_t *tm,
    const char *name,
    uint32_t interval_ms,
    mtimer_mode_t mode,
    mtimer_cb_fn callback,
    void *ctx);

mtimer_err_t mtimer_destroy(mtimer_t *tm, uint8_t id);
mtimer_err_t mtimer_start(mtimer_t *tm, uint8_t id);
mtimer_err_t mtimer_stop(mtimer_t *tm, uint8_t id);
mtimer_err_t mtimer_pause(mtimer_t *tm, uint8_t id);
mtimer_err_t mtimer_resume(mtimer_t *tm, uint8_t id);
mtimer_err_t mtimer_set_interval(mtimer_t *tm, uint8_t id, uint32_t interval_ms);

int mtimer_tick(mtimer_t *tm);

mtimer_err_t mtimer_get_count(const mtimer_t *tm, uint8_t *out_count);
const mtimer_entry_t *mtimer_at(const mtimer_t *tm, uint8_t id);
int mtimer_find(const mtimer_t *tm, const char *name);
mtimer_err_t mtimer_get_state(const mtimer_t *tm, uint8_t id, mtimer_state_t *out_state);
mtimer_err_t mtimer_get_remaining(const mtimer_t *tm, uint8_t id, uint32_t *out_remaining_ms);
mtimer_err_t mtimer_get_fire_count(const mtimer_t *tm, uint8_t id, uint32_t *out_fire_count);
mtimer_err_t mtimer_get_total_fires(const mtimer_t *tm, uint32_t *out_total_fires);
mtimer_err_t mtimer_get_total_ticks(const mtimer_t *tm, uint32_t *out_total_ticks);

#ifdef __cplusplus
}
#endif

#endif /* MTIMER_H */
