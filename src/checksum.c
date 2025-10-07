#include "checksum.h"
#include "logger.h"
#include <stdio.h>
#include <string.h>
#include <openssl/sha.h>

int calculate_sha256(const char *filename, char *output_hash) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        log_error("Failed to open file for checksum: %s", filename);
        return -1;
    }

    SHA256_CTX sha256;
    SHA256_Init(&sha256);

    unsigned char buffer[8192];
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        SHA256_Update(&sha256, buffer, bytes_read);
    }

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &sha256);

    fclose(file);

    // Convert hash to hex string
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(output_hash + (i * 2), "%02x", hash[i]);
    }
    output_hash[SHA256_DIGEST_LENGTH * 2] = '\0';

    return 0;
}

bool verify_sha256(const char *filename, const char *expected_hash) {
    if (!expected_hash || strlen(expected_hash) == 0) {
        log_info("No checksum provided for %s, skipping verification", filename);
        return true;
    }

    // Comparision for case insensitivity
    for (int i = 0; i < SHA256_DIGEST_LENGTH * 2; i++) {
        char c1 = calculated_hash[i];
        char c2  =expected_hash[i];
        if (c1 >= 'A' && c1 <= 'Z') c1 += 32;
        if (c2 >= 'A' && c2 <= 'Z') c2 += 32;
        if (c1 != c2) {
            log_error("Checksum mismatch for %s", filename);
            log_error("Expected: %s", expected_hash);
            log_error("Got: %s", calculated_hash);
            return false;
        }
    }

    log_info("Checksum verified for %s", filename);
    return true;
}