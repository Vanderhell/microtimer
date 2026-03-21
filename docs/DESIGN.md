# Design Rationale

## 1. Slot-based allocation
Timers live in a fixed array. create() finds a free slot, destroy() frees it. No heap, no linked lists.

## 2. Drift-corrected periodic
Periodic timers anchor to start_ms + interval, not "now." This prevents drift accumulation over long runs. If we're way behind (missed multiple periods), we snap to now.

## 3. Pause saves remaining
Pause captures how much time is left. Resume restores from that point, regardless of how long the pause lasted.

## 4. Oneshot auto-stops
No need to manually stop a oneshot timer. It transitions to STOPPED after firing.

| Decision | Gains | Costs |
|----------|-------|-------|
| Fixed array | Zero alloc, O(1) access | Max timer count |
| Drift correction | Accurate long-term timing | Slightly complex tick |
| Pause/resume | Useful for UI, power save | Extra state per timer |
| Auto-stop oneshot | Clean semantics | Can't restart without explicit start |
