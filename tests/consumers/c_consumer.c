#include "mtimer.h"

static uint32_t consumer_clock(void)
{
    return 0u;
}

static void consumer_cb(uint8_t id, void *ctx)
{
    (void)id;
    (void)ctx;
}

int main(void)
{
    mtimer_t tm;

    if (mtimer_init(&tm, consumer_clock) != MTIMER_OK) {
        return 1;
    }
    if (mtimer_create(&tm, "consumer", 1u, MTIMER_ONESHOT, consumer_cb, 0) < 0) {
        return 1;
    }
    return 0;
}
