/**
 * AI skills config loader implementation
 */

#include "game/ai/ai_skills_config_loader.h"

#include "controller/controller.h"
#include "game/ai/ai_tactic_engine.h"
#include "game/ai/ai_utils.h"
#include "game/common_defines.h"
#include "resources/modmanager.h"
#include "resources/resource_files.h"
#include "utils/allocator.h"
#include "utils/path.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool g_loaded[NUMBER_OF_HAR_TYPES] = {false};
static ai_char_config g_configs[NUMBER_OF_HAR_TYPES] = {{0}};

static const char *har_config_name(int har_id) {
    switch(har_id) {
        case HAR_JAGUAR:
            return "jaguar";
        case HAR_SHADOW:
            return "shadow";
        case HAR_THORN:
            return "thorn";
        case HAR_PYROS:
            return "pyros";
        case HAR_ELECTRA:
            return "electra";
        case HAR_KATANA:
            return "katana";
        case HAR_SHREDDER:
            return "shredder";
        case HAR_FLAIL:
            return "flail";
        case HAR_GARGOYLE:
            return "gargoyle";
        case HAR_CHRONOS:
            return "chronos";
        case HAR_NOVA:
            return "nova";
        default:
            return NULL;
    }
}

static bool parse_json_id(const char *json, int *out_id) {
    const char *id_key = strstr(json, "\"id\"");
    if(id_key == NULL) {
        return false;
    }

    const char *colon = strchr(id_key, ':');
    if(colon == NULL) {
        return false;
    }

    const char *cursor = colon + 1;
    while(*cursor != '\0' && isspace((unsigned char)*cursor)) {
        cursor++;
    }

    errno = 0;
    char *endptr = NULL;
    long parsed = strtol(cursor, &endptr, 10);
    if(cursor == endptr || errno != 0 || endptr == NULL) {
        return false;
    }

    *out_id = (int)parsed;
    return true;
}

static const char *find_matching_bracket(const char *start) {
    int depth = 0;
    bool in_string = false;
    bool escaped = false;

    for(const char *cursor = start; *cursor != '\0'; cursor++) {
        char c = *cursor;

        if(in_string) {
            if(escaped) {
                escaped = false;
                continue;
            }
            if(c == '\\') {
                escaped = true;
                continue;
            }
            if(c == '"') {
                in_string = false;
            }
            continue;
        }

        if(c == '"') {
            in_string = true;
            continue;
        }

        if(c == '[') {
            depth++;
        } else if(c == ']') {
            depth--;
            if(depth == 0) {
                return cursor;
            }
            if(depth < 0) {
                return NULL;
            }
        }
    }
    return NULL;
}

bool ai_parse_sequence_token(const char *token, int *out_bits) {
    if(token == NULL || out_bits == NULL || token[0] == '\0') {
        return false;
    }

    const char *plus = strchr(token, '+');
    if(plus != NULL) {
        char left[8] = {0};
        char right[8] = {0};
        size_t left_len = (size_t)(plus - token);
        if(left_len == 0 || left_len >= sizeof(left)) {
            return false;
        }
        memcpy(left, token, left_len);
        strncpy(right, plus + 1, sizeof(right) - 1);

        int left_bits = 0;
        int right_bits = 0;
        if(!ai_parse_sequence_token(left, &left_bits) || !ai_parse_sequence_token(right, &right_bits)) {
            return false;
        }

        *out_bits = left_bits | right_bits;
        return true;
    }

    if(strcmp(token, "F") == 0) {
        *out_bits = ACT_RIGHT;
        return true;
    }
    if(strcmp(token, "B") == 0) {
        *out_bits = ACT_LEFT;
        return true;
    }
    if(strcmp(token, "D") == 0) {
        *out_bits = ACT_DOWN;
        return true;
    }
    if(strcmp(token, "U") == 0) {
        *out_bits = ACT_UP;
        return true;
    }
    if(strcmp(token, "DF") == 0) {
        *out_bits = ACT_DOWN | ACT_RIGHT;
        return true;
    }
    if(strcmp(token, "DB") == 0) {
        *out_bits = ACT_DOWN | ACT_LEFT;
        return true;
    }
    if(strcmp(token, "5") == 0) {
        *out_bits = ACT_STOP;
        return true;
    }
    if(strcmp(token, "P") == 0) {
        *out_bits = ACT_PUNCH;
        return true;
    }
    if(strcmp(token, "K") == 0) {
        *out_bits = ACT_KICK;
        return true;
    }

    return false;
}

ai_move_condition ai_parse_condition_string(const char *cond) {
    if(cond == NULL) {
        return MOVE_COND_NONE;
    }
    if(strcmp(cond, "high_difficulty") == 0) {
        return MOVE_COND_HIGH_DIFFICULTY;
    }
    if(strcmp(cond, "special_preferred") == 0) {
        return MOVE_COND_SPECIAL_PREF;
    }
    if(strcmp(cond, "low_preferred") == 0) {
        return MOVE_COND_LOW_PREFERRED;
    }
    if(strcmp(cond, "jump_preferred") == 0) {
        return MOVE_COND_JUMP_PREFERRED;
    }
    if(strcmp(cond, "roll_d2") == 0) {
        return MOVE_COND_ROLL_D2;
    }
    if(strcmp(cond, "roll_d3") == 0) {
        return MOVE_COND_ROLL_D3;
    }
    if(strcmp(cond, "roll_d4") == 0) {
        return MOVE_COND_ROLL_D4;
    }
    if(strcmp(cond, "roll_d10") == 0) {
        return MOVE_COND_ROLL_D10;
    }
    if(strcmp(cond, "roll_d20") == 0) {
        return MOVE_COND_ROLL_D20;
    }
    if(strcmp(cond, "enemy_not_stunned") == 0) {
        return MOVE_COND_ENEMY_NOT_STUNNED;
    }
    return MOVE_COND_NONE;
}

int ai_parse_tactic_string(const char *tactic) {
    if(tactic == NULL) {
        return -1;
    }

    if(strcmp(tactic, "escape") == 0) {
        return TACTIC_ESCAPE;
    }
    if(strcmp(tactic, "turtle") == 0) {
        return TACTIC_TURTLE;
    }
    if(strcmp(tactic, "grab") == 0) {
        return TACTIC_GRAB;
    }
    if(strcmp(tactic, "spam") == 0) {
        return TACTIC_SPAM;
    }
    if(strcmp(tactic, "shoot") == 0) {
        return TACTIC_SHOOT;
    }
    if(strcmp(tactic, "trip") == 0) {
        return TACTIC_TRIP;
    }
    if(strcmp(tactic, "quick") == 0) {
        return TACTIC_QUICK;
    }
    if(strcmp(tactic, "close") == 0) {
        return TACTIC_CLOSE;
    }
    if(strcmp(tactic, "fly") == 0) {
        return TACTIC_FLY;
    }
    if(strcmp(tactic, "push") == 0) {
        return TACTIC_PUSH;
    }
    if(strcmp(tactic, "counter") == 0) {
        return TACTIC_COUNTER;
    }

    return -1;
}

static const char *find_matching_brace(const char *start) {
    int depth = 0;
    bool in_string = false;
    bool escaped = false;

    for(const char *cursor = start; *cursor != '\0'; cursor++) {
        char c = *cursor;

        if(in_string) {
            if(escaped) {
                escaped = false;
                continue;
            }
            if(c == '\\') {
                escaped = true;
                continue;
            }
            if(c == '"') {
                in_string = false;
            }
            continue;
        }

        if(c == '"') {
            in_string = true;
            continue;
        }

        if(c == '{') {
            depth++;
        } else if(c == '}') {
            depth--;
            if(depth == 0) {
                return cursor;
            }
            if(depth < 0) {
                return NULL;
            }
        }
    }

    return NULL;
}

static bool find_json_array_bounds(const char *json, const char *key, const char **arr_start, const char **arr_end) {
    char field[64];
    snprintf(field, sizeof(field), "\"%s\"", key);

    const char *found = strstr(json, field);
    if(found == NULL) {
        return false;
    }

    const char *colon = strchr(found + strlen(field), ':');
    if(colon == NULL) {
        return false;
    }

    const char *start = strchr(colon + 1, '[');
    if(start == NULL) {
        return false;
    }

    const char *end = find_matching_bracket(start);
    if(end == NULL) {
        return false;
    }

    *arr_start = start;
    *arr_end = end;
    return true;
}

static bool extract_string_field(const char *obj_start, const char *obj_end, const char *key, char *out, size_t out_len) {
    char field[64];
    snprintf(field, sizeof(field), "\"%s\"", key);

    const char *found = strstr(obj_start, field);
    if(found == NULL || found >= obj_end) {
        return false;
    }

    const char *colon = strchr(found + strlen(field), ':');
    if(colon == NULL || colon >= obj_end) {
        return false;
    }

    const char *q1 = strchr(colon + 1, '"');
    if(q1 == NULL || q1 >= obj_end) {
        return false;
    }

    const char *q2 = q1 + 1;
    bool escaped = false;
    while(q2 < obj_end) {
        if(!escaped && *q2 == '"') {
            break;
        }
        if(!escaped && *q2 == '\\') {
            escaped = true;
        } else {
            escaped = false;
        }
        q2++;
    }

    if(q2 >= obj_end) {
        return false;
    }

    size_t n = (size_t)(q2 - (q1 + 1));
    if(n >= out_len) {
        n = out_len - 1;
    }
    memcpy(out, q1 + 1, n);
    out[n] = '\0';
    return true;
}

typedef void (*json_string_cb)(const char *value, void *userdata);

static void for_each_string_in_array_field(const char *obj_start, const char *obj_end, const char *key,
                                           json_string_cb cb, void *userdata) {
    char field[64];
    snprintf(field, sizeof(field), "\"%s\"", key);

    const char *found = strstr(obj_start, field);
    if(found == NULL || found >= obj_end) {
        return;
    }

    const char *colon = strchr(found + strlen(field), ':');
    if(colon == NULL || colon >= obj_end) {
        return;
    }

    const char *arr_start = strchr(colon + 1, '[');
    if(arr_start == NULL || arr_start >= obj_end) {
        return;
    }

    const char *arr_end = find_matching_bracket(arr_start);
    if(arr_end == NULL || arr_end > obj_end) {
        return;
    }

    bool in_string = false;
    bool escaped = false;
    char value[64];
    size_t idx = 0;

    for(const char *p = arr_start + 1; p < arr_end; p++) {
        char c = *p;

        if(!in_string) {
            if(c == '"') {
                in_string = true;
                escaped = false;
                idx = 0;
            }
            continue;
        }

        if(escaped) {
            if(idx < sizeof(value) - 1) {
                value[idx++] = c;
            }
            escaped = false;
            continue;
        }

        if(c == '\\') {
            escaped = true;
            continue;
        }

        if(c == '"') {
            value[idx] = '\0';
            cb(value, userdata);
            in_string = false;
            continue;
        }

        if(idx < sizeof(value) - 1) {
            value[idx++] = c;
        }
    }
}

static ai_move_range parse_range(const char *range_str) {
    if(range_str == NULL || strcmp(range_str, "ANY") == 0) {
        return MOVE_RANGE_ANY;
    }
    if(strcmp(range_str, "CLOSE") == 0) {
        return MOVE_RANGE_CLOSE;
    }
    if(strcmp(range_str, "MID") == 0) {
        return MOVE_RANGE_MID;
    }
    if(strcmp(range_str, "FAR") == 0) {
        return MOVE_RANGE_FAR;
    }
    return MOVE_RANGE_ANY;
}

typedef struct {
    ai_move_def *move;
} seq_ctx;

static void sequence_value_cb(const char *value, void *userdata) {
    seq_ctx *ctx = (seq_ctx *)userdata;
    if(ctx->move->input_count >= AI_MOVE_MAX_INPUTS) {
        return;
    }

    int bits = 0;
    if(ai_parse_sequence_token(value, &bits)) {
        ctx->move->inputs[ctx->move->input_count++] = bits;
    }
}

typedef struct {
    ai_move_def *move;
} cond_ctx;

static void condition_value_cb(const char *value, void *userdata) {
    cond_ctx *ctx = (cond_ctx *)userdata;
    ai_move_condition cond = ai_parse_condition_string(value);
    ctx->move->conditions = (ai_move_condition)(ctx->move->conditions | cond);
}

typedef struct {
    ai_move_def *move;
} tactic_ctx;

static void tactic_value_cb(const char *value, void *userdata) {
    tactic_ctx *ctx = (tactic_ctx *)userdata;
    if(ctx->move->follow_up_tactic_count >= AI_MOVE_MAX_FOLLOW_TACTICS) {
        return;
    }

    int tactic = ai_parse_tactic_string(value);
    if(tactic >= 0) {
        ctx->move->follow_up_tactics[ctx->move->follow_up_tactic_count++] = tactic;
    }
}

bool ai_parse_move_array(const char *json, const char *key, ai_move_def *out, uint8_t *out_count, uint8_t max_count) {
    if(json == NULL || key == NULL || out == NULL || out_count == NULL) {
        return false;
    }

    const char *arr_start = NULL;
    const char *arr_end = NULL;
    if(!find_json_array_bounds(json, key, &arr_start, &arr_end)) {
        return false;
    }

    *out_count = 0;

    const char *cursor = arr_start + 1;
    while(cursor < arr_end && *out_count < max_count) {
        const char *obj_start = strchr(cursor, '{');
        if(obj_start == NULL || obj_start >= arr_end) {
            break;
        }

        const char *obj_end = find_matching_brace(obj_start);
        if(obj_end == NULL || obj_end > arr_end) {
            break;
        }

        ai_move_def *move = &out[*out_count];
        memset(move, 0, sizeof(*move));
        move->range_min = MOVE_RANGE_ANY;
        move->range_max = MOVE_RANGE_FAR;

        extract_string_field(obj_start, obj_end, "name", move->name, sizeof(move->name));

        char range_buf[16] = {0};
        if(extract_string_field(obj_start, obj_end, "range_min", range_buf, sizeof(range_buf))) {
            move->range_min = parse_range(range_buf);
        }

        char range_max_buf[16] = {0};
        if(extract_string_field(obj_start, obj_end, "range_max", range_max_buf, sizeof(range_max_buf))) {
            move->range_max = parse_range(range_max_buf);
        }

        seq_ctx sctx = {move};
        for_each_string_in_array_field(obj_start, obj_end, "sequence", sequence_value_cb, &sctx);

        cond_ctx cctx = {move};
        for_each_string_in_array_field(obj_start, obj_end, "conditions", condition_value_cb, &cctx);

        tactic_ctx tctx = {move};
        for_each_string_in_array_field(obj_start, obj_end, "follow_up_tactics", tactic_value_cb, &tctx);

        (*out_count)++;
        cursor = obj_end + 1;
    }

    return true;
}

static void ai_skills_overlay_cb(const char *json_buf, void *userdata);

static void load_har_config(int har_id) {
    if(har_id < 0 || har_id >= NUMBER_OF_HAR_TYPES || g_loaded[har_id]) {
        return;
    }

    g_loaded[har_id] = true;
    g_configs[har_id].har_id = har_id;
    g_configs[har_id].loaded_from_file = false;
    g_configs[har_id].has_charge_moves = har_has_charge(har_id);
    g_configs[har_id].has_push_moves = har_has_push(har_id);
    g_configs[har_id].has_projectile_moves = har_has_projectiles(har_id);
    g_configs[har_id].charge_move_count = 0;
    g_configs[har_id].push_move_count = 0;
    g_configs[har_id].projectile_move_count = 0;

    const char *name = har_config_name(har_id);
    if(name == NULL) {
        return;
    }

    char rel_path[128];
    snprintf(rel_path, sizeof(rel_path), "ai_config/hars/%s.json", name);

    path cfg_file = get_resource_filename(rel_path);
    size_t file_size = 0;
    if(!path_filesize(&cfg_file, &file_size) || file_size == 0) {
        return;
    }

    char *json = omf_calloc(file_size + 1, sizeof(char));
    if(!path_read_file(&cfg_file, json, file_size)) {
        omf_free(json);
        return;
    }
    json[file_size] = '\0';

    int parsed_id = -1;
    if(!parse_json_id(json, &parsed_id) || parsed_id != har_id) {
        omf_free(json);
        return;
    }

    g_configs[har_id].loaded_from_file = true;
    ai_parse_move_array(json, "charge_moves", g_configs[har_id].charge_moves,
                        &g_configs[har_id].charge_move_count, AI_MAX_MOVES_PER_TYPE);
    ai_parse_move_array(json, "push_moves", g_configs[har_id].push_moves,
                        &g_configs[har_id].push_move_count, AI_MAX_MOVES_PER_TYPE);
    ai_parse_move_array(json, "projectile_moves", g_configs[har_id].projectile_moves,
                        &g_configs[har_id].projectile_move_count, AI_MAX_MOVES_PER_TYPE);
    g_configs[har_id].has_charge_moves = g_configs[har_id].charge_move_count > 0;
    g_configs[har_id].has_push_moves = g_configs[har_id].push_move_count > 0;
    g_configs[har_id].has_projectile_moves = g_configs[har_id].projectile_move_count > 0;

    omf_free(json);

    modmanager_apply_json_overlays(rel_path, ai_skills_overlay_cb, &g_configs[har_id]);
}

bool ai_skills_config_apply_overlay(ai_char_config *cfg, const char *json_buf) {
    if(cfg == NULL || json_buf == NULL) {
        return false;
    }

    bool changed = false;

    if(strstr(json_buf, "\"charge_moves\"") != NULL) {
        ai_parse_move_array(json_buf, "charge_moves", cfg->charge_moves, &cfg->charge_move_count,
                            AI_MAX_MOVES_PER_TYPE);
        cfg->has_charge_moves = cfg->charge_move_count > 0;
        changed = true;
    }

    if(strstr(json_buf, "\"push_moves\"") != NULL) {
        ai_parse_move_array(json_buf, "push_moves", cfg->push_moves, &cfg->push_move_count,
                            AI_MAX_MOVES_PER_TYPE);
        cfg->has_push_moves = cfg->push_move_count > 0;
        changed = true;
    }

    if(strstr(json_buf, "\"projectile_moves\"") != NULL) {
        ai_parse_move_array(json_buf, "projectile_moves", cfg->projectile_moves,
                            &cfg->projectile_move_count, AI_MAX_MOVES_PER_TYPE);
        cfg->has_projectile_moves = cfg->projectile_move_count > 0;
        changed = true;
    }

    return changed;
}

static void ai_skills_overlay_cb(const char *json_buf, void *userdata) {
    ai_char_config *cfg = (ai_char_config *)userdata;
    ai_skills_config_apply_overlay(cfg, json_buf);
}

const ai_char_config *ai_skills_config_get(int har_id) {
    if(har_id < 0 || har_id >= NUMBER_OF_HAR_TYPES) {
        return NULL;
    }

    load_har_config(har_id);
    return &g_configs[har_id];
}

void ai_skills_config_reset_cache(void) {
    memset(g_loaded, 0, sizeof(g_loaded));
    memset(g_configs, 0, sizeof(g_configs));
}
