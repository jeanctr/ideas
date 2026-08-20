#ifndef COLORS_H
#define COLORS_H

// Whether colored output should be used for a given stream (checks if it's
// a real terminal vs. a pipe/redirected file).
int stdout_colors_enabled(void);
int stderr_colors_enabled(void);

// Status/priority colors (used on stdout, in list/search/stats tables).
const char *color_for_status(const char *status);
const char *color_for_priority(const char *priority);

// General-purpose message colors.
const char *color_success(void);  // stdout, green
const char *color_info(void);     // stdout, cyan
const char *color_warning(void);  // stdout, yellow
const char *color_error(void);    // stderr, red

// Resets must match the stream the color was printed to.
const char *color_reset_out(void);
const char *color_reset_err(void);

#endif
