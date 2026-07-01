# Design Rationale

## Execution Model

One execution context owns one `mtimer_t` manager by default. `mtimer_tick()` and all control APIs are expected to be serialized externally. The library holds no hidden locks and does not claim general thread safety.

ISR use is limited: the manager must be exclusively owned by the ISR or protected externally, the clock callback must be ISR-safe and bounded, and timer callbacks must be ISR-safe, bounded, non-blocking, and avoid prohibited same-manager mutations.

Nonlocal exit and abnormal termination have no special cleanup semantics. `return`, `goto`, `break`, and `continue` in caller code do not trigger library cleanup. The library never performs `free`, `fclose`, mutex unlock, or custom cleanup callbacks. `longjmp` from a timer callback is unsupported and may leave the active-tick guard set. `abort`, `_Exit`, reset, hard fault, and power loss stop execution immediately and provide no pending-callback, persistence, durability, recovery, or exactly-once guarantees.

## Storage and ABI

- Timers live in a fixed array selected by `MTIMER_MAX_TIMERS`.
- The public struct layout is part of the ABI, so initialization routes through `mtimer_init_sized()` and checks the caller-visible `sizeof(mtimer_t)` before touching storage.
- Enum values remain symbolic in the API, while runtime storage uses fixed-width integer fields to avoid compiler-dependent enum layout.

## Timing Model

- Clock units are milliseconds on an unsigned 32-bit modulo counter.
- Natural wraparound is supported as long as operations continue within one full `uint32_t` cycle and the clock is not reset while active timers exist.
- `mtimer_tick()` captures one `now` value and evaluates expiries in ascending slot order.
- Periodic timers fire at most once per successful tick. If one period is due, the original phase is retained. If multiple periods were missed, skipped callbacks are rebased deterministically by advancing `start_ms` by `floor(elapsed / interval) * interval`.
- Counters use unsigned modulo wrap.

## Callback Safety

- `mtimer_tick()` sets a per-manager active-tick guard.
- Recursive `mtimer_tick()` on the same manager returns `MTIMER_ERR_BUSY`.
- Same-manager `create`, `destroy`, `start`, `stop`, `pause`, `resume`, `set_interval`, and reinitialization return `MTIMER_ERR_BUSY` while callbacks are active.
- Post-expiry timer state is finalized before invoking user callbacks so callback-side observations stay coherent.
