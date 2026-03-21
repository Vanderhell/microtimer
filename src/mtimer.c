/*
 * microtimer — Implementation.
 *
 * SPDX-License-Identifier: MIT
 * https://github.com/Vanderhell/microtimer
 */

#include "mtimer.h"
#include <string.h>

/* ── Strings ───────────────────────────────────────────────────────────── */

const char *mtimer_err_str(mtimer_err_t err)
{
    switch (err) {
    case MTIMER_OK:            return "ok";
    case MTIMER_ERR_NULL:      return "null pointer";
    case MTIMER_ERR_FULL:      return "timer table full";
    case MTIMER_ERR_NOT_FOUND: return "timer not found";
    case MTIMER_ERR_INVALID:   return "invalid config";
    default:                   return "unknown error";
    }
}

const char *mtimer_state_str(mtimer_state_t state)
{
    switch (state) {
    case MTIMER_STOPPED: return "STOPPED";
    case MTIMER_RUNNING: return "RUNNING";
    case MTIMER_PAUSED:  return "PAUSED";
    default:             return "?";
    }
}

/* ── Helpers ───────────────────────────────────────────────────────────── */

static inline uint32_t elapsed(uint32_t from, uint32_t now)
{
    return now - from;
}

static bool valid_id(const mtimer_t *tm, uint8_t id)
{
    return id < MTIMER_MAX_TIMERS && tm->timers[id].allocated;
}

/* ── Init ──────────────────────────────────────────────────────────────── */

mtimer_err_t mtimer_init(mtimer_t *tm, mtimer_clock_fn clock)
{
    if (tm == NULL || clock == NULL) return MTIMER_ERR_NULL;
    memset(tm, 0, sizeof(*tm));
    tm->clock = clock;
    return MTIMER_OK;
}

/* ── Create / Destroy ──────────────────────────────────────────────────── */

int mtimer_create(mtimer_t *tm, const char *name, uint32_t interval_ms,
                   mtimer_mode_t mode, mtimer_cb_fn callback, void *ctx)
{
    if (tm == NULL || callback == NULL) return MTIMER_ERR_NULL;
    if (interval_ms == 0) return MTIMER_ERR_INVALID;

    /* Find free slot */
    int idx = -1;
    for (int i = 0; i < MTIMER_MAX_TIMERS; i++) {
        if (!tm->timers[i].allocated) {
            idx = i;
            break;
        }
    }
    if (idx < 0) return MTIMER_ERR_FULL;

    mtimer_entry_t *t = &tm->timers[idx];
    memset(t, 0, sizeof(*t));
    t->name        = name;
    t->interval_ms = interval_ms;
    t->mode        = mode;
    t->callback    = callback;
    t->ctx         = ctx;
    t->state       = MTIMER_STOPPED;
    t->allocated   = true;

    return idx;
}

mtimer_err_t mtimer_destroy(mtimer_t *tm, uint8_t id)
{
    if (tm == NULL) return MTIMER_ERR_NULL;
    if (!valid_id(tm, id)) return MTIMER_ERR_NOT_FOUND;

    memset(&tm->timers[id], 0, sizeof(mtimer_entry_t));
    return MTIMER_OK;
}

/* ── Control ───────────────────────────────────────────────────────────── */

mtimer_err_t mtimer_start(mtimer_t *tm, uint8_t id)
{
    if (tm == NULL) return MTIMER_ERR_NULL;
    if (!valid_id(tm, id)) return MTIMER_ERR_NOT_FOUND;

    mtimer_entry_t *t = &tm->timers[id];
    t->state    = MTIMER_RUNNING;
    t->start_ms = tm->clock();

    return MTIMER_OK;
}

mtimer_err_t mtimer_stop(mtimer_t *tm, uint8_t id)
{
    if (tm == NULL) return MTIMER_ERR_NULL;
    if (!valid_id(tm, id)) return MTIMER_ERR_NOT_FOUND;

    tm->timers[id].state = MTIMER_STOPPED;
    return MTIMER_OK;
}

mtimer_err_t mtimer_pause(mtimer_t *tm, uint8_t id)
{
    if (tm == NULL) return MTIMER_ERR_NULL;
    if (!valid_id(tm, id)) return MTIMER_ERR_NOT_FOUND;

    mtimer_entry_t *t = &tm->timers[id];
    if (t->state != MTIMER_RUNNING) return MTIMER_ERR_INVALID;

    uint32_t el = elapsed(t->start_ms, tm->clock());
    if (el >= t->interval_ms) {
        t->remaining_ms = 0;
    } else {
        t->remaining_ms = t->interval_ms - el;
    }
    t->state = MTIMER_PAUSED;

    return MTIMER_OK;
}

mtimer_err_t mtimer_resume(mtimer_t *tm, uint8_t id)
{
    if (tm == NULL) return MTIMER_ERR_NULL;
    if (!valid_id(tm, id)) return MTIMER_ERR_NOT_FOUND;

    mtimer_entry_t *t = &tm->timers[id];
    if (t->state != MTIMER_PAUSED) return MTIMER_ERR_INVALID;

    /* Set start_ms so that (now - start_ms) = (interval - remaining) */
    uint32_t now = tm->clock();
    t->start_ms = now - (t->interval_ms - t->remaining_ms);
    t->state = MTIMER_RUNNING;

    return MTIMER_OK;
}

mtimer_err_t mtimer_set_interval(mtimer_t *tm, uint8_t id, uint32_t interval_ms)
{
    if (tm == NULL) return MTIMER_ERR_NULL;
    if (!valid_id(tm, id)) return MTIMER_ERR_NOT_FOUND;
    if (interval_ms == 0) return MTIMER_ERR_INVALID;

    mtimer_entry_t *t = &tm->timers[id];
    t->interval_ms = interval_ms;

    /* Restart if running */
    if (t->state == MTIMER_RUNNING) {
        t->start_ms = tm->clock();
    }

    return MTIMER_OK;
}

/* ── Tick ──────────────────────────────────────────────────────────────── */

int mtimer_tick(mtimer_t *tm)
{
    if (tm == NULL || tm->clock == NULL) return 0;

    uint32_t now = tm->clock();
    int fired = 0;

    for (int i = 0; i < MTIMER_MAX_TIMERS; i++) {
        mtimer_entry_t *t = &tm->timers[i];
        if (!t->allocated || t->state != MTIMER_RUNNING) continue;

        uint32_t el = elapsed(t->start_ms, now);
        if (el < t->interval_ms) continue;

        /* Timer expired! */
        t->fire_count++;
        tm->fire_count++;
        fired++;

        t->callback((uint8_t)i, t->ctx);

        /* Oneshot → stop, Periodic → restart */
        if (t->mode == MTIMER_ONESHOT) {
            t->state = MTIMER_STOPPED;
        } else {
            /* Anchor next period to avoid drift */
            t->start_ms += t->interval_ms;
            /* If we're way behind, snap to now */
            if (elapsed(t->start_ms, now) >= t->interval_ms) {
                t->start_ms = now;
            }
        }
    }

    tm->tick_count++;
    return fired;
}

/* ── Query ─────────────────────────────────────────────────────────────── */

uint8_t mtimer_count(const mtimer_t *tm)
{
    if (tm == NULL) return 0;
    uint8_t n = 0;
    for (int i = 0; i < MTIMER_MAX_TIMERS; i++) {
        if (tm->timers[i].allocated) n++;
    }
    return n;
}

const mtimer_entry_t *mtimer_at(const mtimer_t *tm, uint8_t id)
{
    if (tm == NULL || !valid_id(tm, id)) return NULL;
    return &tm->timers[id];
}

int mtimer_find(const mtimer_t *tm, const char *name)
{
    if (tm == NULL || name == NULL) return -1;
    for (int i = 0; i < MTIMER_MAX_TIMERS; i++) {
        if (tm->timers[i].allocated && tm->timers[i].name != NULL &&
            strcmp(tm->timers[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

mtimer_state_t mtimer_state(const mtimer_t *tm, uint8_t id)
{
    if (tm == NULL || !valid_id(tm, id)) return MTIMER_STOPPED;
    return tm->timers[id].state;
}

uint32_t mtimer_remaining(const mtimer_t *tm, uint8_t id)
{
    if (tm == NULL || !valid_id(tm, id)) return 0;
    const mtimer_entry_t *t = &tm->timers[id];

    if (t->state == MTIMER_PAUSED) return t->remaining_ms;
    if (t->state != MTIMER_RUNNING) return 0;

    uint32_t el = elapsed(t->start_ms, tm->clock());
    if (el >= t->interval_ms) return 0;
    return t->interval_ms - el;
}

uint32_t mtimer_fire_count(const mtimer_t *tm, uint8_t id)
{
    if (tm == NULL || !valid_id(tm, id)) return 0;
    return tm->timers[id].fire_count;
}

uint32_t mtimer_total_fires(const mtimer_t *tm)
{
    if (tm == NULL) return 0;
    return tm->fire_count;
}

uint32_t mtimer_total_ticks(const mtimer_t *tm)
{
    if (tm == NULL) return 0;
    return tm->tick_count;
}
