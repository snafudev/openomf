/**
 * AI skills config loader implementation
 */

#include "game/ai/ai_skills_config_loader.h"

#include "game/ai/ai_utils.h"
#include "game/common_defines.h"
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

static uint8_t json_array_entry_count(const char *json, const char *field_name) {
    char key[64];
    snprintf(key, sizeof(key), "\"%s\"", field_name);

    const char *field = strstr(json, key);
    if(field == NULL) {
        return 0;
    }

    const char *colon = strchr(field, ':');
    if(colon == NULL) {
        return 0;
    }

    const char *arr_start = strchr(colon, '[');
    if(arr_start == NULL) {
        return 0;
    }

    const char *arr_end = find_matching_bracket(arr_start);
    if(arr_end == NULL) {
        return 0;
    }

    uint8_t count = 0;
    int depth = 0;
    bool in_string = false;
    bool escaped = false;

    for(const char *cursor = arr_start; cursor < arr_end; cursor++) {
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
        } else if(c == '{' && depth == 1) {
            count++;
        }
    }

    return count;
}

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
    snprintf(rel_path, sizeof(rel_path), "ai_config/characters/%s.json", name);

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
    g_configs[har_id].charge_move_count = json_array_entry_count(json, "charge_moves");
    g_configs[har_id].push_move_count = json_array_entry_count(json, "push_moves");
    g_configs[har_id].projectile_move_count = json_array_entry_count(json, "projectile_moves");
    g_configs[har_id].has_charge_moves = g_configs[har_id].charge_move_count > 0;
    g_configs[har_id].has_push_moves = g_configs[har_id].push_move_count > 0;
    g_configs[har_id].has_projectile_moves = g_configs[har_id].projectile_move_count > 0;

    omf_free(json);
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
