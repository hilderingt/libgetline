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
	char *dpos;
	size_t size;
	size_t used;
	int file;
	struct flags {
		unsigned char eof:1;
		unsigned char verbose:1;
		unsigned char closefd:1;
	} flg;
	char data[0];
};

struct libgetln_context *libgetln_new_context(size_t size, int verbose) 
{
	struct libgetln_context *ctx;
	
	if (!size)
		size == LIBGETLN_SIZE_DEFAULT;

	ctx = malloc(sizeof(struct libgetln_context) + size * sizeof(char));

	if (ctx == NULL) {
		if (verbose)
			perror("libgetln_new_context: malloc");

		return (NULL);
	}
	
	ctx->flg.verbose = verbose;
	ctx->size = size;	
	ctx->flg.eof = 0;
	ctx->file = -1;

	return (ctx);
}

int libgetln_free_context(struct libgetln_context *ctx, int *fd)
{
	if (ctx == NULL) {
		errno = EINVAL;
		
		if (ctx->flg.verbose)
			perror("libgetln_free_context");
		
		return (-1);
	}
	
	if (ctx->file > 0 && 
		ctx->flg.closefd && close(ctx->file) < 0) {
		if (ctx->flg.verbose)
			perror("libgetln_free_context: close");

		if (fd != NULL)
			*fd = ctx->file;

		return (-1);
	}

	free(ctx);

	return (0);
}

int libgetln_reset_buffer(struct libgetln_context *ctx)
{
	if (ctx == NULL) {
		errno = EINVAL;
		
		if (ctx->flg.verbose)
			perror("libgetln_reset_context");
		
		return (-1);
	}
	
	ctx->dpos = ctx->data;
	ctx->used = 0;
	
	return (0);
}

int libgetln_set_file(struct libgetln_context *ctx, void *fd, unsigned int opts)
{
	if (ctx == NULL || fd == NULL) {
		errno = EINVAL;
		
		if (ctx->flg.verbose)
			perror("libgetln_set_file");
		
		return (-1);
	}
	
	if (opts & LIBGETLN_OPT_FILE_ISFD) {
		if (*(int *)file < 0) {
			errno = EBADF;

			if (ctx->flg.verbose)
				perror("libgetln_set_file");

			return (-1);
		}

		ctx->file = *(int *)file;
		ctx->flg.closefd = !!(opts & LIBGETLN_OPT_FILE_CLOSE);
	} else {
		int fd = open((char *)file, O_RDONLY);

		if (fd < 0) {
			if (ctx->flg.verbose)
				perror("libgetln_new_context: open");

			return (-1);
		}

		ctx->file = fd;
		ctx->flg.closefd = 1 ^ !!(opts & 
								LIBGETLN_OPT_FILE_NOCLOSE);
	}
	
	ctx->dpos = ctx->data;
	ctx->used = 0;

	return (0);
}

int libgetln_get_file(struct libgetln_context *ctx)
{
	if (ctx == NULL) {
		errno = EINVAL;
		
		if (ctx->flg.verbose)
			perror("libgetln_get_file");
		
		return (-1);
	}
	
	return (ctx->file);
}

size_t libgetln_getline(struct libgetln_context *ctx, char **line, size_t *size)
{
	char *p, *new, *end = &ctx->dpos[ctx->used];
	size_t cnt, cplen, lsize, llen = 0;
	int err;
	
	if (ctx == NULL || line == NULL) {
		errno = EINVAL;
		
		if (ctx->flg.verbose)
			perror("libgetln_getline");
		
		return (SIZE_MAX);
	}

	if (ctx->file < 0) {
		errno = EBADF;
		
		if (ctx->flg.verbose)
			perror("libgetln_getline");
		
		return (SIZE_MAX);
	}

	if (ctx->flg.eof)
		return (0);
	
	if (*line == NULL && *size) {
			*line = malloc(*size * sizeof(char));
			
			if (*line == NULL) {
				if (ctx->flg.verbose)
					perror("libgetln_getline: malloc");
				
				return (SIZE_MAX);
			}
	}

	for (;;) {
		if (ctx->used) {
			cplen = ctx->used;
			p = memchr(ctx->dpos, '\n', ctx->used);

			if (p != NULL)
				cplen = ++p - ctx->dpos;

			if (SIZE_MAX - llen < cplen + 1) {
				errno = EOVERFLOW;

				if (ctx->flg.verbose)
					perror("libgetln_getline:");

				return (SIZE_MAX);
			}

			lsize = llen + cplen + !llen;		

			if (*size < lsize) {
				if (!MSB(size_t, *size) && 
								lsize < *size * 2)
					lsize = *size * 2;
		
				new = realloc(*line, lsize);

				if (new == NULL) {
					if (ctx->flg.verbose)
						perror("libgetln_getline: realloc");

					return (SIZE_MAX);
				}

				*line = new;
				*size = lsize;
			}

			memcpy(&(*line)[llen], ctx->dpos, cplen);

			llen += cplen;
			(*line)[llen] = '\0';

			if (p == NULL || p == end) {
				ctx->dpos = ctx->data;
				ctx->used = 0;
			} else {
				ctx->used = &ctx->dpos[ctx->used] - p;
				ctx->dpos = p;
				
				return (llen);				
			}
		} else {
			err = errno;
			cnt = read(ctx->file, ctx->data, ctx->size);

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
