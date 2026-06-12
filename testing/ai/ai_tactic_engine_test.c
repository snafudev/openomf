/**
 * Unit tests for AI tactic engine config gating.
 */

#include "game/ai/ai_tactic_engine.h"
#include "game/ai/ai_utils.h"
#include "resources/resource_files.h"
#include "resources/resource_paths.h"
#include "resources/ids.h"
#include "utils/log.h"
#include "utils/path.h"
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

static void ensure_test_runtime_ready(void) {
    static bool initialized = false;
    if(initialized) {
        return;
    }

    log_init();
    log_add_stderr(LOG_ERROR, false);
    initialized = true;
}

void test_tactic_is_enabled_invalid_id_returns_false(void) {
    ensure_test_runtime_ready();
    ai_tactic_reset_config_cache();
    CU_ASSERT_FALSE(ai_tactic_is_enabled(0));
    CU_ASSERT_FALSE(ai_tactic_is_enabled(99));
}

void test_tactic_is_enabled_defaults_true_for_valid_ids(void) {
    ensure_test_runtime_ready();
    ai_tactic_reset_config_cache();
    CU_ASSERT_TRUE(ai_tactic_is_enabled(TACTIC_ESCAPE));
    CU_ASSERT_TRUE(ai_tactic_is_enabled(TACTIC_COUNTER));
}

void test_tactic_is_enabled_respects_disabled_flag_in_config(void) {
    ensure_test_runtime_ready();
    CU_ASSERT_TRUE_FATAL(resource_path_init());

    path tactics_path = get_resource_filename("ai_config/tactics.json");

    char original[16384] = {0};
    size_t original_size = 0;
    CU_ASSERT_TRUE_FATAL(read_whole_file(&tactics_path, original, sizeof(original), &original_size));

    char modified[16384] = {0};
    strncpy(modified, original, sizeof(modified) - 1);

    char *target = strstr(modified, "\"SHOOT\"");
    CU_ASSERT_PTR_NOT_NULL_FATAL(target);
    char *enabled = strstr(target, "\"enabled\": true");
    CU_ASSERT_PTR_NOT_NULL_FATAL(enabled);
    memcpy(enabled, "\"enabled\": false", strlen("\"enabled\": false"));

    CU_ASSERT_TRUE_FATAL(write_whole_file(&tactics_path, modified, strlen(modified)));

    ai_tactic_reset_config_cache();
    CU_ASSERT_FALSE(ai_tactic_is_enabled(TACTIC_SHOOT));
    CU_ASSERT_TRUE(ai_tactic_is_enabled(TACTIC_GRAB));

    CU_ASSERT_TRUE_FATAL(write_whole_file(&tactics_path, original, original_size));
    ai_tactic_reset_config_cache();
}

void test_tactic_is_enabled_partial_config_keeps_unspecified_enabled(void) {
    ensure_test_runtime_ready();
    CU_ASSERT_TRUE_FATAL(resource_path_init());

    path tactics_path = get_resource_filename("ai_config/tactics.json");

    char original[16384] = {0};
    size_t original_size = 0;
    CU_ASSERT_TRUE_FATAL(read_whole_file(&tactics_path, original, sizeof(original), &original_size));

    const char *partial =
        "{\n"
        "  \"tactics\": {\n"
        "    \"SHOOT\": {\n"
        "      \"id\": 5,\n"
        "      \"enabled\": false\n"
        "    }\n"
        "  }\n"
        "}\n";

    CU_ASSERT_TRUE_FATAL(write_whole_file(&tactics_path, partial, strlen(partial)));

    ai_tactic_reset_config_cache();
    CU_ASSERT_FALSE(ai_tactic_is_enabled(TACTIC_SHOOT));
    CU_ASSERT_TRUE(ai_tactic_is_enabled(TACTIC_GRAB));
    CU_ASSERT_TRUE(ai_tactic_is_enabled(TACTIC_COUNTER));

    CU_ASSERT_TRUE_FATAL(write_whole_file(&tactics_path, original, original_size));
    ai_tactic_reset_config_cache();
}

void test_tactic_first_enabled_returns_first_enabled_in_list(void) {
    ensure_test_runtime_ready();
    ai_tactic_reset_config_cache();

    int tactics[] = {TACTIC_SHOOT, TACTIC_GRAB, TACTIC_COUNTER};
    CU_ASSERT_EQUAL(ai_tactic_first_enabled(tactics, 3), TACTIC_SHOOT);
}

void test_tactic_first_enabled_skips_disabled_entries(void) {
    ensure_test_runtime_ready();
    CU_ASSERT_TRUE_FATAL(resource_path_init());

    path tactics_path = get_resource_filename("ai_config/tactics.json");

    char original[16384] = {0};
    size_t original_size = 0;
    CU_ASSERT_TRUE_FATAL(read_whole_file(&tactics_path, original, sizeof(original), &original_size));

    const char *partial =
        "{\n"
        "  \"tactics\": {\n"
        "    \"SHOOT\": {\n"
        "      \"id\": 5,\n"
        "      \"enabled\": false\n"
        "    },\n"
        "    \"GRAB\": {\n"
        "      \"id\": 3,\n"
        "      \"enabled\": true\n"
        "    }\n"
        "  }\n"
        "}\n";

    CU_ASSERT_TRUE_FATAL(write_whole_file(&tactics_path, partial, strlen(partial)));

    ai_tactic_reset_config_cache();
    int tactics[] = {TACTIC_SHOOT, TACTIC_GRAB, TACTIC_COUNTER};
    CU_ASSERT_EQUAL(ai_tactic_first_enabled(tactics, 3), TACTIC_GRAB);

    CU_ASSERT_TRUE_FATAL(write_whole_file(&tactics_path, original, original_size));
    ai_tactic_reset_config_cache();
}

void test_tactic_metadata_strings_loaded_from_config(void) {
    ensure_test_runtime_ready();
    CU_ASSERT_TRUE_FATAL(resource_path_init());

    ai_tactic_reset_config_cache();

    CU_ASSERT_STRING_EQUAL(ai_tactic_config_move_type(TACTIC_SHOOT), "MOVE_AVOID_IF_CRAMPED");
    CU_ASSERT_STRING_EQUAL(ai_tactic_config_attack_type(TACTIC_SHOOT), "ATTACK_RANGED");
    CU_ASSERT_STRING_EQUAL(ai_tactic_config_move_type(TACTIC_COUNTER), "MOVE_BLOCK_IF_NOT_CRAMPED");
    CU_ASSERT_STRING_EQUAL(ai_tactic_config_attack_type(TACTIC_COUNTER), "ATTACK_TRIP_OR_HEAVY");
}

void test_tactic_metadata_missing_fields_return_empty_string(void) {
    ensure_test_runtime_ready();
    CU_ASSERT_TRUE_FATAL(resource_path_init());

    path tactics_path = get_resource_filename("ai_config/tactics.json");

    char original[16384] = {0};
    size_t original_size = 0;
    CU_ASSERT_TRUE_FATAL(read_whole_file(&tactics_path, original, sizeof(original), &original_size));

    const char *partial =
        "{\n"
        "  \"tactics\": {\n"
        "    \"SHOOT\": {\n"
        "      \"id\": 5,\n"
        "      \"enabled\": true\n"
        "    }\n"
        "  }\n"
        "}\n";

    CU_ASSERT_TRUE_FATAL(write_whole_file(&tactics_path, partial, strlen(partial)));

    ai_tactic_reset_config_cache();
    CU_ASSERT_STRING_EQUAL(ai_tactic_config_move_type(TACTIC_SHOOT), "");
    CU_ASSERT_STRING_EQUAL(ai_tactic_config_attack_type(TACTIC_SHOOT), "");
    CU_ASSERT_FALSE(ai_tactic_config_move_type_supported(TACTIC_SHOOT));
    CU_ASSERT_FALSE(ai_tactic_config_attack_type_supported(TACTIC_SHOOT));

    CU_ASSERT_TRUE_FATAL(write_whole_file(&tactics_path, original, original_size));
    ai_tactic_reset_config_cache();
}

void test_tactic_metadata_tokens_supported_for_default_config(void) {
    ensure_test_runtime_ready();
    CU_ASSERT_TRUE_FATAL(resource_path_init());

    ai_tactic_reset_config_cache();

    CU_ASSERT_TRUE(ai_tactic_config_move_type_supported(TACTIC_SHOOT));
    CU_ASSERT_TRUE(ai_tactic_config_attack_type_supported(TACTIC_SHOOT));
    CU_ASSERT_TRUE(ai_tactic_config_move_type_supported(TACTIC_COUNTER));
    CU_ASSERT_TRUE(ai_tactic_config_attack_type_supported(TACTIC_COUNTER));
}

void test_tactic_metadata_tokens_supported_for_all_default_tactics(void) {
    ensure_test_runtime_ready();
    CU_ASSERT_TRUE_FATAL(resource_path_init());

    ai_tactic_reset_config_cache();

    for(int tactic_id = TACTIC_ESCAPE; tactic_id <= TACTIC_COUNTER; tactic_id++) {
        CU_ASSERT_TRUE(ai_tactic_config_move_type_supported(tactic_id));
        CU_ASSERT_TRUE(ai_tactic_config_attack_type_supported(tactic_id));
    }
}

void test_tactic_metadata_supported_queries_reject_invalid_tactic_id(void) {
    ensure_test_runtime_ready();
    ai_tactic_reset_config_cache();

    CU_ASSERT_FALSE(ai_tactic_config_move_type_supported(0));
    CU_ASSERT_FALSE(ai_tactic_config_attack_type_supported(0));
    CU_ASSERT_FALSE(ai_tactic_config_move_type_supported(99));
    CU_ASSERT_FALSE(ai_tactic_config_attack_type_supported(99));
}

void test_tactic_metadata_tokens_unknown_are_reported_unsupported(void) {
    ensure_test_runtime_ready();
    CU_ASSERT_TRUE_FATAL(resource_path_init());

    path tactics_path = get_resource_filename("ai_config/tactics.json");

    char original[16384] = {0};
    size_t original_size = 0;
    CU_ASSERT_TRUE_FATAL(read_whole_file(&tactics_path, original, sizeof(original), &original_size));

    const char *partial =
        "{\n"
        "  \"tactics\": {\n"
        "    \"SHOOT\": {\n"
        "      \"id\": 5,\n"
        "      \"enabled\": true,\n"
        "      \"move_type\": \"MOVE_NOT_A_REAL_TOKEN\",\n"
        "      \"attack_type\": \"ATTACK_UNKNOWN_MODE\"\n"
        "    }\n"
        "  }\n"
        "}\n";

    CU_ASSERT_TRUE_FATAL(write_whole_file(&tactics_path, partial, strlen(partial)));

    ai_tactic_reset_config_cache();
    CU_ASSERT_STRING_EQUAL(ai_tactic_config_move_type(TACTIC_SHOOT), "MOVE_NOT_A_REAL_TOKEN");
    CU_ASSERT_STRING_EQUAL(ai_tactic_config_attack_type(TACTIC_SHOOT), "ATTACK_UNKNOWN_MODE");
    CU_ASSERT_FALSE(ai_tactic_config_move_type_supported(TACTIC_SHOOT));
    CU_ASSERT_FALSE(ai_tactic_config_attack_type_supported(TACTIC_SHOOT));
    CU_ASSERT_TRUE(ai_tactic_is_enabled(TACTIC_SHOOT));

    CU_ASSERT_TRUE_FATAL(write_whole_file(&tactics_path, original, original_size));
    ai_tactic_reset_config_cache();
}

void test_tactic_conditions_missing_by_default(void) {
    ensure_test_runtime_ready();
    CU_ASSERT_TRUE_FATAL(resource_path_init());

    ai_tactic_reset_config_cache();

    CU_ASSERT_EQUAL(ai_tactic_config_condition_count(TACTIC_SHOOT), 0);
    CU_ASSERT_FALSE(ai_tactic_config_has_condition(TACTIC_SHOOT, "pref_hyper"));
    CU_ASSERT_TRUE(ai_tactic_config_conditions_supported(TACTIC_SHOOT));
}

void test_tactic_conditions_parsed_from_config(void) {
    ensure_test_runtime_ready();
    CU_ASSERT_TRUE_FATAL(resource_path_init());

    path tactics_path = get_resource_filename("ai_config/tactics.json");

    char original[16384] = {0};
    size_t original_size = 0;
    CU_ASSERT_TRUE_FATAL(read_whole_file(&tactics_path, original, sizeof(original), &original_size));

    const char *partial =
        "{\n"
        "  \"tactics\": {\n"
        "    \"SHOOT\": {\n"
        "      \"id\": 5,\n"
        "      \"enabled\": true,\n"
        "      \"move_type\": \"MOVE_AVOID_IF_CRAMPED\",\n"
        "      \"attack_type\": \"ATTACK_RANGED\",\n"
        "      \"conditions\": [\"pref_hyper\", \"not_thrown_too_much\"]\n"
        "    }\n"
        "  }\n"
        "}\n";

    CU_ASSERT_TRUE_FATAL(write_whole_file(&tactics_path, partial, strlen(partial)));

    ai_tactic_reset_config_cache();
    CU_ASSERT_EQUAL(ai_tactic_config_condition_count(TACTIC_SHOOT), 2);
    CU_ASSERT_TRUE(ai_tactic_config_has_condition(TACTIC_SHOOT, "pref_hyper"));
    CU_ASSERT_TRUE(ai_tactic_config_has_condition(TACTIC_SHOOT, "not_thrown_too_much"));
    CU_ASSERT_FALSE(ai_tactic_config_has_condition(TACTIC_SHOOT, "enemy_stunned"));

    CU_ASSERT_TRUE_FATAL(write_whole_file(&tactics_path, original, original_size));
    ai_tactic_reset_config_cache();
}

void test_tactic_conditions_invalid_id_or_query_returns_false(void) {
    ensure_test_runtime_ready();
    ai_tactic_reset_config_cache();

    CU_ASSERT_EQUAL(ai_tactic_config_condition_count(0), 0);
    CU_ASSERT_EQUAL(ai_tactic_config_condition_count(99), 0);
    CU_ASSERT_FALSE(ai_tactic_config_conditions_supported(0));
    CU_ASSERT_FALSE(ai_tactic_config_conditions_supported(99));
    CU_ASSERT_FALSE(ai_tactic_config_has_condition(0, "pref_hyper"));
    CU_ASSERT_FALSE(ai_tactic_config_has_condition(99, "pref_hyper"));
    CU_ASSERT_FALSE(ai_tactic_config_has_condition(TACTIC_SHOOT, NULL));
    CU_ASSERT_FALSE(ai_tactic_config_has_condition(TACTIC_SHOOT, ""));
    CU_ASSERT_FALSE(ai_tactic_config_condition_token_supported(NULL));
    CU_ASSERT_FALSE(ai_tactic_config_condition_token_supported(""));
    CU_ASSERT_TRUE(ai_tactic_config_condition_token_supported("pref_hyper"));
    CU_ASSERT_FALSE(ai_tactic_config_condition_token_supported("totally_unknown_condition"));
}

void test_tactic_conditions_unknown_token_reports_unsupported(void) {
    ensure_test_runtime_ready();
    CU_ASSERT_TRUE_FATAL(resource_path_init());

    path tactics_path = get_resource_filename("ai_config/tactics.json");

    char original[16384] = {0};
    size_t original_size = 0;
    CU_ASSERT_TRUE_FATAL(read_whole_file(&tactics_path, original, sizeof(original), &original_size));

    const char *partial =
        "{\n"
        "  \"tactics\": {\n"
        "    \"SHOOT\": {\n"
        "      \"id\": 5,\n"
        "      \"enabled\": true,\n"
        "      \"move_type\": \"MOVE_AVOID_IF_CRAMPED\",\n"
        "      \"attack_type\": \"ATTACK_RANGED\",\n"
        "      \"conditions\": [\"pref_hyper\", \"unknown_condition_token\"]\n"
        "    }\n"
        "  }\n"
        "}\n";

    CU_ASSERT_TRUE_FATAL(write_whole_file(&tactics_path, partial, strlen(partial)));

    ai_tactic_reset_config_cache();
    CU_ASSERT_EQUAL(ai_tactic_config_condition_count(TACTIC_SHOOT), 2);
    CU_ASSERT_TRUE(ai_tactic_config_has_condition(TACTIC_SHOOT, "pref_hyper"));
    CU_ASSERT_TRUE(ai_tactic_config_has_condition(TACTIC_SHOOT, "unknown_condition_token"));
    CU_ASSERT_FALSE(ai_tactic_config_conditions_supported(TACTIC_SHOOT));

    CU_ASSERT_TRUE_FATAL(write_whole_file(&tactics_path, original, original_size));
    ai_tactic_reset_config_cache();
}

void test_tactic_conditions_match_context_with_deterministic_conditions(void) {
    ensure_test_runtime_ready();
    CU_ASSERT_TRUE_FATAL(resource_path_init());

    path tactics_path = get_resource_filename("ai_config/tactics.json");

    char original[16384] = {0};
    size_t original_size = 0;
    CU_ASSERT_TRUE_FATAL(read_whole_file(&tactics_path, original, sizeof(original), &original_size));

    const char *partial =
        "{\n"
        "  \"tactics\": {\n"
        "    \"SHOOT\": {\n"
        "      \"id\": 5,\n"
        "      \"enabled\": true,\n"
        "      \"move_type\": \"MOVE_AVOID_IF_CRAMPED\",\n"
        "      \"attack_type\": \"ATTACK_RANGED\",\n"
        "      \"conditions\": [\"has_projectiles\", \"enemy_not_cramped\", \"not_thrown_too_much\"]\n"
        "    }\n"
        "  }\n"
        "}\n";

    CU_ASSERT_TRUE_FATAL(write_whole_file(&tactics_path, partial, strlen(partial)));

    ai_tactic_reset_config_cache();

    sd_pilot pilot;
    memset(&pilot, 0, sizeof(pilot));

    CU_ASSERT_TRUE(ai_tactic_conditions_match_context(TACTIC_SHOOT, HAR_JAGUAR, RANGE_MID, 1, 0, &pilot));
    CU_ASSERT_FALSE(ai_tactic_conditions_match_context(TACTIC_SHOOT, HAR_JAGUAR, RANGE_CRAMPED, 1, 0, &pilot));
    CU_ASSERT_FALSE(ai_tactic_conditions_match_context(TACTIC_SHOOT, HAR_JAGUAR, RANGE_MID, 9, 0, &pilot));
    CU_ASSERT_FALSE(ai_tactic_conditions_match_context(TACTIC_SHOOT, HAR_KATANA, RANGE_MID, 1, 0, &pilot));

    CU_ASSERT_TRUE_FATAL(write_whole_file(&tactics_path, original, original_size));
    ai_tactic_reset_config_cache();
}

void test_tactic_conditions_match_context_unknown_condition_ignored(void) {
    ensure_test_runtime_ready();
    CU_ASSERT_TRUE_FATAL(resource_path_init());

    path tactics_path = get_resource_filename("ai_config/tactics.json");

    char original[16384] = {0};
    size_t original_size = 0;
    CU_ASSERT_TRUE_FATAL(read_whole_file(&tactics_path, original, sizeof(original), &original_size));

    const char *partial =
        "{\n"
        "  \"tactics\": {\n"
        "    \"SHOOT\": {\n"
        "      \"id\": 5,\n"
        "      \"enabled\": true,\n"
        "      \"move_type\": \"MOVE_AVOID_IF_CRAMPED\",\n"
        "      \"attack_type\": \"ATTACK_RANGED\",\n"
        "      \"conditions\": [\"unknown_condition_token\"]\n"
        "    }\n"
        "  }\n"
        "}\n";

    CU_ASSERT_TRUE_FATAL(write_whole_file(&tactics_path, partial, strlen(partial)));

    ai_tactic_reset_config_cache();

    sd_pilot pilot;
    memset(&pilot, 0, sizeof(pilot));

    CU_ASSERT_TRUE(ai_tactic_conditions_match_context(TACTIC_SHOOT, HAR_JAGUAR, RANGE_MID, 20, 20, &pilot));

    CU_ASSERT_TRUE_FATAL(write_whole_file(&tactics_path, original, original_size));
    ai_tactic_reset_config_cache();
}

void test_tactic_conditions_match_context_invalid_input_returns_false(void) {
    ai_tactic_reset_config_cache();

    sd_pilot pilot;
    memset(&pilot, 0, sizeof(pilot));

    CU_ASSERT_FALSE(ai_tactic_conditions_match_context(0, HAR_JAGUAR, RANGE_MID, 0, 0, &pilot));
    CU_ASSERT_FALSE(ai_tactic_conditions_match_context(99, HAR_JAGUAR, RANGE_MID, 0, 0, &pilot));
    CU_ASSERT_FALSE(ai_tactic_conditions_match_context(TACTIC_SHOOT, HAR_JAGUAR, RANGE_MID, 0, 0, NULL));
}

void test_tactic_conditions_match_context_close_has_charge(void) {
    ensure_test_runtime_ready();
    CU_ASSERT_TRUE_FATAL(resource_path_init());

    path tactics_path = get_resource_filename("ai_config/tactics.json");

    char original[16384] = {0};
    size_t original_size = 0;
    CU_ASSERT_TRUE_FATAL(read_whole_file(&tactics_path, original, sizeof(original), &original_size));

    const char *partial =
        "{\n"
        "  \"tactics\": {\n"
        "    \"CLOSE\": {\n"
        "      \"id\": 8,\n"
        "      \"enabled\": true,\n"
        "      \"move_type\": \"MOVE_CLOSE\",\n"
        "      \"attack_type\": \"ATTACK_RANDOM\",\n"
        "      \"conditions\": [\"has_charge\", \"enemy_not_cramped\"]\n"
        "    }\n"
        "  }\n"
        "}\n";

    CU_ASSERT_TRUE_FATAL(write_whole_file(&tactics_path, partial, strlen(partial)));

    ai_tactic_reset_config_cache();

    sd_pilot pilot;
    memset(&pilot, 0, sizeof(pilot));

    CU_ASSERT_TRUE(ai_tactic_conditions_match_context(TACTIC_CLOSE, HAR_JAGUAR, RANGE_MID, 0, 0, &pilot));
    CU_ASSERT_FALSE(ai_tactic_conditions_match_context(TACTIC_CLOSE, HAR_JAGUAR, RANGE_CRAMPED, 0, 0, &pilot));
    CU_ASSERT_FALSE(ai_tactic_conditions_match_context(TACTIC_CLOSE, HAR_NOVA, RANGE_MID, 0, 0, &pilot));

    CU_ASSERT_TRUE_FATAL(write_whole_file(&tactics_path, original, original_size));
    ai_tactic_reset_config_cache();
}

void test_tactic_conditions_match_context_all_tactics_enemy_not_cramped(void) {
    ensure_test_runtime_ready();
    CU_ASSERT_TRUE_FATAL(resource_path_init());

    path tactics_path = get_resource_filename("ai_config/tactics.json");

    char original[16384] = {0};
    size_t original_size = 0;
    CU_ASSERT_TRUE_FATAL(read_whole_file(&tactics_path, original, sizeof(original), &original_size));

    const char *partial =
        "{\n"
        "  \"tactics\": {\n"
        "    \"ESCAPE\":  {\"id\": 1,  \"enabled\": true, \"move_type\": \"MOVE_AVOID\",              \"attack_type\": \"NONE\",                  \"conditions\": [\"enemy_not_cramped\"]},\n"
        "    \"TURTLE\":  {\"id\": 2,  \"enabled\": true, \"move_type\": \"MOVE_BLOCK\",              \"attack_type\": \"NONE\",                  \"conditions\": [\"enemy_not_cramped\"]},\n"
        "    \"GRAB\":    {\"id\": 3,  \"enabled\": true, \"move_type\": \"MOVE_CLOSE\",              \"attack_type\": \"ATTACK_GRAB\",           \"conditions\": [\"enemy_not_cramped\"]},\n"
        "    \"SPAM\":    {\"id\": 4,  \"enabled\": true, \"move_type\": \"NONE\",                    \"attack_type\": \"ATTACK_ID_OR_LIGHT\",    \"conditions\": [\"enemy_not_cramped\"]},\n"
        "    \"SHOOT\":   {\"id\": 5,  \"enabled\": true, \"move_type\": \"MOVE_AVOID_IF_CRAMPED\",   \"attack_type\": \"ATTACK_RANGED\",         \"conditions\": [\"enemy_not_cramped\"]},\n"
        "    \"TRIP\":    {\"id\": 6,  \"enabled\": true, \"move_type\": \"MOVE_CLOSE\",              \"attack_type\": \"ATTACK_TRIP\",           \"conditions\": [\"enemy_not_cramped\"]},\n"
        "    \"QUICK\":   {\"id\": 7,  \"enabled\": true, \"move_type\": \"MOVE_CLOSE\",              \"attack_type\": \"ATTACK_LIGHT\",          \"conditions\": [\"enemy_not_cramped\"]},\n"
        "    \"CLOSE\":   {\"id\": 8,  \"enabled\": true, \"move_type\": \"MOVE_CLOSE\",              \"attack_type\": \"ATTACK_RANDOM\",         \"conditions\": [\"enemy_not_cramped\"]},\n"
        "    \"FLY\":     {\"id\": 9,  \"enabled\": true, \"move_type\": \"MOVE_HIGH_JUMP\",          \"attack_type\": \"ATTACK_JUMP_OR_NONE\",   \"conditions\": [\"enemy_not_cramped\"]},\n"
        "    \"PUSH\":    {\"id\": 10, \"enabled\": true, \"move_type\": \"NONE\",                    \"attack_type\": \"ATTACK_PUSH_OR_HEAVY\",  \"conditions\": [\"enemy_not_cramped\"]},\n"
        "    \"COUNTER\": {\"id\": 11, \"enabled\": true, \"move_type\": \"MOVE_BLOCK_IF_NOT_CRAMPED\", \"attack_type\": \"ATTACK_TRIP_OR_HEAVY\", \"conditions\": [\"enemy_not_cramped\"]}\n"
        "  }\n"
        "}\n";

    CU_ASSERT_TRUE_FATAL(write_whole_file(&tactics_path, partial, strlen(partial)));

    ai_tactic_reset_config_cache();

    sd_pilot pilot;
    memset(&pilot, 0, sizeof(pilot));

    for(int tactic_id = TACTIC_ESCAPE; tactic_id <= TACTIC_COUNTER; tactic_id++) {
        CU_ASSERT_TRUE(ai_tactic_conditions_match_context(tactic_id, HAR_JAGUAR, RANGE_MID, 0, 0, &pilot));
        CU_ASSERT_FALSE(ai_tactic_conditions_match_context(tactic_id, HAR_JAGUAR, RANGE_CRAMPED, 0, 0, &pilot));
    }

    CU_ASSERT_TRUE_FATAL(write_whole_file(&tactics_path, original, original_size));
    ai_tactic_reset_config_cache();
}

void test_tactic_conditions_match_context_push_has_push(void) {
    ensure_test_runtime_ready();
    CU_ASSERT_TRUE_FATAL(resource_path_init());

    path tactics_path = get_resource_filename("ai_config/tactics.json");

    char original[16384] = {0};
    size_t original_size = 0;
    CU_ASSERT_TRUE_FATAL(read_whole_file(&tactics_path, original, sizeof(original), &original_size));

    const char *partial =
        "{\n"
        "  \"tactics\": {\n"
        "    \"PUSH\": {\n"
        "      \"id\": 10,\n"
        "      \"enabled\": true,\n"
        "      \"move_type\": \"NONE\",\n"
        "      \"attack_type\": \"ATTACK_PUSH_OR_HEAVY\",\n"
        "      \"conditions\": [\"has_push\", \"enemy_not_cramped\"]\n"
        "    }\n"
        "  }\n"
        "}\n";

    CU_ASSERT_TRUE_FATAL(write_whole_file(&tactics_path, partial, strlen(partial)));

    ai_tactic_reset_config_cache();

    sd_pilot pilot;
    memset(&pilot, 0, sizeof(pilot));

    CU_ASSERT_TRUE(ai_tactic_conditions_match_context(TACTIC_PUSH, HAR_JAGUAR, RANGE_MID, 0, 0, &pilot));
    CU_ASSERT_FALSE(ai_tactic_conditions_match_context(TACTIC_PUSH, HAR_CHRONOS, RANGE_MID, 0, 0, &pilot));
    CU_ASSERT_FALSE(ai_tactic_conditions_match_context(TACTIC_PUSH, HAR_JAGUAR, RANGE_CRAMPED, 0, 0, &pilot));

    CU_ASSERT_TRUE_FATAL(write_whole_file(&tactics_path, original, original_size));
    ai_tactic_reset_config_cache();
}

void test_tactic_conditions_match_context_fly_not_shot_too_much(void) {
    ensure_test_runtime_ready();
    CU_ASSERT_TRUE_FATAL(resource_path_init());

    path tactics_path = get_resource_filename("ai_config/tactics.json");

    char original[16384] = {0};
    size_t original_size = 0;
    CU_ASSERT_TRUE_FATAL(read_whole_file(&tactics_path, original, sizeof(original), &original_size));

    const char *partial =
        "{\n"
        "  \"tactics\": {\n"
        "    \"FLY\": {\n"
        "      \"id\": 9,\n"
        "      \"enabled\": true,\n"
        "      \"move_type\": \"MOVE_HIGH_JUMP\",\n"
        "      \"attack_type\": \"ATTACK_JUMP_OR_NONE\",\n"
        "      \"conditions\": [\"not_shot_too_much\"]\n"
        "    }\n"
        "  }\n"
        "}\n";

    CU_ASSERT_TRUE_FATAL(write_whole_file(&tactics_path, partial, strlen(partial)));

    ai_tactic_reset_config_cache();

    sd_pilot pilot;
    memset(&pilot, 0, sizeof(pilot));

    CU_ASSERT_TRUE(ai_tactic_conditions_match_context(TACTIC_FLY, HAR_GARGOYLE, RANGE_MID, 0, 3, &pilot));
    CU_ASSERT_FALSE(ai_tactic_conditions_match_context(TACTIC_FLY, HAR_GARGOYLE, RANGE_MID, 0, 5, &pilot));

    CU_ASSERT_TRUE_FATAL(write_whole_file(&tactics_path, original, original_size));
    ai_tactic_reset_config_cache();
}

void test_tactic_conditions_match_context_grab_not_thrown_too_much(void) {
    ensure_test_runtime_ready();
    CU_ASSERT_TRUE_FATAL(resource_path_init());

    path tactics_path = get_resource_filename("ai_config/tactics.json");

    char original[16384] = {0};
    size_t original_size = 0;
    CU_ASSERT_TRUE_FATAL(read_whole_file(&tactics_path, original, sizeof(original), &original_size));

    const char *partial =
        "{\n"
        "  \"tactics\": {\n"
        "    \"GRAB\": {\n"
        "      \"id\": 3,\n"
        "      \"enabled\": true,\n"
        "      \"move_type\": \"MOVE_CLOSE\",\n"
        "      \"attack_type\": \"ATTACK_GRAB\",\n"
        "      \"conditions\": [\"not_thrown_too_much\"]\n"
        "    }\n"
        "  }\n"
        "}\n";

    CU_ASSERT_TRUE_FATAL(write_whole_file(&tactics_path, partial, strlen(partial)));

    ai_tactic_reset_config_cache();

    sd_pilot pilot;
    memset(&pilot, 0, sizeof(pilot));

    CU_ASSERT_TRUE(ai_tactic_conditions_match_context(TACTIC_GRAB, HAR_JAGUAR, RANGE_MID, 3, 0, &pilot));
    CU_ASSERT_FALSE(ai_tactic_conditions_match_context(TACTIC_GRAB, HAR_JAGUAR, RANGE_MID, 4, 0, &pilot));

    CU_ASSERT_TRUE_FATAL(write_whole_file(&tactics_path, original, original_size));
    ai_tactic_reset_config_cache();
}

void ai_tactic_engine_test_suite(CU_pSuite suite) {
    if(CU_add_test(suite, "tactic enabled: invalid id false", test_tactic_is_enabled_invalid_id_returns_false) == NULL)
        return;
    if(CU_add_test(suite, "tactic enabled: valid ids true by default", test_tactic_is_enabled_defaults_true_for_valid_ids) == NULL)
        return;
    if(CU_add_test(suite, "tactic enabled: config disabled flag", test_tactic_is_enabled_respects_disabled_flag_in_config) == NULL)
        return;
    if(CU_add_test(suite, "tactic enabled: partial config keeps unspecified enabled",
                   test_tactic_is_enabled_partial_config_keeps_unspecified_enabled) == NULL)
        return;
    if(CU_add_test(suite, "tactic first_enabled: returns first enabled in list",
                   test_tactic_first_enabled_returns_first_enabled_in_list) == NULL)
        return;
    if(CU_add_test(suite, "tactic first_enabled: skips disabled entries",
                   test_tactic_first_enabled_skips_disabled_entries) == NULL)
        return;
    if(CU_add_test(suite, "tactic metadata: strings loaded from config",
                   test_tactic_metadata_strings_loaded_from_config) == NULL)
        return;
    if(CU_add_test(suite, "tactic metadata: missing fields return empty",
                   test_tactic_metadata_missing_fields_return_empty_string) == NULL)
        return;
    if(CU_add_test(suite, "tactic metadata: default tokens are supported",
                   test_tactic_metadata_tokens_supported_for_default_config) == NULL)
        return;
    if(CU_add_test(suite, "tactic metadata: all default tactic tokens supported",
                   test_tactic_metadata_tokens_supported_for_all_default_tactics) == NULL)
        return;
    if(CU_add_test(suite, "tactic metadata: support queries reject invalid ids",
                   test_tactic_metadata_supported_queries_reject_invalid_tactic_id) == NULL)
        return;
    if(CU_add_test(suite, "tactic metadata: unknown tokens unsupported",
                   test_tactic_metadata_tokens_unknown_are_reported_unsupported) == NULL)
        return;
    if(CU_add_test(suite, "tactic conditions: missing by default",
                   test_tactic_conditions_missing_by_default) == NULL)
        return;
    if(CU_add_test(suite, "tactic conditions: parsed from config",
                   test_tactic_conditions_parsed_from_config) == NULL)
        return;
    if(CU_add_test(suite, "tactic conditions: invalid id or query",
                   test_tactic_conditions_invalid_id_or_query_returns_false) == NULL)
        return;
    if(CU_add_test(suite, "tactic conditions: unknown token unsupported",
                   test_tactic_conditions_unknown_token_reports_unsupported) == NULL)
        return;
    if(CU_add_test(suite, "tactic conditions: match context deterministic",
                   test_tactic_conditions_match_context_with_deterministic_conditions) == NULL)
        return;
    if(CU_add_test(suite, "tactic conditions: unknown condition ignored",
                   test_tactic_conditions_match_context_unknown_condition_ignored) == NULL)
        return;
    if(CU_add_test(suite, "tactic conditions: match context invalid input",
                   test_tactic_conditions_match_context_invalid_input_returns_false) == NULL)
        return;
    if(CU_add_test(suite, "tactic conditions: close has_charge",
                   test_tactic_conditions_match_context_close_has_charge) == NULL)
        return;
    if(CU_add_test(suite, "tactic conditions: all tactics enemy_not_cramped",
                   test_tactic_conditions_match_context_all_tactics_enemy_not_cramped) == NULL)
        return;
    if(CU_add_test(suite, "tactic conditions: push has_push",
                   test_tactic_conditions_match_context_push_has_push) == NULL)
        return;
    if(CU_add_test(suite, "tactic conditions: fly not_shot_too_much",
                   test_tactic_conditions_match_context_fly_not_shot_too_much) == NULL)
        return;
    if(CU_add_test(suite, "tactic conditions: grab not_thrown_too_much",
                   test_tactic_conditions_match_context_grab_not_thrown_too_much) == NULL)
        return;
}
