CC ?= cc
CFLAGS ?= -std=c11 -O1 -g -Wall -Wextra -Wpedantic -Werror
CPPFLAGS += -Imain
NATIVE_SRCS := main/chaon_script.c main/chaon_hid.c main/chaon_vm.c tests/test_native.c
NATIVE_BIN := tests/chaon_tests

.PHONY: test clean
test: $(NATIVE_BIN)
	./$(NATIVE_BIN)

$(NATIVE_BIN): $(NATIVE_SRCS) main/chaon_script.h main/chaon_hid.h main/chaon_vm.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(NATIVE_SRCS) -o $(NATIVE_BIN)

clean:
	rm -f $(NATIVE_BIN)
