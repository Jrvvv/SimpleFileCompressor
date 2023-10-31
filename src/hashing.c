#include "hashing.h"

int calculate_file_hash(const char* filename, unsigned char hash[SHA256_DIGEST_LENGTH]) {
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Error opening file");
        return -1;
    }

    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (mdctx == NULL) {
        perror("Error creating context");
        fclose(file);
        return -1;
    }

    if (EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL) != 1) {
        perror("Error initializing digest");
        EVP_MD_CTX_free(mdctx);
        fclose(file);
        return -1;
    }

    unsigned char buffer[2 << 13];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) != 0) {
        if (EVP_DigestUpdate(mdctx, buffer, bytesRead) != 1) {
            perror("Error updating digest");
            EVP_MD_CTX_free(mdctx);
            fclose(file);
            return -1;
        }
    }

    unsigned int digestLength;
    if (EVP_DigestFinal_ex(mdctx, hash, &digestLength) != 1) {
        perror("Error finalizing digest");
        EVP_MD_CTX_free(mdctx);
        fclose(file);
        return -1;
    }

    EVP_MD_CTX_free(mdctx);
    fclose(file);

    return 0;
}
