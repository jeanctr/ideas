#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "commands.h"
#include "colors.h"

#define MAX_TEXT_LEN 1024
#define MAX_LINE_LEN 2048

static const char *VALID_STATUSES[] = {
    "new", "in_progress", "paused", "completed", "discarded"
};
static const int VALID_STATUSES_COUNT = 5;

static const char *VALID_PRIORITIES[] = { "low", "medium", "high" };
static const int VALID_PRIORITIES_COUNT = 3;

/* ---------- Validation helpers ---------- */

static int is_valid_status(const char *status) {
    for (int i = 0; i < VALID_STATUSES_COUNT; i++) {
        if (strcmp(status, VALID_STATUSES[i]) == 0) return 1;
    }
    return 0;
}

static int is_valid_priority(const char *priority) {
    for (int i = 0; i < VALID_PRIORITIES_COUNT; i++) {
        if (strcmp(priority, VALID_PRIORITIES[i]) == 0) return 1;
    }
    return 0;
}

static void print_valid_statuses(void) {
    printf("Valid statuses: ");
    for (int i = 0; i < VALID_STATUSES_COUNT; i++) {
        printf("%s%s", VALID_STATUSES[i], (i < VALID_STATUSES_COUNT - 1) ? ", " : "\n");
    }
}

static void print_valid_priorities(void) {
    printf("Valid priorities: ");
    for (int i = 0; i < VALID_PRIORITIES_COUNT; i++) {
        printf("%s%s", VALID_PRIORITIES[i], (i < VALID_PRIORITIES_COUNT - 1) ? ", " : "\n");
    }
}

static void get_current_date(char *out, size_t out_size) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(out, out_size, "%Y-%m-%d", tm_info);
}

/* ---------- Row printing ---------- */

static void print_idea_row(int id, const char *created_at, const char *text,
                            const char *status, const char *priority, const char *tags) {
    printf("[%d] (%s) %s%-12s%s %s%-6s%s %s",
        id, created_at,
        color_for_status(status), status, color_reset_out(),
        color_for_priority(priority), priority, color_reset_out(),
        text);
    if (tags != NULL && tags[0] != '\0') {
        printf(" #%s", tags);
    }
    printf("\n");
}

/* ---------- add ---------- */

void cmd_add(sqlite3 *db, const char *text, const char *priority, const char *tags) {
    if (text == NULL || strlen(text) == 0) {
        fprintf(stderr, "%sError: idea text cannot be empty%s\n", color_error(), color_reset_err());
        return;
    }

    const char *final_priority = (priority != NULL) ? priority : "medium";
    const char *final_tags = (tags != NULL) ? tags : "";

    if (!is_valid_priority(final_priority)) {
        fprintf(stderr, "%sError: invalid priority '%s'%s\n", color_error(), final_priority, color_reset_err());
        print_valid_priorities();
        return;
    }

    char date[11];
    get_current_date(date, sizeof(date));

    const char *sql = "INSERT INTO ideas (created_at, text, status, priority, tags) "
                       "VALUES (?, ?, 'new', ?, ?);";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "%sError preparing insert: %s%s\n", color_error(), sqlite3_errmsg(db), color_reset_err());
        return;
    }

    sqlite3_bind_text(stmt, 1, date, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, text, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, final_priority, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, final_tags, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        fprintf(stderr, "%sError inserting idea: %s%s\n", color_error(), sqlite3_errmsg(db), color_reset_err());
    } else {
        printf("%sIdea saved%s (id %lld): %s\n", color_success(), color_reset_out(), (long long)sqlite3_last_insert_rowid(db), text);
    }

    sqlite3_finalize(stmt);
}

/* ---------- list ---------- */

void cmd_list(sqlite3 *db, const char *status_filter, const char *priority_filter,
              const char *tag_filter, const char *sort_by, int show_all) {
    char sql[512] = "SELECT id, created_at, text, status, priority, tags FROM ideas WHERE 1=1";

    if (status_filter != NULL) {
        strcat(sql, " AND status = ?");
    } else if (!show_all) {
        // Default view: hide completed/discarded clutter unless asked for.
        strcat(sql, " AND status NOT IN ('completed', 'discarded')");
    }

    if (priority_filter != NULL) strcat(sql, " AND priority = ?");
    if (tag_filter != NULL) strcat(sql, " AND tags LIKE ?");

    if (sort_by != NULL && strcmp(sort_by, "date") == 0) {
        strcat(sql, " ORDER BY created_at");
    } else if (sort_by != NULL && strcmp(sort_by, "priority") == 0) {
        strcat(sql, " ORDER BY CASE priority WHEN 'high' THEN 0 WHEN 'medium' THEN 1 ELSE 2 END");
    } else {
        strcat(sql, " ORDER BY id");
    }
    strcat(sql, ";");

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "%sError preparing select: %s%s\n", color_error(), sqlite3_errmsg(db), color_reset_err());
        return;
    }

    int idx = 1;
    if (status_filter != NULL) sqlite3_bind_text(stmt, idx++, status_filter, -1, SQLITE_STATIC);
    if (priority_filter != NULL) sqlite3_bind_text(stmt, idx++, priority_filter, -1, SQLITE_STATIC);
    char tag_pattern[MAX_TEXT_LEN];
    if (tag_filter != NULL) {
        snprintf(tag_pattern, sizeof(tag_pattern), "%%%s%%", tag_filter);
        sqlite3_bind_text(stmt, idx++, tag_pattern, -1, SQLITE_STATIC);
    }

    int count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        count++;
        print_idea_row(
            sqlite3_column_int(stmt, 0),
            (const char *)sqlite3_column_text(stmt, 1),
            (const char *)sqlite3_column_text(stmt, 2),
            (const char *)sqlite3_column_text(stmt, 3),
            (const char *)sqlite3_column_text(stmt, 4),
            (const char *)sqlite3_column_text(stmt, 5));
    }

    if (count == 0) {
        printf("%sNo ideas found.%s\n", color_info(), color_reset_out());
    }

    sqlite3_finalize(stmt);
}

/* ---------- status ---------- */

void cmd_status(sqlite3 *db, int id, const char *new_status) {
    if (!is_valid_status(new_status)) {
        fprintf(stderr, "%sError: invalid status '%s'%s\n", color_error(), new_status, color_reset_err());
        print_valid_statuses();
        return;
    }

    const char *sql = "UPDATE ideas SET status = ? WHERE id = ?;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "%sError preparing update: %s%s\n", color_error(), sqlite3_errmsg(db), color_reset_err());
        return;
    }

    sqlite3_bind_text(stmt, 1, new_status, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, id);
    sqlite3_step(stmt);

    if (sqlite3_changes(db) == 0) {
        printf("%sNo idea found with id %d%s\n", color_warning(), id, color_reset_out());
    } else {
        printf("%sIdea %d status updated to '%s'%s\n", color_success(), id, new_status, color_reset_out());
    }

    sqlite3_finalize(stmt);
}

/* ---------- edit ---------- */

void cmd_edit(sqlite3 *db, int id, const char *new_text) {
    if (new_text == NULL || strlen(new_text) == 0) {
        fprintf(stderr, "%sError: idea text cannot be empty%s\n", color_error(), color_reset_err());
        return;
    }

    const char *sql = "UPDATE ideas SET text = ? WHERE id = ?;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "%sError preparing update: %s%s\n", color_error(), sqlite3_errmsg(db), color_reset_err());
        return;
    }

    sqlite3_bind_text(stmt, 1, new_text, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, id);
    sqlite3_step(stmt);

    if (sqlite3_changes(db) == 0) {
        printf("%sNo idea found with id %d%s\n", color_warning(), id, color_reset_out());
    } else {
        printf("%sIdea %d updated%s\n", color_success(), id, color_reset_out());
    }

    sqlite3_finalize(stmt);
}

/* ---------- delete ---------- */

void cmd_delete(sqlite3 *db, int id, int force) {
    if (!force) {
        printf("%sDelete idea %d?%s [y/N] ", color_warning(), id, color_reset_out());
        fflush(stdout);
        char answer[16];
        if (fgets(answer, sizeof(answer), stdin) == NULL ||
            (answer[0] != 'y' && answer[0] != 'Y')) {
            printf("%sCancelled.%s\n", color_info(), color_reset_out());
            return;
        }
    }

    const char *sql = "DELETE FROM ideas WHERE id = ?;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "%sError preparing delete: %s%s\n", color_error(), sqlite3_errmsg(db), color_reset_err());
        return;
    }

    sqlite3_bind_int(stmt, 1, id);
    sqlite3_step(stmt);

    if (sqlite3_changes(db) == 0) {
        printf("%sNo idea found with id %d%s\n", color_warning(), id, color_reset_out());
    } else {
        printf("%sIdea %d deleted%s\n", color_success(), id, color_reset_out());
    }

    sqlite3_finalize(stmt);
}

/* ---------- search ---------- */

void cmd_search(sqlite3 *db, const char *keyword) {
    const char *sql = "SELECT id, created_at, text, status, priority, tags "
                       "FROM ideas WHERE text LIKE ? OR tags LIKE ? ORDER BY id;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "%sError preparing search: %s%s\n", color_error(), sqlite3_errmsg(db), color_reset_err());
        return;
    }

    char pattern[MAX_TEXT_LEN];
    snprintf(pattern, sizeof(pattern), "%%%s%%", keyword);
    sqlite3_bind_text(stmt, 1, pattern, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, pattern, -1, SQLITE_STATIC);

    int count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        count++;
        print_idea_row(
            sqlite3_column_int(stmt, 0),
            (const char *)sqlite3_column_text(stmt, 1),
            (const char *)sqlite3_column_text(stmt, 2),
            (const char *)sqlite3_column_text(stmt, 3),
            (const char *)sqlite3_column_text(stmt, 4),
            (const char *)sqlite3_column_text(stmt, 5));
    }

    if (count == 0) {
        printf("%sNo matching ideas found.%s\n", color_info(), color_reset_out());
    }

    sqlite3_finalize(stmt);
}

/* ---------- stats ---------- */

void cmd_stats(sqlite3 *db) {
    const char *sql = "SELECT status, COUNT(*) FROM ideas GROUP BY status;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "%sError preparing stats query: %s%s\n", color_error(), sqlite3_errmsg(db), color_reset_err());
        return;
    }

    int total = 0;
    int counts[5] = {0, 0, 0, 0, 0}; // matches VALID_STATUSES order

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *status = (const char *)sqlite3_column_text(stmt, 0);
        int n = sqlite3_column_int(stmt, 1);
        total += n;
        for (int i = 0; i < VALID_STATUSES_COUNT; i++) {
            if (strcmp(status, VALID_STATUSES[i]) == 0) {
                counts[i] = n;
                break;
            }
        }
    }
    sqlite3_finalize(stmt);

    printf("%sTotal ideas:%s %d\n", color_info(), color_reset_out(), total);
    for (int i = 0; i < VALID_STATUSES_COUNT; i++) {
        printf("  %s%-12s%s %d\n", color_for_status(VALID_STATUSES[i]),
               VALID_STATUSES[i], color_reset_out(), counts[i]);
    }
}

/* ---------- export ---------- */

static void write_csv_field(FILE *f, const char *text) {
    fputc('"', f);
    for (const char *p = text; *p != '\0'; p++) {
        if (*p == '"') fputc('"', f);
        fputc(*p, f);
    }
    fputc('"', f);
}

static void write_json_field(FILE *f, const char *text) {
    fputc('"', f);
    for (const char *p = text; *p != '\0'; p++) {
        if (*p == '"' || *p == '\\') fputc('\\', f);
        fputc(*p, f);
    }
    fputc('"', f);
}

void cmd_export(sqlite3 *db, const char *format, const char *filename) {
    if (strcmp(format, "csv") != 0 && strcmp(format, "json") != 0 && strcmp(format, "txt") != 0) {
        fprintf(stderr, "%sError: unsupported export format '%s' (use csv, json, or txt)%s\n", color_error(), format, color_reset_err());
        return;
    }

    FILE *f = fopen(filename, "w");
    if (f == NULL) {
        fprintf(stderr, "%sError: could not open %s for writing%s\n", color_error(), filename, color_reset_err());
        return;
    }

    const char *sql = "SELECT id, created_at, text, status, priority, tags FROM ideas ORDER BY id;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "%sError preparing export query: %s%s\n", color_error(), sqlite3_errmsg(db), color_reset_err());
        fclose(f);
        return;
    }

    if (strcmp(format, "csv") == 0) {
        fprintf(f, "id,created_at,text,status,priority,tags\n");
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            fprintf(f, "%d,", sqlite3_column_int(stmt, 0));
            write_csv_field(f, (const char *)sqlite3_column_text(stmt, 1));
            fputc(',', f);
            write_csv_field(f, (const char *)sqlite3_column_text(stmt, 2));
            fputc(',', f);
            write_csv_field(f, (const char *)sqlite3_column_text(stmt, 3));
            fputc(',', f);
            write_csv_field(f, (const char *)sqlite3_column_text(stmt, 4));
            fputc(',', f);
            write_csv_field(f, (const char *)sqlite3_column_text(stmt, 5));
            fputc('\n', f);
        }
    } else if (strcmp(format, "json") == 0) {
        fprintf(f, "[\n");
        int first = 1;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            if (!first) fprintf(f, ",\n");
            first = 0;
            fprintf(f, "  {\"id\": %d, \"created_at\": ", sqlite3_column_int(stmt, 0));
            write_json_field(f, (const char *)sqlite3_column_text(stmt, 1));
            fprintf(f, ", \"text\": ");
            write_json_field(f, (const char *)sqlite3_column_text(stmt, 2));
            fprintf(f, ", \"status\": ");
            write_json_field(f, (const char *)sqlite3_column_text(stmt, 3));
            fprintf(f, ", \"priority\": ");
            write_json_field(f, (const char *)sqlite3_column_text(stmt, 4));
            fprintf(f, ", \"tags\": ");
            write_json_field(f, (const char *)sqlite3_column_text(stmt, 5));
            fprintf(f, "}");
        }
        fprintf(f, "\n]\n");
    } else { // txt
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            fprintf(f, "[%d] (%s) %s/%s | %s",
                sqlite3_column_int(stmt, 0),
                sqlite3_column_text(stmt, 1),
                sqlite3_column_text(stmt, 3),
                sqlite3_column_text(stmt, 4),
                sqlite3_column_text(stmt, 2));
            const char *tags = (const char *)sqlite3_column_text(stmt, 5);
            if (tags != NULL && tags[0] != '\0') fprintf(f, " #%s", tags);
            fprintf(f, "\n");
        }
    }

    sqlite3_finalize(stmt);
    fclose(f);
    printf("%sExported to %s%s\n", color_success(), filename, color_reset_out());
}

/* ---------- import ---------- */

// Parses one CSV line into up to `max_fields` fields, handling quoted fields
// (including doubled "" as an escaped quote). Returns the number of fields found.
static int parse_csv_line(const char *line, char fields[][MAX_TEXT_LEN], int max_fields) {
    int field_count = 0;
    int in_quotes = 0;
    int pos = 0;
    fields[0][0] = '\0';

    for (const char *p = line; *p != '\0' && *p != '\n'; p++) {
        if (in_quotes) {
            if (*p == '"') {
                if (*(p + 1) == '"') {
                    fields[field_count][pos++] = '"';
                    p++;
                } else {
                    in_quotes = 0;
                }
            } else {
                fields[field_count][pos++] = *p;
            }
        } else {
            if (*p == '"') {
                in_quotes = 1;
            } else if (*p == ',') {
                fields[field_count][pos] = '\0';
                field_count++;
                if (field_count >= max_fields) break;
                pos = 0;
                fields[field_count][0] = '\0';
            } else {
                fields[field_count][pos++] = *p;
            }
        }
    }
    fields[field_count][pos] = '\0';
    field_count++;

    return field_count;
}

static void import_csv(sqlite3 *db, FILE *f) {
    char line[MAX_LINE_LEN];
    char fields[6][MAX_TEXT_LEN];
    int imported = 0;
    int line_num = 0;

    while (fgets(line, sizeof(line), f) != NULL) {
        line_num++;
        if (line_num == 1) continue; // skip header row

        int n = parse_csv_line(line, fields, 6);
        // Expected columns: id, created_at, text, status, priority, tags
        if (n < 6) {
            fprintf(stderr, "%sSkipping malformed line %d%s\n", color_warning(), line_num, color_reset_err());
            continue;
        }

        const char *sql = "INSERT INTO ideas (created_at, text, status, priority, tags) "
                           "VALUES (?, ?, ?, ?, ?);";
        sqlite3_stmt *stmt;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) continue;

        sqlite3_bind_text(stmt, 1, fields[1], -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, fields[2], -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, fields[3], -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 4, fields[4], -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 5, fields[5], -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) == SQLITE_DONE) imported++;
        sqlite3_finalize(stmt);
    }

    printf("%sImported %d idea(s) from CSV%s\n", color_success(), imported, color_reset_out());
}

// Minimal parser for our own JSON export format only: an array of flat
// objects with string fields "created_at", "text", "status", "priority", "tags".
// This is NOT a general-purpose JSON parser.
static int extract_json_field(const char *obj_line, const char *key, char *out, size_t out_size) {
    char search_key[64];
    snprintf(search_key, sizeof(search_key), "\"%s\": \"", key);

    const char *start = strstr(obj_line, search_key);
    if (start == NULL) return 0;
    start += strlen(search_key);

    size_t i = 0;
    while (*start != '\0' && !(*start == '"' && *(start - 1) != '\\') && i < out_size - 1) {
        if (*start == '\\' && *(start + 1) == '"') {
            out[i++] = '"';
            start += 2;
        } else {
            out[i++] = *start++;
        }
    }
    out[i] = '\0';
    return 1;
}

static void import_json(sqlite3 *db, FILE *f) {
    char line[MAX_LINE_LEN];
    char created_at[MAX_TEXT_LEN], text[MAX_TEXT_LEN], status[MAX_TEXT_LEN];
    char priority[MAX_TEXT_LEN], tags[MAX_TEXT_LEN];
    int imported = 0;

    while (fgets(line, sizeof(line), f) != NULL) {
        if (strstr(line, "\"id\":") == NULL) continue; // only object lines

        if (!extract_json_field(line, "created_at", created_at, sizeof(created_at))) continue;
        if (!extract_json_field(line, "text", text, sizeof(text))) continue;
        if (!extract_json_field(line, "status", status, sizeof(status))) continue;
        if (!extract_json_field(line, "priority", priority, sizeof(priority))) continue;
        if (!extract_json_field(line, "tags", tags, sizeof(tags))) tags[0] = '\0';

        const char *sql = "INSERT INTO ideas (created_at, text, status, priority, tags) "
                           "VALUES (?, ?, ?, ?, ?);";
        sqlite3_stmt *stmt;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) continue;

        sqlite3_bind_text(stmt, 1, created_at, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, text, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, status, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 4, priority, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 5, tags, -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) == SQLITE_DONE) imported++;
        sqlite3_finalize(stmt);
    }

    printf("%sImported %d idea(s) from JSON%s\n", color_success(), imported, color_reset_out());
}

void cmd_import(sqlite3 *db, const char *format, const char *filename) {
    if (strcmp(format, "csv") != 0 && strcmp(format, "json") != 0) {
        fprintf(stderr, "%sError: import only supports 'csv' or 'json' (not '%s')%s\n", color_error(), format, color_reset_err());
        return;
    }

    FILE *f = fopen(filename, "r");
    if (f == NULL) {
        fprintf(stderr, "%sError: could not open %s for reading%s\n", color_error(), filename, color_reset_err());
        return;
    }

    if (strcmp(format, "csv") == 0) {
        import_csv(db, f);
    } else {
        import_json(db, f);
    }

    fclose(f);
}

/* ---------- Help ---------- */

void print_usage(const char *prog_name) {
    printf("Usage: %s <command> [args]\n\n", prog_name);
    printf("Commands:\n");
    printf("  add <text...> [--priority=P] [--tags=a,b]   Add a new idea\n");
    printf("  list [--status=S] [--priority=P] [--tag=T]\n");
    printf("       [--sort=date|priority] [--all]          List ideas\n");
    printf("  status <id> <new_status>                     Change an idea's status\n");
    printf("  edit <id> <text...>                           Edit an idea's text\n");
    printf("  delete <id> [--force]                         Delete an idea\n");
    printf("  search <keyword>                              Search text and tags\n");
    printf("  stats                                         Show counts by status\n");
    printf("  export <csv|json|txt> [file]                  Export all ideas\n");
    printf("  import <csv|json> <file>                      Import ideas from a file\n");
    printf("  help [command]                                Show this help, or help for one command\n\n");
    print_valid_statuses();
    print_valid_priorities();
}

void print_command_help(const char *prog_name, const char *command) {
    if (strcmp(command, "add") == 0) {
        printf("%s add <text...> [--priority=low|medium|high] [--tags=a,b,c]\n", prog_name);
        printf("Adds a new idea with status 'new'. Text can be written without quotes.\n");
    } else if (strcmp(command, "list") == 0) {
        printf("%s list [--status=S] [--priority=P] [--tag=T] [--sort=date|priority] [--all]\n", prog_name);
        printf("Lists ideas. By default hides 'completed' and 'discarded' unless --all is given.\n");
    } else if (strcmp(command, "status") == 0) {
        printf("%s status <id> <new_status>\n", prog_name);
        print_valid_statuses();
    } else if (strcmp(command, "edit") == 0) {
        printf("%s edit <id> <new text...>\n", prog_name);
        printf("Overwrites the idea's text. Creation date is kept.\n");
    } else if (strcmp(command, "delete") == 0) {
        printf("%s delete <id> [--force]\n", prog_name);
        printf("Asks for confirmation unless --force is given.\n");
    } else if (strcmp(command, "search") == 0) {
        printf("%s search <keyword>\n", prog_name);
        printf("Matches against both idea text and tags.\n");
    } else if (strcmp(command, "stats") == 0) {
        printf("%s stats\n", prog_name);
        printf("Shows the total idea count and a breakdown by status.\n");
    } else if (strcmp(command, "export") == 0) {
        printf("%s export <csv|json|txt> [filename]\n", prog_name);
        printf("Default filename is ideas_export.<format>.\n");
    } else if (strcmp(command, "import") == 0) {
        printf("%s import <csv|json> <filename>\n", prog_name);
        printf("Note: JSON import only understands this program's own export format.\n");
    } else {
        printf("Unknown command: %s\n\n", command);
        print_usage(prog_name);
    }
}
