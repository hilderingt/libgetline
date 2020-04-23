#ifndef _LIBGETLINE_H_
#define _LIBGETLINE_H_

enum {
	LIBGETLN_FILE_FD     = 1,
	LIBGETLN_ERR_VERBOSE = 2
};

struct libgetln_context;

extern struct libgetln_context *libgetln_new_context(void *, size_t, unsigned int);
extern int libgetln_free_context(struct libgetln_context *, int *);
extern int libgetln_set_file(struct libgetln_context *, int);
extern int libgetln_get_file(struct libgetln_context *);
extern void libgetln_reset_buffer(struct libgetln_context *);
extern size_t libgetln_getline(struct libgetln_context *, char **, size_t *);

#endif /* _LIBGETLINE_H_ */
