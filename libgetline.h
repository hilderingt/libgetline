#ifndef _LIBGETLINE_H_
#define _LIBGETLINE_H_

struct libgetln_context;

extern struct libgetln_context *libgetln_new_context(char const *, size_t , int);
extern int libgetln_free_context(struct libgetln_context *, int *);
extern int libgetln_file_seek(struct libgetln_context *, off_t, int);
extern size_t libgetln_getline(struct libgetln_context *, char **, size_t *);

#endif /* _LIBGETLINE_H_ */
