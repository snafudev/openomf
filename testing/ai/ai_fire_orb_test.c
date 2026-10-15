/**
 * Deterministic tests for the fire orb punch path (ARENA4 fire pit).
 *
 * These drive the real har_collide_with_hazard() against a synthetic orb so we
 * don't depend on the original BK/AF game data. The orb gets a single solid
 * 20x20 sprite and a "chain_hit" animation; the HAR gets an attack animation
 * whose two hitpoints land inside the orb sprite when lined up. Events are
 * captured through the controller har_hook, either with a recorder or with the
 * real ai_har_event() callback.
 */

#include "CUnit/CUnit.h"
#include "controller/ai_controller.h"
#include "controller/controller.h"
#include "game/ai/ai_tactic_engine.h"
#include "game/ai/ai_types.h"
#include "game/game_player.h"
#include "game/game_state.h"
#include "game/objects/arena_constraints.h"
#include "game/objects/har.h"
#include "resources/bk.h"
#include "utils/allocator.h"
#include "utils/log.h"

#include <string.h>

enum
{
    ORB_ANIM_IDLE = 1,
    ORB_ANIM_PUNCHED = 2,
    HAR_ANIM_ATTACK = 20,
    MAX_RECORDED_EVENTS = 8,
};

// Mirrors render_obj in game_state.c, which game_state_find_object() iterates
typedef struct {
    int layer;
    int persistent;
    int singleton;
    object *obj;
} orb_test_render_obj;

typedef struct {
    game_state gs;
    game_player players[2];
    controller ctrl;
    object har_obj;
    object orb_obj;
    har har_data;
    animation har_attack;
    bk orb_bk;
} orb_fixture;

static orb_fixture fx;
static har_event recorded[MAX_RECORDED_EVENTS];
static int recorded_count;

static void ensure_test_runtime_ready(void) {
    // Another suite may have initialised logging already
    if(log_is_initialized()) {
        return;
    }
    log_init();
    log_add_stderr(LOG_ERROR, false);
}

static int record_har_event(controller *ctrl, har_event event) {
    (void)ctrl;
    if(recorded_count < MAX_RECORDED_EVENTS) {
        recorded[recorded_count] = event;
    }
    recorded_count++;
    return 0;
}

/** Single-sprite animation with a solid (non-transparent) w x h surface. */
static animation make_solid_animation(int id, vec2i sprite_pos, int w, int h) {
    surface *sfc = omf_calloc(1, sizeof(surface));
    surface_create(sfc, w, h);
    // vga_pixel may be 16-bit, so fill per pixel rather than memset
    for(int i = 0; i < w * h; i++) {
        sfc->data[i] = 1;
    }
    sprite *sp = omf_calloc(1, sizeof(sprite));
    sprite_create_custom(sp, sprite_pos, sfc);
    sp->owned = true;

    animation *tmp = create_animation_from_single(sp, vec2i_create(0, 0));
    animation ani = *tmp;
    omf_free(tmp);
    ani.id = id;
    return ani;
}

static void put_orb_info(int id, unsigned int chain_hit) {
    bk_info info;
    memset(&info, 0, sizeof(info));
    info.chain_hit = chain_hit;
    info.ani = make_solid_animation(id, vec2i_create(0, 0), 20, 20);
    hashmap_put_int(&fx.orb_bk.infos, id, &info, sizeof(info));
}

static void set_orb_animation(int id) {
    object_set_animation(&fx.orb_obj, &bk_get_info(&fx.orb_bk, id)->ani);
    fx.orb_obj.cur_sprite_id = 0;
}

/**
 * HAR at (har_x, har_y) with an attack frame whose hitpoints sit at
 * (+40, -30) and (+41, -30) from its position. The orb is placed so those
 * hitpoints land inside its 20x20 sprite when `lined_up` is true.
 */
static void setup_fixture(float har_y, bool lined_up) {
    ensure_test_runtime_ready();
    memset(&fx, 0, sizeof(fx));
    memset(recorded, 0, sizeof(recorded));
    recorded_count = 0;

    vector_create(&fx.gs.objects, sizeof(orb_test_render_obj));
    fx.gs.players[0] = &fx.players[0];
    fx.gs.players[1] = &fx.players[1];

    object_create(&fx.har_obj, &fx.gs, vec2i_create(100, (int)har_y), vec2f_create(0, 0));
    object_set_group(&fx.har_obj, GROUP_HAR);
    fx.har_data.player_id = 0;
    fx.har_data.state = har_y < ARENA_FLOOR ? STATE_JUMPING : STATE_STANDING;
    list_create(&fx.har_data.har_hooks);
    object_set_userdata(&fx.har_obj, &fx.har_data);

    fx.har_attack = make_solid_animation(HAR_ANIM_ATTACK, vec2i_create(0, -40), 40, 40);
    collision_coord cc = {.pos = vec2i_create(40, -30), .frame_index = 0};
    vector_append(&fx.har_attack.collision_coords, &cc);
    cc.pos.x = 41;
    vector_append(&fx.har_attack.collision_coords, &cc);
    object_set_animation(&fx.har_obj, &fx.har_attack);
    fx.har_obj.cur_sprite_id = 0;

    hashmap_create(&fx.orb_bk.infos);
    put_orb_info(ORB_ANIM_IDLE, ORB_ANIM_PUNCHED);
    put_orb_info(ORB_ANIM_PUNCHED, 0);

    int orb_x = lined_up ? 130 : 260;
    object_create(&fx.orb_obj, &fx.gs, vec2i_create(orb_x, (int)har_y - 40), vec2f_create(0, 0));
    object_set_group(&fx.orb_obj, GROUP_HAZARD);
    object_set_userdata(&fx.orb_obj, &fx.orb_bk);
    fx.orb_obj.orb_val = 5;
    set_orb_animation(ORB_ANIM_IDLE);

    orb_test_render_obj har_render = {.layer = RENDER_LAYER_MIDDLE, .obj = &fx.har_obj};
    orb_test_render_obj orb_render = {.layer = RENDER_LAYER_BOTTOM, .obj = &fx.orb_obj};
    vector_append(&fx.gs.objects, &har_render);
    vector_append(&fx.gs.objects, &orb_render);

    fx.ctrl.gs = &fx.gs;
    fx.ctrl.har_obj_id = fx.har_obj.id;
    fx.ctrl.har_hook = record_har_event;
    fx.players[0].ctrl = &fx.ctrl;
    fx.players[0].har_obj_id = fx.har_obj.id;
}

static void teardown_fixture(void) {
    object_free(&fx.har_obj);
    object_free(&fx.orb_obj);
    animation_free(&fx.har_attack);
    animation_free(&bk_get_info(&fx.orb_bk, ORB_ANIM_IDLE)->ani);
    animation_free(&bk_get_info(&fx.orb_bk, ORB_ANIM_PUNCHED)->ani);
    hashmap_free(&fx.orb_bk.infos);
    list_free(&fx.har_data.har_hooks);
    vector_free(&fx.gs.objects);
}

static void assert_single_punch_event(int expected_mode) {
    CU_ASSERT_EQUAL_FATAL(recorded_count, 1);
    CU_ASSERT_EQUAL(recorded[0].type, HAR_EVENT_HAZARD_PUNCHED);
    CU_ASSERT_EQUAL(recorded[0].player_id, 0);
    CU_ASSERT_EQUAL(recorded[0].hazard_punch_mode, expected_mode);
}

void test_fire_orb_ground_punch_fires_event_and_chains_orb(void) {
    setup_fixture(ARENA_FLOOR, true);

    har_collide_with_hazard(&fx.har_obj, &fx.orb_obj);

    assert_single_punch_event(HAR_HAZARD_PUNCH_GROUND);
    CU_ASSERT_EQUAL(fx.orb_obj.cur_animation->id, ORB_ANIM_PUNCHED);
    CU_ASSERT_EQUAL(fx.orb_obj.animation_state.finished, 0);
    CU_ASSERT_EQUAL(fx.har_data.damage_received, 0);

    teardown_fixture();
}

void test_fire_orb_jump_punch_reports_overhead_mode(void) {
    setup_fixture(ARENA_FLOOR - 70, true);

    har_collide_with_hazard(&fx.har_obj, &fx.orb_obj);

    assert_single_punch_event(HAR_HAZARD_PUNCH_OVERHEAD);
    CU_ASSERT_EQUAL(fx.orb_obj.cur_animation->id, ORB_ANIM_PUNCHED);

    teardown_fixture();
}

void test_fire_orb_out_of_reach_fires_nothing(void) {
    setup_fixture(ARENA_FLOOR, false);

    har_collide_with_hazard(&fx.har_obj, &fx.orb_obj);

    CU_ASSERT_EQUAL(recorded_count, 0);
    CU_ASSERT_EQUAL(fx.orb_obj.cur_animation->id, ORB_ANIM_IDLE);

    teardown_fixture();
}

void test_fire_orb_punched_orb_cannot_be_repunched(void) {
    setup_fixture(ARENA_FLOOR, true);

    har_collide_with_hazard(&fx.har_obj, &fx.orb_obj);
    assert_single_punch_event(HAR_HAZARD_PUNCH_GROUND);

    // Punched animation has no chain_hit, so overlapping hitpoints do nothing
    recorded_count = 0;
    har_collide_with_hazard(&fx.har_obj, &fx.orb_obj);
    CU_ASSERT_EQUAL(recorded_count, 0);

    // Back to the punchable idle animation: the next hit registers again
    set_orb_animation(ORB_ANIM_IDLE);
    har_collide_with_hazard(&fx.har_obj, &fx.orb_obj);
    assert_single_punch_event(HAR_HAZARD_PUNCH_GROUND);

    teardown_fixture();
}

void test_fire_orb_hazard_damage_path_skips_punch_events(void) {
    setup_fixture(ARENA_FLOOR, true);
    fx.orb_obj.hit_pixels_disabled = true;

    har_collide_with_hazard(&fx.har_obj, &fx.orb_obj);

    CU_ASSERT_EQUAL(recorded_count, 0);
    CU_ASSERT_EQUAL(fx.orb_obj.cur_animation->id, ORB_ANIM_IDLE);

    teardown_fixture();
}

static void run_ai_punch(int difficulty, ai *a, sd_pilot *pilot, tactic_state *t, af_move *selected) {
    setup_fixture(ARENA_FLOOR, true);
    memset(a, 0, sizeof(*a));
    memset(pilot, 0, sizeof(*pilot));
    memset(t, 0, sizeof(*t));
    memset(selected, 0, sizeof(*selected));
    pilot->learning = 2.0f;
    a->difficulty = difficulty;
    a->pilot = pilot;
    a->tactic = t;
    a->selected_move = selected;
    t->tactic_type = TACTIC_CLOSE;
    fx.ctrl.data = a;
    fx.ctrl.har_hook = ai_har_event;

    har_collide_with_hazard(&fx.har_obj, &fx.orb_obj);
}

void test_fire_orb_punch_clears_ai_tactic_when_enabled(void) {
    ai a;
    sd_pilot pilot;
    tactic_state t;
    af_move selected;
    run_ai_punch(6, &a, &pilot, &t, &selected);

    CU_ASSERT_EQUAL(fx.orb_obj.cur_animation->id, ORB_ANIM_PUNCHED);
    CU_ASSERT_EQUAL(t.tactic_type, 0);
    CU_ASSERT_EQUAL(t.last_tactic, TACTIC_CLOSE);
    CU_ASSERT_PTR_NULL(a.selected_move);

    teardown_fixture();
}

void test_fire_orb_punch_keeps_ai_tactic_when_disabled(void) {
    ai a;
    sd_pilot pilot;
    tactic_state t;
    af_move selected;
    run_ai_punch(2, &a, &pilot, &t, &selected);

    // The orb still reacts to the punch; only the AI response is gated
    CU_ASSERT_EQUAL(fx.orb_obj.cur_animation->id, ORB_ANIM_PUNCHED);
    CU_ASSERT_EQUAL(t.tactic_type, TACTIC_CLOSE);
    CU_ASSERT_PTR_EQUAL(a.selected_move, &selected);

    teardown_fixture();
}

void ai_fire_orb_test_suite(CU_pSuite suite) {
    CU_add_test(suite, "fire orb: ground punch fires event and chains orb",
                test_fire_orb_ground_punch_fires_event_and_chains_orb);
    CU_add_test(suite, "fire orb: jump punch reports overhead mode", test_fire_orb_jump_punch_reports_overhead_mode);
    CU_add_test(suite, "fire orb: out of reach fires nothing", test_fire_orb_out_of_reach_fires_nothing);
    CU_add_test(suite, "fire orb: punched orb cannot be re-punched", test_fire_orb_punched_orb_cannot_be_repunched);
    CU_add_test(suite, "fire orb: disabled hit pixels skip punch events",
                test_fire_orb_hazard_damage_path_skips_punch_events);
    CU_add_test(suite, "fire orb: punch clears AI tactic when enabled",
                test_fire_orb_punch_clears_ai_tactic_when_enabled);
    CU_add_test(suite, "fire orb: punch keeps AI tactic when disabled",
                test_fire_orb_punch_keeps_ai_tactic_when_disabled);
}
