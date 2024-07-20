sigexec_cflags:=-std=c11 -Wall -Wextra -pedantic -g -D_POSIX_C_SOURCE $(CFLAGS)

all: sigexec

sigexec: sigexec.c
	$(CC) $(sigexec_cflags) $< -o $@

clean:
	rm -f sigexec

.PHONY: all clean
