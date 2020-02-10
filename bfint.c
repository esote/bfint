#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUF_LEN	(1 << 13)
#define MEM_LEN	(1 << 16)

#if MEM_LEN < 30000
#warn Adjusted memory capacity makes interpreter unportable.
#endif

struct pair {
	off_t start, end;
};

static struct pair *	precomp_pairs(int, size_t *);

int
main(int argc, char *argv[])
{
	char memory[MEM_LEN];
	char buf[BUF_LEN];
	struct pair *pairs;
	size_t c, pn;
	off_t off;
	ssize_t r, i;
	int fd;
	char *dp;

	pn = 0;
	off = 0;
	fd = 0;
	dp = memory;

	if (argc <= 1) {
		(void)fprintf(stderr, "usage: %s file\n", argv[0]);
		return 1;
	}

	if ((fd = open(argv[1], O_RDONLY)) == -1) {
		err(1, "open");
	}

	if ((pairs = precomp_pairs(fd, &pn)) == NULL && pn != 0) {
		err(1, "precomp_pairs");
	}

	if (lseek(fd, 0, SEEK_SET) == -1) {
		err(1, "lseek");
	}

	(void)memset(memory, 0, sizeof(memory));

	while ((r = read(fd, buf, BUF_LEN)) > 0) {
		for (i = 0; i < r; i++) {
			switch (buf[i]) {
			case '>':
				if (dp - memory == MEM_LEN) {
					(void)raise(SIGSEGV);
					errx(1, "SIGSEGV");
				}
				++dp;
				break;
			case '<':
				if (dp == memory) {
					(void)raise(SIGSEGV);
					errx(1, "SIGSEGV");
				}
				--dp;
				break;
			case '+':
				(*dp)++;
				break;
			case '-':
				(*dp)--;
				break;
			case '.':
				if (write(1, dp, 1) <= 0) {
					err(1, "write");
				}
				break;
			case ',':
				switch (read(0, dp, 1)) {
				case 0:
					/* stdin EOF */
					r = 0;
					break;
				case -1:
					err(1, "read");
				}
				break;
			case '[':
				if (*dp != '\0') {
					break;
				}

				for (c = 0; c < pn && pairs[c].start != off + i; c++);

				if (pairs[c].end >= off && pairs[c].end < off +r) {
					i = pairs[c].end - off;
					break;
				}

				if ((off = lseek(fd, pairs[c].end, SEEK_SET)) == -1) {
					err(1, "lseek");
				}

				i = r = 0;
				break;
			case ']':
				if (*dp == '\0') {
					break;
				}

				for (c = 0; c < pn && pairs[c].end != off + i; c++);

				if (pairs[c].start >= off && pairs[c].start < off +r) {
					i = pairs[c].start - off;
					break;
				}

				if ((off = lseek(fd, pairs[c].start, SEEK_SET)) == -1) {
					err(1, "lseek");
				}

				i = r = 0;
				break;
			}
		}
		off += r;
	}

	free(pairs);

	if (r == -1) {
		err(1, "read");
	}

	if (close(fd) == -1) {
		err(1, "close");
	}

	return 0;
}

static struct pair *
precomp_pairs(int fd, size_t *n)
{
	char buf[BUF_LEN];
	struct pair *pairs;
	off_t *stack;
	size_t sn;
	ssize_t r, i;
	off_t off;

	pairs = NULL;
	stack = NULL;
	sn = 0;
	off = 0;
	*n = 0;

	while ((r = read(fd, buf, BUF_LEN)) > 0) {
		for (i = 0; i < r; i++) {
			if (buf[i] == '[') {
				sn++;
				stack = realloc(stack, sn * sizeof(size_t));
				if (stack == NULL) {
					return NULL;
				}
				stack[sn - 1] = off + (off_t)i;
			} else if (buf[i] == ']') {
				if (sn == 0) {
					errno = EINVAL;
					return NULL;
				}
				sn--;
				(*n)++;
				pairs = realloc(pairs, *n * sizeof(struct pair));
				if (pairs == NULL) {
					return NULL;
				}
				pairs[*n - 1].start = stack[sn];
				pairs[*n - 1].end = off + (off_t)i;
			}
		}
		off += r;
	}

	free(stack);

	if (r == -1) {
		return NULL;
	}

	if (sn != 0) {
		errno = EINVAL;
		return NULL;
	}

	return pairs;
}
