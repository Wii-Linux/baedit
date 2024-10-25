#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdint.h>
#include <assert.h>

static const char startMarker[] = "mark.start=1";
static const char endMarker[]   = "mark.end=1";

#define ERR "\x1b[1;31mERROR: "
#define RESET "\x1b[0m"

static int fd = 0;
static uint8_t *kernel = NULL;
static uint8_t *argsStart = NULL;
static uint8_t *argsEnd = NULL;
static char *args = NULL;
static size_t fileSize = 0;

static void common(const char *file, const char *rdwr) {
	if (fd == -1) {
		// failed
		fprintf(stderr, ERR "Failed to open %s as %s: %s\r\n" RESET, file, rdwr, strerror(errno)); 
		exit(1);
	}

	// get file size
	struct stat st;
	stat(file, &st);

	fileSize = st.st_size;
	kernel = malloc(fileSize);
	if (!kernel) {
		fprintf(stderr, ERR "Failed to allocate %zu bytes of memory: %s\r\n" RESET, fileSize, strerror(errno));
		close(fd);
		exit(1);
	}
	ssize_t r = read(fd, kernel, fileSize);
	if (r == -1) {
		fprintf(stderr, ERR "Failed to read %s: %s\r\n" RESET, file, strerror(errno));
		close(fd);
		free(kernel);
		exit(1);
	}
	if ((size_t)r != fileSize) {
		fprintf(stderr, ERR "Tried to read %zu bytes from %s, but only got %zd.\r\n" RESET, fileSize, file, r);
		close(fd);
		free(kernel);
		exit(1);
	}

	// read successfully
	printf("OK: read %zu bytes\r\n", fileSize);

	// find start marker
	for (size_t i = 0; i != fileSize; i++) {
		if (kernel[i] != (uint8_t)'m') continue;

		// got an m
		if (argsStart == NULL) {
			if (i + (sizeof(startMarker) - 1) > fileSize) {
				fprintf(stderr, ERR "Reading start marker would put us past end of file.\r\n" RESET);
				exit(1);
			}

			if (memcmp(&kernel[i], startMarker, sizeof(startMarker) - 1) != 0) {
				// not it
				continue;
			}

			// we got the start marker!
			i += sizeof(startMarker);
			argsStart = &kernel[i];
		}
		if (argsEnd == NULL) {
			if (i + (sizeof(endMarker) - 1) > fileSize) {
				fprintf(stderr, ERR "Reading end marker would put us past end of file.\r\n" RESET);
				exit(1);
			}

			if (memcmp(&kernel[i], endMarker, sizeof(endMarker) - 1) != 0) {
				// not it
				continue;
			}

			// we got the end marker!
			argsEnd = &kernel[i];
			i += sizeof(endMarker);
		}
	}
	if (argsStart == NULL) {
		fprintf(stderr, ERR "Failed to find start marker \"%s\"\r\n" RESET, startMarker);
		close(fd);
		free(kernel);
		exit(1);
	}
	if (argsEnd == NULL) {
		fprintf(stderr, ERR "Failed to find end marker \"%s\"\r\n" RESET, endMarker);
		close(fd);
		free(kernel);
		exit(1);
	}
	puts("Found both start and end marker");
	args = malloc((argsEnd - argsStart) + 8);
	memset(args, 0, (argsEnd - argsStart) + 8);
	if (args == NULL) {
		fprintf(stderr, ERR "Failed to allocated %zu bytes to hold arguments: %s\r\n" RESET, argsEnd - argsStart, strerror(errno));
		close(fd);
		free(kernel);
		exit(1);
	}
	printf("argsStart: %p, argsEnd = %p\r\n", argsStart, argsEnd);
	memcpy(args, argsStart, argsEnd - argsStart);
	
}

static void doPrint(const char *file) {
	fd = open(file, O_RDONLY);
	common(file, "read-only");
	printf("Current kernel cmdline: %s\r\n", args);
}

static void doReplace(const char *file, const char *newArgs) {
	assert(file != NULL);
	assert(newArgs != NULL);

	fd = open(file, O_RDWR);
	common(file, "read-write");
	printf("Current kernel cmdline: %s\r\n", args);
	printf("New kernel cmdline: %s\r\n", newArgs);

	memset(argsStart, ' ', argsEnd - argsStart);
	*argsEnd ='\0';
	
	// copy without NULL terminator
	printf("New kernel cmdline: %s (%zu)\r\n", newArgs, strlen(newArgs));
	printf("In kernel: %s\r\n", argsStart);
	memcpy(argsStart, newArgs, strlen(newArgs));
	printf("In kernel: %s\r\n", argsStart);

	ssize_t wrote = write(fd, kernel, fileSize);
	if (wrote == -1) {
		fprintf(stderr, ERR "Failed to write %zu bytes to %s: %s\r\n" RESET, fileSize, file, strerror(errno));
		close(fd);
		free(kernel);
		free(args);
		exit(1);
	}
	if ((size_t)wrote != fileSize) {
		fprintf(stderr, ERR "Tried to write %zu bytes to %s, but only wrote %zd" RESET, fileSize, file, wrote);
		close(fd);
		free(kernel);
		free(args);
		exit(1);
	}

	printf("OK: %zu bytes written to %s\r\n", fileSize, file);
}

static void usage(const char *progName) {
	printf(
"Usage: %s [kernel] <'new cmdline'>\r\n\
\r\n\
Example, to print the current cmdline: %s v4_5_0.krn\r\n\
Example, to replace the cmdline:       %s v4_5_0.krn 'root=/dev/sda1 video=gcnfb:tv=auto,nostalgic rootwait consolewait loglevel=4\r\n", progName, progName, progName);
}

int main(int argc, char *argv[]) {
	switch (argc) {
		case 2: doPrint(argv[1]); break;
		case 3: doReplace(argv[1], argv[2]); break;
		default: usage(argv[0]); return 1;
	}
	// cleanup
	close(fd);
	free(kernel);
	free(args);

	return 0;
}
