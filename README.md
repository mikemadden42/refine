# refine

A small command-line tool that lists the regular files in a directory, grouped
and sorted by file extension.

## Building

Requires a C17 compiler and a POSIX system (uses `dirent.h` and `stat`).

### With CMake

```sh
cmake -S . -B build
cmake --build build
```

The resulting binary is `build/refine`.

### Directly

```sh
gcc -std=c17 -Wall -Wextra -o refine refine.c
```

## Usage

```
refine [-a|--all] [-h|--help] [path]
```

| Option | Description |
| ------ | ----------- |
| `-a`, `--all`  | Include hidden files (those starting with `.`). |
| `-h`, `--help` | Show the help message and exit. |
| `path`         | Directory to inspect (default: current directory). |

## Example

```
$ refine
No Extension:
- refine

c:
- refine.c

md:
- README.md
- TODO.md

txt:
- CMakeLists.txt
```

Files with no extension are listed under `No Extension`. Within each group,
names are sorted alphabetically, and the groups themselves are sorted by
extension.

## Exit status

| Code | Meaning |
| ---- | ------- |
| `0`  | Success (or help displayed). |
| `1`  | The path does not exist or is not a directory. |
| `2`  | Invalid command-line arguments. |

## License

Released under the [MIT License](LICENSE).
