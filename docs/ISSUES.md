# Issues and Troubleshooting

## ABI mismatch or configuration mismatch

`MTIMER_ERR_ABI` from `mtimer_init_sized()` indicates the caller and library were built with different `mtimer_t` layouts or resolved `MTIMER_MAX_TIMERS` values.

## Invalid `MTIMER_MAX_TIMERS`

Values below `1` or above `255` intentionally fail compilation through `mtimer_config.h`.

## Old ambiguous query API

Use the explicit status-plus-output query functions. `mtimer_tick(NULL)` now returns `MTIMER_ERR_NULL`, not `0`.

## Invalid timer mode

Only `MTIMER_ONESHOT` and `MTIMER_PERIODIC` are accepted.

## Duplicate timer name

Non-`NULL` names must be unique per manager.

## Stale ID after destroy

Destroy invalidates the slot immediately. A later create may reuse the same numeric ID for a different timer.

## Create does not start

`mtimer_create()` only allocates and configures a timer. Call `mtimer_start()` separately.

## Pause and resume transitions

Pause is valid only from `MTIMER_RUNNING`. Resume is valid only from `MTIMER_PAUSED`.

## Zero interval

`mtimer_create()` and `mtimer_set_interval()` reject `0`.

## Interval change while paused

Paused timers stay paused and adopt the full new interval as remaining time, regardless of whether the new interval is smaller or larger.

## Recursive tick or mutation during callback

Same-manager `mtimer_tick()` and mutating APIs return `MTIMER_ERR_BUSY` while a callback from that manager is active.

## `longjmp` from a callback

Unsupported. It may leave the active-tick guard set and the manager unusable.

## Clock reset or backward jump

Unsupported while timers are active. Only natural unsigned wraparound is supported.

## Unsigned wrap limitations

One full `uint32_t` cycle is the maximum distinguishable elapsed time window.

## CMake package not found

Pass the install prefix through `CMAKE_PREFIX_PATH` or `microtimer_DIR`.

## Makefile flags

The Makefiles honor caller-provided `CC`, `CPPFLAGS`, `CFLAGS`, and `LDFLAGS`. Inject platform flags there instead of editing the files.

## Compiler diagnostic differences

Compile-fail wording may vary slightly across GCC, Clang, and MSVC, but the tests target the intended project diagnostic class.

## Sanitizer or analyzer tools unavailable

Some CI jobs are conditional on tool availability. Missing tools should be reported as skipped, not as passed.
