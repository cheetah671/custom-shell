# custom-shell

`custom-shell` is a small interactive shell written in C. It is designed to behave like a practical POSIX-style shell while keeping the implementation focused on the core operating-systems features covered in the project: parsing, process creation, job control, redirection, signals, and command history.

## Overview

The shell reads a command line, parses it into command groups, and then executes each group either in the foreground or in the background. It supports:

- command pipelines with `|`
- sequential execution with `;`
- background jobs with `&`
- input and output redirection with `<`, `>`, and `>>`
- built-in commands for navigation, process control, and history
- signal handling for `Ctrl-C` and `Ctrl-Z`

The prompt shows the current user, host, and working directory. The home directory is shortened to `~` in the prompt, which makes long paths easier to read during interactive use.

## Build And Run

The project builds with the provided `Makefile`.

```bash
make
```

This creates the executable `shell.out` in the project root.

Useful targets:

```bash
make run
make clean
```

`make run` starts the shell after building, and `make clean` removes the object files and the executable.

## Usage

Start the shell and enter commands exactly as you would in a terminal:

```bash
./shell.out
```

Examples:

```bash
ls -l
cat input.txt | grep error > output.txt
sleep 10 &
echo first; echo second; echo third
```

When the shell receives EOF, it exits gracefully and prints `logout`.

## Built-In Commands

### `hop`

```bash
hop [path]
```

Changes the current working directory. Supported paths include `~`, `.`, `..`, `-`, and any relative or absolute path. If no path is provided, `hop` behaves like `hop ~`.

The shell also remembers the previous directory, so `hop -` switches back to the last location when one exists.

### `reveal`

```bash
reveal [-a] [-l] [path]
```

Lists directory contents for the given path or for the current directory if no path is supplied.

- `-a` includes hidden files
- `-l` prints one entry per line instead of a single line

Entries are sorted alphabetically before being displayed.

### `activities`

```bash
activities
```

Prints the shell’s tracked jobs and their current state. This is useful for checking which processes are running or stopped in the background.

### `ping`

```bash
ping <pid> <signal>
```

Sends a signal to a process by PID. The implementation applies `signal % 32` before delivery, which matches the behavior expected by the project code.

### `fg` and `bg`

```bash
fg [job_number]
bg [job_number]
```

`fg` brings a background or stopped job to the foreground and waits for it to finish or stop again. `bg` resumes a stopped job in the background. If a job number is not provided, the most recent tracked job is used.

### `log`

```bash
log
log purge
log execute <index>
```

Shows the stored command history, clears it, or re-executes an entry from history.

- `log` prints commands from oldest to newest
- `log purge` clears the history file and in-memory history
- `log execute <index>` replays a command using reverse indexing, where `1` is the most recent command

History is capped at 15 commands and is persisted in `.shell_history` next to the executable.

## Behavior Notes

- `Ctrl-C` sends `SIGINT` to the foreground process group
- `Ctrl-Z` sends `SIGTSTP` to the foreground process group
- Background jobs are tracked so the shell can report their status and clean them up on exit
- The shell ignores empty lines
- The parser supports command groups and pipelines before execution

## Project Layout

```text
custom-shell/
├── Makefile
├── README.md
├── include/
└── src/
```

The code is split into small modules for prompts, parsing, built-ins, logging, process control, and the main shell loop.

## Troubleshooting

- If `shell.out` is not executable, run `chmod +x shell.out`
- If build artifacts are stale, run `make clean && make`
- If a directory change fails, the shell prints `No such directory!`
- If a job number is invalid, `fg` and `bg` print `No such job`

## Summary

This shell demonstrates the key pieces of a command-line interpreter: input parsing, process management, redirection, history, and signal-driven job control. It is intentionally compact, but it covers the core behaviors expected from an educational Unix shell implementation.
