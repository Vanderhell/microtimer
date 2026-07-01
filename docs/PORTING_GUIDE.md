# Porting Guide

## Clock Source

Provide a bounded, non-blocking function that returns milliseconds on an unsigned 32-bit counter.

## Serialization

Own each manager from one execution context by default. If a manager is shared, serialize every API call externally with one mutex or critical section.

## ISR Usage

Only use a manager from an ISR when the ISR exclusively owns it or all access is externally protected. Host tests do not verify hardware ISR behavior.

## Build Integration

- CMake consumers can use `add_subdirectory()` or an installed package.
- Make-based builds can inject flags through `CC`, `CPPFLAGS`, `CFLAGS`, and `LDFLAGS`.
- Do not override the resolved `mtimer_config.h` with a conflicting `MTIMER_MAX_TIMERS` definition.
