/**
 * AI tactic engine implementation
 */

#include "game/ai/ai_tactic_engine.h"

#include "game/ai/ai_decision_engine.h"
#include "game/ai/ai_utils.h"
#include "game/game_state.h"
#include "game/objects/har.h"
#include "resources/resource_files.h"
#include "resources/ids.h"
#include "utils/allocator.h"
#include "utils/log.h"
#include "utils/path.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TIMES_THROWN 3
#define MAX_TIMES_SHOT 4
#define TACTIC_MOVE_TIMER_MAX 30
#define TACTIC_ATTACK_TIMER_MAX 3
#define TACTIC_JUMP_ATTACK_TIMER_MAX 12

static bool g_tactic_config_attempted = false;
static bool g_tactic_config_loaded = false;
static bool g_tactic_enabled[TACTIC_COUNTER + 1] = {false};
static char g_tactic_move_type[TACTIC_COUNTER + 1][32] = {{0}};
static char g_tactic_attack_type[TACTIC_COUNTER + 1][40] = {{0}};
static char g_tactic_conditions[TACTIC_COUNTER + 1][8][40] = {{{0}}};
static uint8_t g_tactic_condition_count[TACTIC_COUNTER + 1] = {0};

static bool move_type_token_supported(const char *move_cfg);
static bool attack_type_token_supported(const char *attack_cfg);
static bool condition_token_supported(const char *condition);
static void warn_unsupported_tactic_tokens(int tactic_type);

static const char *tactic_name_from_id(int tactic_id) {
    switch(tactic_id) {
        case TACTIC_ESCAPE:
            return "ESCAPE";
        case TACTIC_TURTLE:
            return "TURTLE";
        case TACTIC_GRAB:
            return "GRAB";
        case TACTIC_SPAM:
            return "SPAM";
        case TACTIC_SHOOT:
            return "SHOOT";
        case TACTIC_TRIP:
            return "TRIP";
        case TACTIC_QUICK:
            return "QUICK";
        case TACTIC_CLOSE:
            return "CLOSE";
        case TACTIC_FLY:
            return "FLY";
        case TACTIC_PUSH:
            return "PUSH";
        case TACTIC_COUNTER:
            return "COUNTER";
        default:
            return "UNKNOWN";
    }
}

void ai_tactic_reset_config_cache(void) {
    g_tactic_config_attempted = false;
    g_tactic_config_loaded = false;
    memset(g_tactic_enabled, 0, sizeof(g_tactic_enabled));
    memset(g_tactic_move_type, 0, sizeof(g_tactic_move_type));
    memset(g_tactic_attack_type, 0, sizeof(g_tactic_attack_type));
    memset(g_tactic_conditions, 0, sizeof(g_tactic_conditions));
    memset(g_tactic_condition_count, 0, sizeof(g_tactic_condition_count));
}

static const char *find_matching_brace(const char *start) {
    int depth = 0;
    for(const char *cursor = start; *cursor != '\0'; cursor++) {
        if(*cursor == '{') {
            depth++;
        } else if(*cursor == '}') {
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

static bool parse_json_string_field(const char *obj_start, const char *obj_end, const char *field,
                                    char *dst, size_t dst_size) {
    if(obj_start == NULL || obj_end == NULL || field == NULL || dst == NULL || dst_size == 0) {
        return false;
    }

    char key[64];
    snprintf(key, sizeof(key), "\"%s\"", field);
    const char *field_key = strstr(obj_start, key);
    if(field_key == NULL || field_key >= obj_end) {
        return false;
    }

    const char *colon = strchr(field_key, ':');
    if(colon == NULL || colon >= obj_end) {
        return false;
    }

    const char *quote_start = strchr(colon, '"');
    if(quote_start == NULL || quote_start >= obj_end) {
        return false;
    }
    quote_start++;

    const char *quote_end = strchr(quote_start, '"');
    if(quote_end == NULL || quote_end > obj_end) {
        return false;
    }

    size_t len = (size_t)(quote_end - quote_start);
    if(len >= dst_size) {
        len = dst_size - 1;
    }
    memcpy(dst, quote_start, len);
    dst[len] = '\0';
    return true;
}

static int parse_json_string_array_field(const char *obj_start, const char *obj_end, const char *field,
                                         char dst[][40], int max_items) {
    if(obj_start == NULL || obj_end == NULL || field == NULL || dst == NULL || max_items <= 0) {
        return 0;
    }

    char key[64];
    snprintf(key, sizeof(key), "\"%s\"", field);
    const char *field_key = strstr(obj_start, key);
    if(field_key == NULL || field_key >= obj_end) {
        return 0;
    }

    const char *colon = strchr(field_key, ':');
    if(colon == NULL || colon >= obj_end) {
        return 0;
    }

    const char *arr_start = strchr(colon, '[');
    if(arr_start == NULL || arr_start >= obj_end) {
        return 0;
    }
    const char *arr_end = strchr(arr_start, ']');
    if(arr_end == NULL || arr_end > obj_end) {
        return 0;
    }

    int count = 0;
    const char *cursor = arr_start;
    while(count < max_items && cursor < arr_end) {
        const char *q1 = strchr(cursor, '"');
        if(q1 == NULL || q1 >= arr_end) {
            break;
        }
        const char *q2 = strchr(q1 + 1, '"');
        if(q2 == NULL || q2 > arr_end) {
            break;
        }

        size_t len = (size_t)(q2 - (q1 + 1));
        if(len >= sizeof(dst[count])) {
            len = sizeof(dst[count]) - 1;
        }
        memcpy(dst[count], q1 + 1, len);
        dst[count][len] = '\0';

        count++;
        cursor = q2 + 1;
    }

    return count;
}

static bool load_tactic_registry_from_config(void) {
    if(g_tactic_config_attempted) {
        return g_tactic_config_loaded;
    }
    g_tactic_config_attempted = true;

    path tactic_cfg = get_resource_filename("ai_config/tactics.json");
    size_t file_size = 0;
    if(!path_filesize(&tactic_cfg, &file_size) || file_size == 0) {
        return false;
    }

    char *json = omf_calloc(file_size + 1, sizeof(char));
    if(!path_read_file(&tactic_cfg, json, file_size)) {
        omf_free(json);
        return false;
    }
    json[file_size] = '\0';

    const char *tactics_key = strstr(json, "\"tactics\"");
    if(tactics_key == NULL) {
        omf_free(json);
        return false;
    }

    // Preserve legacy behavior for partial configs: unspecified tactics remain enabled.
    for(int i = TACTIC_ESCAPE; i <= TACTIC_COUNTER; i++) {
        g_tactic_enabled[i] = true;
    }

    int enabled_count = 0;
    const char *cursor = tactics_key;
    while((cursor = strstr(cursor, "\"id\"")) != NULL) {
        const char *colon = strchr(cursor, ':');
        if(colon == NULL) {
            break;
        }

        errno = 0;
        char *endptr = NULL;
        long parsed_id = strtol(colon + 1, &endptr, 10);
        if(colon + 1 == endptr || errno != 0 || endptr == NULL) {
            cursor += 4;
            continue;
        }

        const char *obj_start = cursor;
        while(obj_start > tactics_key && *obj_start != '{') {
            obj_start--;
        }
        if(*obj_start != '{') {
            cursor = endptr;
            continue;
        }

        const char *obj_end = find_matching_brace(obj_start);
        if(obj_end == NULL) {
            break;
        }

        bool enabled = true;
        const char *enabled_key = strstr(obj_start, "\"enabled\"");
        if(enabled_key != NULL && enabled_key < obj_end) {
            const char *enabled_colon = strchr(enabled_key, ':');
            if(enabled_colon != NULL && enabled_colon < obj_end) {
                const char *v = enabled_colon + 1;
                while(v < obj_end && (*v == ' ' || *v == '\t' || *v == '\n' || *v == '\r')) {
                    v++;
                }
                if(v < obj_end && strncmp(v, "false", 5) == 0) {
                    enabled = false;
                }
            }
        }

        if(parsed_id >= TACTIC_ESCAPE && parsed_id <= TACTIC_COUNTER) {
            g_tactic_enabled[parsed_id] = enabled;
            parse_json_string_field(obj_start, obj_end, "move_type", g_tactic_move_type[parsed_id],
                                    sizeof(g_tactic_move_type[parsed_id]));
            parse_json_string_field(obj_start, obj_end, "attack_type", g_tactic_attack_type[parsed_id],
                                    sizeof(g_tactic_attack_type[parsed_id]));
            g_tactic_condition_count[parsed_id] = (uint8_t)parse_json_string_array_field(
                obj_start, obj_end, "conditions", g_tactic_conditions[parsed_id], 8);
            warn_unsupported_tactic_tokens((int)parsed_id);
            enabled_count++;
        }

        cursor = obj_end + 1;
    }

    omf_free(json);
    g_tactic_config_loaded = enabled_count > 0;
    if(g_tactic_config_loaded) {
        log_info("Loaded AI tactic registry from tactics.json (%d entries)", enabled_count);
    }
    return g_tactic_config_loaded;
}

bool ai_tactic_is_enabled(int tactic_type) {
    if(tactic_type < TACTIC_ESCAPE || tactic_type > TACTIC_COUNTER) {
        return false;
    }

    if(!load_tactic_registry_from_config()) {
        return true;
    }

    return g_tactic_enabled[tactic_type];
}

int ai_tactic_first_enabled(const int *tactics, size_t n_tactics) {
    if(tactics == NULL || n_tactics == 0) {
        return 0;
    }

    for(size_t i = 0; i < n_tactics; i++) {
        if(ai_tactic_is_enabled(tactics[i])) {
            return tactics[i];
        }
    }

    return 0;
}

const char *ai_tactic_config_move_type(int tactic_type) {
    if(tactic_type < TACTIC_ESCAPE || tactic_type > TACTIC_COUNTER) {
        return "";
    }
    load_tactic_registry_from_config();
    return g_tactic_move_type[tactic_type];
}

const char *ai_tactic_config_attack_type(int tactic_type) {
    if(tactic_type < TACTIC_ESCAPE || tactic_type > TACTIC_COUNTER) {
        return "";
    }
    load_tactic_registry_from_config();
    return g_tactic_attack_type[tactic_type];
}

static bool move_type_token_supported(const char *move_cfg) {
    if(move_cfg == NULL || move_cfg[0] == '\0') {
        return false;
    }

    return strcmp(move_cfg, "NONE") == 0 || strcmp(move_cfg, "MOVE_HIGH_JUMP") == 0 ||
           strcmp(move_cfg, "MOVE_AVOID_IF_CRAMPED") == 0 || strcmp(move_cfg, "MOVE_BLOCK_IF_NOT_CRAMPED") == 0 ||
           strcmp(move_cfg, "MOVE_AVOID") == 0 || strcmp(move_cfg, "MOVE_BLOCK") == 0 ||
           strcmp(move_cfg, "MOVE_CLOSE") == 0;
}

static bool attack_type_token_supported(const char *attack_cfg) {
    if(attack_cfg == NULL || attack_cfg[0] == '\0') {
        return false;
    }

    return strcmp(attack_cfg, "NONE") == 0 || strcmp(attack_cfg, "ATTACK_GRAB") == 0 ||
           strcmp(attack_cfg, "ATTACK_TRIP") == 0 || strcmp(attack_cfg, "ATTACK_LIGHT") == 0 ||
           strcmp(attack_cfg, "ATTACK_RANDOM") == 0 || strcmp(attack_cfg, "ATTACK_RANGED") == 0 ||
           strcmp(attack_cfg, "ATTACK_CHARGE") == 0 || strcmp(attack_cfg, "ATTACK_ID_OR_LIGHT") == 0 ||
           strcmp(attack_cfg, "ATTACK_JUMP_OR_NONE") == 0 || strcmp(attack_cfg, "ATTACK_PUSH_OR_HEAVY") == 0 ||
           strcmp(attack_cfg, "ATTACK_TRIP_OR_HEAVY") == 0;
}

static bool condition_token_supported(const char *condition) {
    if(condition == NULL || condition[0] == '\0') {
        return false;
    }

    return strcmp(condition, "pref_hyper") == 0 || strcmp(condition, "pref_def") == 0 ||
           strcmp(condition, "pref_jump") == 0 || strcmp(condition, "pref_sniper") == 0 ||
           strcmp(condition, "pref_normal") == 0 || strcmp(condition, "not_thrown_too_much") == 0 ||
           strcmp(condition, "not_shot_too_much") == 0 || strcmp(condition, "has_projectiles") == 0 ||
           strcmp(condition, "has_charge") == 0 || strcmp(condition, "has_push") == 0 ||
           strcmp(condition, "enemy_cramped") == 0 || strcmp(condition, "enemy_not_cramped") == 0;
}

static void warn_unsupported_tactic_tokens(int tactic_type) {
    const char *name = tactic_name_from_id(tactic_type);

    const char *move_token = g_tactic_move_type[tactic_type];
    if(move_token[0] != '\0' && !move_type_token_supported(move_token)) {
        log_warn("Unsupported move_type '%s' for tactic %s (%d); using legacy fallback behavior", move_token, name,
                 tactic_type);
    }

    const char *attack_token = g_tactic_attack_type[tactic_type];
    if(attack_token[0] != '\0' && !attack_type_token_supported(attack_token)) {
        log_warn("Unsupported attack_type '%s' for tactic %s (%d); using legacy fallback behavior", attack_token,
                 name, tactic_type);
    }

    int n_conditions = g_tactic_condition_count[tactic_type];
    for(int i = 0; i < n_conditions; i++) {
        const char *cond = g_tactic_conditions[tactic_type][i];
        if(cond[0] != '\0' && !condition_token_supported(cond)) {
            log_warn("Unsupported condition '%s' for tactic %s (%d); ignoring condition token", cond, name,
                     tactic_type);
        }
    }
}

bool ai_tactic_config_move_type_supported(int tactic_type) {
    return move_type_token_supported(ai_tactic_config_move_type(tactic_type));
}

bool ai_tactic_config_attack_type_supported(int tactic_type) {
    return attack_type_token_supported(ai_tactic_config_attack_type(tactic_type));
}

int ai_tactic_config_condition_count(int tactic_type) {
    if(tactic_type < TACTIC_ESCAPE || tactic_type > TACTIC_COUNTER) {
        return 0;
    }
    load_tactic_registry_from_config();
    return g_tactic_condition_count[tactic_type];
}

bool ai_tactic_config_has_condition(int tactic_type, const char *condition) {
    if(tactic_type < TACTIC_ESCAPE || tactic_type > TACTIC_COUNTER || condition == NULL || condition[0] == '\0') {
        return false;
    }

    load_tactic_registry_from_config();
    int count = g_tactic_condition_count[tactic_type];
    for(int i = 0; i < count; i++) {
        if(strcmp(g_tactic_conditions[tactic_type][i], condition) == 0) {
            return true;
        }
    }
    return false;
}

bool ai_tactic_config_conditions_supported(int tactic_type) {
    if(tactic_type < TACTIC_ESCAPE || tactic_type > TACTIC_COUNTER) {
        return false;
    }

    load_tactic_registry_from_config();
    int count = g_tactic_condition_count[tactic_type];
    for(int i = 0; i < count; i++) {
        if(!condition_token_supported(g_tactic_conditions[tactic_type][i])) {
            return false;
        }
    }

    return true;
}

bool ai_tactic_config_condition_token_supported(const char *condition) {
    return condition_token_supported(condition);
}

static bool tactic_condition_matches_context(const char *condition, const ai *a, const har *h, int enemy_range) {
    if(condition == NULL || condition[0] == '\0') {
        return true;
    }

    if(strcmp(condition, "pref_hyper") == 0) {
        return roll_pref(a->pilot->att_hyper);
    }
    if(strcmp(condition, "pref_def") == 0) {
        return roll_pref(a->pilot->att_def);
    }
    if(strcmp(condition, "pref_jump") == 0) {
        return roll_pref(a->pilot->att_jump);
    }
    if(strcmp(condition, "pref_sniper") == 0) {
        return roll_pref(a->pilot->att_sniper);
    }
    if(strcmp(condition, "pref_normal") == 0) {
        return roll_pref(a->pilot->att_normal);
    }
    if(strcmp(condition, "not_thrown_too_much") == 0) {
        return a->thrown <= MAX_TIMES_THROWN;
    }
    if(strcmp(condition, "not_shot_too_much") == 0) {
        return a->shot <= MAX_TIMES_SHOT;
    }
    if(strcmp(condition, "has_projectiles") == 0) {
        return har_has_projectiles(h->id);
    }
    if(strcmp(condition, "has_charge") == 0) {
        return har_has_charge(h->id);
    }
    if(strcmp(condition, "has_push") == 0) {
        return har_has_push(h->id);
    }
    if(strcmp(condition, "enemy_cramped") == 0) {
        return enemy_range == RANGE_CRAMPED;
    }
    if(strcmp(condition, "enemy_not_cramped") == 0) {
        return enemy_range > RANGE_CRAMPED;
    }

    // Unknown condition tokens are ignored for behavioral safety.
    return true;
}

static bool tactic_conditions_match_context(int tactic_type, const ai *a, const har *h, int enemy_range) {
    if(tactic_type < TACTIC_ESCAPE || tactic_type > TACTIC_COUNTER) {
        return false;
    }

    int count = ai_tactic_config_condition_count(tactic_type);
    for(int i = 0; i < count; i++) {
        if(!tactic_condition_matches_context(g_tactic_conditions[tactic_type][i], a, h, enemy_range)) {
            return false;
        }
    }

    return true;
}

bool ai_tactic_conditions_match_context(int tactic_type, int har_id, int enemy_range, int thrown, int shot,
                                        const sd_pilot *pilot) {
    if(pilot == NULL || tactic_type < TACTIC_ESCAPE || tactic_type > TACTIC_COUNTER) {
        return false;
    }

    ai fake_ai;
    memset(&fake_ai, 0, sizeof(fake_ai));
    fake_ai.pilot = (sd_pilot *)pilot;
    fake_ai.thrown = thrown;
    fake_ai.shot = shot;

    har fake_har;
    memset(&fake_har, 0, sizeof(fake_har));
    fake_har.id = har_id;

    return tactic_conditions_match_context(tactic_type, &fake_ai, &fake_har, enemy_range);
}

static bool apply_configured_move_type(ai *a, const har *h, int tactic_type, int enemy_range, bool enemy_close,
                                       bool wall_close, const char *move_cfg, bool *do_charge) {
    if(!move_type_token_supported(move_cfg)) {
        return false;
    }

    if(strcmp(move_cfg, "NONE") == 0) {
        a->tactic->move_type = 0;
        return true;
    }

    if(strcmp(move_cfg, "MOVE_HIGH_JUMP") == 0) {
        a->tactic->move_type = MOVE_HIGH_JUMP;
        return true;
    }

    if(strcmp(move_cfg, "MOVE_AVOID_IF_CRAMPED") == 0) {
        a->tactic->move_type = (enemy_range == RANGE_CRAMPED && !wall_close) ? MOVE_AVOID : 0;
        return true;
    }

    if(strcmp(move_cfg, "MOVE_BLOCK_IF_NOT_CRAMPED") == 0) {
        a->tactic->move_type = enemy_range > RANGE_CRAMPED ? MOVE_BLOCK : 0;
        return true;
    }

    if(strcmp(move_cfg, "MOVE_AVOID") == 0) {
        // Keep legacy wall-aware escape behavior while honoring config token.
        if(tactic_type == TACTIC_ESCAPE && wall_close) {
            a->tactic->move_type = MOVE_JUMP;
        } else {
            a->tactic->move_type = MOVE_AVOID;
        }
        return true;
    }

    if(strcmp(move_cfg, "MOVE_BLOCK") == 0) {
        // TURTLE is context-sensitive in legacy behavior; preserve it.
        if(tactic_type == TACTIC_TURTLE) {
            if(enemy_range == RANGE_CRAMPED) {
                a->tactic->move_type = wall_close ? MOVE_JUMP : MOVE_AVOID;
            } else {
                a->tactic->move_type = MOVE_BLOCK;
            }
        } else {
            a->tactic->move_type = MOVE_BLOCK;
        }
        return true;
    }

    if(strcmp(move_cfg, "MOVE_CLOSE") == 0) {
        if(enemy_close) {
            a->tactic->move_type = 0;
            return true;
        }

        if((tactic_type == TACTIC_CLOSE || (tactic_type == TACTIC_QUICK && roll_chance(3))) && smart_usually(a) &&
           har_has_charge(h->id)) {
            a->tactic->move_type = 0;
            *do_charge = true;
            return true;
        }

        if(smart_usually(a) && roll_pref(a->pilot->pref_jump)) {
            a->tactic->move_type = MOVE_JUMP;
        } else {
            a->tactic->move_type = MOVE_CLOSE;
        }
        return true;
    }

    return false;
}

static bool apply_configured_attack_type(ai *a, const har *h, int tactic_type, int enemy_range,
                                         const char *attack_cfg) {
    if(!attack_type_token_supported(attack_cfg)) {
        return false;
    }

    if(strcmp(attack_cfg, "NONE") == 0) {
        a->tactic->attack_type = 0;
        a->tactic->attack_id = 0;
        return true;
    }

    if(strcmp(attack_cfg, "ATTACK_GRAB") == 0) {
        a->tactic->attack_type = ATTACK_GRAB;
        a->tactic->attack_id = 0;
        return true;
    }

    if(strcmp(attack_cfg, "ATTACK_TRIP") == 0) {
        a->tactic->attack_type = ATTACK_TRIP;
        a->tactic->attack_id = 0;
        if(a->tactic->move_type == MOVE_JUMP) {
            a->tactic->attack_on = HAR_EVENT_LAND;
        }
        return true;
    }

    if(strcmp(attack_cfg, "ATTACK_LIGHT") == 0) {
        a->tactic->attack_type = ATTACK_LIGHT;
        a->tactic->attack_id = 0;
        return true;
    }

    if(strcmp(attack_cfg, "ATTACK_RANDOM") == 0) {
        a->tactic->attack_type = ATTACK_RANDOM;
        a->tactic->attack_id = 0;
        return true;
    }

    if(strcmp(attack_cfg, "ATTACK_RANGED") == 0) {
        a->tactic->attack_type = ATTACK_RANGED;
        a->tactic->attack_id = 0;
        return true;
    }

    if(strcmp(attack_cfg, "ATTACK_CHARGE") == 0) {
        a->tactic->attack_type = ATTACK_CHARGE;
        a->tactic->attack_id = 0;
        return true;
    }

    if(strcmp(attack_cfg, "ATTACK_ID_OR_LIGHT") == 0) {
        if(a->last_move_id > 0) {
            a->tactic->attack_type = ATTACK_ID;
            a->tactic->attack_id = a->last_move_id;
        } else {
            a->tactic->attack_type = ATTACK_LIGHT;
            a->tactic->attack_id = 0;
        }
        return true;
    }

    if(strcmp(attack_cfg, "ATTACK_JUMP_OR_NONE") == 0) {
        a->tactic->attack_type = smart_usually(a) ? ATTACK_JUMP : 0;
        a->tactic->attack_id = 0;
        return true;
    }

    if(strcmp(attack_cfg, "ATTACK_PUSH_OR_HEAVY") == 0) {
        if(har_has_push(h->id)) {
            a->tactic->attack_type = ATTACK_PUSH;
        } else {
            a->tactic->attack_type = ATTACK_HEAVY;
        }
        a->tactic->attack_id = 0;
        return true;
    }

    if(strcmp(attack_cfg, "ATTACK_TRIP_OR_HEAVY") == 0) {
        a->tactic->attack_type = roll_chance(3) ? ATTACK_TRIP : ATTACK_HEAVY;
        a->tactic->attack_id = 0;
        if(tactic_type == TACTIC_COUNTER && enemy_range > RANGE_CRAMPED) {
            a->tactic->attack_on = HAR_EVENT_BLOCK;
        }
        return true;
    }

    return false;
}

bool ai_tactic_likes_it(const controller *ctrl, int tactic_type) {
    if(!ai_tactic_is_enabled(tactic_type)) {
        return false;
    }

    ai *a = ctrl->data;

    object *o = game_state_find_object(ctrl->gs, ctrl->har_obj_id);
    har *h = object_get_userdata(o);
    sd_pilot *pilot = a->pilot;

    if((a->tactic->last_tactic == tactic_type && roll_chance(2)) || h->state == STATE_JUMPING) {
        return false;
    }

    bool enemy_close = h->close;
    int enemy_range = get_enemy_range(ctrl);
    bool wall_close = h->is_wallhugging;

    if(!tactic_conditions_match_context(tactic_type, a, h, enemy_range)) {
        return false;
    }

    switch(tactic_type) {
        case TACTIC_SHOOT: {
            // Avoid double-gating when an equivalent condition token is configured.
            bool needs_projectiles_check = !ai_tactic_config_has_condition(TACTIC_SHOOT, "has_projectiles");
            bool needs_sniper_pref_check = !ai_tactic_config_has_condition(TACTIC_SHOOT, "pref_sniper");
            bool needs_enemy_not_cramped_check =
                !ai_tactic_config_has_condition(TACTIC_SHOOT, "enemy_not_cramped");

            if((!needs_projectiles_check || har_has_projectiles(h->id)) &&
               (!needs_sniper_pref_check || roll_pref(pilot->att_sniper)) &&
               (!needs_enemy_not_cramped_check || enemy_range > RANGE_CRAMPED) &&
               (h->id != HAR_SHREDDER || ((enemy_range <= RANGE_MID && smart_usually(a)) ||
                                          dumb_sometimes(a)) // shredder prefers to be close-mid range
                )) {
                return true;
            }
            break;
        }
        case TACTIC_CLOSE: {
            // Avoid double-gating when an equivalent condition token is configured.
            bool needs_close_enemy_not_cramped_check =
                !ai_tactic_config_has_condition(TACTIC_CLOSE, "enemy_not_cramped");
            bool needs_has_charge_check = !ai_tactic_config_has_condition(TACTIC_CLOSE, "has_charge");
            bool needs_pref_hyper_check = !ai_tactic_config_has_condition(TACTIC_CLOSE, "pref_hyper");

            if((!needs_close_enemy_not_cramped_check || enemy_range > RANGE_CRAMPED) &&
               (!needs_has_charge_check || har_has_charge(h->id) || roll_chance(4)) &&
               (!needs_pref_hyper_check || roll_pref(pilot->att_hyper))) {
                return true;
            }
            break;
        }
        case TACTIC_QUICK: {
            bool needs_quick_enemy_not_cramped_check = !ai_tactic_config_has_condition(TACTIC_QUICK, "enemy_not_cramped");
            bool needs_quick_pref_sniper_check = !ai_tactic_config_has_condition(TACTIC_QUICK, "pref_sniper");
            bool needs_quick_pref_hyper_check = !ai_tactic_config_has_condition(TACTIC_QUICK, "pref_hyper");
            bool needs_quick_pref_normal_check = !ai_tactic_config_has_condition(TACTIC_QUICK, "pref_normal");

            if((!needs_quick_enemy_not_cramped_check || enemy_range > RANGE_CRAMPED) && enemy_range < RANGE_FAR &&
               (((!needs_quick_pref_sniper_check || roll_pref(pilot->att_sniper)) && roll_chance(3)) ||
                ((!needs_quick_pref_hyper_check || roll_pref(pilot->att_hyper)) && roll_chance(6)) ||
                ((!needs_quick_pref_normal_check || roll_pref(pilot->att_normal)) && roll_chance(8)))) {
                return true;
            }
            break;
        }
        case TACTIC_GRAB: {
            bool needs_grab_not_thrown_check = !ai_tactic_config_has_condition(TACTIC_GRAB, "not_thrown_too_much");
            bool needs_grab_pref_hyper_check = !ai_tactic_config_has_condition(TACTIC_GRAB, "pref_hyper");

            if((!needs_grab_not_thrown_check || a->thrown <= MAX_TIMES_THROWN || roll_chance(2)) &&
               (((!needs_grab_pref_hyper_check || roll_pref(pilot->att_hyper)) && roll_chance(3)) ||
                ((h->id == HAR_FLAIL || h->id == HAR_THORN) && roll_chance(3)))) {
                return true;
            }
            break;
        }
        case TACTIC_TURTLE: {
            bool needs_turtle_not_thrown_check = !ai_tactic_config_has_condition(TACTIC_TURTLE, "not_thrown_too_much");
            bool needs_turtle_pref_def_check = !ai_tactic_config_has_condition(TACTIC_TURTLE, "pref_def");

            if((!needs_turtle_not_thrown_check || a->thrown <= MAX_TIMES_THROWN) &&
               (!needs_turtle_pref_def_check || roll_pref(pilot->att_def)) && roll_chance(3)) {
                return true;
            }
            break;
        }
        case TACTIC_COUNTER: {
            bool needs_counter_pref_def_check = !ai_tactic_config_has_condition(TACTIC_COUNTER, "pref_def");

            if(a->thrown < MAX_TIMES_THROWN && (!needs_counter_pref_def_check || roll_pref(pilot->att_def)) &&
               roll_chance(3)) {
                return true;
            }
            break;
        }
        case TACTIC_ESCAPE: {
            bool needs_escape_pref_jump_check = !ai_tactic_config_has_condition(TACTIC_ESCAPE, "pref_jump");
            bool needs_escape_pref_def_check = !ai_tactic_config_has_condition(TACTIC_ESCAPE, "pref_def");

            if(((!needs_escape_pref_jump_check || roll_pref(pilot->att_jump)) && roll_chance(3)) ||
               ((!needs_escape_pref_def_check || roll_pref(pilot->att_def)) && roll_chance(5))) {
                return true;
            }
            break;
        }
        case TACTIC_FLY: {
            bool needs_fly_pref_jump_check = !ai_tactic_config_has_condition(TACTIC_FLY, "pref_jump");

            if(((!needs_fly_pref_jump_check || roll_pref(a->pilot->att_jump)) ||
                (a->shot > MAX_TIMES_SHOT && learning_moment(a)) ||
                (h->id == HAR_GARGOYLE || h->id == HAR_PYROS)) &&
               ((wall_close && roll_chance(2)) || roll_chance(4))) {
                return true;
            }
            break;
        }
        case TACTIC_PUSH: {
            bool needs_push_has_push_check = !ai_tactic_config_has_condition(TACTIC_PUSH, "has_push");
            bool needs_push_pref_hyper_check = !ai_tactic_config_has_condition(TACTIC_PUSH, "pref_hyper");
            bool needs_push_pref_def_check = !ai_tactic_config_has_condition(TACTIC_PUSH, "pref_def");

            if((enemy_range <= RANGE_CLOSE || ((h->id == HAR_THORN || h->id == HAR_KATANA) && enemy_range <= RANGE_MID)) &&
               (((!needs_push_has_push_check || har_has_push(h->id)) && smart_usually(a)) &&
                (((!needs_push_pref_hyper_check || roll_pref(pilot->att_hyper)) && roll_chance(2)) ||
                 ((!needs_push_pref_def_check || roll_pref(pilot->att_def)) && roll_chance(4)) ||
                 (wall_close && roll_chance(5))))) {
                return true;
            }
            break;
        }
        case TACTIC_TRIP: {
            bool needs_trip_pref_def_check = !ai_tactic_config_has_condition(TACTIC_TRIP, "pref_def");
            bool needs_trip_pref_sniper_check = !ai_tactic_config_has_condition(TACTIC_TRIP, "pref_sniper");

            if(enemy_range <= RANGE_MID &&
               (((!needs_trip_pref_def_check || roll_pref(pilot->att_def)) && roll_chance(4)) ||
                ((!needs_trip_pref_sniper_check || roll_pref(pilot->att_sniper)) && roll_chance(6)))) {
                return true;
            }
            break;
        }
        case TACTIC_SPAM: {
            bool needs_spam_pref_normal_check = !ai_tactic_config_has_condition(TACTIC_SPAM, "pref_normal");

            if((enemy_close || dumb_usually(a)) && (wall_close || roll_chance(6)) &&
               (!needs_spam_pref_normal_check || roll_pref(pilot->att_normal))) {
                return true;
            }
            break;
        }
    }

    return false;
}

void ai_tactic_queue(controller *ctrl, int tactic_type) {
    if(!ai_tactic_is_enabled(tactic_type)) {
        return;
    }

    ai *a = ctrl->data;
    object *o = game_state_find_object(ctrl->gs, ctrl->har_obj_id);
    har *h = object_get_userdata(o);

    a->tactic->last_tactic = a->tactic->tactic_type > 0 ? a->tactic->tactic_type : 0;
    a->tactic->tactic_type = tactic_type;

    switch(tactic_type) {
        case TACTIC_GRAB:
            log_debug("HAR %d queued tactic: GRAB", h->id);
            break;
        case TACTIC_TRIP:
            log_debug("HAR %d queued tactic: TRIP", h->id);
            break;
        case TACTIC_QUICK:
            log_debug("HAR %d queued tactic: QUICK", h->id);
            break;
        case TACTIC_CLOSE:
            log_debug("HAR %d queued tactic: CLOSE", h->id);
            break;
        case TACTIC_FLY:
            log_debug("HAR %d queued tactic: FLY", h->id);
            break;
        case TACTIC_SHOOT:
            log_debug("HAR %d queued tactic: SHOOT", h->id);
            break;
        case TACTIC_PUSH:
            log_debug("HAR %d queued tactic: PUSH", h->id);
            break;
        case TACTIC_SPAM:
            log_debug("HAR %d queued tactic: SPAM", h->id);
            break;
        case TACTIC_ESCAPE:
            log_debug("HAR %d queued tactic: ESCAPE", h->id);
            break;
        case TACTIC_TURTLE:
            log_debug("HAR %d queued tactic: TURTLE", h->id);
            break;
        case TACTIC_COUNTER:
            log_debug("HAR %d queued tactic: COUNTER", h->id);
            break;
    }

    bool enemy_close = h->close;
    bool wall_close = h->is_wallhugging;
    int enemy_range = get_enemy_range(ctrl);

    bool do_charge = false;
    const char *move_cfg = ai_tactic_config_move_type(tactic_type);
    const char *attack_cfg = ai_tactic_config_attack_type(tactic_type);

    if(!apply_configured_move_type(a, h, tactic_type, enemy_range, enemy_close, wall_close, move_cfg, &do_charge)) {
        switch(tactic_type) {
            case TACTIC_GRAB:
            case TACTIC_TRIP:
            case TACTIC_QUICK:
            case TACTIC_CLOSE:
                if(enemy_close) {
                    a->tactic->move_type = 0;
                } else if((tactic_type == TACTIC_CLOSE || (tactic_type == TACTIC_QUICK && roll_chance(3))) &&
                          smart_usually(a) && har_has_charge(h->id)) {
                    a->tactic->move_type = 0;
                    do_charge = true;
                } else if(smart_usually(a) && roll_pref(a->pilot->pref_jump)) {
                    a->tactic->move_type = MOVE_JUMP;
                } else {
                    a->tactic->move_type = MOVE_CLOSE;
                }
                break;
            case TACTIC_FLY:
                a->tactic->move_type = MOVE_HIGH_JUMP;
                break;
            case TACTIC_SHOOT:
                a->tactic->move_type = (enemy_range == RANGE_CRAMPED && !wall_close) ? MOVE_AVOID : 0;
                break;
            case TACTIC_PUSH:
            case TACTIC_SPAM:
                a->tactic->move_type = 0;
                break;
            case TACTIC_ESCAPE:
                a->tactic->move_type = wall_close ? MOVE_JUMP : MOVE_AVOID;
                break;
            case TACTIC_TURTLE:
                if(enemy_range == RANGE_CRAMPED) {
                    a->tactic->move_type = wall_close ? MOVE_JUMP : MOVE_AVOID;
                } else {
                    a->tactic->move_type = MOVE_BLOCK;
                }
                break;
            case TACTIC_COUNTER:
                a->tactic->move_type = enemy_range > RANGE_CRAMPED ? MOVE_BLOCK : 0;
                break;
        }
    }

    if(a->tactic->move_type > 0) {
        a->tactic->move_timer = TACTIC_MOVE_TIMER_MAX;
    }

    if(do_charge) {
        a->tactic->attack_type = ATTACK_CHARGE;
        a->tactic->attack_id = 0;
    } else {
        if(!apply_configured_attack_type(a, h, tactic_type, enemy_range, attack_cfg)) {
            switch(tactic_type) {
                case TACTIC_GRAB:
                    a->tactic->attack_type = ATTACK_GRAB;
                    a->tactic->attack_id = 0;
                    break;
                case TACTIC_TRIP:
                    a->tactic->attack_type = ATTACK_TRIP;
                    a->tactic->attack_id = 0;
                    if(a->tactic->move_type == MOVE_JUMP) {
                        a->tactic->attack_on = HAR_EVENT_LAND;
                    }
                    break;
                case TACTIC_QUICK:
                    a->tactic->attack_type = ATTACK_LIGHT;
                    a->tactic->attack_id = 0;
                    break;
                case TACTIC_FLY:
                    a->tactic->attack_type = smart_usually(a) ? ATTACK_JUMP : 0;
                    a->tactic->attack_id = 0;
                    break;
                case TACTIC_SHOOT:
                    a->tactic->attack_type = ATTACK_RANGED;
                    a->tactic->attack_id = 0;
                    break;
                case TACTIC_PUSH:
                    if(har_has_push(h->id)) {
                        a->tactic->attack_type = ATTACK_PUSH;
                    } else {
                        a->tactic->attack_type = ATTACK_HEAVY;
                    }
                    a->tactic->attack_id = 0;
                    break;
                case TACTIC_SPAM:
                    if(a->last_move_id > 0) {
                        a->tactic->attack_type = ATTACK_ID;
                        a->tactic->attack_id = a->last_move_id;
                    } else {
                        a->tactic->attack_type = ATTACK_LIGHT;
                        a->tactic->attack_id = 0;
                    }
                    break;
                case TACTIC_COUNTER:
                    a->tactic->attack_type = roll_chance(3) ? ATTACK_TRIP : ATTACK_HEAVY;
                    if(enemy_range > RANGE_CRAMPED) {
                        a->tactic->attack_on = HAR_EVENT_BLOCK;
                    }
                    break;
                case TACTIC_CLOSE:
                    a->tactic->attack_type = ATTACK_RANDOM;
                    a->tactic->attack_id = 0;
                    break;
                case TACTIC_ESCAPE:
                case TACTIC_TURTLE:
                    a->tactic->attack_type = 0;
                    a->tactic->attack_id = 0;
            }
        }
    }

    if(a->tactic->attack_type > 0) {
        if(a->tactic->move_type == MOVE_JUMP || a->tactic->move_type == MOVE_HIGH_JUMP) {
            a->tactic->attack_timer = TACTIC_JUMP_ATTACK_TIMER_MAX;
        } else {
            a->tactic->attack_timer = TACTIC_ATTACK_TIMER_MAX;
        }
    }
}

void ai_tactic_consider_list(controller *ctrl, int tactics[], size_t n_tactics) {
    size_t i = 0;
    while(i < n_tactics) {
        int tactic = ai_tactic_first_enabled(&tactics[i], n_tactics - i);
        if(tactic == 0) {
            return;
        }

        if(ai_tactic_likes_it(ctrl, tactic)) {
            ai_tactic_queue(ctrl, tactic);
            return;
        }

        while(i < n_tactics && tactics[i] != tactic) {
            i++;
        }
        if(i < n_tactics) {
            i++;
        }
    }
}
