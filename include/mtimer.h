/*
 * microtimer — Software timer manager for embedded systems.
 *
 * Register oneshot and periodic timers, tick from SysTick or main loop.
 * Replaces scattered `if (now - last > interval)` patterns.
 *
 * C99 · Zero dependencies · Zero allocations · Portable
 *
 * SPDX-License-Identifier: MIT
 * https://github.com/Vanderhell/microtimer
 */

#ifndef MTIMER_H
#define MTIMER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ── Configuration ─────────────────────────────────────────────────────── */

#ifndef MTIMER_MAX_TIMERS
#define MTIMER_MAX_TIMERS 8
#endif

/* ── Error codes ───────────────────────────────────────────────────────── */

typedef enum {
    MTIMER_OK            =  0,
    MTIMER_ERR_NULL      = -1,
    MTIMER_ERR_FULL      = -2,
    MTIMER_ERR_NOT_FOUND = -3,
    MTIMER_ERR_INVALID   = -4,
} mtimer_err_t;

const char *mtimer_err_str(mtimer_err_t err);

/* ── Timer mode ────────────────────────────────────────────────────────── */

typedef enum {
    MTIMER_ONESHOT  = 0,   /**< Fires once, then auto-stops.              */
    MTIMER_PERIODIC = 1,   /**< Fires repeatedly at interval.             */
} mtimer_mode_t;

/* ── Timer state ───────────────────────────────────────────────────────── */

typedef enum {
    MTIMER_STOPPED = 0,
    MTIMER_RUNNING = 1,
    MTIMER_PAUSED  = 2,
} mtimer_state_t;

const char *mtimer_state_str(mtimer_state_t state);

/* ── Platform callback ─────────────────────────────────────────────────── */

typedef uint32_t (*mtimer_clock_fn)(void);

/* ── Timer callback ────────────────────────────────────────────────────── */

/**
 * Timer expiry callback.
 *
 * @param timer_id  Timer index.
 * @param ctx       User context.
 */
typedef void (*mtimer_cb_fn)(uint8_t timer_id, void *ctx);

/* ── Timer descriptor ──────────────────────────────────────────────────── */

typedef struct {
    const char     *name;          /**< Timer name (static string).        */
    uint32_t        interval_ms;   /**< Timer interval / delay.            */
    mtimer_mode_t   mode;          /**< ONESHOT or PERIODIC.               */
    mtimer_cb_fn    callback;      /**< Expiry callback.                   */
    void           *ctx;           /**< User context for callback.         */

    /* Runtime state */
    mtimer_state_t  state;
    uint32_t        start_ms;      /**< When timer was (re)started.        */
    uint32_t        remaining_ms;  /**< For pause/resume.                  */
    uint32_t        fire_count;    /**< Times this timer has fired.        */
    bool            allocated;     /**< Slot in use?                       */
} mtimer_entry_t;

/* ── Timer manager instance ────────────────────────────────────────────── */

typedef struct {
    mtimer_entry_t   timers[MTIMER_MAX_TIMERS];
    mtimer_clock_fn  clock;
    uint32_t         tick_count;    /**< Total tick() calls.               */
    uint32_t         fire_count;    /**< Total callbacks fired.            */
} mtimer_t;

/* ── Init ──────────────────────────────────────────────────────────────── */

mtimer_err_t mtimer_init(mtimer_t *tm, mtimer_clock_fn clock);

/* ── Create / Destroy ──────────────────────────────────────────────────── */

/**
 * Create a timer.
 *
 * @param tm           Timer manager.
 * @param name         Timer name (static/const, for debug).
 * @param interval_ms  Interval or delay in ms.
 * @param mode         MTIMER_ONESHOT or MTIMER_PERIODIC.
 * @param callback     Expiry callback.
 * @param ctx          User context.
 * @return Timer ID (0-based) or negative error.
 */
int mtimer_create(mtimer_t *tm, const char *name, uint32_t interval_ms,
                   mtimer_mode_t mode, mtimer_cb_fn callback, void *ctx);

/**
 * Destroy a timer (free the slot).
 */
mtimer_err_t mtimer_destroy(mtimer_t *tm, uint8_t id);

/* ── Control ───────────────────────────────────────────────────────────── */

/** Start (or restart) a timer. */
mtimer_err_t mtimer_start(mtimer_t *tm, uint8_t id);

/** Stop a timer. */
mtimer_err_t mtimer_stop(mtimer_t *tm, uint8_t id);

/** Pause a running timer (saves remaining time). */
mtimer_err_t mtimer_pause(mtimer_t *tm, uint8_t id);

/** Resume a paused timer. */
mtimer_err_t mtimer_resume(mtimer_t *tm, uint8_t id);

/** Restart with a new interval. */
mtimer_err_t mtimer_set_interval(mtimer_t *tm, uint8_t id, uint32_t interval_ms);

/* ── Tick ──────────────────────────────────────────────────────────────── */

/**
 * Process all timers. Call from main loop or periodic interrupt.
 *
 * Checks each running timer against the clock. Fires callbacks for
 * expired timers. Periodic timers auto-restart. Oneshot timers stop.
 *
 * @return Number of timers that fired this tick.
 */
int mtimer_tick(mtimer_t *tm);

/* ── Query ─────────────────────────────────────────────────────────────── */

uint8_t mtimer_count(const mtimer_t *tm);
const mtimer_entry_t *mtimer_at(const mtimer_t *tm, uint8_t id);
int mtimer_find(const mtimer_t *tm, const char *name);
mtimer_state_t mtimer_state(const mtimer_t *tm, uint8_t id);
uint32_t mtimer_remaining(const mtimer_t *tm, uint8_t id);
uint32_t mtimer_fire_count(const mtimer_t *tm, uint8_t id);
uint32_t mtimer_total_fires(const mtimer_t *tm);
uint32_t mtimer_total_ticks(const mtimer_t *tm);

#ifdef __cplusplus
}
#endif

#endif /* MTIMER_H */
