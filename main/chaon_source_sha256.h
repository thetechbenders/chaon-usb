/* SPDX-License-Identifier: AGPL-3.0-only */
#ifndef CHAON_SOURCE_SHA256_H
#define CHAON_SOURCE_SHA256_H
#include <stddef.h>
#include <stdint.h>
enum { CHAON_SHA256_SIZE = 32 };
/* SHA-256 over EXACT raw source bytes (excluding the C NUL terminator).
 * This is a digest, not an execution authorization or freshness proof. */
void chaon_source_sha256(const uint8_t *data, size_t length,
                         uint8_t digest[CHAON_SHA256_SIZE]);
#endif
