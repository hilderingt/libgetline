#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <stdio.h>
#include <errno.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "libgetline.h"

#define MSB(t, n)  ((t)1 << sizeof(t) * CHAR_BIT - 1 & n)

struct libgetln_context {
	char *buffer;
	char *rdptr;
	size_t size;
	size_t used;
	int file;
	struct flags {
		unsigned char eof:1;
		unsigned char verbose:1;
		unsigned char closefd:1;
	} flg;
};

struct libgetln_context *libgetln_new_context(void *file, size_t size, unsigned int opts) 
{
	struct libgetln_context *ctx;
	unsigned char verbose;

	verbose = !!(opts & LIBGETLN_OPT_ERR_VERBOSE);
	ctx = malloc(sizeof(struct libgetln_context));

	if (ctx == NULL) {
		if (verbose)
			perror("libgetln_new_context: malloc");

		return (NULL);
	}

	ctx->buffer = malloc(size * sizeof(char));

	if (ctx->buffer == NULL) {
		if (verbose)
			perror("libgetln_new_context: malloc");

		free(ctx);
		return (NULL);
	}

	if (opts & LIBGETLN_OPT_FILE_ISFD) {
		if (*(int *)file < 0) {
			errno = EBADF;

			if (verbose)
				perror("libgetln_new_context");

			goto out_free;
		}

		ctx->file = *(int *)file;
		ctx->flg.closefd = !!(opts & LIBGETLN_OPT_FILE_CLOSE);
	} else {
		ctx->file = open((char *)file, O_RDONLY);

		if (ctx->file < 0) {
			if (verbose)
				perror("libgetln_new_context: open");

			goto out_free;
		}

		ctx->flg.closefd = 1 ^ !!(opts & LIBGETLN_OPT_FILE_NOCLOSE);
	}		

	ctx->rdptr = ctx->buffer;
	ctx->size = size;	
	ctx->flg.eof = ctx->used = 0;
	ctx->flg.verbose = verbose;

	return (ctx);

out_free:
	free(ctx->buffer);
	free(ctx);

	return (NULL);
}

int libgetln_free_context(struct libgetln_context *ctx, int *fd)
{
	if (ctx->flg.closefd && close(ctx->file) < 0) {
		if (ctx->flg.verbose)
			perror("libgetln_free_context: close");

		if (fd != NULL)
			*fd = ctx->file;

		return (-1);
	}

	free(ctx->buffer);
	free(ctx);

	return (0);
}

void libgetln_reset_buffer(struct libgetln_context *ctx)
{
	ctx->rdptr = ctx->buffer;
	ctx->used = 0;
}

int libgetln_set_file(struct libgetln_context *ctx, int fd)
{
	if (fd < 0) {
		errno = EINVAL;

		if (ctx->flg.verbose)
			perror("libgetln_set_file");

		return (-1);
	}
	
	ctx->rdptr = ctx->buffer;
	ctx->used = 0;
	ctx->file = fd;

	return (0);
}

int libgetln_get_file(struct libgetln_context *ctx)
{
	return (ctx->file);
}

size_t libgetln_getline(struct libgetln_context *ctx, char **line, size_t *size)
{
	char *p, *newptr, *endptr = &ctx->buffer[ctx->used];
	size_t cnt, cplen, lsize, llen = 0;
	int err;

	if (ctx->flg.eof)
		return (0);

	for (;;) {
		if (ctx->used) {
			cplen = ctx->used;
			p = memchr(ctx->rdptr, '\n', ctx->used);

			if (p != NULL)
				cplen = ++p - ctx->rdptr;

			if (SIZE_MAX - llen < cplen + 1) {
				errno = EOVERFLOW;

				if (ctx->flg.verbose)
					perror("libgetln_getline:");

				return (SIZE_MAX);
			}

			lsize = llen + cplen + !llen;		

			if (*size < lsize) {
				if (!MSB(size_t, *size) && lsize < *size * 2)
					lsize = *size * 2;
		
				newptr = realloc(*line, lsize);

				if (newptr == NULL) {
					if (ctx->flg.verbose)
						perror("libgetln_getline: realloc");

					return (SIZE_MAX);
				}

				*line = newptr;
				*size = lsize;
			}

			memcpy(&(*line)[llen], ctx->rdptr, cplen);

			llen += cplen;
			(*line)[llen] = '\0';

			if (p == NULL || p == endptr) {
				ctx->rdptr = ctx->buffer;
				ctx->used = 0;
			} else {
				ctx->used = &ctx->rdptr[ctx->used] - p;
				ctx->rdptr = p;
				
				return (llen);				
			}
		} else {
			err = errno;
			cnt = read(ctx->file, ctx->buffer, ctx->size);

			if (cnt == (size_t)-1) {
				if (errno == EINTR || errno == EAGAIN) {
					errno = err;
					continue;
				}

				if (ctx->flg.verbose)
					perror("libgetln_getline: read");

				return (SIZE_MAX);
			}

			if (!cnt) {
				ctx->flg.eof = 1;
				return (llen);
			}

			ctx->used = cnt;
		}
	}

	return (0);
}
