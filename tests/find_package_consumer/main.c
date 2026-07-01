#include "mtimer.h"

static uint32_t installed_clock(void)
{
    return 0u;
}

static void installed_cb(uint8_t id, void *ctx)
{
    (void)id;
    (void)ctx;
}

int main(void)
{
    mtimer_t tm;

    if (mtimer_init(&tm, installed_clock) != MTIMER_OK) {
        return 1;
    }
    return mtimer_create(&tm, "installed", 1u, MTIMER_ONESHOT, installed_cb, 0) >= 0 ? 0 : 1;
}
