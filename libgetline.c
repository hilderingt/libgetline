#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "libgetline.h"

struct libgetln_context {
	char *buffer;
	char *rdptr;
	size_t bufsz;
	size_t buflen;
	int file;
	unsigned char eof:1;
	unsigned char verbose:1;
};

struct libgetln_context *libgetln_new_context(void *file, size_t size, unsigned int opts) 
{
	struct libgetln_context *ctx;
	unsigned char verbose;
	int fd;

	verbose = !!(opts & LIBGETLN_ERR_VERBOSE);

	if (opts & LIBGETLN_FILE_FD) {
		if (*(int *)file < 0) {
			errno = EBADF;

			if (verbose)
				perror("libgetln_new_context");
		}

		fd = *(int *)file;
	} else {
		fd = open((char *)file, O_RDONLY);

		if (fd < 0) {
			if (verbose)
				perror("libgetln_new_context: open");

			return (NULL);
		}
	}		

	ctx = malloc(sizeof(struct libgetln_context));

	if (ctx == NULL) {
		if (ctx->verbose)
			perror("libgetln_new_context: malloc");

		goto out_close;
	}

	ctx->buffer = malloc(size * sizeof(char));

	if (ctx->buffer == NULL) {
		if (ctx->verbose)
			perror("libgetln_new_context: malloc");

		goto out_free_ctx;
		return (NULL);
	}

	ctx->rdptr = ctx->buffer;
	ctx->bufsz = size;
	ctx->file = fd;
	ctx->eof = ctx->buflen = 0;
	ctx->verbose = verbose;

	return (ctx);

out_free_ctx:
	free(ctx);

out_close:
	close(fd);

	return (NULL);
}

int libgetln_free_context(struct libgetln_context *ctx, int *fd)
{
	int ret = close(ctx->file);

	if (ret < 0) {
		if (ctx->verbose)
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
	ctx->buflen = 0;
}

int libgetln_set_file(struct libgetln_context *ctx, int fd)
{
	if (fd < 0) {
		errno = EINVAL;

		if (ctx->verbose)
			perror("libgetln_set_file");

		return (-1);
	}
	
	ctx->rdptr = ctx->buffer;
	ctx->buflen = 0;
	ctx->file = fd;

	return (0);
}

int libgetln_get_file(struct libgetln_context *ctx)
{
	return (ctx->file);
}

size_t libgetln_getline(struct libgetln_context *ctx, char **line, size_t *size)
{
	size_t need, avail, cplen, llen = 0, lsize = *size;
	char *new, *endp = NULL;
	ssize_t count;

	for (;;) {
		if (ctx->buflen) {
			endp = memchr(ctx->rdptr, '\n', ctx->buflen);

			if (endp != NULL)
				cplen = ++endp - ctx->rdptr;
			else
				cplen = ctx->buflen;

			need = llen + cplen + 1;		

			if (lsize < need) {
				if (SIZE_MAX - lsize < need - lsize) {
					errno = EOVERFLOW;

					if (ctx->verbose)
						perror("libgetln_getline:");

					return (SIZE_MAX);
				}

				lsize += need;		
				new = realloc(*line, lsize);

				if (new == NULL) {
					if (ctx->verbose)
						perror("libgetln_getline: realloc");

					return (SIZE_MAX);
				}

				*line = new;
				*size = lsize;
			}

			memcpy(&(*line)[llen], ctx->rdptr, cplen);

			llen += cplen;
			(*line)[llen] = '\0';

			if (endp == NULL || endp == &ctx->buffer[ctx->bufsz]) {
				ctx->rdptr = ctx->buffer;
				ctx->buflen = 0;

				if (ctx->eof)
					return (llen);
			} else {
				ctx->buflen = &ctx->rdptr[ctx->buflen] - endp;
				ctx->rdptr = endp;
				
				return (llen);				
			}
		} else if (!ctx->eof) {
			count = read(ctx->file, ctx->buffer, ctx->bufsz);

			if (count < 0) {
				if (errno == EINTR || errno == EAGAIN)
					continue;

				if (ctx->verbose)
					perror("libgetln_getline: read");

				return (SIZE_MAX);
			}

			if (!count) {
				ctx->eof = 1;
				return (llen);
			}

			ctx->buflen = count;
		} else
			break;
	}

	return (0);
}
