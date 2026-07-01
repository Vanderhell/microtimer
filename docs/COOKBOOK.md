# Cookbook

## 1. Minimal initialization

```c
mtimer_t tm;
if (mtimer_init(&tm, platform_millis) != MTIMER_OK) {
    return;
}
```

## 2. Complete quick start

```c
static volatile int timeout_due = 0;

static void timeout_cb(uint8_t id, void *ctx)
{
    (void)id;
    *(volatile int *)ctx = 1;
}

void app_loop(void)
{
    mtimer_t tm;
    int timer_id;

    if (mtimer_init(&tm, platform_millis) != MTIMER_OK) {
        return;
    }
    timer_id = mtimer_create(&tm, "timeout", 3000u, MTIMER_ONESHOT, timeout_cb, (void *)&timeout_due);
    if (timer_id < 0) {
        return;
    }
    if (mtimer_start(&tm, (uint8_t)timer_id) != MTIMER_OK) {
        return;
    }
    for (;;) {
        int tick_rc = mtimer_tick(&tm);
        if (tick_rc < 0) {
            return;
        }
        if (timeout_due) {
            timeout_due = 0;
            handle_timeout();
        }
    }
}
```

## 3. Oneshot timer

Create with `MTIMER_ONESHOT`, then call `mtimer_start()`. It stops itself before the callback runs.

## 4. Periodic timer

Create with `MTIMER_PERIODIC`. Missed periods are skipped without callback bursts.

## 5. Pause and resume

Pause only from `MTIMER_RUNNING`, resume only from `MTIMER_PAUSED`. If a timer was already due when paused, resuming makes it eligible on the next tick.

## 6. Change interval

Running timers restart from the current clock. Paused timers keep the paused state and reset remaining time to the full new interval.

## 7. Named timer lookup

Use `mtimer_find(&tm, "name")`. Duplicate non-`NULL` names are rejected.

## 8. Multiple managers

Independent managers may be ticked independently because the active-tick guard is per manager.

## 9. C++ consumer

The public header is valid in C++ translation units. See `tests/consumers/cpp_consumer.cpp`.

## 10. Installed CMake package consumer

See `tests/find_package_consumer/` for a `find_package(microtimer CONFIG REQUIRED)` fixture that links `microtimer::microtimer`.

## 11. add_subdirectory consumer

See `tests/add_subdirectory_consumer/` for an `add_subdirectory()` fixture.

## 12. Main-loop integration

Use `mtimer_tick()` from one serialized loop or task that owns the manager.

## 13. ISR-limited usage

Only use from an ISR when the manager is ISR-owned or externally protected, the clock function is ISR-safe, and callbacks stay bounded and non-blocking.

## 14. External locking

Protect every API call with the same mutex or critical section when sharing one manager across execution contexts.

## 15. Counter policy

Per-timer and global counters use unsigned modulo wrap at `UINT32_MAX`.

## 16. Clock wraparound

Unsigned `uint32_t` wraparound is supported, but elapsed time longer than one complete counter cycle is not distinguishable.

## 17. Context and name lifetime

Callback context pointers and non-`NULL` timer names are caller-owned and must remain valid for the timer lifetime.

## 18. Set-flag callback pattern

Prefer callbacks that set flags or enqueue lightweight work, then act after `mtimer_tick()` returns.
