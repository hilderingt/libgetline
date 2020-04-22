#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

struct libgetln_context {
	char *buffer;
	char *bufptr;
	size_t bufsz;
	size_t buflen;
	int file;
	_Bool eof;
	_Bool verbose;
};

struct libgetln_context *libgetln_new_context(char const *filename, size_t size, int verbose) 
{
	struct libgetln_context *ctx;
	int fd;

	fd = open(filename, O_RDONLY);

	if (fd < 0) {
		if (verbose)
			perror("libgetln_new_context: open");

		return (NULL);
	}

	ctx = malloc(sizeof(struct libgetln_context));

	if (ctx == NULL) {
		if (verbose)
			perror("libgetln_new_context: malloc");

		goto out_close;
	}

	ctx->buffer = malloc(size * sizeof(char));

	if (ctx->buffer == NULL) {
		if (verbose)
			perror("libgetln_new_context: malloc");

		goto out_free_ctx;
		return (NULL);
	}

	ctx->bufptr = ctx->buffer;
	ctx->bufsz = size;
	ctx->file = fd;
	ctx->buflen = 0;
	ctx->eof = false;
	ctx->verbose = !!verbose;

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

int libgetln_file_seek(struct libgetln_context *ctx, off_t offset, int whence)
{
	off_t ret = lseek(ctx->file, offset, whence);

	if (ret < 0) {
		if (ctx->verbose)
			perror("libgetln_file_seek: lseek");

		return (-1);
	}

	ctx->bufptr = ctx->buffer;
	ctx->buflen = 0;

	return (0);
}

size_t libgetln_getline(struct libgetln_context *ctx, char **line, size_t *size)
{
	size_t avail, cplen, llen = 0, lsize = *size;
	char *new, *endp = NULL;
	ssize_t count;

	do {
		if (ctx->buflen) {
			endp = memchr(ctx->bufptr, '\n', ctx->buflen);

			if (endp != NULL)
				cplen = endp++ - ctx->bufptr;
			else
				cplen = ctx->buflen;

			if (++cplen > 1) {
				avail = lsize - llen;

				if (cplen > avail) {
					lsize += cplen - avail;		
					new = realloc(*line, lsize);

					if (new == NULL) {
						if (ctx->verbose)
							perror("readln: realloc");

						return (SIZE_MAX);
					}

					*line = new;
					*size = lsize;
				}

				memcpy(&(*line)[llen], ctx->bufptr, cplen - 1);

				llen += cplen;
				(*line)[llen - 1] = '\0';
			}

			if (endp == NULL || endp == &ctx->buffer[ctx->bufsz]) {
				ctx->bufptr = ctx->buffer;
				ctx->buflen = 0;
			} else {
				ctx->buflen = &ctx->bufptr[ctx->buflen] - endp;
				ctx->bufptr = endp;				
			}
		} else if (!ctx->eof) {
			count = read(ctx->file, ctx->buffer, ctx->bufsz);

			if (count < 0) {
				if (errno == EINTR || errno == EAGAIN)
					continue;

				if (ctx->verbose)
					perror("readln: read");

				return (SIZE_MAX);
			}

			if (!count) {
				ctx->eof = true;
				return (llen);
			}

			ctx->buflen = count;
		} else
			break;
	} while (endp == NULL || cplen == 1);

	return (llen);
}
