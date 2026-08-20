#ifndef COMMANDS_H
#define COMMANDS_H

#include <sqlite3.h>

// priority and tags may be NULL to use defaults ("medium" and "" respectively)
void cmd_add(sqlite3 *db, const char *text, const char *priority, const char *tags);

// Any filter may be NULL to mean "no filter on this field".
// show_all: if 0, hides "completed" and "discarded" ideas unless status_filter
// explicitly asks for them. sort_by: "date", "priority", or NULL (= id order).
void cmd_list(sqlite3 *db, const char *status_filter, const char *priority_filter,
              const char *tag_filter, const char *sort_by, int show_all);

void cmd_status(sqlite3 *db, int id, const char *new_status);
void cmd_edit(sqlite3 *db, int id, const char *new_text);
void cmd_delete(sqlite3 *db, int id, int force);
void cmd_search(sqlite3 *db, const char *keyword);
void cmd_export(sqlite3 *db, const char *format, const char *filename);
void cmd_import(sqlite3 *db, const char *format, const char *filename);
void cmd_stats(sqlite3 *db);

void print_usage(const char *prog_name);
void print_command_help(const char *prog_name, const char *command);

#endif
