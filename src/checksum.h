#ifndef CHECKSUM_H
#define CHECKSUM_H

#include <openssl/sha.h>
#include <stdbool.h>

int calculate_sha256(const char *filename, char *output_hash);
bool verify_sha256(const char *filename, const char *expected_hash);

#endif