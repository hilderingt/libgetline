#ifndef _LIBGETLINEP_H_
#define _LIBGETLINEP_H_

#define MSB(t, n) ((t)1 << (sizeof(t) * CHAR_BIT - 1) & n)

struct libgetln_context {
	char *dpos;
	size_t size;
	size_t used;
	int file;
	unsigned int state;
	char data[0];
};

#endif /* _LIBGETLINEP_H_ */
