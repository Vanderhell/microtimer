#include "mtimer.h"
#include "test_harness.h"

#include <limits.h>
#include <string.h>

static uint32_t mock_time_a = 1000u;
static uint32_t mock_time_b = 2000u;
static uint8_t fired_ids[32];
static unsigned fired_count = 0u;
static unsigned callback_mutation_rc = 0u;
static int callback_value = 0;

static uint32_t mock_clock_a(void)
{
    return mock_time_a;
}

static uint32_t mock_clock_b(void)
{
    return mock_time_b;
}

static void log_fire(uint8_t id, void *ctx)
{
    (void)ctx;
    if (fired_count < 32u) {
        fired_ids[fired_count++] = id;
    }
}

static void store_value(uint8_t id, void *ctx)
{
    (void)id;
    callback_value = *(int *)ctx;
}

static void reentrant_tick(uint8_t id, void *ctx)
{
    mtimer_t *tm = (mtimer_t *)ctx;
    (void)id;
    callback_mutation_rc = (unsigned)mtimer_tick(tm);
}

static void mutate_during_callback(uint8_t id, void *ctx)
{
    mtimer_t *tm = (mtimer_t *)ctx;

    (void)id;
    callback_mutation_rc = (unsigned)mtimer_start(tm, 0u);
}

static void reset_logs(void)
{
    memset(fired_ids, 0, sizeof(fired_ids));
    fired_count = 0u;
    callback_mutation_rc = 0u;
    callback_value = 0;
}

static int test_init_and_count(void)
{
    mtimer_t tm;
    uint8_t count = 99u;
    uint32_t ticks = 99u;

    reset_logs();
    TEST_ASSERT_EQ(MTIMER_OK, mtimer_init(&tm, mock_clock_a));
    TEST_ASSERT_EQ(MTIMER_OK, mtimer_get_count(&tm, &count));
    TEST_ASSERT_EQ(0u, count);
    TEST_ASSERT_EQ(MTIMER_OK, mtimer_get_total_ticks(&tm, &ticks));
    TEST_ASSERT_EQ(0u, ticks);
    return 0;
}

static int test_init_null_and_abi(void)
{
    mtimer_t tm;

    TEST_ASSERT_EQ(MTIMER_ERR_NULL, mtimer_init_sized(NULL, sizeof(tm), mock_clock_a));
    TEST_ASSERT_EQ(MTIMER_ERR_NULL, mtimer_init_sized(&tm, sizeof(tm), NULL));
    TEST_ASSERT_EQ(MTIMER_ERR_ABI, mtimer_init_sized(&tm, sizeof(tm) - 1u, mock_clock_a));
    return 0;
}

static int test_create_name_mode_contracts(void)
{
    mtimer_t tm;
    int id0;
    int id1;

    TEST_ASSERT_EQ(MTIMER_OK, mtimer_init(&tm, mock_clock_a));
    id0 = mtimer_create(&tm, NULL, 100u, MTIMER_ONESHOT, log_fire, NULL);
    TEST_ASSERT_EQ(0, id0);
    id1 = mtimer_create(&tm, "dup", 200u, MTIMER_PERIODIC, log_fire, NULL);
    TEST_ASSERT_EQ(1, id1);
    TEST_ASSERT_EQ(MTIMER_ERR_INVALID, mtimer_create(&tm, "dup", 300u, MTIMER_PERIODIC, log_fire, NULL));
    TEST_ASSERT_EQ(MTIMER_ERR_INVALID, mtimer_create(&tm, "bad", 300u, (mtimer_mode_t)-1, log_fire, NULL));
    TEST_ASSERT_EQ(MTIMER_ERR_INVALID, mtimer_create(&tm, "bad2", 300u, (mtimer_mode_t)2, log_fire, NULL));
    return 0;
}

static int test_table_full_and_slot_reuse(void)
{
    mtimer_t tm;
    int i;
    int reused;

    TEST_ASSERT_EQ(MTIMER_OK, mtimer_init(&tm, mock_clock_a));
    for (i = 0; i < MTIMER_MAX_TIMERS; ++i) {
        TEST_ASSERT_EQ(i, mtimer_create(&tm, NULL, 100u, MTIMER_ONESHOT, log_fire, NULL));
    }
    TEST_ASSERT_EQ(MTIMER_ERR_FULL, mtimer_create(&tm, "full", 100u, MTIMER_ONESHOT, log_fire, NULL));
    TEST_ASSERT_EQ(MTIMER_OK, mtimer_destroy(&tm, 2u));
    reused = mtimer_create(&tm, "reused", 100u, MTIMER_ONESHOT, log_fire, NULL);
    TEST_ASSERT_EQ(2, reused);
    TEST_ASSERT_EQ(MTIMER_ERR_NOT_FOUND, mtimer_start(&tm, (uint8_t)MTIMER_MAX_TIMERS));
    return 0;
}

static int test_oneshot_exact_expiry(void)
{
    mtimer_t tm;
    mtimer_state_t state = MTIMER_RUNNING;

    TEST_ASSERT_EQ(MTIMER_OK, mtimer_init(&tm, mock_clock_a));
    TEST_ASSERT_EQ(0, mtimer_create(&tm, "once", 50u, MTIMER_ONESHOT, log_fire, NULL));
    TEST_ASSERT_EQ(MTIMER_OK, mtimer_start(&tm, 0u));
    mock_time_a += 49u;
    TEST_ASSERT_EQ(0, mtimer_tick(&tm));
    mock_time_a += 1u;
    TEST_ASSERT_EQ(1, mtimer_tick(&tm));
    TEST_ASSERT_EQ(1u, fired_count);
    TEST_ASSERT_EQ(MTIMER_OK, mtimer_get_state(&tm, 0u, &state));
    TEST_ASSERT_EQ(MTIMER_STOPPED, state);
    return 0;
}

static int test_periodic_exact_and_single_fire_per_tick(void)
{
    mtimer_t tm;
    uint32_t fire_count_value = 0u;

    TEST_ASSERT_EQ(MTIMER_OK, mtimer_init(&tm, mock_clock_a));
    TEST_ASSERT_EQ(0, mtimer_create(&tm, "periodic", 10u, MTIMER_PERIODIC, log_fire, NULL));
    TEST_ASSERT_EQ(MTIMER_OK, mtimer_start(&tm, 0u));
    mock_time_a += 10u;
    TEST_ASSERT_EQ(1, mtimer_tick(&tm));
    mock_time_a += 35u;
    TEST_ASSERT_EQ(1, mtimer_tick(&tm));
    TEST_ASSERT_EQ(MTIMER_OK, mtimer_get_fire_count(&tm, 0u, &fire_count_value));
    TEST_ASSERT_EQ(2u, fire_count_value);
    return 0;
}

static int test_wraparound_and_pause_resume(void)
{
    mtimer_t tm;
    int pause_id;
    uint32_t remaining = 0u;

    mock_time_a = UINT32_MAX - 3u;
    TEST_ASSERT_EQ(MTIMER_OK, mtimer_init(&tm, mock_clock_a));
    TEST_ASSERT_EQ(0, mtimer_create(&tm, "wrap", 5u, MTIMER_ONESHOT, log_fire, NULL));
    TEST_ASSERT_EQ(MTIMER_OK, mtimer_start(&tm, 0u));
    mock_time_a += 2u;
    TEST_ASSERT_EQ(0, mtimer_tick(&tm));
    mock_time_a += 3u;
    TEST_ASSERT_EQ(1, mtimer_tick(&tm));

    mock_time_a = 100u;
    pause_id = mtimer_create(&tm, "pause", 20u, MTIMER_ONESHOT, log_fire, NULL);
    TEST_ASSERT_EQ(1, pause_id);
    TEST_ASSERT_EQ(MTIMER_OK, mtimer_start(&tm, (uint8_t)pause_id));
    mock_time_a += 20u;
    TEST_ASSERT_EQ(MTIMER_OK, mtimer_pause(&tm, (uint8_t)pause_id));
    TEST_ASSERT_EQ(MTIMER_OK, mtimer_get_remaining(&tm, (uint8_t)pause_id, &remaining));
    TEST_ASSERT_EQ(0u, remaining);
    TEST_ASSERT_EQ(MTIMER_OK, mtimer_resume(&tm, (uint8_t)pause_id));
    TEST_ASSERT_EQ(1, mtimer_tick(&tm));
    return 0;
}

static int test_interval_changes(void)
{
    mtimer_t tm;
    uint32_t remaining = 0u;

    TEST_ASSERT_EQ(MTIMER_OK, mtimer_init(&tm, mock_clock_a));
    TEST_ASSERT_EQ(0, mtimer_create(&tm, "x", 100u, MTIMER_PERIODIC, log_fire, NULL));
    TEST_ASSERT_EQ(MTIMER_OK, mtimer_set_interval(&tm, 0u, 50u));
    TEST_ASSERT_EQ(MTIMER_OK, mtimer_start(&tm, 0u));
    mock_time_a += 10u;
    TEST_ASSERT_EQ(MTIMER_OK, mtimer_set_interval(&tm, 0u, 30u));
    mock_time_a += 29u;
    TEST_ASSERT_EQ(0, mtimer_tick(&tm));
    mock_time_a += 1u;
    TEST_ASSERT_EQ(1, mtimer_tick(&tm));
    TEST_ASSERT_EQ(MTIMER_OK, mtimer_pause(&tm, 0u));
    TEST_ASSERT_EQ(MTIMER_OK, mtimer_set_interval(&tm, 0u, 200u));
    TEST_ASSERT_EQ(MTIMER_OK, mtimer_get_remaining(&tm, 0u, &remaining));
    TEST_ASSERT_EQ(200u, remaining);
    return 0;
}

static int test_read_queries_and_callback_context(void)
{
    mtimer_t tm;
    mtimer_state_t state = MTIMER_STOPPED;

    TEST_ASSERT_EQ(MTIMER_OK, mtimer_init(&tm, mock_clock_a));
    TEST_ASSERT_EQ(0, mtimer_create(&tm, "ctx", 5u, MTIMER_ONESHOT, store_value, &callback_value));
    callback_value = 77;
    TEST_ASSERT_EQ(MTIMER_OK, mtimer_start(&tm, 0u));
    mock_time_a += 5u;
    TEST_ASSERT_EQ(1, mtimer_tick(&tm));
    TEST_ASSERT_EQ(77, callback_value);
    TEST_ASSERT_EQ(MTIMER_OK, mtimer_get_state(&tm, 0u, &state));
    TEST_ASSERT_EQ(MTIMER_STOPPED, state);
    TEST_ASSERT_EQ(0, mtimer_find(&tm, "ctx"));
    TEST_ASSERT_EQ(MTIMER_ERR_NOT_FOUND, mtimer_find(&tm, "missing"));
    return 0;
}

static int test_busy_rules_and_independent_managers(void)
{
    mtimer_t tm_a;
    mtimer_t tm_b;

    TEST_ASSERT_EQ(MTIMER_OK, mtimer_init(&tm_a, mock_clock_a));
    TEST_ASSERT_EQ(MTIMER_OK, mtimer_init(&tm_b, mock_clock_b));
    TEST_ASSERT_EQ(0, mtimer_create(&tm_a, "reentrant", 1u, MTIMER_ONESHOT, reentrant_tick, &tm_a));
    TEST_ASSERT_EQ(0, mtimer_create(&tm_b, "other", 1u, MTIMER_ONESHOT, log_fire, NULL));
    TEST_ASSERT_EQ(MTIMER_OK, mtimer_start(&tm_a, 0u));
    TEST_ASSERT_EQ(MTIMER_OK, mtimer_start(&tm_b, 0u));
    mock_time_a += 1u;
    mock_time_b += 1u;
    TEST_ASSERT_EQ(1, mtimer_tick(&tm_a));
    TEST_ASSERT_EQ((unsigned)MTIMER_ERR_BUSY, callback_mutation_rc);
    TEST_ASSERT_EQ(1, mtimer_tick(&tm_b));

    reset_logs();
    TEST_ASSERT_EQ(MTIMER_OK, mtimer_init(&tm_a, mock_clock_a));
    TEST_ASSERT_EQ(0, mtimer_create(&tm_a, "mutate", 1u, MTIMER_ONESHOT, mutate_during_callback, &tm_a));
    TEST_ASSERT_EQ(MTIMER_OK, mtimer_start(&tm_a, 0u));
    mock_time_a += 1u;
    TEST_ASSERT_EQ(1, mtimer_tick(&tm_a));
    TEST_ASSERT_EQ((unsigned)MTIMER_ERR_BUSY, callback_mutation_rc);
    return 0;
}

static int test_callback_order_and_counter_wrap_policy(void)
{
    mtimer_t tm;
    uint32_t fires = 0u;

    reset_logs();
    TEST_ASSERT_EQ(MTIMER_OK, mtimer_init(&tm, mock_clock_a));
    tm.fire_count = UINT32_MAX;
    TEST_ASSERT_EQ(0, mtimer_create(&tm, "a", 1u, MTIMER_ONESHOT, log_fire, NULL));
    TEST_ASSERT_EQ(1, mtimer_create(&tm, "b", 1u, MTIMER_ONESHOT, log_fire, NULL));
    TEST_ASSERT_EQ(MTIMER_OK, mtimer_start(&tm, 0u));
    TEST_ASSERT_EQ(MTIMER_OK, mtimer_start(&tm, 1u));
    mock_time_a += 1u;
    TEST_ASSERT_EQ(2, mtimer_tick(&tm));
    TEST_ASSERT_EQ(2u, fired_count);
    TEST_ASSERT_EQ(0u, fired_ids[0]);
    TEST_ASSERT_EQ(1u, fired_ids[1]);
    TEST_ASSERT_EQ(MTIMER_OK, mtimer_get_total_fires(&tm, &fires));
    TEST_ASSERT_EQ(1u, fires);
    return 0;
}

static int test_null_and_invalid_queries(void)
{
    mtimer_t tm;
    uint8_t count = 0u;
    uint32_t value = 0u;
    mtimer_state_t state = MTIMER_STOPPED;

    TEST_ASSERT_EQ(MTIMER_ERR_NULL, mtimer_tick(NULL));
    TEST_ASSERT_EQ(MTIMER_ERR_NULL, mtimer_get_count(NULL, &count));
    TEST_ASSERT_EQ(MTIMER_ERR_NULL, mtimer_get_count(&tm, NULL));
    TEST_ASSERT_EQ(MTIMER_ERR_NULL, mtimer_find(NULL, "x"));
    TEST_ASSERT_EQ(MTIMER_ERR_NULL, mtimer_get_state(NULL, 0u, &state));
    TEST_ASSERT_EQ(MTIMER_OK, mtimer_init(&tm, mock_clock_a));
    TEST_ASSERT_EQ(MTIMER_ERR_NOT_FOUND, mtimer_get_state(&tm, 0u, &state));
    TEST_ASSERT_EQ(MTIMER_ERR_NOT_FOUND, mtimer_get_remaining(&tm, 0u, &value));
    TEST_ASSERT_EQ(MTIMER_ERR_NOT_FOUND, mtimer_get_fire_count(&tm, 0u, &value));
    return 0;
}

int main(void)
{
    int failures = 0;

    failures += run_test("test_init_and_count", test_init_and_count);
    failures += run_test("test_init_null_and_abi", test_init_null_and_abi);
    failures += run_test("test_create_name_mode_contracts", test_create_name_mode_contracts);
    failures += run_test("test_table_full_and_slot_reuse", test_table_full_and_slot_reuse);
    failures += run_test("test_oneshot_exact_expiry", test_oneshot_exact_expiry);
    failures += run_test("test_periodic_exact_and_single_fire_per_tick", test_periodic_exact_and_single_fire_per_tick);
    failures += run_test("test_wraparound_and_pause_resume", test_wraparound_and_pause_resume);
    failures += run_test("test_interval_changes", test_interval_changes);
    failures += run_test("test_read_queries_and_callback_context", test_read_queries_and_callback_context);
    failures += run_test("test_busy_rules_and_independent_managers", test_busy_rules_and_independent_managers);
    failures += run_test("test_callback_order_and_counter_wrap_policy", test_callback_order_and_counter_wrap_policy);
    failures += run_test("test_null_and_invalid_queries", test_null_and_invalid_queries);

    printf("Assertions: %d\n", test_assertions);
    return failures == 0 ? 0 : 1;
}
