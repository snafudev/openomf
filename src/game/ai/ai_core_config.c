/**
 * AI Core Configuration Loader Implementation
 */

#include "game/ai/ai_core_config.h"

#include "resources/resource_files.h"
#include "utils/allocator.h"
#include "utils/path.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static ai_core_config *g_ai_core_config = NULL;

static const char *ai_difficulty_filename_for_level(int difficulty) {
    switch(difficulty) {
        case 0:
            return "punching_bag.ini";
        case 1:
            return "rookie.ini";
        case 2:
            return "veteran.ini";
        case 3:
            return "world_class.ini";
        case 4:
            return "champion.ini";
        case 5:
            return "deadly.ini";
        case 6:
            return "ultimate.ini";
        default:
            return NULL;
    }
}

/**
 * \brief Trim whitespace from a string
 */
static char *trim_string(char *str) {
    while(*str && isspace((unsigned char)*str)) {
        str++;
    }
    char *end = str + strlen(str) - 1;
    while(end >= str && isspace((unsigned char)*end)) {
        *end-- = '\0';
    }
    return str;
}

/**
 * \brief Parse integer value from INI file line
 *
 * \param line Line from INI file (e.g., "key = 42")
 * \param key Key to search for
 * \param value Output parameter for parsed value
 * \return true if key was found and parsed, false otherwise
 */
static bool parse_int_line(const char *line, const char *key, int *value) {
    if(strncmp(line, key, strlen(key)) != 0) {
        return false;
    }

    const char *eq = strchr(line, '=');
    if(eq == NULL) {
        return false;
    }

    *value = atoi(eq + 1);
    return true;
}

/**
 * \brief Load AI core configuration from INI file
 *
 * \param config Pointer to config structure to populate
 * \return true if successfully loaded, false otherwise
 */
static bool load_ai_core_config(ai_core_config *config) {
    if(config == NULL) {
        return false;
    }

    // Set defaults
    config->base_act_chance = 5;
    config->base_fwd_jump_chance = 5;
    config->base_back_jump_chance = 5;
    config->base_still_jump_chance = 40;
    config->random_attack_chance = 10;
    config->base_act_timer = 28;
    config->block_chance = 15;
    config->aggressive_tactics = 0;
    config->jump_frequency_mult = 100;

    path cfg_path = get_resource_filename("ai_config/ai_core.ini");
    size_t file_size = 0;

    if(!path_filesize(&cfg_path, &file_size) || file_size == 0) {
        // Config file not found, using defaults
        return false;
    }

    char *buf = omf_calloc(file_size + 1, sizeof(char));
    if(!path_read_file(&cfg_path, buf, file_size)) {
        omf_free(buf);
        return false;
    }
    buf[file_size] = '\0';

    // Parse line by line
    char *line = buf;
    char *end = buf + file_size;

    while(line < end) {
        char *newline = strchr(line, '\n');
        if(newline == NULL) {
            newline = end;
        }

        // Extract the line
        size_t line_len = newline - line;
        char *line_copy = omf_calloc(line_len + 1, sizeof(char));
        strncpy(line_copy, line, line_len);
        line_copy[line_len] = '\0';

        // Trim and skip empty lines and comments
        char *trimmed = trim_string(line_copy);
        if(trimmed[0] != '\0' && trimmed[0] != '#') {
            // Try to parse each configuration key
            parse_int_line(trimmed, "base_act_chance", &config->base_act_chance);
            parse_int_line(trimmed, "base_fwd_jump_chance", &config->base_fwd_jump_chance);
            parse_int_line(trimmed, "base_back_jump_chance", &config->base_back_jump_chance);
            parse_int_line(trimmed, "base_still_jump_chance", &config->base_still_jump_chance);
            parse_int_line(trimmed, "random_attack_chance", &config->random_attack_chance);
            parse_int_line(trimmed, "base_act_timer", &config->base_act_timer);
            parse_int_line(trimmed, "block_chance", &config->block_chance);
            parse_int_line(trimmed, "aggressive_tactics", &config->aggressive_tactics);
            parse_int_line(trimmed, "jump_frequency_mult", &config->jump_frequency_mult);
        }

        omf_free(line_copy);
        line = newline + 1;
    }

    omf_free(buf);

    return true;
}

static bool load_ai_difficulty_config(ai_core_config *config, int difficulty) {
    if(config == NULL) {
        return false;
    }

    const char *difficulty_file = ai_difficulty_filename_for_level(difficulty);
    if(difficulty_file == NULL) {
        return true;
    }

    char path_buffer[128];
    snprintf(path_buffer, sizeof(path_buffer), "ai_config/ai_difficulty/%s", difficulty_file);
    path cfg_path = get_resource_filename(path_buffer);
    size_t file_size = 0;

    if(!path_filesize(&cfg_path, &file_size) || file_size == 0) {
        return true;
    }

    char *buf = omf_calloc(file_size + 1, sizeof(char));
    if(!path_read_file(&cfg_path, buf, file_size)) {
        omf_free(buf);
        return false;
    }
    buf[file_size] = '\0';

    char *line = buf;
    char *end = buf + file_size;
    while(line < end) {
        char *newline = strchr(line, '\n');
        if(newline == NULL) {
            newline = end;
        }

        size_t line_len = newline - line;
        char *line_copy = omf_calloc(line_len + 1, sizeof(char));
        strncpy(line_copy, line, line_len);
        line_copy[line_len] = '\0';

        char *trimmed = trim_string(line_copy);
        if(trimmed[0] != '\0' && trimmed[0] != '#') {
            parse_int_line(trimmed, "base_act_chance", &config->base_act_chance);
            parse_int_line(trimmed, "base_fwd_jump_chance", &config->base_fwd_jump_chance);
            parse_int_line(trimmed, "base_back_jump_chance", &config->base_back_jump_chance);
            parse_int_line(trimmed, "base_still_jump_chance", &config->base_still_jump_chance);
            parse_int_line(trimmed, "random_attack_chance", &config->random_attack_chance);
            parse_int_line(trimmed, "base_act_timer", &config->base_act_timer);
            parse_int_line(trimmed, "block_chance", &config->block_chance);
            parse_int_line(trimmed, "aggressive_tactics", &config->aggressive_tactics);
            parse_int_line(trimmed, "jump_frequency_mult", &config->jump_frequency_mult);
        }

        omf_free(line_copy);
        line = newline + 1;
    }

    omf_free(buf);
    return true;
}

const ai_core_config *ai_core_config_get(void) {
    if(g_ai_core_config == NULL) {
        g_ai_core_config = omf_calloc(1, sizeof(ai_core_config));
        load_ai_core_config(g_ai_core_config);
    }
    return g_ai_core_config;
}

bool ai_core_config_load_for_difficulty(int difficulty, ai_core_config *config) {
    if(config == NULL) {
        return false;
    }

    load_ai_core_config(config);
    return load_ai_difficulty_config(config, difficulty);
}

const ai_core_config *ai_core_config_get_for_difficulty(int difficulty) {
    static ai_core_config *g_ai_core_config_by_difficulty[7] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL};

    if(difficulty < 0 || difficulty > 6) {
        return ai_core_config_get();
    }

    if(g_ai_core_config_by_difficulty[difficulty] == NULL) {
        g_ai_core_config_by_difficulty[difficulty] = omf_calloc(1, sizeof(ai_core_config));
        ai_core_config_load_for_difficulty(difficulty, g_ai_core_config_by_difficulty[difficulty]);
    }

    return g_ai_core_config_by_difficulty[difficulty];
}

bool ai_core_config_reload(void) {
    if(g_ai_core_config != NULL) {
        return load_ai_core_config(g_ai_core_config);
    }
    return false;
}

void ai_core_config_free(void) {
    if(g_ai_core_config != NULL) {
        omf_free(g_ai_core_config);
        g_ai_core_config = NULL;
    }
}
