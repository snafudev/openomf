#include "utils/log.h"

#include <SDL_mutex.h>
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "utils/allocator.h"

#define MAX_TARGETS 3
#define LOG_LEVELS 4

static char last_error[256];

typedef struct log_target {
    FILE *fp;
    log_level level;
    bool colors;
    bool close;
    SDL_mutex *lock;
} log_target;

typedef struct log_state {
    bool colors;
    log_level level;
    log_module module_filter;
    log_target targets[MAX_TARGETS];
    int target_count;
    uint32_t tick;
} log_state;

static const char *level_names[] = {
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
};

static const char *level_colors[] = {
    "\x1b[36m",
    "\x1b[32m",
    "\x1b[33m",
    "\x1b[31m",
};

// Module names, indexed by bit position (must match the LOG_MODULE_* defines in log.h).
// Keep the legacy "tactic" alias accepted for compatibility, but prefer the more
// specific "ai-tactics" label for the AI decision brain channel.
static const char *module_names[] = {
    "ai",
    "ai-tactics",
    "har",
    "rec",
    "move",
    "movement",
    "learning",
    "config",
};

#define MODULE_NAME_COUNT (sizeof(module_names) / sizeof(module_names[0]))

static bool module_name_matches(const char *token, size_t len, const char *name) {
    return strlen(name) == len && strncmp(token, name, len) == 0;
}

static bool module_token_matches(const char *token, size_t len, size_t index) {
    if(index == 1) {
        return module_name_matches(token, len, "ai-tactics") || module_name_matches(token, len, "ai_tactic") ||
               module_name_matches(token, len, "ai_tactics") || module_name_matches(token, len, "tactic");
    }
    return module_name_matches(token, len, module_names[index]);
}

static log_state *state = NULL;

void log_init(void) {
    assert(state == NULL);
    state = omf_calloc(1, sizeof(log_state));
    state->level = LOG_DEBUG;
    state->colors = false;
    state->module_filter = LOG_MODULE_ALL;
    state->target_count = 0;
}

log_level log_level_text_to_enum(const char *level, log_level default_value) {
    for(int i = 0; i < LOG_LEVELS; i++) {
        if(strcmp(level_names[i], level) == 0) {
            return i;
        }
    }
    return default_value;
}

bool is_log_level(const char *level) {
    for(int i = 0; i < LOG_LEVELS; i++) {
        if(strcmp(level_names[i], level) == 0) {
            return true;
        }
    }
    return false;
}

static void close_targets(void) {
    assert(state != NULL);
    for(int i = 0; i < state->target_count; i++) {
        const log_target *target = &state->targets[i];
        if(target->close) {
            fclose(target->fp);
        }
        if(target->lock) {
            SDL_DestroyMutex(target->lock);
        }
    }
    state->target_count = 0;
}

void log_close(void) {
    if(state != NULL) {
        close_targets();
        omf_free(state);
    }
}

void log_set_level(log_level level) {
    assert(state != NULL);
    state->level = level;
}

void log_set_colors(bool toggle) {
    assert(state != NULL);
    state->colors = toggle;
}

bool log_is_initialized(void) {
    return state != NULL;
}

void log_set_module_filter(log_module modules) {
    assert(state != NULL);
    state->module_filter = modules;
}

log_module log_get_module_filter(void) {
    assert(state != NULL);
    return state->module_filter;
}

bool log_module_enabled(log_module module) {
    assert(state != NULL);
    if(module == LOG_MODULE_NONE) {
        return true;
    }
    return (state->module_filter & module) == module;
}

log_module log_modules_from_string(const char *modules) {
    if(modules == NULL || modules[0] == '\0') {
        return LOG_MODULE_ALL;
    }

    log_module result = LOG_MODULE_NONE;
    const char *p = modules;
    while(*p) {
        // Skip separators and whitespace between tokens.
        while(*p == ',' || *p == ' ' || *p == '\t') {
            p++;
        }
        if(!*p) {
            break;
        }

        const char *start = p;
        while(*p && *p != ',' && *p != ' ' && *p != '\t') {
            p++;
        }
        size_t len = (size_t)(p - start);

        if(len == 3 && strncmp(start, "all", 3) == 0) {
            return LOG_MODULE_ALL;
        }

        for(size_t i = 0; i < MODULE_NAME_COUNT; i++) {
            if(module_token_matches(start, len, i)) {
                result |= ((log_module)1 << i);
                break;
            }
        }
    }

    // Unknown or empty lists fall back to "everything enabled" rather than
    // silently silencing all module output.
    return result == LOG_MODULE_NONE ? LOG_MODULE_ALL : result;
}

// Render a module bitmask as a compact tag like " [ai,tactic]"; empty for LOG_MODULE_NONE.
static void format_module_tag(log_module module, char *buf, size_t len) {
    buf[0] = '\0';
    if(module == LOG_MODULE_NONE) {
        return;
    }

    size_t pos = 0;
    buf[pos++] = ' ';
    buf[pos++] = '[';
    bool first = true;
    for(size_t i = 0; i < MODULE_NAME_COUNT && pos + 2 < len; i++) {
        if(module & ((log_module)1 << i)) {
            if(!first && pos + 1 < len) {
                buf[pos++] = ',';
            }
            const char *name = module_names[i];
            while(*name && pos + 1 < len) {
                buf[pos++] = *name++;
            }
            first = false;
        }
    }
    if(pos + 1 < len) {
        buf[pos++] = ']';
    }
    buf[pos] = '\0';
}

static void log_add_fp(FILE *fp, bool close, log_level level, bool colors) {
    assert(state != NULL);
    assert(state->target_count < MAX_TARGETS - 1);
    log_target *target = &state->targets[state->target_count++];
    target->close = close;
    target->fp = fp;
    target->level = level;
    target->colors = colors;
    target->lock = SDL_CreateMutex();
}

void log_add_stderr(log_level level, bool colors) {
    log_add_fp(stderr, false, level, colors);
}

void log_add_file(const char *filename, log_level level) {
    FILE *fp = fopen(filename, "w");
    if(fp) {
        log_add_fp(fp, true, level, false);
    }
}

static void format_timestamp(char *buffer, size_t len) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    strftime(buffer, len, "%H:%M:%S", tm);
    buffer[len - 1] = 0;
}

static void log_vmsg_module(log_module module, log_level level, const char *fmt, va_list args) {
    assert(state != NULL);
    char dt[16];
    char module_tag[64];
    const char *color = level_colors[level];
    const char *name = level_names[level];

    if(level < state->level) {
        return;
    }
    // When a specific module filter is active (not ALL), only messages from the
    // enabled modules are emitted at DEBUG level. Untagged DEBUG messages are
    // dropped so that "listen to module X only" stays quiet, while INFO/WARN/ERROR
    // remain visible for important breadcrumbs and diagnostics.
    if(state->module_filter != LOG_MODULE_ALL) {
        if(module == LOG_MODULE_NONE) {
            if(level < LOG_INFO) {
                return;
            }
        } else if((state->module_filter & module) != module) {
            return;
        }
    }

    format_timestamp(dt, 16);
    format_module_tag(module, module_tag, sizeof(module_tag));
    for(int i = 0; i < state->target_count; i++) {
        const log_target *target = &state->targets[i];
        if(level < target->level) {
            continue;
        }
        if(SDL_LockMutex(target->lock) != 0) {
            continue;
        }
        if(state->colors && target->colors) {
            fprintf(target->fp, "%s %s%-5s\x1b[0m%s \x1b[0m ", dt, color, name, module_tag);
        } else {
            fprintf(target->fp, "%s %-5s%s ", dt, name, module_tag);
        }
        va_list args_copy;
        va_copy(args_copy, args);
        vfprintf(target->fp, fmt, args_copy);
        va_end(args_copy);
        if(level == LOG_ERROR) {
            va_copy(args_copy, args);
            vsnprintf(last_error, sizeof(last_error), fmt, args_copy);
            va_end(args_copy);
        }
        if(state->colors && target->colors) {
            fprintf(target->fp, "\x1b[0m\n");
        } else {
            fprintf(target->fp, "\n");
        }
        fflush(target->fp);
        SDL_UnlockMutex(target->lock);
    }
}

void log_msg(log_level level, const char *fmt, ...) {
    assert(state != NULL);
    va_list args;
    va_start(args, fmt);
    log_vmsg_module(LOG_MODULE_NONE, level, fmt, args);
    va_end(args);
}

void log_msg_module(log_module module, log_level level, const char *fmt, ...) {
    assert(state != NULL);
    va_list args;
    va_start(args, fmt);
    log_vmsg_module(module, level, fmt, args);
    va_end(args);
}

const char *log_last_error(void) {
    return last_error;
}
