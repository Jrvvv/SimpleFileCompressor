#ifndef SRC_ZIP_H_
#define SRC_ZIP_H_

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

#define USAGE "Usage: zip [options] filename [archive.zip]\n"\
              "  Run with -h for more info\n"

#define MB                  (1024U * 1024U)

#define MAX_CHUNK_SIZE      (MB * 2U)
#define MIN_CHUNK_SIZE      (1U)
#define DEFAULT_CHUNK_SIZE  (2U << 12)

// config for read parameters
typedef struct {
  int hFlag;
  int fFlag;        // force writing to output (error if not set)
  int cFlag;
  int chunksNumber;
} OptionsConfig;

// error types
typedef enum {
  ERROR         = -1,
  OK            =  0
} eErrorCode;

int         zip          (const char* filename, const char* archive_filename, OptionsConfig config);
int         scanOptions  (int argc, char** argv , OptionsConfig* config);
eErrorCode  readFromC    (char* optarg, OptionsConfig* config);

#endif  // SRC_ZIP_H_