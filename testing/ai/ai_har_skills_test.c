/**
 * Unit tests for AI character skills config loader.
 */

#include "game/ai/ai_skills_config_loader.h"
#include "game/ai/ai_har_skills.h"
#include "game/ai/ai_utils.h"
#include "game/common_defines.h"
#include "resources/resource_files.h"
#include "resources/resource_paths.h"
#include "utils/path.h"
#include "ai/ai_controller_test.h"
#include "CUnit/CUnit.h"

#include <stdio.h>
#include <string.h>

static bool read_whole_file(const path *p, char *buffer, size_t max_size, size_t *actual_size) {
    size_t sz = 0;
    if(!path_filesize(p, &sz) || sz == 0 || sz >= max_size) {
        return false;
    }
    if(!path_read_file(p, buffer, sz)) {
        return false;
    }
    buffer[sz] = '\0';
    if(actual_size != NULL) {
        *actual_size = sz;
    }
    return true;
}

static bool write_whole_file(const path *p, const char *content, size_t size) {
    FILE *fp = path_fopen(p, "wb");
    if(fp == NULL) {
        return false;
    }
    size_t written = fwrite(content, 1, size, fp);
    fclose(fp);
    return written == size;
}

static int count_substring(const char *haystack, const char *needle) {
    int count = 0;
    const char *cursor = haystack;
    size_t needle_len = strlen(needle);
    while((cursor = strstr(cursor, needle)) != NULL) {
        count++;
        cursor += needle_len;
    }
    return count;
}

void test_ai_skills_config_invalid_har_returns_null(void) {
    ai_skills_config_reset_cache();
    CU_ASSERT_PTR_NULL(ai_skills_config_get(-1));
    CU_ASSERT_PTR_NULL(ai_skills_config_get(999));
}

static void assert_har_config_present(int har_id) {
    const ai_har_config *cfg = ai_skills_config_get(har_id);
    CU_ASSERT_PTR_NOT_NULL_FATAL(cfg);
    CU_ASSERT_EQUAL(cfg->har_id, har_id);
}

static void assert_har_loaded_from_file(int har_id) {
    const ai_har_config *cfg = ai_skills_config_get(har_id);
    CU_ASSERT_PTR_NOT_NULL_FATAL(cfg);
    CU_ASSERT_TRUE(cfg->loaded_from_file);
}

static void assert_har_charge_flag(int har_id, bool expected) {
    const ai_har_config *cfg = ai_skills_config_get(har_id);
    CU_ASSERT_PTR_NOT_NULL_FATAL(cfg);
    if(expected) {
        CU_ASSERT_TRUE(cfg->has_charge_moves);
    } else {
        CU_ASSERT_FALSE(cfg->has_charge_moves);
    }
}

static void assert_har_push_flag(int har_id, bool expected) {
    const ai_har_config *cfg = ai_skills_config_get(har_id);
    CU_ASSERT_PTR_NOT_NULL_FATAL(cfg);
    if(expected) {
        CU_ASSERT_TRUE(cfg->has_push_moves);
    } else {
        CU_ASSERT_FALSE(cfg->has_push_moves);
    }
}

static void assert_har_projectile_flag(int har_id, bool expected) {
    const ai_har_config *cfg = ai_skills_config_get(har_id);
    CU_ASSERT_PTR_NOT_NULL_FATAL(cfg);
    if(expected) {
        CU_ASSERT_TRUE(cfg->has_projectile_moves);
    } else {
        CU_ASSERT_FALSE(cfg->has_projectile_moves);
    }
}

static void assert_har_trip_flag(int har_id, bool expected) __attribute__((unused));
static void assert_har_trip_flag(int har_id, bool expected) {
    const ai_har_config *cfg = ai_skills_config_get(har_id);
    CU_ASSERT_PTR_NOT_NULL_FATAL(cfg);
    if(expected) {
        CU_ASSERT_TRUE(cfg->has_trip_moves);
    } else {
        CU_ASSERT_FALSE(cfg->has_trip_moves);
    }
}

static void assert_har_charge_count(int har_id, uint8_t expected) {
    const ai_har_config *cfg = ai_skills_config_get(har_id);
    CU_ASSERT_PTR_NOT_NULL_FATAL(cfg);
    CU_ASSERT_EQUAL(cfg->charge_move_count, expected);
}

static void assert_har_push_count(int har_id, uint8_t expected) {
    const ai_har_config *cfg = ai_skills_config_get(har_id);
    CU_ASSERT_PTR_NOT_NULL_FATAL(cfg);
    CU_ASSERT_EQUAL(cfg->push_move_count, expected);
}

static void assert_har_projectile_count(int har_id, uint8_t expected) {
    const ai_har_config *cfg = ai_skills_config_get(har_id);
    CU_ASSERT_PTR_NOT_NULL_FATAL(cfg);
    CU_ASSERT_EQUAL(cfg->projectile_move_count, expected);
}

static void assert_har_trip_count(int har_id, uint8_t expected) __attribute__((unused));
static void assert_har_trip_count(int har_id, uint8_t expected) {
    const ai_har_config *cfg = ai_skills_config_get(har_id);
    CU_ASSERT_PTR_NOT_NULL_FATAL(cfg);
    CU_ASSERT_EQUAL(cfg->trip_move_count, expected);
}

static void assert_har_total_count(int har_id, uint8_t expected) {
    const ai_har_config *cfg = ai_skills_config_get(har_id);
    CU_ASSERT_PTR_NOT_NULL_FATAL(cfg);
    uint8_t total = (uint8_t)(cfg->charge_move_count + cfg->push_move_count + cfg->projectile_move_count +
                              cfg->trip_move_count);
    CU_ASSERT_EQUAL(total, expected);
}

#define DEFINE_HAR_CONFIG_TESTS(name, har_id, charge_expected, push_expected, projectile_expected,             \
                                charge_count_expected, push_count_expected, projectile_count_expected,          \
                                trip_count_expected, total_count_expected)                                      \
    void test_##name##_config_present(void) {                                                                 \
        CU_ASSERT_TRUE_FATAL(resource_path_init());                                                           \
        ai_skills_config_reset_cache();                                                                       \
        assert_har_config_present(har_id);                                                                    \
    }                                                                                                         \
    void test_##name##_loaded_from_file(void) {                                                               \
        CU_ASSERT_TRUE_FATAL(resource_path_init());                                                           \
        ai_skills_config_reset_cache();                                                                       \
        assert_har_loaded_from_file(har_id);                                                                  \
    }                                                                                                         \
    void test_##name##_charge_flag(void) {                                                                    \
        CU_ASSERT_TRUE_FATAL(resource_path_init());                                                           \
        ai_skills_config_reset_cache();                                                                       \
        assert_har_charge_flag(har_id, charge_expected);                                                      \
    }                                                                                                         \
    void test_##name##_push_flag(void) {                                                                      \
        CU_ASSERT_TRUE_FATAL(resource_path_init());                                                           \
        ai_skills_config_reset_cache();                                                                       \
        assert_har_push_flag(har_id, push_expected);                                                          \
    }                                                                                                         \
    void test_##name##_projectile_flag(void) {                                                                \
        CU_ASSERT_TRUE_FATAL(resource_path_init());                                                           \
        ai_skills_config_reset_cache();                                                                       \
        assert_har_projectile_flag(har_id, projectile_expected);                                              \
    }                                                                                                         \
    void test_##name##_trip_flag(void) {                                                                      \
        CU_ASSERT_TRUE_FATAL(resource_path_init());                                                           \
        ai_skills_config_reset_cache();                                                                       \
        assert_har_trip_flag(har_id, trip_count_expected > 0);                                               \
    }                                                                                                         \
    void test_##name##_charge_count(void) {                                                                   \
        CU_ASSERT_TRUE_FATAL(resource_path_init());                                                           \
        ai_skills_config_reset_cache();                                                                       \
        assert_har_charge_count(har_id, charge_count_expected);                                               \
    }                                                                                                         \
    void test_##name##_push_count(void) {                                                                     \
        CU_ASSERT_TRUE_FATAL(resource_path_init());                                                           \
        ai_skills_config_reset_cache();                                                                       \
        assert_har_push_count(har_id, push_count_expected);                                                   \
    }                                                                                                         \
    void test_##name##_projectile_count(void) {                                                               \
        CU_ASSERT_TRUE_FATAL(resource_path_init());                                                           \
        ai_skills_config_reset_cache();                                                                       \
        assert_har_projectile_count(har_id, projectile_count_expected);                                       \
    }                                                                                                         \
    void test_##name##_trip_count(void) {                                                                     \
        CU_ASSERT_TRUE_FATAL(resource_path_init());                                                           \
        ai_skills_config_reset_cache();                                                                       \
        assert_har_trip_count(har_id, trip_count_expected);                                                   \
    }                                                                                                         \
    void test_##name##_total_count(void) {                                                                    \
        CU_ASSERT_TRUE_FATAL(resource_path_init());                                                           \
        ai_skills_config_reset_cache();                                                                       \
        assert_har_total_count(har_id, total_count_expected);                                                 \
    }

DEFINE_HAR_CONFIG_TESTS(jaguar, HAR_JAGUAR, true, true, true, 2, 1, 1, 1, 5)
DEFINE_HAR_CONFIG_TESTS(shadow, HAR_SHADOW, true, false, true, 1, 0, 2, 1, 4)
DEFINE_HAR_CONFIG_TESTS(thorn, HAR_THORN, true, true, false, 1, 2, 0, 1, 4)
DEFINE_HAR_CONFIG_TESTS(pyros, HAR_PYROS, true, true, false, 2, 1, 0, 1, 4)
DEFINE_HAR_CONFIG_TESTS(electra, HAR_ELECTRA, true, true, true, 2, 1, 1, 1, 5)
DEFINE_HAR_CONFIG_TESTS(katana, HAR_KATANA, true, true, false, 4, 2, 0, 1, 7)
DEFINE_HAR_CONFIG_TESTS(shredder, HAR_SHREDDER, true, false, true, 3, 0, 1, 1, 5)
DEFINE_HAR_CONFIG_TESTS(flail, HAR_FLAIL, true, true, false, 2, 2, 0, 1, 5)
DEFINE_HAR_CONFIG_TESTS(gargoyle, HAR_GARGOYLE, true, false, false, 2, 0, 0, 1, 3)
DEFINE_HAR_CONFIG_TESTS(chronos, HAR_CHRONOS, true, false, true, 2, 0, 1, 1, 4)
DEFINE_HAR_CONFIG_TESTS(nova, HAR_NOVA, false, true, true, 0, 2, 2, 1, 5)

void test_ai_skills_config_returns_entries_for_all_hars(void) {
    ai_skills_config_reset_cache();

    for(int har_id = HAR_JAGUAR; har_id <= HAR_NOVA; har_id++) {
        const ai_har_config *cfg = ai_skills_config_get(har_id);
        CU_ASSERT_PTR_NOT_NULL_FATAL(cfg);
        CU_ASSERT_EQUAL(cfg->har_id, har_id);
    }
}

void test_ai_skills_config_expected_default_flags(void) {
    ai_skills_config_reset_cache();

    const ai_har_config *jaguar = ai_skills_config_get(HAR_JAGUAR);
    CU_ASSERT_PTR_NOT_NULL_FATAL(jaguar);
    CU_ASSERT_TRUE(jaguar->has_charge_moves);
    CU_ASSERT_TRUE(jaguar->has_push_moves);
    CU_ASSERT_TRUE(jaguar->has_projectile_moves);
    CU_ASSERT_TRUE(jaguar->has_trip_moves);

    const ai_har_config *gargoyle = ai_skills_config_get(HAR_GARGOYLE);
    CU_ASSERT_PTR_NOT_NULL_FATAL(gargoyle);
    CU_ASSERT_TRUE(gargoyle->has_charge_moves);
    CU_ASSERT_FALSE(gargoyle->has_push_moves);
    CU_ASSERT_FALSE(gargoyle->has_projectile_moves);
    CU_ASSERT_TRUE(gargoyle->has_trip_moves);
}

void test_ai_skills_config_cache_reset_rereads_file(void) {
    CU_ASSERT_TRUE_FATAL(resource_path_init());

    path jaguar_path = get_resource_filename("ai_config/hars/jaguar.json");

    char original[16384] = {0};
    size_t original_size = 0;
    CU_ASSERT_TRUE_FATAL(read_whole_file(&jaguar_path, original, sizeof(original), &original_size));

    char modified[16384] = {0};
    strncpy(modified, original, sizeof(modified) - 1);

    char *charge_key = strstr(modified, "\"charge_moves\"");
    CU_ASSERT_PTR_NOT_NULL_FATAL(charge_key);
    char *array_start = strchr(charge_key, '[');
    CU_ASSERT_PTR_NOT_NULL_FATAL(array_start);
    char *array_end = strchr(array_start, ']');
    CU_ASSERT_PTR_NOT_NULL_FATAL(array_end);

    size_t prefix_len = (size_t)(array_start - modified) + 1;
    size_t suffix_len = strlen(array_end);

    char rewritten[16384] = {0};
    memcpy(rewritten, modified, prefix_len);
    rewritten[prefix_len] = ']';
    memcpy(rewritten + prefix_len + 1, array_end + 1, suffix_len);

    CU_ASSERT_TRUE_FATAL(write_whole_file(&jaguar_path, rewritten, strlen(rewritten)));

    ai_skills_config_reset_cache();
    const ai_har_config *jaguar = ai_skills_config_get(HAR_JAGUAR);
    CU_ASSERT_PTR_NOT_NULL_FATAL(jaguar);
    CU_ASSERT_FALSE(jaguar->has_charge_moves);

    CU_ASSERT_TRUE_FATAL(write_whole_file(&jaguar_path, original, original_size));
    ai_skills_config_reset_cache();

    const ai_har_config *restored = ai_skills_config_get(HAR_JAGUAR);
    CU_ASSERT_PTR_NOT_NULL_FATAL(restored);
    CU_ASSERT_TRUE(restored->has_charge_moves);
}

void test_ai_skills_config_mismatched_id_falls_back_to_defaults(void) {
    CU_ASSERT_TRUE_FATAL(resource_path_init());

    path jaguar_path = get_resource_filename("ai_config/hars/jaguar.json");

    char original[16384] = {0};
    size_t original_size = 0;
    CU_ASSERT_TRUE_FATAL(read_whole_file(&jaguar_path, original, sizeof(original), &original_size));

    const char *invalid_id =
        "{\n"
        "  \"id\": 999,\n"
        "  \"charge_moves\": [],\n"
        "  \"push_moves\": [],\n"
        "  \"projectile_moves\": []\n"
        "}\n";

    CU_ASSERT_TRUE_FATAL(write_whole_file(&jaguar_path, invalid_id, strlen(invalid_id)));

    ai_skills_config_reset_cache();
    const ai_har_config *cfg = ai_skills_config_get(HAR_JAGUAR);
    CU_ASSERT_PTR_NOT_NULL_FATAL(cfg);
    CU_ASSERT_FALSE(cfg->loaded_from_file);
    CU_ASSERT_TRUE(cfg->has_charge_moves);
    CU_ASSERT_TRUE(cfg->has_push_moves);
    CU_ASSERT_TRUE(cfg->has_projectile_moves);
    CU_ASSERT_EQUAL(cfg->charge_move_count, 0);
    CU_ASSERT_EQUAL(cfg->push_move_count, 0);
    CU_ASSERT_EQUAL(cfg->projectile_move_count, 0);

    CU_ASSERT_TRUE_FATAL(write_whole_file(&jaguar_path, original, original_size));
    ai_skills_config_reset_cache();
}

void test_ai_skills_config_missing_id_falls_back_to_defaults(void) {
    CU_ASSERT_TRUE_FATAL(resource_path_init());

    path jaguar_path = get_resource_filename("ai_config/hars/jaguar.json");

    char original[16384] = {0};
    size_t original_size = 0;
    CU_ASSERT_TRUE_FATAL(read_whole_file(&jaguar_path, original, sizeof(original), &original_size));

    const char *missing_id =
        "{\n"
        "  \"charge_moves\": [],\n"
        "  \"push_moves\": [],\n"
        "  \"projectile_moves\": []\n"
        "}\n";

    CU_ASSERT_TRUE_FATAL(write_whole_file(&jaguar_path, missing_id, strlen(missing_id)));

    ai_skills_config_reset_cache();
    const ai_har_config *cfg = ai_skills_config_get(HAR_JAGUAR);
    CU_ASSERT_PTR_NOT_NULL_FATAL(cfg);
    CU_ASSERT_FALSE(cfg->loaded_from_file);
    CU_ASSERT_TRUE(cfg->has_charge_moves);
    CU_ASSERT_TRUE(cfg->has_push_moves);
    CU_ASSERT_TRUE(cfg->has_projectile_moves);
    CU_ASSERT_EQUAL(cfg->charge_move_count, 0);
    CU_ASSERT_EQUAL(cfg->push_move_count, 0);
    CU_ASSERT_EQUAL(cfg->projectile_move_count, 0);

    CU_ASSERT_TRUE_FATAL(write_whole_file(&jaguar_path, original, original_size));
    ai_skills_config_reset_cache();
}

void test_ai_skills_config_non_numeric_id_falls_back_to_defaults(void) {
    CU_ASSERT_TRUE_FATAL(resource_path_init());

    path jaguar_path = get_resource_filename("ai_config/hars/jaguar.json");

    char original[16384] = {0};
    size_t original_size = 0;
    CU_ASSERT_TRUE_FATAL(read_whole_file(&jaguar_path, original, sizeof(original), &original_size));

    const char *bad_id =
        "{\n"
        "  \"id\": \"jaguar\",\n"
        "  \"charge_moves\": [],\n"
        "  \"push_moves\": [],\n"
        "  \"projectile_moves\": []\n"
        "}\n";

    CU_ASSERT_TRUE_FATAL(write_whole_file(&jaguar_path, bad_id, strlen(bad_id)));

    ai_skills_config_reset_cache();
    const ai_har_config *cfg = ai_skills_config_get(HAR_JAGUAR);
    CU_ASSERT_PTR_NOT_NULL_FATAL(cfg);
    CU_ASSERT_FALSE(cfg->loaded_from_file);
    CU_ASSERT_TRUE(cfg->has_charge_moves);
    CU_ASSERT_TRUE(cfg->has_push_moves);
    CU_ASSERT_TRUE(cfg->has_projectile_moves);
    CU_ASSERT_EQUAL(cfg->charge_move_count, 0);
    CU_ASSERT_EQUAL(cfg->push_move_count, 0);
    CU_ASSERT_EQUAL(cfg->projectile_move_count, 0);

    CU_ASSERT_TRUE_FATAL(write_whole_file(&jaguar_path, original, original_size));
    ai_skills_config_reset_cache();
}

void test_ai_skills_config_cache_is_isolated_per_har(void) {
    CU_ASSERT_TRUE_FATAL(resource_path_init());

    path jaguar_path = get_resource_filename("ai_config/hars/jaguar.json");

    char original[16384] = {0};
    size_t original_size = 0;
    CU_ASSERT_TRUE_FATAL(read_whole_file(&jaguar_path, original, sizeof(original), &original_size));

    const char *modified_jaguar =
        "{\n"
        "  \"id\": 0,\n"
        "  \"charge_moves\": [],\n"
        "  \"push_moves\": [ { \"name\": \"jab\", \"sequence\": [\"B\", \"K\"] } ],\n"
        "  \"projectile_moves\": [ { \"name\": \"shot\", \"sequence\": [\"D\", \"B\", \"P\"] } ]\n"
        "}\n";

    CU_ASSERT_TRUE_FATAL(write_whole_file(&jaguar_path, modified_jaguar, strlen(modified_jaguar)));

    ai_skills_config_reset_cache();

    const ai_har_config *jaguar = ai_skills_config_get(HAR_JAGUAR);
    CU_ASSERT_PTR_NOT_NULL_FATAL(jaguar);
    CU_ASSERT_TRUE(jaguar->loaded_from_file);
    CU_ASSERT_FALSE(jaguar->has_charge_moves);
    CU_ASSERT_EQUAL(jaguar->charge_move_count, 0);

    const ai_har_config *shadow = ai_skills_config_get(HAR_SHADOW);
    CU_ASSERT_PTR_NOT_NULL_FATAL(shadow);
    CU_ASSERT_TRUE(shadow->loaded_from_file);
    CU_ASSERT_TRUE(shadow->has_charge_moves);
    CU_ASSERT_EQUAL(shadow->charge_move_count, 1);
    CU_ASSERT_TRUE(shadow->has_projectile_moves);
    CU_ASSERT_EQUAL(shadow->projectile_move_count, 2);

    CU_ASSERT_TRUE_FATAL(write_whole_file(&jaguar_path, original, original_size));
    ai_skills_config_reset_cache();
}

void test_ai_skills_config_missing_arrays_parses_with_zero_counts(void) {
    CU_ASSERT_TRUE_FATAL(resource_path_init());

    path jaguar_path = get_resource_filename("ai_config/hars/jaguar.json");

    char original[16384] = {0};
    size_t original_size = 0;
    CU_ASSERT_TRUE_FATAL(read_whole_file(&jaguar_path, original, sizeof(original), &original_size));

    const char *missing_arrays =
        "{\n"
        "  \"id\": 0,\n"
        "  \"name\": \"jaguar\"\n"
        "}\n";

    CU_ASSERT_TRUE_FATAL(write_whole_file(&jaguar_path, missing_arrays, strlen(missing_arrays)));

    ai_skills_config_reset_cache();
    const ai_har_config *cfg = ai_skills_config_get(HAR_JAGUAR);
    CU_ASSERT_PTR_NOT_NULL_FATAL(cfg);
    CU_ASSERT_TRUE(cfg->loaded_from_file);
    CU_ASSERT_FALSE(cfg->has_charge_moves);
    CU_ASSERT_FALSE(cfg->has_push_moves);
    CU_ASSERT_FALSE(cfg->has_projectile_moves);
    CU_ASSERT_EQUAL(cfg->charge_move_count, 0);
    CU_ASSERT_EQUAL(cfg->push_move_count, 0);
    CU_ASSERT_EQUAL(cfg->projectile_move_count, 0);

    CU_ASSERT_TRUE_FATAL(write_whole_file(&jaguar_path, original, original_size));
    ai_skills_config_reset_cache();
}

void test_ai_skills_config_count_flag_consistency_for_all_hars(void) {
    ai_skills_config_reset_cache();

    for(int har_id = HAR_JAGUAR; har_id <= HAR_NOVA; har_id++) {
        const ai_har_config *cfg = ai_skills_config_get(har_id);
        CU_ASSERT_PTR_NOT_NULL_FATAL(cfg);

        CU_ASSERT_EQUAL(cfg->has_charge_moves, cfg->charge_move_count > 0);
        CU_ASSERT_EQUAL(cfg->has_push_moves, cfg->push_move_count > 0);
        CU_ASSERT_EQUAL(cfg->has_projectile_moves, cfg->projectile_move_count > 0);
        CU_ASSERT_EQUAL(cfg->has_trip_moves, cfg->trip_move_count > 0);
        CU_ASSERT_EQUAL(cfg->trip_move_count, 1);
    }
}

void test_ai_skills_config_trip_overlay_parses_and_flags_correctly(void) {
    ai_har_config cfg = {0};
    cfg.har_id = HAR_JAGUAR;
    cfg.has_charge_moves = true;
    cfg.charge_move_count = 1;

    const char *json =
        "{\n"
        "  \"trip_moves\": [\n"
        "    {\n"
        "      \"name\": \"trip\",\n"
        "      \"sequence\": [\"D\", \"B\", \"K\"],\n"
        "      \"range_min\": \"CLOSE\"\n"
        "    }\n"
        "  ]\n"
        "}\n";

    bool result = ai_skills_config_apply_overlay(&cfg, json);
    CU_ASSERT_TRUE(result);
    CU_ASSERT_EQUAL(cfg.trip_move_count, 1);
    CU_ASSERT_TRUE(cfg.has_trip_moves);
    CU_ASSERT_EQUAL(cfg.trip_moves[0].input_count, 3);
    CU_ASSERT_EQUAL(cfg.trip_moves[0].range_min, MOVE_RANGE_CLOSE);
}

void test_ai_skills_config_chronos_projectile_single_entry(void) {
    ai_skills_config_reset_cache();

    const ai_har_config *cfg = ai_skills_config_get(HAR_CHRONOS);
    CU_ASSERT_PTR_NOT_NULL_FATAL(cfg);
    CU_ASSERT_EQUAL(cfg->projectile_move_count, 1);
    CU_ASSERT_TRUE(cfg->has_projectile_moves);
}

void test_ai_skills_config_nova_charge_zero_entry(void) {
    ai_skills_config_reset_cache();

    const ai_har_config *cfg = ai_skills_config_get(HAR_NOVA);
    CU_ASSERT_PTR_NOT_NULL_FATAL(cfg);
    CU_ASSERT_EQUAL(cfg->charge_move_count, 0);
    CU_ASSERT_FALSE(cfg->has_charge_moves);
}

void test_ai_skills_config_count_ignores_brackets_inside_strings(void) {
    CU_ASSERT_TRUE_FATAL(resource_path_init());

    path jaguar_path = get_resource_filename("ai_config/hars/jaguar.json");

    char original[16384] = {0};
    size_t original_size = 0;
    CU_ASSERT_TRUE_FATAL(read_whole_file(&jaguar_path, original, sizeof(original), &original_size));

    const char *tricky_json =
        "{\n"
        "  \"id\": 0,\n"
        "  \"charge_moves\": [\n"
        "    {\n"
        "      \"name\": \"trap ] bracket\",\n"
        "      \"sequence\": [\"B\", \"D\", \"F\", \"P\"]\n"
        "    },\n"
        "    {\n"
        "      \"name\": \"second [ move\",\n"
        "      \"sequence\": [\"F\", \"D\", \"B\", \"K\"]\n"
        "    }\n"
        "  ],\n"
        "  \"push_moves\": [],\n"
        "  \"projectile_moves\": []\n"
        "}\n";

    CU_ASSERT_TRUE_FATAL(write_whole_file(&jaguar_path, tricky_json, strlen(tricky_json)));

    ai_skills_config_reset_cache();
    const ai_har_config *cfg = ai_skills_config_get(HAR_JAGUAR);
    CU_ASSERT_PTR_NOT_NULL_FATAL(cfg);
    CU_ASSERT_TRUE(cfg->loaded_from_file);
    CU_ASSERT_EQUAL(cfg->charge_move_count, 2);
    CU_ASSERT_TRUE(cfg->has_charge_moves);

    CU_ASSERT_TRUE_FATAL(write_whole_file(&jaguar_path, original, original_size));
    ai_skills_config_reset_cache();
}

void test_ai_skills_config_count_ignores_braces_inside_strings(void) {
    CU_ASSERT_TRUE_FATAL(resource_path_init());

    path jaguar_path = get_resource_filename("ai_config/hars/jaguar.json");

    char original[16384] = {0};
    size_t original_size = 0;
    CU_ASSERT_TRUE_FATAL(read_whole_file(&jaguar_path, original, sizeof(original), &original_size));

    const char *tricky_json =
        "{\n"
        "  \"id\": 0,\n"
        "  \"charge_moves\": [],\n"
        "  \"push_moves\": [\n"
        "    {\n"
        "      \"name\": \"grip {stage} and \\\"quote\\\"\",\n"
        "      \"sequence\": [\"F\", \"F\", \"P\"]\n"
        "    }\n"
        "  ],\n"
        "  \"projectile_moves\": []\n"
        "}\n";

    CU_ASSERT_TRUE_FATAL(write_whole_file(&jaguar_path, tricky_json, strlen(tricky_json)));

    ai_skills_config_reset_cache();
    const ai_har_config *cfg = ai_skills_config_get(HAR_JAGUAR);
    CU_ASSERT_PTR_NOT_NULL_FATAL(cfg);
    CU_ASSERT_TRUE(cfg->loaded_from_file);
    CU_ASSERT_EQUAL(cfg->push_move_count, 1);
    CU_ASSERT_TRUE(cfg->has_push_moves);

    CU_ASSERT_TRUE_FATAL(write_whole_file(&jaguar_path, original, original_size));
    ai_skills_config_reset_cache();
}

void test_ai_skills_config_malformed_array_reports_zero_entries(void) {
    CU_ASSERT_TRUE_FATAL(resource_path_init());

    path jaguar_path = get_resource_filename("ai_config/hars/jaguar.json");

    char original[16384] = {0};
    size_t original_size = 0;
    CU_ASSERT_TRUE_FATAL(read_whole_file(&jaguar_path, original, sizeof(original), &original_size));

    const char *malformed_json =
        "{\n"
        "  \"id\": 0,\n"
        "  \"charge_moves\": [\n"
        "    {\n"
        "      \"name\": \"broken\",\n"
        "      \"sequence\": [\"B\", \"D\", \"F\", \"P\"]\n"
        "    }\n"
        "  ,\n"
        "  \"push_moves\": [],\n"
        "  \"projectile_moves\": []\n"
        "}\n";

    CU_ASSERT_TRUE_FATAL(write_whole_file(&jaguar_path, malformed_json, strlen(malformed_json)));

    ai_skills_config_reset_cache();
    const ai_har_config *cfg = ai_skills_config_get(HAR_JAGUAR);
    CU_ASSERT_PTR_NOT_NULL_FATAL(cfg);
    CU_ASSERT_TRUE(cfg->loaded_from_file);
    CU_ASSERT_EQUAL(cfg->charge_move_count, 0);
    CU_ASSERT_FALSE(cfg->has_charge_moves);

    CU_ASSERT_TRUE_FATAL(write_whole_file(&jaguar_path, original, original_size));
    ai_skills_config_reset_cache();
}

void test_ai_har_execute_charge_null_controller_returns_false(void) {
    CU_ASSERT_FALSE(ai_har_execute_charge(NULL, NULL, NULL));
}

void test_ai_har_execute_push_null_controller_returns_false(void) {
    CU_ASSERT_FALSE(ai_har_execute_push(NULL, NULL, NULL));
}

void test_ai_har_execute_trip_null_controller_returns_false(void) {
    CU_ASSERT_FALSE(ai_har_execute_trip(NULL, NULL, NULL));
}

void test_ai_har_execute_projectile_null_controller_returns_false(void) {
    CU_ASSERT_FALSE(ai_har_execute_projectile(NULL, NULL, NULL));
}

void test_ai_skills_config_sequence_entries_match_expected_per_har_files(void) {
    CU_ASSERT_TRUE_FATAL(resource_path_init());

    struct {
        const char *filename;
        int expected_sequence_entries;
    } expected[] = {
        {"jaguar.json", 5},  {"shadow.json", 4}, {"thorn.json", 4},   {"pyros.json", 4},
        {"electra.json", 5}, {"katana.json", 7}, {"shredder.json", 5}, {"flail.json", 5},
        {"gargoyle.json", 3}, {"chronos.json", 4}, {"nova.json", 5},
    };

    for(size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); i++) {
        char rel[256] = {0};
        snprintf(rel, sizeof(rel), "ai_config/hars/%s", expected[i].filename);

        path cfg_path = get_resource_filename(rel);
        char content[16384] = {0};
        CU_ASSERT_TRUE_FATAL(read_whole_file(&cfg_path, content, sizeof(content), NULL));

        CU_ASSERT_EQUAL(count_substring(content, "\"sequence\""), expected[i].expected_sequence_entries);
    }
}

void test_ai_skills_config_range_min_entries_match_sequence_entries(void) {
    CU_ASSERT_TRUE_FATAL(resource_path_init());

    const char *files[] = {
        "jaguar.json",  "shadow.json", "thorn.json",   "pyros.json",   "electra.json", "katana.json",
        "shredder.json", "flail.json",  "gargoyle.json", "chronos.json", "nova.json",
    };

    for(size_t i = 0; i < sizeof(files) / sizeof(files[0]); i++) {
        char rel[256] = {0};
        snprintf(rel, sizeof(rel), "ai_config/hars/%s", files[i]);

        path cfg_path = get_resource_filename(rel);
        char content[16384] = {0};
        CU_ASSERT_TRUE_FATAL(read_whole_file(&cfg_path, content, sizeof(content), NULL));

        int sequence_count = count_substring(content, "\"sequence\"");
        int range_min_count = count_substring(content, "\"range_min\"");
        CU_ASSERT_EQUAL(range_min_count, sequence_count);
    }
}

void test_ai_har_move_def_score_prefers_contextual_move(void) {
    test_pilot_fixture *pilot = test_pilot_create(0);
    test_ai_fixture *ai_fix = test_ai_create(3, pilot);

    ai_move_def stunned_move = {0};
    stunned_move.range_min = MOVE_RANGE_CLOSE;
    stunned_move.range_max = MOVE_RANGE_CLOSE;
    stunned_move.conditions = MOVE_COND_ENEMY_STUNNED;

    ai_move_def neutral_move = {0};
    neutral_move.range_min = MOVE_RANGE_CLOSE;
    neutral_move.range_max = MOVE_RANGE_CLOSE;
    neutral_move.conditions = MOVE_COND_ENEMY_NOT_STUNNED;

    int stunned_score = ai_move_def_score(&stunned_move, RANGE_CLOSE, true, &ai_fix->ai_data);
    int neutral_score = ai_move_def_score(&neutral_move, RANGE_CLOSE, true, &ai_fix->ai_data);

    CU_ASSERT(stunned_score > neutral_score);

    test_ai_free(ai_fix);
    test_pilot_free(pilot);
}

void test_ai_har_select_best_move_prefers_contextual_choice(void) {
    test_pilot_fixture *pilot = test_pilot_create(0);
    test_ai_fixture *ai_fix = test_ai_create(3, pilot);

    ai_move_def neutral_move = {0};
    neutral_move.range_min = MOVE_RANGE_CLOSE;
    neutral_move.range_max = MOVE_RANGE_CLOSE;
    neutral_move.conditions = MOVE_COND_ENEMY_NOT_STUNNED;

    ai_move_def stunned_move = {0};
    stunned_move.range_min = MOVE_RANGE_CLOSE;
    stunned_move.range_max = MOVE_RANGE_CLOSE;
    stunned_move.conditions = MOVE_COND_ENEMY_STUNNED;

    ai_move_def moves[] = {neutral_move, stunned_move};
    const ai_move_def *selected = ai_har_select_best_move(moves, 2, RANGE_CLOSE, true, &ai_fix->ai_data);

    CU_ASSERT_PTR_EQUAL(selected, &moves[1]);

    test_ai_free(ai_fix);
    test_pilot_free(pilot);
}

void ai_har_skills_test_suite(CU_pSuite suite) {
    if(CU_add_test(suite, "skills config: invalid har", test_ai_skills_config_invalid_har_returns_null) == NULL) return;
    if(CU_add_test(suite, "skills config: all hars", test_ai_skills_config_returns_entries_for_all_hars) == NULL) return;
    if(CU_add_test(suite, "skills config: expected flags", test_ai_skills_config_expected_default_flags) == NULL) return;
    if(CU_add_test(suite, "skills config: cache reset rereads", test_ai_skills_config_cache_reset_rereads_file) == NULL) return;
    if(CU_add_test(suite, "skills config: mismatched id fallback", test_ai_skills_config_mismatched_id_falls_back_to_defaults) == NULL) return;
    if(CU_add_test(suite, "skills config: missing id fallback", test_ai_skills_config_missing_id_falls_back_to_defaults) == NULL) return;
    if(CU_add_test(suite, "skills config: non-numeric id fallback", test_ai_skills_config_non_numeric_id_falls_back_to_defaults) == NULL) return;
    if(CU_add_test(suite, "skills config: cache isolated per har", test_ai_skills_config_cache_is_isolated_per_har) == NULL) return;
    if(CU_add_test(suite, "skills config: missing arrays zero counts", test_ai_skills_config_missing_arrays_parses_with_zero_counts) == NULL) return;
    if(CU_add_test(suite, "skills config: count flag consistency", test_ai_skills_config_count_flag_consistency_for_all_hars) == NULL) return;
    if(CU_add_test(suite, "skills config: trip overlay parses", test_ai_skills_config_trip_overlay_parses_and_flags_correctly) == NULL) return;
    if(CU_add_test(suite, "skills config: chronos projectile count", test_ai_skills_config_chronos_projectile_single_entry) == NULL) return;
    if(CU_add_test(suite, "skills config: nova charge count", test_ai_skills_config_nova_charge_zero_entry) == NULL) return;
    if(CU_add_test(suite, "skills config: count ignores brackets in strings", test_ai_skills_config_count_ignores_brackets_inside_strings) == NULL) return;
    if(CU_add_test(suite, "skills config: count ignores braces in strings", test_ai_skills_config_count_ignores_braces_inside_strings) == NULL) return;
    if(CU_add_test(suite, "skills config: malformed array zero entries", test_ai_skills_config_malformed_array_reports_zero_entries) == NULL) return;
    if(CU_add_test(suite, "skills exec: charge null controller", test_ai_har_execute_charge_null_controller_returns_false) == NULL) return;
    if(CU_add_test(suite, "skills exec: push null controller", test_ai_har_execute_push_null_controller_returns_false) == NULL) return;
    if(CU_add_test(suite, "skills exec: trip null controller", test_ai_har_execute_trip_null_controller_returns_false) == NULL) return;
    if(CU_add_test(suite, "skills exec: projectile null controller", test_ai_har_execute_projectile_null_controller_returns_false) == NULL) return;
    if(CU_add_test(suite, "skills config: sequence entries expected per har files", test_ai_skills_config_sequence_entries_match_expected_per_har_files) == NULL) return;
    if(CU_add_test(suite, "skills config: range_min matches sequence entries", test_ai_skills_config_range_min_entries_match_sequence_entries) == NULL) return;
    if(CU_add_test(suite, "skills exec: contextual move prefers matching condition", test_ai_har_move_def_score_prefers_contextual_move) == NULL) return;
    if(CU_add_test(suite, "skills exec: best move prefers contextual choice", test_ai_har_select_best_move_prefers_contextual_choice) == NULL) return;

    if(CU_add_test(suite, "jaguar: config present", test_jaguar_config_present) == NULL) return;
    if(CU_add_test(suite, "jaguar: loaded from file", test_jaguar_loaded_from_file) == NULL) return;
    if(CU_add_test(suite, "jaguar: charge flag", test_jaguar_charge_flag) == NULL) return;
    if(CU_add_test(suite, "jaguar: push flag", test_jaguar_push_flag) == NULL) return;
    if(CU_add_test(suite, "jaguar: projectile flag", test_jaguar_projectile_flag) == NULL) return;
    if(CU_add_test(suite, "jaguar: charge count", test_jaguar_charge_count) == NULL) return;
    if(CU_add_test(suite, "jaguar: push count", test_jaguar_push_count) == NULL) return;
    if(CU_add_test(suite, "jaguar: projectile count", test_jaguar_projectile_count) == NULL) return;
    if(CU_add_test(suite, "jaguar: total count", test_jaguar_total_count) == NULL) return;

    if(CU_add_test(suite, "shadow: config present", test_shadow_config_present) == NULL) return;
    if(CU_add_test(suite, "shadow: loaded from file", test_shadow_loaded_from_file) == NULL) return;
    if(CU_add_test(suite, "shadow: charge flag", test_shadow_charge_flag) == NULL) return;
    if(CU_add_test(suite, "shadow: push flag", test_shadow_push_flag) == NULL) return;
    if(CU_add_test(suite, "shadow: projectile flag", test_shadow_projectile_flag) == NULL) return;
    if(CU_add_test(suite, "shadow: charge count", test_shadow_charge_count) == NULL) return;
    if(CU_add_test(suite, "shadow: push count", test_shadow_push_count) == NULL) return;
    if(CU_add_test(suite, "shadow: projectile count", test_shadow_projectile_count) == NULL) return;
    if(CU_add_test(suite, "shadow: total count", test_shadow_total_count) == NULL) return;

    if(CU_add_test(suite, "thorn: config present", test_thorn_config_present) == NULL) return;
    if(CU_add_test(suite, "thorn: loaded from file", test_thorn_loaded_from_file) == NULL) return;
    if(CU_add_test(suite, "thorn: charge flag", test_thorn_charge_flag) == NULL) return;
    if(CU_add_test(suite, "thorn: push flag", test_thorn_push_flag) == NULL) return;
    if(CU_add_test(suite, "thorn: projectile flag", test_thorn_projectile_flag) == NULL) return;
    if(CU_add_test(suite, "thorn: charge count", test_thorn_charge_count) == NULL) return;
    if(CU_add_test(suite, "thorn: push count", test_thorn_push_count) == NULL) return;
    if(CU_add_test(suite, "thorn: projectile count", test_thorn_projectile_count) == NULL) return;
    if(CU_add_test(suite, "thorn: total count", test_thorn_total_count) == NULL) return;

    if(CU_add_test(suite, "pyros: config present", test_pyros_config_present) == NULL) return;
    if(CU_add_test(suite, "pyros: loaded from file", test_pyros_loaded_from_file) == NULL) return;
    if(CU_add_test(suite, "pyros: charge flag", test_pyros_charge_flag) == NULL) return;
    if(CU_add_test(suite, "pyros: push flag", test_pyros_push_flag) == NULL) return;
    if(CU_add_test(suite, "pyros: projectile flag", test_pyros_projectile_flag) == NULL) return;
    if(CU_add_test(suite, "pyros: charge count", test_pyros_charge_count) == NULL) return;
    if(CU_add_test(suite, "pyros: push count", test_pyros_push_count) == NULL) return;
    if(CU_add_test(suite, "pyros: projectile count", test_pyros_projectile_count) == NULL) return;
    if(CU_add_test(suite, "pyros: total count", test_pyros_total_count) == NULL) return;

    if(CU_add_test(suite, "electra: config present", test_electra_config_present) == NULL) return;
    if(CU_add_test(suite, "electra: loaded from file", test_electra_loaded_from_file) == NULL) return;
    if(CU_add_test(suite, "electra: charge flag", test_electra_charge_flag) == NULL) return;
    if(CU_add_test(suite, "electra: push flag", test_electra_push_flag) == NULL) return;
    if(CU_add_test(suite, "electra: projectile flag", test_electra_projectile_flag) == NULL) return;
    if(CU_add_test(suite, "electra: charge count", test_electra_charge_count) == NULL) return;
    if(CU_add_test(suite, "electra: push count", test_electra_push_count) == NULL) return;
    if(CU_add_test(suite, "electra: projectile count", test_electra_projectile_count) == NULL) return;
    if(CU_add_test(suite, "electra: total count", test_electra_total_count) == NULL) return;

    if(CU_add_test(suite, "katana: config present", test_katana_config_present) == NULL) return;
    if(CU_add_test(suite, "katana: loaded from file", test_katana_loaded_from_file) == NULL) return;
    if(CU_add_test(suite, "katana: charge flag", test_katana_charge_flag) == NULL) return;
    if(CU_add_test(suite, "katana: push flag", test_katana_push_flag) == NULL) return;
    if(CU_add_test(suite, "katana: projectile flag", test_katana_projectile_flag) == NULL) return;
    if(CU_add_test(suite, "katana: charge count", test_katana_charge_count) == NULL) return;
    if(CU_add_test(suite, "katana: push count", test_katana_push_count) == NULL) return;
    if(CU_add_test(suite, "katana: projectile count", test_katana_projectile_count) == NULL) return;
    if(CU_add_test(suite, "katana: total count", test_katana_total_count) == NULL) return;

    if(CU_add_test(suite, "shredder: config present", test_shredder_config_present) == NULL) return;
    if(CU_add_test(suite, "shredder: loaded from file", test_shredder_loaded_from_file) == NULL) return;
    if(CU_add_test(suite, "shredder: charge flag", test_shredder_charge_flag) == NULL) return;
    if(CU_add_test(suite, "shredder: push flag", test_shredder_push_flag) == NULL) return;
    if(CU_add_test(suite, "shredder: projectile flag", test_shredder_projectile_flag) == NULL) return;
    if(CU_add_test(suite, "shredder: charge count", test_shredder_charge_count) == NULL) return;
    if(CU_add_test(suite, "shredder: push count", test_shredder_push_count) == NULL) return;
    if(CU_add_test(suite, "shredder: projectile count", test_shredder_projectile_count) == NULL) return;
    if(CU_add_test(suite, "shredder: total count", test_shredder_total_count) == NULL) return;

    if(CU_add_test(suite, "flail: config present", test_flail_config_present) == NULL) return;
    if(CU_add_test(suite, "flail: loaded from file", test_flail_loaded_from_file) == NULL) return;
    if(CU_add_test(suite, "flail: charge flag", test_flail_charge_flag) == NULL) return;
    if(CU_add_test(suite, "flail: push flag", test_flail_push_flag) == NULL) return;
    if(CU_add_test(suite, "flail: projectile flag", test_flail_projectile_flag) == NULL) return;
    if(CU_add_test(suite, "flail: charge count", test_flail_charge_count) == NULL) return;
    if(CU_add_test(suite, "flail: push count", test_flail_push_count) == NULL) return;
    if(CU_add_test(suite, "flail: projectile count", test_flail_projectile_count) == NULL) return;
    if(CU_add_test(suite, "flail: total count", test_flail_total_count) == NULL) return;

    if(CU_add_test(suite, "gargoyle: config present", test_gargoyle_config_present) == NULL) return;
    if(CU_add_test(suite, "gargoyle: loaded from file", test_gargoyle_loaded_from_file) == NULL) return;
    if(CU_add_test(suite, "gargoyle: charge flag", test_gargoyle_charge_flag) == NULL) return;
    if(CU_add_test(suite, "gargoyle: push flag", test_gargoyle_push_flag) == NULL) return;
    if(CU_add_test(suite, "gargoyle: projectile flag", test_gargoyle_projectile_flag) == NULL) return;
    if(CU_add_test(suite, "gargoyle: charge count", test_gargoyle_charge_count) == NULL) return;
    if(CU_add_test(suite, "gargoyle: push count", test_gargoyle_push_count) == NULL) return;
    if(CU_add_test(suite, "gargoyle: projectile count", test_gargoyle_projectile_count) == NULL) return;
    if(CU_add_test(suite, "gargoyle: total count", test_gargoyle_total_count) == NULL) return;

    if(CU_add_test(suite, "chronos: config present", test_chronos_config_present) == NULL) return;
    if(CU_add_test(suite, "chronos: loaded from file", test_chronos_loaded_from_file) == NULL) return;
    if(CU_add_test(suite, "chronos: charge flag", test_chronos_charge_flag) == NULL) return;
    if(CU_add_test(suite, "chronos: push flag", test_chronos_push_flag) == NULL) return;
    if(CU_add_test(suite, "chronos: projectile flag", test_chronos_projectile_flag) == NULL) return;
    if(CU_add_test(suite, "chronos: charge count", test_chronos_charge_count) == NULL) return;
    if(CU_add_test(suite, "chronos: push count", test_chronos_push_count) == NULL) return;
    if(CU_add_test(suite, "chronos: projectile count", test_chronos_projectile_count) == NULL) return;
    if(CU_add_test(suite, "chronos: total count", test_chronos_total_count) == NULL) return;

    if(CU_add_test(suite, "nova: config present", test_nova_config_present) == NULL) return;
    if(CU_add_test(suite, "nova: loaded from file", test_nova_loaded_from_file) == NULL) return;
    if(CU_add_test(suite, "nova: charge flag", test_nova_charge_flag) == NULL) return;
    if(CU_add_test(suite, "nova: push flag", test_nova_push_flag) == NULL) return;
    if(CU_add_test(suite, "nova: projectile flag", test_nova_projectile_flag) == NULL) return;
    if(CU_add_test(suite, "nova: charge count", test_nova_charge_count) == NULL) return;
    if(CU_add_test(suite, "nova: push count", test_nova_push_count) == NULL) return;
    if(CU_add_test(suite, "nova: projectile count", test_nova_projectile_count) == NULL) return;
    if(CU_add_test(suite, "nova: total count", test_nova_total_count) == NULL) return;
}
