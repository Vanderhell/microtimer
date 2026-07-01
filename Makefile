.PHONY: all test clean

all: test

test:
	$(MAKE) -C tests CC="$(CC)" CPPFLAGS="$(CPPFLAGS)" CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)"

clean:
	$(MAKE) -C tests clean
