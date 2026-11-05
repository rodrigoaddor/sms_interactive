CC ?= cc
NUT_SOURCE ?= .cache/nut
.PHONY: test integrate
test:
	mkdir -p build
	$(CC) -std=c99 -Wall -Wextra -Werror -fsanitize=address,undefined -Idrivers tests/protocol.c -o build/test-protocol
	./build/test-protocol
integrate:
	python3 tools/integrate.py $(NUT_SOURCE)
