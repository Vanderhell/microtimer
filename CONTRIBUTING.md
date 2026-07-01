# Contributing

## Project Rules

- Keep the public C API compatible with C99.
- Preserve fixed-capacity, caller-owned storage semantics.
- Do not add heap allocation, OS or RTOS dependencies, hidden locks, or generated code systems.
- Keep access serialized per manager unless callers provide external synchronization.
- Do not weaken runtime tests or remove compile-fail tests.
- Do not create tags or releases unless explicitly requested.

## Development Notes

- Prefer CMake for package and consumer verification.
- The checked-in `include/mtimer_config.h` is the source-tree default; build and install trees use a generated resolved configuration header.
- Timer callbacks return `void`; error propagation must happen through caller-managed state.
- `longjmp` out of a callback is unsupported and may leave the active-tick guard set.
