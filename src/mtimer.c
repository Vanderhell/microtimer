/*
 * microtimer - Implementation.
 *
 * SPDX-License-Identifier: MIT
 * https://github.com/Vanderhell/microtimer
 */

#include "mtimer.h"

#include <string.h>

static uint32_t mtimer_elapsed(uint32_t from, uint32_t now)
{
    return now - from;
}

static int mtimer_mode_is_valid(mtimer_mode_t mode)
{
    return mode == MTIMER_ONESHOT || mode == MTIMER_PERIODIC;
}

static int mtimer_state_is_valid_storage(uint8_t state)
{
    return state == (uint8_t)MTIMER_STOPPED ||
           state == (uint8_t)MTIMER_RUNNING ||
           state == (uint8_t)MTIMER_PAUSED;
}

static mtimer_state_t mtimer_state_from_storage(uint8_t state)
{
    if (state == (uint8_t)MTIMER_RUNNING) {
        return MTIMER_RUNNING;
    }
    if (state == (uint8_t)MTIMER_PAUSED) {
        return MTIMER_PAUSED;
    }
    return MTIMER_STOPPED;
}

static int mtimer_valid_id(const mtimer_t *tm, uint8_t id)
{
    return id < MTIMER_MAX_TIMERS && tm->timers[id].allocated != 0u;
}

static int mtimer_manager_busy(const mtimer_t *tm)
{
    return tm->active_tick != 0u;
}

static int mtimer_name_exists(const mtimer_t *tm, const char *name)
{
    int i;

    if (name == NULL) {
        return 0;
    }

    for (i = 0; i < MTIMER_MAX_TIMERS; ++i) {
        if (tm->timers[i].allocated != 0u &&
            tm->timers[i].name != NULL &&
            strcmp(tm->timers[i].name, name) == 0) {
            return 1;
        }
    }

    return 0;
}

const char *mtimer_err_str(mtimer_err_t err)
{
    switch (err) {
    case MTIMER_OK:
        return "ok";
    case MTIMER_ERR_NULL:
        return "null pointer";
    case MTIMER_ERR_FULL:
        return "timer table full";
    case MTIMER_ERR_NOT_FOUND:
        return "timer not found";
    case MTIMER_ERR_INVALID:
        return "invalid argument";
    case MTIMER_ERR_BUSY:
        return "manager busy";
    case MTIMER_ERR_ABI:
        return "abi/config mismatch";
    default:
        return "unknown error";
    }
}

const char *mtimer_state_str(mtimer_state_t state)
{
    switch (state) {
    case MTIMER_STOPPED:
        return "STOPPED";
    case MTIMER_RUNNING:
        return "RUNNING";
    case MTIMER_PAUSED:
        return "PAUSED";
    default:
        return "?";
    }
}

mtimer_err_t mtimer_init_sized(mtimer_t *tm, size_t tm_size, mtimer_clock_fn clock)
{
    if (tm == NULL || clock == NULL) {
        return MTIMER_ERR_NULL;
    }
    if (tm_size != sizeof(*tm)) {
        return MTIMER_ERR_ABI;
    }

    memset(tm, 0, sizeof(*tm));
    tm->clock = clock;
    return MTIMER_OK;
}

int mtimer_create(
    mtimer_t *tm,
    const char *name,
    uint32_t interval_ms,
    mtimer_mode_t mode,
    mtimer_cb_fn callback,
    void *ctx)
{
    int i;

    if (tm == NULL || callback == NULL) {
        return MTIMER_ERR_NULL;
    }
    if (mtimer_manager_busy(tm)) {
        return MTIMER_ERR_BUSY;
    }
    if (interval_ms == 0u || !mtimer_mode_is_valid(mode)) {
        return MTIMER_ERR_INVALID;
    }
    if (mtimer_name_exists(tm, name)) {
        return MTIMER_ERR_INVALID;
    }

    for (i = 0; i < MTIMER_MAX_TIMERS; ++i) {
        mtimer_entry_t *timer;

        if (tm->timers[i].allocated != 0u) {
            continue;
        }

        timer = &tm->timers[i];
        memset(timer, 0, sizeof(*timer));
        timer->name = name;
        timer->interval_ms = interval_ms;
        timer->callback = callback;
        timer->ctx = ctx;
        timer->mode_storage = (uint8_t)mode;
        timer->state_storage = (uint8_t)MTIMER_STOPPED;
        timer->allocated = 1u;
        return i;
    }

    return MTIMER_ERR_FULL;
}

mtimer_err_t mtimer_destroy(mtimer_t *tm, uint8_t id)
{
    if (tm == NULL) {
        return MTIMER_ERR_NULL;
    }
    if (mtimer_manager_busy(tm)) {
        return MTIMER_ERR_BUSY;
    }
    if (!mtimer_valid_id(tm, id)) {
        return MTIMER_ERR_NOT_FOUND;
    }

    memset(&tm->timers[id], 0, sizeof(tm->timers[id]));
    return MTIMER_OK;
}

mtimer_err_t mtimer_start(mtimer_t *tm, uint8_t id)
{
    mtimer_entry_t *timer;

    if (tm == NULL) {
        return MTIMER_ERR_NULL;
    }
    if (mtimer_manager_busy(tm)) {
        return MTIMER_ERR_BUSY;
    }
    if (!mtimer_valid_id(tm, id)) {
        return MTIMER_ERR_NOT_FOUND;
    }

    timer = &tm->timers[id];
    timer->start_ms = tm->clock();
    timer->remaining_ms = 0u;
    timer->state_storage = (uint8_t)MTIMER_RUNNING;
    return MTIMER_OK;
}

mtimer_err_t mtimer_stop(mtimer_t *tm, uint8_t id)
{
    mtimer_entry_t *timer;

    if (tm == NULL) {
        return MTIMER_ERR_NULL;
    }
    if (mtimer_manager_busy(tm)) {
        return MTIMER_ERR_BUSY;
    }
    if (!mtimer_valid_id(tm, id)) {
        return MTIMER_ERR_NOT_FOUND;
    }

    timer = &tm->timers[id];
    timer->state_storage = (uint8_t)MTIMER_STOPPED;
    timer->remaining_ms = 0u;
    return MTIMER_OK;
}

mtimer_err_t mtimer_pause(mtimer_t *tm, uint8_t id)
{
    mtimer_entry_t *timer;
    uint32_t elapsed_ms;

    if (tm == NULL) {
        return MTIMER_ERR_NULL;
    }
    if (mtimer_manager_busy(tm)) {
        return MTIMER_ERR_BUSY;
    }
    if (!mtimer_valid_id(tm, id)) {
        return MTIMER_ERR_NOT_FOUND;
    }

    timer = &tm->timers[id];
    if (timer->state_storage != (uint8_t)MTIMER_RUNNING) {
        return MTIMER_ERR_INVALID;
    }

    elapsed_ms = mtimer_elapsed(timer->start_ms, tm->clock());
    if (elapsed_ms >= timer->interval_ms) {
        timer->remaining_ms = 0u;
    } else {
        timer->remaining_ms = timer->interval_ms - elapsed_ms;
    }
    timer->state_storage = (uint8_t)MTIMER_PAUSED;
    return MTIMER_OK;
}

mtimer_err_t mtimer_resume(mtimer_t *tm, uint8_t id)
{
    mtimer_entry_t *timer;
    uint32_t now;

    if (tm == NULL) {
        return MTIMER_ERR_NULL;
    }
    if (mtimer_manager_busy(tm)) {
        return MTIMER_ERR_BUSY;
    }
    if (!mtimer_valid_id(tm, id)) {
        return MTIMER_ERR_NOT_FOUND;
    }

    timer = &tm->timers[id];
    if (timer->state_storage != (uint8_t)MTIMER_PAUSED) {
        return MTIMER_ERR_INVALID;
    }

    now = tm->clock();
    if (timer->remaining_ms == 0u) {
        timer->start_ms = now - timer->interval_ms;
    } else {
        timer->start_ms = now - (timer->interval_ms - timer->remaining_ms);
    }
    timer->state_storage = (uint8_t)MTIMER_RUNNING;
    return MTIMER_OK;
}

mtimer_err_t mtimer_set_interval(mtimer_t *tm, uint8_t id, uint32_t interval_ms)
{
    mtimer_entry_t *timer;

    if (tm == NULL) {
        return MTIMER_ERR_NULL;
    }
    if (mtimer_manager_busy(tm)) {
        return MTIMER_ERR_BUSY;
    }
    if (!mtimer_valid_id(tm, id)) {
        return MTIMER_ERR_NOT_FOUND;
    }
    if (interval_ms == 0u) {
        return MTIMER_ERR_INVALID;
    }

    timer = &tm->timers[id];
    timer->interval_ms = interval_ms;

    if (timer->state_storage == (uint8_t)MTIMER_RUNNING) {
        timer->start_ms = tm->clock();
        timer->remaining_ms = 0u;
    } else if (timer->state_storage == (uint8_t)MTIMER_PAUSED) {
        timer->remaining_ms = interval_ms;
    } else {
        timer->remaining_ms = 0u;
    }

    return MTIMER_OK;
}

int mtimer_tick(mtimer_t *tm)
{
    uint32_t now;
    int fired;
    int i;

    if (tm == NULL) {
        return MTIMER_ERR_NULL;
    }
    if (tm->clock == NULL) {
        return MTIMER_ERR_INVALID;
    }
    if (mtimer_manager_busy(tm)) {
        return MTIMER_ERR_BUSY;
    }

    tm->active_tick = 1u;
    now = tm->clock();
    fired = 0;

    for (i = 0; i < MTIMER_MAX_TIMERS; ++i) {
        mtimer_entry_t *timer;
        uint32_t elapsed_ms;
        uint32_t intervals_elapsed;
        mtimer_cb_fn callback;
        void *ctx;

        timer = &tm->timers[i];
        if (timer->allocated == 0u || timer->state_storage != (uint8_t)MTIMER_RUNNING) {
            continue;
        }

        elapsed_ms = mtimer_elapsed(timer->start_ms, now);
        if (elapsed_ms < timer->interval_ms) {
            continue;
        }

        callback = timer->callback;
        ctx = timer->ctx;

        timer->fire_count++;
        tm->fire_count++;
        fired++;

        if (timer->mode_storage == (uint8_t)MTIMER_ONESHOT) {
            timer->state_storage = (uint8_t)MTIMER_STOPPED;
            timer->remaining_ms = 0u;
        } else {
            intervals_elapsed = elapsed_ms / timer->interval_ms;
            timer->start_ms += intervals_elapsed * timer->interval_ms;
            timer->remaining_ms = 0u;
        }

        callback((uint8_t)i, ctx);
    }

    tm->active_tick = 0u;
    tm->tick_count++;
    return fired;
}

mtimer_err_t mtimer_get_count(const mtimer_t *tm, uint8_t *out_count)
{
    int i;
    uint8_t count;

    if (tm == NULL || out_count == NULL) {
        return MTIMER_ERR_NULL;
    }

    count = 0u;
    for (i = 0; i < MTIMER_MAX_TIMERS; ++i) {
        if (tm->timers[i].allocated != 0u) {
            count++;
        }
    }

    *out_count = count;
    return MTIMER_OK;
}

const mtimer_entry_t *mtimer_at(const mtimer_t *tm, uint8_t id)
{
    if (tm == NULL || !mtimer_valid_id(tm, id)) {
        return NULL;
    }
    return &tm->timers[id];
}

int mtimer_find(const mtimer_t *tm, const char *name)
{
    int i;

    if (tm == NULL || name == NULL) {
        return MTIMER_ERR_NULL;
    }

    for (i = 0; i < MTIMER_MAX_TIMERS; ++i) {
        if (tm->timers[i].allocated != 0u &&
            tm->timers[i].name != NULL &&
            strcmp(tm->timers[i].name, name) == 0) {
            return i;
        }
    }

    return MTIMER_ERR_NOT_FOUND;
}

mtimer_err_t mtimer_get_state(const mtimer_t *tm, uint8_t id, mtimer_state_t *out_state)
{
    const mtimer_entry_t *timer;

    if (tm == NULL || out_state == NULL) {
        return MTIMER_ERR_NULL;
    }
    if (!mtimer_valid_id(tm, id)) {
        return MTIMER_ERR_NOT_FOUND;
    }

    timer = &tm->timers[id];
    if (!mtimer_state_is_valid_storage(timer->state_storage)) {
        return MTIMER_ERR_INVALID;
    }

    *out_state = mtimer_state_from_storage(timer->state_storage);
    return MTIMER_OK;
}

mtimer_err_t mtimer_get_remaining(const mtimer_t *tm, uint8_t id, uint32_t *out_remaining_ms)
{
    const mtimer_entry_t *timer;
    uint32_t elapsed_ms;

    if (tm == NULL || out_remaining_ms == NULL) {
        return MTIMER_ERR_NULL;
    }
    if (!mtimer_valid_id(tm, id)) {
        return MTIMER_ERR_NOT_FOUND;
    }

    timer = &tm->timers[id];
    if (timer->state_storage == (uint8_t)MTIMER_PAUSED) {
        *out_remaining_ms = timer->remaining_ms;
        return MTIMER_OK;
    }
    if (timer->state_storage == (uint8_t)MTIMER_STOPPED) {
        *out_remaining_ms = 0u;
        return MTIMER_OK;
    }
    if (timer->state_storage != (uint8_t)MTIMER_RUNNING) {
        return MTIMER_ERR_INVALID;
    }

    elapsed_ms = mtimer_elapsed(timer->start_ms, tm->clock());
    if (elapsed_ms >= timer->interval_ms) {
        *out_remaining_ms = 0u;
    } else {
        *out_remaining_ms = timer->interval_ms - elapsed_ms;
    }
    return MTIMER_OK;
}

mtimer_err_t mtimer_get_fire_count(const mtimer_t *tm, uint8_t id, uint32_t *out_fire_count)
{
    if (tm == NULL || out_fire_count == NULL) {
        return MTIMER_ERR_NULL;
    }
    if (!mtimer_valid_id(tm, id)) {
        return MTIMER_ERR_NOT_FOUND;
    }

    *out_fire_count = tm->timers[id].fire_count;
    return MTIMER_OK;
}

mtimer_err_t mtimer_get_total_fires(const mtimer_t *tm, uint32_t *out_total_fires)
{
    if (tm == NULL || out_total_fires == NULL) {
        return MTIMER_ERR_NULL;
    }

    *out_total_fires = tm->fire_count;
    return MTIMER_OK;
}

mtimer_err_t mtimer_get_total_ticks(const mtimer_t *tm, uint32_t *out_total_ticks)
{
    if (tm == NULL || out_total_ticks == NULL) {
        return MTIMER_ERR_NULL;
    }

    *out_total_ticks = tm->tick_count;
    return MTIMER_OK;
}
