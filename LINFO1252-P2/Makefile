# Makefile for lib_tar implementation

CFLAGS = -g -Wall -Werror
CC = gcc

# Default target
all: tests lib_tar.o lib_tar.a

# Build object file
lib_tar.o: lib_tar.c lib_tar.h
	$(CC) $(CFLAGS) -c lib_tar.c -o lib_tar.o

# Build static library
lib_tar.a: lib_tar.o
	ar rcs lib_tar.a lib_tar.o

# Build tests
tests: tests.c lib_tar.o
	$(CC) $(CFLAGS) tests.c lib_tar.o -o tests

# Clean target
clean:
	rm -f lib_tar.o lib_tar.a tests soumission.tar

# Submit target
submit: all
	tar --posix --pax-option delete=".*" --pax-option delete="*time*" --no-xattrs --no-acl --no-selinux -c *.h *.c Makefile > soumission.tar
