#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include "db.h"

#define MAX_PATH_LEN 512

// Builds ~/.ideas/ideas.db and makes sure the ~/.ideas directory exists.
static void get_db_path(char *out_path, size_t out_size) {
    const char *home = getenv("HOME");
    if (home == NULL) {
        fprintf(stderr, "Error: could not determine HOME directory\n");
        exit(1);
    }

    char dir_path[MAX_PATH_LEN];
    snprintf(dir_path, sizeof(dir_path), "%s/.ideas", home);

    if (mkdir(dir_path, 0755) != 0 && errno != EEXIST) {
        fprintf(stderr, "Warning: could not create %s: %s\n", dir_path, strerror(errno));
    }

    snprintf(out_path, out_size, "%s/ideas.db", dir_path);
}

// Checks whether a column already exists on a table, via PRAGMA table_info.
static int column_exists(sqlite3 *db, const char *table, const char *column) {
    char sql[128];
    snprintf(sql, sizeof(sql), "PRAGMA table_info(%s);", table);

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return 0;
    }

    int found = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char *name = sqlite3_column_text(stmt, 1); // column 1 = name
        if (name != NULL && strcmp((const char *)name, column) == 0) {
            found = 1;
            break;
        }
    }

    sqlite3_finalize(stmt);
    return found;
}

// Adds any columns that were introduced after the table was first created,
// so existing databases from earlier versions of the program keep working.
static void migrate_schema(sqlite3 *db) {
    char *err_msg = NULL;

    if (!column_exists(db, "ideas", "priority")) {
        const char *sql = "ALTER TABLE ideas ADD COLUMN priority TEXT NOT NULL DEFAULT 'medium';";
        if (sqlite3_exec(db, sql, NULL, NULL, &err_msg) != SQLITE_OK) {
            fprintf(stderr, "Error migrating schema (priority): %s\n", err_msg);
            sqlite3_free(err_msg);
        }
    }

    if (!column_exists(db, "ideas", "tags")) {
        const char *sql = "ALTER TABLE ideas ADD COLUMN tags TEXT NOT NULL DEFAULT '';";
        if (sqlite3_exec(db, sql, NULL, NULL, &err_msg) != SQLITE_OK) {
            fprintf(stderr, "Error migrating schema (tags): %s\n", err_msg);
            sqlite3_free(err_msg);
        }
    }
}

sqlite3 *init_db(void) {
    char db_path[MAX_PATH_LEN];
    get_db_path(db_path, sizeof(db_path));

    sqlite3 *db;
    if (sqlite3_open(db_path, &db) != SQLITE_OK) {
        fprintf(stderr, "Error: cannot open database: %s\n", sqlite3_errmsg(db));
        exit(1);
    }

    const char *create_table_sql =
        "CREATE TABLE IF NOT EXISTS ideas ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  created_at TEXT NOT NULL,"
        "  text TEXT NOT NULL,"
        "  status TEXT NOT NULL DEFAULT 'new',"
        "  priority TEXT NOT NULL DEFAULT 'medium',"
        "  tags TEXT NOT NULL DEFAULT ''"
        ");";

    char *err_msg = NULL;
    if (sqlite3_exec(db, create_table_sql, NULL, NULL, &err_msg) != SQLITE_OK) {
        fprintf(stderr, "Error creating table: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        exit(1);
    }

    migrate_schema(db);

    return db;
}
