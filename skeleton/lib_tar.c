#include "lib_tar.h"
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#define BLOCK_SIZE 512

/**************** Auxiliary functions  *****************/

/**
 * Computes the checksum of a tar header.
 * This is the sum of all bytes in the header, treating the `chksum` field as spaces.
 */

unsigned int compute_checksum(const tar_header_t *header) {
    unsigned int sum = 0;
    const unsigned char *bytes = (const unsigned char *)header;

    for (size_t i = 0; i < sizeof(tar_header_t); ++i) {
        if (i >= 148 && i < 156) {
            sum += ' ';
        } else {
            sum += bytes[i];
        }
    }
    return sum;
}

/**
 * Reads the next tar header from the archive.
 *
 * @param tar_fd The file descriptor for the tar archive.
 * @param header A pointer to store the read header.
 * @return 0 on success, -1 on EOF, -2 on read error.
 */
int read_next_header(int tar_fd, tar_header_t *header) {
    ssize_t bytes_read = read(tar_fd, header, BLOCK_SIZE);
    if (bytes_read == 0) {
        return -1; // EOF
    } else if (bytes_read < 0) {
        return -2; // Read error
    } else if (bytes_read != BLOCK_SIZE || header->name[0] == '\0') {
        return -1; // End of archive or incomplete block
    }
    return 0;
}

/********* End of auxiliary functions ****************/

/**
 * Checks whether the archive is valid.
 *
 * Each non-null header of a valid archive has:
 *  - a magic value of "ustar" and a null,
 *  - a version value of "00" and no null,
 *  - a correct checksum
 *
 * @param tar_fd A file descriptor pointing to the start of a file supposed to contain a tar archive.
 *
 * @return a zero or positive value if the archive is valid, representing the number of non-null headers in the archive,
 *         -1 if the archive contains a header with an invalid magic value,
 *         -2 if the archive contains a header with an invalid version value,
 *         -3 if the archive contains a header with an invalid checksum value
 */
int check_archive(int tar_fd) {
    tar_header_t header;
    int header_count = 0;

    while (1) {
        int status = read_next_header(tar_fd, &header);
        if (status == -1) {
            break; // EOF
        } else if (status == -2) {
            return -3; // Read error
        }

        // Check magic and version
        if (strncmp(header.magic, TMAGIC, TMAGLEN) != 0) {
            return -1;
        }
        if (strncmp(header.version, TVERSION, TVERSLEN) != 0) {
            return -2;
        }

        // Validate checksum
        unsigned int expected_chksum = TAR_INT(header.chksum);
        unsigned int actual_chksum = compute_checksum(&header);
        if (expected_chksum != actual_chksum) {
            return -3;
        }

        header_count++;
        // Skip the data block of a regular file
        if (header.typeflag == REGTYPE || header.typeflag == AREGTYPE) {
            size_t file_size = TAR_INT(header.size);
            size_t blocks_to_skip = (file_size + BLOCK_SIZE - 1) / BLOCK_SIZE;
            lseek(tar_fd, blocks_to_skip * BLOCK_SIZE, SEEK_CUR);
        }
    }
    return header_count;
}

/**
 * Checks whether an entry exists in the archive.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive.
 *
 * @return zero if no entry at the given path exists in the archive,
 *         any other value otherwise.
 */
int exists(int tar_fd, char *path) {
    lseek(tar_fd, 0, SEEK_SET);   // start at beginning of archive

    int null_block_count = 0;
    char null_block[512];
    memset(null_block, 0, 512);
    tar_header_t current_header;

    while (null_block_count < 2) {   // until end of archive
        if (read(tar_fd, &current_header, 512) == -1) {   // store header content in strucutre
            return EOF;
        }
        if (memcmp(&current_header, null_block, 512) == 0) {   // null block
            null_block_count ++;
            continue;
        } else {   // non-null block
            null_block_count = 0;
        }
        if (strcmp(path, current_header.name) == 0) {
            switch(current_header.typeflag) {   // return different values based on the type of the file described by the header (to be reused)
                case REGTYPE:
                case AREGTYPE:
                    return 1;
                case DIRTYPE:
                    return 2;
                case LNKTYPE:
                    return 3;
                case SYMTYPE:
                    return 4;
            }
            break;
        }
        next_header(tar_fd, current_header);
    }
    return 0;
}

/**
 * Checks whether an entry exists in the archive and is a directory.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive.
 *
 * @return zero if no entry at the given path exists in the archive or the entry is not a directory,
 *         any other value otherwise.
 */
int is_dir(int tar_fd, char *path) {
    return (exists(tar_fd, path) ==2); // checks if value is correct it'll return 1 if dir type
}

/**
 * Checks whether an entry exists in the archive and is a file.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive.
 *
 * @return zero if no entry at the given path exists in the archive or the entry is not a file,
 *         any other value otherwise.
 */
int is_file(int tar_fd, char *path) {
    return(exists(tar_fd, path) == 1); // checks return value returns 1 if it's file type
}

/**
 * Checks w
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive.
 * @return zero if no entry at the given path exists in the archive or the entry is not symlink,
 *         any other value otherwise.
 */
int is_symlink(int tar_fd, char *path) {
  return (exists(tar_fd, path)==4); // checks return value returns 1 if it's symlink type
}

/**
 * Lists the entries at a given path in the archive.
 * list() does not recurse into the directories listed at the given path.
 *
 * Example:
 *  dir/          list(..., "dir/", ...) lists "dir/a", "dir/b", "dir/c/" and "dir/e/"
 *   ├── a
 *   ├── b
 *   ├── c/
 *   │   └── d
 *   └── e/
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive. If the entry is a symlink, it must be resolved to its linked-to entry.
 * @param entries An array of char arrays, each one is long enough to contain a tar entry path.
 * @param no_entries An in-out argument.
 *                   The caller set it to the number of entries in `entries`.
 *                   The callee set it to the number of entries listed.
 *
 * @return zero if no directory at the given path exists in the archive,
 *         any other value otherwise.
 */
int list(int tar_fd, char *path, char **entries, size_t *no_entries) {
    tar_header_t header;
    size_t count = 0;

    while (1) {
        int status = read_next_header(tar_fd, &header);
        if (status == -1) {
            break; // EOF
        } else if (status == -2) {
            return 0; // Read error
        }

        if (strncmp(header.name, path, strlen(path)) == 0) {
            if (count < *no_entries) {
                strncpy(entries[count], header.name, 100);
                entries[count][99] = '\0'; // Ensure null termination
                count++;
            }
        }
    }
    *no_entries = count;
    return count > 0 ? 1 : 0;
}

/**
 * Reads a file at a given path in the archive.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive to read from.  If the entry is a symlink, it must be resolved to its linked-to entry.
 * @param offset An offset in the file from which to start reading from, zero indicates the start of the file.
 * @param dest A destination buffer to read the given file into.
 * @param len An in-out argument.
 *            The caller set it to the size of dest.
 *            The callee set it to the number of bytes written to dest.
 *
 * @return -1 if no entry at the given path exists in the archive or the entry is not a file,
 *         -2 if the offset is outside the file total length,
 *         zero if the file was read in its entirety into the destination buffer,
 *         a positive value if the file was partially read, representing the remaining bytes left to be read to reach
 *         the end of the file.
 *
 */
ssize_t read_file(int tar_fd, char *path, size_t offset, uint8_t *dest, size_t *len) {
    tar_header_t header;

    while (1) {
        int status = read_next_header(tar_fd, &header);
        if (status == -1) {
            break; // EOF
        } else if (status == -2) {
            return -1; // Read error
        }

        if (strcmp(header.name, path) == 0 &&
            (header.typeflag == REGTYPE || header.typeflag == AREGTYPE)) {
            size_t file_size = TAR_INT(header.size);
            if (offset >= file_size) {
                return -2; // Offset out of bounds
            }

            size_t to_read = file_size - offset;
            if (to_read > *len) {
                to_read = *len;
            }

            lseek(tar_fd, offset, SEEK_CUR);
            ssize_t bytes_read = read(tar_fd, dest, to_read);

            *len = bytes_read;
            return file_size - offset - bytes_read;
        }
    }
    return -1; // Not found or not a file
}
