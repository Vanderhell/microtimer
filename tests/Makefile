CC      ?= gcc
CFLAGS  := -std=c99 -Wall -Wextra -Wpedantic -Werror -I../include -I../../microtest/include
SRC     := ../src/mtimer.c

.PHONY: all clean

all: test_all
	@./test_all

test_all: test_all.c $(SRC) ../include/mtimer.h
	$(CC) $(CFLAGS) $(SRC) test_all.c -lm -o $@

clean:
	rm -f test_all
