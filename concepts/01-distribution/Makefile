CFLAGS = -O2 -Wall -Wextra -std=c11

build/distribution: distribution.c
	mkdir -p build
	$(CC) $(CFLAGS) -o $@ $<

run: build/distribution
	./build/distribution

clean:
	rm -rf build

.PHONY: run clean
