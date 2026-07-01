# API Reference

## Initialization

- `mtimer_init(mtimer_t *tm, mtimer_clock_fn clock)`
  Initializes a manager using the caller-visible `sizeof(*tm)`.
- `mtimer_init_sized(mtimer_t *tm, size_t tm_size, mtimer_clock_fn clock)`
  Returns `MTIMER_ERR_ABI` when the caller and library disagree on manager size or resolved configuration.

## Creation and Lifetime

- `mtimer_create(...)`
  Returns a non-negative slot index on success or a negative `mtimer_err_t` on failure.
- `mtimer_destroy(mtimer_t *tm, uint8_t id)`
  Invalidates the slot index immediately. Later creates may reuse the same slot.

`name == NULL` creates an unnamed timer. Non-`NULL` names are caller-owned, must remain valid and immutable for the timer lifetime, and must be unique within the manager.

## Control

- `mtimer_start()`
  Fully restarts a timer from the current clock value.
- `mtimer_stop()`
  Stops the timer and clears paused bookkeeping relevance.
- `mtimer_pause()`
  Valid only from `MTIMER_RUNNING`. Already-due timers store `remaining_ms == 0`.
- `mtimer_resume()`
  Valid only from `MTIMER_PAUSED`. Zero remaining time becomes eligible on the next tick.
- `mtimer_set_interval()`
  Rejects zero. Running timers restart from the current clock. Paused timers remain paused and adopt the full new interval as remaining time. Stopped timers keep the new interval and remain stopped.

## Tick

- `mtimer_tick(mtimer_t *tm)`
  Returns the number of timers fired on success, or a negative `mtimer_err_t` such as `MTIMER_ERR_NULL`, `MTIMER_ERR_INVALID`, or `MTIMER_ERR_BUSY`.

Each successful tick captures a single `now` value, processes timers in ascending slot order, and fires each timer at most once.

## Queries

- `mtimer_get_count()`
- `mtimer_at()`
- `mtimer_find()`
- `mtimer_get_state()`
- `mtimer_get_remaining()`
- `mtimer_get_fire_count()`
- `mtimer_get_total_fires()`
- `mtimer_get_total_ticks()`

All query functions report failure explicitly instead of returning values that could be mistaken for valid state.

## Error Codes

- `MTIMER_OK`
- `MTIMER_ERR_NULL`
- `MTIMER_ERR_FULL`
- `MTIMER_ERR_NOT_FOUND`
- `MTIMER_ERR_INVALID`
- `MTIMER_ERR_BUSY`
- `MTIMER_ERR_ABI`
