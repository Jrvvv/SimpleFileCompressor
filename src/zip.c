#include "zip.h"

#include "hashing.h"

static int isNumber(char* s);
static void printInfo();

int main(int argc, char** argv) {
  OptionsConfig config = {0};
  int retCode = (int)OK;

  // parsing args with getopt
  if (argc > 1) {
    int ind;
    if ((ind = scanOptions(argc, argv, &config)) != -1) {
      argv += ind;
      argc = argc - ind;
    } else {
      retCode = (int)ERROR;
    }
  } else {
    retCode = (int)ERROR;
  }

  if (config.hFlag) {
    printInfo();
  
  } else if (retCode == (int)OK) {
    // filename or filename + archive name
    if (argc >= 1 && argc <= 2) {
      const char* input_filename = argv[0];
      const char* archive_filename = (argv[1]) ? argv[1] : "archive.zip";
      if (!config.cFlag) {
        config.chunksNumber = DEFAULT_CHUNK_SIZE;
      }
      retCode = zip(input_filename, archive_filename, config);
    } else {
      fprintf(stderr, USAGE);
    }
  } else {
    fprintf(stderr, USAGE);
  }

  return retCode;
}

// parsing args with getopt
int scanOptions(int argc, char** argv, OptionsConfig* config) {
  int flag;
  int result = 0;
  while (((flag = getopt(argc, argv, "hfc:"))) != -1) {
    switch (flag) {
      case 'h':
        config->hFlag = 1;
        break;
      case 'f':
        config->fFlag = 1;
        break;
      case 'c':
        config->cFlag = 1;
        if (result == 0)
          result = (int)readFromC(optarg, config);
        break;
      default:
        result = -1;
    }

    if (config->hFlag == 1) {
      break;
    }
  }

  if (result >= 0) {
    result = optind;
  } else if (config->hFlag) {
    result = 0;
  }

  return result;
}

eErrorCode readFromC(char* optarg, OptionsConfig* config) {
  eErrorCode retCode = OK;
  int chunksNum = atoi(optarg);
  int isNum = isNumber(optarg);
  if (!chunksNum || !isNum || ((unsigned)chunksNum > MAX_CHUNK_SIZE) ||
      ((unsigned)chunksNum < MIN_CHUNK_SIZE)) {
    fprintf(stderr, "-c arg is number from 1 to 1024 * 1024 * 1024\n");
    retCode = ERROR;
  } else {
    config->chunksNumber = chunksNum;
    config->cFlag = 1;
  }

  return retCode;
}

// compression func
int zip(const char* filename, const char* archive_filename,
        OptionsConfig config) {
  // calc file hash at start
  unsigned char hash_start[SHA256_DIGEST_LENGTH];
  if (calculate_file_hash(filename, hash_start) == -1) {
    perror("Error hashing files");
    return (int)ERROR;
  }

  // open input file for reading
  int input_fd = open(filename, O_RDONLY);
  if (input_fd == -1) {
    perror("Error opening input file");
    return (int)ERROR;
  }

  // getting input file stats
  struct stat st;
  if (fstat(input_fd, &st) == -1) {
    perror("Error getting file size");
    close(input_fd);
    return (int)ERROR;
  }

  // open archive file for writing
  int archiveOverwrite = (config.fFlag)
                             ? (O_CREAT | O_WRONLY | O_TRUNC)
                             : (O_CREAT | O_WRONLY | O_TRUNC | O_EXCL);
  int archive_fd = open(archive_filename, archiveOverwrite, 0666);
  if (archive_fd == -1 || archive_fd == EEXIST) {
    if (archive_fd == EEXIST) {
      perror("Archive already exist, run with -f to overwrite");
    } else {
      perror("Error opening output file");
    }
    close(input_fd);
    return (int)ERROR;
  }

  // getting input file size
  off_t input_size = st.st_size;

  // check if file is not empty
  // if empty -- empty archive already written
  if (input_size != 0) {
    // mem mapping of input file
    void* input_data =
        mmap(NULL, input_size, PROT_READ, MAP_PRIVATE, input_fd, 0);
    if (input_data == MAP_FAILED) {
      perror("Error mapping input file to memory");
      close(input_fd);
      close(archive_fd);
      return (int)ERROR;
    }

    // struct init for compression
    z_stream stream;
    memset(&stream, 0, sizeof(stream));

    if (deflateInit(&stream, Z_DEFAULT_COMPRESSION) != Z_OK) {
      perror("Error initializing compression stream");
      munmap(input_data, input_size);
      close(input_fd);
      close(archive_fd);
      return (int)ERROR;
    }

    // passing input data ptr and its size to struct
    stream.avail_in = input_size;
    stream.next_in = (Bytef*)input_data;

    unsigned char out[config.chunksNumber];
    do {
      stream.avail_out = config.chunksNumber;
      stream.next_out = out;
      // input file compression and writing to archive
      if (deflate(&stream, Z_FINISH) == Z_STREAM_ERROR) {
        perror("Error compressing data");
        deflateEnd(&stream);
        munmap(input_data, input_size);
        close(input_fd);
        close(archive_fd);
        return (int)ERROR;
      }
      write(archive_fd, out, config.chunksNumber - stream.avail_out);
    } while (stream.avail_out == 0);

    // free
    munmap(input_data, input_size);
    deflateEnd(&stream);
  }

  // calc file hash at end
  unsigned char hash_end[SHA256_DIGEST_LENGTH];
  if (calculate_file_hash(filename, hash_end) == -1) {
    perror("Error while hashing files");
    close(input_fd);
    close(archive_fd);
    return (int)ERROR;
  }

  // checking hashes (file at start <-> file at end)
  if ((memcmp(hash_end, hash_start, SHA256_DIGEST_LENGTH) != 0)) {
    perror("Error: File content was modified during copying\n");
    close(input_fd);
    close(archive_fd);
    return (int)ERROR;
  }

  // free
  close(input_fd);
  close(archive_fd);
  return 0;
}

// -------------------------- static funcs --------------------------
int isNumber(char* s) {
  for (int i = 0; s[i] != '\0'; i++) {
    if (!isdigit(s[i])) {
      return 0;
    }
  }
  return 1;
}

void printInfo() {
  printf(
      "Usage: zip [options] filename [dest_archive.zip]\n"
      "Options:\n"
      "  -c CHUNKS_NUMBER\n"
      "    Chunks size in compression algorithm in bytes from 1 to 1024 * 1024 "
      "* 2 (2MB)\n"
      "  -f\n"
      "    Force overwriting of destonation file, if it's existing.");
}
