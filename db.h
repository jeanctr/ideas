#ifndef DB_H
#define DB_H

#include <sqlite3.h>

// Opens (creating if needed) the database at ~/.ideas/ideas.db,
// ensures the schema exists and is up to date, and returns the handle.
// Exits the program on unrecoverable errors.
sqlite3 *init_db(void);

#endif
