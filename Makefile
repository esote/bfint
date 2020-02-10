PROG=	bfint
SRCS=	bfint.c

CFLAGS=		-O2 -fstack-protector -D_FORTIFY_SOURCE=2 -pie -fPIE
LDFLAGS=	-Wl,-z,now -Wl,-z,relro
DFLAGS=		-ansi -pedantic -g -Wall -Wextra -Wconversion -Wundef

$(PROG): $(SRCS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(PROG).out $(SRCS)

debug: $(SRCS)
	$(CC) $(DFLAGS) -o $(PROG).out $(SRCS)

clean:
	rm -f $(PROG).out
