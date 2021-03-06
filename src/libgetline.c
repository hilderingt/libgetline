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

#include "libgetlineP.h"
#include "libgetline.h"

#include "config.h"

#if !HAVE_REALLOC

static void *realloc_int(void *ptr, size_t size)
{
	if (ptr == NULL)
		return (malloc(size));
		
	return (realloc(ptr, size));
}

#define realloc(p, s) realloc_int(p, s)

#endif /* HAVE_REALLOC */

struct libgetln_context *libgetln_new_context(size_t size, unsigned int state) 
{
	struct libgetln_context *ctx;

	if (!size)
		size = LIBGETLN_SIZE_DEFAULT;

	ctx = malloc(sizeof(struct libgetln_context) + size * sizeof(char));

	if (ctx == NULL) {
		if (LIBGETLN_VERBOSE(state))
			perror("libgetln_new_context: malloc");

		return (NULL);
	}

	ctx->state = state & (LIBGETLN_ST_VERBOSE |
				LIBGETLN_ST_NOBLANK |
				LIBGETLN_ST_NOCLOSE);
	ctx->size = size;
	ctx->file = -1;

	return (ctx);
}

int libgetln_free_context(struct libgetln_context *ctx, int * const fd)
{
	if (ctx == NULL) {
		errno = EINVAL;

		if (LIBGETLN_VERBOSE(ctx->state))
			perror("libgetln_free_context");

		return (-1);
	}

	if (ctx->file >= 0 &&
	!LIBGETLN_NOCLOSE(ctx->state) && close(ctx->file) < 0) {
		if (LIBGETLN_VERBOSE(ctx->state))
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

		if (LIBGETLN_VERBOSE(ctx->state))
			perror("libgetln_reset_context");

		return (-1);
	}

	ctx->dpos = ctx->data;
	ctx->used = 0;

	return (0);
}

int libgetln_set_file(struct libgetln_context *ctx, int fd)
{
	if (ctx == NULL) {
		errno = EINVAL;

		if (LIBGETLN_VERBOSE(ctx->state))
			perror("libgetln_set_file");

		return (-1);
	}

	if (ctx->file >= 0 &&
	!LIBGETLN_NOCLOSE(ctx->state) && close(ctx->file) < 0) {
		if (LIBGETLN_VERBOSE(ctx->state))
			perror("libgetln_set_file: close");

		return (-1);
	}

	if (fd < 0) {
		errno = EBADF;

		if (LIBGETLN_VERBOSE(ctx->state))
			perror("libgetln_set_file");

		return (-1);
	}

	ctx->file = fd;
	ctx->dpos = ctx->data;
	ctx->used = 0;

	return (0);
}

int libgetln_open_file(struct libgetln_context *ctx, char const *filename)
{
	int fd;

	if (ctx == NULL || filename == NULL) {
		errno = EINVAL;

		if (LIBGETLN_VERBOSE(ctx->state))
			perror("libgetln_set_file");

		return (-1);
	}

	if (ctx->file >= 0 &&
	!LIBGETLN_NOCLOSE(ctx->state) && close(ctx->file) < 0) {
		if (LIBGETLN_VERBOSE(ctx->state))
			perror("libgetln_open_file: close");

		return (-1);
	}

	fd = open((char *)filename, O_RDONLY);

	if (fd < 0) {
		if (LIBGETLN_VERBOSE(ctx->state))
			perror("libgetln_new_context: open");

		return (-1);
	}

	ctx->file = fd;
	ctx->dpos = ctx->data;
	ctx->used = 0;

	return (0);
}

int libgetln_get_file(struct libgetln_context const *ctx)
{
	if (ctx == NULL) {
		errno = EINVAL;

		if (LIBGETLN_VERBOSE(ctx->state))
			perror("libgetln_get_file");

		return (-1);
	}

	return (ctx->file);
}

int libgetln_set_state(struct libgetln_context *ctx, unsigned int state)
{
	if (ctx == NULL) {
		errno = EINVAL;

		if (LIBGETLN_VERBOSE(ctx->state))
			perror("libgetln_set_state");

		return (-1);
	}

	ctx->state |= (state & (LIBGETLN_ST_VERBOSE |
				LIBGETLN_ST_NOBLANK |
				LIBGETLN_ST_NOCLOSE));

	return (0);
}

int libgetln_get_state(struct libgetln_context *ctx, unsigned int *state)
{
	if (ctx == NULL || state == NULL) {
		errno = EINVAL;

		if (LIBGETLN_VERBOSE(ctx->state))
			perror("libgetln_get_state");

		return (-1);
	}
		
	*state = ctx->state;
	return (0);
}

int libgetln_clear_state(struct libgetln_context *ctx, unsigned int state)
{
	if (ctx == NULL) {
		errno = EINVAL;

		if (LIBGETLN_VERBOSE(ctx->state))
			perror("libgetln_clear_state");

		return (-1);
	}

	ctx->state &= (~state & (LIBGETLN_ST_VERBOSE |
				LIBGETLN_ST_NOBLANK |
				LIBGETLN_ST_NOCLOSE)) | 
				LIBGETLN_ST_EOF;

	return (0);
}

size_t libgetln_getline(struct libgetln_context *ctx, char **line, size_t * const size)
{
	size_t need, cplen, lsize, llen = 0;
	char *p, *new, *end;
	ssize_t cnt;
	int err;

	if (ctx == NULL || line == NULL) {
		errno = EINVAL;

		if (LIBGETLN_VERBOSE(ctx->state))
			perror("libgetln_getline");

		return (SIZE_MAX);
	}

	if (ctx->file < 0) {
		errno = EBADF;

		if (LIBGETLN_VERBOSE(ctx->state))
			perror("libgetln_getline");

		return (SIZE_MAX);
	}

	if (LIBGETLN_EOF(ctx->state))
		return (0);

	if (*line == NULL && *size) {
		*line = malloc(*size * sizeof(char));

		if (*line == NULL) {
			if (LIBGETLN_VERBOSE(ctx->state))
				perror("libgetln_getline: malloc");

			return (SIZE_MAX);
		}
	}
	
	end = ctx->dpos + ctx->used;

	for (;;) {
		if (ctx->used) {
			cplen = ctx->used;
			p = memchr(ctx->dpos, '\n', ctx->used);

			if (p != NULL)
				cplen = ++p - ctx->dpos;

			if (cplen > 1 || p == NULL ||
			!LIBGETLN_NOBLANK(ctx->state)) {
				need = cplen + !llen;

				if (~lsize < need) {
					errno = EOVERFLOW;

					if (LIBGETLN_VERBOSE(ctx->state))
						perror("libgetln_getline:");

					return (SIZE_MAX);
				}

				lsize = lsize + need;

				if (*size < lsize) {
					if (lsize < *size * 2 && !MSB(size_t, *size))
						lsize = *size * 2;

					new = realloc(*line, lsize);

					if (new == NULL) {
						if (LIBGETLN_VERBOSE(ctx->state))
							perror("libgetln_getline: realloc");

						return (SIZE_MAX);
					}

					*line = new;
					*size = lsize;
				}

				memcpy(*line + llen, ctx->dpos, cplen);

				llen += cplen;
				(*line)[llen] = '\0';
			}

			if (p == NULL || p == end) {
				ctx->dpos = ctx->data;
				ctx->used = 0;
				
				if (p == end)
					return (llen);
			} else {
				ctx->used = &ctx->dpos[ctx->used] - p;
				ctx->dpos = p;

				return (llen);
			}
		} else {
			err = errno;
			cnt = read(ctx->file, ctx->data, ctx->size);

			if (cnt < 0) {
				if (errno == EINTR || errno == EAGAIN) {
					errno = err;
					continue;
				}

				if (LIBGETLN_VERBOSE(ctx->state))
					perror("libgetln_getline: read");

				return (SIZE_MAX);
			}

			if (!cnt) {
				ctx->state |= LIBGETLN_ST_EOF;
				return (llen);
			}

			end = &ctx->data[cnt];
			ctx->used = cnt;
		}
	}

	return (0);
}
