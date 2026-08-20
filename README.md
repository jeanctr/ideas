# Idea Tracker (CLI)

A command-line idea tracker written in C, using SQLite for storage.

## Build

```bash
make
```

Requires `libsqlite3-dev`:
```bash
sudo apt install libsqlite3-dev
```

## Commands

```bash
./ideas add <text...> [--priority=low|medium|high] [--tags=a,b]
./ideas list [--status=S] [--priority=P] [--tag=T] [--sort=date|priority] [--all]
./ideas status <id> <new_status>
./ideas edit <id> <new text...>
./ideas delete <id> [--force]
./ideas search <keyword>
./ideas stats
./ideas export <csv|json|txt> [file]
./ideas import <csv|json> <file>
./ideas help [command]
```

Statuses: `new`, `in_progress`, `paused`, `completed`, `discarded`
Priorities: `low`, `medium`, `high`

## Examples

```bash
./ideas add build a CLI idea tracker --priority=high --tags=cli,c
./ideas list --all --sort=priority
./ideas status 1 in_progress
./ideas export json
```

## Where the database lives

`~/.ideas/ideas.db` — always in your home folder, no matter which directory you run the program from.

## Project files

```
main.c                 -> parses arguments, calls the right command
db.h / db.c             -> SQLite connection and schema migration
commands.h / commands.c -> logic for each command
colors.h / colors.c     -> terminal colors
Makefile                -> "make" to build, "make test" to test
tests/test.sh            -> automated tests
```

## Test

```bash
make test
```

## TODO

- [ ] Add color to `help` output (`print_usage` and `print_command_help` in `commands.c` are still plain text)
- [ ] Confirm colors display correctly in WSL (verified on Linux, still pending on the user's setup)
- [ ] Make sure `tests/test.sh` is placed inside a `tests/` folder before running `make test`
