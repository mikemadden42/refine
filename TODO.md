# TODO

Project-level improvements for `refine`. The `refine.c` code itself is
memory-safe with error paths handled; these are mostly tooling/packaging gaps.

## Worth doing

- [x] **Set a default CMake build type.** A bare `cmake -S . -B build` configures
      with no optimization and no `NDEBUG`, silently producing an unoptimized
      binary. Default to `Release` when the user doesn't specify one:
      ```cmake
      if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
          set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
      endif()
      ```
- [x] **Add a LICENSE file.** The README reads like a public project, but with no
      license it is legally "all rights reserved." Add one (MIT is typical for a
      tool like this) if it is meant to be shared.
- [x] **Add tests + CI.** Fine for a ~200-line tool today, but nothing guards
      against regressions. A small shell test (build a temp dir, diff expected
      output) plus a GitHub Actions workflow would lock in the hardened behavior.
      Done: `tests/run_tests.sh` (wired into CTest) + `.github/workflows/ci.yml`
      building and testing on Ubuntu and macOS.

## Minor / cosmetic

- [x] **Fix README example drift.** The example output lists a `Makefile`, but the
      project ships CMake, not a Makefile. Swap it for files that actually exist
      (`CMakeLists.txt`, `README.md`).
- [x] **Support `--` end-of-options separator.** A file literally named `-foo`
      cannot currently be passed as a path. Niche — the original C++ had the same
      limitation, so skip unless needed. Done: `--` ends option parsing; covered
      by tests and documented in the usage/help output.

## Non-issues (deliberate, matching the original C++)

- Non-recursive, regular-files-only, byte-wise (case-sensitive) sort — all
  faithful to the C++ source.
- Per-entry `malloc`/`free` of the full path — could reuse one growing buffer,
  but it is a micro-optimization that is not measurable here.
