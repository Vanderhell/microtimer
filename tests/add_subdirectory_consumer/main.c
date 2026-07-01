#include "mtimer.h"

static uint32_t subdir_clock(void)
{
    return 0u;
}

static void subdir_cb(uint8_t id, void *ctx)
{
    (void)id;
    (void)ctx;
}

int main(void)
{
    mtimer_t tm;

    if (mtimer_init(&tm, subdir_clock) != MTIMER_OK) {
        return 1;
    }
    return mtimer_create(&tm, "subdir", 1u, MTIMER_PERIODIC, subdir_cb, 0) >= 0 ? 0 : 1;
}
