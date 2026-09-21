/**
 * @file log.h
 * @brief Logging system with multiple outputs and severity levels.
 * @details Provides a flexible logging system that supports multiple output destinations
 *          (stderr, files) with independent log levels. Supports colored output for terminals.
 * @copyright MIT License
 * @date 2026
 * @author OpenOMF Project
 */

#ifndef LOG_H
#define LOG_H

#include <stdbool.h>
#include <stdlib.h>

/**
 * @brief Log message severity levels.
 */
typedef enum log_level
{
    LOG_DEBUG, ///< Debug messages for development
    LOG_INFO,  ///< Informational messages
    LOG_WARN,  ///< Warning messages
    LOG_ERROR  ///< Error messages
} log_level;

/**
 * @brief Log module tags, used for per-module filtering of debug output.
 * @details Messages logged with log_debug_m() and friends are only emitted when all the
 *          module bits are present in the active module filter (see
 *          log_set_module_filter()). Messages with no module tag (log_debug() etc.) are
 *          always emitted, subject to the configured level; when a specific module filter
 *          is set, untagged DEBUG messages are suppressed while INFO/WARN/ERROR remain.
 */
typedef uint64_t log_module;
#define LOG_MODULE_NONE ((log_module)0)
#define LOG_MODULE_AI ((log_module)1 << 0)
#define LOG_MODULE_TACTIC ((log_module)1 << 1)
#define LOG_MODULE_HAR ((log_module)1 << 2)
#define LOG_MODULE_REC ((log_module)1 << 3)
#define LOG_MODULE_MOVE ((log_module)1 << 4)
#define LOG_MODULE_MOVEMENT ((log_module)1 << 5)
#define LOG_MODULE_LEARNING ((log_module)1 << 6)
#define LOG_MODULE_CONFIG ((log_module)1 << 7)
#define LOG_MODULE_ALL (~((log_module)0))

/**
 * @brief Log a debug message.
 * @param ... Printf-style format string and arguments
 */
#define log_debug(...) log_msg(LOG_DEBUG, __VA_ARGS__)

/**
 * @brief Log an informational message.
 * @param ... Printf-style format string and arguments
 */
#define log_info(...) log_msg(LOG_INFO, __VA_ARGS__)

/**
 * @brief Log a warning message.
 * @param ... Printf-style format string and arguments
 */
#define log_warn(...) log_msg(LOG_WARN, __VA_ARGS__)

/**
 * @brief Log an error message.
 * @param ... Printf-style format string and arguments
 */
#define log_error(...) log_msg(LOG_ERROR, __VA_ARGS__)

/**
 * @brief Log a debug message tagged with a module.
 * @param module Module bitmask to tag the message with.
 * @param ... Printf-style format string and arguments
 */
#define log_debug_m(module, ...) log_msg_module(module, LOG_DEBUG, __VA_ARGS__)

/**
 * @brief Log an informational message tagged with a module.
 * @param module Module bitmask to tag the message with.
 * @param ... Printf-style format string and arguments
 */
#define log_info_m(module, ...) log_msg_module(module, LOG_INFO, __VA_ARGS__)

/**
 * @brief Log a warning message tagged with a module.
 * @param module Module bitmask to tag the message with.
 * @param ... Printf-style format string and arguments
 */
#define log_warn_m(module, ...) log_msg_module(module, LOG_WARN, __VA_ARGS__)

/**
 * @brief Log an error message tagged with a module.
 * @param module Module bitmask to tag the message with.
 * @param ... Printf-style format string and arguments
 */
#define log_error_m(module, ...) log_msg_module(module, LOG_ERROR, __VA_ARGS__)

/**
 * @brief Set the module filter for tagged log messages.
 * @details Only messages whose module bits are all present in the filter are emitted.
 *          Pass LOG_MODULE_ALL (or LOG_MODULE_NONE) to enable all modules.
 * @param modules Bitmask of modules to enable.
 */
void log_set_module_filter(log_module modules);

/**
 * @brief Get the current module filter.
 * @return The active module bitmask.
 */
log_module log_get_module_filter(void);

/**
 * @brief Check whether a tagged module is currently enabled.
 * @param module Module bitmask to check.
 * @return true if the module's bits are all enabled, false otherwise.
 */
bool log_module_enabled(log_module module);

/**
 * @brief Whether the logging system has been initialized.
 */
bool log_is_initialized(void);

/**
 * @brief Parse a comma-separated list of module names into a bitmask.
 * @details Recognized names: ai, tactic, har, rec, move, movement, learning, config, all.
 *          Unknown names are ignored. "all", empty, or NULL yields LOG_MODULE_ALL.
 * @param modules Comma-separated module name list.
 * @return The combined module bitmask.
 */
log_module log_modules_from_string(const char *modules);

/**
 * @brief Log a message at the specified level, tagged with a module.
 * @details Prefer using the log_debug_m/info_m/warn_m/error_m macros instead.
 * @param module Module bitmask to tag the message with.
 * @param level Severity level
 * @param fmt Printf-style format string
 * @param ... Format arguments
 */
void log_msg_module(log_module module, log_level level, const char *fmt, ...);

/**
 * @brief Initialize the logging system.
 * @details Must be called before any logging functions.
 */
void log_init(void);

/**
 * @brief Close the logging system and release resources.
 * @details Closes all open log file handles.
 */
void log_close(void);

/**
 * @brief Set the minimum log level for all outputs.
 * @details Messages below this level will not be logged anywhere.
 * @param level Minimum log level
 */
void log_set_level(log_level level);

/**
 * @brief Enable or disable colored output globally.
 * @param toggle true to enable colors, false to disable
 */
void log_set_colors(bool toggle);

/**
 * @brief Add stderr as a log output destination.
 * @param level Minimum log level for this output
 * @param colors Whether to use ANSI colors for this output
 */
void log_add_stderr(log_level level, bool colors);

/**
 * @brief Add a file as a log output destination.
 * @param filename Path to the log file
 * @param level Minimum log level for this output
 */
void log_add_file(const char *filename, log_level level);

/**
 * @brief Convert a log level string to enum value.
 * @param level String representation (e.g., "DEBUG", "INFO")
 * @param default_value Value to return if string is not recognized
 * @return The corresponding log_level enum value
 */
log_level log_level_text_to_enum(const char *level, log_level default_value);

/**
 * @brief Check if a string is a valid log level name.
 * @param level String to check
 * @return true if valid log level, false otherwise
 */
bool is_log_level(const char *level);

/**
 * @brief Log a message at the specified level.
 * @details Prefer using the log_debug/info/warn/error macros instead.
 * @param level Severity level
 * @param fmt Printf-style format string
 * @param ... Format arguments
 */
void log_msg(log_level level, const char *fmt, ...);

/**
 * @brief Get the last logged error message.
 * @return The most recent error message, or NULL if none
 */
const char *log_last_error(void);

#endif // LOG_H
