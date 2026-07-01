CC ?= cc
CSTD ?= -std=c99
WARN_CFLAGS ?= -Wall -Wextra -Wpedantic -Werror
CPPFLAGS ?=
CFLAGS ?=
LDFLAGS ?=
LDLIBS ?=
SRC := ../src/mtimer.c

.PHONY: all clean

all: test_all
	./test_all

test_all: test_all.c test_harness.h $(SRC) ../include/mtimer.h ../include/mtimer_config.h
	$(CC) $(CPPFLAGS) -I../include $(CSTD) $(WARN_CFLAGS) $(CFLAGS) $(SRC) test_all.c $(LDFLAGS) $(LDLIBS) -o $@

clean:
	$(RM) test_all test_all.exe
