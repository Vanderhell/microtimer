# microtimer

[![CI](https://github.com/Vanderhell/microtimer/actions/workflows/ci.yml/badge.svg?branch=master)](https://github.com/Vanderhell/microtimer/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![C Standard](https://img.shields.io/badge/C-C99-blue.svg)](https://en.wikipedia.org/wiki/C99)

`microtimer` is a fixed-capacity software timer manager for C99 projects.

It targets serialized access to one manager from one execution context by default. It does not provide heap allocation, hidden locks, persistence, or general ISR/thread safety.

## Support Scope

- C99 and C11 consumers
- C++ header consumption
- GCC, Clang, and MSVC builds
- Main-loop usage
- ISR-owned usage with strict limits
- External synchronization around shared access
- ARM Cortex-M compile-only verification
- CMake `find_package()` and `add_subdirectory()` consumers

## Quick Start

```c
#include "mtimer.h"

static uint32_t app_clock_ms(void) { return platform_millis(); }

static void blink_cb(uint8_t id, void *ctx)
{
    (void)id;
    *(volatile int *)ctx = 1;
}

int main(void)
{
    mtimer_t tm;
    volatile int blink_due = 0;
    int timer_id;

    if (mtimer_init(&tm, app_clock_ms) != MTIMER_OK) {
        return 1;
    }

    timer_id = mtimer_create(&tm, "blink", 500u, MTIMER_PERIODIC, blink_cb, (void *)&blink_due);
    if (timer_id < 0) {
        return 1;
    }

    if (mtimer_start(&tm, (uint8_t)timer_id) != MTIMER_OK) {
        return 1;
    }

    for (;;) {
        int tick_rc = mtimer_tick(&tm);
        if (tick_rc < 0) {
            return 1;
        }
        if (blink_due) {
            blink_due = 0;
            toggle_led();
        }
    }
}
```

## Key Contracts

- Clock units are milliseconds on an unsigned 32-bit modulo counter.
- Natural `uint32_t` wraparound is supported if the clock is not reset while timers are active.
- `mtimer_tick()` fires each running timer at most once per successful call.
- Missed periodic intervals are skipped without callback bursts; phase is retained when only one interval is due.
- Timer names are optional and caller-owned. Non-`NULL` names must remain valid, immutable, and unique per manager.
- Timer IDs are slot indexes. Destroying a timer invalidates its ID, and later creates may reuse that slot.
- Same-manager mutation during callbacks returns `MTIMER_ERR_BUSY`.

## Build

### CMake

```bash
cmake -S . -B build -DMICROTIMER_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

### Make

```bash
make
```

Caller-provided `CC`, `CPPFLAGS`, `CFLAGS`, and `LDFLAGS` are honored by the Makefiles.

## Documentation

- [API reference](docs/API_REFERENCE.md)
- [Cookbook](docs/COOKBOOK.md)
- [Design notes](docs/DESIGN.md)
- [Issues and troubleshooting](docs/ISSUES.md)
- [Porting guide](docs/PORTING_GUIDE.md)
- [Verification status](docs/VERIFICATION.md)
- [Contributing](CONTRIBUTING.md)
- [Security reporting](SECURITY.md)
- [Changelog](CHANGELOG.md)

Releases are tag-driven through `.github/workflows/release.yml` and are only published from pushed `v*` tags.
