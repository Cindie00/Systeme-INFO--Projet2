#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "lib_tar.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>



/**
 * You are free to use this file to write tests for your implementation
 */

void debug_dump(const uint8_t *bytes, size_t len) {
    for (int i = 0; i < len;) {
        printf("%04x:  ", (int) i);

        for (int j = 0; j < 16 && i + j < len; j++) {
            printf("%02x ", bytes[i + j]);
        }
        printf("\t");
        for (int j = 0; j < 16 && i < len; j++, i++) {
            printf("%c ", bytes[i]);
        }
        printf("\n");
    }
}

int main() {
    printf("Testing lib_tar functions:\n");

    test_check_archive();
    test_exists();
    test_is_file();
    test_is_dir();
    test_is_symlink();
    test_read_file();
    test_list();
    return 0;
}


void test_check_archive() {
    int fd = open("test.tar", O_RDONLY);
    if (fd < 0) {
        perror("Failed to open test.tar");
        exit(EXIT_FAILURE);
    }

    int result = check_archive(fd);
    printf("check_archive result: %d\n", result);

    close(fd);
}

void test_exists() {
    int fd = open("test.tar", O_RDONLY);
    if (fd < 0) {
        perror("Failed to open test.tar");
        exit(EXIT_FAILURE);
    }

    int result = exists(fd, "testfile.txt");
    printf("exists result: %d\n", result);

    close(fd);
}

void test_is_file() {
    int fd = open("test.tar", O_RDONLY);
    if (fd < 0) {
        perror("Failed to open test.tar");
        exit(EXIT_FAILURE);
    }

    int result = is_file(fd, "testfile.txt");
    printf("is_file result: %d\n", result);

    close(fd);
}

void test_is_dir() {
    int fd = open("test.tar", O_RDONLY);
    if (fd < 0) {
        perror("Failed to open test.tar");
        exit(EXIT_FAILURE);
    }

    int result = is_dir(fd, "testdir/");
    printf("is_dir result: %d\n", result);

    close(fd);
}

void test_is_symlink() {
    int fd = open("test.tar", O_RDONLY);
    if (fd < 0) {
        perror("Failed to open test.tar");
        exit(EXIT_FAILURE);
    }

    int result = is_symlink(fd, "linkfile");
    printf("is_symlink result: %d\n", result);

    close(fd);
}

void test_read_file() {
    int fd = open("test.tar", O_RDONLY);
    if (fd < 0) {
        perror("Failed to open test.tar");
        exit(EXIT_FAILURE);
    }

    uint8_t buffer[512];
    size_t len = sizeof(buffer);
    ssize_t result = read_file(fd, "testfile.txt", 0, buffer, &len);

    printf("read_file result: %ld, bytes read: %zu\n", result, len);
    printf("Content: %.*s\n", (int)len, buffer);

    close(fd);
}

void test_list() {
    int fd = open("test.tar", O_RDONLY);
    if (fd < 0) {
        perror("Failed to open test.tar");
        exit(EXIT_FAILURE);
    }

    char *entries[10];
    for (int i = 0; i < 10; i++) {
        entries[i] = malloc(100);
    }
    size_t no_entries = 10;

    int result = list(fd, "testdir/", entries, &no_entries);
    printf("list result: %d, entries listed: %zu\n", result, no_entries);
    for (size_t i = 0; i < no_entries; i++) {
        printf("Entry: %s\n", entries[i]);
        free(entries[i]);
    }

    close(fd);
}

