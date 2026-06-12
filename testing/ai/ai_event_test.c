/**
 * Unit tests for AI event handlers
 *
 * Tests: ai_event_check_cancel_tactic (tactic cancellation logic)
 *        ai_event_on_land_hit (move stat tracking)
 *        ai_event_on_enemy_block (blocked-flag logic)
 *        ai_event_on_block (counter-attack trigger)
 *        ai_event_on_take_hit (learning delegation)
 *        ai_event_on_land (act_timer reset)
 *
 * Note: Handlers that call ai_tactic_consider_list require a full game state
 * (via ai_tactic_queue → game_state_find_object). Tests here exercise only
 * the paths that return early or modify state without queuing new tactics.
 * The tactic-queuing paths are validated by the existing tactic engine tests
 * and by the integration tests.
 */

#include "game/ai/ai_event.h"
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
 * Minimal mock helpers
 * -------------------------------------------------------------------- */

/** Build a minimal controller whose ->data points at the provided ai struct. */
static void make_mock_ctrl(controller *ctrl, ai *a) {
    memset(ctrl, 0, sizeof(controller));
    ctrl->data = a;
    ctrl->gs = NULL;
    ctrl->har_obj_id = 0;
}

/** Populate an ai struct with sensible defaults for event tests. */
static void make_mock_ai(ai *a, sd_pilot *p, tactic_state *t) {
    memset(a, 0, sizeof(ai));
    memset(p, 0, sizeof(sd_pilot));
    memset(t, 0, sizeof(tactic_state));

    p->pilot_id = 0;
    p->forget = 0.0f;     // never forgets — keeps tests deterministic
    p->att_def = 50;
    p->att_sniper = 50;
    p->att_hyper = 50;
    p->att_jump = 50;
    p->att_normal = 50;
    p->learning = 2.0f;

    a->difficulty = 6;    // high difficulty → smart_usually() usually true
    a->pilot = p;
    a->tactic = t;
}

/** Build a minimal har_event with a given type and a mock move. */
static void make_event(har_event *ev, uint8_t type, af_move *move) {
    memset(ev, 0, sizeof(har_event));
    ev->type = type;
    ev->move = move;
}

/** Build a mock af_move with a given id and category. */
static void make_move(af_move *m, int id, uint8_t category) {
    memset(m, 0, sizeof(af_move));
    m->id = id;
    m->category = category;
}

/* -----------------------------------------------------------------------
 * ai_event_check_cancel_tactic
 * -------------------------------------------------------------------- */

void test_event_cancel_no_tactic_returns_false(void) {
    ai a;
    sd_pilot p;
    tactic_state t;
    controller ctrl;
    af_move m;
    har_event ev;
    make_mock_ai(&a, &p, &t);
    make_mock_ctrl(&ctrl, &a);
    make_move(&m, 1, CAT_BASIC);
    make_event(&ev, HAR_EVENT_BLOCK, &m);

    // No tactic queued — should always return false
    t.tactic_type = 0;
    CU_ASSERT_FALSE(ai_event_check_cancel_tactic(&ctrl, ev));
}

void test_event_cancel_block_resets_close_tactic(void) {
    ai a;
    sd_pilot p;
    tactic_state t;
    controller ctrl;
    af_move m;
    har_event ev;
    make_mock_ai(&a, &p, &t);
    make_mock_ctrl(&ctrl, &a);
    make_move(&m, 1, CAT_BASIC);
    make_event(&ev, HAR_EVENT_BLOCK, &m);

    t.tactic_type = TACTIC_CLOSE;  // not in whitelist → should be reset
    t.chain_hit_on = 0;
    bool still_queued = ai_event_check_cancel_tactic(&ctrl, ev);

    CU_ASSERT_FALSE(still_queued);
    CU_ASSERT_EQUAL(t.tactic_type, 0);
}

void test_event_cancel_block_keeps_counter_tactic(void) {
    ai a;
    sd_pilot p;
    tactic_state t;
    controller ctrl;
    af_move m;
    har_event ev;
    make_mock_ai(&a, &p, &t);
    make_mock_ctrl(&ctrl, &a);
    make_move(&m, 1, CAT_BASIC);
    make_event(&ev, HAR_EVENT_BLOCK, &m);

    t.tactic_type = TACTIC_COUNTER;  // whitelisted
    bool still_queued = ai_event_check_cancel_tactic(&ctrl, ev);

    CU_ASSERT_TRUE(still_queued);
    CU_ASSERT_EQUAL(t.tactic_type, TACTIC_COUNTER);
}

void test_event_cancel_block_keeps_turtle_tactic(void) {
    ai a;
    sd_pilot p;
    tactic_state t;
    controller ctrl;
    af_move m;
    har_event ev;
    make_mock_ai(&a, &p, &t);
    make_mock_ctrl(&ctrl, &a);
    make_move(&m, 1, CAT_BASIC);
    make_event(&ev, HAR_EVENT_BLOCK, &m);

    t.tactic_type = TACTIC_TURTLE;
    bool still_queued = ai_event_check_cancel_tactic(&ctrl, ev);

    CU_ASSERT_TRUE(still_queued);
}

void test_event_cancel_block_keeps_trip_tactic(void) {
    ai a;
    sd_pilot p;
    tactic_state t;
    controller ctrl;
    af_move m;
    har_event ev;
    make_mock_ai(&a, &p, &t);
    make_mock_ctrl(&ctrl, &a);
    make_move(&m, 1, CAT_BASIC);
    make_event(&ev, HAR_EVENT_BLOCK, &m);

    t.tactic_type = TACTIC_TRIP;
    bool still_queued = ai_event_check_cancel_tactic(&ctrl, ev);

    CU_ASSERT_TRUE(still_queued);
}

void test_event_cancel_block_keeps_push_tactic(void) {
    ai a;
    sd_pilot p;
    tactic_state t;
    controller ctrl;
    af_move m;
    har_event ev;
    make_mock_ai(&a, &p, &t);
    make_mock_ctrl(&ctrl, &a);
    make_move(&m, 1, CAT_BASIC);
    make_event(&ev, HAR_EVENT_BLOCK, &m);

    t.tactic_type = TACTIC_PUSH;
    bool still_queued = ai_event_check_cancel_tactic(&ctrl, ev);

    CU_ASSERT_TRUE(still_queued);
}

void test_event_cancel_block_keeps_fly_tactic(void) {
    ai a;
    sd_pilot p;
    tactic_state t;
    controller ctrl;
    af_move m;
    har_event ev;
    make_mock_ai(&a, &p, &t);
    make_mock_ctrl(&ctrl, &a);
    make_move(&m, 1, CAT_BASIC);
    make_event(&ev, HAR_EVENT_BLOCK, &m);

    t.tactic_type = TACTIC_FLY;
    bool still_queued = ai_event_check_cancel_tactic(&ctrl, ev);

    CU_ASSERT_TRUE(still_queued);
}

void test_event_cancel_block_keeps_spam_tactic(void) {
    ai a;
    sd_pilot p;
    tactic_state t;
    controller ctrl;
    af_move m;
    har_event ev;
    make_mock_ai(&a, &p, &t);
    make_mock_ctrl(&ctrl, &a);
    make_move(&m, 1, CAT_BASIC);
    make_event(&ev, HAR_EVENT_BLOCK, &m);

    t.tactic_type = TACTIC_SPAM;
    bool still_queued = ai_event_check_cancel_tactic(&ctrl, ev);

    CU_ASSERT_TRUE(still_queued);
}

void test_event_cancel_block_chain_hit_on_keeps_tactic(void) {
    ai a;
    sd_pilot p;
    tactic_state t;
    controller ctrl;
    af_move m;
    har_event ev;
    make_mock_ai(&a, &p, &t);
    make_mock_ctrl(&ctrl, &a);
    // Move category matches chain_hit_on → keep tactic regardless of type
    make_move(&m, 1, CAT_MEDIUM);
    make_event(&ev, HAR_EVENT_BLOCK, &m);

    t.tactic_type = TACTIC_CLOSE;  // would normally be reset
    t.chain_hit_on = CAT_MEDIUM;   // but chain_hit_on matches → keep
    bool still_queued = ai_event_check_cancel_tactic(&ctrl, ev);

    CU_ASSERT_TRUE(still_queued);
}

void test_event_cancel_take_hit_resets_close_tactic(void) {
    ai a;
    sd_pilot p;
    tactic_state t;
    controller ctrl;
    af_move m;
    har_event ev;
    make_mock_ai(&a, &p, &t);
    make_mock_ctrl(&ctrl, &a);
    make_move(&m, 1, CAT_BASIC);
    make_event(&ev, HAR_EVENT_TAKE_HIT, &m);

    t.tactic_type = TACTIC_CLOSE;
    bool still_queued = ai_event_check_cancel_tactic(&ctrl, ev);

    CU_ASSERT_FALSE(still_queued);
    CU_ASSERT_EQUAL(t.tactic_type, 0);
}

void test_event_cancel_take_hit_resets_fly_tactic(void) {
    ai a;
    sd_pilot p;
    tactic_state t;
    controller ctrl;
    af_move m;
    har_event ev;
    make_mock_ai(&a, &p, &t);
    make_mock_ctrl(&ctrl, &a);
    make_move(&m, 1, CAT_BASIC);
    make_event(&ev, HAR_EVENT_TAKE_HIT, &m);

    t.tactic_type = TACTIC_FLY;
    bool still_queued = ai_event_check_cancel_tactic(&ctrl, ev);

    CU_ASSERT_FALSE(still_queued);
}

void test_event_cancel_take_hit_keeps_grab_tactic(void) {
    ai a;
    sd_pilot p;
    tactic_state t;
    controller ctrl;
    af_move m;
    har_event ev;
    make_mock_ai(&a, &p, &t);
    make_mock_ctrl(&ctrl, &a);
    make_move(&m, 1, CAT_BASIC);
    make_event(&ev, HAR_EVENT_TAKE_HIT, &m);

    t.tactic_type = TACTIC_GRAB;  // not in reset list for TAKE_HIT
    bool still_queued = ai_event_check_cancel_tactic(&ctrl, ev);

    CU_ASSERT_TRUE(still_queued);
}

void test_event_cancel_take_hit_resets_turtle_when_no_def(void) {
    ai a;
    sd_pilot p;
    tactic_state t;
    controller ctrl;
    af_move m;
    har_event ev;
    make_mock_ai(&a, &p, &t);
    make_mock_ctrl(&ctrl, &a);
    make_move(&m, 1, CAT_BASIC);
    make_event(&ev, HAR_EVENT_TAKE_HIT, &m);

    t.tactic_type = TACTIC_TURTLE;
    p.att_def = 0;  // no defensive preference → reset turtle
    bool still_queued = ai_event_check_cancel_tactic(&ctrl, ev);

    CU_ASSERT_FALSE(still_queued);
}

void test_event_cancel_take_hit_keeps_turtle_with_def(void) {
    ai a;
    sd_pilot p;
    tactic_state t;
    controller ctrl;
    af_move m;
    har_event ev;
    make_mock_ai(&a, &p, &t);
    make_mock_ctrl(&ctrl, &a);
    make_move(&m, 1, CAT_BASIC);
    make_event(&ev, HAR_EVENT_TAKE_HIT, &m);

    t.tactic_type = TACTIC_TURTLE;
    p.att_def = 70;  // has defensive preference → keep turtle
    bool still_queued = ai_event_check_cancel_tactic(&ctrl, ev);

    CU_ASSERT_TRUE(still_queued);
}

void test_event_cancel_enemy_stun_extends_timer_for_grab(void) {
    ai a;
    sd_pilot p;
    tactic_state t;
    controller ctrl;
    har_event ev;
    make_mock_ai(&a, &p, &t);
    make_mock_ctrl(&ctrl, &a);
    memset(&ev, 0, sizeof(ev));
    ev.type = HAR_EVENT_ENEMY_STUN;

    t.tactic_type = TACTIC_GRAB;
    t.move_timer = 0;
    bool still_queued = ai_event_check_cancel_tactic(&ctrl, ev);

    CU_ASSERT_TRUE(still_queued);
    CU_ASSERT_EQUAL(t.move_timer, TACTIC_MOVE_TIMER_MAX);
}

void test_event_cancel_enemy_stun_extends_timer_for_trip(void) {
    ai a;
    sd_pilot p;
    tactic_state t;
    controller ctrl;
    har_event ev;
    make_mock_ai(&a, &p, &t);
    make_mock_ctrl(&ctrl, &a);
    memset(&ev, 0, sizeof(ev));
    ev.type = HAR_EVENT_ENEMY_STUN;

    t.tactic_type = TACTIC_TRIP;
    t.move_timer = 0;
    bool still_queued = ai_event_check_cancel_tactic(&ctrl, ev);

    CU_ASSERT_TRUE(still_queued);
    CU_ASSERT_EQUAL(t.move_timer, TACTIC_MOVE_TIMER_MAX);
}

void test_event_cancel_enemy_stun_keeps_shoot_tactic(void) {
    ai a;
    sd_pilot p;
    tactic_state t;
    controller ctrl;
    har_event ev;
    make_mock_ai(&a, &p, &t);
    make_mock_ctrl(&ctrl, &a);
    memset(&ev, 0, sizeof(ev));
    ev.type = HAR_EVENT_ENEMY_STUN;

    t.tactic_type = TACTIC_SHOOT;
    t.move_timer = 5;
    bool still_queued = ai_event_check_cancel_tactic(&ctrl, ev);

    CU_ASSERT_TRUE(still_queued);
    // Timer should NOT be extended for SHOOT (not grab/close/trip)
    CU_ASSERT_EQUAL(t.move_timer, 5);
}

void test_event_cancel_enemy_stun_resets_non_shoot_non_grab(void) {
    ai a;
    sd_pilot p;
    tactic_state t;
    controller ctrl;
    har_event ev;
    make_mock_ai(&a, &p, &t);
    make_mock_ctrl(&ctrl, &a);
    memset(&ev, 0, sizeof(ev));
    ev.type = HAR_EVENT_ENEMY_STUN;

    t.tactic_type = TACTIC_ESCAPE;  // not GRAB/CLOSE/TRIP/SHOOT → reset
    bool still_queued = ai_event_check_cancel_tactic(&ctrl, ev);

    CU_ASSERT_FALSE(still_queued);
    CU_ASSERT_EQUAL(t.tactic_type, 0);
}

void test_event_cancel_other_events_leave_tactic_unchanged(void) {
    ai a;
    sd_pilot p;
    tactic_state t;
    controller ctrl;
    har_event ev;
    make_mock_ai(&a, &p, &t);
    make_mock_ctrl(&ctrl, &a);
    memset(&ev, 0, sizeof(ev));
    ev.type = HAR_EVENT_LAND;  // not handled in cancel switch

    t.tactic_type = TACTIC_SPAM;
    bool still_queued = ai_event_check_cancel_tactic(&ctrl, ev);

    CU_ASSERT_TRUE(still_queued);
    CU_ASSERT_EQUAL(t.tactic_type, TACTIC_SPAM);
}

/* -----------------------------------------------------------------------
 * ai_event_on_land_hit — move stat tracking
 * -------------------------------------------------------------------- */

void test_event_land_hit_increments_move_value(void) {
    ai a;
    sd_pilot p;
    tactic_state t;
    controller ctrl;
    af_move m;
    har_event ev;
    make_mock_ai(&a, &p, &t);
    make_mock_ctrl(&ctrl, &a);
    make_move(&m, 3, CAT_BASIC);
    make_event(&ev, HAR_EVENT_LAND_HIT, &m);

    a.move_stats[3].value = 2;
    a.move_stats[3].last_dist = 100;
    a.move_stats[3].max_hit_dist = -1;
    a.move_stats[3].min_hit_dist = -1;
    t.chain_hit_on = 0;

    // has_queued_tactic=true → early exit after stat update, no tactic queuing
    ai_event_on_land_hit(&ctrl, ev, true);

    CU_ASSERT_EQUAL(a.move_stats[3].value, 3);
}

void test_event_land_hit_clamps_move_value_at_10(void) {
    ai a;
    sd_pilot p;
    tactic_state t;
    controller ctrl;
    af_move m;
    har_event ev;
    make_mock_ai(&a, &p, &t);
    make_mock_ctrl(&ctrl, &a);
    make_move(&m, 3, CAT_BASIC);
    make_event(&ev, HAR_EVENT_LAND_HIT, &m);

    a.move_stats[3].value = 10;
    a.move_stats[3].last_dist = 100;
    a.move_stats[3].max_hit_dist = -1;
    a.move_stats[3].min_hit_dist = -1;
    t.chain_hit_on = 0;

    ai_event_on_land_hit(&ctrl, ev, true);

    CU_ASSERT_EQUAL(a.move_stats[3].value, 10);
}

void test_event_land_hit_updates_last_move_id(void) {
    ai a;
    sd_pilot p;
    tactic_state t;
    controller ctrl;
    af_move m;
    har_event ev;
    make_mock_ai(&a, &p, &t);
    make_mock_ctrl(&ctrl, &a);
    make_move(&m, 7, CAT_BASIC);
    make_event(&ev, HAR_EVENT_LAND_HIT, &m);

    a.move_stats[7].last_dist = 50;
    a.move_stats[7].max_hit_dist = -1;
    a.move_stats[7].min_hit_dist = -1;
    t.chain_hit_on = 0;
    a.last_move_id = 0;

    ai_event_on_land_hit(&ctrl, ev, true);

    CU_ASSERT_EQUAL(a.last_move_id, 7);
}

void test_event_land_hit_updates_max_hit_dist(void) {
    ai a;
    sd_pilot p;
    tactic_state t;
    controller ctrl;
    af_move m;
    har_event ev;
    make_mock_ai(&a, &p, &t);
    make_mock_ctrl(&ctrl, &a);
    make_move(&m, 2, CAT_BASIC);
    make_event(&ev, HAR_EVENT_LAND_HIT, &m);

    a.move_stats[2].last_dist = 200;
    a.move_stats[2].max_hit_dist = -1;
    a.move_stats[2].min_hit_dist = -1;
    t.chain_hit_on = 0;

    ai_event_on_land_hit(&ctrl, ev, true);

    CU_ASSERT_EQUAL(a.move_stats[2].max_hit_dist, 200);
}

void test_event_land_hit_updates_min_hit_dist(void) {
    ai a;
    sd_pilot p;
    tactic_state t;
    controller ctrl;
    af_move m;
    har_event ev;
    make_mock_ai(&a, &p, &t);
    make_mock_ctrl(&ctrl, &a);
    make_move(&m, 2, CAT_BASIC);
    make_event(&ev, HAR_EVENT_LAND_HIT, &m);

    a.move_stats[2].last_dist = 50;
    a.move_stats[2].max_hit_dist = 200;
    a.move_stats[2].min_hit_dist = -1;
    t.chain_hit_on = 0;

    ai_event_on_land_hit(&ctrl, ev, true);

    CU_ASSERT_EQUAL(a.move_stats[2].min_hit_dist, 50);
}

/* -----------------------------------------------------------------------
 * ai_event_on_enemy_block — blocked-flag logic
 * -------------------------------------------------------------------- */

void test_event_enemy_block_sets_blocked_flag(void) {
    ai a;
    sd_pilot p;
    tactic_state t;
    controller ctrl;
    af_move m;
    har_event ev;
    make_mock_ai(&a, &p, &t);
    make_mock_ctrl(&ctrl, &a);
    make_move(&m, 5, CAT_BASIC);
    make_event(&ev, HAR_EVENT_ENEMY_BLOCK, &m);

    a.blocked = 0;
    a.move_stats[5].value = 3;

    // has_queued_tactic=true → early exit after setting blocked
    ai_event_on_enemy_block(&ctrl, ev, true);

    CU_ASSERT_EQUAL(a.blocked, 1);
}

void test_event_enemy_block_decrements_move_value(void) {
    ai a;
    sd_pilot p;
    tactic_state t;
    controller ctrl;
    af_move m;
    har_event ev;
    make_mock_ai(&a, &p, &t);
    make_mock_ctrl(&ctrl, &a);
    make_move(&m, 5, CAT_BASIC);
    make_event(&ev, HAR_EVENT_ENEMY_BLOCK, &m);

    a.blocked = 0;
    a.move_stats[5].value = 3;

    ai_event_on_enemy_block(&ctrl, ev, true);

    CU_ASSERT_EQUAL(a.move_stats[5].value, 2);
}

void test_event_enemy_block_ignores_repeat_block(void) {
    ai a;
    sd_pilot p;
    tactic_state t;
    controller ctrl;
    af_move m;
    har_event ev;
    make_mock_ai(&a, &p, &t);
    make_mock_ctrl(&ctrl, &a);
    make_move(&m, 5, CAT_BASIC);
    make_event(&ev, HAR_EVENT_ENEMY_BLOCK, &m);

    a.blocked = 1;  // already blocked — second call should be a no-op
    a.move_stats[5].value = 3;

    ai_event_on_enemy_block(&ctrl, ev, true);

    // No change: value stays at 3
    CU_ASSERT_EQUAL(a.move_stats[5].value, 3);
    CU_ASSERT_EQUAL(a.blocked, 1);
}

/* -----------------------------------------------------------------------
 * ai_event_on_block — counter-attack trigger
 * -------------------------------------------------------------------- */

void test_event_block_counter_trigger_sets_move_timer_zero(void) {
    ai a;
    sd_pilot p;
    tactic_state t;
    controller ctrl;
    af_move m;
    har_event ev;
    make_mock_ai(&a, &p, &t);
    make_mock_ctrl(&ctrl, &a);
    make_move(&m, 1, CAT_BASIC);
    make_event(&ev, HAR_EVENT_BLOCK, &m);

    t.tactic_type = TACTIC_COUNTER;
    t.attack_on = HAR_EVENT_BLOCK;
    t.move_timer = 15;

    // has_queued_tactic=true AND attack_on==BLOCK → trigger counter
    ai_event_on_block(&ctrl, ev, true);

    CU_ASSERT_EQUAL(t.move_timer, 0);
}

void test_event_block_no_counter_when_attack_on_mismatch(void) {
    ai a;
    sd_pilot p;
    tactic_state t;
    controller ctrl;
    af_move m;
    har_event ev;
    make_mock_ai(&a, &p, &t);
    make_mock_ctrl(&ctrl, &a);
    make_move(&m, 1, CAT_BASIC);
    make_event(&ev, HAR_EVENT_BLOCK, &m);

    t.tactic_type = TACTIC_COUNTER;
    t.attack_on = HAR_EVENT_LAND;  // different trigger → no immediate counter
    t.move_timer = 15;

    // has_queued_tactic=true but attack_on doesn't match → early exit
    ai_event_on_block(&ctrl, ev, true);

    // Timer unchanged
    CU_ASSERT_EQUAL(t.move_timer, 15);
}

/* -----------------------------------------------------------------------
 * ai_event_on_take_hit — learning delegation
 * -------------------------------------------------------------------- */

void test_event_take_hit_increments_thrown_on_close_move(void) {
    ai a;
    sd_pilot p;
    tactic_state t;
    controller ctrl;
    af_move m;
    har_event ev;
    make_mock_ai(&a, &p, &t);
    make_mock_ctrl(&ctrl, &a);
    make_move(&m, 1, CAT_CLOSE);
    make_event(&ev, HAR_EVENT_TAKE_HIT, &m);

    a.thrown = 0;
    // has_queued_tactic=true → no tactic queuing needed
    ai_event_on_take_hit(&ctrl, ev, true);

    CU_ASSERT_EQUAL(a.thrown, 1);
}

void test_event_take_hit_increments_shot_on_projectile_event(void) {
    ai a;
    sd_pilot p;
    tactic_state t;
    controller ctrl;
    af_move m;
    har_event ev;
    make_mock_ai(&a, &p, &t);
    make_mock_ctrl(&ctrl, &a);
    make_move(&m, 1, CAT_PROJECTILE);
    make_event(&ev, HAR_EVENT_TAKE_HIT_PROJECTILE, &m);

    a.shot = 0;
    ai_event_on_take_hit(&ctrl, ev, true);

    CU_ASSERT_EQUAL(a.shot, 1);
}

void test_event_take_hit_does_not_increment_thrown_for_basic(void) {
    ai a;
    sd_pilot p;
    tactic_state t;
    controller ctrl;
    af_move m;
    har_event ev;
    make_mock_ai(&a, &p, &t);
    make_mock_ctrl(&ctrl, &a);
    make_move(&m, 1, CAT_BASIC);
    make_event(&ev, HAR_EVENT_TAKE_HIT, &m);

    a.thrown = 0;
    a.shot = 0;
    ai_event_on_take_hit(&ctrl, ev, true);

    CU_ASSERT_EQUAL(a.thrown, 0);
    CU_ASSERT_EQUAL(a.shot, 0);
}

void test_event_take_hit_does_not_increment_shot_for_close_hit(void) {
    ai a;
    sd_pilot p;
    tactic_state t;
    controller ctrl;
    af_move m;
    har_event ev;
    make_mock_ai(&a, &p, &t);
    make_mock_ctrl(&ctrl, &a);
    // CAT_CLOSE via a TAKE_HIT (not TAKE_HIT_PROJECTILE)
    make_move(&m, 1, CAT_CLOSE);
    make_event(&ev, HAR_EVENT_TAKE_HIT, &m);

    a.thrown = 0;
    a.shot = 0;
    ai_event_on_take_hit(&ctrl, ev, true);

    CU_ASSERT_EQUAL(a.shot, 0);
    CU_ASSERT_EQUAL(a.thrown, 1);  // only thrown incremented
}

/* -----------------------------------------------------------------------
 * ai_event_on_land — act_timer reset
 * -------------------------------------------------------------------- */

void test_event_land_resets_act_timer(void) {
    ai a;
    sd_pilot p;
    tactic_state t;
    controller ctrl;
    har_event ev;
    make_mock_ai(&a, &p, &t);
    make_mock_ctrl(&ctrl, &a);
    memset(&ev, 0, sizeof(ev));
    ev.type = HAR_EVENT_LAND;

    a.act_timer = 99;
    t.attack_on = 0;  // no pending land attack

    // has_queued_tactic=false, attack_on=0 → act_timer reset path
    // (smart_usually at difficulty 6 with high att_* → may queue tactic via
    //  ai_tactic_consider_list, but with NULL gs that path is skipped when
    //  smart_usually returns false — set difficulty=1 to be safe)
    a.difficulty = 1;
    ai_event_on_land(&ctrl, ev, false);

    CU_ASSERT_EQUAL(a.act_timer, 0);
}

void test_event_land_queued_tactic_without_attack_on_resets_act_timer(void) {
    ai a;
    sd_pilot p;
    tactic_state t;
    controller ctrl;
    har_event ev;
    make_mock_ai(&a, &p, &t);
    make_mock_ctrl(&ctrl, &a);
    memset(&ev, 0, sizeof(ev));
    ev.type = HAR_EVENT_LAND;

    a.act_timer = 50;
    t.tactic_type = TACTIC_GRAB;
    t.attack_on = 0;  // queued tactic but not waiting for land event

    ai_event_on_land(&ctrl, ev, true);

    // act_timer should be reset because attack_on doesn't match
    CU_ASSERT_EQUAL(a.act_timer, 0);
}

/* -----------------------------------------------------------------------
 * Test suite registration
 * -------------------------------------------------------------------- */

void ai_event_test_suite(CU_pSuite suite) {
    // ai_event_check_cancel_tactic
    CU_add_test(suite, "cancel: no tactic → returns false",
                test_event_cancel_no_tactic_returns_false);
    CU_add_test(suite, "cancel: BLOCK resets CLOSE tactic",
                test_event_cancel_block_resets_close_tactic);
    CU_add_test(suite, "cancel: BLOCK keeps COUNTER tactic",
                test_event_cancel_block_keeps_counter_tactic);
    CU_add_test(suite, "cancel: BLOCK keeps TURTLE tactic",
                test_event_cancel_block_keeps_turtle_tactic);
    CU_add_test(suite, "cancel: BLOCK keeps TRIP tactic",
                test_event_cancel_block_keeps_trip_tactic);
    CU_add_test(suite, "cancel: BLOCK keeps PUSH tactic",
                test_event_cancel_block_keeps_push_tactic);
    CU_add_test(suite, "cancel: BLOCK keeps FLY tactic",
                test_event_cancel_block_keeps_fly_tactic);
    CU_add_test(suite, "cancel: BLOCK keeps SPAM tactic",
                test_event_cancel_block_keeps_spam_tactic);
    CU_add_test(suite, "cancel: BLOCK with chain_hit_on match keeps tactic",
                test_event_cancel_block_chain_hit_on_keeps_tactic);
    CU_add_test(suite, "cancel: TAKE_HIT resets CLOSE tactic",
                test_event_cancel_take_hit_resets_close_tactic);
    CU_add_test(suite, "cancel: TAKE_HIT resets FLY tactic",
                test_event_cancel_take_hit_resets_fly_tactic);
    CU_add_test(suite, "cancel: TAKE_HIT keeps GRAB tactic",
                test_event_cancel_take_hit_keeps_grab_tactic);
    CU_add_test(suite, "cancel: TAKE_HIT resets TURTLE when att_def=0",
                test_event_cancel_take_hit_resets_turtle_when_no_def);
    CU_add_test(suite, "cancel: TAKE_HIT keeps TURTLE when att_def>0",
                test_event_cancel_take_hit_keeps_turtle_with_def);
    CU_add_test(suite, "cancel: ENEMY_STUN extends move_timer for GRAB",
                test_event_cancel_enemy_stun_extends_timer_for_grab);
    CU_add_test(suite, "cancel: ENEMY_STUN extends move_timer for TRIP",
                test_event_cancel_enemy_stun_extends_timer_for_trip);
    CU_add_test(suite, "cancel: ENEMY_STUN keeps SHOOT tactic unchanged",
                test_event_cancel_enemy_stun_keeps_shoot_tactic);
    CU_add_test(suite, "cancel: ENEMY_STUN resets non-GRAB/CLOSE/TRIP/SHOOT",
                test_event_cancel_enemy_stun_resets_non_shoot_non_grab);
    CU_add_test(suite, "cancel: unhandled event leaves tactic unchanged",
                test_event_cancel_other_events_leave_tactic_unchanged);

    // ai_event_on_land_hit
    CU_add_test(suite, "land_hit: increments move value",
                test_event_land_hit_increments_move_value);
    CU_add_test(suite, "land_hit: clamps move value at 10",
                test_event_land_hit_clamps_move_value_at_10);
    CU_add_test(suite, "land_hit: updates last_move_id",
                test_event_land_hit_updates_last_move_id);
    CU_add_test(suite, "land_hit: updates max_hit_dist",
                test_event_land_hit_updates_max_hit_dist);
    CU_add_test(suite, "land_hit: updates min_hit_dist",
                test_event_land_hit_updates_min_hit_dist);

    // ai_event_on_enemy_block
    CU_add_test(suite, "enemy_block: sets blocked flag",
                test_event_enemy_block_sets_blocked_flag);
    CU_add_test(suite, "enemy_block: decrements move value",
                test_event_enemy_block_decrements_move_value);
    CU_add_test(suite, "enemy_block: ignores repeat block events",
                test_event_enemy_block_ignores_repeat_block);

    // ai_event_on_block
    CU_add_test(suite, "block: counter trigger resets move_timer to 0",
                test_event_block_counter_trigger_sets_move_timer_zero);
    CU_add_test(suite, "block: no counter when attack_on does not match",
                test_event_block_no_counter_when_attack_on_mismatch);

    // ai_event_on_take_hit
    CU_add_test(suite, "take_hit: increments thrown counter on CAT_CLOSE",
                test_event_take_hit_increments_thrown_on_close_move);
    CU_add_test(suite, "take_hit: increments shot counter on projectile event",
                test_event_take_hit_increments_shot_on_projectile_event);
    CU_add_test(suite, "take_hit: does not increment thrown for basic hit",
                test_event_take_hit_does_not_increment_thrown_for_basic);
    CU_add_test(suite, "take_hit: does not increment shot for close hit",
                test_event_take_hit_does_not_increment_shot_for_close_hit);

    // ai_event_on_land
    CU_add_test(suite, "land: resets act_timer (no attack_on, difficulty 1)",
                test_event_land_resets_act_timer);
    CU_add_test(suite, "land: resets act_timer when tactic queued but not waiting for land",
                test_event_land_queued_tactic_without_attack_on_resets_act_timer);
}
