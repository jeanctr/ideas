#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "db.h"
#include "commands.h"
#include "colors.h"

#define MAX_TEXT_LEN 1024

// Looks for a token of the form "--name=value" among argv[start..argc-1].
// Returns a pointer into argv on match, or NULL if not present.
static const char *find_flag(int argc, char *argv[], int start, const char *name) {
    char prefix[64];
    snprintf(prefix, sizeof(prefix), "--%s=", name);
    size_t prefix_len = strlen(prefix);

    for (int i = start; i < argc; i++) {
        if (strncmp(argv[i], prefix, prefix_len) == 0) {
            return argv[i] + prefix_len;
        }
    }
    return NULL;
}

// True if a bare flag like "--all" or "--force" is present.
static int find_bare_flag(int argc, char *argv[], int start, const char *name) {
    char flag[64];
    snprintf(flag, sizeof(flag), "--%s", name);

    for (int i = start; i < argc; i++) {
        if (strcmp(argv[i], flag) == 0) return 1;
    }
    return 0;
}

// Joins every argv token from `start` that is NOT a "--flag" or "--flag=value"
// into a single space-separated string. Used to collect free-text idea content
// while still allowing flags anywhere on the command line.
static void collect_text(char *out, size_t out_size, int argc, char *argv[], int start) {
    out[0] = '\0';
    int first = 1;
    for (int i = start; i < argc; i++) {
        if (strncmp(argv[i], "--", 2) == 0) continue; // skip flags
        if (!first) strncat(out, " ", out_size - strlen(out) - 1);
        strncat(out, argv[i], out_size - strlen(out) - 1);
        first = 0;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "help") == 0) {
        if (argc >= 3) {
            print_command_help(argv[0], argv[2]);
        } else {
            print_usage(argv[0]);
        }
        return 0;
    }

    sqlite3 *db = init_db();
    char text_buffer[MAX_TEXT_LEN];
    int status_code = 0;

    if (strcmp(argv[1], "add") == 0) {
        if (argc < 3) {
            fprintf(stderr, "%sUsage: %s add <idea text> [--priority=P] [--tags=a,b]%s\n", color_error(), argv[0], color_reset_err());
            status_code = 1;
        } else {
            collect_text(text_buffer, sizeof(text_buffer), argc, argv, 2);
            const char *priority = find_flag(argc, argv, 2, "priority");
            const char *tags = find_flag(argc, argv, 2, "tags");
            cmd_add(db, text_buffer, priority, tags);
        }
    } else if (strcmp(argv[1], "list") == 0) {
        const char *status_filter = find_flag(argc, argv, 2, "status");
        const char *priority_filter = find_flag(argc, argv, 2, "priority");
        const char *tag_filter = find_flag(argc, argv, 2, "tag");
        const char *sort_by = find_flag(argc, argv, 2, "sort");
        int show_all = find_bare_flag(argc, argv, 2, "all");
        cmd_list(db, status_filter, priority_filter, tag_filter, sort_by, show_all);
    } else if (strcmp(argv[1], "status") == 0) {
        if (argc < 4) {
            fprintf(stderr, "%sUsage: %s status <id> <new_status>%s\n", color_error(), argv[0], color_reset_err());
            status_code = 1;
        } else {
            cmd_status(db, atoi(argv[2]), argv[3]);
        }
    } else if (strcmp(argv[1], "edit") == 0) {
        if (argc < 4) {
            fprintf(stderr, "%sUsage: %s edit <id> <new text>%s\n", color_error(), argv[0], color_reset_err());
            status_code = 1;
        } else {
            collect_text(text_buffer, sizeof(text_buffer), argc, argv, 3);
            cmd_edit(db, atoi(argv[2]), text_buffer);
        }
    } else if (strcmp(argv[1], "delete") == 0) {
        if (argc < 3) {
            fprintf(stderr, "%sUsage: %s delete <id> [--force]%s\n", color_error(), argv[0], color_reset_err());
            status_code = 1;
        } else {
            int force = find_bare_flag(argc, argv, 3, "force");
            cmd_delete(db, atoi(argv[2]), force);
        }
    } else if (strcmp(argv[1], "search") == 0) {
        if (argc < 3) {
            fprintf(stderr, "%sUsage: %s search <keyword>%s\n", color_error(), argv[0], color_reset_err());
            status_code = 1;
        } else {
            cmd_search(db, argv[2]);
        }
    } else if (strcmp(argv[1], "stats") == 0) {
        cmd_stats(db);
    } else if (strcmp(argv[1], "export") == 0) {
        if (argc < 3) {
            fprintf(stderr, "%sUsage: %s export <csv|json|txt> [filename]%s\n", color_error(), argv[0], color_reset_err());
            status_code = 1;
        } else {
            char default_name[256];
            snprintf(default_name, sizeof(default_name), "ideas_export.%s", argv[2]);
            const char *filename = (argc >= 4) ? argv[3] : default_name;
            cmd_export(db, argv[2], filename);
        }
    } else if (strcmp(argv[1], "import") == 0) {
        if (argc < 4) {
            fprintf(stderr, "%sUsage: %s import <csv|json> <filename>%s\n", color_error(), argv[0], color_reset_err());
            status_code = 1;
        } else {
            cmd_import(db, argv[2], argv[3]);
        }
    } else {
        printf("%sUnknown command: %s%s\n\n", color_error(), argv[1], color_reset_out());
        print_usage(argv[0]);
        status_code = 1;
    }

    sqlite3_close(db);
    return status_code;
}
