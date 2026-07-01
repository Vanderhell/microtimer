extern "C" {
#include "mtimer.h"
}

static uint32_t consumer_clock()
{
    return 0u;
}

static void consumer_cb(uint8_t id, void *ctx)
{
    (void)id;
    (void)ctx;
}

int main()
{
    mtimer_t tm;
    return mtimer_init(&tm, consumer_clock) == MTIMER_OK &&
                   mtimer_create(&tm, "cpp", 1u, MTIMER_ONESHOT, consumer_cb, nullptr) >= 0
               ? 0
               : 1;
}
