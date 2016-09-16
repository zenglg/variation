#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>

#ifndef USERMESG
#define USERMESG	2048
#endif

#ifndef PAGE_SIZE
#define PAGE_SIZE	4096
#endif

#ifndef VARIATION
#define VARIATION	"./variation.c"
#endif

#ifndef PATH_MAX
#define PATH_MAX	1024
#endif

/* EXPAND_VAR_ARGS copy from ltp. */
/*
 * EXPAND_VAR_ARGS - Expand the variable portion (arg_fmt) of a result
 *                   message into the specified string.
 *
 * NOTE (garrcoop):  arg_fmt _must_ be the last element in each function
 *                   argument list that employs this.
 */
#define EXPAND_VAR_ARGS(buf, arg_fmt, buf_len) do {\
	va_list ap;				\
	assert(arg_fmt != NULL);		\
	va_start(ap, arg_fmt);			\
	vsnprintf(buf, buf_len, arg_fmt, ap);	\
	va_end(ap);				\
	assert(strlen(buf) > 0);		\
} while (0)

int read_fd;
int write_fd;

static int variation_error(void (*func)(void), const char *arg_fmt, ...)
{
	char tmesg[USERMESG];

	EXPAND_VAR_ARGS(tmesg, arg_fmt, USERMESG);

	printf("%s\n", tmesg);

	if (func != NULL)
		(*func)();

	exit(-1);
}

static int variation_open(void (cleanup_fn)(void), const char *pathname,
			  int flags, ...)
{
	va_list ap;
	int rval;
	mode_t mode;

	va_start(ap, flags);
	mode = va_arg(ap, mode_t);
	va_end(ap);

	rval = open(pathname, flags, mode);
	if (rval == -1) {
		variation_error(cleanup_fn,
				"%s:%d: open(%s,%d,0%o) failed",
				__FILE__, __LINE__, pathname, flags, mode);
	}

	return rval;
}

static int variation_close(void (cleanup_fn)(void), int fd)
{
	int rval;

	rval = close(fd);
	if (rval == -1) {
		variation_error(cleanup_fn,
				"%s:%d: close(%d) failed",
				__FILE__, __LINE__, fd);
	}

	return rval;
}

static int variation_read(void (cleanup_fn)(void), int fd, char *buf,
			  size_t nbyte)
{
	int rval = 0;

	rval = read(fd, buf, nbyte);
	if (rval < 0) {
		variation_error(cleanup_fn,
				"%s:%d: read(%d,%p,%zu) failed",
				__FILE__, __LINE__, fd, buf, nbyte);
	}

	return rval;
}

static int variation_write(void (cleanup_fn)(void), int fd, char *buf,
			   size_t nbyte)
{
	int rval = 0;

	rval = write(fd, buf, nbyte);
	if (rval < 0) {
		variation_error(cleanup_fn,
				"%s:%d: write(%d,%p,%zu) failed",
				__FILE__, __LINE__, fd, buf, nbyte);
	}

	return rval;
}

static int variation_compile(void (cleanup_fn)(void), const char *filename)
{
	char command[2*PATH_MAX];

	sprintf(command, "gcc -o %s %s.c 2> /dev/null", filename, filename);

	return system(command);
}

static void cleanup(void)
{
	if (read_fd > 0)
		close(read_fd);

	if (write_fd > 0)
		close(write_fd);
}

int main(void)
{
	int len;
	int index;
	char buf[PAGE_SIZE];

	read_fd = variation_open(cleanup, VARIATION, O_RDONLY);

	len = variation_read(cleanup, read_fd, buf, PAGE_SIZE);

	srandom((unsigned int)time(NULL));

	index = random() % len;

	while (1) {
		buf[index] = random() % 0xff;

		write_fd = variation_open(cleanup, "./tmp.c", O_WRONLY | O_CREAT, 0644);

		variation_write(cleanup, write_fd, buf, len);

		variation_close(cleanup, write_fd);

		if (variation_compile(cleanup, "tmp") == 0)
			break;
	}

	cleanup();

	return 0;
}
