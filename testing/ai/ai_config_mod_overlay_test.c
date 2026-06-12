/**
 * Tests for AI config mod overlay semantics (Phase 7).
 *
 * Tests for:
 * - ai_config_apply_pilot_overlay(): partial JSON overlay for pilot personality
 * - ai_skills_config_apply_overlay(): partial JSON overlay for character skill config
 * - Missing-key fallback (fields absent from overlay keep base value)
 * - Sequential overlay application (load-order semantics)
 * - modmanager returns no overlays when no mods are loaded
 */

#include "game/ai/ai_config_loader.h"
#include "game/ai/ai_skills_config_loader.h"
#include "game/common_defines.h"
#include "resources/modmanager.h"
#include "CUnit/CUnit.h"

#include <string.h>

/* ---- Pilot overlay tests ---- */

static sd_pilot make_test_pilot(int pilot_id) {
    sd_pilot p;
    memset(&p, 0, sizeof(sd_pilot));
    p.pilot_id = (uint8_t)pilot_id;
    p.att_normal = 10;
    p.att_hyper = 20;
    p.att_jump = 30;
    p.att_def = 40;
    p.att_sniper = 50;
    p.ap_throw = 100;
    p.ap_special = 110;
    p.ap_jump = 120;
    p.ap_high = 130;
    p.ap_low = 140;
    p.ap_middle = 150;
    p.pref_jump = 5;
    p.pref_fwd = 6;
    p.pref_back = 7;
    p.learning = 0.5f;
    p.forget = 0.1f;
    return p;
}

void test_pilot_overlay_null_pilot(void) {
    const char *json = "{\"pilots\":[{\"id\":0,\"att_normal\":99}]}";
    CU_ASSERT_FALSE(ai_config_apply_pilot_overlay(NULL, json));
}

void test_pilot_overlay_null_json(void) {
    sd_pilot p = make_test_pilot(0);
    CU_ASSERT_FALSE(ai_config_apply_pilot_overlay(&p, NULL));
    CU_ASSERT_EQUAL(p.att_normal, 10);
}

void test_pilot_overlay_empty_json_no_change(void) {
    sd_pilot p = make_test_pilot(0);
    const char *json = "{}";
    bool result = ai_config_apply_pilot_overlay(&p, json);
    CU_ASSERT_FALSE(result);
    CU_ASSERT_EQUAL(p.att_normal, 10);
    CU_ASSERT_EQUAL(p.att_hyper, 20);
    CU_ASSERT_DOUBLE_EQUAL(p.learning, 0.5f, 0.001f);
}

void test_pilot_overlay_missing_pilot_id_no_change(void) {
    sd_pilot p = make_test_pilot(5);
    const char *json = "{\"pilots\":[{\"id\":0,\"att_normal\":99}]}";
    bool result = ai_config_apply_pilot_overlay(&p, json);
    CU_ASSERT_FALSE(result);
    CU_ASSERT_EQUAL(p.att_normal, 10);
}

void test_pilot_overlay_single_field_updates_one(void) {
    sd_pilot p = make_test_pilot(0);
    const char *json = "{\"pilots\":[{\"id\":0,\"att_normal\":77}]}";
    bool result = ai_config_apply_pilot_overlay(&p, json);
    CU_ASSERT_TRUE(result);
    CU_ASSERT_EQUAL(p.att_normal, 77);
    CU_ASSERT_EQUAL(p.att_hyper, 20);
    CU_ASSERT_EQUAL(p.att_jump, 30);
    CU_ASSERT_EQUAL(p.att_def, 40);
    CU_ASSERT_EQUAL(p.att_sniper, 50);
}

void test_pilot_overlay_multiple_fields(void) {
    sd_pilot p = make_test_pilot(0);
    const char *json =
        "{\"pilots\":[{\"id\":0,"
        "\"att_normal\":1,"
        "\"att_hyper\":2,"
        "\"learning\":0.9,"
        "\"forget\":0.05}]}";
    bool result = ai_config_apply_pilot_overlay(&p, json);
    CU_ASSERT_TRUE(result);
    CU_ASSERT_EQUAL(p.att_normal, 1);
    CU_ASSERT_EQUAL(p.att_hyper, 2);
    CU_ASSERT_EQUAL(p.att_jump, 30);
    CU_ASSERT_DOUBLE_EQUAL(p.learning, 0.9f, 0.001f);
    CU_ASSERT_DOUBLE_EQUAL(p.forget, 0.05f, 0.001f);
}

void test_pilot_overlay_attack_prefs(void) {
    sd_pilot p = make_test_pilot(0);
    const char *json =
        "{\"pilots\":[{\"id\":0,"
        "\"ap_throw\":200,"
        "\"pref_jump\":9}]}";
    bool result = ai_config_apply_pilot_overlay(&p, json);
    CU_ASSERT_TRUE(result);
    CU_ASSERT_EQUAL(p.ap_throw, 200);
    CU_ASSERT_EQUAL(p.pref_jump, 9);
    CU_ASSERT_EQUAL(p.ap_special, 110);
    CU_ASSERT_EQUAL(p.pref_fwd, 6);
}

void test_pilot_overlay_sequential_last_wins(void) {
    sd_pilot p = make_test_pilot(0);

    const char *overlay1 = "{\"pilots\":[{\"id\":0,\"att_normal\":50,\"att_hyper\":60}]}";
    const char *overlay2 = "{\"pilots\":[{\"id\":0,\"att_normal\":99}]}";

    ai_config_apply_pilot_overlay(&p, overlay1);
    CU_ASSERT_EQUAL(p.att_normal, 50);
    CU_ASSERT_EQUAL(p.att_hyper, 60);

    ai_config_apply_pilot_overlay(&p, overlay2);
    CU_ASSERT_EQUAL(p.att_normal, 99);
    CU_ASSERT_EQUAL(p.att_hyper, 60);
}

void test_pilot_overlay_multiple_pilots_picks_correct(void) {
    sd_pilot p = make_test_pilot(2);
    const char *json =
        "{\"pilots\":["
        "{\"id\":0,\"att_normal\":11},"
        "{\"id\":1,\"att_normal\":22},"
        "{\"id\":2,\"att_normal\":33},"
        "{\"id\":3,\"att_normal\":44}"
        "]}";
    bool result = ai_config_apply_pilot_overlay(&p, json);
    CU_ASSERT_TRUE(result);
    CU_ASSERT_EQUAL(p.att_normal, 33);
}

/* ---- Char skills overlay tests ---- */

static ai_char_config make_test_char_config(int har_id) {
    ai_char_config cfg;
    memset(&cfg, 0, sizeof(ai_char_config));
    cfg.har_id = har_id;
    cfg.loaded_from_file = true;
    cfg.has_charge_moves = true;
    cfg.has_push_moves = true;
    cfg.has_projectile_moves = true;
    cfg.charge_move_count = 2;
    cfg.push_move_count = 1;
    cfg.projectile_move_count = 1;
    return cfg;
}

void test_char_overlay_null_cfg(void) {
    const char *json = "{\"id\":0,\"charge_moves\":[{}]}";
    CU_ASSERT_FALSE(ai_skills_config_apply_overlay(NULL, json));
}

void test_char_overlay_null_json(void) {
    ai_char_config cfg = make_test_char_config(HAR_JAGUAR);
    CU_ASSERT_FALSE(ai_skills_config_apply_overlay(&cfg, NULL));
    CU_ASSERT_EQUAL(cfg.charge_move_count, 2);
}

void test_char_overlay_empty_json_no_change(void) {
    ai_char_config cfg = make_test_char_config(HAR_JAGUAR);
    bool result = ai_skills_config_apply_overlay(&cfg, "{}");
    CU_ASSERT_FALSE(result);
    CU_ASSERT_EQUAL(cfg.charge_move_count, 2);
    CU_ASSERT_EQUAL(cfg.push_move_count, 1);
    CU_ASSERT_EQUAL(cfg.projectile_move_count, 1);
}

void test_char_overlay_clears_charge_moves(void) {
    ai_char_config cfg = make_test_char_config(HAR_JAGUAR);
    const char *json = "{\"id\":0,\"charge_moves\":[]}";
    bool result = ai_skills_config_apply_overlay(&cfg, json);
    CU_ASSERT_TRUE(result);
    CU_ASSERT_EQUAL(cfg.charge_move_count, 0);
    CU_ASSERT_FALSE(cfg.has_charge_moves);
    CU_ASSERT_EQUAL(cfg.push_move_count, 1);
    CU_ASSERT_EQUAL(cfg.projectile_move_count, 1);
}

void test_char_overlay_adds_charge_moves(void) {
    ai_char_config cfg = make_test_char_config(HAR_JAGUAR);
    cfg.charge_move_count = 0;
    cfg.has_charge_moves = false;

    const char *json = "{\"id\":0,\"charge_moves\":[{\"a\":1},{\"b\":2},{\"c\":3}]}";
    bool result = ai_skills_config_apply_overlay(&cfg, json);
    CU_ASSERT_TRUE(result);
    CU_ASSERT_EQUAL(cfg.charge_move_count, 3);
    CU_ASSERT_TRUE(cfg.has_charge_moves);
}

void test_char_overlay_partial_only_updates_present_arrays(void) {
    ai_char_config cfg = make_test_char_config(HAR_JAGUAR);
    const char *json = "{\"id\":0,\"push_moves\":[{\"x\":1}]}";
    bool result = ai_skills_config_apply_overlay(&cfg, json);
    CU_ASSERT_TRUE(result);
    CU_ASSERT_EQUAL(cfg.push_move_count, 1);
    CU_ASSERT_EQUAL(cfg.charge_move_count, 2);
    CU_ASSERT_EQUAL(cfg.projectile_move_count, 1);
}

void test_char_overlay_all_arrays(void) {
    ai_char_config cfg = make_test_char_config(HAR_JAGUAR);
    const char *json =
        "{\"id\":0,"
        "\"charge_moves\":[{\"a\":1}],"
        "\"push_moves\":[],"
        "\"projectile_moves\":[{\"b\":2},{\"c\":3}]}";
    bool result = ai_skills_config_apply_overlay(&cfg, json);
    CU_ASSERT_TRUE(result);
    CU_ASSERT_EQUAL(cfg.charge_move_count, 1);
    CU_ASSERT_TRUE(cfg.has_charge_moves);
    CU_ASSERT_EQUAL(cfg.push_move_count, 0);
    CU_ASSERT_FALSE(cfg.has_push_moves);
    CU_ASSERT_EQUAL(cfg.projectile_move_count, 2);
    CU_ASSERT_TRUE(cfg.has_projectile_moves);
}

void test_char_overlay_sequential_last_wins(void) {
    ai_char_config cfg = make_test_char_config(HAR_JAGUAR);

    const char *overlay1 = "{\"charge_moves\":[{\"a\":1},{\"b\":2},{\"c\":3}]}";
    ai_skills_config_apply_overlay(&cfg, overlay1);
    CU_ASSERT_EQUAL(cfg.charge_move_count, 3);

    const char *overlay2 = "{\"charge_moves\":[{\"x\":1}]}";
    ai_skills_config_apply_overlay(&cfg, overlay2);
    CU_ASSERT_EQUAL(cfg.charge_move_count, 1);

    CU_ASSERT_EQUAL(cfg.push_move_count, 1);
    CU_ASSERT_EQUAL(cfg.projectile_move_count, 1);
}

/* ---- Modmanager no-mod baseline test ---- */

void test_modmanager_no_overlays_when_no_mods(void) {
    modmanager_set_allowed(false);

    sd_pilot p = make_test_pilot(0);
    uint8_t orig_att_normal = p.att_normal;
    bool result = modmanager_apply_json_overlays("ai_config/pilots.json", NULL, &p);
    CU_ASSERT_FALSE(result);
    CU_ASSERT_EQUAL(p.att_normal, orig_att_normal);

    modmanager_set_allowed(true);
}

void test_modmanager_overlays_disabled(void) {
    modmanager_set_allowed(false);

    ai_char_config cfg = make_test_char_config(HAR_JAGUAR);
    uint8_t orig_count = cfg.charge_move_count;
    bool result = modmanager_apply_json_overlays("ai_config/characters/jaguar.json", NULL, &cfg);
    CU_ASSERT_FALSE(result);
    CU_ASSERT_EQUAL(cfg.charge_move_count, orig_count);

    modmanager_set_allowed(true);
}

void ai_config_mod_overlay_test_suite(CU_pSuite suite) {
    CU_add_test(suite, "pilot overlay: null pilot returns false", test_pilot_overlay_null_pilot);
    CU_add_test(suite, "pilot overlay: null json returns false", test_pilot_overlay_null_json);
    CU_add_test(suite, "pilot overlay: empty json does not change fields", test_pilot_overlay_empty_json_no_change);
    CU_add_test(suite, "pilot overlay: missing pilot id does not change fields",
                test_pilot_overlay_missing_pilot_id_no_change);
    CU_add_test(suite, "pilot overlay: single field update leaves others unchanged",
                test_pilot_overlay_single_field_updates_one);
    CU_add_test(suite, "pilot overlay: multiple fields update correctly", test_pilot_overlay_multiple_fields);
    CU_add_test(suite, "pilot overlay: attack and pref fields", test_pilot_overlay_attack_prefs);
    CU_add_test(suite, "pilot overlay: sequential application - last overlay wins",
                test_pilot_overlay_sequential_last_wins);
    CU_add_test(suite, "pilot overlay: multiple pilots - picks correct id",
                test_pilot_overlay_multiple_pilots_picks_correct);

    CU_add_test(suite, "char overlay: null cfg returns false", test_char_overlay_null_cfg);
    CU_add_test(suite, "char overlay: null json returns false", test_char_overlay_null_json);
    CU_add_test(suite, "char overlay: empty json does not change fields", test_char_overlay_empty_json_no_change);
    CU_add_test(suite, "char overlay: clears charge moves", test_char_overlay_clears_charge_moves);
    CU_add_test(suite, "char overlay: adds charge moves", test_char_overlay_adds_charge_moves);
    CU_add_test(suite, "char overlay: partial overlay only updates present arrays",
                test_char_overlay_partial_only_updates_present_arrays);
    CU_add_test(suite, "char overlay: all arrays updated correctly", test_char_overlay_all_arrays);
    CU_add_test(suite, "char overlay: sequential application - last overlay wins",
                test_char_overlay_sequential_last_wins);

    CU_add_test(suite, "modmanager: no overlays when mods disabled", test_modmanager_no_overlays_when_no_mods);
    CU_add_test(suite, "modmanager: overlays disabled returns false", test_modmanager_overlays_disabled);
}
