/**
 * @file ai_catalog_matching_test.c
 * @brief Tests for AI move catalog position-based matching
 *
 * Validates that assign_move_by_name() correctly matches move names to AF move IDs
 * using the position-based matching strategy.
 *
 * These tests verify:
 * 1. Catalog initialization completes without errors
 * 2. Known moves are matched to valid AF move IDs
 * 3. Invalid move names return false
 * 4. Multiple HARs can be tested (Chronos, Electra, Nova, etc.)
 */

#include "CUnit/CUnit.h"
#include "controller/ai_controller.h"
#include "controller/controller.h"
#include "game/ai/ai_types.h"
#include "game/objects/har.h"
#include "resources/af.h"
#include "resources/af_loader.h"
#include "formats/pilot.h"
#include "utils/allocator.h"
#include "utils/log.h"
#include <string.h>

/* -----------------------------------------------------------------------
 * Test fixtures
 * -------------------------------------------------------------------- */

typedef struct {
    controller ctrl;
    ai ai_data;
    har har_data;
    af af_data;
    sd_pilot pilot_data;
} catalog_test_fixture;

static catalog_test_fixture *catalog_test_create(int har_id) {
    catalog_test_fixture *fix = omf_calloc(1, sizeof(catalog_test_fixture));
    
    // Initialize AF data for the HAR
    memset(&fix->af_data, 0, sizeof(af));
    if(load_af_file(&fix->af_data, har_id) != 0) {
        log_error("Failed to load AF file for HAR %d", har_id);
        omf_free(fix);
        return NULL;
    }
    
    // Initialize HAR data
    memset(&fix->har_data, 0, sizeof(har));
    fix->har_data.id = har_id;
    fix->har_data.af_data = &fix->af_data;
    
    // Initialize AI data
    memset(&fix->ai_data, 0, sizeof(ai));
    memset(&fix->pilot_data, 0, sizeof(sd_pilot));
    fix->pilot_data.pilot_id = 0;
    fix->ai_data.pilot = &fix->pilot_data;
    fix->ai_data.difficulty = 3;
    
    // Initialize controller
    memset(&fix->ctrl, 0, sizeof(controller));
    fix->ctrl.data = &fix->ai_data;
    
    return fix;
}

static void catalog_test_free(catalog_test_fixture *fix) {
    if(fix) {
        // Free catalog if it was initialized
        ai_catalog_free(&fix->ai_data);
        // Free AF data
        af_free(&fix->af_data);
        omf_free(fix);
    }
}

/* -----------------------------------------------------------------------
 * Helper to check move matching
 * -------------------------------------------------------------------- */

static bool test_move_match(controller *ctrl, ai *a, har *h, const char *move_name, 
                            bool should_succeed) {
    log_debug("Testing move: %s (expected: %s)", move_name, 
              should_succeed ? "success" : "failure");
    
    bool result = assign_move_by_name(ctrl, move_name);
    
    if(result && should_succeed) {
        // Move matched - get the matched ID for logging
        ai_move_ref *ref = ai_catalog_find_by_name(a, move_name);
        if(ref && ref->move_id >= 0) {
            log_debug("  ✓ Matched to AF move ID %d", ref->move_id);
            return true;
        }
    } else if(!result && !should_succeed) {
        log_debug("  ✓ Correctly rejected");
        return true;
    }
    
    log_error("  ✗ Mismatch: got %s, expected %s", 
              result ? "success" : "failure",
              should_succeed ? "success" : "failure");
    return false;
}

/* -----------------------------------------------------------------------
 * Test: Chronos catalog initialization and matching
 * -------------------------------------------------------------------- */

void test_chronos_catalog_init_and_moves(void) {
    catalog_test_fixture *fix = catalog_test_create(0);  // Chronos = HAR ID 0
    CU_ASSERT_PTR_NOT_NULL(fix);
    
    if(!fix) return;
    
    // Force catalog initialization
    ai_catalog_init(&fix->ai_data, &fix->har_data);
    CU_ASSERT_PTR_NOT_NULL(fix->ai_data.move_catalog);
    
    if(fix->ai_data.move_catalog) {
        log_debug("Chronos catalog initialized with %d entries", 
                  fix->ai_data.move_catalog->entry_count);
    }
    
    // Test known moves
    CU_ASSERT_TRUE(test_move_match(&fix->ctrl, &fix->ai_data, &fix->har_data, 
                                   "teleportation", true));
    CU_ASSERT_TRUE(test_move_match(&fix->ctrl, &fix->ai_data, &fix->har_data, 
                                   "trip_slide", true));
    CU_ASSERT_TRUE(test_move_match(&fix->ctrl, &fix->ai_data, &fix->har_data, 
                                   "matter_phasing", true));
    CU_ASSERT_TRUE(test_move_match(&fix->ctrl, &fix->ai_data, &fix->har_data, 
                                   "stasis", true));
    
    // Test invalid move
    CU_ASSERT_TRUE(test_move_match(&fix->ctrl, &fix->ai_data, &fix->har_data, 
                                   "nonexistent_move", false));
    
    catalog_test_free(fix);
}

/* -----------------------------------------------------------------------
 * Test: Electra catalog and moves
 * -------------------------------------------------------------------- */

void test_electra_catalog_init_and_moves(void) {
    catalog_test_fixture *fix = catalog_test_create(1);  // Electra = HAR ID 1
    CU_ASSERT_PTR_NOT_NULL(fix);
    
    if(!fix) return;
    
    ai_catalog_init(&fix->ai_data, &fix->har_data);
    CU_ASSERT_PTR_NOT_NULL(fix->ai_data.move_catalog);
    
    // Test Electra moves
    CU_ASSERT_TRUE(test_move_match(&fix->ctrl, &fix->ai_data, &fix->har_data, 
                                   "super_rolling_thunder", true));
    CU_ASSERT_TRUE(test_move_match(&fix->ctrl, &fix->ai_data, &fix->har_data, 
                                   "rolling_thunder", true));
    CU_ASSERT_TRUE(test_move_match(&fix->ctrl, &fix->ai_data, &fix->har_data, 
                                   "electric_shards", true));
    CU_ASSERT_TRUE(test_move_match(&fix->ctrl, &fix->ai_data, &fix->har_data, 
                                   "ball_lightning", true));
    
    catalog_test_free(fix);
}

/* -----------------------------------------------------------------------
 * Test: Nova catalog and moves
 * -------------------------------------------------------------------- */

void test_nova_catalog_init_and_moves(void) {
    catalog_test_fixture *fix = catalog_test_create(7);  // Nova = HAR ID 7
    CU_ASSERT_PTR_NOT_NULL(fix);
    
    if(!fix) return;
    
    ai_catalog_init(&fix->ai_data, &fix->har_data);
    CU_ASSERT_PTR_NOT_NULL(fix->ai_data.move_catalog);
    
    // Test Nova moves
    CU_ASSERT_TRUE(test_move_match(&fix->ctrl, &fix->ai_data, &fix->har_data, 
                                   "earthquake_slam", true));
    CU_ASSERT_TRUE(test_move_match(&fix->ctrl, &fix->ai_data, &fix->har_data, 
                                   "heavy_kick", true));
    CU_ASSERT_TRUE(test_move_match(&fix->ctrl, &fix->ai_data, &fix->har_data, 
                                   "mini_grenade", true));
    CU_ASSERT_TRUE(test_move_match(&fix->ctrl, &fix->ai_data, &fix->har_data, 
                                   "missile", true));
    CU_ASSERT_TRUE(test_move_match(&fix->ctrl, &fix->ai_data, &fix->har_data, 
                                   "belly_flop", true));
    
    catalog_test_free(fix);
}

/* -----------------------------------------------------------------------
 * Test: Thorn catalog and shadow_speed_kick
 * -------------------------------------------------------------------- */

void test_thorn_catalog_init_and_shadow_speed_kick(void) {
    catalog_test_fixture *fix = catalog_test_create(2);  // Thorn = HAR ID 2
    CU_ASSERT_PTR_NOT_NULL(fix);
    
    if(!fix) return;
    
    ai_catalog_init(&fix->ai_data, &fix->har_data);
    CU_ASSERT_PTR_NOT_NULL(fix->ai_data.move_catalog);
    
    // Test Thorn moves including new shadow_speed_kick
    CU_ASSERT_TRUE(test_move_match(&fix->ctrl, &fix->ai_data, &fix->har_data, 
                                   "spike_charge", true));
    CU_ASSERT_TRUE(test_move_match(&fix->ctrl, &fix->ai_data, &fix->har_data, 
                                   "shadow_kick", true));
    CU_ASSERT_TRUE(test_move_match(&fix->ctrl, &fix->ai_data, &fix->har_data, 
                                   "speed_kick", true));
    CU_ASSERT_TRUE(test_move_match(&fix->ctrl, &fix->ai_data, &fix->har_data, 
                                   "shadow_speed_kick", true));
    
    catalog_test_free(fix);
}

/* -----------------------------------------------------------------------
 * Test: Multiple HARs to verify no cross-contamination
 * -------------------------------------------------------------------- */

void test_catalog_multiple_hars_independent(void) {
    // Create catalogs for two different HARs
    catalog_test_fixture *chronos = catalog_test_create(0);
    catalog_test_fixture *electra = catalog_test_create(1);
    
    CU_ASSERT_PTR_NOT_NULL(chronos);
    CU_ASSERT_PTR_NOT_NULL(electra);
    
    if(!chronos || !electra) {
        catalog_test_free(chronos);
        catalog_test_free(electra);
        return;
    }
    
    ai_catalog_init(&chronos->ai_data, &chronos->har_data);
    ai_catalog_init(&electra->ai_data, &electra->har_data);
    
    // Chronos move should work for Chronos, not Electra
    CU_ASSERT_TRUE(assign_move_by_name(&chronos->ctrl, "teleportation"));
    CU_ASSERT_FALSE(assign_move_by_name(&electra->ctrl, "teleportation"));
    
    // Electra move should work for Electra, not Chronos
    CU_ASSERT_TRUE(assign_move_by_name(&electra->ctrl, "rolling_thunder"));
    CU_ASSERT_FALSE(assign_move_by_name(&chronos->ctrl, "rolling_thunder"));
    
    catalog_test_free(chronos);
    catalog_test_free(electra);
}

/* -----------------------------------------------------------------------
 * Test suite registration
 * -------------------------------------------------------------------- */

void ai_catalog_matching_test_suite(CU_pSuite suite) {
    if(CU_add_test(suite, "catalog: Chronos init and moves", 
                   test_chronos_catalog_init_and_moves) == NULL) return;
    if(CU_add_test(suite, "catalog: Electra init and moves", 
                   test_electra_catalog_init_and_moves) == NULL) return;
    if(CU_add_test(suite, "catalog: Nova init and moves", 
                   test_nova_catalog_init_and_moves) == NULL) return;
    if(CU_add_test(suite, "catalog: Thorn shadow_speed_kick", 
                   test_thorn_catalog_init_and_shadow_speed_kick) == NULL) return;
    if(CU_add_test(suite, "catalog: Multiple HARs independent", 
                   test_catalog_multiple_hars_independent) == NULL) return;
}
