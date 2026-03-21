# microtimer

[![CI](https://github.com/Vanderhell/microtimer/actions/workflows/ci.yml/badge.svg?branch=master)](https://github.com/Vanderhell/microtimer/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![C Standard](https://img.shields.io/badge/C-C99-blue.svg)](https://en.wikipedia.org/wiki/C99)

Software timer manager for embedded systems.

C99 | Zero dependencies | Zero allocations | Oneshot + Periodic | Portable

## Why microtimer?

Embedded main loops often contain repeated timing checks:

```c
if (now - last_blink > 500) { toggle_led(); last_blink = now; }
if (now - last_send > 5000) { send_telemetry(); last_send = now; }
if (now - last_check > 1000) { check_sensors(); last_check = now; }
```

`microtimer` replaces this with registered timers and a single tick path:

```c
mtimer_create(&tm, "blink",   500,  MTIMER_PERIODIC, on_blink, NULL);
mtimer_create(&tm, "send",    5000, MTIMER_PERIODIC, on_send, NULL);
mtimer_create(&tm, "timeout", 3000, MTIMER_ONESHOT,  on_timeout, NULL);

while (1) {
    mtimer_tick(&tm);
}
```

## Features

- Oneshot timers that auto-stop after firing.
- Periodic timers with drift correction.
- Pause/resume with remaining-time preservation.
- Dynamic interval changes at runtime.
- Slot reuse through destroy/create.
- Named timer lookup for diagnostics and shell commands.
- Per-timer and global fire counters.

## Build and Test

Requirements:
- C99 compiler (`gcc` or `clang`)
- `make`

Run tests:

```bash
# clone microtest next to this repository root
# expected path: ../microtest/include
make -C tests
```

## Public API

Key functions:
- `mtimer_init`
- `mtimer_create`, `mtimer_destroy`
- `mtimer_start`, `mtimer_stop`, `mtimer_pause`, `mtimer_resume`
- `mtimer_set_interval`
- `mtimer_tick`
- `mtimer_count`, `mtimer_find`, `mtimer_remaining`

See [`include/mtimer.h`](include/mtimer.h) for full API details.

## Repository Layout

- `include/mtimer.h` - public API
- `src/mtimer.c` - implementation
- `tests/test_all.c` - unit tests
- `docs/DESIGN.md` - design rationale

## Ecosystem

- [microhealth](https://github.com/Vanderhell/microhealth)
- [microwdt](https://github.com/Vanderhell/microwdt)
- [microres](https://github.com/Vanderhell/microres)
- [microsh](https://github.com/Vanderhell/microsh)

## Configuration

- `MTIMER_MAX_TIMERS` (default: `8`)

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md).

## Changelog

See [CHANGELOG.md](CHANGELOG.md).

## License

MIT - see [LICENSE](LICENSE).
