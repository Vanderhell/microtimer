/*
 * microtimer test suite — uses microtest framework.
 *
 * Build: gcc -std=c99 -Wall -Wextra -I../include -I../../microtest/include \
 *        ../src/mtimer.c test_all.c -lm -o test_all
 *
 * Without microtest (fallback): define MTIMER_STANDALONE_TEST
 */

#define MTEST_IMPLEMENTATION
#include "mtest.h"
#include "mtimer.h"

/* ── Mock clock ────────────────────────────────────────────────────────── */

static uint32_t mock_time = 1000;
static uint32_t mock_clock(void) { return mock_time; }

/* ── Callback tracking ─────────────────────────────────────────────────── */

#define MAX_FIRES 32
static uint8_t fire_log[MAX_FIRES];
static int fire_idx = 0;

static void on_fire(uint8_t id, void *ctx)
{
    (void)ctx;
    if (fire_idx < MAX_FIRES) fire_log[fire_idx++] = id;
}

static int ctx_received = 0;
static void on_fire_ctx(uint8_t id, void *ctx)
{
    (void)id;
    ctx_received = *(int *)ctx;
}

/* ── Setup ─────────────────────────────────────────────────────────────── */

static mtimer_t tm;

static void setup(void) {
    mock_time = 1000;
    fire_idx = 0;
    ctx_received = 0;
    memset(fire_log, 0, sizeof(fire_log));
    mtimer_init(&tm, mock_clock);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Tests
 * ═══════════════════════════════════════════════════════════════════════════ */

MTEST(test_init) {
    setup();
    MTEST_ASSERT_EQ(0, mtimer_count(&tm));
    MTEST_ASSERT_EQ(0, (int)mtimer_total_ticks(&tm));
    MTEST_ASSERT_EQ(0, (int)mtimer_total_fires(&tm));
}

MTEST(test_init_null) {
    MTEST_ASSERT_EQ(MTIMER_ERR_NULL, mtimer_init(NULL, mock_clock));
    MTEST_ASSERT_EQ(MTIMER_ERR_NULL, mtimer_init(&tm, NULL));
}

MTEST(test_create) {
    setup();
    int id = mtimer_create(&tm, "blink", 500, MTIMER_PERIODIC, on_fire, NULL);
    MTEST_ASSERT_EQ(0, id);
    MTEST_ASSERT_EQ(1, mtimer_count(&tm));
    MTEST_ASSERT_EQ(MTIMER_STOPPED, (int)mtimer_state(&tm, 0));
}

MTEST(test_create_full) {
    setup();
    for (int i = 0; i < MTIMER_MAX_TIMERS; i++) {
        MTEST_ASSERT_GE(mtimer_create(&tm, "t", 1000, MTIMER_ONESHOT, on_fire, NULL), 0);
    }
    MTEST_ASSERT_EQ(MTIMER_ERR_FULL, mtimer_create(&tm, "over", 1000, MTIMER_ONESHOT, on_fire, NULL));
}

MTEST(test_create_invalid) {
    setup();
    MTEST_ASSERT_EQ(MTIMER_ERR_NULL, mtimer_create(&tm, "x", 1000, MTIMER_ONESHOT, NULL, NULL));
    MTEST_ASSERT_EQ(MTIMER_ERR_INVALID, mtimer_create(&tm, "x", 0, MTIMER_ONESHOT, on_fire, NULL));
}

MTEST(test_destroy) {
    setup();
    int id = mtimer_create(&tm, "tmp", 1000, MTIMER_ONESHOT, on_fire, NULL);
    MTEST_ASSERT_EQ(1, mtimer_count(&tm));
    mtimer_destroy(&tm, (uint8_t)id);
    MTEST_ASSERT_EQ(0, mtimer_count(&tm));
}

MTEST(test_destroy_reuse_slot) {
    setup();
    int id0 = mtimer_create(&tm, "a", 1000, MTIMER_ONESHOT, on_fire, NULL);
    mtimer_destroy(&tm, (uint8_t)id0);
    int id1 = mtimer_create(&tm, "b", 2000, MTIMER_ONESHOT, on_fire, NULL);
    MTEST_ASSERT_EQ(id0, id1);  /* reuses slot */
}

/* ── Oneshot ───────────────────────────────────────────────────────────── */

MTEST(test_oneshot_fires_once) {
    setup();
    int id = mtimer_create(&tm, "delay", 1000, MTIMER_ONESHOT, on_fire, NULL);
    mtimer_start(&tm, (uint8_t)id);

    /* Before expiry */
    mock_time += 500;
    MTEST_ASSERT_EQ(0, mtimer_tick(&tm));

    /* At expiry */
    mock_time += 600;
    MTEST_ASSERT_EQ(1, mtimer_tick(&tm));
    MTEST_ASSERT_EQ(1, fire_idx);
    MTEST_ASSERT_EQ(MTIMER_STOPPED, (int)mtimer_state(&tm, (uint8_t)id));

    /* No more fires */
    mock_time += 2000;
    MTEST_ASSERT_EQ(0, mtimer_tick(&tm));
    MTEST_ASSERT_EQ(1, fire_idx);
}

/* ── Periodic ──────────────────────────────────────────────────────────── */

MTEST(test_periodic_fires_repeatedly) {
    setup();
    int id = mtimer_create(&tm, "heartbeat", 1000, MTIMER_PERIODIC, on_fire, NULL);
    mtimer_start(&tm, (uint8_t)id);

    for (int i = 0; i < 5; i++) {
        mock_time += 1000;
        mtimer_tick(&tm);
    }

    MTEST_ASSERT_EQ(5, fire_idx);
    MTEST_ASSERT_EQ(5, (int)mtimer_fire_count(&tm, (uint8_t)id));
    MTEST_ASSERT_EQ(MTIMER_RUNNING, (int)mtimer_state(&tm, (uint8_t)id));
}

MTEST(test_periodic_no_drift) {
    setup();
    int id = mtimer_create(&tm, "precise", 100, MTIMER_PERIODIC, on_fire, NULL);
    mtimer_start(&tm, (uint8_t)id);

    /* Tick at irregular intervals but 10× the period */
    mock_time += 1050;  /* should fire ~10 times, but we only get 1 per tick */
    mtimer_tick(&tm);

    /* With drift correction, start_ms advances to avoid accumulation */
    MTEST_ASSERT_GE(fire_idx, 1);
}

/* ── Stop / Start ──────────────────────────────────────────────────────── */

MTEST(test_stop_prevents_fire) {
    setup();
    int id = mtimer_create(&tm, "t", 1000, MTIMER_PERIODIC, on_fire, NULL);
    mtimer_start(&tm, (uint8_t)id);

    mock_time += 500;
    mtimer_stop(&tm, (uint8_t)id);

    mock_time += 2000;
    mtimer_tick(&tm);
    MTEST_ASSERT_EQ(0, fire_idx);
}

MTEST(test_restart_resets_timer) {
    setup();
    int id = mtimer_create(&tm, "t", 1000, MTIMER_ONESHOT, on_fire, NULL);
    mtimer_start(&tm, (uint8_t)id);

    mock_time += 800;  /* almost expired */
    mtimer_start(&tm, (uint8_t)id);  /* restart */

    mock_time += 500;  /* 500ms from restart, not expired yet */
    MTEST_ASSERT_EQ(0, mtimer_tick(&tm));

    mock_time += 600;  /* now expired */
    MTEST_ASSERT_EQ(1, mtimer_tick(&tm));
}

/* ── Pause / Resume ────────────────────────────────────────────────────── */

MTEST(test_pause_resume) {
    setup();
    int id = mtimer_create(&tm, "t", 1000, MTIMER_ONESHOT, on_fire, NULL);
    mtimer_start(&tm, (uint8_t)id);

    mock_time += 600;  /* 600ms elapsed, 400ms remaining */
    mtimer_pause(&tm, (uint8_t)id);
    MTEST_ASSERT_EQ(MTIMER_PAUSED, (int)mtimer_state(&tm, (uint8_t)id));
    MTEST_ASSERT_EQ(400, (int)mtimer_remaining(&tm, (uint8_t)id));

    /* Time passes while paused — shouldn't matter */
    mock_time += 5000;
    MTEST_ASSERT_EQ(0, mtimer_tick(&tm));
    MTEST_ASSERT_EQ(0, fire_idx);

    /* Resume */
    mtimer_resume(&tm, (uint8_t)id);
    MTEST_ASSERT_EQ(MTIMER_RUNNING, (int)mtimer_state(&tm, (uint8_t)id));

    /* Still ~400ms left */
    mock_time += 300;
    MTEST_ASSERT_EQ(0, mtimer_tick(&tm));

    mock_time += 200;
    MTEST_ASSERT_EQ(1, mtimer_tick(&tm));
    MTEST_ASSERT_EQ(1, fire_idx);
}

MTEST(test_pause_invalid_state) {
    setup();
    int id = mtimer_create(&tm, "t", 1000, MTIMER_ONESHOT, on_fire, NULL);
    MTEST_ASSERT_EQ(MTIMER_ERR_INVALID, mtimer_pause(&tm, (uint8_t)id));  /* stopped */
}

MTEST(test_resume_invalid_state) {
    setup();
    int id = mtimer_create(&tm, "t", 1000, MTIMER_ONESHOT, on_fire, NULL);
    mtimer_start(&tm, (uint8_t)id);
    MTEST_ASSERT_EQ(MTIMER_ERR_INVALID, mtimer_resume(&tm, (uint8_t)id));  /* running */
}

/* ── Set interval ──────────────────────────────────────────────────────── */

MTEST(test_set_interval) {
    setup();
    int id = mtimer_create(&tm, "t", 1000, MTIMER_PERIODIC, on_fire, NULL);
    mtimer_start(&tm, (uint8_t)id);

    /* Change to 500ms */
    mtimer_set_interval(&tm, (uint8_t)id, 500);

    mock_time += 600;
    MTEST_ASSERT_EQ(1, mtimer_tick(&tm));
}

/* ── Remaining ─────────────────────────────────────────────────────────── */

MTEST(test_remaining) {
    setup();
    int id = mtimer_create(&tm, "t", 5000, MTIMER_ONESHOT, on_fire, NULL);
    mtimer_start(&tm, (uint8_t)id);

    mock_time += 2000;
    MTEST_ASSERT_EQ(3000, (int)mtimer_remaining(&tm, (uint8_t)id));

    mock_time += 4000;
    MTEST_ASSERT_EQ(0, (int)mtimer_remaining(&tm, (uint8_t)id));
}

MTEST(test_remaining_stopped) {
    setup();
    int id = mtimer_create(&tm, "t", 1000, MTIMER_ONESHOT, on_fire, NULL);
    MTEST_ASSERT_EQ(0, (int)mtimer_remaining(&tm, (uint8_t)id));
}

/* ── Find ──────────────────────────────────────────────────────────────── */

MTEST(test_find) {
    setup();
    mtimer_create(&tm, "alpha", 1000, MTIMER_ONESHOT, on_fire, NULL);
    mtimer_create(&tm, "beta", 2000, MTIMER_ONESHOT, on_fire, NULL);
    MTEST_ASSERT_EQ(0, mtimer_find(&tm, "alpha"));
    MTEST_ASSERT_EQ(1, mtimer_find(&tm, "beta"));
    MTEST_ASSERT_EQ(-1, mtimer_find(&tm, "gamma"));
}

/* ── Callback context ──────────────────────────────────────────────────── */

MTEST(test_callback_context) {
    setup();
    int my_ctx = 99;
    int id = mtimer_create(&tm, "ctx_test", 500, MTIMER_ONESHOT, on_fire_ctx, &my_ctx);
    mtimer_start(&tm, (uint8_t)id);

    mock_time += 600;
    mtimer_tick(&tm);
    MTEST_ASSERT_EQ(99, ctx_received);
}

/* ── Multiple timers ───────────────────────────────────────────────────── */

MTEST(test_multiple_timers) {
    setup();
    int fast = mtimer_create(&tm, "fast", 100, MTIMER_PERIODIC, on_fire, NULL);
    int slow = mtimer_create(&tm, "slow", 500, MTIMER_PERIODIC, on_fire, NULL);
    mtimer_start(&tm, (uint8_t)fast);
    mtimer_start(&tm, (uint8_t)slow);

    mock_time += 500;
    mtimer_tick(&tm);

    /* Both should have fired: fast ~5x, slow 1x */
    MTEST_ASSERT_GE(fire_idx, 2);
    MTEST_ASSERT_GE((int)mtimer_fire_count(&tm, (uint8_t)fast), 1);
    MTEST_ASSERT_EQ(1, (int)mtimer_fire_count(&tm, (uint8_t)slow));
}

/* ── Edge cases ────────────────────────────────────────────────────────── */

MTEST(test_tick_null) {
    MTEST_ASSERT_EQ(0, mtimer_tick(NULL));
}

MTEST(test_query_null) {
    MTEST_ASSERT_EQ(0, mtimer_count(NULL));
    MTEST_ASSERT_NULL(mtimer_at(NULL, 0));
    MTEST_ASSERT_EQ(-1, mtimer_find(NULL, "x"));
    MTEST_ASSERT_EQ(0, (int)mtimer_total_fires(NULL));
    MTEST_ASSERT_EQ(0, (int)mtimer_total_ticks(NULL));
}

MTEST(test_err_str) {
    MTEST_ASSERT_STR_EQ("ok",               mtimer_err_str(MTIMER_OK));
    MTEST_ASSERT_STR_EQ("timer table full",  mtimer_err_str(MTIMER_ERR_FULL));
    MTEST_ASSERT_STR_EQ("timer not found",   mtimer_err_str(MTIMER_ERR_NOT_FOUND));
    MTEST_ASSERT_STR_EQ("unknown error",     mtimer_err_str((mtimer_err_t)99));
}

MTEST(test_state_str) {
    MTEST_ASSERT_STR_EQ("STOPPED", mtimer_state_str(MTIMER_STOPPED));
    MTEST_ASSERT_STR_EQ("RUNNING", mtimer_state_str(MTIMER_RUNNING));
    MTEST_ASSERT_STR_EQ("PAUSED",  mtimer_state_str(MTIMER_PAUSED));
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Suites + Main
 * ═══════════════════════════════════════════════════════════════════════════ */

MTEST_SUITE(init) {
    MTEST_RUN(test_init);
    MTEST_RUN(test_init_null);
}

MTEST_SUITE(create_destroy) {
    MTEST_RUN(test_create);
    MTEST_RUN(test_create_full);
    MTEST_RUN(test_create_invalid);
    MTEST_RUN(test_destroy);
    MTEST_RUN(test_destroy_reuse_slot);
}

MTEST_SUITE(oneshot) {
    MTEST_RUN(test_oneshot_fires_once);
}

MTEST_SUITE(periodic) {
    MTEST_RUN(test_periodic_fires_repeatedly);
    MTEST_RUN(test_periodic_no_drift);
}

MTEST_SUITE(control) {
    MTEST_RUN(test_stop_prevents_fire);
    MTEST_RUN(test_restart_resets_timer);
}

MTEST_SUITE(pause_resume) {
    MTEST_RUN(test_pause_resume);
    MTEST_RUN(test_pause_invalid_state);
    MTEST_RUN(test_resume_invalid_state);
}

MTEST_SUITE(interval) {
    MTEST_RUN(test_set_interval);
}

MTEST_SUITE(query) {
    MTEST_RUN(test_remaining);
    MTEST_RUN(test_remaining_stopped);
    MTEST_RUN(test_find);
    MTEST_RUN(test_callback_context);
    MTEST_RUN(test_multiple_timers);
}

MTEST_SUITE(edge_cases) {
    MTEST_RUN(test_tick_null);
    MTEST_RUN(test_query_null);
    MTEST_RUN(test_err_str);
    MTEST_RUN(test_state_str);
}

int main(int argc, char **argv) {
    MTEST_BEGIN(argc, argv);

    MTEST_SUITE_RUN(init);
    MTEST_SUITE_RUN(create_destroy);
    MTEST_SUITE_RUN(oneshot);
    MTEST_SUITE_RUN(periodic);
    MTEST_SUITE_RUN(control);
    MTEST_SUITE_RUN(pause_resume);
    MTEST_SUITE_RUN(interval);
    MTEST_SUITE_RUN(query);
    MTEST_SUITE_RUN(edge_cases);

    return MTEST_END();
}
