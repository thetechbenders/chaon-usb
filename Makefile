CC ?= cc
CFLAGS ?= -std=c11 -O1 -g -Wall -Wextra -Wpedantic -Werror
CPPFLAGS += -Imain
NATIVE_SRCS := main/chaon_script.c main/chaon_hid.c main/chaon_vm.c tests/test_native.c
VISUAL_SRCS := main/chaon_status.c main/chaon_screen.c tests/test_visual.c
NATIVE_BIN := tests/chaon_tests
VISUAL_BIN := tests/chaon_visual_tests
CONTROL_SRCS := main/chaon_control.c tests/test_control.c
CONTROL_BIN := tests/chaon_control_tests

.PHONY: test clean
test: $(NATIVE_BIN) $(VISUAL_BIN) $(CONTROL_BIN)
	./$(NATIVE_BIN)
	./$(VISUAL_BIN)
	./$(CONTROL_BIN)

$(NATIVE_BIN): $(NATIVE_SRCS) main/chaon_script.h main/chaon_hid.h main/chaon_vm.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(NATIVE_SRCS) -o $(NATIVE_BIN)

$(VISUAL_BIN): $(VISUAL_SRCS) main/chaon_status.h main/chaon_screen.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(VISUAL_SRCS) -o $(VISUAL_BIN)

$(CONTROL_BIN): $(CONTROL_SRCS) main/chaon_control.h main/chaon_status.h main/chaon_vm.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(CONTROL_SRCS) -o $(CONTROL_BIN)

clean:
	rm -f $(NATIVE_BIN) $(VISUAL_BIN) $(CONTROL_BIN)
