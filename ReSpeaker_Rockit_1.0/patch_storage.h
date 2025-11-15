#pragma once
#include <stdint.h>

/**
 * Patch Save/Recall System - ReSpeaker Port
 *
 * Based on original Rockit save_recall.c but adapted for filesystem instead of EEPROM.
 * Stores patches as JSON files in /tmp/rockit_patches/ directory.
 *
 * Original Rockit: EEPROM storage with multiple patch slots
 * ReSpeaker Port: JSON files for easy editing and cross-platform compatibility
 */

// Maximum number of patches
#define MAX_PATCHES 16

// Patch storage directory (persistent across reboots)
#define PATCH_DIR "/root/rockit_patches"

/**
 * Initialize patch storage system
 * Creates patch directory if it doesn't exist
 */
void patch_storage_init(void);

/**
 * Save current synth state to a patch slot
 *
 * @param patch_number Patch slot (0-15)
 * @return 0 on success, -1 on error
 */
int patch_save(uint8_t patch_number);

/**
 * Recall a patch from a slot
 *
 * @param patch_number Patch slot (0-15)
 * @return 0 on success, -1 on error (patch doesn't exist or is invalid)
 */
int patch_recall(uint8_t patch_number);

/**
 * Check if a patch exists
 *
 * @param patch_number Patch slot (0-15)
 * @return 1 if patch exists, 0 if empty
 */
int patch_exists(uint8_t patch_number);

/**
 * Delete a patch
 *
 * @param patch_number Patch slot (0-15)
 * @return 0 on success, -1 on error
 */
int patch_delete(uint8_t patch_number);
