#define _POSIX_C_SOURCE 200809L

#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* A group of file names that share one extension. */
typedef struct {
    char *extension;   /* includes leading '.', or "" for none; owned */
    char **names;      /* owned array of owned strings */
    size_t count;
    size_t capacity;
} ExtensionGroup;

typedef struct {
    ExtensionGroup *groups;
    size_t count;
    size_t capacity;
} ExtensionMap;

static void *xrealloc(void *ptr, size_t size) {
    void *p = realloc(ptr, size);
    if (!p) {
        fprintf(stderr, "Error: out of memory\n");
        exit(1);
    }
    return p;
}

static char *xstrdup(const char *s) {
    size_t len = strlen(s) + 1;
    char *copy = xrealloc(NULL, len);
    memcpy(copy, s, len);
    return copy;
}

/* Find the group for an extension, creating it if absent. */
static ExtensionGroup *map_get(ExtensionMap *map, const char *extension) {
    for (size_t i = 0; i < map->count; ++i) {
        if (strcmp(map->groups[i].extension, extension) == 0) {
            return &map->groups[i];
        }
    }
    if (map->count == map->capacity) {
        map->capacity = map->capacity ? map->capacity * 2 : 8;
        map->groups = xrealloc(map->groups, map->capacity * sizeof(*map->groups));
    }
    ExtensionGroup *group = &map->groups[map->count++];
    group->extension = xstrdup(extension);
    group->names = NULL;
    group->count = 0;
    group->capacity = 0;
    return group;
}

static void group_add(ExtensionGroup *group, char *name) {
    if (group->count == group->capacity) {
        group->capacity = group->capacity ? group->capacity * 2 : 8;
        group->names = xrealloc(group->names, group->capacity * sizeof(*group->names));
    }
    group->names[group->count++] = name;
}

static void map_free(ExtensionMap *map) {
    for (size_t i = 0; i < map->count; ++i) {
        ExtensionGroup *group = &map->groups[i];
        for (size_t j = 0; j < group->count; ++j) {
            free(group->names[j]);
        }
        free(group->names);
        free(group->extension);
    }
    free(map->groups);
}

static int compare_strings(const void *a, const void *b) {
    const char *const *sa = a;
    const char *const *sb = b;
    return strcmp(*sa, *sb);
}

static int compare_groups(const void *a, const void *b) {
    const ExtensionGroup *ga = a;
    const ExtensionGroup *gb = b;
    return strcmp(ga->extension, gb->extension);
}

/* Return a pointer into name at the start of the extension (the last '.'),
   matching std::filesystem::path::extension semantics: a leading dot on an
   otherwise dotless name is not an extension. Returns "" if none. */
static const char *get_extension(const char *name) {
    const char *dot = strrchr(name, '.');
    if (!dot || dot == name) {
        return "";
    }
    return dot;
}

static void print_usage(const char *prog) {
    printf("Usage: %s [-a|--all] [-h|--help] [--] [path]\n", prog);
    printf("  -a, --all   Include hidden files (those starting with '.')\n");
    printf("  -h, --help  Show this help message\n");
    printf("  --          Treat the following argument as a path, even if it starts with '-'\n");
    printf("  path        Directory to inspect (default: current directory)\n");
}

int main(int argc, char *argv[]) {
    const char *path = ".";
    bool show_hidden = false;
    bool path_set = false;
    bool options_done = false;  /* set once "--" is seen */

    for (int i = 1; i < argc; ++i) {
        if (!options_done && strcmp(argv[i], "--") == 0) {
            options_done = true;
        } else if (!options_done &&
                   (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--all") == 0)) {
            show_hidden = true;
        } else if (!options_done &&
                   (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)) {
            print_usage(argv[0]);
            return 0;
        } else if (!options_done && argv[i][0] == '-') {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 2;
        } else if (!path_set) {
            path = argv[i];
            path_set = true;
        } else {
            fprintf(stderr, "Unexpected argument: %s\n", argv[i]);
            return 2;
        }
    }

    struct stat st;
    if (stat(path, &st) != 0) {
        fprintf(stderr, "Error: cannot access path: %s: %s\n", path, strerror(errno));
        return 1;
    }
    if (!S_ISDIR(st.st_mode)) {
        fprintf(stderr, "Error: not a directory: %s\n", path);
        return 1;
    }

    DIR *dir = opendir(path);
    if (!dir) {
        fprintf(stderr, "Error opening directory: %s: %s\n", path, strerror(errno));
        return 1;
    }

    ExtensionMap map = {0};
    size_t path_len = strlen(path);

    struct dirent *entry;
    errno = 0;
    while ((entry = readdir(dir)) != NULL) {
        const char *name = entry->d_name;
        if (!show_hidden && name[0] == '.') {
            continue;
        }

        /* Build "path/name" and stat it to keep only regular files. */
        size_t name_len = strlen(name);
        char *full = xrealloc(NULL, path_len + 1 + name_len + 1);
        memcpy(full, path, path_len);
        full[path_len] = '/';
        memcpy(full + path_len + 1, name, name_len + 1);

        struct stat entry_st;
        bool is_regular = stat(full, &entry_st) == 0 && S_ISREG(entry_st.st_mode);
        free(full);
        if (!is_regular) {
            continue;
        }

        ExtensionGroup *group = map_get(&map, get_extension(name));
        group_add(group, xstrdup(name));
        errno = 0;
    }
    if (errno != 0) {
        fprintf(stderr, "Error reading directory: %s: %s\n", path, strerror(errno));
        closedir(dir);
        map_free(&map);
        return 1;
    }
    closedir(dir);

    qsort(map.groups, map.count, sizeof(*map.groups), compare_groups);

    for (size_t i = 0; i < map.count; ++i) {
        ExtensionGroup *group = &map.groups[i];
        qsort(group->names, group->count, sizeof(*group->names), compare_strings);

        const char *display = group->extension[0] == '\0'
                                  ? "No Extension"
                                  : group->extension + 1;  /* drop leading '.' */
        printf("%s:\n", display);
        for (size_t j = 0; j < group->count; ++j) {
            printf("- %s\n", group->names[j]);
        }
        printf("\n");
    }

    map_free(&map);
    return 0;
}
