CC = gcc
CFLAGS = -Wall -Werror -Wextra -std=c11
LIBS = -lz -lssl -lcrypto

SRCS_ZIP := src/zip.c src/hashing.c
SRCS_UNZIP := src/unzip.c src/hashing.c


OS := $(shell uname)
ifeq ($(OS),Linux) # определяем ОС
TEST_LIBS = -lcheck -lm -lsubunit
else
TEST_LIBS = -lcheck
endif

all : clean zip unzip test

zip :
	$(CC) $(CFLAGS) $(SRCS_ZIP) -o build/$@ $(LIBS)

unzip :
	$(CC) $(CFLAGS) $(SRCS_UNZIP) -o build/$@ $(LIBS)

test: zip unzip
	sh test.sh

cppcheck:
	cppcheck --enable=all --suppress=missingIncludeSystem src/*.h src/*.c

clean :
	rm -rf build/* tests/files/*.zip tests/files/*.unzip

rebuild : clean all

style :
	clang-format -i --style=Google src/*.c src/*.h
	clang-format -n --style=Google src/*.c src/*.h