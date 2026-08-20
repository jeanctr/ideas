# Idea Tracker

A lightweight command-line tool for capturing and tracking ideas, written in C with SQLite for persistent storage.

## Features

- Track ideas through a lifecycle: `new` → `in_progress` / `paused` → `completed` / `discarded`
- Priority levels (`low`, `medium`, `high`) and free-form tags
- Full-text search across idea text and tags
- Export to CSV, JSON, or plain text — import from CSV or JSON
- Colored terminal output (auto-disabled when output isn't a terminal)
- Single SQLite file at `~/.ideas/ideas.db`, accessible from any directory

## Requirements

- GCC (or another C11-compatible compiler)
- `libsqlite3-dev`

```bash
sudo apt install libsqlite3-dev
```

## Installation

```bash
git clone <repo-url>
cd idea-tracker
make
```

This produces an `ideas` executable in the project directory. Optionally, copy it somewhere on your `PATH`:

```bash
cp ideas ~/.local/bin/
```

## Usage

```bash
ideas add <text...> [--priority=low|medium|high] [--tags=a,b]
ideas list [--status=S] [--priority=P] [--tag=T] [--sort=date|priority] [--all]
ideas status <id> <new_status>
ideas edit <id> <new text...>
ideas delete <id> [--force]
ideas search <keyword>
ideas stats
ideas export <csv|json|txt> [file]
ideas import <csv|json> <file>
ideas help [command]
```

**Statuses:** `new`, `in_progress`, `paused`, `completed`, `discarded`
**Priorities:** `low`, `medium`, `high`

### Examples

```bash
ideas add build a CLI idea tracker --priority=high --tags=cli,c
ideas list --all --sort=priority
ideas status 1 in_progress
ideas export json
```

## Data storage

Ideas are stored in a single SQLite database at `~/.ideas/ideas.db`, created automatically on first run. Because the path is fixed to the home directory, the tool behaves the same regardless of your current working directory.

## Project structure

| File | Responsibility |
|---|---|
| `main.c` | Parses arguments and flags, dispatches to the right command |
| `db.h` / `db.c` | Opens the SQLite connection, creates and migrates the schema |
| `commands.h` / `commands.c` | Implements each command's logic |
| `colors.h` / `colors.c` | Terminal color output |
| `Makefile` | Build (`make`) and test (`make test`) targets |
| `tests/test.sh` | Automated smoke tests |

## Testing

```bash
make test
```

Runs an end-to-end smoke test suite against a temporary, isolated database.

## Roadmap

- [ ] Continuous integration to run `make test` automatically on every change

## License

Personal project — no license specified yet.
