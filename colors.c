#include <string.h>
#include <unistd.h>
#include "colors.h"

#define ANSI_RESET   "\033[0m"
#define ANSI_GREEN   "\033[32m"
#define ANSI_YELLOW  "\033[33m"
#define ANSI_BLUE    "\033[34m"
#define ANSI_CYAN    "\033[36m"
#define ANSI_RED     "\033[31m"
#define ANSI_GRAY    "\033[90m"

int stdout_colors_enabled(void) {
    return isatty(STDOUT_FILENO);
}

int stderr_colors_enabled(void) {
    return isatty(STDERR_FILENO);
}

const char *color_for_status(const char *status) {
    if (!stdout_colors_enabled()) return "";

    if (strcmp(status, "new") == 0) return ANSI_BLUE;
    if (strcmp(status, "in_progress") == 0) return ANSI_YELLOW;
    if (strcmp(status, "paused") == 0) return ANSI_GRAY;
    if (strcmp(status, "completed") == 0) return ANSI_GREEN;
    if (strcmp(status, "discarded") == 0) return ANSI_RED;
    return "";
}

const char *color_for_priority(const char *priority) {
    if (!stdout_colors_enabled()) return "";

    if (strcmp(priority, "high") == 0) return ANSI_RED;
    if (strcmp(priority, "medium") == 0) return ANSI_YELLOW;
    if (strcmp(priority, "low") == 0) return ANSI_GRAY;
    return "";
}

const char *color_success(void) {
    return stdout_colors_enabled() ? ANSI_GREEN : "";
}

const char *color_info(void) {
    return stdout_colors_enabled() ? ANSI_CYAN : "";
}

const char *color_warning(void) {
    return stdout_colors_enabled() ? ANSI_YELLOW : "";
}

const char *color_error(void) {
    return stderr_colors_enabled() ? ANSI_RED : "";
}

const char *color_reset_out(void) {
    return stdout_colors_enabled() ? ANSI_RESET : "";
}

const char *color_reset_err(void) {
    return stderr_colors_enabled() ? ANSI_RESET : "";
}
