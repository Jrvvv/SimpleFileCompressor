#include "unzip.h"

#include "hashing.h"

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
      const char* archive_filename = argv[0];
      const char* output_filename = (argv[1]) ? argv[1] : "output";
      retCode = unzip(archive_filename, output_filename, config);
    } else {
      if (!config.hFlag) fprintf(stderr, USAGE);
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
  while (((flag = getopt(argc, argv, "hf"))) != -1) {
    switch (flag) {
      case 'h':
        config->hFlag = 1;
        break;
      case 'f':
        config->fFlag = 1;
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

// decompression func
int unzip(const char* archive_filename, const char* filename,
          OptionsConfig config) {
  // calc archive hash at start
  unsigned char hash_start[SHA256_DIGEST_LENGTH];
  if (calculate_file_hash(archive_filename, hash_start) == -1) {
    perror("Error while hashing files");
    return (int)ERROR;
  }

  // open archive file for reading
  int archive_fd = open(archive_filename, O_RDONLY);
  if (archive_fd == -1) {
    perror("Error opening input file");
    return (int)ERROR;
  }

  // getting input file stats
  struct stat st;
  if (fstat(archive_fd, &st) == -1) {
    perror("Error getting file size");
    close(archive_fd);
    return (int)ERROR;
  }

  // open output file for writing
  int fileOverwrite = (config.fFlag) ? (O_CREAT | O_WRONLY | O_TRUNC)
                                     : (O_CREAT | O_WRONLY | O_TRUNC | O_EXCL);
  int output_fd = open(filename, fileOverwrite, 0666);
  if (output_fd == -1 || output_fd == EEXIST) {
    if (output_fd == EEXIST) {
      perror("Archive already exist, run with -f to overwrite");
    } else {
      perror("Error opening output file");
    }
    close(archive_fd);
    return (int)ERROR;
  }

  // getting archive file size
  off_t archive_size = st.st_size;

  // check if archive is not empty
  // if empty -- empty output file already written
  if (archive_size) {
    // mem mapping of archive file
    void* archive_data =
        mmap(NULL, archive_size, PROT_READ, MAP_PRIVATE, archive_fd, 0);
    if (archive_data == MAP_FAILED) {
      perror("Error mapping input file to memory");
      close(archive_fd);
      close(output_fd);
      return (int)ERROR;
    }

    // struct init for decompression
    z_stream stream;
    memset(&stream, 0, sizeof(stream));

    if (inflateInit(&stream) != Z_OK) {
      perror("Error initializing decompression stream");
      munmap(archive_data, archive_size);
      close(archive_fd);
      close(output_fd);
      return (int)ERROR;
    }

    // translating archive data ptr and its size to struct
    stream.avail_in = archive_size;
    stream.next_in = (Bytef*)archive_data;

    unsigned char out[DEFAULT_CHUNK_SIZE];
    int ret;
    do {
      stream.avail_out = DEFAULT_CHUNK_SIZE;
      stream.next_out = out;
      // archive decompression and writing to output file
      ret = inflate(&stream, Z_NO_FLUSH);
      if (ret == Z_STREAM_ERROR) {
        perror("Error decompressing data");
        inflateEnd(&stream);
        munmap(archive_data, archive_size);
        close(archive_fd);
        close(output_fd);
        return (int)ERROR;
      }
      write(output_fd, out, DEFAULT_CHUNK_SIZE - stream.avail_out);
    } while (ret != Z_STREAM_END);

    // free
    inflateEnd(&stream);
    munmap(archive_data, archive_size);
  }

  // calc file hash at end
  unsigned char hash_end[SHA256_DIGEST_LENGTH];
  if (calculate_file_hash(archive_filename, hash_end) == -1) {
    perror("Error while hashing files");
    close(archive_fd);
    close(output_fd);
    return (int)ERROR;
  }

  // checking hashes (archive at start <-> archive at end)
  if ((memcmp(hash_end, hash_start, SHA256_DIGEST_LENGTH) != 0)) {
    perror("Error: File content was modified during copying");
    close(archive_fd);
    close(output_fd);
    return (int)ERROR;
  }

  // free
  close(archive_fd);
  close(output_fd);
  return 0;
}

// -------------------------- static funcs --------------------------
void printInfo() {
  printf(
      "Usage: unzip source_archive.zip [options] [output]\n"
      "Options:\n"
      "  -f\n"
      "    Force overwriting of destonation file, if it's existing.");
}