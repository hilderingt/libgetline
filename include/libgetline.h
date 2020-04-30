#ifndef _LIBGETLINE_H_
#define _LIBGETLINE_H_

#define LIBGETLN_VERBOSE 1u
#define LIBGETLN_NOBLANK 2u
#define LIBGETLN_NOCLOSE 4u

#define LIBGETLN_SIZE_DEFAULT 4096u

struct libgetln_context;

extern struct libgetln_context *libgetln_new_context(size_t, unsigned int);
extern int libgetln_free_context(struct libgetln_context *, int *);
extern int libgetln_open_file(struct libgetln_context *, char *);
extern int libgetln_set_file(struct libgetln_context *, int);
extern int libgetln_get_file(struct libgetln_context *);
extern int libgetln_set_state(struct libgetln_context *, unsigned int);
extern int libgetln_clear_state(struct libgetln_context *, unsigned int);
extern int libgetln_reset_buffer(struct libgetln_context *);
extern size_t libgetln_getline(struct libgetln_context *, char **, size_t *);

#endif /* _LIBGETLINE_H_ */
