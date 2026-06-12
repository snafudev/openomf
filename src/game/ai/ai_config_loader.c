/**
 * AI config loader implementation
 */

#include "game/ai/ai_config_loader.h"

#include "resources/modmanager.h"
#include "resources/resource_files.h"
#include "utils/allocator.h"
#include "utils/path.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum parse_field_result {
    PARSE_FIELD_MISSING = 0,
    PARSE_FIELD_OK,
    PARSE_FIELD_INVALID
};

static const char *find_in_range(const char *begin, const char *end, const char *needle) {
    const char *cursor = begin;
    size_t needle_len = strlen(needle);
    if(needle_len == 0 || begin >= end) {
        return NULL;
    }

    while(cursor < end) {
        const char *match = strstr(cursor, needle);
        if(match == NULL || match >= end) {
            return NULL;
        }
        if(match + needle_len <= end) {
            return match;
        }
        cursor = match + 1;
    }

    return NULL;
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

static enum parse_field_result parse_int_field(const char *obj_start, const char *obj_end, const char *field_name,
                                               int *value) {
    char key[64];
    snprintf(key, sizeof(key), "\"%s\"", field_name);

    const char *field = find_in_range(obj_start, obj_end, key);
    if(field == NULL) {
        return PARSE_FIELD_MISSING;
    }

    const char *colon = strchr(field, ':');
    if(colon == NULL || colon >= obj_end) {
        return PARSE_FIELD_INVALID;
    }

    const char *num = colon + 1;
    while(num < obj_end && isspace((unsigned char)*num)) {
        num++;
    }

    errno = 0;
    char *endptr = NULL;
    long parsed = strtol(num, &endptr, 10);
    if(num == endptr || errno != 0 || endptr == NULL || endptr > obj_end) {
        return PARSE_FIELD_INVALID;
    }

    *value = (int)parsed;
    return PARSE_FIELD_OK;
}

static enum parse_field_result parse_float_field(const char *obj_start, const char *obj_end, const char *field_name,
                                                 float *value) {
    char key[64];
    snprintf(key, sizeof(key), "\"%s\"", field_name);

    const char *field = find_in_range(obj_start, obj_end, key);
    if(field == NULL) {
        return PARSE_FIELD_MISSING;
    }

    const char *colon = strchr(field, ':');
    if(colon == NULL || colon >= obj_end) {
        return PARSE_FIELD_INVALID;
    }

    const char *num = colon + 1;
    while(num < obj_end && isspace((unsigned char)*num)) {
        num++;
    }

    errno = 0;
    char *endptr = NULL;
    float parsed = strtof(num, &endptr);
    if(num == endptr || errno != 0 || endptr == NULL || endptr > obj_end) {
        return PARSE_FIELD_INVALID;
    }

    *value = parsed;
    return PARSE_FIELD_OK;
}

static bool find_pilot_object(const char *json, int pilot_id, const char **obj_start, const char **obj_end) {
    const char *pilots_key = strstr(json, "\"pilots\"");
    if(pilots_key == NULL) {
        return false;
    }

    const char *cursor = pilots_key;
    while((cursor = strstr(cursor, "\"id\"")) != NULL) {
        const char *colon = strchr(cursor, ':');
        if(colon == NULL) {
            return false;
        }

        errno = 0;
        char *id_end = NULL;
        long parsed_id = strtol(colon + 1, &id_end, 10);
        if(colon + 1 == id_end || errno != 0 || id_end == NULL) {
            cursor += 4;
            continue;
        }

        const char *start = cursor;
        while(start > pilots_key && *start != '{') {
            start--;
        }
        if(*start != '{') {
            cursor = id_end;
            continue;
        }

        const char *end = find_matching_brace(start);
        if(end == NULL) {
            return false;
        }

        if((int)parsed_id == pilot_id) {
            *obj_start = start;
            *obj_end = end;
            return true;
        }

        cursor = end + 1;
    }

    return false;
}

static bool apply_pilot_field_int(const char *obj_start, const char *obj_end, const char *field_name,
                                  int *fields_loaded, uint8_t *target_u8, int16_t *target_i16) {
    int value = 0;
    enum parse_field_result result = parse_int_field(obj_start, obj_end, field_name, &value);
    if(result == PARSE_FIELD_INVALID) {
        return false;
    }
    if(result == PARSE_FIELD_OK) {
        (*fields_loaded)++;
        if(target_u8 != NULL) {
            *target_u8 = (uint8_t)value;
        }
        if(target_i16 != NULL) {
            *target_i16 = (int16_t)value;
        }
    }
    return true;
}

static bool apply_pilot_field_float(const char *obj_start, const char *obj_end, const char *field_name,
                                    int *fields_loaded, float *target) {
    float value = 0.0f;
    enum parse_field_result result = parse_float_field(obj_start, obj_end, field_name, &value);
    if(result == PARSE_FIELD_INVALID) {
        return false;
    }
    if(result == PARSE_FIELD_OK) {
        (*fields_loaded)++;
        *target = value;
    }
    return true;
}

static void apply_pilot_overlay_cb(const char *json_buf, void *userdata) {
    sd_pilot *pilot = (sd_pilot *)userdata;
    ai_config_apply_pilot_overlay(pilot, json_buf);
}

bool ai_config_apply_pilot_overlay(sd_pilot *pilot, const char *json_buf) {
    if(pilot == NULL || json_buf == NULL) {
        return false;
    }

    const char *obj_start = NULL;
    const char *obj_end = NULL;
    bool found = find_pilot_object(json_buf, pilot->pilot_id, &obj_start, &obj_end);
    if(!found) {
        return false;
    }

    int fields_loaded = 0;
    bool ok = true;

    ok &= apply_pilot_field_int(obj_start, obj_end, "att_normal", &fields_loaded, &pilot->att_normal, NULL);
    ok &= apply_pilot_field_int(obj_start, obj_end, "att_hyper", &fields_loaded, &pilot->att_hyper, NULL);
    ok &= apply_pilot_field_int(obj_start, obj_end, "att_jump", &fields_loaded, &pilot->att_jump, NULL);
    ok &= apply_pilot_field_int(obj_start, obj_end, "att_def", &fields_loaded, &pilot->att_def, NULL);
    ok &= apply_pilot_field_int(obj_start, obj_end, "att_sniper", &fields_loaded, &pilot->att_sniper, NULL);

    ok &= apply_pilot_field_int(obj_start, obj_end, "ap_throw", &fields_loaded, NULL, &pilot->ap_throw);
    ok &= apply_pilot_field_int(obj_start, obj_end, "ap_special", &fields_loaded, NULL, &pilot->ap_special);
    ok &= apply_pilot_field_int(obj_start, obj_end, "ap_jump", &fields_loaded, NULL, &pilot->ap_jump);
    ok &= apply_pilot_field_int(obj_start, obj_end, "ap_high", &fields_loaded, NULL, &pilot->ap_high);
    ok &= apply_pilot_field_int(obj_start, obj_end, "ap_low", &fields_loaded, NULL, &pilot->ap_low);
    ok &= apply_pilot_field_int(obj_start, obj_end, "ap_middle", &fields_loaded, NULL, &pilot->ap_middle);

    ok &= apply_pilot_field_int(obj_start, obj_end, "pref_jump", &fields_loaded, NULL, &pilot->pref_jump);
    ok &= apply_pilot_field_int(obj_start, obj_end, "pref_fwd", &fields_loaded, NULL, &pilot->pref_fwd);
    ok &= apply_pilot_field_int(obj_start, obj_end, "pref_back", &fields_loaded, NULL, &pilot->pref_back);

    ok &= apply_pilot_field_float(obj_start, obj_end, "learning", &fields_loaded, &pilot->learning);
    ok &= apply_pilot_field_float(obj_start, obj_end, "forget", &fields_loaded, &pilot->forget);

    return ok && fields_loaded > 0;
}

bool ai_config_load_pilot_personality(sd_pilot *pilot) {
    if(pilot == NULL) {
        return false;
    }

    path pilot_cfg = get_resource_filename("ai_config/pilots.json");
    size_t file_size = 0;
    if(!path_filesize(&pilot_cfg, &file_size) || file_size == 0) {
        return false;
    }

    char *json = omf_calloc(file_size + 1, sizeof(char));
    if(!path_read_file(&pilot_cfg, json, file_size)) {
        omf_free(json);
        return false;
    }
    json[file_size] = '\0';

    const char *obj_start = NULL;
    const char *obj_end = NULL;
    bool found = find_pilot_object(json, pilot->pilot_id, &obj_start, &obj_end);
    if(!found) {
        omf_free(json);
        return false;
    }

    int fields_loaded = 0;
    bool ok = true;

    ok &= apply_pilot_field_int(obj_start, obj_end, "att_normal", &fields_loaded, &pilot->att_normal, NULL);
    ok &= apply_pilot_field_int(obj_start, obj_end, "att_hyper", &fields_loaded, &pilot->att_hyper, NULL);
    ok &= apply_pilot_field_int(obj_start, obj_end, "att_jump", &fields_loaded, &pilot->att_jump, NULL);
    ok &= apply_pilot_field_int(obj_start, obj_end, "att_def", &fields_loaded, &pilot->att_def, NULL);
    ok &= apply_pilot_field_int(obj_start, obj_end, "att_sniper", &fields_loaded, &pilot->att_sniper, NULL);

    ok &= apply_pilot_field_int(obj_start, obj_end, "ap_throw", &fields_loaded, NULL, &pilot->ap_throw);
    ok &= apply_pilot_field_int(obj_start, obj_end, "ap_special", &fields_loaded, NULL, &pilot->ap_special);
    ok &= apply_pilot_field_int(obj_start, obj_end, "ap_jump", &fields_loaded, NULL, &pilot->ap_jump);
    ok &= apply_pilot_field_int(obj_start, obj_end, "ap_high", &fields_loaded, NULL, &pilot->ap_high);
    ok &= apply_pilot_field_int(obj_start, obj_end, "ap_low", &fields_loaded, NULL, &pilot->ap_low);
    ok &= apply_pilot_field_int(obj_start, obj_end, "ap_middle", &fields_loaded, NULL, &pilot->ap_middle);

    ok &= apply_pilot_field_int(obj_start, obj_end, "pref_jump", &fields_loaded, NULL, &pilot->pref_jump);
    ok &= apply_pilot_field_int(obj_start, obj_end, "pref_fwd", &fields_loaded, NULL, &pilot->pref_fwd);
    ok &= apply_pilot_field_int(obj_start, obj_end, "pref_back", &fields_loaded, NULL, &pilot->pref_back);

    ok &= apply_pilot_field_float(obj_start, obj_end, "learning", &fields_loaded, &pilot->learning);
    ok &= apply_pilot_field_float(obj_start, obj_end, "forget", &fields_loaded, &pilot->forget);

    omf_free(json);

    if(ok && fields_loaded > 0) {
        modmanager_apply_json_overlays("ai_config/pilots.json", apply_pilot_overlay_cb, pilot);
        return true;
    }
    return false;
}
