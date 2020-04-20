#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

struct readln_context {
	unsigned char *data;
	unsigned char *dpos;
	size_t size;
	size_t llen;
	int file;
	_Bool eof;
	_Bool verbose;
};

struct readln_context *readln_new_context(char const *filename, size_t size, int verbose) 
{
	struct readln_context *ctx;
	int fd;

	fd = open(filename, O_RDONLY);

	if (fd < 0) {
		if (verbose)
			perror("readln_new_context: open");

		return (NULL);
	}

	ctx = malloc(sizeof(struct readln_context));

	if (ctx == NULL) {
		if (verbose)
			perror("readln_new_context: malloc");

		goto out_close;
	}

	ctx->data = malloc(size * sizeof(char));

	if (ctx->data == NULL) {
		if (verbose)
			perror("readln_new_context: malloc");

		goto out_free_ctx;
		return (NULL);
	}

	ctx->dpos = ctx->data;
	ctx->size = size;
	ctx->file = fd;
	ctx->llen = 0;
	ctx->eof = false;
	ctx->verbose = !!verbose;

	return (ctx);

out_free_ctx:
	free(ctx);

out_close:
	close(fd);

	return (NULL);
}

int readln_free_context(struct readln_context *ctx, int *fd)
{
	int ret = close(ctx->file);

	if (ret < 0) {
		if (ctx->verbose)
			perror("readln_free_context: close");

		if (fd != NULL)
			*fd = ctx->file;

		return (-1);
	}

	free(ctx->data);
	free(ctx);

	return (0);
}

ssize_t readln(struct readln_context *ctx, unsigned char **line, size_t *size)
{
	ssize_t count, avail, cplen, llen = 0, lsize = *size;
	unsigned char *new, *endp = NULL;

	do {
		if (ctx->llen) {
			endp = memchr(ctx->dpos, '\n', ctx->llen);

			if (endp != NULL)
				cplen = endp++ - ctx->dpos;
			else
				cplen = ctx->llen;

			if (++cplen > 1) {
				avail = lsize - llen;

				if (avail < 0 || cplen > avail) {
					do {
						avail += lsize;
						lsize += lsize;
					} while (avail < cplen);
		
					new = realloc(*line, lsize);

					if (new == NULL) {
						if (ctx->verbose)
							perror("readln: realloc");

						return (-1);
					}

					*line = new;
					*size = lsize;
				}

				memcpy(&(*line)[llen], ctx->dpos, cplen - 1);

				llen += cplen;
				(*line)[llen - 1] = '\0';
			}

			if (endp == NULL || endp == &ctx->data[lsize]) {
				ctx->dpos = ctx->data;
				ctx->llen = 0;
			} else {
				ctx->llen = &ctx->dpos[ctx->llen] - endp;
				ctx->dpos = endp;				
			}
		} else if (!ctx->eof) {
			count = read(ctx->file, ctx->data, lsize);

			if (count < 0) {
				if (errno == EINTR || errno == EAGAIN)
					continue;

				if (ctx->verbose)
					perror("readln: read");

				return (-1);
			}

			if (!count) {
				ctx->eof = true;
				return (llen);
			}

			ctx->llen = count;
		} else
			break;
	} while (endp == NULL || cplen == 1);

	return (llen);
}
