#ifndef SRC_HASHING_H_
#define SRC_HASHING_H_

#include <stdio.h>
#include <string.h>
#include <openssl/evp.h>
#include <openssl/sha.h>

int calculate_file_hash(const char* filename, unsigned char hash[SHA256_DIGEST_LENGTH]);


#endif // SRC_HASHING_H_