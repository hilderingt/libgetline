#ifndef _LIBGETLINE_H_
#define _LIBGETLINE_H_

#define LIBGETLN_VERBOSE(s) (s & LIBGETLN_ST_VERBOSE)
#define LIBGETLN_NOBLANK(s) (s & LIBGETLN_ST_NOBLANK)
#define LIBGETLN_NOCLOSE(s) (s & LIBGETLN_ST_NOCLOSE)
#define LIBGETLN_EOF(s)     (s & LIBGETLN_ST_EOF)

#define LIBGETLN_ST_VERBOSE ((unsigned int)1)
#define LIBGETLN_ST_NOBLANK ((unsigned int)2)
#define LIBGETLN_ST_NOCLOSE ((unsigned int)4)
#define LIBGETLN_ST_EOF     ((unsigned int)8)

#define LIBGETLN_SIZE_DEFAULT ((size_t)4096)

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
