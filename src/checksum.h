#ifndef CHECKSUM_H
#define CHECKSUM_H

#include <stdbool.h>

int calculate_sha256(const chat *filename, char *output_hash);
bool verify_sha256(const char *filename, const char *expected_hash); // Verify file against expected SHA256 hash

#endif