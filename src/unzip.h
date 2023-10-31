#ifndef SRC_UNZIP_H_
#define SRC_UNZIP_H_

#define _GNU_SOURCE

#include <ctype.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <zlib.h>

#include <err.h>
#include <errno.h>

#define USAGE "Usage: unzip source_archive.zip [options] [output]\n"

#define DEFAULT_CHUNK_SIZE  (2U << 12)

// config for read parameters
typedef struct {
  int hFlag;
  int fFlag;
} OptionsConfig;

// error types
typedef enum {
  ERROR         = -1,
  OK            =  0
} eErrorCode;

int         unzip        (const char* archive_filename, const char* filename, OptionsConfig config);
int         scanOptions  (int argc, char** argv , OptionsConfig* config);

#endif  // SRC_UNZIP_H_