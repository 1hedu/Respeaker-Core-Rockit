/**
 * Patch Save/Recall System - ReSpeaker Port
 *
 * Based on original Rockit save_recall.c (EEPROM-based)
 * Adapted for filesystem storage using simple text format
 */

#include "patch_storage.h"
#include "params.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>

// Get patch file path
static void get_patch_path(uint8_t patch_number, char *path_buf, size_t buf_size) {
    snprintf(path_buf, buf_size, "%s/patch_%02d.txt", PATCH_DIR, patch_number);
}

/**
 * Initialize patch storage system
 */
void patch_storage_init(void) {
    // Create patch directory if it doesn't exist
    struct stat st = {0};
    if (stat(PATCH_DIR, &st) == -1) {
        mkdir(PATCH_DIR, 0755);
    }
}

/**
 * Save current synth state to a patch slot
 */
int patch_save(uint8_t patch_number) {
    if (patch_number >= MAX_PATCHES) {
        fprintf(stderr, "patch_save: Invalid patch number %d\n", patch_number);
        return -1;
    }

    char path[256];
    get_patch_path(patch_number, path, sizeof(path));

    FILE *fp = fopen(path, "w");
    if (!fp) {
        fprintf(stderr, "patch_save: Failed to open %s: %s\n", path, strerror(errno));
        return -1;
    }

    // Write header
    fprintf(fp, "# Rockit Patch %02d\n", patch_number);
    fprintf(fp, "# Saved from ReSpeaker Core v1.0 Port\n");
    fprintf(fp, "# Format: param_name=value\n\n");

    // Save all parameters
    for (int i = 0; i < P_COUNT; i++) {
        int16_t value = params_get((param_id_t)i);
        const char *name = PARAM_SPECS[i].name;
        fprintf(fp, "%s=%d\n", name, value);
    }

    fclose(fp);
    fprintf(stderr, "Saved patch %d to %s\n", patch_number, path);
    return 0;
}

/**
 * Recall a patch from a slot
 */
int patch_recall(uint8_t patch_number) {
    if (patch_number >= MAX_PATCHES) {
        fprintf(stderr, "patch_recall: Invalid patch number %d\n", patch_number);
        return -1;
    }

    char path[256];
    get_patch_path(patch_number, path, sizeof(path));

    FILE *fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "patch_recall: Patch %d does not exist\n", patch_number);
        return -1;
    }

    char line[256];
    int params_loaded = 0;

    while (fgets(line, sizeof(line), fp)) {
        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') {
            continue;
        }

        // Parse "name=value" format
        char *eq = strchr(line, '=');
        if (!eq) continue;

        *eq = '\0';  // Split string at '='
        const char *name = line;
        int value = atoi(eq + 1);

        // Find matching parameter
        for (int i = 0; i < P_COUNT; i++) {
            if (strcmp(PARAM_SPECS[i].name, name) == 0) {
                params_set((param_id_t)i, (int16_t)value);
                params_loaded++;
                break;
            }
        }
    }

    fclose(fp);

    if (params_loaded == 0) {
        fprintf(stderr, "patch_recall: No parameters loaded from patch %d\n", patch_number);
        return -1;
    }

    fprintf(stderr, "Recalled patch %d (%d parameters loaded)\n", patch_number, params_loaded);
    return 0;
}

/**
 * Check if a patch exists
 */
int patch_exists(uint8_t patch_number) {
    if (patch_number >= MAX_PATCHES) {
        return 0;
    }

    char path[256];
    get_patch_path(patch_number, path, sizeof(path));

    return (access(path, F_OK) == 0) ? 1 : 0;
}

/**
 * Delete a patch
 */
int patch_delete(uint8_t patch_number) {
    if (patch_number >= MAX_PATCHES) {
        fprintf(stderr, "patch_delete: Invalid patch number %d\n", patch_number);
        return -1;
    }

    char path[256];
    get_patch_path(patch_number, path, sizeof(path));

    if (unlink(path) == 0) {
        fprintf(stderr, "Deleted patch %d\n", patch_number);
        return 0;
    } else {
        fprintf(stderr, "patch_delete: Failed to delete %s: %s\n", path, strerror(errno));
        return -1;
    }
}
