/**
 * Integration tests for the AI event + learning pipeline
 *
 * These tests verify that chains of events produce the correct combined state
 * changes across the event, learning, and tactic cancel modules.
 *
 * Note: These tests exercise state mutations only — they do not call into
 * ai_tactic_queue / ai_tactic_consider_list (which require a full game state).
 * The tactic-queuing end of the pipeline is covered by ai_tactic_engine_test.c
 * and by the regression (.REC) tests.
 */

#include "game/ai/ai_event.h"
#include "game/ai/ai_learning.h"
#include "game/ai/ai_state.h"
#include "game/ai/ai_tactic_engine.h"
#include "game/ai/ai_types.h"
#include "controller/controller.h"
#include "game/objects/har.h"
#include "resources/af_move.h"
#include "resources/ids.h"
#include "CUnit/CUnit.h"
#include <string.h>

/* -----------------------------------------------------------------------
 * Shared helpers (duplicated here to avoid inter-test-file coupling)
 * -------------------------------------------------------------------- */

static void setup_ctrl(controller *ctrl, ai *a) {
    memset(ctrl, 0, sizeof(controller));
    ctrl->data = a;
}

static void setup_ai(ai *a, sd_pilot *p, tactic_state *t) {
    memset(a, 0, sizeof(ai));
    memset(p, 0, sizeof(sd_pilot));
    memset(t, 0, sizeof(tactic_state));
    p->pilot_id = 0;
    p->forget = 0.0f;
    p->att_def = 50;
    p->att_sniper = 50;
    p->att_hyper = 50;
    p->att_jump = 50;
    p->att_normal = 50;
    p->learning = 2.0f;
    a->difficulty = 6;
    a->pilot = p;
    a->tactic = t;
}

static void make_move(af_move *m, int id, uint8_t category) {
    memset(m, 0, sizeof(af_move));
    m->id = id;
    m->category = category;
}

static void make_event(har_event *ev, uint8_t type, af_move *move) {
    memset(ev, 0, sizeof(har_event));
    ev->type = type;
    ev->move = move;
}

/* -----------------------------------------------------------------------
 * Integration: cancel → land_hit stat tracking
 * -------------------------------------------------------------------- */

/**
 * Simulate landing a hit while a non-whitelisted tactic is queued.
 * Expected: tactic is NOT cancelled by the LAND_HIT event (not in the cancel
 * switch), but move stats are still updated correctly.
 */
void test_integration_land_hit_does_not_cancel_grab_tactic(void) {
    ai a;
    sd_pilot p;
    tactic_state t;
    controller ctrl;
    af_move m;
    har_event ev;

    setup_ai(&a, &p, &t);
    setup_ctrl(&ctrl, &a);
    make_move(&m, 4, CAT_BASIC);
    make_event(&ev, HAR_EVENT_LAND_HIT, &m);

    t.tactic_type = TACTIC_GRAB;
    t.chain_hit_on = 0;
    a.move_stats[4].last_dist = 80;
    a.move_stats[4].max_hit_dist = -1;
    a.move_stats[4].min_hit_dist = -1;
    a.move_stats[4].value = 1;

    bool still_queued = ai_event_check_cancel_tactic(&ctrl, ev);
    CU_ASSERT_TRUE(still_queued);  // LAND_HIT doesn't trigger cancel

    ai_event_on_land_hit(&ctrl, ev, still_queued);

    // move stats updated despite tactic being queued
    CU_ASSERT_EQUAL(a.move_stats[4].value, 2);
    CU_ASSERT_EQUAL(a.move_stats[4].max_hit_dist, 80);
    CU_ASSERT_EQUAL(a.last_move_id, 4);
}

/**
 * Simulate a block event arriving while a CLOSE tactic is queued.
 * Expected: tactic is cancelled; the ai state reflects the cancellation.
 */
void test_integration_block_cancels_close_tactic(void) {
    ai a;
    sd_pilot p;
    tactic_state t;
    controller ctrl;
    af_move m;
    har_event ev;

    setup_ai(&a, &p, &t);
    setup_ctrl(&ctrl, &a);
    make_move(&m, 2, CAT_BASIC);
    make_event(&ev, HAR_EVENT_BLOCK, &m);

    t.tactic_type = TACTIC_CLOSE;
    t.chain_hit_on = 0;

    bool still_queued = ai_event_check_cancel_tactic(&ctrl, ev);

    CU_ASSERT_FALSE(still_queued);
    CU_ASSERT_EQUAL(t.tactic_type, 0);

    // ai_event_on_block with has_queued_tactic=false and NULL gs → smart_usually
    // may fail at difficulty 1; set difficulty low to avoid tactic queuing crash
    a.difficulty = 1;
    ai_event_on_block(&ctrl, ev, false);  // should not crash, early-returns
}

/**
 * Simulate repeated throws building up the thrown counter and triggering
 * personality adjustment after the threshold.
 */
void test_integration_repeated_throws_trigger_learning(void) {
    ai a;
    sd_pilot p;
    tactic_state t;
    controller ctrl;
    af_move m;
    har_event ev;

    setup_ai(&a, &p, &t);
    setup_ctrl(&ctrl, &a);
    make_move(&m, 1, CAT_CLOSE);
    make_event(&ev, HAR_EVENT_TAKE_HIT, &m);

    p.att_def = 95;
    p.att_sniper = 20;
    p.learning = 8.0f;  // guarantee learning_moment always returns true at diff 6

    // Simulate being thrown MAX_TIMES_THROWN times (threshold = 3)
    for(int i = 0; i < MAX_TIMES_THROWN; i++) {
        ai_event_on_take_hit(&ctrl, ev, true);
    }

    CU_ASSERT_EQUAL(a.thrown, MAX_TIMES_THROWN);
    // att_def should have been clamped down on the 3rd throw
    CU_ASSERT_EQUAL(p.att_def, 10);
}

/**
 * Simulate repeated projectile hits building up the shot counter and
 * triggering a forward-movement personality shift.
 */
void test_integration_repeated_shots_trigger_learning(void) {
    ai a;
    sd_pilot p;
    tactic_state t;
    controller ctrl;
    af_move m;
    har_event ev;

    setup_ai(&a, &p, &t);
    setup_ctrl(&ctrl, &a);
    make_move(&m, 1, CAT_PROJECTILE);
    make_event(&ev, HAR_EVENT_TAKE_HIT_PROJECTILE, &m);

    p.att_def = 95;
    p.pref_fwd = 20;
    p.pref_back = 95;
    p.learning = 8.0f;  // guarantee learning_moment always returns true at diff 6

    for(int i = 0; i < MAX_TIMES_SHOT; i++) {
        ai_event_on_take_hit(&ctrl, ev, true);
    }

    CU_ASSERT_EQUAL(a.shot, MAX_TIMES_SHOT);
    CU_ASSERT_EQUAL(p.att_def, 10);
    CU_ASSERT(p.pref_fwd > 20);
    CU_ASSERT(p.pref_back < 95);
}

/**
 * Verify that a TAKE_HIT cancel check for COUNTER tactic results in reset,
 * then a subsequent TAKE_HIT on CLOSE triggers throw learning independently.
 */
void test_integration_take_hit_cancel_then_learning(void) {
    ai a;
    sd_pilot p;
    tactic_state t;
    controller ctrl;
    af_move m_basic;
    af_move m_close;
    har_event ev_basic;
    har_event ev_close;

    setup_ai(&a, &p, &t);
    setup_ctrl(&ctrl, &a);
    make_move(&m_basic, 1, CAT_BASIC);
    make_move(&m_close, 2, CAT_CLOSE);
    make_event(&ev_basic, HAR_EVENT_TAKE_HIT, &m_basic);
    make_event(&ev_close, HAR_EVENT_TAKE_HIT, &m_close);

    // First event: COUNTER tactic + TAKE_HIT → cancel tactic
    t.tactic_type = TACTIC_COUNTER;
    bool still_queued = ai_event_check_cancel_tactic(&ctrl, ev_basic);
    CU_ASSERT_FALSE(still_queued);
    CU_ASSERT_EQUAL(t.tactic_type, 0);

    // Second event: close hit → learning counter incremented.
    // Use difficulty=1 to prevent smart_usually → ai_tactic_consider_list → crash (NULL gs).
    a.thrown = 0;
    a.difficulty = 1;
    ai_event_on_take_hit(&ctrl, ev_close, false);

    CU_ASSERT_EQUAL(a.thrown, 1);
}

/**
 * Verify the enemy_stun cancel path: tactic timer extension for GRAB, then
 * a second stun event still keeps the tactic alive.
 */
void test_integration_enemy_stun_extends_grab_timer(void) {
    ai a;
    sd_pilot p;
    tactic_state t;
    controller ctrl;
    har_event ev;

    setup_ai(&a, &p, &t);
    setup_ctrl(&ctrl, &a);
    memset(&ev, 0, sizeof(ev));
    ev.type = HAR_EVENT_ENEMY_STUN;

    t.tactic_type = TACTIC_GRAB;
    t.move_timer = 5;

    // First stun: extends timer
    bool still_queued = ai_event_check_cancel_tactic(&ctrl, ev);
    CU_ASSERT_TRUE(still_queued);
    CU_ASSERT_EQUAL(t.move_timer, TACTIC_MOVE_TIMER_MAX);

    // Second stun while still in GRAB: extend again
    t.move_timer = 0;
    still_queued = ai_event_check_cancel_tactic(&ctrl, ev);
    CU_ASSERT_TRUE(still_queued);
    CU_ASSERT_EQUAL(t.move_timer, TACTIC_MOVE_TIMER_MAX);
}

/**
 * Verify move stat hit distance tracking over multiple land hits.
 */
void test_integration_move_stat_distance_tracking(void) {
    ai a;
    sd_pilot p;
    tactic_state t;
    controller ctrl;
    af_move m;
    har_event ev;

    setup_ai(&a, &p, &t);
    setup_ctrl(&ctrl, &a);
    make_move(&m, 10, CAT_BASIC);
    make_event(&ev, HAR_EVENT_LAND_HIT, &m);

    a.move_stats[10].max_hit_dist = -1;
    a.move_stats[10].min_hit_dist = -1;
    t.chain_hit_on = 0;

    // First hit at dist=100
    a.move_stats[10].last_dist = 100;
    ai_event_on_land_hit(&ctrl, ev, true);
    CU_ASSERT_EQUAL(a.move_stats[10].max_hit_dist, 100);
    CU_ASSERT_EQUAL(a.move_stats[10].min_hit_dist, 100);

    // Second hit at dist=200 (further) → updates max
    a.move_stats[10].last_dist = 200;
    ai_event_on_land_hit(&ctrl, ev, true);
    CU_ASSERT_EQUAL(a.move_stats[10].max_hit_dist, 200);
    CU_ASSERT_EQUAL(a.move_stats[10].min_hit_dist, 100);

    // Third hit at dist=50 (closer) → updates min
    a.move_stats[10].last_dist = 50;
    ai_event_on_land_hit(&ctrl, ev, true);
    CU_ASSERT_EQUAL(a.move_stats[10].max_hit_dist, 200);
    CU_ASSERT_EQUAL(a.move_stats[10].min_hit_dist, 50);
}

/* -----------------------------------------------------------------------
 * Test suite registration
 * -------------------------------------------------------------------- */

void ai_integration_test_suite(CU_pSuite suite) {
    CU_add_test(suite, "integration: LAND_HIT does not cancel GRAB tactic",
                test_integration_land_hit_does_not_cancel_grab_tactic);
    CU_add_test(suite, "integration: BLOCK cancels CLOSE tactic cleanly",
                test_integration_block_cancels_close_tactic);
    CU_add_test(suite, "integration: repeated throws trigger learning at threshold",
                test_integration_repeated_throws_trigger_learning);
    CU_add_test(suite, "integration: repeated shots trigger learning at threshold",
                test_integration_repeated_shots_trigger_learning);
    CU_add_test(suite, "integration: cancel then learning work independently",
                test_integration_take_hit_cancel_then_learning);
    CU_add_test(suite, "integration: ENEMY_STUN extends GRAB timer repeatedly",
                test_integration_enemy_stun_extends_grab_timer);
    CU_add_test(suite, "integration: move stat distance tracking over multiple hits",
                test_integration_move_stat_distance_tracking);
}
